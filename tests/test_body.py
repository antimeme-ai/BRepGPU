"""Direct canonical-body sampling contracts; native calls run on the fleet."""
from dataclasses import FrozenInstanceError
import math
import os
import struct
from itertools import repeat
import unittest
from unittest.mock import patch


def engine_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")),
                plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN"))


class BodyTests(unittest.TestCase):
    def test_box_samples_preserve_exact_source_and_invalid_rows(self):
        # Existing native engine must work; absent new body API is the direct red.
        from brepgpu import Engine
        with Engine(**engine_arguments()) as engine:
            from brepgpu.body import make_body
            from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
            from brepgpu.body_oracles import qualify_batch
            snapshot = make_body("box")
            top = next(i for i, chart in enumerate(snapshot.charts)
                       if snapshot.faces[chart.face_slot].name == "top")
            request = SampleRequest(snapshot, (Candidate(top, .25, .75),
                                                Candidate(top, math.nan, .5),
                                                Candidate(top, -1., .5)))
            with BodySampler(engine, snapshot) as sampler:
                batch = sampler.sample(request)
            metrics = qualify_batch(batch)
            self.assertEqual((batch.count, batch.evaluated_count), (3, 1))
            self.assertEqual((metrics["accepted"], metrics["invalid"]), (1, 2))
            for actual, expected in zip(batch.rows[0].position, (-.375, .375, .5)):
                self.assertLessEqual(abs(actual-expected), 2.**-36)
            self.assertEqual(batch.rows[0].normal, (0., 0., 1.))


    def assert_request_preserved(self, batch, request):
        self.assertIs(batch.snapshot, request.snapshot)
        self.assertEqual(batch.count, len(request.candidates))
        for row, candidate in zip(batch.rows, request.candidates):
            self.assertEqual(row.patch_slot, candidate.patch_slot)
            self.assertEqual(struct.pack("dd", row.u, row.v), struct.pack("dd", candidate.u, candidate.v))

    def test_canonical_graph_wire_and_immutable_source(self):
        from brepgpu.body import Frame, make_body
        from brepgpu.body_oracles import qualify_snapshot
        namespaces = []
        origin = [1., -2., 3.]
        frame = Frame(origin, .5)
        origin[0] = 9.
        self.assertEqual(frame.origin, (1., -2., 3.))
        for shape in ("box", "cylinder", "plate"):
            with self.subTest(shape=shape):
                snapshot = make_body(shape, frame)
                namespaces.append(snapshot.body_id)
                metrics = qualify_snapshot(snapshot)
                self.assertEqual(metrics["euler"], 0 if shape == "plate" else 2)
                for obj, field, value in ((snapshot, "shape", "box"), (snapshot.frame, "scale", 1.),
                                          (snapshot.vertices[0], "position", (0., 0., 0.)),
                                          (snapshot.patches[0], "controls", ())):
                    with self.assertRaises((FrozenInstanceError, AttributeError)):
                        setattr(obj, field, value)
                with self.assertRaises(TypeError):
                    snapshot.patches[0].controls[0] = 0.
        self.assertEqual(len(set(namespaces)), 3)

    def test_exact_submitted_trim_and_quarter_seam_labels(self):
        from brepgpu.body import make_body
        from brepgpu.body_oracles import trim_label, BOUNDARY, INTERIOR, REJECTED
        uv = ((0., 0.), (1., 1.), (.5, .5), (0., .5), (1., .5), (.5, 0.), (.5, 1.),
              (.8, .9), (.6, 19./30.), (1./3., .5), (2./3., .5),
              (math.nextafter(1./3., 0.), .5), (math.nextafter(1./3., 1.), .5),
              (math.nextafter(0., 1.), .5), (math.nextafter(1., 0.), .5),
              (-0., .25), (math.nan, .5), (math.inf, .5), (-1., .5), (.5, 2.))
        for shape in ("box", "cylinder", "plate"):
            snapshot = make_body(shape)
            for slot in range(len(snapshot.charts)):
                for u, v in uv:
                    with self.subTest(shape=shape, slot=slot, u=u, v=v):
                        self.assertEqual(snapshot.classify(slot, u, v), trim_label(snapshot, slot, u, v))
            if shape == "plate":
                self.assertEqual(snapshot.classify(0, .5, .5), REJECTED)
                for u in (1./3., 2./3., math.nextafter(1./3., 0.), math.nextafter(1./3., 1.)):
                    self.assertNotEqual(snapshot.classify(0, u, .5), BOUNDARY)
            if shape != "box":
                wall = snapshot.faces[-1].chart_slots
                self.assertEqual(snapshot.classify(wall[0], 0., .5), BOUNDARY)
                self.assertEqual(snapshot.classify(wall[3], 1., .5), BOUNDARY)
                for left, right in zip(wall, wall[1:]):
                    self.assertEqual(snapshot.classify(left, 1., .5), INTERIOR)
                    self.assertEqual(snapshot.classify(right, 0., .5), INTERIOR)

    def test_public_edge_and_uv_evaluators_follow_canonical_parameter(self):
        from fractions import Fraction
        from brepgpu.body import make_body
        from brepgpu.body_oracles import curve_expression, uv_expression, value, exact
        for shape in ("box", "cylinder", "plate"):
            snapshot = make_body(shape)
            for coedge in snapshot.coedges:
                edge = snapshot.edges[coedge.edge_slot]
                curve = snapshot.curves[edge.curve_slot]
                for numerator in range(9):
                    t = Fraction(numerator, 8)
                    q = min(int(4*t), 3)
                    local = 4*t-q
                    expected_point = tuple(value(a, local if curve.kind == "circle" else t)
                                           for a in curve_expression(curve, q))
                    expected_uv = tuple(value(a, local if coedge.uv_map.kind.startswith("circle_") else t)
                                        for a in uv_expression(coedge.uv_map, q))
                    for actual, expected in zip(snapshot.edge_point(coedge.edge_slot, float(t)), expected_point):
                        self.assertLessEqual(abs(exact(actual)-expected), Fraction(1, 1 << 36))
                    for actual, expected in zip(coedge.uv_map.evaluate(float(t)), expected_uv):
                        self.assertLessEqual(abs(exact(actual)-expected), Fraction(1, 1 << 36))
                for invalid in (-.1, 1.1, math.nan, math.inf):
                    with self.assertRaises(ValueError):
                        curve.evaluate(invalid)
                    with self.assertRaises(ValueError):
                        coedge.uv_map.evaluate(invalid)

    def test_uniform_grid_and_adversarial_scatter_all_bodies(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler, prepare_grid
        from brepgpu.body_oracles import qualify_batch
        uv = ((.25, .75), (.5, .5), (0., .5), (1., .5), (.5, 0.), (.5, 1.),
              (0., 0.), (.8, .9), (1./3., .5), (2./3., .5), (-0., .25),
              (math.nextafter(0., 1.), .5), (math.nan, .5), (.5, math.inf), (-1., .5))
        with Engine(**engine_arguments()) as engine:
            for shape in ("box", "cylinder", "plate"):
                with self.subTest(shape=shape):
                    snapshot = make_body(shape)
                    with BodySampler(engine, snapshot) as sampler:
                        for request in (prepare_grid(snapshot, 17),
                                        SampleRequest(snapshot, (Candidate(slot, u, v)
                                            for slot in reversed(range(len(snapshot.charts))) for u, v in uv))):
                            batch = sampler.sample(request)
                            self.assert_request_preserved(batch, request)
                            metrics = qualify_batch(batch)
                            self.assertGreater(metrics["accepted"], 0)
                            self.assertGreater(metrics["boundary"], 0)

    def test_positive_power_two_frames_and_immutable_batches(self):
        from brepgpu import Engine
        from brepgpu.body import Frame, make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
        from brepgpu.body_oracles import qualify_batch
        with Engine(**engine_arguments()) as engine:
            for shape in ("box", "cylinder", "plate"):
                for scale in (.25, 1., 4.):
                    with self.subTest(shape=shape, scale=scale):
                        snapshot = make_body(shape, Frame((16., -16., .125), scale))
                        source = [Candidate(i, .25, .75) for i in range(len(snapshot.charts))]
                        request = SampleRequest(snapshot, source)
                        source.clear()
                        with BodySampler(engine, snapshot) as sampler:
                            batch = sampler.sample(request)
                        self.assert_request_preserved(batch, request)
                        qualify_batch(batch)
                        for obj, field, value in ((request, "labels", ()), (batch, "rows", ()),
                                                  (batch.rows[0], "normal", (0., 0., 0.))):
                            with self.assertRaises((FrozenInstanceError, AttributeError)):
                                setattr(obj, field, value)
                        self.assertIsInstance(batch.timings, tuple)

    def test_selected_faces_empty_inputs_validation_and_request_cap(self):
        from brepgpu.body import Frame, make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, prepare_grid, MAX_CANDIDATES
        snapshot = make_body("plate")
        selected = (snapshot.faces[-1].id, snapshot.faces[0].id)
        request = prepare_grid(snapshot, 3, selected)
        slots = (*snapshot.faces[0].chart_slots, *snapshot.faces[-1].chart_slots)
        expected = tuple((slot, u/2, v/2) for slot in slots for u in range(3) for v in range(3))
        self.assertEqual(tuple((c.patch_slot, c.u, c.v) for c in request.candidates), expected)
        self.assertEqual(prepare_grid(snapshot, 2, ()).candidates, ())
        for resolution in (1, 258, 2.5):
            with self.assertRaises((TypeError, ValueError)):
                prepare_grid(snapshot, resolution)
        for selected in ((999,), (snapshot.faces[0].id,)*2):
            with self.assertRaises(ValueError):
                prepare_grid(snapshot, 2, selected)
        for slot in (-1, 1 << 32, len(snapshot.charts)):
            with self.assertRaises(ValueError):
                SampleRequest(snapshot, (Candidate(slot, .5, .5),))
        def early_invalid():
            yield Candidate(len(snapshot.charts), .5, .5)
            raise AssertionError("request validation consumed beyond invalid structural row")
        with self.assertRaises(ValueError):
            SampleRequest(snapshot, early_invalid())
        with self.assertRaises(ValueError):
            SampleRequest(snapshot, repeat(Candidate(0, .5, .5), MAX_CANDIDATES+1))
        for origin, scale in (((17., 0., 0.), 1.), ((math.inf, 0., 0.), 1.), ((0., 0., 0.), .3)):
            with self.assertRaises(ValueError):
                Frame(origin, scale)

    def test_exact_snapshot_binding_empty_rows_and_closed_lifetimes(self):
        from brepgpu import Engine, BgError
        from brepgpu.body import Frame, make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
        from brepgpu.body_oracles import qualify_batch
        engine = Engine(**engine_arguments())
        try:
            snapshot = make_body("cylinder")
            sampler = BodySampler(engine, snapshot)
            try:
                request = SampleRequest(snapshot, (Candidate(2, .5, .5),))
                for foreign in (make_body("cylinder"), make_body("cylinder", Frame((1., 0., 0.), 2.))):
                    with self.assertRaises(ValueError):
                        sampler.sample(SampleRequest(foreign, (Candidate(2, .5, .5),)))
                empty = sampler.sample(SampleRequest(snapshot, ()))
                self.assertEqual((qualify_batch(empty)["count"], empty.evaluated_count), (0, 0))
                batch = sampler.sample(request)
                sampler.close()
                sampler.close()
                with self.assertRaises(BgError):
                    sampler.sample(request)
                with BodySampler(engine, snapshot) as other:
                    engine.close()
                    with self.assertRaises(BgError):
                        other.sample(request)
                qualify_batch(batch)
            finally:
                sampler.close()
        finally:
            engine.close()

    def test_sampling_primary_error_survives_both_child_cleanup_failures(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
        from brepgpu.body_oracles import qualify_batch
        primary = ArithmeticError("chosen body download failure")
        children = []
        original_query = Engine.create_query_set
        original_evaluate = Engine.evaluate

        def query_with_cleanup(engine, *args, **kwargs):
            child = original_query(engine, *args, **kwargs)
            children.append(child)
            close = child.close
            def fail_close():
                child.close = close
                close()
                raise SystemExit("chosen query cleanup failure")
            child.close = fail_close
            return child

        def result_with_failure(engine, *args, **kwargs):
            child = original_evaluate(engine, *args, **kwargs)
            children.append(child)
            close = child.close
            def fail_download():
                raise primary
            def fail_close():
                child.close = close
                close()
                raise KeyboardInterrupt("chosen result cleanup failure")
            child.download = fail_download
            child.close = fail_close
            return child

        with Engine(**engine_arguments()) as engine:
            snapshot = make_body("box")
            request = SampleRequest(snapshot, (Candidate(0, .25, .75),))
            with BodySampler(engine, snapshot) as sampler:
                with patch.object(Engine, "create_query_set", query_with_cleanup), \
                        patch.object(Engine, "evaluate", result_with_failure):
                    with self.assertRaises(ArithmeticError) as caught:
                        sampler.sample(request)
                self.assertIs(caught.exception, primary)
                self.assertEqual(len(children), 2)
                self.assertTrue(all(not child.handle for child in children))
                self.assertTrue(getattr(primary, "brepgpu_cleanup_error", None)
                                or getattr(primary, "__notes__", None))
                qualify_batch(sampler.sample(request))


if __name__ == "__main__":
    unittest.main()
