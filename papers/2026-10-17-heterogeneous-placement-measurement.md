# Heterogeneous placement and the measurement protocol

Research lane M4-G8 of the fast-mesher campaign, 2026-10-17 (fleet runs started
02:15 UTC on 2026-10-18). Brief:
[docs/fast-mesher-campaign/briefs/M4-G8.md](../docs/fast-mesher-campaign/briefs/M4-G8.md),
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub:
[lifecycle map §4.1 G8](2026-10-17-geometry-lifecycle-map.md) (S18, X11). Bead
`brepgpu-12s.9`. Sources and read depth:
[2026-10-17-heterogeneous-placement-measurement-sources.tsv](2026-10-17-heterogeneous-placement-measurement-sources.tsv).

What ran: owned hardware microbenchmarks only, on the fleet host, in the lane
directory `lanes/M4-G8/`. No mesher, Boolean or NVIDIA run. Nothing was built
or run on the laptop. The probes are summarised in §A.

Tags: **[src]** a source establishes it; **[data]** a number already in this
repository from the previous regime (file:line); **[meas]** measured by this
lane on the fleet; **[own]** this lane's derivation; **[inf]** inferred, not
established.

## 0. Decision-relevant findings

1. **Placement rule.** Inside a socket, priority work stealing over
   over-decomposed tasks, seeded largest-first. Across device types,
   HeteroPrio-style allocation by acceleration factor with spoliation, at
   batch granularity, using per-device (not per-type) cost models. Across the two
   sockets (whole-host mode only), an LPT pre-partition by a-priori work estimate
   plus coarse cross-socket stealing. Fallback: CPU-only priority work stealing
   with the GPU off. Plain greedy list scheduling with a GPU in the pool is
   ruled out: it has no constant approximation ratio [src survey §4.1.1], and
   StarPU measured exactly that failure [src StarPU §4.2]. HEFT's upward rank
   is kept as the priority; its O(n²) insertion scheduler is not (§3).
2. **The two GPU lanes are not symmetric.** GPU0 trains its PCIe link at Gen3
   **x8**; GPU1 trains at Gen3 x16 [meas, `nvidia-smi` under load]. Pinned
   256 MiB transfers: GPU0 5.7–5.8 GB/s from either node; GPU1 10.5 GB/s H2D
   and 9.0 GB/s D2H from its own node, 9.1 and 7.9 GB/s from the remote node.
   Pageable memory gives about 3 GB/s on every path. Cost models must be
   calibrated per device. The arena's reference configuration, repeated on both
   lanes, will see this asymmetry as a lane effect, not as interference.
3. **GPU worth is a batch-size and cost-class question, not a per-face one.**
   At NVIDIA's measured 2.5 ms of CPU per face, per-face offload buys nothing.
   The usual model (§4) puts the break-even at about 10³ items per batch for
   B-spline-class evaluation on GPU1. Analytic-surface evaluation never pays,
   because the PCIe transfer alone costs more per item than eleven CPU cores
   need to compute it. At watch scale, vertex evaluation is a small slice of
   NVIDIA's 98.8 µs per triangle. So the hypothesis to falsify is that the P100
   does not change the meshing stage at watch scale, and pays only at
   10⁵–10⁶-item batches (fine ε, large inputs) and in display.
4. **CUDA context creation is about 0.21–0.29 s per process, and it is not
   driver initialisation.** Keeping another process holding the device saves
   only about 20 ms [meas]. The cost is per process, so it is hidden by starting
   context creation on its own thread at process start, overlapped with ingress,
   or amortised by a long-lived session. Cold timings must include it.
5. **AutoNUMA is on and moves pages fast.** 2 GiB first-touched on node 1 and
   read from node 0 was fully migrated in about 4.3 s. During migration,
   single-thread read bandwidth fell to 1.9 GB/s, against 7–8 GB/s remote before
   and 11–12 GB/s local after. Binding memory with `--membind` stopped migration
   entirely [meas]. Arenas therefore bind their pages explicitly. First touch
   alone is not a placement under AutoNUMA.
6. **Intra-face split threshold:** split a face when its estimated work exceeds
   τ = δ·W/(P−1), with δ = 0.1, and stop splitting at g_min ≈ 100·o, where o is
   the per-task scheduling overhead. With NVIDIA-class work on one 12-core lane
   (W ≈ 23 CPU-s), τ ≈ 0.21 s, about 80 times NVIDIA's mean face (§5).
7. **Measurement protocol** (§7): separate cold-process and declared-warm
   scopes, never merged; per-definition mesh accounting with per-occurrence
   emission reported separately; at least 10 fresh processes for headline
   numbers, reported as a median with a rank-based CI; configurations
   interleaved in randomised order with randomised environment padding;
   numactl lane pinning; and context rows emitted before and after every run.
   NVIDIA is pinned per worker to physical cores of the same socket, gets an
   LPT assignment from its own measured per-definition times, and keeps its
   fastest valid arm out of {12, 24} workers.

## 1. Inputs read

Doctrine and campaign: BLACKBIRD.md, AGENTS.md, the fast-mesher direction journal
(gate G1 decisions), the campaign PLAN, the working-draft design (§5.5, §6.2,
§9.5 especially), and the lifecycle map in full for S18 and X11. Earlier papers
built on, not redone: the owned-mesher audit §2 and §5, the NVIDIA SmTess study
§4, fast-tessellation §6, parallel-growth-clusters §§0, 4–6, whole-mesher native
runtime ("P100 is part of the geometry design"), GPU lifetime/reuse study and
the Wafer/GPU MODE study (P100 profiler limits). Previous-regime GPU numbers were
collected from every `results/*/REPORT.md` (§4.1). Literature is listed in the
sources TSV.

## 2. Hardware microbenchmarks (question 6)

### 2.1 Host state, recorded with every run

| Item | Value | Can this lane change it? |
|---|---|---|
| CPU | 2× Xeon E5-2697 v2, 12 cores / 24 threads per socket; AVX, no AVX2/FMA | — |
| SMT | `smt/control = on`; siblings are cpu *n* and *n*+24 (cpu0 with cpu24, cpu1 with cpu25) | no (root) |
| NUMA | node 0 = even CPU ids, node 1 = odd; distances 10/20; ~387 GB per node | — |
| AutoNUMA | `kernel.numa_balancing = 1` | no (root); can be neutralised per allocation by binding (§2.4) |
| Frequency | driver `intel_cpufreq` (intel_pstate passive), governor `schedutil`, range 1.2–3.5 GHz, turbo not disabled | no (root) |
| THP | `madvise` | no; probes did not madvise (4 KiB pages) |
| GPUs | 2× P100-PCIE-16GB; GPU0 bus 05 on node 0; GPU1 bus 42 on node 1; persistence mode **off**; ECC on; application clocks not set | no (root) |
| PCIe | GPU0 **Gen3 x8** (link max x16); GPU1 Gen3 x16, read under transfer load | no (physical) |
| Driver / toolkit | driver 580.178.04 (CUDA 13.0 driver API); probes built with the pinned CUDA 12.8.1 `nvcc`, sm_60; CPU probe with the pinned Clang 18.1.3 (`-O2 -mavx`) | — |

Competing load: other campaign lanes and services shared the host. Every run
recorded load average, instantaneous CPU of other processes (≥5% each, summed),
GPU utilisation and processes, free memory per node, memory PSI, and frequency of
the lane's CPUs, before and after (`ctx.sh`, §A). During pass p1 the 1-minute
load average was 4.7–17.7, and other processes used 0.1–16 cores (sum of their
instantaneous CPU, 13–1617%). Pass p1 was therefore a **loaded** run.
§2.7 states which runs were repeated on a quiet host.

### 2.2 Memory bandwidth, local and remote [meas, pass p1, loaded]

STREAM-style kernels, 3 × 1 GiB arrays of doubles, compiler vectorised, no
non-temporal stores (so write-allocate traffic is not counted, per STREAM
convention), threads pinned one per CPU, memory bound with `numactl --membind`.
Best of 10 iterations, with the median in parentheses (GB/s).

| Threads on node | Memory node | Triad | Read-only sum | Copy |
|---|---|---:|---:|---:|
| 1 core, node 0 | 0 (local) | 11.3 (10.6) | 13.2 (12.4) | 7.1 |
| 1 core, node 0 | 1 (remote) | 7.8 (7.4) | 7.8 (7.5) | 3.6 |
| 1 core, node 1 | 1 (local) | 10.8 (10.6) | 13.2 (12.9) | 6.8 |
| 1 core, node 1 | 0 (remote) | 8.6 (8.2) | 8.3 (7.8) | 3.7 |
| 12 cores, node 0 | 0 | 25.2 (25.1) | 37.0 (36.6) | 27.8 |
| 12 cores, node 0 | 1 | 23.7 (23.4) | 24.9 (24.1) | 21.0 |
| 12 cores, node 1 | 1 | 26.0 (25.8) | 38.3 (37.1) | 28.6 |
| 12 cores, node 1 | 0 | 23.8 (23.3) | 24.9 (23.5) | 21.3 |
| 24 threads (SMT), node 0 | 0 | 24.8 (24.4) | 36.1 (35.4) | 25.9 |
| 24 threads (SMT), node 1 | 1 | 25.7 (25.7) | 37.9 (37.7) | 28.0 |
| 24 cores, both nodes, first touch | local | 50.8 (50.3) | 72.2 (70.5) | 55.6 |
| 24 cores, both nodes, `--interleave=0,1` | mixed | 49.3 (48.9) | 68.8 (67.3) | 46.9 |

- Remote read bandwidth for a whole socket is about 65% of local (24.9 vs 37–38
  GB/s). Triad, which writes as well, loses only about 7%.
- SMT adds no memory bandwidth.
- Both sockets with local first touch reach 72 GB/s read. Interleaving costs
  5–16%, depending on the kernel.

### 2.3 Latency and cache-line transfer [meas, p1]

- **Dependent random loads** over 1 GiB with 4 KiB pages, so TLB misses are
  included:
  - local 128.2 ns on node 0 and 125.5 ns on node 1;
  - remote 222.5 ns (node 0 reading node 1) and 200.0 ns (node 1 reading
    node 0).
  - Remote is 1.6–1.7× local (median of 5 each).
- **Cache-line ping-pong round trip** (median of 5 × 10⁶ round trips):
  - SMT siblings 40.4 ns;
  - two cores on one socket 147.6 ns;
  - across sockets 298.0 ns.
- A work-stealing transaction costs a few such transfers. The estimate is about
  0.3–0.6 µs on one socket and 0.6–1.2 µs across sockets, before the stolen
  task's data misses [own, from the round trips].
- **Central-queue dispatch cost** (pass p5, `cpu_probe2` mode `sched`).
  - Setup: 2·10⁶ empty tasks taken from one shared queue.
  - Per-task overhead as seen by a worker = (P·wall − Σwork)/tasks, over 3
    repetitions.
  - Mutex FIFO, which has the same shape as today's
    `native/mesher/scheduler.cpp:56-70`:
    - 28 ns with one thread;
    - 1.11–1.15 µs with 12 threads on one socket;
    - 1.29–1.42 µs with 12 threads and 1 µs of work per task;
    - 2.49–2.56 µs with 24 threads across both sockets.
  - Shared atomic counter:
    - 10.6 ns with one thread;
    - 0.56–0.59 µs with 12 threads;
    - 0.97–1.02 µs with 24 threads.
  - **Caveat:** these p5 numbers were taken at load average 30–33, while
    another lane's processes used 25–28 cores. The quiet repeat (§2.8) gives
    1.01–1.08 µs (mutex FIFO, 12 threads) and 0.54 µs (atomic counter).
  - A central queue costs about 1 µs per task at 12 workers. That is why
    per-worker deques with stealing are the rule (§3.3), and why o ≈ 1 µs is
    the conservative overhead in §5.

### 2.4 AutoNUMA in action [meas, p1]

The probe first-touches 2 GiB from cpu1 (node 1), then reads it repeatedly from
cpu0 (node 0). Every 0.5 s it samples page residency with `move_pages(…, NULL,
…)` on every 256th page.

| t (s) | Read GB/s | Fraction on node 0 |
|---:|---:|---:|
| 0.31 | 7.0 | 0.00 |
| 1.42 | 3.8 | 0.09 |
| 2.55 | 1.9 | 0.44 |
| 3.60 | 2.1 | 0.78 |
| 4.33 | 2.9 | 1.00 |
| 4.52–60 | 10.5–12.5 | 1.00 |

With `numactl --membind=1`, the same reads stayed remote for 30 s at 7.1–8.1
GB/s, with no page moved. AutoNUMA does what the Linux documentation says
(unmap, fault, migrate toward the accessor). Explicit binding switches it off for
the bound pages, as the kernel documentation recommends for bound workloads
[src LinuxSysctl] and NVIDIA recommends for GPU applications [src CUDA BP
§13.4].

### 2.5 Frequency and FP64 throughput under load [meas, p1]

The frequency probe runs a dependent integer-add chain (one add per cycle), so
adds/s equals delivered core clock for one thread per core. With two SMT threads
per core, the number is per-thread throughput, not clock.

| Threads (node 0 unless stated) | Rate per thread, GHz-equivalent | Load avg (1 min) before |
|---|---|---:|
| 1 | 3.06 | 7.9 |
| 6 cores | 2.95 (min 2.90) | 12.1 |
| 12 cores | 2.94 (min 2.86) | 7.7 |
| 24 threads (SMT) | 2.53 (min 2.30) | 9.5 |
| 48 threads, both sockets | 1.89 (min 0.05: threads time-shared with other lanes) | 13.3 |

- The delivered clock under schedutil was 2.9–3.1 GHz on loaded cores, against
  `scaling_cur_freq` snapshots of 1.2–3.5 GHz taken between runs. The snapshot
  does not represent the run, so the protocol measures delivered clock
  directly (§7.9).
- **AVX FP64, independent mul+add chains (no FMA on this ISA):**
  - 23.0 GFLOP/s on one core (8 flop/cycle at about 2.9 GHz);
  - 268.7 GFLOP/s for 12 cores of one socket;
  - 261.0 GFLOP/s for 24 SMT threads (SMT adds nothing);
  - 531.9 GFLOP/s for 24 cores on both sockets.
- **P100 FP64 FMA:**
  - GPU0 4.10 TFLOP/s median (best 4.20), with its SM clock seen at 987 MHz
    during one sample;
  - GPU1 4.60 TFLOP/s (best 4.71).
  - FP32: 7.5 and 8.2 TFLOP/s.
  - Device copy: 508 GB/s on both.
- **Peak ratios:**
  - one P100 ≈ 15–17 CPU sockets of FP64, and about 14–20 sockets of memory
    bandwidth;
  - one P100 ≈ 8 whole hosts of FP64.

### 2.6 PCIe, launch latency and context creation [meas, p1 and p3]

Pinned (`cudaHostAlloc`) and pageable memory, each copy timed with a host clock
around the copy and the stream sync. Medians in GB/s; n = 20 at 256 MiB.

| GPU (link) | Host node | H2D pinned | D2H pinned | Bidirectional pinned (sum) | H2D pageable | D2H pageable |
|---|---|---:|---:|---:|---:|---:|
| GPU0 (x8) | 0 | 5.80 | 5.73 | 10.96 | 3.08 | 2.97 |
| GPU0 (x8) | 1 | 5.74 | 5.26 | 10.26 | 3.07 | 2.73 |
| GPU1 (x16) | 0 (remote) | 9.07 | 7.91 | 15.55 | 3.02 | 2.69 |
| GPU1 (x16) | 1 (local) | 10.51 | 8.95 | 16.81 | 3.14 | 3.01 |

- **Small transfers.** A 4 KiB pinned copy plus sync takes 6.4–7.8 µs. At
  64 KiB it is 12.6–18.5 µs, and at 1 MiB 106–193 µs. Pageable memory triples
  the time from 64 KiB upward.
- **Launch cost.** Medians over 10⁴ samples, every GPU/node combination:
  - empty kernel launch plus `cudaStreamSynchronize`: 7.4–7.8 µs (p99 20–23 µs);
  - back-to-back asynchronous launches: 3.6–4.0 µs each;
  - one-node CUDA graph launch plus sync: 5.9–6.6 µs;
  - smallest offload round trip (pinned 4 KiB H2D, kernel, 4 KiB D2H, sync):
    16.2–17.7 µs;
  - the same round trip at 64 KiB: 28–37 µs, and at 1 MiB 211 µs (GPU1) to
    386 µs (GPU0 from node 1).
- **Context creation in a fresh process** (`cudaSetDevice` + `cudaFree(0)`).
  Medians of 10 processes, with rank 95% CIs:
  - pass p1, every GPU/node combination: 242.8–249.5 ms (CI 227–279 ms);
  - pass p3, with no other process on the device: 228.7 ms on GPU0 and
    291.0 ms on GPU1;
  - pass p3, with another process holding the device: 207.9 ms on GPU0 and
    263.5 ms on GPU1 (GPU1 n = 5).
  - Holding the device saves about 20–30 ms. The NVIDIA documentation's 1–3 s
    of ECC scrubbing on GPU initialisation [src NvidiaPersistence §2] was not
    observed. The cost is per process.
- **First use after context creation.**
  - First kernel launch plus sync, which includes lazy module loading: 0.24 ms.
  - Second launch: 0.010 ms.
  - `cudaMalloc` of 64 MiB: 0.17 ms.
- Earlier engine timings recorded P1001 at about 2× P1000 (for example
  272.909 vs 561.780 ms, `results/boolean-2026-10-15/REPORT.md:109`) [data].
  This probe shows equal costs on both devices. That supports the lifetime
  study's hypothesis that the old `DeviceScope` restore initialised device 0
  as well (`papers/2026-10-14-gpu-lifetime-reuse-study.md:35-37`) [inf].

### 2.7 Quiet repeats

A watcher on the fleet host (`quietwait.sh`) ran the key subset of probes only
after three consecutive checks, about a minute apart, showed:

- 1-minute load average ≤ 4.5;
- other processes ≤ 150% CPU;
- no GPU compute process.

The key subset is:

- local and remote bandwidth at 12 threads;
- frequency at 1, 12 and 24 threads;
- dispatch cost;
- PCIe and launch, on both GPUs from both nodes.

The watcher waited from 02:26 to 03:29 UTC while other lanes ran, at load
averages up to 41. Its last checks before triggering read load 4.45/3.90/3.81
and other processes at 102/22/30% CPU. Pass **p2** then ran 03:29–03:32 UTC.

- Other processes used 16–143% CPU during the p2 runs.
- The rise of the 1-minute load average to 5–9 is the probes' own threads.
- p2 is the **quiet** pass. p1, p4 and p5 are **loaded**.

### 2.8 Quiet-pass results (p2) against the loaded passes

| Probe | Quiet p2 | Loaded p1 / p4 |
|---|---|---|
| Triad, 12 cores, local (node 0 / node 1) | 25.3 / 25.3 GB/s | 25.2 / 26.0; 25.4 / 25.4 |
| Triad, 12 cores, remote (cores 0→mem 1 / 1→0) | 23.4 / 23.8 | 23.7 / 23.8; 23.1 / 22.9 |
| Read, 12 cores, local / remote (node 0 cores) | 36.7 / 24.6 | 37.0 / 24.9; 37.1 / 23.7 |
| Dependent-add rate, 1 / 12 / 24 threads | 3.13 / 2.97 / 2.54 | 3.06 / 2.94 / 2.53; 3.05 / 2.96 / 2.51 |
| Pinned 256 MiB H2D, GPU0 n0 / GPU0 n1 / GPU1 n0 / GPU1 n1 | 5.78 / 5.62 / 8.83 / 10.20 GB/s | 5.80 / 5.74 / 9.07 / 10.51; 6.09 / 6.03 / 9.64 / 10.35 |
| Pinned 256 MiB D2H, same order | 5.91 / 5.24 / 7.38 / 8.76 | 5.73 / 5.26 / 7.91 / 8.95; 5.95 / 5.52 / 8.40 / 9.32 |
| Bidirectional, same order | 11.0 / 10.3 / 15.3 / 17.3 | 11.0 / 10.3 / 15.6 / 16.8; 11.7 / 10.7 / 16.5 / 17.0 |
| Empty launch + sync, median | 7.33–7.81 µs | 7.40–7.75; 7.20–8.06 |
| 4 KiB offload round trip, median | 16.4–17.9 µs | 16.2–17.7; 16.1–18.7 |
| Mutex FIFO dispatch, 1 / 12 / 24 threads | 22–23 ns / 1.01–1.08 µs / 2.08–2.09 µs | p5: 28 ns / 1.11–1.15 µs / 2.49–2.56 µs |
| Atomic-counter dispatch, 1 / 12 / 24 threads | 6.8–7.0 ns / 0.54 µs / 1.02 µs | p5: 10.6 ns / 0.56–0.59 µs / 0.97–1.02 µs |

Quiet and loaded passes agree within a few percent for bandwidth, frequency, PCIe
and launch. The probes pin one thread per core, and the competing processes
mostly ran elsewhere. Dispatch cost under heavy load (p5, load 30) is up to about
20% higher than quiet. The protocol's quiet rule still stands for whole-operation
timings: those run on every core of a lane and cannot avoid competing processes
the way a pinned probe often does.

## 3. Placement (question 1)

### 3.1 What the candidates establish

- **LPT and list scheduling (Graham)** [src survey §3.1.1; Graham 1969].
  - List scheduling on identical machines: C ≤ W/m + p_max ≤ (2 − 1/m)·C*.
  - LPT order improves this to (4/3 − 1/(3m))·C*.
  - With precedence, Graham's bound uses the critical path T∞ in place of
    p_max.
  - On two resource types, list scheduling has **no constant ratio**. With one
    CPU, one GPU and two tasks of CPU time M and GPU time 1, any list scheduler
    starts one on the CPU and takes M [src survey §4.1.1].
- **HEFT** [src survey §6.1.1; Topcuoglu 2002 abstract].
  - Ranks tasks by upward rank, which is the bottom level with averaged device
    costs.
  - Puts each task on the processor with the earliest finish time, inserting
    into idle gaps.
  - Costs O(n²) and has no constant approximation ratio.
  - Still "behaves well" in the survey's unified experiments, and is the basis
    of StarPU's `dmda`/`heft-tm`.
- **StarPU performance models** [src StarPU §3.3, §4.2].
  - Greedy policies let "CPUs grab tasks even if the GPU is about to become
    available".
  - `heft-tm`, driven by per-architecture performance models, improves
    small and medium problems "up to twofold".
  - 5% random mis-prediction causes "very low performance degradation".
  - Automatic history-based models land within about 1% for regular kernels.
- **HeteroPrio** [src survey §5.1.3, Theorem 5.4].
  - Sort tasks by acceleration factor α = p_cpu/p_gpu. Idle CPUs take from the
    low-α end and idle GPUs from the high-α end.
  - When the list empties, an idle resource *spoliates*: it restarts the task
    that would finish latest, if it can finish it sooner.
  - Ratio: 2 + √2 ≈ 3.41 in general; 2 when every task is no longer than C*.
    Complexity O(n log n + n log m + m log m).
  - The best low-cost heuristic in the survey's experiments. It struggles
    when CPU and GPU counts are close, because fewer spoliation opportunities
    arise.
  - Extends to DAGs: HEFT-like priorities break ties and choose what an idle
    GPU spoliates (§7.2.1).
- **Online lower bounds** [src survey §4.2].
  - No online algorithm for independent tasks beats ratio 2.
  - With precedence, no online algorithm beats √(m/k), even when upward ranks
    are known.
  - Allocation therefore needs a global view of the work (the a-priori
    estimate), not decisions made one task at a time.
- **Work stealing**
  - [src Blumofe–Leiserson 1999 Thm 13] Expected time T₁/P + O(T∞) for fully
    strict computations, with a high-probability version.
  - [src Arora–Blumofe–Plaxton 1998, abstract and §2] Under multiprogramming,
    expected time is O(T₁/P_A + T∞·P/P_A), where P_A is the average number of
    processors the kernel actually gives. Over-subscribing worker threads
    (P > P_A) inflates the critical-path term by P/P_A.
  - [src Acar–Blelloch–Blumofe 2000, abstract] Work stealing can lose
    locality; locality-guided stealing (thread affinity) matched static
    partitioning on dedicated machines, and beat it by up to 50% under
    multiprogramming.
- **NUMA arenas** [src oneTBB guide, via lifecycle map].
  - oneTBB uses one arena per NUMA node.
  - The lifecycle map's contradiction X11 is that two strictly separate socket
    pools reintroduce static imbalance.

### 3.2 Fit to our task graph [own]

The graph, from design §5: per-edge sampling and resolution batches, then
per-face work (lattice ranges, band components, staged-healer items), then
publication. It is shallow, with T∞ ≈ max over faces of (edge batch + face
work). Tasks number 10⁴–10⁶ with heavy-tailed costs. A-priori work estimates
exist before work begins (design §5.5; growth clusters C3). The staged healer
makes some arrivals online (design §7).

| Candidate | Fits | Does not fit |
|---|---|---|
| LPT alone | Independent face work with good estimates; simple; 4/3 bound | Heterogeneous devices (no constant ratio); estimate errors; multiprogramming |
| HEFT | Priorities from upward rank; per-device costs | O(n²) insertion at 10⁵ tasks; offline; no guarantee |
| StarPU dmda/heft-tm | Performance-model calibration; robust to 5% noise | A runtime dependency (not adopted; library rule); greedy variants fail |
| HeteroPrio | Independent-dominated graphs; device choice by α; spoliation fixes the tail; cheap | Needs α per task class; CPU/GPU count 12:1 per lane is the favourable case |
| Work stealing | Tolerates estimate error and background load (Arora); locality-friendly | Device-blind; T∞ term must be bounded by splitting |

### 3.3 The rule

**Lane mode** (one socket, its memory node and its local GPU; the shipped
configuration that must win, design §2 goal 1):

1. **Tasks and estimates.**
   - Every task carries a class κ: edge-sampling batch, edge-resolution batch,
     lattice range, band component, healer item, publication, or GPU batch of
     stage s. It also carries a-priori work units u (samples, cells, band
     trims × layers).
   - Priority is the bottom level (HEFT upward rank). In this shallow graph
     that is "face work remaining after this task".
2. **CPU workers.**
   - One worker per physical core, minus the core whose SMT sibling hosts the
     GPU driver thread (see 4).
   - Per-worker deques, seeded in LPT order of (priority, estimated cost).
     Owners pop the highest-priority local task; thieves steal the oldest,
     largest task.
   - Over-decompose so that no task exceeds τ (§5).
   - Never run more workers than cores the lane owns. Arora's bound charges
     over-subscription to the critical path.
3. **Device allocation (HeteroPrio at batch granularity).**
   - Only stages with a GPU implementation are eligible: lattice evaluation,
     emission and normals; span sizing; display.
   - Each eligible stage-class batch gets α = t_cpu/t_gpu from the cost model.
   - The GPU pulls from the high-α end; CPU workers pull eligible work from
     the low-α end.
   - Classes with α below the break-even of §4 (all analytic-surface
     evaluation) never go to the GPU.
   - When the GPU runs dry, it may spoliate a running or queued CPU batch it
     can finish sooner, and the CPU result is dropped. Spoliated work is
     re-executed, never merged, so the D-determinism contract is unaffected:
     the output is that of one backend.
4. **GPU driver thread.**
   - One host thread per GPU, pinned to the SMT sibling of a lane core.
   - Double-buffered pinned staging on the GPU's node, so copies overlap
     kernels on the P100's copy engines.
   - Context creation starts at process start on this thread, overlapped with
     ingress. The GPU joins the pool when its context is ready.
   - Whether the sibling thread costs the core's worker anything is an arena
     flag (§8).
5. **Memory.**
   - Every arena, staging buffer and output buffer is allocated with
     `mbind(MPOL_BIND)` to the lane's node.
   - Read-mostly source and carrier data (§5.1 cache) is bound to the lane's
     node too.

**Whole-host mode** (two sockets, two GPUs: an additional configuration, and
the one that matches a two-socket NVIDIA arm):

6. **Socket pre-partition.**
   - Definitions, or face groups of very large definitions, are assigned to
     sockets by LPT on estimated cost.
   - LPT on two bins is within 7/6 of optimal by Graham's bound, and near
     perfect with thousands of items.
   - Each socket then runs the lane rule with its own GPU.
7. **Cross-socket stealing.**
   - A socket whose deques are empty steals from the other socket, choosing
     only tasks whose estimated cost is ≥ 10× the cross-socket steal overhead.
     That overhead is about 1 µs (§2.3), so in practice any task above about
     10 µs qualifies.
   - The stolen task's outputs go to the thief's arena. Its inputs are read
     remotely: 1.6–1.7× latency, about 65% of bandwidth (§2.2).
   - This replaces design §5.5's "one pool per socket", which X11 shows
     reintroduces static imbalance, with a partition plus overflow.
8. **Read-mostly source data** is replicated once per node. Copying about 10 MB
   at 24 GB/s remote costs about 0.4 ms [own]. Remote dependent loads cost
   1.6–1.7×.

### 3.4 Cost model

For task class κ on device d (each CPU socket and each GPU separately; GPU0 ≠
GPU1, §2.6):

- CPU: t_cpu(κ) = a_κ + b_κ·u.
- GPU batch: t_gpu(s, d) = t0_d + u·max(g_s,d , β_s/B_d), where:
  - t0_d is the batch round-trip floor: 16–18 µs measured for the smallest
    batch, plus about 3.6 µs per additional kernel;
  - g_s,d is device time per item;
  - β_s is bytes per item crossing PCIe;
  - B_d is the device's pinned bandwidth (GPU0 about 5.7 GB/s, GPU1 about 9–10
    GB/s).
  - max() assumes copies overlap compute through two streams. Without
    overlap, it is the sum.
- Calibration is history-based, as in StarPU:
  - least-squares fit per (κ, device, host, binary hash) from the previous
    run's per-task timings, persisted beside the binary;
  - first-run defaults come from the microbenchmarks above and from the
    previous regime's numbers in §4.1.
- Accuracy needed: StarPU tolerates 5% random error [src]. The growth-cluster
  lane's H3 asks R² ≥ 0.8 between predicted and measured item time across
  workload families. This lane adopts that falsifier.

### 3.5 Fallback

The rule falls back to **CPU-only priority work stealing, GPU off** when any of
these holds:

- the cost model for a class is uncalibrated;
- the median measured/predicted ratio for a class leaves [0.5, 2] in the current
  run;
- the GPU context is not ready;
- the predicted GPU saving is below the remaining context-creation time.

In whole-host mode the fallback keeps cross-socket stealing.

Fallback decisions are logged per run.

### 3.6 Hypotheses and falsifiers for the arena

- **P1.** Lane-mode makespan ≤ 1.1·max(W/P, T∞) on the corpus. Falsifier:
  makespan above that, with idle time from a single unsplittable item, or a
  measured/predicted ratio outside [0.5, 2].
- **P2.** HeteroPrio-style allocation beats CPU-only fallback on the GPU-eligible
  stages for inputs whose eligible batches exceed N* (§4). Falsifier: no
  improvement beyond the CI on those inputs, or a slowdown on any input class
  (greedy failure).
- **P3.** Whole-host mode with cross-socket stealing beats two strictly separate
  socket pools on the large inputs. Falsifier: equal within CI. Then the simpler
  separate pools win, and cross-socket stealing is dropped.

## 4. GPU worth (question 2)

### 4.1 What the previous regime measured [data]

All FP64, P100, previous-regime code. Batch sweeps are resident unless noted;
1–7 repetitions on a shared host with load not recorded.

| Operation | Batch | CPU | P100 | Source |
|---|---|---|---|---|
| Bicubic full jet (position, 1st, 2nd) | 1 / 4,096 / 1,048,576 | CPU1 0.00075 / 1.388 / 496.7 ms | 0.073 / 0.079 / 6.115 ms | `results/iteration1-2026-10-13/REPORT.md:41-43` |
| Same, host to host, pageable | 1,048,576 | CPU24 169.0 ms | 85.2 / 73.3 ms | same `:21-32` |
| Seeded projection | 1 / 256 / 65,536 | CPU1 0.0032 / 0.830 / 162.0 ms | 0.285 / 0.429 / 1.910 ms | `results/projection-2026-10-14/REPORT.md:44-48` |
| Bernstein discovery | 256 / 65,536 | CPU24 4.54 / 1,255.98 ms | ~5.9 / 34.75 ms | `results/discovery-2026-10-14/REPORT.md:15-18, 59-63` |
| Pair shortlist, reused index | 879,840 pairs | CPU24 9.631 ms | 7.776 / 6.920 ms | `results/pairs-2026-10-14/REPORT.md:110-113` |
| Edge-use CSR construction stages | 491,520 edges | ordered 5.58, permuted 97.8 ms | 0.317, 1.419 ms | `results/adjacency-2026-10-14/REPORT.md:135-138` |
| Whole adjacency op incl. transfers | same | ordered 8.19 ms | 17.3 / 17.0 ms | same `:119-122` |
| OM10 complete meshing | 9,162 faces | CPU12 847.7 s | P1001+CPU12 845.8 s | `results/om10-watch-2026-10-17/REPORT.md:61` |
| Display, 4K assembled frame | — | 1.814 s | 0.211 s | same `:106-107` |
| Every complete cold job under ~1 s | small | CPU wins | engine open 0.2–0.6 s dominates | feature, curve, trimmed-face reports |

Shape of the evidence:

- Regular FP64 batches win on the P100 by 10–36× resident, from about 256–4,096
  items upward.
- Transfers through pageable memory erase much of that win host to host (1.98×
  at 10⁶ jets).
- Context creation decides every small cold job.
- The old mesher's GPU work (bounds and display) did not move the meshing stage.

### 4.2 Break-even model [own, using §2]

- **Model.** A lane offloads N items of class s to its GPU instead of running
  them on the lane's remaining c = 11 CPU cores. It wins when:
  t0 + N·max(g, β/B) < N·p/c,
  where p is the CPU time per item on one core.
  - So N* = t0 / (p/c − max(g, β/B)), which exists only when
    p > c·max(g, β/B). Below that, the class is never offloaded.
- **Worked example: bicubic full jet** (the only class with matched numbers).
  - p = 496.7 ms / 1,048,576 = 474 ns; g = 6.115 ms / 1,048,576 = 5.8 ns
    [data].
  - β = 148 B out plus 16 B in; B = 9–10 GB/s on GPU1, so β/B ≈ 18 ns.
    GPU0 at 5.7 GB/s gives about 29 ns.
  - p/c = 43 ns. Without overlap the GPU costs 23.8 ns per item (GPU1) or
    34.6 ns (GPU0).
  - N* = 16–36 µs / (43 − 23.8) ns ≈ 830–1,900 items on GPU1, and about
    1,900–4,200 on GPU0.
  - With overlapped copies, the per-item cost falls to max(5.8, 18) = 18 ns on
    GPU1.
  - Acceleration factor against one core: α ≈ 20 (GPU1) or ≈ 14 (GPU0). One
    P100 therefore does this class about as fast as 1.2–1.7 sockets.
- **Analytic surfaces** (plane, cylinder, cone, sphere, torus closed forms).
  - CPU time per item is tens of ns. p/c is then a few ns, below β/B ≈ 7 ns
    for a 64-byte vertex (FP64 position and normal plus UV) on GPU1.
  - **Never offload.** By earlier lanes' counts, 92% of OM10 faces are
    analytic (growth clusters C4).
- **Watch scale** [own].
  - About 1.4·10⁵ output vertices × 474 ns ≈ 0.07 CPU-s, even if every vertex
    cost a B-spline jet.
  - NVIDIA spends 22.9 CPU-s, or 98.8 µs per triangle.
  - Vertex evaluation is therefore not where per-face time goes. Sizing,
    subdivision tests, trims and triangulation are. Only the part of those that
    is regular, batchable FP64 over control nets is a GPU candidate:
    span second-difference bounds, slab tests and lattice cell classification
    (design §6.1–6.2).

### 4.3 Hypotheses with falsifiers (not conclusions)

- **G0 (watch scale).** On a one-lane run of the watch at ε = 0.005 mm, CPU+GPU
  meshing time equals CPU-only within the CI.
  - Falsifier: the GPU arm faster by more than the CI's half-width, across ≥ 10
    cold processes.
  - If G0 holds, the GPU's role at watch scale is display only, and the GPU flag
    in the arena should show no main effect on the watch.
- **G1 (B-spline lattice evaluation, emission and normals).** With batches
  aggregated across faces per stage generation, GPU1 beats the lane's 11 CPU
  cores above N* ≈ 10³ items, and GPU0 above roughly twice that.
  - Measure p, g and β per class on the corpus.
  - Falsifier: measured N* above the batch sizes the corpus produces, or the
    GPU slower than CPU at the corpus's largest batches.
- **G2 (span sizing and cell classification).** Batched over all spans of all
  definitions, these profit when p per span exceeds c·max(g, β/B).
  - Falsifier: per-span CPU cost below about 11× the per-span transfer cost.
    With β ≈ 16 control points × 32 B = 512 B per bicubic rational span
    (u,v-ordered xyzw), that is about 600 ns on GPU1.
- **G3 (analytic classes).** α < 1 for every analytic class.
  - Falsifier: any analytic class with p > c·β/B.
- **G4 (context).** With context creation overlapped with ingress, the visible
  cold-process cost of the GPU arm is max(0, 0.21–0.29 s − ingress time).
  - Falsifier: the cold GPU arm's stage times exceed the CPU arm's by more than
    that.
- **G5 (FP64 rate).** The P100's 1/2-rate FP64 (4.1–4.6 TFLOP/s measured, about
  15–17 sockets) means FP32 is not needed for speed on this GPU. The FP32 flag
  matters only for Metal and for memory-bound kernels.
  - Falsifier: an FP32 arm faster by more than the CI on a compute-bound GPU
    stage.

## 5. Intra-face split threshold from T∞ (question 3)

- **Derivation** [own, from Graham and Blumofe–Leiserson].
  - For a lane of P CPU workers, greedy and work-stealing schedules give
    T_P ≤ W/P + (1 − 1/P)·T∞ (Graham, deterministic) and
    E[T_P] ≤ W/P + O(T∞) (work stealing).
  - For makespan within (1 + δ) of perfect, (1 − 1/P)·T∞ ≤ δ·W/P is
    sufficient, that is T∞ ≤ δ·W/(P − 1).
  - In our shallow graph T∞ ≈ max_f (e_f + w_f): the edge batches feeding face
    f plus the face's own work. Therefore:
- **Split rule.** Split face f into pieces when e_f + w_f > τ = δ·W/(P − 1).
  - Pieces are lattice clusters and band components (growth clusters C1–C2).
  - Each piece, and the face's residual serial part (2:1 balance, node
    numbering, the largest band component), must be ≤ τ.
  - Default δ = 0.1, P = 12 in lane mode or 24 in whole-host mode.
- **Lower limit.** Never split below g_min = o/η, with o the per-task overhead
  and η = 1% the tolerated overhead fraction.
  - From §2.3 and §2.8, o ≈ 1 µs, so g_min ≈ 0.1 ms. The quiet measurement
    gives 1.0 µs for a central mutex queue at 12 threads and 0.54 µs for an
    atomic counter. Per-worker deques should do better; the arena measures o
    for the real scheduler.
  - Clusters also carry seam cost, which H1′ bounds at ≤ 10% of emission.
    Pieces are therefore at least max(g_min, 10·seam cost).
- **Numbers.**
  - Lane mode at NVIDIA-class work for the watch (W ≈ 23 CPU-s): τ ≈ 0.21 s,
    about 80× NVIDIA's mean 2.5 ms per face.
  - Single-definition remesh of a 2 s definition: τ ≈ 18 ms. Its largest faces
    split, which is the single-part latency case the growth-cluster lane
    named.
  - The rule depends only on estimates of the actual input, not on source IDs,
    so it complies with the no-overfitting rule.
- **GPU batches.** The same τ bounds a GPU batch's round trip, because a batch
  sits on the critical path while the CPU waits for its results. Batches above
  τ are cut into ≤ τ chunks on two streams.
- **What is unknown.** The per-face cost distribution has never been recorded
  (audit §2; growth clusters §6). Whether any watch or corpus face exceeds τ is
  the measurement that settles whether splitting ships in the default.

## 6. NUMA (question 4)

- **First touch versus binding.**
  - First touch places a page on the touching thread's node.
  - AutoNUMA then migrates it toward whoever reads it. 2 GiB moved in about
    4.3 s, with read bandwidth collapsing to 1.9 GB/s during the move (§2.4).
  - With AutoNUMA on and no root, first touch is a starting point, not a
    placement.
  - **Rule:** arenas, staging and outputs are allocated with explicit
    `mbind`/`set_mempolicy(MPOL_BIND)` to the lane's node. Pages so bound did
    not migrate [meas].
  - The process-level equivalent for measurement runs is `numactl
    --membind=N --cpunodebind=N`.
- **Per-node pools and arenas.**
  - One arena set per node (oneTBB's one-arena-per-NUMA-node pattern [src]).
  - Per-task scratch comes from the worker's node.
  - Cross-node stealing takes tasks, not memory: outputs go to the thief's
    node.
- **GPU affinity.**
  - Each GPU is driven by a thread on its own node, and its pinned staging is
    on that node.
  - Measured penalty for driving GPU1 from node 0: H2D 9.07 vs 10.51 GB/s
    (−14%), D2H 7.91 vs 8.95 GB/s (−12%), bidirectional 15.6 vs 16.8 GB/s.
  - GPU0's x8 link hides any node effect (5.7–5.8 GB/s either way).
  - Pageable transfers are about 3 GB/s everywhere, so all staging is pinned
    (`cudaHostAlloc`). Locked pages are not migrated by AutoNUMA.
- **SMT.**
  - Adds nothing to FP64 throughput (261 vs 269 GFLOP/s) or memory bandwidth on
    this host [meas].
  - Whether it helps the branchy, latency-bound CPU stages (band CDT,
    topology) is not established by the literature read. It is an arena flag:
    12 vs 24 workers per socket.
- **Where NUMA does not matter.** In lane mode everything lives on one node.
  NUMA matters only in whole-host mode and for the GPU driver thread.

## 7. Measurement protocol (question 5)

Every rule below names the mechanism that emits the data. It is one layer: each
timing row carries its own context. There is no second validation system.

1. **Scopes.**
   - **Cold** means a fresh process. It includes CUDA context creation, module
     load, source decode, all caches empty in-process, and file pages as found
     (page-cache dropping needs root, so each row records whether the source
     file was read in the previous 60 s).
   - **Warm** means a second query in the same process after a full drain, with
     its retained caches named (source decode, carrier cache, GPU context,
     resident tables).
   - Cold and warm are reported side by side. Warm never replaces cold
     (existing doctrine; gpu-lifetime study).
2. **Stages.** Import, conversion and preparation, meshing, occurrence emission,
   and export are timed separately (design §2 goal 1). NVIDIA's OCCT healing
   lands in its import stage.
3. **Accounting (D-measure).**
   - Mesh work is accounted **per definition**: each definition is meshed once,
     and its time is charged once.
   - Placement or emission is accounted **per occurrence**.
   - Both totals are reported: Σ_definitions mesh + Σ_occurrences emit.
   - NVIDIA's 22.9 CPU-s is per definition (`SmApiTessellate` once per
     definition per worker; SmTess study §4) and is compared against owned
     per-definition mesh work.
   - Definitions meshed more than once (per-scale meshes under D-placements)
     are listed and charged as extra definition work.
   - The instancing key itself belongs to G9.
4. **Timers.**
   - `CLOCK_MONOTONIC` wall per stage, and CPU-seconds from
     `CLOCK_PROCESS_CPUTIME_ID` at the same boundaries, so CPU/wall gives
     effective parallelism per stage (audit §5 item 1).
   - CUDA events for device spans, on owned streams.
   - Timer overhead stays below 5% of the interval [src Hoefler–Belli §4.2].
5. **Repetitions and statistics.**
   - Headline comparisons: ≥ 10 fresh processes per (input, configuration,
     lane), reported as median with a rank-based 95% CI of the median, plus min
     and p90 [src Hoefler–Belli rules 5–7, Le Boudec ranks via §3.1.3].
     n = 10 gives ranks 2–9; n = 5 gives only min–max at 94% coverage.
   - Arena corpus runs: ≥ 3 processes per input. Corpus summaries are sums of
     costs, never averaged ratios (rule 4); a geometric mean of ratios is used
     only when costs are unavailable.
   - Normality is never assumed (rule 6).
   - Microbenchmarks of deterministic kernels report the minimum as the location
     estimator, because timing noise is non-negative [src Chen–Revels §III].
   - Comparisons use non-overlapping CIs, or say "not distinguishable"
     (rule 7).
6. **Bias control** [src Mytkowicz 2009; Curtsinger–Berger 2013].
   - Configurations run interleaved in a randomised order, not in blocks.
   - Each repetition pads the environment by a random 0–4 KiB.
   - Binary hashes are recorded.
   - Layout re-randomisation (STABILIZER) is not adopted. Setup randomisation is
     the one mechanism. A second, layout-level mechanism is added only if
     ablation effects fall inside the setup-randomisation spread.
7. **Process isolation and pinning.**
   - Each run is its own process under `numactl --cpunodebind=N --membind=N`,
     with `CUDA_VISIBLE_DEVICES` set to the lane's local GPU.
   - Never two arena runs on one lane at once.
   - A reference configuration repeats on both lanes. A lane difference beyond
     the CI is attributed first to the known GPU0 x8 link (§2.6), and to
     interference only after that.
8. **Context per row** (owned `ctx.sh`, §A), before and after every run:
   - the lane: CPU set, memory node, GPU;
   - load average;
   - other processes ≥ 5% CPU, and their sum;
   - GPU utilisation and compute processes;
   - free memory per node and memory PSI;
   - governor, SMT, AutoNUMA and THP state;
   - cold or warm.
   - The operator's rule: every timing emits its lane and competing load.
9. **Frequency.**
   - Governor left as found (schedutil), per the open operator question.
   - Delivered clock is measured, not read from `scaling_cur_freq`. A 50 ms
     dependent-add probe runs on the lane's cores before and after each run
     and writes GHz to the row. Snapshots of `scaling_cur_freq` between runs
     ranged 1.2–3.5 GHz while delivered clock under load was 2.9–3.1 GHz
     (§2.5).
10. **Quiet and loaded.**
    - A run is **quiet** when, for three checks a minute apart before it:
      1-minute load ≤ 4.5, other processes ≤ 150% CPU, and no foreign GPU
      process.
    - Headline numbers come from quiet runs. Loaded runs are labelled as such
      and never mixed into a quiet summary.
11. **NVIDIA at its strongest (steel-man).**
    - NVIDIA's concurrency unit is the single-threaded process (SmTess study
      §4).
    - Its arm on a lane is W worker processes, each pinned with `taskset` to one
      physical core of that socket, with memory bound to the node.
      - W ∈ {12, 24}, both measured; the fastest valid is kept.
      - Definitions are assigned to workers by **LPT on NVIDIA's own measured
        per-definition `tessellate` times** from a calibration run, not by face
        count. Face count is a poor proxy: 1713 has 438 faces and costs 2.0 s,
        while 1808 has 1,112 faces and costs 1.7 s.
    - The two-socket arm (24 or 48 workers) is compared only with owned
      whole-host mode.
    - NVIDIA's `tbb::parallel_for` USD path (`SmApiUsdTessellate.cpp:276-292`)
      is an additional arm if it produces valid output.
    - Its fastest valid arm, over input arms × healing policies × worker counts,
      is its time (design §9.6).
12. **Fairness of hardware.** The owned lane (socket plus GPU) is compared with
    NVIDIA on the same socket. NVIDIA has no GPU path, which is NVIDIA's
    limitation, not ours to remove. CPU-only owned results are reported beside
    it.

## 8. Where the literature does not say, and the measurement that would

| Question | Literature | Measurement that settles it |
|---|---|---|
| Per-face cost distribution and the largest face's share against τ | Not available for B-rep meshing (lifecycle map S18: no peer-reviewed faces/s figures) | Per-face predicted and measured work on watch, NIST/CAx-IF and ABC samples once a-priori sizing exists |
| SMT for branchy CDT/topology stages | Silent for this workload | Arena flag: 12 vs 24 workers per socket |
| Cost of the GPU driver thread on an SMT sibling | Silent | Lane run with the driver on a sibling vs on a dedicated core |
| α per stage class (p, g, β) | Only for the previous regime's bicubic evaluator | Per-class calibration run on the corpus (§3.4) |
| Whether HeteroPrio's spoliation helps with 12 CPUs : 1 GPU | Survey: helps most when counts differ, as here; no B-rep data | P2 in the arena |
| AutoNUMA effect on whole-host mode with bound arenas | Kernel and NVIDIA docs recommend binding; no quantified B-rep case | Whole-host run with and without binding (the only probe here is 2 GiB, one thread) |
| Frequency sensitivity under schedutil versus performance | Operator question; no root | One quiet pass with the performance governor if the operator allows |

## 9. Return items

### Defects found (file and line; the lane does not fix them)

1. `examples/render_om10_nvidia.py:107-112` assigns definitions to NVIDIA workers
   by face count, and no NVIDIA or owned watch run pins CPUs or memory. There is
   no `taskset`/`numactl`/affinity in `examples/`, `native/` or `python/`; only
   `scripts/benchmark_direct_cdt.py:42` pins. Published NVIDIA times are
   therefore not its strongest arm: 1808 alone with 1,112 faces on worker 0 is a
   face-count artefact. Owned watch timings ran unpinned under AutoNUMA. This is
   a measurement defect against the steel-man rule.
2. Hardware, not code: GPU0's PCIe link trains at Gen3 x8 (maximum x16). It may
   be the slot wiring or a seating or training fault. It needs physical
   inspection by the operator.

### Operator questions

1. May arena runs set the governor to `performance`? (Open since the campaign
   plan; needs root.)
2. May AutoNUMA be disabled host-wide for arena runs (`kernel.numa_balancing =
   0`, needs root)? Explicit binding neutralises it for our pages, but not for
   NVIDIA's processes unless they run under `numactl --membind` too, which the
   protocol does.
3. GPU0's x8 link: inspect or move the card, or accept the asymmetry and record
   it per lane?
4. Persistence mode is off. Measured effect is about 20 ms per process, so this
   lane does not ask to change it. Recorded only.

### Shopping items

See the section below.

## For SHOPPING_LIST.md

- Topcuoglu, Hariri, Wu, "Performance-effective and low-complexity task
  scheduling for heterogeneous computing", IEEE TPDS 13(3), 2002. Full text is
  not openly available; HEFT is read through the survey §6.1.1 and StarPU.
- Beaumont, Eyraud-Dubois, Kumar, "Approximation proofs of a fast and efficient
  list scheduling algorithm for task-based runtime systems on multicores and
  GPUs", IPDPS 2017 (HAL hal-01386174). HAL served a bot-check page to the
  fetch; HeteroPrio is read through the survey §5.1.3.
- Augonnet, Thibault, Namyst, "Automatic calibration of performance models on
  heterogeneous multicore architectures", HPPC 2009 (HAL inria-00421333).
  Same HAL block. History-based models are read through the StarPU paper.
- Kalibera and Jones, "Rigorous benchmarking in reasonable time", ISMM 2013. The
  Kent repository link returned HTML, not the PDF.
- Intel's E5-2697 v2 specification (all-core turbo bins), to interpret delivered
  clock. Not fetched; delivered clock is measured directly instead.

## A. Probes and reproduction

All in `lanes/M4-G8/` on the fleet host, owned by this lane, not tracked.
Source SHA-256:

| File | Purpose | SHA-256 |
|---|---|---|
| `cpu_probe.c` | pthreads; modes `bw` (STREAM-style copy/scale/add/triad + read sum, threads pinned, parallel first touch), `lat` (random cache-line chase), `freq` (dependent integer adds), `flops` (6+6 independent AVX mul/add chains), `c2c` (atomic ping-pong), `migrate` (first touch on one CPU, stream from another, `move_pages` residency) | `26d96502891403c2578dc9eea7158a2d943065d630b03b9d96e88a986894c735` |
| `gpu_probe.cu` | modes `cold`, `xfer`, `launch`, `flops`, `hold` (the last added after pass p1; other modes unchanged) | `53bdae9152e468b990eb14f4a9d0a45b89e5a3fb64a20a80368623703d4905fe` |
| `ctx.sh` | context row per run (§7.8) | `cfd428104d20bc0b01bc398c108bfe5724233db8e8ac6a2df57b64f6ff50f868` |
| `run.sh` | context before, probe, context after | `a40da0abe5d1eb9e13cf318e15d5f8aa829b78034cbe9e8d90fad994266602a6` |
| `campaign.sh` | full and key passes | `2643f812f1cf9cff8fdad4c129439a2ccc952b485c60d03779b3ec726d5693b5` for p1 and p4; `d295c38bba68779c7972af34215ec61f0d2619bb8eca38abcf68c67f455e3618` for p2, which adds the `sched` runs |
| `cpu_probe2.c` | `cpu_probe.c` plus mode `sched` (central mutex FIFO versus atomic counter) | `334430e4fe929249e3474df3c700a1ac5e5dad4f009b1f88b35cf9e64fc5c7c4` |
| `campaign2.sh` | cold context with and without a holder process | `11c0aa019e174ae27ebb2db9c554765274bd7b225edb8b18a6929bfd0839cd61` |
| `analyze.py` | medians, rank CIs, join with context rows | `a9ca0b591fdc4b4c63ba0466ed6f58af368af340498e8a3f68ce5ce61c843e54` |
| `quietwait.sh` | waits for the quiet condition of §7.10, then runs the key pass | `d9c5b15ac2303a5f9692213adc317d29e5b63eeaaf3df03ade7794329407b6fa` |

Builds:

- `clang-18 -O2 -mavx -std=c11 -pthread cpu_probe.c`
- the pinned `nvcc -std=c++17 -O2 --cudart shared -gencode
  arch=compute_60,code=sm_60` with the toolkit's `lib` in the rpath

Passes:

- **p1** — full, loaded, 02:15–02:25 UTC;
- **p3** — cold free/held, 02:26–02:29 UTC;
- **p4** — key subset, loaded repeat, 02:33–02:36 UTC;
- **p5** — dispatch cost, heavily loaded, 02:36 UTC;
- **p2** — key subset, quiet, 03:29–03:32 UTC (§2.7–2.8).

Raw rows: `raw/<pass>-<probe>.tsv` plus `raw/context.tsv`.
