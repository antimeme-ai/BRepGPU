"""Measurement-lane and NVIDIA-harness contracts (G8 §7.5-7.11; G1 §5; G9 §0.2).

The binding oracles read the kernel's own view of a running child
(sched_getaffinity, /proc/PID/numa_maps). Context rows are checked against
load this test injects. Scope and assignment oracles are hand-derived from
authored stage tables. Fleet only: needs Linux, numactl, the built
delivered-clock probe, and for the caller and driver tests the reference
exporter and the NVIDIA producer.
"""
import csv
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from types import MappingProxyType, SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'scripts'), str(ROOT/'examples'), str(ROOT/'python')]
import measurement_lane as lane_module
from measurement_lane import (CONTEXT_COLUMNS, Lane, Recorder, bound, lane_cpus, make_lane, other_load,
                              read_topology, scan_processes, sysfs_pci_address)

REPORT = ('import os,sys\n'
          'policy=open("/proc/self/numa_maps").readline().split()[1]\n'
          'print(",".join(map(str,sorted(os.sched_getaffinity(0)))),policy)\n')
ADAPTER = Path(os.environ.get('BREPGPU_STEP_SOURCE', ROOT/'build/om10-step-source/brepgpu-step-source')).resolve()
EXPORTER = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE',
                               ROOT/'build/om10-reference-source/brepgpu-reference-source'))
PRODUCER = Path(os.environ.get('BREPGPU_OM10_NVIDIA', ROOT/'build/om10-nvidia/brepgpu-om10-nvidia'))
PACKAGE = Path(os.environ.get('BREPGPU_NVIDIA_PACKAGE', ROOT/'quarantine/nvidia-usd-brep-1.0.0-linux'))
INGRESS = Path(os.environ.get('BREPGPU_NVIDIA_INGRESS', ROOT/'build/nvidia-ingress/brepgpu-nvidia-ingress'))
CONVERTER = Path(os.environ.get('BREPGPU_NVIDIA_CONVERTER', ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-convert'))
BUSY = 'import time\nend=time.monotonic()+%s\nwhile time.monotonic()<end:pass\n'


def rows(path):
    with Path(path).open(newline='') as stream:
        return list(csv.DictReader(stream, delimiter='\t'))


def write(path, header, body):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    with Path(path).open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t')
        writer.writerow(header)
        writer.writerows(body)


def matched_fixture(root, step=ROOT/'tests/fixtures/reference-source.step'):
    """Matched inputs for an authored reference fixture (by default the two-definition one)."""
    from brepgpu.reference_source import load_reference_source
    from brepgpu.imported_assembly import assemble
    from om10_scene import View, pack_views
    from render_om10_nvidia import tsv, digest
    subprocess.run((str(EXPORTER), str(step), str(root/'control')), check=True, capture_output=True)
    assembly = assemble(load_reference_source(root/'control/source.bgocct'))
    matched = root/'matched'
    matched.mkdir()
    tsv(matched/'metadata.tsv', ('field', 'value'), (('source_sha256', digest(step)), ('tolerance_mm', .005)))
    tsv(matched/'bom.tsv', ('occurrence_id', 'source_usage_path', 'product_id', 'solid_id',
        *(f'matrix_{i}' for i in range(12))),
        ((o.id, '/'.join(map(str, o.path)), o.product_id, o.definition_id, *o.transform)
         for o in assembly.occurrences))
    tsv(matched/'explosion.tsv', ('occurrence_id', 'world_offset_x_mm', 'world_offset_y_mm', 'world_offset_z_mm'),
        ((o.id, 0., 0., 0.) for o in assembly.occurrences))
    (matched/'views.bgview').write_bytes(pack_views(tuple(
        View(4, 4, 1, float(i), (1., 0., 0., 0., 1., 0., 0., 0., 1.), (0., 0., 0.), (0., 0.), 1.)
        for i in range(2))))
    # The adapter's source of the same STEP: the legacy matched mode compares
    # every face with it (review round 2 ruling). A file the adapter refuses
    # has none, and the caller then refuses the matched inputs by name.
    subprocess.run((str(ADAPTER), str(step), str(matched/'source.bgsource')), capture_output=True)
    return step, matched


def fake_sysfs(root, nodes, cores_per_node, threads_per_core):
    """Fleet-shaped topology: node = cpu % nodes; SMT sibling = cpu + nodes*cores."""
    physical = nodes*cores_per_node
    for cpu in range(physical*threads_per_core):
        node = cpu % nodes
        core = (cpu % physical)//nodes
        topology = root/'cpu'/f'cpu{cpu}'/'topology'
        topology.mkdir(parents=True)
        (topology/'physical_package_id').write_text(f'{node}\n')
        (topology/'core_id').write_text(f'{core}\n')
    for node in range(nodes):
        (root/'node'/f'node{node}').mkdir(parents=True)
        cpus = [c for c in range(physical*threads_per_core) if c % nodes == node]
        (root/'node'/f'node{node}'/'cpulist').write_text(','.join(map(str, cpus))+'\n')


def fixture_lane(workers=None):
    node = max(read_topology())
    return node, make_lane(node, workers)


class LaneSelection(unittest.TestCase):
    def test_workers_take_one_cpu_per_physical_core_before_any_sibling(self):
        with tempfile.TemporaryDirectory() as d:
            fake_sysfs(Path(d), nodes=2, cores_per_node=4, threads_per_core=2)
            topology = read_topology(Path(d))
            # Node 1: cores {1,9},{3,11},{5,13},{7,15}.
            self.assertEqual(lane_cpus(topology, 1, 4), (1, 3, 5, 7))
            self.assertEqual(lane_cpus(topology, 1, 6), (1, 3, 5, 7, 9, 11))
            self.assertEqual(lane_cpus(topology, 0, 8), (0, 2, 4, 6, 8, 10, 12, 14))
            with self.assertRaisesRegex(ValueError, 'do not fit'):
                lane_cpus(topology, 0, 9)

    def test_lane_never_escapes_the_caller_affinity(self):
        with tempfile.TemporaryDirectory() as d:
            fake_sysfs(Path(d), nodes=2, cores_per_node=4, threads_per_core=2)
            topology = read_topology(Path(d))
            # Published NVIDIA runs had only node 0's primary threads.
            allowed = {0, 2, 4, 6}
            self.assertEqual(make_lane(0, 4, topology=topology, allowed=allowed).cpus, (0, 2, 4, 6))
            with self.assertRaisesRegex(ValueError, 'outside this process affinity'):
                make_lane(0, 5, topology=topology, allowed=allowed)

    def test_unreadable_topology_is_a_named_refusal(self):
        with tempfile.TemporaryDirectory() as d:
            fake_sysfs(Path(d), nodes=1, cores_per_node=2, threads_per_core=1)
            (Path(d)/'cpu/cpu1/topology/core_id').unlink()
            with self.assertRaisesRegex(ValueError, 'cannot read CPU topology'):
                read_topology(Path(d))

    def test_pci_address_keeps_its_domain(self):
        self.assertEqual(sysfs_pci_address('00000000:05:00.0'), '0000:05:00.0')
        self.assertEqual(sysfs_pci_address('00000001:AF:1F.7'), '0001:af:1f.7')

    @unittest.skipUnless(sys.platform == 'linux', 'reads the host topology')
    def test_host_lane_cpus_are_distinct_cores_of_their_node(self):
        topology = read_topology()
        for node in topology:
            cpus = lane_cpus(topology, node, len(topology[node]))
            siblings = set()
            for cpu in cpus:
                # Independent sysfs paths: the CPU's node link and its sibling list.
                self.assertTrue(Path(f'/sys/devices/system/cpu/cpu{cpu}/node{node}').exists())
                group = Path(f'/sys/devices/system/cpu/cpu{cpu}/topology/thread_siblings_list').read_text()
                self.assertNotIn(group, siblings)
                siblings.add(group)


class Assignment(unittest.TestCase):
    def definitions(self):
        # G8 §7.11's example: face count misorders cost.
        faces = {1808: 1112, 1713: 438, 1: 300, 2: 200}
        return tuple(SimpleNamespace(id=i, faces=range(n)) for i, n in faces.items())

    def test_lpt_by_measured_cost_not_face_count(self):
        from render_om10_nvidia import assignments
        seconds = {1808: 1.7, 1713: 2.0, 1: 1.0, 2: 0.8}
        groups = assignments(self.definitions(), 2, seconds)
        # LPT by cost: 1713 -> w0 (2.0); 1808 -> w1 (1.7); 1 -> w1 (2.7); 2 -> w0 (2.8).
        self.assertEqual(tuple(tuple(d.id for d in g) for g in groups), ((1713, 2), (1808, 1)))
        self.assertAlmostEqual(max(sum(seconds[d.id] for d in g) for g in groups), 2.8)
        # The uncalibrated face-count basis gives the 3.8 s makespan the protocol rejects.
        faces = assignments(self.definitions(), 2)
        self.assertAlmostEqual(max(sum(seconds[d.id] for d in g) for g in faces), 3.8)

    def calibration(self, d, expected, pins, times, **changes):
        from render_om10_nvidia import CALIBRATION_FIELDS
        metadata = {**{k: expected[k] for k in CALIBRATION_FIELDS}, 'caller_complete': 1, **changes.get('metadata', {})}
        write(d/'metadata.tsv', ('field', 'value'), metadata.items())
        write(d/'worker-exits.tsv', ('worker', 'exit_code'), changes.get('exits', ((0, 0), (1, 0))))
        write(d/'runtime-pins.tsv', ('component', 'sha256'), changes.get('changed_pins', pins))
        write(d/'definition-times.tsv', ('definition_id', 'status', 'weight_cpu_seconds'),
              ((i, changes.get('status', {}).get(i, 'PASS'), t) for i, t in times.items()))

    def test_calibration_refusals_and_the_w24_allowance(self):
        from render_om10_nvidia import HarnessRefusal, calibration_weights
        source = SimpleNamespace(definitions=self.definitions())
        expected = dict(source_sha256='100', heal=1, chord_height_mm=.005, curve_angle_degrees=0.,
                        surface_angle_degrees=0., reference_preparation='periodic-plane',
                        ingress_arm='occt-no-shape-process', workers=12, worker_placement='pinned')
        w24 = dict(expected, workers=24, worker_placement='floating')
        pins = (('producer', 'a'*64), ('probe', 'b'*64))
        times = {1808: 1.7, 1713: 2.0, 1: 1.0, 2: 0.8}
        cases = (
            ({}, expected, 'own_settings'),
            # A 24-worker arm may use the uncontended 12-worker pinned run, and only that.
            ({}, w24, 'w12_pinned_for_w24'),
            (dict(metadata={'worker_placement': 'floating'}), w24, 'calibration workers'),
            (dict(metadata={'worker_placement': 'floating'}), expected, 'calibration worker_placement'),
            (dict(metadata={'workers': 24}), expected, 'calibration workers'),
            (dict(metadata={'heal': 0}), expected, 'calibration heal'),
            (dict(metadata={'ingress_arm': 'occt-default-read'}), expected, 'calibration ingress_arm'),
            (dict(metadata={'chord_height_mm': .025}), expected, 'calibration chord_height_mm'),
            # A hash compared as a number would let '1e2' pass for '100'.
            (dict(metadata={'source_sha256': '1e2'}), expected, 'calibration source_sha256'),
            (dict(metadata={'caller_complete': 0}), expected, 'did not complete'),
            (dict(exits=((0, 0), (1, 1))), expected, 'did not exit 0'),
            (dict(changed_pins=(('producer', 'c'*64), ('probe', 'b'*64))), expected, 'runtime pins differ'),
            (dict(status={1: 'FAIL'}), expected, 'did not PASS'),
        )
        for changes, run, outcome in cases:
            with self.subTest(changes=changes, run=run['workers']), tempfile.TemporaryDirectory() as d:
                self.calibration(Path(d), expected, pins, times, **changes)
                if outcome in ('own_settings', 'w12_pinned_for_w24'):
                    weights, _, kind = calibration_weights(Path(d), source, run, pins)
                    self.assertEqual((weights, kind), (times, outcome))
                else:
                    with self.assertRaisesRegex(HarnessRefusal, outcome):
                        calibration_weights(Path(d), source, run, pins)
        for bad in (0., -1., float('inf')):
            with self.subTest(weight=bad), tempfile.TemporaryDirectory() as d:
                self.calibration(Path(d), expected, pins, {**times, 2: bad})
                with self.assertRaisesRegex(HarnessRefusal, 'finite and positive'):
                    calibration_weights(Path(d), source, expected, pins)
        with tempfile.TemporaryDirectory() as d:
            self.calibration(Path(d), expected, pins, {i: t for i, t in times.items() if i != 2})
            with self.assertRaisesRegex(HarnessRefusal, 'cover exactly'):
                calibration_weights(Path(d), source, expected, pins)


class Scopes(unittest.TestCase):
    """Hand-derived owners. NVIDIA: process start, context setup, converter,
    import, tessellation, its rectangle call, object teardown and process exit.
    Listed apart: a refused first conversion (NVIDIA's failed attempt) and our
    preparation, charged as required only where it changed the input."""

    def test_definition_owners_and_headline(self):
        from render_om10_nvidia import definition_times, nvidia_scopes
        with tempfile.TemporaryDirectory() as d:
            out = Path(d)
            producer = ('definition_id', 'stage', 'seconds', 'process_cpu_seconds')
            write(out/'worker-00/stages.tsv', producer, (
                (0, 'process_main_start_monotonic', 100.2, ''), (0, 'worker_context_plugin_setup', .5, .5),
                (7, 'copy_prepared_usd', .1, .1), (7, 'import_usd_brep_heal', 1., 1.), (7, 'tessellate', 2., 2.),
                (7, 'official_rectangle_triangulation_nested_in_extract', .25, ''),
                (7, 'extract_f64_corner_mesh', .75, .75), (7, 'write_genuine_mesh', .2, .2),
                (7, 'definition_complete_through_object_teardown', 4.6, 4.5), (0, 'main_body', 5.6, 5.5),
                (0, 'process_main_end_monotonic', 105.8, '')))
            write(out/'worker-01/stages.tsv', producer, (
                (0, 'process_main_start_monotonic', 200.3, ''), (0, 'worker_context_plugin_setup', .5, .5),
                (8, 'copy_prepared_usd', .1, .1), (8, 'import_usd_brep_heal', 3., 3.), (8, 'tessellate', 1., 1.),
                (8, 'extract_f64_corner_mesh', .1, .1), (8, 'write_genuine_mesh', .1, .1),
                (8, 'definition_complete_through_object_teardown', 4.5, 4.4), (0, 'main_body', 5.5, 5.4),
                (0, 'process_main_end_monotonic', 205.8, '')))
            for worker, id in ((0, 7), (1, 8)):
                write(out/f'worker-{worker:02d}/definitions.tsv', ('definition_id', 'status'), ((id, 'PASS'),))
            preparation = ('definition_id', 'stage', 'seconds', 'exit', 'cpu_seconds', 'changed_input',
                           'conversion_seconds', 'process_start_seconds', 'process_exit_seconds')
            # Converter rows: whole process wall, and its in-process conversion span.
            write(out/'preparation-worker-00/stages.tsv', preparation, (
                (7, 'convert_original', .3, 1, .3, '', .2, .06, .04), (7, 'prepare_periodic_copy', .7, 0, .7, 1, '', '', ''),
                (7, 'convert_periodic_copy', .2, 0, .2, '', .15, .03, .02),
                (7, 'prepare_plane_envelopes', 1., 0, 1., 1, '', '', ''),
                (0, 'genuine_sdk_prepared_batch', 6., 0, 6., '', '', '', ''),
                (0, 'complete_worker', 9., 0, 9., '', '', '', '')))
            write(out/'preparation-worker-01/stages.tsv', preparation, (
                (8, 'converter_service_failure', .07, -1, '', '', '', '', ''),
                (8, 'convert_original', .4, 0, .4, '', .3, .06, .04), (8, 'prepare_plane_envelopes', .1, 0, .1, 0, '', '', ''),
                (0, 'genuine_sdk_prepared_batch', 6.2, 0, 6.2, '', '', '', ''),
                (0, 'complete_worker', 7., 0, 7., '', '', '', '')))
            # Worker 0's long-lived converter server: ready .05 s after launch, closed after 1.0 s
            # of which .5 s answered jobs (.5 s idle while our own preparation ran), reaped .05 s later.
            write(out/'preparation-worker-00/converter-process.tsv',
                  ('launch_monotonic', 'main_start_monotonic', 'ready_monotonic', 'close_monotonic',
                   'main_end_monotonic', 'reaped_monotonic', 'jobs', 'job_round_trip_seconds', 'failed_request_seconds',
                   'ended'), ((50., 50.03, 50.05, 51.05, 51.08, 51.1, 2, .5, 0., 'closed'),))
            # The preparation worker launches and reaps the producer on CLOCK_MONOTONIC.
            write(out/'preparation-worker-00/producer-process.tsv', ('launch_monotonic', 'reaped_monotonic'), ((100., 106.),))
            write(out/'preparation-worker-01/producer-process.tsv', ('launch_monotonic', 'reaped_monotonic'), ((200., 206.2),))
            groups = ((SimpleNamespace(id=7),), (SimpleNamespace(id=8),))
            definitions, workers = definition_times(out, groups, True, {})
        by_id = {r['definition_id']: r for r in definitions}
        # 7: stages measured .1+1+2+.75+.2 = 4.05, so teardown 4.6-4.05 = .55. NVIDIA:
        #    1 import + 2 tessellate + .25 rectangle + .55 teardown + .15 periodic conversion span = 3.95.
        #    The refused first conversion's span .2 is a failed attempt; converter process overhead
        #    (.3-.2) + (.2-.15) = .15 is ours; required .7 + 1.0 = 1.7; output .1 + .5 + .2 = .8.
        #    Weight CPU: 3 + .25 + (4.5-4.05) + .15 + 1.7 = 5.55.
        row = by_id[7]
        for key, value in (('nvidia_seconds', 3.95), ('nvidia_teardown_seconds', .55), ('nvidia_failed_attempt_seconds', .2),
                           ('required_input_seconds', 1.7), ('input_preparation_as_run_seconds', 1.7),
                           ('ours_converter_process_seconds', .15), ('ours_output_seconds', .8),
                           ('weight_cpu_seconds', 5.55)):
            self.assertAlmostEqual(float(row[key]), value, msg=key)
        # 8: teardown 4.5-4.3 = .2; NVIDIA 3 + 1 + .2 + .3 conversion span = 4.5; converter overhead .1.
        #    The Plane step was a byte-identical passthrough: not required, but in the as-run
        #    preparation. Weight 4 + .1 + .3.
        row = by_id[8]
        for key, value in (('nvidia_seconds', 4.5), ('nvidia_teardown_seconds', .2), ('nvidia_failed_attempt_seconds', 0.),
                           ('required_input_seconds', 0.), ('input_preparation_as_run_seconds', .1),
                           ('ours_converter_process_seconds', .1+.07), ('ours_output_seconds', .3),
                           ('weight_cpu_seconds', 4.4)):
            self.assertAlmostEqual(float(row[key]), value, msg=key)
        # Workers: start = main start - launch (.2, .3); exit = reap - main end (.2, .4);
        # bookkeeping = body - setup - definitions.
        w0, w1 = workers
        for key, value in (('after_main_return_seconds', .2), ('bookkeeping_seconds', .5), ('nvidia_seconds', 4.85)):
            self.assertAlmostEqual(float(w0[key]), value, msg='w0 '+key)
        for key, value in (('after_main_return_seconds', .4), ('bookkeeping_seconds', .5), ('nvidia_seconds', 5.7)):
            self.assertAlmostEqual(float(w1[key]), value, msg='w1 '+key)
        scopes = {name: (owner, seconds, marked, basis) for name, owner, seconds, basis, marked in
                  nvidia_scopes(definitions, workers, 2.0, 7.0)}
        # Headline = ingress 2.0 + NVIDIA-only makespan max(4.85, 5.7); our preparation is listed, not added.
        for key, value in (('nvidia_only_worker_makespan', 5.7), ('nvidia_headline', 7.7),
                           ('nvidia_headline_excluding_ingress', 5.7), ('nvidia_failed_attempt_sum', .2),
                           ('required_input_preparation_sum', 1.7), ('input_preparation_as_run_sum', 1.8),
                           ('ours_converter_process_overhead_sum', .25+.07+.1), ('converter_service_idle_sum', .5),
                           ('nvidia_with_required_input_worker_makespan', 6.55), ('nvidia_batch_wall', 7.0),
                           ('as_run_total', 9.0), ('nvidia_teardown_sum', .75), ('nvidia_after_main_return_sum', .6),
                           ('nvidia_exec_to_main_sum', .5)):
            self.assertAlmostEqual(scopes[key][1], value, msg=key)
        self.assertEqual({n for n, s in scopes.items() if s[2]}, {'nvidia_ingress_process_wall', 'nvidia_only_worker_makespan'})
        self.assertIn('requires our preparation', scopes['nvidia_headline'][3])
        self.assertAlmostEqual(float(w0['ours_converter_service_seconds']), .1)


class Selection(unittest.TestCase):
    def row(self, arm, heal, status, seconds, quiet='measured_quiet', basis='measured_definition_cpu_seconds',
            scope='fresh_source_nominal_request_both_views'):
        from benchmark_om10_nvidia import COLUMNS
        base = dict(zip(COLUMNS, (arm, 'both', scope, '1', status, seconds, '99', 's', 'v', 'detail')))
        return dict(base, arm=arm, heal=str(heal), assignment_basis=basis, quiet_status=quiet, representative='1',
                    finite_quality='not_applicable')

    def test_quiet_calibrated_valid_arms_with_faster_unquiet_named(self):
        from benchmark_om10_nvidia import selected_outcomes
        valid = 'COMPLETE_UNQUALIFIED'
        arms = [self.row('a-heal0', 0, valid, '30'), self.row('b-heal0', 0, valid, '20', quiet='arm b not measured quiet'),
                self.row('c-heal0', 0, valid, '10', basis='source_face_count_uncalibrated'),
                self.row('d-heal1', 1, 'OURS_FAILED', '15', quiet='arm d not measured quiet'),
                self.row('e-heal1', 1, valid, '25', quiet='arm e not measured quiet'),
                dict(self.row('f-heal1', 1, valid, '1'), representative='0')]
        heal0, heal1 = selected_outcomes(arms, (0, 1), valid)
        # Quiet wins, the faster unquiet arm is named beside it, an uncalibrated arm never is.
        self.assertEqual((heal0['status'], heal0['complete_seconds']), (valid, '30'))
        self.assertIn('a-heal0, measured quiet; faster arm b-heal0 at 20 s was not measured quiet', heal0['detail'])
        self.assertIn('n=1, not a headline', heal0['detail'])
        # No quiet arm: the fastest is reported and labelled; ours_failed keeps NVIDIA's time valid;
        # a non-representative try is never chosen.
        self.assertEqual((heal1['status'], heal1['complete_seconds']), ('OURS_FAILED', '15'))
        self.assertIn('not measured quiet (no arm ran quiet)', heal1['detail'])

    def test_exporter_declarations_per_ingress_arm(self):
        from render_om10_nvidia import HarnessRefusal, exporter_declaration
        self.assertEqual(exporter_declaration('occt-no-shape-process', 'translation-only', 'x'), 'current')
        self.assertEqual(exporter_declaration('occt-no-shape-process', 'none', 'x'), 'older exporter (none)')
        self.assertEqual(exporter_declaration('occt-default-read', 'default', 'x'), 'current')
        for arm, declared in (('occt-default-read', 'translation-only'), ('occt-default-read', 'none'),
                              ('occt-no-shape-process', 'default'), ('owned-projected-pcurves', 'translation-only')):
            with self.subTest(arm=arm, declared=declared), self.assertRaises(HarnessRefusal):
                exporter_declaration(arm, declared, 'x')

    def test_our_problems_are_never_nvidia_failures(self):
        from benchmark_om10_nvidia import classify_exit, selected_outcomes
        self.assertEqual(classify_exit(3), ('HARNESS_REFUSED', 'harness_refused'))
        self.assertEqual(classify_exit(4), ('INGRESS_REFUSED', 'ingress_refused'))
        self.assertEqual(classify_exit(5), ('OURS_FAILED', 'ours_failed'))
        self.assertEqual(classify_exit(1), ('FAIL', 'failed_pipeline'))
        valid = 'COMPLETE_UNQUALIFIED'
        arms = [self.row('a-heal0', 0, 'HARNESS_REFUSED', '', scope='harness_refused'),
                self.row('b-heal1', 1, valid, '10', basis='source_face_count_uncalibrated')]
        heal0, heal1 = selected_outcomes(arms, (0, 1), valid)
        self.assertEqual((heal0['status'], heal0['scope']), ('HARNESS_REFUSED', 'harness_refused'))
        self.assertEqual((heal1['status'], heal1['scope'], heal1['complete_seconds']),
                         ('HARNESS_REFUSED', 'harness_refused', ''))
        ingress, = selected_outcomes([self.row('c-heal0', 0, 'INGRESS_REFUSED', '', scope='ingress_refused')], (0,), valid)
        self.assertEqual((ingress['status'], ingress['scope']), ('INGRESS_REFUSED', 'ingress_refused'))
        failed, = selected_outcomes([self.row('d-heal0', 0, 'FAIL', '', scope='failed_pipeline')], (0,), valid)
        self.assertEqual((failed['status'], failed['scope']), ('FAIL', 'failed_pipeline'))

    def test_finite_mode_ours_failed_needs_geometry_quality(self):
        from benchmark_om10_nvidia import finite_outcome, nvidia_valid, selected_outcomes
        self.assertEqual(finite_outcome(0, 'PASS', 24, 'a')[:2], ('PASS', 'fresh_step_both_labeled_views'))
        self.assertEqual(finite_outcome(5, 'PASS', 24, 'a')[:2], ('OURS_FAILED', 'ours_failed'))
        self.assertEqual(finite_outcome(5, 'FAIL', 24, 'a')[:2], ('FAIL', 'failed_qualification'))
        self.assertEqual(finite_outcome(0, 'FAIL', 24, 'a')[:2], ('FAIL', 'failed_qualification'))
        ours = dict(self.row('a-heal0', 0, 'OURS_FAILED', '9', scope='ours_failed'), finite_quality='FAIL')
        self.assertFalse(nvidia_valid(ours, 'PASS'))
        self.assertTrue(nvidia_valid(dict(ours, finite_quality='PASS'), 'PASS'))
        self.assertTrue(nvidia_valid(dict(ours, finite_quality='not_applicable'), 'COMPLETE_UNQUALIFIED'))
        heal0, = selected_outcomes([ours, self.row('b-heal0', 0, 'PASS', '20', scope='fresh_step_both_labeled_views')],
                                   (0,), 'PASS')
        self.assertEqual((heal0['status'], heal0['complete_seconds']), ('PASS', '20'))

    def test_shared_quiet_rule_retries_and_uses_one_estimator(self):
        from measurement_lane import measure_quietly
        quiet = dict(load1=9., foreign=10., own=0., host=10., gpu_foreign=0)
        loud = dict(quiet, foreign=900.)
        def run(plan):
            samples = iter(plan)
            calls = []
            def settle(checks, interval):
                return [next(samples)], 0.
            def attempt(index, s, window):
                calls.append(index)
                return f'try{index}', 0., index != 99
            return measure_quietly(attempt, 1, 0, 3, settle), calls
        series, calls = run([loud, quiet, quiet])
        self.assertEqual(calls, [1, 2])
        self.assertEqual([e['representative'] for e in series], [False, True])
        self.assertEqual({e['quiet_status'] for e in series}, {'measured_quiet'})
        series, calls = run([loud, loud, loud])
        # Never quiet: the first attempt represents, not the fastest of three.
        self.assertEqual((calls, [e['representative'] for e in series]), ([1, 2, 3], [True, False, False]))
        self.assertEqual({(e['quiet_status'], e['tries']) for e in series}, {('not_measured_quiet', 3)})

    def test_load_class_reads_foreign_load_not_ours(self):
        from measurement_lane import load_class
        ours = dict(load1=30., foreign=40., own=2400., host=2440., gpu_foreign=0)
        # Our own decaying load (high load average, our own CPU) does not make a run loaded.
        self.assertEqual(load_class([ours, ours], 60.), 'quiet')
        self.assertEqual(load_class([ours, dict(ours, foreign=400.)], 60.), 'loaded')
        self.assertEqual(load_class([ours], 400.), 'loaded')
        self.assertEqual(load_class([ours], 60., run_foreign=300.), 'loaded')
        self.assertEqual(load_class([dict(ours, gpu_foreign=1)], 60.), 'loaded')

    @unittest.skipUnless(sys.platform == 'linux', 'reads /proc')
    def test_live_sample_charges_our_busy_child_to_us(self):
        from measurement_lane import quiet_sample
        own = subprocess.Popen((sys.executable, '-c', BUSY % 1.6))
        try:
            time.sleep(.2)
            sample = quiet_sample(1.0)
        finally:
            own.wait()
        self.assertGreaterEqual(sample['own'], 80)
        # Our child is in own, so foreign is host minus at least that much.
        self.assertLessEqual(sample['foreign'], sample['host']-sample['own']+15)


@unittest.skipUnless(sys.platform == 'linux', 'binding is a Linux kernel contract')
class Binding(unittest.TestCase):
    def test_bound_child_runs_on_its_cpu_with_memory_bound_to_its_node(self):
        node, lane = fixture_lane(1)
        output = subprocess.run(bound((sys.executable, '-c', REPORT), lane),
                                check=True, capture_output=True, text=True).stdout.split()
        self.assertEqual(output, [str(lane.cpus[0]), f'bind:{node}'])

    def script(self, directory, lane):
        path = Path(directory)/'probe.py'
        path.write_text(f'import sys;sys.path.insert(0,{str(ROOT/"scripts")!r})\n'
                        'import measurement_lane as m\n'
                        f'm.ensure_bound(m.Lane({lane.node},{lane.cpus!r}))\n'+REPORT)
        return path

    def test_program_rebinds_itself_to_its_lane(self):
        node, lane = fixture_lane(2)
        with tempfile.TemporaryDirectory() as d:
            output = subprocess.run((sys.executable, '-B', str(self.script(d, lane))), check=True,
                                    capture_output=True, text=True).stdout.split()
        self.assertEqual(output, [','.join(map(str, sorted(lane.cpus))), f'bind:{node}'])

    def test_rebinding_that_does_not_take_is_refused_not_looped(self):
        node, lane = fixture_lane(2)
        with tempfile.TemporaryDirectory() as d:
            fake = Path(d)/'numactl'
            # A numactl that drops its options and runs the command unbound.
            fake.write_text('#!/bin/sh\nwhile [ "${1#--}" != "$1" ]; do shift; done\nexec "$@"\n')
            fake.chmod(0o700)
            environment = dict(os.environ, PATH=f'{d}:{os.environ["PATH"]}')
            environment.pop(lane_module.REEXEC_MARK, None)
            result = subprocess.run((sys.executable, '-B', str(self.script(d, lane))), env=environment,
                                    capture_output=True, text=True, timeout=30)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('lane binding did not take', result.stderr)


def sleeping_probe(directory, seconds, fail=False):
    probe = Path(directory)/'probe'
    probe.write_text(f'#!{sys.executable}\nimport sys,time\ntime.sleep({seconds})\n'
                     + ('sys.exit(4)\n' if fail else
                        'print("cpu\\tghz\\tcpu_share")\n'
                        '[print(f"{c}\\t3.0\\t1.0") for c in sys.argv[2].split(",")]\n'))
    probe.chmod(0o700)
    return probe


@unittest.skipUnless(sys.platform == 'linux', 'reads /proc and runs the native clock probe')
class ContextRows(unittest.TestCase):
    def test_foreign_load_is_counted_but_never_named(self):
        node, lane = fixture_lane(2)
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'context.tsv'
            recorder = Recorder(path, lane, 'cold', (('suite', 'test'),))
            pid_file = Path(d)/'foreign.pid'
            # Foreign: setsid -f forks and exits, so the busy process is not our descendant.
            subprocess.run(('setsid', '-f', sys.executable, '-c',
                            f'import os;open({str(pid_file)!r},"w").write(str(os.getpid()))\n'+BUSY % 3.0), check=True)
            deadline = time.monotonic()+5
            while not pid_file.exists() and time.monotonic() < deadline:
                time.sleep(.05)
            foreign = int(pid_file.read_text())
            start = scan_processes()
            with recorder.timing('probe', labels=(('case', 'injected'),)):
                own = subprocess.Popen((sys.executable, '-c', BUSY % 1.5))
                time.sleep(1.5)
                own.wait()
            load = other_load(start, scan_processes())
            row, = rows(path)
        # In memory: the injected pid is the foreign process, our child is ours.
        self.assertIn(foreign, [pid for _, pid, pct in load['foreign_rows'] if pct >= 50])
        self.assertNotIn(own.pid, [pid for _, pid, _ in load['foreign_rows']])
        # Our child was reaped inside the window, so its CPU is our own cutime.
        self.assertIn(os.getpid(), [pid for _, pid, pct in load['own_rows'] if pct >= 50])
        self.assertGreaterEqual(load['foreign_cpu_percent'], 50)
        # Written: counts and CPU only; no foreign id appears anywhere in the row.
        self.assertNotIn(str(foreign), '\t'.join(row.values()))
        self.assertGreaterEqual(int(row['foreign_processes_over_threshold']), 1)
        self.assertGreaterEqual(float(row['foreign_cpu_percent']), 50)
        self.assertIn('python', row['own_tree'])
        self.assertEqual((row['context_id'], row['scope'], row['lane_node'], row['lane_cpus'], row['lane_gpu']),
                         ('probe', 'cold', str(node), ','.join(map(str, lane.cpus)), '-'))
        for key in ('clock_ghz_mean_min_max_before', 'clock_ghz_mean_min_max_after'):
            mean, low, high = map(float, row[key].split(','))
            self.assertTrue(.3 < low <= mean <= high < 6, row[key])
        for key in ('cpu_share_mean_min_before', 'cpu_share_mean_min_after'):
            mean, low = map(float, row[key].split(','))
            self.assertTrue(0 < low <= mean <= 1.01, row[key])
        self.assertEqual(row['smt'], Path('/sys/devices/system/cpu/smt/control').read_text().strip())
        self.assertEqual(row['autonuma'], Path('/proc/sys/kernel/numa_balancing').read_text().strip())
        self.assertEqual(len(row['gpu_compute_own_foreign_before'].split(',')), 2)
        self.assertEqual(row['capture_errors'], 'none')
        self.assertIn('outcome=completed', row['labels'])

    def test_capture_stays_outside_the_timed_interval(self):
        node, lane = fixture_lane(2)
        with tempfile.TemporaryDirectory() as d:
            recorder = Recorder(Path(d)/'context.tsv', lane, probe=sleeping_probe(d, .4))
            with recorder.timing('work'):
                start = time.perf_counter()
                time.sleep(.05)
                work = time.perf_counter()-start
            row, = rows(Path(d)/'context.tsv')
        self.assertLess(work, .2)
        self.assertGreaterEqual(recorder.capture_seconds, .8)
        # The process scans bracket the work, not the probes.
        self.assertLess(float(row['window_seconds']), .3)

    def test_ingress_cache_policy_drops_the_step_file(self):
        from measurement_lane import page_cache_residency, prepare_ingress_cache
        # On the lane's disk: on tmpfs the page cache is the file's storage and fadvise drops nothing.
        with tempfile.TemporaryDirectory(dir=ROOT) as d:
            kind = subprocess.run(('stat', '-f', '-c', '%T', d), capture_output=True, text=True).stdout.strip()
            if kind == 'tmpfs':
                self.skipTest('posix_fadvise drops nothing on tmpfs')
            step = Path(d)/'part.step'
            step.write_bytes(os.urandom(1 << 20))
            os.sync()
            step.read_bytes()
            self.assertGreater(page_cache_residency((step,))['part.step'], .9)
            residency = prepare_ingress_cache(step, (sys.executable,))
        self.assertLess(residency['part.step'], .1)
        self.assertIn(Path(sys.executable).name, residency)

    def test_owned_cli_runs_use_the_same_quiet_rule(self):
        node, _ = fixture_lane(1)
        with tempfile.TemporaryDirectory() as d:
            context = Path(d)/'context.tsv'
            result = subprocess.run((sys.executable, '-B', str(ROOT/'scripts/measurement_lane.py'), 'run', '--node',
                str(node), '--workers', '1', '--context', str(context), '--label', 'owned', '--quiet-checks', '1',
                '--quiet-interval', '0', '--quiet-tries', '2', '--ingress-file', str(ROOT/'tests/fixtures/apex-cone.step'),
                '--', 'true'), capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            tries = rows(context.with_suffix('.timings.tsv'))
            labels = rows(context)[0]['labels']
        self.assertIn(len(tries), (1, 2))
        self.assertEqual(sum(t['representative'] == '1' for t in tries), 1)
        self.assertTrue(all(t['tries'] == str(len(tries)) for t in tries))
        quiet = [t for t in tries if t['load_class'] == 'quiet']
        self.assertEqual([t for t in tries if t['representative'] == '1'], quiet[:1] or tries[:1])
        self.assertEqual(len(tries) == 2, tries[0]['load_class'] == 'loaded')
        self.assertIn('page_cache=apex-cone.step/', labels)

    def test_capture_never_raises_or_masks_the_timed_outcome(self):
        node, lane = fixture_lane(2)
        with tempfile.TemporaryDirectory() as d:
            recorder = Recorder(Path(d)/'context.tsv', lane, probe=sleeping_probe(d, 0, fail=True))
            with self.assertRaises(ZeroDivisionError):
                with recorder.timing('fails'):
                    1/0
            row, = rows(Path(d)/'context.tsv')
            self.assertEqual(row['clock_ghz_mean_min_max_before'], 'error')
            self.assertIn('clock:ValueError', row['capture_errors'])
            self.assertIn('outcome=raised_ZeroDivisionError', row['labels'])
            other = Path(d)/'other.tsv'
            other.write_text('a\tb\n')
            with self.assertRaisesRegex(ValueError, 'different context columns'):
                Recorder(other, lane)
        self.assertEqual(len(CONTEXT_COLUMNS), len(set(CONTEXT_COLUMNS)))



def stand_in(path, code, record=None):
    """An executable that optionally records its arguments and exits with `code`."""
    path.write_text(f'#!{sys.executable}\nimport sys\n'
                    + (f'open({str(record)!r},"w").write("\\t".join(sys.argv[1:]))\n' if record else '')
                    + f'sys.exit({code})\n')
    path.chmod(0o700)
    return path


def scope_rows(dataset):
    return {r['scope']: r for r in rows(Path(dataset)/'scopes.tsv')}


@unittest.skipUnless(sys.platform == 'linux' and EXPORTER.is_file() and INGRESS.is_file(),
                     'needs the fleet reference exporter and the minimal NVIDIA ingress')
class NvidiaCallerLane(unittest.TestCase):
    """The actual caller on the authored fixture, with a stand-in producer that
    reports the binding the kernel gave it and its parent, then fails."""

    def caller(self, root, output, producer, extra=(), environment=None):
        if not (root/'matched').exists():
            matched_fixture(root)
        return subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
            '--step', str(ROOT/'tests/fixtures/reference-source.step'), '--matched', str(root/'matched'),
            '--output', str(output), '--heal', '0', '--workers', '2', '--allow-fixture',
            '--exporter', str(EXPORTER), '--nvidia-ingress', str(INGRESS), '--producer', str(producer),
            '--package', str(PACKAGE), *extra), capture_output=True, text=True, env=environment)

    def stand_in(self, root):
        producer = root/'producer'
        producer.write_text(
            f'#!{sys.executable}\nimport os,sys\n'
            'out=sys.argv[sys.argv.index("--output-directory")+1];os.makedirs(out)\n'
            'policy=lambda p:open(f"/proc/{p}/numa_maps").readline().split()[1]\n'
            'cpus=lambda p:",".join(map(str,sorted(os.sched_getaffinity(p))))\n'
            'ids=[sys.argv[i+1] for i,a in enumerate(sys.argv) if a=="--definition"]\n'
            'paths=[sys.argv[i+2] for i,a in enumerate(sys.argv) if a=="--definition"]\n'
            'open(out+"/binding","w").write("\\t".join((cpus(0),policy("self"),cpus(os.getppid()),'
            'policy(os.getppid()),",".join(ids),",".join(paths))))\n'
            'sys.exit(1)\n')
        producer.chmod(0o700)
        return producer

    def test_workers_pinned_or_floating_on_minimal_ingress_input(self):
        topology = read_topology()
        node = max(topology)
        expected = lane_cpus(topology, node, 2)
        node_cpus = ','.join(map(str, sorted(lane_module.lane_order(topology, node))))
        for placement in ('pinned', 'floating'):
            with self.subTest(placement=placement), tempfile.TemporaryDirectory() as d:
                root = Path(d)
                output = root/'out'
                result = self.caller(root, output, self.stand_in(root),
                                     ('--lane-node', str(node), '--worker-placement', placement))
                # A producer failure is an NVIDIA-side failure (1), not ours (3, 4, 5).
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn('NVIDIA strict worker failure', result.stderr)
                assignment = rows(output/'assignments.tsv')
                self.assertEqual({r['basis'] for r in assignment}, {'source_face_count_uncalibrated'})
                for worker in (0, 1):
                    cpus, policy, parent_cpus, parent_policy, ids, paths = \
                        (output/f'worker-{worker:02d}/binding').read_text().split('\t')
                    mine = [r for r in assignment if r['worker'] == str(worker)]
                    want = str(expected[worker]) if placement == 'pinned' else ','.join(map(str, sorted(expected)))
                    self.assertEqual((cpus, mine[0]['cpus']), (want, want))
                    self.assertEqual(policy, f'bind:{node}')
                    self.assertEqual((parent_cpus, parent_policy), (node_cpus, f'bind:{node}'))
                    self.assertEqual(sorted(ids.split(',')), sorted(r['source_definition_id'] for r in mine))
                    # NVIDIA consumes the minimal ingress output, not our XDE export.
                    self.assertTrue(all(Path(p).parent == output/'nvidia-input' for p in paths.split(',')), paths)
                context = {r['context_id']: r for r in rows(output/'context.tsv')}
                self.assertEqual(set(context), {'ours_xde_export', 'nvidia_ingress', 'nvidia_batch', 'caller'})
                self.assertEqual(context['nvidia_batch']['lane_cpus'], ','.join(map(str, expected)))
                self.assertIn('outcome=raised_ValueError', context['caller']['labels'])
                overhead = {r['scope']: float(r['seconds']) for r in rows(output/'measurement-overhead.tsv')}
                self.assertGreater(overhead['context_capture_in_caller_process'], 0)
                self.assertGreater(overhead['runtime_pin_hashing_outside_caller_bracket'], 0)

    def test_minimal_ingress_matches_our_xde_export_on_the_fixture(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            step = ROOT/'tests/fixtures/reference-source.step'
            subprocess.run((str(INGRESS), '--shape-processing', 'none', str(step), str(root/'minimal')), check=True)
            subprocess.run((str(EXPORTER), str(step), str(root/'xde')), check=True, capture_output=True)
            files = rows(root/'minimal/definitions.tsv')
            self.assertEqual(len(files), 2)
            for r in files:
                self.assertEqual((root/'minimal'/r['filename']).read_bytes(), (root/'xde'/r['filename']).read_bytes())
                # Multi-solid fixture (two definitions, one reused under a rotated placement):
                # each written root is an identity-located forward solid in its own frame.
                self.assertEqual((r['shape_type'], r['identity_location'], r['orientation']), ('solid', '1', 'forward'))
            policy = {r['field']: r['value'] for r in rows(root/'minimal/policy.tsv')}
            self.assertEqual((policy['flags_supplied'], policy['operations']), ('1', '0'))
            # Default read: OCCT applies shape fixing, and on a fixture with no pcurves the
            # output differs from the translation-only read.
            cone = ROOT/'tests/fixtures/apex-cone.step'
            self.assertNotIn('PCURVE', cone.read_text())
            for policy_name in ('none', 'default'):
                subprocess.run((str(INGRESS), '--shape-processing', policy_name, str(cone), str(root/policy_name)),
                               check=True, capture_output=True)
            policy = {r['field']: r['value'] for r in rows(root/'default/policy.tsv')}
            self.assertEqual(policy['shape_processing'], 'default')
            self.assertGreater(int(policy['operations']), 0)
            self.assertIn('1', policy['operation_bits'])
            for r in rows(root/'none/definitions.tsv'):
                self.assertNotEqual((root/'none'/r['filename']).read_bytes(), (root/'default'/r['filename']).read_bytes())

    @unittest.skipUnless(PACKAGE.is_dir() and CONVERTER.is_file(), 'needs the NVIDIA package and converter')
    def test_converter_reports_its_in_process_conversion_span(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            subprocess.run((str(EXPORTER), str(ROOT/'tests/fixtures/reference-source.step'), str(root/'xde')),
                           check=True, capture_output=True)
            environment = dict(os.environ, OMNISOLID_PLUGIN_PATH=str(PACKAGE/'omniSolid/resources'))
            launch = time.monotonic()
            result = subprocess.run((str(CONVERTER), str(root/'xde/definition-41.brep'), str(root/'out.usdc')),
                                    capture_output=True, text=True, env=environment)
            reaped = time.monotonic()
            self.assertEqual(result.returncode, 0, result.stderr)
            values = dict(line.split('=', 1) for line in result.stdout.splitlines() if '=' in line)
            start, end = float(values['main_start_monotonic']), float(values['main_end_monotonic'])
            span = float(values['conversion_seconds'])
            # One clock with the launcher: launch <= main start < main end <= reap, span inside main.
            self.assertTrue(launch <= start < end <= reaped, (launch, start, end, reaped))
            self.assertTrue(0 < span <= end-start)
            sys.path.insert(0, str(ROOT/'scripts'))
            from prepare_nvidia_reference_worker import converter_times
            log = root/'converter.log'
            log.write_text(result.stdout)
            conversion, process_start, process_exit = converter_times(log, launch, reaped)
            self.assertAlmostEqual(conversion, span)
            self.assertLessEqual(process_start+span+process_exit, reaped-launch+1e-6)
            self.assertGreaterEqual(min(process_start, process_exit), 0)

    def test_byte_check_is_per_definition_and_placements_exempt_nothing(self):
        """B2-reader-export review round 1 (dispatcher ruling): the translation-only byte
        check is per definition. A definition our export corrected (a
        reference_face_orientation_raw record) takes our corrected .brep; every other
        definition must be byte-identical; a declared placement correction (P1, P2)
        exempts nothing."""
        from render_om10_nvidia import HarnessRefusal, IngressRefusal, check_nvidia_input
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root/'nvidia').mkdir()
            (root/'xde').mkdir()
            write(root/'nvidia/policy.tsv', ('field', 'value'),
                  (('shape_processing', 'none'), ('flags_supplied', 1), ('operations', 0)))
            write(root/'nvidia/definitions.tsv', ('definition_id', 'filename', 'shape_type', 'identity_location',
                                                  'orientation'), ((1, 'definition-1.brep', 'solid', 1, 'forward'),
                                                                   (2, 'definition-2.brep', 'solid', 1, 'forward')))
            for i in (1, 2):
                (root/f'nvidia/definition-{i}.brep').write_bytes(b'nvidia')
                (root/f'xde/definition-{i}.brep').write_bytes(b'nvidia')
            definitions = (SimpleNamespace(id=1, filename='definition-1.brep'), SimpleNamespace(id=2, filename='definition-2.brep'))
            placed = SimpleNamespace(definitions=definitions,
                                     adapter_revision='v; shape-processing=translation-only; raw-entity-corrections=P1,P2')
            self.assertEqual(check_nvidia_input(root/'nvidia', 'occt-no-shape-process', placed, root/'xde', INGRESS),
                             (2, 'applied', set()))
            (root/'xde/definition-1.brep').write_bytes(b'ours')
            with self.assertRaisesRegex(IngressRefusal, 'differs from our XDE export for 1 definitions we did not correct'):
                check_nvidia_input(root/'nvidia', 'occt-no-shape-process', placed, root/'xde', INGRESS)
            self.assertTrue((root/'nvidia/nvidia-input-diff.tsv').is_file())
            (root/'xde/records.tsv').write_text('stage\tseverity\tids\tmessage\n'
                                                'reference_face_orientation_raw\t1\t1 17\tcorrected\n')
            # Review round 2: our uncorrected translation of a corrected definition
            # must be the ingress's bytes; missing or different is a harness refusal.
            with self.assertRaisesRegex(HarnessRefusal, 'uncorrected translation of corrected definition 1 differs'):
                check_nvidia_input(root/'nvidia', 'occt-no-shape-process', placed, root/'xde', INGRESS)
            (root/'xde/uncorrected').mkdir()
            (root/'xde/uncorrected/definition-1.brep').write_bytes(b'nvidiA')
            with self.assertRaisesRegex(HarnessRefusal, 'uncorrected translation of corrected definition 1 differs'):
                check_nvidia_input(root/'nvidia', 'occt-no-shape-process', placed, root/'xde', INGRESS)
            (root/'xde/uncorrected/definition-1.brep').write_bytes(b'nvidia')
            self.assertEqual(check_nvidia_input(root/'nvidia', 'occt-no-shape-process', placed, root/'xde', INGRESS),
                             (1, 'applied per definition; corrected definitions [1] take our .brep', {1}))
            (root/'xde/definition-2.brep').write_bytes(b'other')
            with self.assertRaisesRegex(IngressRefusal, 'for 1 definitions we did not correct'):
                check_nvidia_input(root/'nvidia', 'occt-no-shape-process', placed, root/'xde', INGRESS)

    def converter_environment(self):
        previous = os.environ.get('OMNISOLID_PLUGIN_PATH')
        os.environ['OMNISOLID_PLUGIN_PATH'] = str(PACKAGE/'omniSolid/resources')
        return previous

    def restore(self, previous):
        if previous is None:
            os.environ.pop('OMNISOLID_PLUGIN_PATH', None)
        else:
            os.environ['OMNISOLID_PLUGIN_PATH'] = previous

    @unittest.skipUnless(PACKAGE.is_dir() and CONVERTER.is_file(), 'needs the NVIDIA package and converter')
    def test_served_conversions_answer_every_job_and_match_one_shot_bytes(self):
        sys.path.insert(0, str(ROOT/'scripts'))
        from prepare_nvidia_reference_worker import ConverterService
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            subprocess.run((str(EXPORTER), str(ROOT/'tests/fixtures/reference-source.step'), str(root/'xde')),
                           check=True, capture_output=True)
            subprocess.run((str(INGRESS), '--shape-processing', 'none', str(ROOT/'tests/fixtures/apex-cone.step'),
                            str(root/'cone')), check=True, capture_output=True)
            jobs = (root/'xde/definition-41.brep', root/'missing.brep', root/'xde/definition-395.brep',
                    root/'xde/definition-41.brep', root/'cone/definition-15.brep')
            previous = self.converter_environment()
            try:
                service = ConverterService(CONVERTER, root)
                answers = [service.convert(job, root/f'served-{i}.usdc') for i, job in enumerate(jobs)]
                service.close()
                one_shot = [subprocess.run((str(CONVERTER), str(job), str(root/f'one-shot-{i}.usdc')),
                                           capture_output=True).returncode for i, job in enumerate(jobs)]
            finally:
                self.restore(previous)
            # A refused job is answered and the same server takes the next one.
            self.assertEqual([a[0] for a in answers], [0, 1, 0, 0, 0])
            self.assertEqual(one_shot, [0, 1, 0, 0, 0])
            for i, (code, span, round_trip, _, failed) in enumerate(answers):
                self.assertEqual(failed, 0.)
                if code == 0:
                    self.assertTrue(0 < span <= round_trip)
                    # Served output, the repeated definition and the job after a refusal included,
                    # is byte-identical to one-shot output.
                    self.assertEqual((root/f'served-{i}.usdc').read_bytes(), (root/f'one-shot-{i}.usdc').read_bytes(), i)
            launch, = rows(root/'converter-process.tsv')
            self.assertEqual((launch['jobs'], launch['ended']), ('5', 'closed'))
            times = [float(launch[k]) for k in ('launch_monotonic', 'main_start_monotonic', 'ready_monotonic',
                                                 'close_monotonic', 'main_end_monotonic', 'reaped_monotonic')]
            self.assertEqual(times, sorted(times))

    def fake_server(self, root, behaviour):
        """A stand-in converter server speaking the reply-descriptor protocol."""
        server = root/f'server-{behaviour}'
        server.write_text(f"""#!{sys.executable}
import os,sys,time
from pathlib import Path
out=os.fdopen(int(sys.argv[2]),'w')
marker=Path({str(root)!r})/'died-once'
out.write('main_start_monotonic=%r\\nready\\n' % time.monotonic());out.flush()
for line in sys.stdin:
    if {behaviour!r}=='chatty':
        sys.stdout.write('OCCT says hello without a newline ');sys.stdout.flush()
    if {behaviour!r}=='always-dies' or ({behaviour!r}=='dies-once' and not marker.exists()):
        marker.touch();os._exit(1)
    if {behaviour!r}=='hangs':
        time.sleep(30)
    out.write('job\\t0\\t0.001\\t\\n');out.flush()
out.write('main_end_monotonic=%r\\n' % time.monotonic());out.flush()
""")
        server.chmod(0o700)
        return server

    def test_converter_service_failures_are_ours_and_retried_once(self):
        sys.path.insert(0, str(ROOT/'scripts'))
        from prepare_nvidia_reference_worker import ConverterService, ConverterServiceFailure
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            # OCCT/USD chatter on stdout cannot corrupt replies on their own descriptor.
            service = ConverterService(self.fake_server(root, 'chatty'), root/'.')
            self.assertEqual(service.convert(root/'a.brep', root/'a.usdc')[:2], (0, .001))
            self.assertEqual(service.convert(root/'b.brep', root/'b.usdc')[0], 0)
            service.close()
            # A server killed mid-job is replaced and the job retried once; the failed request is ours.
            (root/'once').mkdir()
            service = ConverterService(self.fake_server(root, 'dies-once'), root/'once')
            code, span, round_trip, _, failed = service.convert(root/'a.brep', root/'a.usdc')
            service.close()
            self.assertEqual((code, span), (0, .001))
            self.assertGreater(failed, 0)
            launches = rows(root/'once/converter-process.tsv')
            self.assertEqual([l['ended'] for l in launches], ['failed: EOFError', 'closed'])
            # A second failure is ours and raised, never a refusal code.
            (root/'dead').mkdir()
            service = ConverterService(self.fake_server(root, 'always-dies'), root/'dead')
            with self.assertRaisesRegex(ConverterServiceFailure, 'failed twice'):
                service.convert(root/'a.brep', root/'a.usdc')
            (root/'hang').mkdir()
            service = ConverterService(self.fake_server(root, 'hangs'), root/'hang', timeout=1)
            started = time.monotonic()
            with self.assertRaisesRegex(ConverterServiceFailure, 'failed twice'):
                service.convert(root/'a.brep', root/'a.usdc')
            self.assertLess(time.monotonic()-started, 15)
            self.assertEqual([l['ended'] for l in rows(root/'hang/converter-process.tsv')],
                             ['failed: TimeoutError', 'failed: TimeoutError'])
            with self.assertRaisesRegex(ConverterServiceFailure, 'cannot be framed'):
                ConverterService(self.fake_server(root, 'chatty'), root).convert(root/'tab\there.brep', root/'x.usdc')

    def test_mapped_item_solid_is_read_in_its_own_frame_and_placed_by_our_export(self):
        """bead .37.25: NVIDIA's input is the solid in its own frame; our XDE export, which
        refused it before, now places it through the mapped item (usage #901 at (0,5,0)).
        The STEP source adapter refuses the file (it does not read mapped items, .37.26),
        so there is no adapter source to compare faces with, and the caller refuses the
        legacy matched inputs by name (exit 3; review round 2 ruling) after our export
        and NVIDIA's ingress, on both runnable arms."""
        step = ROOT/'tests/fixtures/mapped-item-solid.step'
        for arm, policy in (('occt-no-shape-process', 'none'), ('occt-default-read', 'default')):
            with self.subTest(arm=arm), tempfile.TemporaryDirectory() as d:
                root = Path(d)
                self.assertIn('MAPPED_ITEM', step.read_text())
                subprocess.run((str(INGRESS), '--shape-processing', policy, str(step), str(root/'minimal')),
                               check=True, capture_output=True)
                only, = rows(root/'minimal/definitions.tsv')
                self.assertEqual((only['shape_type'], only['identity_location'], only['orientation']), ('solid', '1', 'forward'))
                _, matched = matched_fixture(root, step)
                bom, = rows(matched/'bom.tsv')
                self.assertEqual((bom['source_usage_path'], bom['product_id'], bom['solid_id'], bom['matrix_7']),
                                 ('901', '10', '15', '5.0'))
                result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                    '--step', str(step), '--matched', str(matched), '--output', str(root/'out'), '--heal', '0',
                    '--workers', '1', '--allow-fixture', '--ingress-arm', arm, '--exporter', str(EXPORTER),
                    '--nvidia-ingress', str(INGRESS), '--producer', str(self.stand_in(root)), '--package', str(PACKAGE)),
                    capture_output=True, text=True)
                self.assertFalse((matched/'source.bgsource').exists())
                self.assertEqual(result.returncode, 3, result.stderr)
                self.assertIn('legacy matched inputs lack the adapter source', result.stderr)
                usage, = [u for u in __import__('brepgpu.reference_source', fromlist=['x']).load_reference_source(
                    root/'out/source/source.bgocct').usages if u.id == 901]
                self.assertEqual(usage.transform[7], 5.)

    def test_default_read_arm_runs_on_our_default_export(self):
        """bead .37.19: the occt-default-read arm runs. Our exporter reads in OCCT's default
        mode and declares it, and its face map describes NVIDIA's default-read input."""
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            output = root/'out'
            result = self.caller(root, output, self.stand_in(root), ('--ingress-arm', 'occt-default-read'))
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn('NVIDIA strict worker failure', result.stderr)
            metadata = {r['field']: r['value'] for r in rows(output/'metadata.tsv')}
            self.assertEqual((metadata['ingress_arm'], metadata['exporter_shape_processing']),
                             ('occt-default-read', 'default'))
            self.assertIn('shape_process_before_transfer\t0\t0', (output/'source-export.log').read_text())
            policy = {r['field']: r['value'] for r in rows(output/'nvidia-input/policy.tsv')}
            self.assertEqual(policy['shape_processing'], 'default')
            for worker in (0, 1):
                *_, paths = (output/f'worker-{worker:02d}/binding').read_text().split('\t')
                self.assertTrue(all(Path(p).parent == output/'nvidia-input' for p in paths.split(',')), paths)

    def test_default_read_input_judgement(self):
        """bead .37.19, B2-reader-export review round 1: on the default-read arm both reads
        must run the same OCCT operations and every NVIDIA input file must be byte-identical
        to our default export; otherwise the harness refuses (our face map would not
        describe NVIDIA's input)."""
        from types import SimpleNamespace
        from brepgpu.reference_source import load_reference_source
        from render_om10_nvidia import HarnessRefusal, check_nvidia_input
        step = ROOT/'tests/fixtures/reference-source.step'
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            subprocess.run((str(EXPORTER), '--shape-processing', 'default', str(step), str(root/'xde')),
                           check=True, capture_output=True)
            subprocess.run((str(INGRESS), '--shape-processing', 'default', str(step), str(root/'nvidia')),
                           check=True, capture_output=True)
            source = load_reference_source(root/'xde/source.bgocct')
            self.assertEqual(check_nvidia_input(root/'nvidia', 'occt-default-read', source, root/'xde', INGRESS),
                             (2, 'applied: default read, every definition byte-identical', set()))
            # Other bytes with the same faces (the translation-only read of the box) refuse.
            subprocess.run((str(EXPORTER), str(step), str(root/'translation')), check=True, capture_output=True)
            (root/'nvidia/definition-41.brep').write_bytes((root/'translation/definition-41.brep').read_bytes())
            with self.assertRaisesRegex(HarnessRefusal, 'differs from our default export for 1 definitions'):
                check_nvidia_input(root/'nvidia', 'occt-default-read', source, root/'xde', INGRESS)
            self.assertTrue((root/'nvidia/nvidia-input-diff.tsv').is_file())
            # Another set of operations than the minimal ingress ran refuses.
            bits = SimpleNamespace(definitions=source.definitions,
                                   adapter_revision='v; shape-processing=default; shape-process-operations=000000000000000001')
            with self.assertRaisesRegex(HarnessRefusal, 'shape-process operations'):
                check_nvidia_input(root/'nvidia', 'occt-default-read', bits, root/'xde', INGRESS)

    def test_corrected_definition_reaches_nvidia_as_our_corrected_brep(self):
        """B2-reader-export review round 1: on the translation-only arm, a definition our
        export corrected (the adapter's S1 block, face #57) is NVIDIA's input as our corrected
        .brep; the caller runs on through NVIDIA's batch (the stand-in producer's failure)."""
        sys.path.insert(0, str(ROOT/'tests'))
        from test_step_reader_raw import flipped_block
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            step = root/'s1.step'
            step.write_text(flipped_block(same_sense=(57,)))
            _, matched = matched_fixture(root, step)
            output = root/'out'
            result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                '--step', str(step), '--matched', str(matched), '--output', str(output), '--heal', '0',
                '--workers', '1', '--allow-fixture', '--exporter', str(EXPORTER), '--nvidia-ingress', str(INGRESS),
                '--producer', str(self.stand_in(root)), '--package', str(PACKAGE)), capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            *_, paths = (output/'worker-00/binding').read_text().split('\t')
            self.assertEqual([Path(p) for p in paths.split(',')], [output/'source/definition-15.brep'])
            metadata = {r['field']: r['value'] for r in rows(output/'metadata.tsv')}
            self.assertEqual(metadata['nvidia_input_ours_corrected'], '15')
            self.assertEqual(metadata['exporter_raw_entity_corrections'], 'S1')

    def test_corrected_input_requires_our_uncorrected_bytes_to_be_the_ingress_bytes(self):
        """Review round 2, item 5: before NVIDIA is handed our corrected .brep, our
        uncorrected translation of that definition must be byte-identical to the minimal
        ingress's; one byte added to the ingress's file of the corrected definition is a
        harness refusal (exit 3)."""
        sys.path.insert(0, str(ROOT/'tests'))
        from test_step_reader_raw import flipped_block
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            step = root/'s1.step'
            step.write_text(flipped_block(same_sense=(57,)))
            _, matched = matched_fixture(root, step)
            for mutation, code in (('none', 1), ('bytes', 3)):
                with self.subTest(mutation=mutation):
                    output = root/f'out-{mutation}'
                    result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                        '--step', str(step), '--matched', str(matched), '--output', str(output), '--heal', '0',
                        '--workers', '1', '--allow-fixture', '--exporter', str(EXPORTER),
                        '--nvidia-ingress', str(self.ingress_wrapper(root, mutation)),
                        '--producer', str(self.stand_in(root)), '--package', str(PACKAGE)), capture_output=True, text=True)
                    self.assertEqual(result.returncode, code, result.stderr[-400:])
                    if code == 3:
                        self.assertIn('our uncorrected translation of corrected definition 15 differs', result.stderr)

    def ingress_wrapper(self, root, mutation):
        """The real minimal ingress, then one authored corruption of its output."""
        wrapper = root/f'ingress-{mutation}'
        wrapper.write_text(f'''#!{sys.executable}
import csv,subprocess,sys
from pathlib import Path
code=subprocess.run([{str(INGRESS)!r},*sys.argv[1:]]).returncode
out=Path(sys.argv[-1])
if code or {mutation!r}=='none' or sys.argv[-2].endswith('tests/fixtures/reference-source.step'):sys.exit(code)
rows=list(csv.reader(open(out/'definitions.tsv'),delimiter='\\t'))
if {mutation!r}=='located':rows[1][3]='0'
if {mutation!r}=='missing':rows=rows[:-1]
if {mutation!r}=='bytes':open(out/rows[1][1],'ab').write(b'x')
if {mutation!r}=='policy':
    lines=open(out/'policy.tsv').read().replace('operations\\t0','operations\\t1');open(out/'policy.tsv','w').write(lines)
csv.writer(open(out/'definitions.tsv','w'),delimiter='\\t',lineterminator='\\n').writerows(rows)
''')
        wrapper.chmod(0o700)
        return wrapper

    def test_nvidia_input_judgement_refuses_as_ingress(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            matched_fixture(root)
            step = root/'copy.step'
            step.write_bytes((ROOT/'tests/fixtures/reference-source.step').read_bytes())
            for mutation, message in (('located', 'not identity-located forward solids'),
                                      ('missing', 'solids differ from the source definitions'),
                                      ('bytes', 'differs from our XDE export'),
                                      ('policy', 'do not match arm')):
                with self.subTest(mutation=mutation):
                    result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                        '--step', str(step), '--matched', str(root/'matched'), '--output', str(root/mutation),
                        '--heal', '0', '--workers', '2', '--allow-fixture', '--exporter', str(EXPORTER),
                        '--nvidia-ingress', str(self.ingress_wrapper(root, mutation)),
                        '--producer', str(self.stand_in(root)), '--package', str(PACKAGE)), capture_output=True, text=True)
                    self.assertEqual(result.returncode, 4, result.stderr)
                    self.assertIn(message, result.stderr)
                    self.assertFalse((root/mutation/'worker-exits.tsv').exists())
                    if mutation == 'bytes':
                        # The geometry diff is written before the refusal.
                        diff, = rows(root/mutation/'nvidia-input/nvidia-input-diff.tsv')
                        # The corruption is one trailing byte: geometry reads back the same.
                        faces = diff['faces_nvidia/ours'].split('/')
                        self.assertEqual(faces[0], faces[1])
                        self.assertEqual(diff['max_vertex_coordinate_difference'], '0')

    def test_refusals_and_our_ingress_failure_are_never_nvidia_failures(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            # An exporter that ignores the arm's mode and reads translation-only cannot
            # serve the default-read arm: a harness refusal, never an NVIDIA outcome.
            ignoring = root/'mode-ignoring-exporter'
            ignoring.write_text(f'#!{sys.executable}\nimport os,sys\nos.execv({str(EXPORTER)!r},[{str(EXPORTER)!r},*sys.argv[3:]])\n')
            ignoring.chmod(0o700)
            result = self.caller(root, root/'default', self.stand_in(root),
                                 ('--ingress-arm', 'occt-default-read', '--exporter', str(ignoring)))
            self.assertEqual(result.returncode, 3, result.stderr)
            self.assertIn('harness refused', result.stderr)
            self.assertRegex(result.stderr, 'declares shape-processing=(translation-only|none)')
            self.assertFalse((root/'default'/'assignments.tsv').exists())
            result = self.caller(root, root/'owned', self.stand_in(root), ('--ingress-arm', 'owned-projected-pcurves'))
            self.assertEqual(result.returncode, 3)
            self.assertIn('awaits an operator decision', result.stderr)
            result = self.caller(root, root/'probe', self.stand_in(root),
                                 environment=dict(os.environ, BREPGPU_DELIVERED_CLOCK=str(root/'missing')))
            self.assertEqual(result.returncode, 3)
            self.assertIn('delivered-clock probe', result.stderr)
            # Our XDE exporter failing is ours: exit 4, before any NVIDIA input or worker.
            failing = stand_in(root/'failing-exporter', 1)
            result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                '--step', str(ROOT/'tests/fixtures/reference-source.step'), '--matched', str(root/'matched'),
                '--output', str(root/'ingress'), '--heal', '0', '--workers', '2', '--allow-fixture',
                '--exporter', str(failing), '--nvidia-ingress', str(INGRESS), '--producer', str(self.stand_in(root)),
                '--package', str(PACKAGE)), capture_output=True, text=True)
            self.assertEqual(result.returncode, 4, result.stderr)
            self.assertIn('ingress refused (ours)', result.stderr)
            self.assertFalse((root/'ingress/worker-exits.tsv').exists())

    def test_incomplete_source_definition_is_our_ingress_refusal(self):
        """B2-ingress third review F1: a definition incomplete at the source (a face refused,
        D-partial-output) is ours. The driver refuses before NVIDIA's ingress (exit 4), every
        physical.tsv row says not evaluated (never FAIL), and the benchmark maps the exit to
        ingress_refused. The admission step names the same refusal if one ever reaches it."""
        from types import SimpleNamespace
        from benchmark_om10_nvidia import classify_exit
        from brepgpu.imported_source import load_source
        adapter = Path(os.environ.get('BREPGPU_STEP_SOURCE', ROOT/'build/om10-step-source/brepgpu-step-source'))
        cone = (ROOT/'tests/fixtures/apex-cone.step').read_text()
        lone = cone.replace("#17 = ADVANCED_FACE('',(#18),#26,.T.);", "#17 = ADVANCED_FACE('',(#18,#42),#26,.T.);").replace(
            'ENDSEC;\nEND-ISO', "#42 = FACE_BOUND('',#43,.T.);\n#43 = VERTEX_LOOP('',#44);\n#44 = VERTEX_POINT('',#45);\n"
            "#45 = CARTESIAN_POINT('',(1.,1.,0.));\nENDSEC;\nEND-ISO")
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            step = root/'lone.step'
            step.write_text(lone)
            matched = root/'matched'
            matched.mkdir()
            subprocess.run((str(adapter), str(step), str(matched/'source.bgsource')), check=True, capture_output=True)
            source = load_source(matched/'source.bgsource')
            (matched/'model.tsv').write_text(f'key\tvalue\nscope\tcomplete_assembly\nsource_digest\t{source.source_sha256}\n')
            self.assertEqual([(x.id, x.status) for x in source.definitions], [(15, 1)])
            result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                '--step', str(step), '--matched', str(matched), '--output', str(root/'out'), '--heal', '0',
                '--workers', '1', '--allow-fixture', '--exporter', str(EXPORTER), '--nvidia-ingress', str(INGRESS),
                '--producer', str(self.stand_in(root)), '--package', str(PACKAGE)), capture_output=True, text=True)
            self.assertEqual(result.returncode, 4, result.stderr)
            self.assertIn('ingress refused (ours)', result.stderr)
            self.assertIn('incomplete at the source', result.stderr)
            self.assertFalse((root/'out/nvidia-input').exists())
            self.assertFalse((root/'out/worker-exits.tsv').exists())
            physical = rows(root/'out/physical.tsv')
            self.assertEqual([(r['definition_id'], r['status']) for r in physical], [('15', 'NOT_EVALUATED')])
            self.assertIn('incomplete at the source', physical[0]['message'])
            self.assertEqual(classify_exit(result.returncode), ('INGRESS_REFUSED', 'ingress_refused'))
            from render_om10_nvidia import physical_row
            row = physical_row(None, SimpleNamespace(source=source), 15)
            self.assertEqual(row[:2], (15, 'NOT_EVALUATED'))

    def test_incomplete_definition_in_legacy_mode_is_our_ingress_refusal(self):
        """Coordinator check after review round 2: the cylinder part of reference-source.step
        with its lateral face's bound flipped. The adapter refuses its faces (definition 395
        incomplete at the source), while the translation-only export writes OCCT's own
        translation. In the legacy matched mode (no model.tsv) the caller now reads the
        adapter's source as the native match does: it refuses before NVIDIA's ingress
        (exit 4, ours), 395 is not evaluated, and the benchmark maps the exit to
        ingress_refused, outside the speed ratio and the NVIDIA-failure count. Before, the
        definition reached NVIDIA and its outcome was scored."""
        import re
        from benchmark_om10_nvidia import classify_exit
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            step = root/'bound.step'
            step.write_text(re.sub(r"(#398 = FACE_BOUND\('',#\d+,)(\.[TF]\.)",
                                   lambda m: m.group(1)+('.F.' if m.group(2) == '.T.' else '.T.'),
                                   (ROOT/'tests/fixtures/reference-source.step').read_text()))
            _, matched = matched_fixture(root, step)
            self.assertFalse((matched/'model.tsv').exists())
            source = load_source(matched/'source.bgsource')
            self.assertEqual({x.id: x.status != 0 for x in source.definitions}, {41: False, 395: True})
            result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                '--step', str(step), '--matched', str(matched), '--output', str(root/'out'), '--heal', '0',
                '--workers', '1', '--allow-fixture', '--exporter', str(EXPORTER), '--nvidia-ingress', str(INGRESS),
                '--producer', str(self.stand_in(root)), '--package', str(PACKAGE)), capture_output=True, text=True)
            self.assertEqual(result.returncode, 4, result.stderr)
            self.assertIn('incomplete at the source', result.stderr)
            self.assertFalse((root/'out/nvidia-input').exists())
            physical = {r['definition_id']: r['status'] for r in rows(root/'out/physical.tsv')}
            self.assertEqual(physical, {'41': 'NOT_EVALUATED', '395': 'NOT_EVALUATED'})
            self.assertEqual(classify_exit(result.returncode), ('INGRESS_REFUSED', 'ingress_refused'))

    @unittest.skipUnless(PRODUCER.is_file() and PACKAGE.is_dir(), 'needs the built NVIDIA producer and package')
    def test_measured_cpu_calibrates_and_scopes_split_owners(self):
        from render_om10_nvidia import digest
        node = max(read_topology())
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            def run(output, *extra, heal='0', environment=None):
                return subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                    '--step', str(ROOT/'tests/fixtures/reference-source.step'), '--matched', str(root/'matched'),
                    '--output', str(output), '--heal', heal, '--workers', '2', '--allow-fixture',
                    '--lane-node', str(node), '--exporter', str(EXPORTER), '--nvidia-ingress', str(INGRESS),
                    '--producer', str(PRODUCER), '--package', str(PACKAGE), *extra),
                    capture_output=True, text=True, env=environment)
            matched_fixture(root)
            # An inherited mark from another process does not count as this caller's re-execution.
            inherited = dict(os.environ, **{lane_module.REEXEC_MARK: '1:0:0'})
            first = run(root/'first', environment=inherited)
            self.assertEqual(first.returncode, 0, first.stderr)
            metadata = {r['field']: r['value'] for r in rows(root/'first/metadata.tsv')}
            self.assertEqual(metadata['caller_reexecuted'], '1')
            measured = {r['definition_id']: r for r in rows(root/'first/definition-times.tsv')}
            for r in measured.values():
                self.assertEqual(r['status'], 'PASS')
                self.assertGreater(float(r['weight_cpu_seconds']), 0)
                self.assertGreaterEqual(float(r['nvidia_teardown_seconds']), 0)
                self.assertGreaterEqual(float(r['nvidia_seconds']), float(r['tessellate_seconds']))
            for w in rows(root/'first/worker-times.tsv'):
                start, by_stat = float(w['exec_to_main_seconds']), float(w['exec_to_main_from_stat_seconds'])
                self.assertGreater(start, 0)
                self.assertGreaterEqual(float(w['after_main_return_seconds']), 0)
                # Two mechanisms: launcher and producer on CLOCK_MONOTONIC, and /proc starttime at
                # clock-tick resolution, which truncates to a tick and so can only read high.
                self.assertLess(abs(start-by_stat), .02, (start, by_stat))
            second = run(root/'second', '--calibration', str(root/'first'))
            self.assertEqual(second.returncode, 0, second.stderr)
            assigned = rows(root/'second/assignments.tsv')
            self.assertEqual({r['basis'] for r in assigned}, {'measured_definition_cpu_seconds'})
            self.assertEqual({r['source_definition_id']: float(r['weight']) for r in assigned},
                             {i: float(r['weight_cpu_seconds']) for i, r in measured.items()})
            metadata = {r['field']: r['value'] for r in rows(root/'second/metadata.tsv')}
            self.assertEqual(metadata['caller_memory_policy'], f'bind:{node}')
            self.assertEqual(metadata['calibration_source'], 'own_settings')
            self.assertEqual(metadata['calibration_definition_times_sha256'], digest(root/'first/definition-times.tsv'))
            scopes = scope_rows(root/'second')
            marked = {n: float(r['seconds']) for n, r in scopes.items() if r['in_headline'] == '1'}
            self.assertEqual(set(marked), {'nvidia_ingress_process_wall', 'nvidia_only_worker_makespan'})
            self.assertAlmostEqual(float(scopes['nvidia_headline']['seconds']), sum(marked.values()))
            self.assertAlmostEqual(float(scopes['nvidia_headline_excluding_ingress']['seconds']),
                                   marked['nvidia_only_worker_makespan'])
            timings = {r['stage']: r for r in rows(root/'second/timings.tsv')}
            self.assertAlmostEqual(float(scopes['nvidia_batch_wall']['seconds']),
                                   float(timings['nvidia_batch_through_all_worker_exits_wall']['seconds']))
            for excluded in ('physical_admission', 'emit_decode_scene_pack_write', 'ours_output_extract_write_sum',
                             'ours_xde_manifest_export_wall', 'context_capture_nested_in_caller'):
                self.assertEqual(scopes[excluded]['in_headline'], '0')
            self.assertEqual(metadata['nvidia_input_identical_to_xde'], '2')
            self.assertEqual(metadata['nvidia_input_byte_check'], 'applied')
            pins = {r['component'] for r in rows(root/'second/runtime-pins.tsv')}
            self.assertTrue(any(c.startswith('occt/nvidia_ingress/libTK') for c in pins))
            self.assertTrue(any(c.startswith('occt/xde_exporter/libTK') for c in pins))
            cache = {(r['moment'], r['file']): float(r['resident_fraction']) for r in rows(root/'second/page-cache.tsv')}
            for moment in ('before_ours_xde_export', 'before_nvidia_ingress'):
                # The STEP file's pages were dropped immediately before each ingress.
                self.assertLess(cache[(moment, 'reference-source.step')], .5)
            contexts = {r['context_id'] for r in rows(root/'second/context.tsv')}
            self.assertTrue(all(r['context'] in contexts for r in timings.values()))
            # Calibration from another heal policy is refused before any worker starts.
            third = run(root/'third', '--calibration', str(root/'first'), heal='1')
            self.assertEqual(third.returncode, 3)
            self.assertIn('calibration heal 0 differs', third.stderr)
            self.assertFalse((root/'third/worker-exits.tsv').exists())
            # Our display failing after a valid batch and admission is ours: exit 5, NVIDIA time kept.
            failing = stand_in(root/'failing-display', 1)
            fourth = run(root/'fourth', '--mode', 'render', '--display', str(failing))
            self.assertEqual(fourth.returncode, 5, fourth.stderr)
            self.assertIn('our work failed (ours', fourth.stderr)
            self.assertIn('display failed', fourth.stderr)
            self.assertTrue(all(r['status'] == 'PASS' for r in rows(root/'fourth/physical.tsv')))
            self.assertIn('nvidia_headline', scope_rows(root/'fourth'))


@unittest.skipUnless(sys.platform == 'linux' and EXPORTER.is_file() and PRODUCER.is_file() and INGRESS.is_file(),
                     'needs the fleet reference exporter, minimal ingress and NVIDIA producer')
class NvidiaDriver(unittest.TestCase):
    """The NVIDIA driver end to end on the authored fixture, transport admission."""

    def driver(self, root, output, *extra, prefix=(), environment=None):
        if not (root/'matched').exists():
            matched_fixture(root)
        return subprocess.run((*prefix, sys.executable, '-B', str(ROOT/'scripts/benchmark_om10_nvidia.py'),
            '--step', str(ROOT/'tests/fixtures/reference-source.step'), '--matched', str(root/'matched'),
            '--output', str(output), '--admission', 'transport', '--allow-fixture',
            '--exporter', f'occt-no-shape-process={EXPORTER}', '--nvidia-ingress', str(INGRESS),
            '--producer', str(PRODUCER), '--package', str(PACKAGE),
            '--quiet-checks', '1', '--quiet-interval', '0', *extra), capture_output=True, text=True, timeout=2400,
            env=environment)

    def test_calibrated_bound_arms_headline_and_load_classes(self):
        from measurement_lane import load_class
        node = max(read_topology())
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            out = root/'run'
            inherited = dict(os.environ, **{lane_module.REEXEC_MARK: '1:0:0'})
            result = self.driver(root, out, '--ingress-arms', 'occt-no-shape-process', '--preparation-arms', 'none',
                                 '--worker-arms', '12,24', '--placement-arms', 'pinned,floating',
                                 '--lane-node', str(node), '--seed', '7', '--quiet-tries', '2', environment=inherited)
            self.assertEqual(result.returncode, 0, result.stderr)
            arms = rows(out/'arm-outcomes.tsv')
            representatives = [a for a in arms if a['representative'] == '1']
            self.assertEqual(sorted(a['arm'] for a in representatives), sorted({a['arm'] for a in arms}))
            self.assertEqual(len(representatives), 8)
            contexts = {r['context_id'] for r in rows(out/'context.tsv')}
            for arm in arms:
                self.assertEqual(arm['status'], 'COMPLETE_UNQUALIFIED', arm['detail'])
                self.assertEqual(arm['assignment_basis'], 'measured_definition_cpu_seconds')
                # Launched already bound: no caller re-executed, even with an inherited mark.
                self.assertEqual(arm['caller_reexecuted'], '0')
                self.assertIn(arm['context_id'], contexts)
                dataset = out/arm['context_id']
                scopes = scope_rows(dataset)
                marked = sum(float(r['seconds']) for r in scopes.values() if r['in_headline'] == '1')
                self.assertAlmostEqual(float(arm['complete_seconds']), marked, places=9)
                timings = {r['stage']: r for r in rows(dataset/'timings.tsv')}
                self.assertAlmostEqual(float(arm['nvidia_batch_wall_seconds']),
                                       float(timings['nvidia_batch_through_all_worker_exits_wall']['seconds']))
                samples = [dict(zip(('foreign', 'own', 'load1', 'gpu_foreign'), map(float, s.split('/'))))
                           for s in arm['settle_samples'].split(';')]
                self.assertEqual(arm['load_class'], load_class(samples, float(arm['settle_foreign_cpu_percent']),
                                                               float(arm['run_foreign_cpu_percent'])))
                # Settled inside the caller, immediately before NVIDIA's region; the run's own load is
                # NVIDIA's ingress and batch rows, not the whole caller.
                settled = rows(dataset/'settle.tsv')
                self.assertEqual(len(settled), 1)
                self.assertAlmostEqual(samples[0]['foreign'], float(settled[0]['foreign_cpu_percent']), places=3)
                region = {r['context_id']: float(r['foreign_cpu_percent']) for r in rows(dataset/'context.tsv')}
                self.assertAlmostEqual(float(arm['run_foreign_cpu_percent']),
                                       max(region['nvidia_ingress'], region['nvidia_batch']), places=3)
                tries = [a for a in arms if a['arm'] == arm['arm']]
                quiet = any(a['load_class'] == 'quiet' for a in tries)
                self.assertEqual(arm['quiet_status'],
                                 'measured_quiet' if quiet else f'arm {arm["arm"]} not measured quiet')
                # A loaded try is retried until quiet, at most --quiet-tries times; the representative
                # is the first quiet try, else the first try (one estimator for both cases).
                self.assertTrue(len(tries) == 2 or tries[0]['load_class'] == 'quiet', [t['load_class'] for t in tries])
                self.assertEqual(arm['tries'], str(len(tries)))
                first_quiet = [t for t in tries if t['load_class'] == 'quiet'][:1]
                chosen = [t for t in tries if t['representative'] == '1']
                self.assertEqual(chosen, first_quiet or tries[:1])
                w24 = arm['workers'] == '24'
                self.assertEqual(arm['calibration_source'], 'w12_pinned_for_w24' if w24 else 'own_settings')
            selected = rows(out/'reference-outcomes.tsv')
            self.assertEqual([r['config'] for r in selected], ['heal0', 'heal1'])
            for row in selected:
                heal = [a for a in representatives if a['heal'] == row['config'][-1]]
                quiet = [a for a in heal if a['quiet_status'] == 'measured_quiet']
                pool = quiet or heal
                self.assertEqual(float(row['complete_seconds']), min(float(a['complete_seconds']) for a in pool))
                self.assertIn('n=1, not a headline', row['detail'])
            plan = {r['field']: r['value'] for r in rows(out/'run-plan.tsv')}
            self.assertEqual(plan['seed'], '7')
            self.assertEqual(len(plan['measured_order'].split(',')), 8)

    def test_our_display_failure_keeps_nvidia_time_and_is_labelled_ours(self):
        node = max(read_topology())
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            failing = stand_in(root/'failing-display', 1)
            result = self.driver(root, root/'run', '--ingress-arms', 'occt-no-shape-process', '--preparation-arms',
                                 'none', '--worker-arms', '1', '--placement-arms', 'pinned', '--lane-node', str(node),
                                 '--quiet-tries', '1', '--display', str(failing))
            self.assertEqual(result.returncode, 0, result.stderr)
            for arm in rows(root/'run/arm-outcomes.tsv'):
                self.assertEqual((arm['status'], arm['scope']), ('OURS_FAILED', 'ours_failed'))
                self.assertEqual(arm['complete_seconds'], arm['nvidia_headline_seconds'])
                self.assertNotEqual(arm['complete_seconds'], '')
            for row in rows(root/'run/reference-outcomes.tsv'):
                self.assertEqual((row['status'], row['scope']), ('OURS_FAILED', 'ours_failed'))

    def test_ingress_refusal_is_per_arm_and_never_stops_the_campaign(self):
        node = max(read_topology())
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            step = root/'copy.step'
            step.write_bytes((ROOT/'tests/fixtures/reference-source.step').read_bytes())
            wrapper = NvidiaCallerLane.ingress_wrapper(None, root, 'located')
            matched_fixture(root)
            result = subprocess.run((sys.executable, '-B', str(ROOT/'scripts/benchmark_om10_nvidia.py'),
                '--step', str(step), '--matched', str(root/'matched'), '--output', str(root/'run'),
                '--admission', 'transport', '--allow-fixture', '--exporter', f'occt-no-shape-process={EXPORTER}',
                '--nvidia-ingress', str(wrapper), '--producer', str(PRODUCER), '--package', str(PACKAGE),
                '--quiet-checks', '1', '--quiet-interval', '0', '--quiet-tries', '1',
                '--ingress-arms', 'occt-no-shape-process', '--preparation-arms', 'none', '--worker-arms', '1',
                '--placement-arms', 'pinned', '--lane-node', str(node)), capture_output=True, text=True, timeout=1200)
            # Every arm ran its calibration pass and is labelled ours; the campaign finished.
            self.assertEqual(result.returncode, 0, result.stderr)
            arms = rows(root/'run/arm-outcomes.tsv')
            self.assertEqual(len(arms), 2)
            self.assertEqual({(a['status'], a['scope']) for a in arms}, {('INGRESS_REFUSED', 'ingress_refused')})
            self.assertEqual({(r['status'], r['scope']) for r in rows(root/'run/reference-outcomes.tsv')},
                             {('INGRESS_REFUSED', 'ingress_refused')})

    def test_default_read_arm_runs_through_the_driver(self):
        """bead .37.19: with no --exporter for it, the default-read arm runs our exporter in
        default mode and completes on the authored fixture (it refused before)."""
        node = max(read_topology())
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            result = self.driver(root, root/'run', '--ingress-arms', 'occt-default-read', '--preparation-arms', 'none',
                                 '--worker-arms', '1', '--placement-arms', 'pinned', '--lane-node', str(node))
            self.assertEqual(result.returncode, 0, result.stderr)
            arms = rows(root/'run/arm-outcomes.tsv')
            self.assertTrue(arms)
            self.assertEqual({a['ingress_arm'] for a in arms}, {'occt-default-read'})
            self.assertFalse({a['status'] for a in arms} & {'INGRESS_REFUSED', 'HARNESS_REFUSED', 'OURS_FAILED', 'FAIL'})

    def test_preflight_refuses_harness_problems_before_any_arm(self):
        node = max(read_topology())
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            result = self.driver(root, root/'arm', '--ingress-arms', 'occt-default-read', '--exporter',
                                 f'occt-default-read={root/"missing"}', '--preparation-arms', 'none',
                                 '--worker-arms', '1', '--placement-arms', 'pinned', '--lane-node', str(node))
            self.assertEqual(result.returncode, 3, result.stderr)
            self.assertIn('no built exporter', result.stderr)
            self.assertFalse(list((root/'arm').glob('*-prepare')))
            # An exporter that ignores the arm's mode reads translation-only: refused before any arm.
            ignoring = root/'mode-ignoring-exporter'
            ignoring.write_text(f'#!{sys.executable}\nimport os,sys\nos.execv({str(EXPORTER)!r},[{str(EXPORTER)!r},*sys.argv[3:]])\n')
            ignoring.chmod(0o700)
            result = self.driver(root, root/'declared', '--ingress-arms', 'occt-default-read', '--exporter',
                                 f'occt-default-read={ignoring}', '--preparation-arms', 'none', '--worker-arms', '1',
                                 '--placement-arms', 'pinned', '--lane-node', str(node))
            self.assertEqual(result.returncode, 3, result.stderr)
            self.assertRegex(result.stderr, 'declares shape-processing=(translation-only|none)')
            self.assertFalse(list((root/'declared').glob('*-prepare')))
            # Two allowed CPUs cannot hold a four-worker arm.
            cpus = ','.join(map(str, lane_cpus(read_topology(), node, 2)))
            result = self.driver(root, root/'fit', '--ingress-arms', 'occt-no-shape-process', '--preparation-arms',
                                 'none', '--worker-arms', '4', '--placement-arms', 'pinned', '--lane-node', str(node),
                                 prefix=('taskset', '-c', cpus))
            self.assertEqual(result.returncode, 3, result.stderr)
            self.assertIn('outside this process affinity', result.stderr)
            self.assertFalse(list((root/'fit').glob('*-prepare')))


if __name__ == '__main__':
    unittest.main()
