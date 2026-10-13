"""Direct exact/analytic conformance of the public batch caller.

Explicitly requested backends are required: absent native libraries/devices are
errors, never successful skips. Set BREPGPU_TEST_BACKEND and BREPGPU_TEST_DEVICE
to run this same unit on CPU or either GPU.
"""

from array import array
from fractions import Fraction
import math
import os
import unittest

from brepgpu.fixtures import (
    BILINEAR_MIDPOINT_JET, ERROR_TARGETS, TERMS, TERM_COUNTS,
    analytic_jet, asymmetric_bicubic, exact_component_error, exact_jet,
    plane, qualification_fixtures, qualification_parameters,
    rational_cylinder, singular_chart, weighted_bilinear,
)


def patch_for(fixture):
    from brepgpu import Patch
    return Patch(degree_u=fixture.degree_u, degree_v=fixture.degree_v,
                 controls=fixture.controls)


def vector_at(jets, request, term):
    return tuple(jets.data[(term * 3 + axis) * jets.count + request] for axis in range(3))


def assert_jet(test, jets, request, expected, *, label=""):
    for term, vector in enumerate(expected):
        target = ERROR_TARGETS[sum(TERMS[term])]
        for axis, truth in enumerate(vector):
            observed = vector_at(jets, request, term)[axis]
            error = exact_component_error(observed, truth)
            if error is None or error > target:
                test.fail(f"{label}: request={request} term={term} axis={axis} "
                          f"value={observed!r} exact~={float(truth)!r} "
                          f"error~={float(error) if error is not None else 'nonfinite'} "
                          f"target={float(target)}")


class QualificationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from brepgpu import Engine
        cls.engine = Engine(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                            device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")))

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "engine"):
            cls.engine.close()

    def evaluate(self, fixtures, requests, order):
        with self.engine.create_patch_set([patch_for(f) for f in fixtures]) as patches:
            slots = array("I", (slot for slot, s, t in requests))
            uv = array("d", (value for slot, s, t in requests for value in (s, t)))
            with self.engine.create_query_set(patches, patch_slots=slots, uv=uv) as queries:
                with self.engine.evaluate(queries, order=order) as result:
                    return result.download()

    def test_all_degree_pairs_orders_and_exact_binary_inputs(self):
        fixtures = qualification_fixtures()
        parameters = qualification_parameters()
        requests = [(slot, s, t) for slot in range(len(fixtures)) for s, t in parameters]
        # Deliberately interleave degrees and reverse local query traversal.
        requests = requests[::2] + list(reversed(requests[1::2]))
        for order in (0, 1, 2):
            with self.subTest(order=order):
                jets = self.evaluate(fixtures, requests, order)
                self.assertEqual(jets.count, len(requests))
                self.assertEqual(jets.order, order)
                self.assertEqual(len(jets.data), 3 * TERM_COUNTS[order] * len(requests))
                self.assertEqual(list(jets.statuses), [0] * len(requests))
                jets.require_ok()
                for request, (slot, s, t) in enumerate(requests):
                    assert_jet(self, jets, request, exact_jet(fixtures[slot], s, t, order),
                               label=fixtures[slot].name)

    def test_independent_analytic_maps(self):
        fixtures = (plane(), asymmetric_bicubic(), rational_cylinder(), weighted_bilinear(), singular_chart())
        requests = [(slot, s, t) for slot in range(len(fixtures))
                    for s, t in ((0.0, 0.0), (0.5, 0.5), (1.0, 1.0), (0.25, 0.75))]
        jets = self.evaluate(fixtures, requests, 2)
        jets.require_ok()
        for i, (slot, s, t) in enumerate(requests):
            assert_jet(self, jets, i, analytic_jet(fixtures[slot], s, t), label="analytic " + fixtures[slot].name)

    def test_literal_dyadic_bilinear_full_jet(self):
        jets = self.evaluate((weighted_bilinear(),), ((0, 0.5, 0.5),), 2)
        jets.require_ok()
        for term, vector in enumerate(BILINEAR_MIDPOINT_JET):
            self.assertEqual(vector_at(jets, 0, term), tuple(float(x) for x in vector),
                             f"literal dyadic jet differs at term {term}")

    def test_public_vector_accessor_preserves_named_and_indexed_jet_order(self):
        jets = self.evaluate((weighted_bilinear(),), ((0,0.5,0.5),), 2)
        jets.require_ok()
        for term,(name,expected) in enumerate(zip(("S","Ss","St","Sss","Sst","Stt"),
                                                BILINEAR_MIDPOINT_JET)):
            literal=tuple(float(value) for value in expected)
            self.assertEqual(jets.vector(name,0),literal)
            self.assertEqual(jets.vector(term,0),literal)
        for term,index in ((-1,0),(6,0),(0,-1),(0,1)):
            with self.assertRaises(IndexError):jets.vector(term,index)

    def test_singular_chart_still_returns_position_and_derivatives(self):
        jets = self.evaluate((singular_chart(),), ((0, 0.0, 0.0),), 2)
        jets.require_ok()
        literal = ((0, 0, 0), (0, 0, 0), (0, 0, 0), (2, 0, 0), (0, 1, 0), (0, 0, 2))
        for term, vector in enumerate(literal):
            self.assertEqual(vector_at(jets, 0, term), vector)

    def test_rational_cylinder_locus_and_differentiated_identities(self):
        f = rational_cylinder()
        requests = [(0, s, 0.25) for s in (0.0, 0.125, 0.5, 0.875, 1.0)]
        jets = self.evaluate((f,), requests, 2)
        jets.require_ok()
        for i, (_, s, t) in enumerate(requests):
            assert_jet(self, jets, i, analytic_jet(f, s, t), label="cylinder")
            x, y, z = vector_at(jets, i, 0)
            xu, yu, zu = vector_at(jets, i, 1)
            xuu, yuu, zuu = vector_at(jets, i, 3)
            self.assertLessEqual(abs(x*x + y*y - 1), 8 * float(ERROR_TARGETS[0]))
            self.assertLessEqual(abs(x*xu + y*yu), 16 * float(ERROR_TARGETS[1]))
            self.assertLessEqual(abs(xu*xu + yu*yu + x*xuu + y*yuu), 32 * float(ERROR_TARGETS[2]))
            self.assertEqual((zu, zuu), (0.0, 0.0))

    def test_common_weight_scaling_and_parameter_axis_swap(self):
        from brepgpu.fixtures import Fixture
        f = weighted_bilinear()
        scaled = Fixture("scaled", f.degree_u, f.degree_v, tuple(x * 8 for x in f.controls))
        swapped_controls = tuple(value for j in range(f.degree_v + 1) for i in range(f.degree_u + 1)
                                 for value in f.controls[4*(i*(f.degree_v+1)+j):4*(i*(f.degree_v+1)+j)+4])
        swapped = Fixture("swapped", f.degree_v, f.degree_u, swapped_controls)
        requests = ((0, 0.25, 0.75), (1, 0.25, 0.75), (2, 0.75, 0.25))
        jets = self.evaluate((f, scaled, swapped), requests, 2)
        jets.require_ok()
        for i, (slot, s, t) in enumerate(requests):
            assert_jet(self, jets, i, exact_jet((f, scaled, swapped)[slot], s, t))
        for term in range(6):
            self.assertLessEqual(max(abs(a-b) for a,b in zip(vector_at(jets,0,term),vector_at(jets,1,term))),
                                 2 * float(ERROR_TARGETS[sum(TERMS[term])]))
        for original, swapped_term in enumerate((0,2,1,5,4,3)):
            self.assertLessEqual(max(abs(a-b) for a,b in zip(vector_at(jets,0,original),vector_at(jets,2,swapped_term))),
                                 2 * float(ERROR_TARGETS[sum(TERMS[original])]))

    def test_empty_singleton_and_tail_batches(self):
        f = rational_cylinder()
        for count in (0, 1, 31, 33, 257):
            with self.subTest(count=count):
                requests = [(0, (i % 17) / 16.0, (i % 13) / 12.0) for i in range(count)]
                jets = self.evaluate((f,), requests, 2)
                self.assertEqual(jets.count, count)
                self.assertEqual(len(jets.data), count * 18)
                self.assertEqual(list(jets.statuses), [0] * count)
                for i, (_, s, t) in enumerate(requests):
                    assert_jet(self, jets, i, exact_jet(f, s, t), label=f"tail {count}")

    def test_homogeneous_subdivision_preserves_geometry_and_derivative_scaling(self):
        from brepgpu.fixtures import Fixture
        f=weighted_bilinear()
        # A degree-one split at s=1/2 has exact dyadic homogeneous controls.
        # Constructing these small test inputs does not own a general extractor.
        midpoint=[(a+b)/2 for a,b in zip(f.controls[:8],f.controls[8:])]
        left=Fixture("bilinear-left",1,1,tuple(f.controls[:8])+tuple(midpoint))
        right=Fixture("bilinear-right",1,1,tuple(midpoint)+tuple(f.controls[8:]))
        requests=((0,0.125,0.75),(1,0.25,0.75),(0,0.625,0.75),(2,0.25,0.75))
        jets=self.evaluate((f,left,right),requests,2)
        jets.require_ok()
        for i,(slot,s,t) in enumerate(requests):
            assert_jet(self,jets,i,exact_jet((f,left,right)[slot],s,t),label="subdivision")
        for parent,child in ((0,1),(2,3)):
            for term,(a,b) in enumerate(TERMS):
                factor=2.0**(-a)
                for original,subdivided in zip(vector_at(jets,parent,term),vector_at(jets,child,term)):
                    self.assertLessEqual(abs(factor*original-subdivided),
                                         (1+factor)*float(ERROR_TARGETS[a+b]))


if __name__ == "__main__":
    unittest.main()
