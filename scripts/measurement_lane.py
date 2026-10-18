#!/usr/bin/env python3
"""Measurement lanes: CPU pinning, memory binding and per-timing context rows.

Implements papers/2026-10-17-heterogeneous-placement-measurement.md §7.7-7.10
and §7.11 for every driver that times a run. A lane is one NUMA node: some of
its logical CPUs, its memory and optionally its local GPU. Each timed process
runs under `numactl --physcpubind --membind` for its lane. Each timing writes
one context row: the lane, and the competing load around it. That is load
average, foreign CPU (host busy time from /proc/stat minus our own process
tree, unthresholded, plus the count and CPU of foreign processes at or above
5%), delivered clock and CPU share on the lane CPUs, free memory per node,
memory pressure, governor, SMT, AutoNUMA and THP state, and GPU utilisation
with own and foreign compute-process counts. Foreign processes are recorded
only as counts and CPU, never by name or id; our own tree is named. Linux
fleet only. Build the probe with scripts/build_measurement_probe.sh.

Owned runs use the same mechanism from the command line, for example

    python3 scripts/measurement_lane.py run --node 0 --workers 12 \\
        --context out/context.tsv --label cpu12 -- brepgpu-adaptive-model ...

With `--gpu G` the lane is GPU G's node and the command sees only that GPU,
as CUDA device 0, enumerated in PCI bus order.
"""
import argparse
import contextlib
import csv
from dataclasses import dataclass
import datetime
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
SYSFS = Path('/sys/devices/system')
CLOCK_PROBE = Path(os.environ.get('BREPGPU_DELIVERED_CLOCK',
                                  ROOT/'build/measurement/brepgpu-delivered-clock'))
CLOCK_MILLISECONDS = 100
OTHERS_THRESHOLD_PERCENT = 5.0
REEXEC_MARK = 'BREPGPU_LANE_REEXEC'
# G8 §7.10 quiet rule, applied to foreign load only: foreign CPU (host busy
# minus our own tree) <= 150% at every check and over the whole settle window,
# and no foreign GPU compute process. The 1-minute load average cannot tell
# our own just-finished arm from other users, so it is recorded, not used; a
# run's own foreign CPU must also meet the bound for the run to count quiet.
QUIET_LOAD, QUIET_FOREIGN_PERCENT = 4.5, 150.0
CONTEXT_COLUMNS = (
    'context_id', 'utc_before', 'utc_after', 'scope', 'lane_node', 'lane_cpus', 'lane_gpu',
    'recorder_cpus', 'recorder_memory_policy', 'window_seconds',
    'foreign_cpu_percent', 'foreign_processes_over_threshold', 'foreign_over_threshold_cpu_percent',
    'own_tree', 'loadavg_before', 'loadavg_after',
    'clock_ghz_mean_min_max_before', 'clock_ghz_mean_min_max_after',
    'cpu_share_mean_min_before', 'cpu_share_mean_min_after',
    'governor', 'smt', 'autonuma', 'thp',
    'node_free_mib_before', 'node_free_mib_after',
    'memory_psi_some_avg10_before', 'memory_psi_some_avg10_after',
    'gpus_before', 'gpus_after', 'gpu_compute_own_foreign_before', 'gpu_compute_own_foreign_after',
    'capture_errors', 'labels')


def require(value, message):
    if not value:
        raise ValueError(message)


def parse_cpus(text):
    cpus = []
    for part in text.strip().split(','):
        if not part:
            continue
        low, _, high = part.partition('-')
        cpus.extend(range(int(low), int(high or low)+1))
    return tuple(cpus)


def format_cpus(cpus):
    return ','.join(map(str, cpus))


@dataclass(frozen=True)
class Core:
    package: int
    core: int
    threads: tuple


@dataclass(frozen=True)
class Lane:
    node: int
    cpus: tuple
    gpu: object = None

    def describe(self):
        return dict(lane_node=self.node, lane_cpus=format_cpus(self.cpus),
                    lane_gpu='-' if self.gpu is None else self.gpu)


def read_topology(root=SYSFS):
    """node -> physical cores, each with its logical CPUs, from sysfs.

    Cores are grouped by (physical_package_id, core_id) and ordered by their
    first logical CPU."""
    nodes = {}
    try:
        for directory in sorted((root/'node').glob('node[0-9]*'), key=lambda p: int(p.name[4:])):
            cores = {}
            for cpu in parse_cpus((directory/'cpulist').read_text()):
                topology = root/'cpu'/f'cpu{cpu}'/'topology'
                key = (int((topology/'physical_package_id').read_text()), int((topology/'core_id').read_text()))
                cores.setdefault(key, []).append(cpu)
            if cores:
                nodes[int(directory.name[4:])] = tuple(sorted(
                    (Core(package, core, tuple(sorted(threads))) for (package, core), threads in cores.items()),
                    key=lambda c: c.threads[0]))
    except (OSError, ValueError) as error:
        raise ValueError(f'cannot read CPU topology under {root}: {error}') from error
    require(nodes, 'no NUMA node with CPUs found in sysfs')
    return nodes


def lane_order(topology, node):
    """One logical CPU per physical core first, then each further SMT sibling."""
    require(node in topology, f'NUMA node {node} has no CPUs')
    cores = topology[node]
    depth = max(len(c.threads) for c in cores)
    return tuple(c.threads[d] for d in range(depth) for c in cores if d < len(c.threads))


def lane_cpus(topology, node, workers):
    """CPUs for `workers` single-threaded workers on one node (G8 §7.11).

    Up to one worker per physical core, each on its own core; beyond that the
    SMT siblings, in core order. More workers than logical CPUs is refused."""
    order = lane_order(topology, node)
    require(1 <= workers <= len(order),
            f'node {node} has {len(order)} logical CPUs; {workers} single-threaded workers do not fit')
    return order[:workers]


def allowed_cpus():
    return frozenset(os.sched_getaffinity(0))


def node_lane(topology, node, allowed=None):
    """Every CPU of `node` in this process affinity, in lane order."""
    allowed = allowed_cpus() if allowed is None else allowed
    cpus = tuple(c for c in lane_order(topology, node) if c in allowed)
    require(cpus, f'no CPU of node {node} is in this process affinity')
    return Lane(node, cpus)


def make_lane(node, workers=None, gpu=None, topology=None, allowed=None):
    """A lane on `node`: `workers` CPUs in lane order, or every CPU of the node.

    Every CPU must already be in this process's affinity; a driver never
    escapes the CPU set its caller gave it."""
    topology = read_topology() if topology is None else topology
    allowed = allowed_cpus() if allowed is None else frozenset(allowed)
    if gpu is not None:
        local = gpu_node(gpu)
        require(node is None or node == local,
                f'GPU {gpu} is on node {local}; a lane drives its GPU from its own node')
        node = local
    require(node is not None, 'a lane needs a NUMA node or a GPU')
    cpus = lane_order(topology, node) if workers is None else lane_cpus(topology, node, workers)
    missing = sorted(set(cpus)-allowed)
    require(not missing, f'lane CPUs {format_cpus(missing)} are outside this process affinity '
                         f'{format_cpus(sorted(allowed))}')
    return Lane(node, tuple(cpus), gpu)


def sysfs_pci_address(bus_id):
    """nvidia-smi's 'DDDDDDDD:BB:DD.F' as sysfs 'dddd:bb:dd.f'."""
    domain, bus, slot = bus_id.strip().lower().split(':')
    device, function = slot.split('.')
    return f'{int(domain, 16):04x}:{int(bus, 16):02x}:{int(device, 16):02x}.{int(function, 16):x}'


def gpu_node(index):
    """NUMA node of nvidia-smi GPU `index`, from its PCI device in sysfs."""
    rows = subprocess.run(('nvidia-smi', '--query-gpu=index,pci.bus_id', '--format=csv,noheader'),
                          check=True, capture_output=True, text=True, timeout=30).stdout.splitlines()
    for row in rows:
        number, bus = (x.strip() for x in row.split(','))
        if int(number) == int(index):
            node = int(Path('/sys/bus/pci/devices', sysfs_pci_address(bus), 'numa_node').read_text())
            require(node >= 0, f'GPU {index} reports no NUMA node')
            return node
    raise ValueError(f'no GPU {index}')


def numactl():
    path = shutil.which('numactl')
    require(path, 'numactl is required for lane binding')
    return path


def bound(command, lane):
    """The command run on the lane's CPUs with memory bound to its node."""
    return (numactl(), f'--physcpubind={format_cpus(lane.cpus)}', f'--membind={lane.node}',
            *map(str, command))


def lane_environment(lane, base=None):
    environment = dict(os.environ if base is None else base)
    if lane.gpu is not None:
        environment.update(CUDA_DEVICE_ORDER='PCI_BUS_ID', CUDA_VISIBLE_DEVICES=str(lane.gpu))
    return environment


def memory_policy(pid='self'):
    """Task memory policy as /proc/PID/numa_maps prints it, e.g. 'bind:1'."""
    with open(f'/proc/{pid}/numa_maps') as stream:
        return stream.readline().split()[1]


def process_binding(pid='self'):
    cpus = os.sched_getaffinity(0 if pid == 'self' else int(pid))
    return tuple(sorted(cpus)), memory_policy(pid)


def is_bound(lane):
    cpus, policy = process_binding()
    return set(cpus) == set(lane.cpus) and policy == f'bind:{lane.node}'


def reexecuted():
    """True if this process re-executed itself into its lane (not an inherited mark)."""
    return os.environ.get(REEXEC_MARK, '').split(':', 1)[0] == str(os.getpid())


def ensure_bound(lane):
    """Re-execute this Python program under the lane binding unless it has it.

    The check reads the kernel's view of this process. A second failure after
    re-execution is refused instead of looping. Drivers launch callers already
    bound, so this returns at once and nothing is charged to the timed run."""
    if is_bound(lane):
        return
    cpus, policy = process_binding()
    # The pid survives exec, so the mark identifies this process, not a parent.
    mark = f'{os.getpid()}:{lane.node}:{format_cpus(lane.cpus)}'
    require(os.environ.get(REEXEC_MARK) != mark,
            f'lane binding did not take: CPUs {format_cpus(cpus)}, memory policy {policy}')
    os.environ[REEXEC_MARK] = mark
    sys.stdout.flush()
    sys.stderr.flush()
    command = bound((sys.executable, *sys.orig_argv[1:]), lane)
    os.execv(command[0], command)


def scan_processes():
    """(ticks since boot, host busy ticks, {pid: (comm, ppid, own cpu ticks, reaped-children cpu ticks, start ticks)})."""
    ticks = os.sysconf('SC_CLK_TCK')
    with open('/proc/uptime') as stream:
        now = float(stream.read().split()[0])*ticks
    with open('/proc/stat') as stream:
        fields = stream.readline().split()
    require(fields[0] == 'cpu', 'unexpected /proc/stat layout')
    # user nice system idle iowait irq softirq steal: busy excludes idle and iowait.
    values = list(map(int, fields[1:9]))
    busy = sum(values)-values[3]-values[4]
    processes = {}
    for entry in os.scandir('/proc'):
        if not entry.name.isdigit():
            continue
        try:
            with open(f'/proc/{entry.name}/stat') as stream:
                data = stream.read()
        except OSError:
            continue
        close = data.rfind(')')
        fields = data[close+2:].split()
        processes[int(entry.name)] = (data[data.index('(')+1:close], int(fields[1]),
                                      int(fields[11])+int(fields[12]), int(fields[13])+int(fields[14]),
                                      int(fields[19]))
    return now, busy, processes


def _descendants(processes, root):
    children = {}
    for pid, (_, parent, _, _, _) in processes.items():
        children.setdefault(parent, []).append(pid)
    found, pending = {root}, [root]
    while pending:
        for child in children.get(pending.pop(), ()):
            if child not in found:
                found.add(child)
                pending.append(child)
    return found


def _used(before, pid, values, started, t0):
    """Per-pid deltas of (own, reaped children) CPU ticks over the window, or None."""
    previous = before.get(pid)
    if previous is not None and previous[4] == started:
        return values[0]-previous[2], values[1]-previous[3]
    return values if started >= t0 else None


def other_load(start, end, root=None):
    """Competing load over the window between two scans.

    Returns a dict: window seconds; foreign CPU percent (host busy from
    /proc/stat minus our own tree, unthresholded, so it includes foreign
    processes that started and exited inside the window); foreign processes
    whose own CPU (utime+stime) is at or above 5%, as in-memory (comm, pid,
    percent) rows; and our own tree as (comm, pid, percent) rows. Our own tree
    counts its reaped children's CPU (cutime/cstime deltas): our children start
    inside the brackets that time them. A foreign process's cutime is not added
    to its row, because a reaper receives its child's whole lifetime CPU at
    once; that work is in the host total instead."""
    root = os.getpid() if root is None else root
    (t0, busy0, before), (t1, busy1, after) = start, end
    window = max(t1-t0, 1)
    own = _descendants(before, root) | _descendants(after, root)
    foreign_rows, own_rows, own_ticks = [], [], 0
    for pid, (comm, _, cpu, children, started) in after.items():
        used = _used(before, pid, (cpu, children), started, t0)
        if used is None:
            continue
        if pid in own:
            own_ticks += used[0]+used[1]
            own_rows.append((comm, pid, 100.0*(used[0]+used[1])/window))
        elif 100.0*used[0]/window >= OTHERS_THRESHOLD_PERCENT:
            foreign_rows.append((comm, pid, 100.0*used[0]/window))
    foreign_rows.sort(key=lambda r: -r[2])
    return dict(window_seconds=window/os.sysconf('SC_CLK_TCK'),
                foreign_cpu_percent=max(0.0, 100.0*(busy1-busy0-own_ticks)/window),
                foreign_rows=foreign_rows, own_rows=own_rows)


def _read(path):
    return Path(path).read_text().strip()


def delivered_clock(cpus, probe=CLOCK_PROBE):
    """(GHz mean,min,max; CPU share mean,min) of the lane CPUs from the probe."""
    result = subprocess.run((str(probe), str(CLOCK_MILLISECONDS), format_cpus(cpus)),
                            capture_output=True, text=True, timeout=60)
    require(result.returncode == 0, 'delivered-clock probe failed: '+result.stderr.strip())
    rows = [row.split('\t') for row in result.stdout.splitlines()[1:]]
    require(len(rows) == len(cpus), 'delivered-clock probe omitted a CPU')
    ghz, share = [float(r[1]) for r in rows], [float(r[2]) for r in rows]
    return (f'{sum(ghz)/len(ghz):.3f},{min(ghz):.3f},{max(ghz):.3f}',
            f'{sum(share)/len(share):.3f},{min(share):.3f}')


def _gpu_state(own_pids):
    if not shutil.which('nvidia-smi'):
        return 'unavailable', 'unavailable'
    def query(*arguments):
        result = subprocess.run(('nvidia-smi', *arguments, '--format=csv,noheader,nounits'),
                                capture_output=True, text=True, timeout=30)
        require(result.returncode == 0, 'nvidia-smi query failed')
        return [row.replace(' ', '') for row in result.stdout.splitlines() if row.strip()]
    gpus = ';'.join(query('--query-gpu=index,utilization.gpu,memory.used,clocks.sm')) or 'none'
    pids = [int(row) for row in query('--query-compute-apps=pid')]
    own = sum(pid in own_pids for pid in pids)
    return gpus, f'{own},{len(pids)-own}'


class _Errors(list):
    def capture(self, name, function, default='error'):
        try:
            return function()
        except Exception as error:
            self.append(f'{name}:{type(error).__name__}')
            return default


def snapshot(lane, probe=CLOCK_PROBE, errors=None):
    """Host state for a context row. Never raises; failures go into `errors`."""
    errors = _Errors() if errors is None else errors
    def free():
        values = []
        for directory in sorted((SYSFS/'node').glob('node[0-9]*'), key=lambda p: int(p.name[4:])):
            for line in _read(directory/'meminfo').splitlines():
                if 'MemFree:' in line:
                    values.append(f'{directory.name[4:]}:{int(line.split()[-2])//1024}')
        return ';'.join(values)
    def psi():
        line = (_read('/proc/pressure/memory').splitlines() or [''])[0]
        require('avg10=' in line, 'no memory PSI line')
        return line.split('avg10=')[1].split()[0]
    def thp():
        text = _read('/sys/kernel/mm/transparent_hugepage/enabled')
        return text[text.index('[')+1:text.index(']')]
    def governor():
        return ','.join(sorted({_read(SYSFS/'cpu'/f'cpu{c}'/'cpufreq/scaling_governor') for c in lane.cpus}))
    def own_pids():
        _, _, processes = scan_processes()
        return _descendants(processes, os.getpid())
    clock, share = errors.capture('clock', lambda: delivered_clock(lane.cpus, probe), ('error', 'error'))
    gpus, compute = errors.capture('gpu', lambda: _gpu_state(errors.capture('own_pids', own_pids, set())),
                                   ('error', 'error'))
    return dict(
        utc=datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%S.%fZ'),
        loadavg=errors.capture('loadavg', lambda: ','.join(_read('/proc/loadavg').split()[:3])),
        clock=clock, share=share,
        governor=errors.capture('governor', governor),
        smt=errors.capture('smt', lambda: _read(SYSFS/'cpu/smt/control')),
        autonuma=errors.capture('autonuma', lambda: _read('/proc/sys/kernel/numa_balancing')),
        thp=errors.capture('thp', thp), free=errors.capture('free', free), psi=errors.capture('psi', psi),
        gpus=gpus, gpu_compute=compute)


def quiet_sample(seconds=1.0):
    """One G8 §7.10 check over `seconds`: load average, foreign, own and host CPU, foreign GPU processes."""
    start = scan_processes()
    time.sleep(seconds)
    end = scan_processes()
    load = other_load(start, end)
    try:
        gpu = int(_gpu_state(_descendants(end[2], os.getpid()))[1].split(',')[1])
    except Exception:
        gpu = -1
    window = max(end[0]-start[0], 1)
    return dict(load1=float(_read('/proc/loadavg').split()[0]), foreign=load['foreign_cpu_percent'],
                own=sum(r[2] for r in load['own_rows']), host=100.0*(end[1]-start[1])/window, gpu_foreign=gpu)


def settle(checks, interval):
    """`checks` samples `interval` apart plus foreign CPU over the whole settle window."""
    start, samples = scan_processes(), []
    for check in range(checks):
        if check:
            time.sleep(interval)
        samples.append(quiet_sample())
    return samples, other_load(start, scan_processes())['foreign_cpu_percent']


def load_class(samples, window_foreign, run_foreign=None):
    """'quiet' if foreign load meets G8 §7.10 at every check, over the settle window and during the run."""
    quiet = (bool(samples) and window_foreign <= QUIET_FOREIGN_PERCENT and
             all(s['foreign'] <= QUIET_FOREIGN_PERCENT and s['gpu_foreign'] == 0 for s in samples) and
             (run_foreign is None or run_foreign <= QUIET_FOREIGN_PERCENT))
    return 'quiet' if quiet else 'loaded'


def measure_quietly(attempt, checks, interval, tries, settle_function=None, settle_in_attempt=False):
    """The one quiet gate and retry rule for every timed run, owned or NVIDIA (G8 §7.10).

    Before each attempt, `checks` settle samples `interval` seconds apart;
    then attempt(index, samples, window_foreign) runs one timed measurement
    and returns (result, run_foreign_percent, may_retry). An attempt is quiet
    when load_class says so, the run's own foreign CPU included. A loaded
    attempt is retried up to `tries` attempts in all; an attempt that may not
    be retried (a failure) stops the series. The representative is the first
    quiet attempt, else the first attempt: one n=1 estimator for quiet and
    never-quiet runs alike, never the minimum over attempts. Returns one dict
    per attempt. With settle_in_attempt the attempt settles itself immediately
    before its own timed region (a caller that starts with work of ours) and
    returns (result, run_foreign, may_retry, samples, window_foreign)."""
    settle_function = settle if settle_function is None else settle_function
    series = []
    for index in range(1, tries+1):
        if settle_in_attempt:
            result, run_foreign, may_retry, samples, window_foreign = attempt(index)
        else:
            samples, window_foreign = settle_function(checks, interval)
            result, run_foreign, may_retry = attempt(index, samples, window_foreign)
        quiet = load_class(samples, window_foreign, run_foreign)
        series.append(dict(index=index, result=result, samples=samples, window_foreign=window_foreign,
                           run_foreign=run_foreign, load_class=quiet, representative=False))
        if quiet == 'quiet' or not may_retry:
            break
    quiet = [entry for entry in series if entry['load_class'] == 'quiet']
    (quiet[0] if quiet else series[0])['representative'] = True
    for entry in series:
        entry['quiet_status'] = 'measured_quiet' if quiet else 'not_measured_quiet'
        entry['tries'] = len(series)
    return series


def page_cache_residency(paths):
    """Fraction of each file's pages resident in the page cache (mincore), by file name.

    G8 §7.1: dropping caches needs root, so runs record what they found. A
    file that cannot be mapped reads as nan."""
    import ctypes
    import mmap
    libc = ctypes.CDLL(None, use_errno=True)
    libc.mmap.restype = ctypes.c_void_p
    libc.mmap.argtypes = (ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_long)
    libc.mincore.argtypes = (ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_ubyte))
    libc.munmap.argtypes = (ctypes.c_void_p, ctypes.c_size_t)
    page = os.sysconf('SC_PAGE_SIZE')
    failed = ctypes.c_void_p(-1).value
    out = {}
    for path in paths:
        name = Path(path).name
        try:
            size = os.path.getsize(path)
            if size == 0:
                out[name] = 1.0
                continue
            descriptor = os.open(path, os.O_RDONLY)
            try:
                address = libc.mmap(None, size, mmap.PROT_READ, mmap.MAP_SHARED, descriptor, 0)
                if address in (None, failed):
                    raise OSError('mmap failed')
                try:
                    pages = (size+page-1)//page
                    vector = (ctypes.c_ubyte*pages)()
                    if libc.mincore(address, size, vector) != 0:
                        raise OSError('mincore failed')
                    out[name] = sum(v & 1 for v in vector)/pages
                finally:
                    libc.munmap(address, size)
            finally:
                os.close(descriptor)
        except OSError:
            out[name] = float('nan')
    return out


def drop_file_cache(path):
    """Ask the kernel to drop a file's clean, unmapped page-cache pages (no root needed)."""
    descriptor = os.open(path, os.O_RDONLY)
    try:
        os.posix_fadvise(descriptor, 0, 0, os.POSIX_FADV_DONTNEED)
    finally:
        os.close(descriptor)


def prepare_ingress_cache(step, tools=()):
    """Shared ingress cache policy (G8 §7.1): drop the STEP file's pages, then
    record residency of the STEP file and of each tool and its OCCT libraries.
    Library pages are recorded, not dropped: other processes map them."""
    drop_file_cache(step)
    files = [step]
    for tool in tools:
        files += [tool, *shared_libraries(tool)]
    return page_cache_residency(files)


def shared_libraries(executable, prefix='libTK'):
    """Resolved shared libraries of an executable whose names start with `prefix` (OCCT's)."""
    try:
        listing = subprocess.run(('ldd', str(executable)), capture_output=True, text=True, timeout=30).stdout
    except (OSError, subprocess.SubprocessError):
        return ()
    found = []
    for line in listing.splitlines():
        name, _, rest = line.strip().partition(' => ')
        path = rest.split(' (')[0].strip()
        if name.startswith(prefix) and path.startswith('/'):
            found.append(path)
    return tuple(found)


class Recorder:
    """Appends one context row per timing to a TSV file.

    `timing(id)` brackets a timed interval: host snapshot, then a process scan,
    before; a process scan, then a snapshot, after. Drivers start and stop
    their own clocks inside the bracket, so capture cost stays outside the
    timed interval. `capture_seconds` totals that cost. Capture never raises:
    a failed field is written as 'error' and named in capture_errors, and
    the timed work's own outcome is never masked. lane_* name the lane the
    timed work ran on; recorder_* is the binding of the process writing the
    row, which is the timed process only for in-process stages. Foreign CPU
    is counted at clock-tick resolution (10 ms on the fleet), so short
    brackets read coarsely. Known limit: host busy time includes interrupt,
    softirq and kernel writeback work that no process owns, so our own disk
    writeback or network softirqs are counted as foreign load."""

    def __init__(self, path, lane, scope='cold', labels=(), probe=None):
        self.path, self.lane, self.scope = Path(path), lane, scope
        self.probe = Path(CLOCK_PROBE if probe is None else probe)
        require(self.probe.is_file(), f'delivered-clock probe {self.probe} is not built; run '
                                      'scripts/build_measurement_probe.sh or set BREPGPU_DELIVERED_CLOCK')
        self.labels = tuple(labels)
        self.capture_seconds = 0.0
        if self.path.exists():
            with self.path.open(newline='', encoding='utf-8') as stream:
                header = tuple(next(csv.reader(stream, delimiter='\t'), ()))
            require(header == CONTEXT_COLUMNS, f'{self.path} has different context columns; use a new file')
        else:
            with self.path.open('w', newline='', encoding='utf-8') as stream:
                csv.writer(stream, delimiter='\t', lineterminator='\n').writerow(CONTEXT_COLUMNS)

    def _before(self, lane, errors):
        before = snapshot(lane, self.probe, errors)
        start = errors.capture('scan_before', scan_processes, None)
        return before, start

    def _row(self, context_id, lane, labels, before, start, outcome, errors):
        end = errors.capture('scan_after', scan_processes, None)
        after = snapshot(lane, self.probe, errors)
        load = errors.capture('load', lambda: other_load(start, end), None) if start and end else None
        cpus, policy = errors.capture('binding', process_binding, (('error',), 'error'))
        def same(key):
            return before[key] if before[key] == after[key] else before[key]+'->'+after[key]
        row = dict(context_id=context_id, utc_before=before['utc'], utc_after=after['utc'],
                   scope=self.scope, **lane.describe(), recorder_cpus=format_cpus(cpus),
                   recorder_memory_policy=policy,
                   window_seconds=f'{load["window_seconds"]:.3f}' if load else 'error',
                   foreign_cpu_percent=f'{load["foreign_cpu_percent"]:.0f}' if load else 'error',
                   foreign_processes_over_threshold=len(load['foreign_rows']) if load else 'error',
                   foreign_over_threshold_cpu_percent=f'{sum(r[2] for r in load["foreign_rows"]):.0f}' if load else 'error',
                   own_tree=';'.join(f'{_clean(c)}:{v:.0f}' for c, _, v in load['own_rows'] if v >= .5) or 'idle'
                   if load else 'error',
                   loadavg_before=before['loadavg'], loadavg_after=after['loadavg'],
                   clock_ghz_mean_min_max_before=before['clock'], clock_ghz_mean_min_max_after=after['clock'],
                   cpu_share_mean_min_before=before['share'], cpu_share_mean_min_after=after['share'],
                   governor=same('governor'), smt=same('smt'), autonuma=same('autonuma'), thp=same('thp'),
                   node_free_mib_before=before['free'], node_free_mib_after=after['free'],
                   memory_psi_some_avg10_before=before['psi'], memory_psi_some_avg10_after=after['psi'],
                   gpus_before=before['gpus'], gpus_after=after['gpus'],
                   gpu_compute_own_foreign_before=before['gpu_compute'],
                   gpu_compute_own_foreign_after=after['gpu_compute'],
                   capture_errors=';'.join(errors) or 'none',
                   labels=';'.join(f'{k}={_clean(str(v))}' for k, v in
                                   (*self.labels, *labels, ('outcome', outcome))))
        with self.path.open('a', newline='', encoding='utf-8') as stream:
            csv.writer(stream, delimiter='\t', lineterminator='\n').writerow([row[c] for c in CONTEXT_COLUMNS])

    @contextlib.contextmanager
    def timing(self, context_id, lane=None, labels=()):
        lane = self.lane if lane is None else lane
        errors = _Errors()
        began = time.perf_counter()
        try:
            before, start = self._before(lane, errors)
        except Exception as error:
            errors.append(f'before:{type(error).__name__}')
            before, start = None, None
        self.capture_seconds += time.perf_counter()-began
        outcome = 'completed'
        try:
            yield
        except BaseException as error:
            outcome = 'raised_'+type(error).__name__
            raise
        finally:
            began = time.perf_counter()
            try:
                if before is None:
                    before = snapshot(lane, self.probe, errors)
                self._row(context_id, lane, labels, before, start, outcome, errors)
            except Exception as error:
                print(f'measurement lane: context row {context_id} not written: '
                      f'{type(error).__name__}: {error}', file=sys.stderr)
            self.capture_seconds += time.perf_counter()-began


def _clean(text):
    return ''.join(c if c.isprintable() and c not in '\t;:=' else '_' for c in text)


def _run(args):
    require(sys.platform == 'linux', 'lane runs execute on the Linux fleet')
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    require(command, 'a command to run is required after --')
    require(args.quiet_checks >= 1 and args.quiet_interval >= 0 and args.quiet_tries >= 1,
            'at least one quiet check and one try are required')
    lane = make_lane(args.node, args.workers, args.gpu)
    numactl()
    recorder = Recorder(args.context, lane, args.scope)
    def attempt(index, samples, window_foreign):
        label = args.label if index == 1 else f'{args.label}-try{index}'
        residency = {}
        if args.ingress_file is not None:
            # The same page-cache state the NVIDIA caller gives its ingress.
            residency = prepare_ingress_cache(args.ingress_file, (command[0],))
        cache = ','.join(f'{k}/{v:.3f}' for k, v in residency.items()) or 'not_managed'
        with recorder.timing(label, labels=(('command', Path(command[0]).name), ('try', index),
                                            ('page_cache', cache))):
            started = time.perf_counter()
            code = subprocess.run(bound(command, lane), env=lane_environment(lane)).returncode
            wall = time.perf_counter()-started
        with Path(args.context).open(newline='', encoding='utf-8') as stream:
            row = [r for r in csv.DictReader(stream, delimiter='\t') if r['context_id'] == label][-1]
        foreign = float(row['foreign_cpu_percent']) if row['foreign_cpu_percent'] != 'error' else float('inf')
        return (label, wall, code), foreign, code == 0
    series = measure_quietly(attempt, args.quiet_checks, args.quiet_interval, args.quiet_tries)
    path = Path(args.context).with_suffix('.timings.tsv')
    with path.open('a', encoding='utf-8') as stream:
        if stream.tell() == 0:
            stream.write('context_id\tprocess_wall_seconds\texit_code\ttry\ttries\tload_class\tquiet_status\t'
                         'representative\tsettle_foreign_cpu_percent\trun_foreign_cpu_percent\n')
        for entry in series:
            label, wall, code = entry['result']
            stream.write(f'{label}\t{wall:.9f}\t{code}\t{entry["index"]}\t{entry["tries"]}\t{entry["load_class"]}\t'
                         f'{entry["quiet_status"]}\t{int(entry["representative"])}\t{entry["window_foreign"]:.3f}\t'
                         f'{entry["run_foreign"]:.3f}\n')
    chosen = next(entry for entry in series if entry['representative'])
    label, wall, code = chosen['result']
    print(f'{label}\t{wall:.6f} s\texit {code}\t{chosen["quiet_status"]}, {chosen["tries"]} tries', file=sys.stderr)
    return code


def _lanes(args):
    topology = read_topology()
    allowed = allowed_cpus()
    for node, cores in topology.items():
        print(f'node {node}: {len(cores)} physical cores, lane order {format_cpus(lane_order(topology, node))}; '
              f'allowed here {format_cpus(sorted(set(lane_order(topology, node)) & allowed))}')
    return 0


def parser():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = p.add_subparsers(dest='action', required=True)
    run = commands.add_parser('run', help='run one command pinned to a lane with a context row')
    run.add_argument('--node', type=int)
    run.add_argument('--gpu', type=int, help='lane GPU (nvidia-smi index); the lane takes its node')
    run.add_argument('--workers', type=int, help='CPUs in lane order; default every CPU of the node')
    run.add_argument('--scope', choices=('cold', 'warm'), default='cold')
    run.add_argument('--context', type=Path, required=True,
                     help='context TSV to append to; process walls go beside it in .timings.tsv')
    run.add_argument('--label', required=True)
    run.add_argument('--quiet-checks', type=int, default=3)
    run.add_argument('--quiet-interval', type=float, default=60., help='seconds between quiet checks (G8 §7.10)')
    run.add_argument('--quiet-tries', type=int, default=3, help='attempts until one runs quiet')
    run.add_argument('--ingress-file', type=Path,
                     help='STEP file the command reads: its pages are dropped before each attempt, and residency '
                          'of it and of the command\'s OCCT libraries is recorded, as for NVIDIA ingress')
    run.add_argument('command', nargs=argparse.REMAINDER)
    commands.add_parser('lanes', help='print each node and its lane CPU order')
    return p


if __name__ == '__main__':
    arguments = parser().parse_args()
    try:
        sys.exit(_run(arguments) if arguments.action == 'run' else _lanes(arguments))
    except ValueError as error:
        print(f'measurement lane: {error}', file=sys.stderr)
        sys.exit(2)
