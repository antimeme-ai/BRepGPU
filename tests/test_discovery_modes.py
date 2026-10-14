"""Focused mode-selection callers; no alternate geometry implementation.

BREPGPU_TEST_BACKEND/DEVICE select the existing public test backend/card.
BREPGPU_LIBRARY selects the core; optional BREPGPU_TEST_PLUGIN selects an actual
CUDA plugin explicitly. A requested unavailable CUDA backend is an error.
The capture client imports these callers to compare their real output bytes.
"""
from array import array
from contextlib import closing
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from fractions import Fraction
import ctypes
import math
import os
import struct
import unittest

from brepgpu import Engine, Patch, Lineage, HierarchyOptions, DiscoveryOptions
from brepgpu.discovery_fixtures import (
    discovery_cases, degree_fixture, constant_third, constant_zero,
    negative_coefficients, orthogonal_plane)


SLABS = ("statuses", "flags", "visits", "pruned", "refinements", "evaluation_counts")
CANONICAL_NAN = 0x7ff8000000000000


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def engine_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                library_path=os.environ.get("BREPGPU_LIBRARY"),
                plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN"))


@dataclass(frozen=True)
class Observation:
    key: str
    kind: str
    batch: object
    controls: tuple
    slots: tuple
    targets: tuple
    auxiliary: tuple = ()
    labels: tuple = ()
    profile: bool = False


def source_patches(fixtures):
    return [Patch(f.degree_u, f.degree_v, f.controls,
                  lineage=Lineage(11, 5, 1000+i, 13, 17))
            for i, f in enumerate(fixtures)]


def controls_of(fixtures):
    return tuple(value for f in fixtures for value in f.controls)


def check_profile(batch, profile, backend):
    measured = profile and backend == "cuda" and batch.count != 0
    require(batch.info.flags == int(measured), "profile flags differ from actual operation")
    if measured:
        require(math.isfinite(batch.info.kernel_ms) and batch.info.kernel_ms >= 0,
                "nonempty profiled operation has no finite duration")
    else:
        require(math.isnan(batch.info.kernel_ms), "unprofiled/empty duration is not NaN")
    require(batch.info.backend == (1 if backend == "cuda" else 0), "wrong selected backend")


def discovery_row(batch, index):
    return tuple(batch.data[k*batch.count+index] for k in range(8))


def check_valid_row(batch, index, truth=None, case=None):
    values = discovery_row(batch, index)
    status, flags, visits, pruned, refinements, evaluations = (
        int(getattr(batch, name)[index]) for name in SLABS)
    require(status in (0, 1) and flags & 1, "valid request has no finite candidate")
    require(all(math.isfinite(v) for v in values), "valid candidate contains a nonfinite field")
    require(all(0 <= v <= 1 for v in values[:2]), "candidate UV outside chart")
    lower, upper = values[6:]
    require(0 <= lower <= upper, "unordered squared bracket")
    if truth is not None:
        require(Fraction.from_float(lower) <= truth <= Fraction.from_float(upper),
                "bracket misses simple analytic answer")
    require(bool(flags & 2) == (status == 0), "status and GAP flag disagree")
    if flags & 2:
        require(Fraction.from_float(upper)-Fraction.from_float(lower)
                <= Fraction.from_float(batch.options.tolerance_squared), "false GAP flag")
    require(bool(flags & 8) != bool(flags & 4), "valid row needs exactly one coverage/cap reason")
    require(not flags & ~31, "valid row contains arithmetic/unknown flags")
    require(1 <= visits <= batch.options.max_visits and 0 <= pruned <= visits,
            "node counters outside requested bounds")
    require(0 <= refinements <= batch.options.max_refinements
            and 1+refinements <= evaluations <= 1+833*refinements,
            "refinement/evaluator counters outside bounds")
    if flags & 16:
        require(refinements == batch.options.max_refinements, "early refinement cap")
    if case is not None:
        if case.status is not None:
            require(status == case.status, case.name+": wrong expected status")
        require(flags & case.required_flags == case.required_flags
                and not flags & case.forbidden_flags, case.name+": wrong independent flags")
        for actual, expected in ((visits, case.exact_visits),
                                 (refinements, case.exact_refinements),
                                 (evaluations, case.exact_evaluations)):
            if expected is not None:
                require(actual == expected, case.name+": wrong exact work count")


def observe(engine, key, fixtures, slots, targets, hierarchy_options, options,
            labels=(), retain=False):
    """Return only downloaded owned buffers; context stays owned by the caller."""
    with engine.create_patch_set(source_patches(fixtures)) as patches:
        with engine.create_hierarchy(patches, hierarchy_options) as hierarchy:
            if hierarchy_options.mode == 0:
                require(len(hierarchy.download().bounds) == 0, "AABB cache is nonempty")
            with engine.create_discovery_query_set(hierarchy, slots, targets) as queries:
                with engine.discover(queries, options) as result:
                    if retain:
                        queries.close(); hierarchy.close(); patches.close()
                    batch = result.download()
    require(batch.count == len(slots), "downloaded count changed")
    require(batch.options == options and batch.hierarchy_options == hierarchy_options,
            "operation option snapshots changed")
    require(batch._slots.tobytes() == array("I", slots).tobytes()
            and batch.targets.tobytes() == array("d", targets).tobytes(),
            "input snapshot/export changed bits")
    require(len(batch.data) == 8*batch.count
            and all(len(getattr(batch, name)) == batch.count for name in SLABS),
            "download slab shape changed")
    check_profile(batch, engine.profile, engine.backend)
    return Observation(key, "discovery", batch, controls_of(fixtures), tuple(slots),
                       tuple(targets), labels=tuple(labels), profile=engine.profile)


def focused_observations(arguments):
    """Small all-mode cases, with analytic checks instead of a cache campaign."""
    cases = {case.name: case for case in discovery_cases()}
    plane = orthogonal_plane()
    for profile in (False, True):
        with Engine(profile=profile, **arguments) as engine:
            for mode in range(3):
                for count in (0, 1, 127, 128, 129, 257):
                    slots, targets = [], []
                    for i in range(count):
                        kind = i % 5
                        slots.append(0xffffffff if kind == 1 else 0)
                        targets.extend((.25, .75, .5) if kind == 0 else
                                       (math.nan, 0., 0.) if kind in (1, 2) else
                                       (0., math.inf, 0.) if kind == 3 else
                                       (math.nextafter(4., math.inf), 0., 0.))
                    observation = observe(engine, f"tail/m{mode}/p{int(profile)}/n{count}",
                                          (plane,), slots, targets,
                                          HierarchyOptions(2, mode=mode), DiscoveryOptions())
                    batch = observation.batch
                    data = batch.data.tobytes()
                    for i in range(count):
                        if i % 5 == 0:
                            check_valid_row(batch, i, Fraction(1, 4))
                            require(batch.statuses[i] == 0, "plane tail failed requested GAP")
                            expected = (.25, .75, 0.)
                            require(all(abs(batch.data[(2+k)*count+i]-expected[k]) <= 2.**-40
                                        for k in range(3)), "plane tail point mismatch")
                        else:
                            require(batch.statuses[i] == (2 if i % 5 == 1 else 3),
                                    "invalid slot/target precedence changed")
                            require(all(struct.unpack_from("=Q", data, (k*count+i)*8)[0]
                                        == CANONICAL_NAN for k in range(8)),
                                    "failed fields are not canonical NaN bytes")
                            require(all(getattr(batch, name)[i] == 0 for name in SLABS[1:]),
                                    "invalid row performed work")
                    yield observation
                # Profile-on/off copies of one retained child exercise every entry point.
                observation = observe(engine, f"retained/m{mode}/p{int(profile)}", (plane,),
                                      (0,), (.25, .75, .5), HierarchyOptions(2, mode=mode),
                                      DiscoveryOptions(), retain=True)
                check_valid_row(observation.batch, 0, Fraction(1, 4))
                yield observation
                if profile:
                    continue
                for name in ("cap_tight", "cap_wide", "complete_wide",
                             "complete_tight_root_leaf", "constant_root_once",
                             "one_third_false_zero"):
                    case = cases[name]
                    observation = observe(engine, f"case/m{mode}/{name}", (case.fixture,),
                                          (0,), case.target, HierarchyOptions(case.depth, mode=mode),
                                          DiscoveryOptions(case.max_visits, case.max_refinements,
                                                           case.tolerance_squared), (name,))
                    check_valid_row(observation.batch, 0, case.distance_answer, case)
                    yield observation
                for refinements in (0, 1, 8, 64):
                    for visits in (1, 2, 8191):
                        observation = observe(engine, f"budget/m{mode}/r{refinements}/v{visits}",
                                              (plane,), (0,), (.25, .75, .5),
                                              HierarchyOptions(2, mode=mode),
                                              DiscoveryOptions(visits, refinements))
                        check_valid_row(observation.batch, 0, Fraction(1, 4))
                        if visits < 3:
                            require(observation.batch.visits[0] == 1
                                    and observation.batch.flags[0] & 8
                                    and not observation.batch.flags[0] & 4,
                                    "two-child reservation/cap fallback changed")
                        yield observation
                fixtures = (constant_third(), degree_fixture(3, 3, "constant"), plane,
                            constant_zero())
                targets = (float(Fraction(1, 3)), 0., 0., .125, -.375, .5,
                           .25, .75, .5, -0., 0., 0.)
                observation = observe(engine, f"mixed/m{mode}", fixtures, (0, 1, 2, 3), targets,
                                      HierarchyOptions(2, mode=mode), DiscoveryOptions())
                truths = ((Fraction(1, 3)-Fraction.from_float(targets[0]))**2,
                          Fraction(3, 32), Fraction(1, 4), Fraction(0))
                for i, truth in enumerate(truths):
                    check_valid_row(observation.batch, i, truth)
                yield observation
            if not profile:
                fixture = negative_coefficients()
                observation = observe(engine, "depth12/aabb", (fixture,), (0,), (.25, 0., 0.),
                                      HierarchyOptions(12, mode=0), DiscoveryOptions(max_refinements=0))
                check_valid_row(observation.batch, 0, Fraction(0))
                require(observation.batch.flags[0] & 4, "full depth12 traversal not complete")
                yield observation


def corpus_observations(arguments):
    """Capture existing corpus; independent Fraction geometry runs separately."""
    groups = {}
    for index, case in enumerate(discovery_cases()):
        key = (case.depth, case.max_visits, case.max_refinements, case.tolerance_squared)
        groups.setdefault(key, []).append((index, case))
    with Engine(**arguments) as engine:
        for mode in range(3):
            for group, (key, items) in enumerate(groups.items()):
                items = items[::2] + list(reversed(items[1::2]))
                fixtures = tuple(case.fixture for _, case in items)
                targets = tuple(v for _, case in items for v in case.target)
                observation = observe(engine, f"corpus/m{mode}/g{group}", fixtures,
                                      tuple(range(len(items))), targets,
                                      HierarchyOptions(key[0], mode=mode),
                                      DiscoveryOptions(*key[1:]),
                                      tuple(f"{index}:{case.name}" for index, case in items))
                for row, (_, case) in enumerate(items):
                    # Structural/work flags only; no duplicate original-UV/cache oracle.
                    check_valid_row(observation.batch, row, case=case)
                yield observation


def smoke_observations(arguments):
    fixture = orthogonal_plane()
    with Engine(**arguments) as engine:
        with engine.create_patch_set(source_patches((fixture,))) as patches:
            with engine.create_query_set(patches, (0,), (.25, .75)) as query:
                with engine.evaluate(query, order=2) as result:
                    batch = result.download()
            batch.require_ok(); check_profile(batch, False, engine.backend)
            expected = (.25, .75, 0., 1., 0., 0., 0., 1., 0., 0., 0., 0.,
                        0., 0., 0., 0., 0., 0.)
            require(tuple(batch.data) == expected, "ABI1 analytic jet smoke failed")
            yield Observation("smoke/evaluator", "evaluator", batch, fixture.controls,
                              (0,), (.25, .75))
            with engine.create_projection_query_set(patches, (0,), (.25, .75, .5), (.5, .5)) as query:
                with engine.project(query) as result:
                    batch = result.download()
            batch.require_stationary(); check_profile(batch, False, engine.backend)
            require(tuple(batch.data[:6]) == (.25, .75, .25, .75, 0., .25)
                    and batch.data[6] <= batch.options.gradient_tolerance,
                    "ABI2 analytic projection smoke failed")
            yield Observation("smoke/projection", "projection", batch, fixture.controls,
                              (0,), (.25, .75, .5), (.5, .5))


def output_bytes(batch):
    return (batch.data.tobytes(), *(getattr(batch, name).tobytes() for name in SLABS))


class DiscoveryModeTests(unittest.TestCase):
    def test_all_mode_boundaries_caps_and_profile_bytes(self):
        observations = {}
        with closing(focused_observations(engine_arguments())) as iterator:
            for observation in iterator:
                with self.subTest(key=observation.key):
                    observations[observation.key] = observation
        for mode in range(3):
            for suffix in [f"tail/m{mode}/p{{}}/n{n}" for n in (0, 1, 127, 128, 129, 257)] \
                    + [f"retained/m{mode}/p{{}}"]:
                self.assertEqual(output_bytes(observations[suffix.format(0)].batch),
                                 output_bytes(observations[suffix.format(1)].batch))

    def test_abi1_and_abi2_analytic_smoke(self):
        with closing(smoke_observations(engine_arguments())) as iterator:
            self.assertEqual([o.key for o in iterator], ["smoke/evaluator", "smoke/projection"])

    def test_each_mode_restores_caller_and_worker_devices(self):
        arguments = engine_arguments()
        if arguments["backend"] != "cuda":
            self.skipTest("device binding is inapplicable to explicitly selected CPU")
        runtime = ctypes.CDLL(os.environ.get("BREPGPU_CUDART", "libcudart.so.12"))
        runtime.cudaGetDevice.argtypes = [ctypes.POINTER(ctypes.c_int)]
        runtime.cudaGetDevice.restype = ctypes.c_int
        runtime.cudaSetDevice.argtypes = [ctypes.c_int]; runtime.cudaSetDevice.restype = ctypes.c_int
        runtime.cudaGetDeviceCount.argtypes = [ctypes.POINTER(ctypes.c_int)]
        runtime.cudaGetDeviceCount.restype = ctypes.c_int
        def current():
            device = ctypes.c_int(-1)
            require(runtime.cudaGetDevice(ctypes.byref(device)) == 0, "get current device failed")
            return device.value
        count = ctypes.c_int()
        require(runtime.cudaGetDeviceCount(ctypes.byref(count)) == 0 and count.value >= 2,
                "both qualification devices are required")
        saved = current(); other = (arguments["device"]+1) % count.value
        require(runtime.cudaSetDevice(other) == 0, "select prior device failed")
        try:
            with Engine(**arguments) as engine:
                require(current() == other, "context creation changed prior device")
                with engine.create_patch_set(source_patches((orthogonal_plane(),))) as patches:
                    for mode in range(3):
                        with engine.create_hierarchy(patches, HierarchyOptions(2, mode=mode)) as hierarchy:
                            with engine.create_discovery_query_set(hierarchy, (0,), (.25, .75, .5)) as query:
                                def worker(_):
                                    previous = current()
                                    require(runtime.cudaSetDevice(other) == 0, "worker device selection failed")
                                    try:
                                        with engine.discover(query) as result:
                                            require(current() == other, "launch failed to restore worker device")
                                            batch = result.download()
                                            require(current() == other, "download failed to restore worker device")
                                            check_valid_row(batch, 0, Fraction(1, 4))
                                        require(current() == other, "result close failed to restore worker device")
                                    finally:
                                        require(runtime.cudaSetDevice(previous) == 0, "worker prior-device reset failed")
                                worker(0)
                                with ThreadPoolExecutor(max_workers=1) as pool:
                                    list(pool.map(worker, (0,)))
                            require(current() == other, "query close changed prior device")
                        require(current() == other, "hierarchy close changed prior device")
                require(current() == other, "patch close changed prior device")
            require(current() == other, "context close changed prior device")
        finally:
            require(runtime.cudaSetDevice(saved) == 0, "caller prior-device reset failed")


if __name__ == "__main__":
    unittest.main()
