"""Direct complete analytic intersections; actual native tests run on the fleet."""
from fractions import Fraction
import os
import unittest


def engine_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")),
                plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN"))


class IntersectionTests(unittest.TestCase):
    def assert_rational_values(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        for value, reference in zip(actual, expected):
            # Inspect raw exact output; do not use production equality/rounding
            # as the expected answer in this literal first contract.
            self.assertEqual(value.b, 0)
            self.assertEqual(value.a, Fraction(reference))

    def test_body_entry_point_complete_circle_and_coupled_maps(self):
        from brepgpu import Engine
        from brepgpu.body import Frame, make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
        plane = make_body("box", Frame((0., 0., -.5), 1.))
        wall = make_body("cylinder")
        # Qualified existing body/native setup precedes the intended absent
        # BodySnapshot.intersect_faces red; no new-module import gates it.
        with Engine(**engine_arguments()) as engine:
            for snapshot, slot, expected in ((plane, 0, (0., 0., 0.)),
                                              (wall, 2, (.25, 0., 0.))):
                with BodySampler(engine, snapshot) as sampler:
                    request = SampleRequest(snapshot, (Candidate(slot, .5 if slot == 0 else 0., .5),))
                    batch = sampler.sample(request)
                self.assertEqual(batch.rows[0].native_status, 0)
                self.assertEqual(batch.rows[0].position, expected)
            result = plane.intersect_faces(1, wall, 3)
        self.assertIs(result.left.snapshot, plane)
        self.assertIs(result.right.snapshot, wall)
        self.assertEqual(len(result.components), 1)
        self.assertTrue(result.components[0].closed)
        self.assertFalse(result.components[0].tangent)
        self.assertEqual(len(result.components[0].piece_ids), 4)
        self.assertEqual(len(result.pieces), 4)
        self.assertEqual(len({piece.span_id for piece in result.pieces}), 4)
        self.assertEqual({(piece.left_slot, piece.right_slot) for piece in result.pieces},
                         {(0, 2), (0, 3), (0, 4), (0, 5)})
        piece = next(piece for piece in result.pieces if piece.right_slot == 2)
        self.assert_rational_values((piece.lo, piece.hi), (0, 1))
        self.assertEqual(len(piece.spatial_map.numerators), 3)
        self.assertEqual(len(piece.left_map.numerators), 2)
        self.assertEqual(len(piece.right_map.numerators), 2)
        self.assertIsInstance(piece.spatial_map.denominator, tuple)
        for parameter, position, left_uv, right_uv in (
                (Fraction(0), (Fraction(1,4), 0, 0), (Fraction(2,3), Fraction(1,2)), (0, Fraction(1,2))),
                (Fraction(1,2), (Fraction(3,20), Fraction(1,5), 0), (Fraction(3,5), Fraction(19,30)), (Fraction(1,2), Fraction(1,2))),
                (Fraction(1), (0, Fraction(1,4), 0), (Fraction(1,2), Fraction(2,3)), (1, Fraction(1,2)))):
            self.assert_rational_values(piece.position(parameter), position)
            self.assert_rational_values(piece.left_uv(parameter), left_uv)
            self.assert_rational_values(piece.right_uv(parameter), right_uv)


    def test_two_sided_circle_consumer_preserves_sources_and_native_rows(self):
        from brepgpu import Engine
        from brepgpu.body import Frame, make_body
        plane=make_body("box",Frame((0.,0.,-.5),1.));wall=make_body("cylinder")
        result=plane.intersect_faces(1,wall,3)
        # Root runs this after construction is healthy, before sampler module.
        from brepgpu.intersection_sampling import prepare_samples, IntersectionSampler
        request=prepare_samples(result,2)
        self.assertIs(request.result,result)
        self.assertEqual(len(request.candidates),8)
        with Engine(**engine_arguments()) as engine:
            with IntersectionSampler(engine,result) as sampler:
                batch=sampler.sample(request)
        self.assertIs(batch.result,result)
        self.assertEqual((batch.count,batch.evaluated_count),(8,16))
        self.assertEqual(len(batch.rows),8)
        cardinals=((Fraction(1,4),0,0),(0,Fraction(1,4),0),
                   (-Fraction(1,4),0,0),(0,-Fraction(1,4),0))
        for row,candidate in zip(batch.rows,request.candidates):
            self.assertEqual((row.piece_id,row.component_id,row.span_id,row.location),
                             (candidate.piece_id,candidate.component_id,candidate.span_id,candidate.location))
            self.assert_rational_values((row.parameter,), (candidate.parameter.a,))
            self.assertEqual(row.parameter.b,0)
            self.assertEqual((row.left_status,row.right_status),(0,0))
            self.assertEqual(row.left_slot,0)
            self.assertIn(row.right_slot,(2,3,4,5))
            q=row.right_slot-2
            expected=cardinals[(q+int(row.parameter.a))%4]
            self.assert_rational_values(row.reference_position,expected)
            for values in (row.left_position,row.right_position):
                self.assertEqual(len(values),3)
                for actual,reference in zip(values,expected):
                    self.assertLessEqual(abs(Fraction.from_float(actual)-reference),Fraction(1,1<<36))
            self.assertEqual(row.right_uv,(float(row.parameter.a),.5))
            expected_plane=((expected[0]+Fraction(3,4))/Fraction(3,2),
                            (expected[1]+Fraction(3,4))/Fraction(3,2))
            for actual,reference in zip(row.left_uv,expected_plane):
                self.assertLessEqual(abs(Fraction.from_float(actual)-reference),Fraction(1,1<<54))


    def test_four_clipped_arcs_complete_exact_endpoints(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import recipe_counts
        plane=make_body("box",Frame((0.,0.,-.125),.25));wall=make_body("cylinder")
        for result in (plane.intersect_faces(1,wall,3),wall.intersect_faces(3,plane,1)):
            self.assertEqual(recipe_counts(result,"clipped")["arcs"],4)

    def test_small_wall_inside_plate_hole_is_empty(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_result
        plate=make_body("plate",Frame((0.,0.,-.5),1.))
        wall=make_body("cylinder",Frame((0.,0.,0.),.5))
        result=plate.intersect_faces(1,wall,3)
        self.assertEqual(qualify_result(result)["components"],0)
        self.assertEqual((result.pieces,result.events),((),()))

    def test_cardinal_generators_one_physical_span_per_segment(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_result
        plane=make_body("box",Frame((-.75,0.,0.),1.));wall=make_body("cylinder")
        result=plane.intersect_faces(5,wall,3)
        metrics=qualify_result(result)
        self.assertEqual(tuple(metrics[k] for k in ("components","pieces","spans","events")),(2,4,2,4))
        for component in result.components:
            self.assertEqual(component.kind,"segment")
            self.assertFalse(component.closed)
            self.assertFalse(component.tangent)
            self.assertEqual(len({result.pieces[i].span_id for i in component.piece_ids}),1)
            self.assertEqual(len(component.piece_ids),2)

    def test_hole_boundary_circle_and_crossing_arc(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_result
        plate=make_body("plate",Frame((0.,0.,-.5),1.))
        wall=make_body("cylinder")
        circle=plate.intersect_faces(1,wall,3)
        self.assertEqual(qualify_result(circle)["circles"],1)
        self.assertTrue(all(p.boundary_constraints==("left:hole",) for p in circle.pieces))
        displaced=make_body("cylinder",Frame((.25,0.,0.),1.))
        arc=plate.intersect_faces(1,displaced,3)
        metrics=qualify_result(arc)
        self.assertEqual(tuple(metrics[k] for k in ("components","pieces","spans","events")),(1,4,4,5))
        self.assertEqual(arc.components[0].kind,"arc")

    def test_disk_lens_and_external_trim_tangent_point(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_result
        disk=make_body("cylinder",Frame((0.,0.,-.5),1.))
        wall=make_body("cylinder",Frame((.25,0.,0.),1.))
        lens=disk.intersect_faces(1,wall,3)
        m=qualify_result(lens)
        self.assertEqual(tuple(m[k] for k in ("components","pieces","spans","events")),(1,2,2,3))
        tangent=disk.intersect_faces(1,make_body("cylinder",Frame((.5,0.,0.),1.)),3)
        m=qualify_result(tangent)
        self.assertEqual(tuple(m[k] for k in ("components","pieces","spans","events")),(1,2,1,1))
        self.assertEqual(tangent.components[0].kind,"point")
        self.assertFalse(tangent.components[0].tangent) # trims touch; carriers cross
        for p in tangent.pieces:self.assert_rational_values(p.position(p.lo),(Fraction(1,4),0,0))

    def test_internal_disk_touch_is_retained_degree_two_event(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_result
        disk=make_body("cylinder",Frame((0.,0.,-2.),4.))
        wall=make_body("cylinder",Frame((.5625,.75,0.),.25))
        result=disk.intersect_faces(1,wall,3)
        m=qualify_result(result)
        self.assertEqual(tuple(m[k] for k in ("components","pieces","spans","events")),(1,5,5,5))
        self.assertTrue(result.components[0].closed)
        self.assertEqual(sum("left:rim" in e.constraints for e in result.events),1)

    def test_vertical_irrational_tangent_point_and_subnormal_branches(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_result,equal,Value
        wall=make_body("cylinder")
        plane=make_body("box",Frame((-.625,0.,0.),1.))
        result=plane.intersect_faces(5,wall,3)
        m=qualify_result(result)
        self.assertEqual(tuple(m[k] for k in ("components","pieces","spans","events")),(2,2,2,4))
        ys=[p.spatial_map.numerators[1][0] for p in result.pieces]
        self.assertTrue(any(equal(y,Value(0,Fraction(1,8),3)) for y in ys))
        self.assertTrue(any(equal(y,Value(0,Fraction(-1,8),3)) for y in ys))
        tangent_plane=make_body("box",Frame((-.5,0.,0.),1.))
        tangent=tangent_plane.intersect_faces(5,wall,3)
        m=qualify_result(tangent)
        self.assertEqual(tuple(m[k] for k in ("components","pieces","spans","events")),(1,2,1,2))
        self.assertTrue(tangent.components[0].tangent)
        point=make_body("box",Frame((-.5,0.,1.),1.)).intersect_faces(5,wall,3)
        self.assertEqual(qualify_result(point)["points"],1)
        self.assertEqual((len(point.pieces),len({p.span_id for p in point.pieces}),len(point.events)),(2,1,1))
        tiny=float.fromhex("0x0.0000000000001p-1022")
        for shift,count in ((tiny,2),(-tiny,0)):
            shifted=make_body("cylinder",Frame((shift,0.,0.),1.))
            near=tangent_plane.intersect_faces(5,shifted,3)
            self.assertEqual(qualify_result(near)["components"],count)
            self.assertTrue(all(not c.tangent for c in near.components))

    def test_scalar_exact_fields_and_binary64_rounding_boundaries(self):
        import math,struct
        from brepgpu.intersections import Scalar
        from brepgpu.intersection_oracles import nearest,Value,equal,compare
        self.assertEqual(Scalar(0,1,8),Scalar(0,2,2))
        self.assertTrue(Scalar(0,1,2)<Scalar(0,1,3))
        self.assert_rational_values((Scalar(1,1,2)*Scalar(1,-1,2),),(-1,))
        self.assertTrue(equal(Scalar(0,1,8)+Scalar(0,1,2),Value(0,3,2)))
        with self.assertRaises(ValueError):Scalar(0,1,2)+Scalar(0,1,3)
        minimum=Fraction(1,1<<1074);normal=Fraction(1,1<<1022)
        maximum=Fraction.from_float(float.fromhex("0x1.fffffffffffffp+1023"))
        cases=(Fraction(1)+Fraction(1,1<<53),Fraction(1)+Fraction(3,1<<53),
               minimum/2,-minimum/2,3*minimum/2,-3*minimum/2,
               normal-minimum/2,normal+minimum/2,maximum)
        for rational in cases:
            with self.subTest(value=rational):
                value=Scalar(rational);lo,hi=value.bracket()
                self.assertLessEqual(Fraction.from_float(lo),rational)
                self.assertGreaterEqual(Fraction.from_float(hi),rational)
                if lo!=hi:self.assertEqual(math.nextafter(lo,math.inf),hi)
                self.assertEqual(struct.pack("<d",float(value)),struct.pack("<d",float(rational)))
        for value in (Scalar(0,1,2),Scalar(1,-1,2),Scalar(1,Fraction(1,1<<1100),2)):
            self.assertEqual(float(value),nearest(value))
            lo,hi=value.bracket()
            self.assertLessEqual(compare(lo,value),0);self.assertGreaterEqual(compare(hi,value),0)
        with self.assertRaises(OverflowError):float(Scalar(maximum+minimum))

    def test_sampling_exact_sources_clipped_swapped_and_empty(self):
        from brepgpu import Engine
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_batch,recipe_counts
        from brepgpu.intersection_sampling import prepare_samples,IntersectionSampler
        plane=make_body("box",Frame((0.,0.,-.125),.25));wall=make_body("cylinder")
        with Engine(**engine_arguments()) as engine:
            for result in (plane.intersect_faces(1,wall,3),wall.intersect_faces(3,plane,1)):
                recipe_counts(result,"clipped");request=prepare_samples(result,7)
                with IntersectionSampler(engine,result) as sampler:batch=sampler.sample(request)
                m=qualify_batch(batch,request)
                self.assertEqual((m["count"],m["evaluated_count"]),(28,56))
            empty=make_body("box",Frame((0.,0.,2.),1.)).intersect_faces(1,wall,3)
            empty_request=prepare_samples(empty,2)
            with IntersectionSampler(engine,empty) as sampler:batch=sampler.sample(empty_request)
            self.assertEqual(qualify_batch(batch,empty_request)["count"],0)
        # Actual returned bytes/positions remain qualified after every owner closed.
        qualify_batch(batch,empty_request)

    def test_sampling_request_identity_immutability_and_resolution_rejection(self):
        from dataclasses import FrozenInstanceError
        from brepgpu import Engine,BgError
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_sampling import prepare_samples,IntersectionSampler,IntersectionSampleRequest
        plane=make_body("box",Frame((0.,0.,-.5),1.));wall=make_body("cylinder")
        result=plane.intersect_faces(1,wall,3);other=plane.intersect_faces(1,wall,3)
        request=prepare_samples(result,2)
        with self.assertRaises(FrozenInstanceError):request.resolution=3
        with self.assertRaises(TypeError):IntersectionSampleRequest(result,(),2)
        for invalid in (1,4098,True,2.5):
            with self.assertRaises((TypeError,ValueError)):prepare_samples(result,invalid)
        with Engine(**engine_arguments()) as engine:
            sampler=IntersectionSampler(engine,result)
            with self.assertRaises(ValueError):sampler.sample(prepare_samples(other,2))
            sampler.close()
            with self.assertRaises(BgError):sampler.sample(request)

    def test_unsupported_pair_explicit(self):
        from brepgpu.body import make_body
        from brepgpu.intersections import UnsupportedIntersection
        box=make_body("box");wall=make_body("cylinder")
        for left,lf,right,rf in ((box,1,box,2),(wall,3,wall,3)):
            with self.assertRaises(UnsupportedIntersection):left.intersect_faces(lf,right,rf)


    def test_sampling_primary_error_survives_both_child_cleanup_failures(self):
        from unittest.mock import patch
        from brepgpu import Engine
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_sampling import prepare_samples,IntersectionSampler
        from brepgpu.intersection_oracles import qualify_batch
        primary=ArithmeticError("chosen intersection download failure");children=[]
        original_query=Engine.create_query_set;original_evaluate=Engine.evaluate
        def query_with_cleanup(engine,*args,**kwargs):
            child=original_query(engine,*args,**kwargs);children.append(child);close=child.close
            def fail_close():
                child.close=close;close();raise SystemExit("chosen query close failure")
            child.close=fail_close;return child
        def result_with_failure(engine,*args,**kwargs):
            child=original_evaluate(engine,*args,**kwargs);children.append(child);close=child.close
            def fail_download():raise primary
            def fail_close():
                child.close=close;close();raise KeyboardInterrupt("chosen result close failure")
            child.download=fail_download;child.close=fail_close;return child
        result=make_body("box",Frame((0.,0.,-.5),1.)).intersect_faces(1,make_body("cylinder"),3)
        request=prepare_samples(result,2)
        with Engine(**engine_arguments()) as engine:
            with IntersectionSampler(engine,result) as sampler:
                with patch.object(Engine,"create_query_set",query_with_cleanup),patch.object(Engine,"evaluate",result_with_failure):
                    with self.assertRaises(ArithmeticError) as caught:sampler.sample(request)
                self.assertIs(caught.exception,primary)
                self.assertEqual(len(children),2)
                self.assertTrue(all(not child.handle for child in children))
                self.assertTrue(getattr(primary,"brepgpu_cleanup_error",None) or getattr(primary,"__notes__",None))
                qualify_batch(sampler.sample(request),request)

    def test_sampling_rejects_actual_returned_uv_and_position_corruption(self):
        from unittest.mock import patch
        from brepgpu import Engine
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_sampling import prepare_samples,IntersectionSampler,SamplingArithmeticError
        from brepgpu.intersection_oracles import qualify_batch
        result=make_body("box",Frame((0.,0.,-.5),1.)).intersect_faces(1,make_body("cylinder"),3)
        request=prepare_samples(result,2);original=Engine.evaluate;children=[]
        with Engine(**engine_arguments()) as engine:
            with IntersectionSampler(engine,result) as sampler:
                for fault,error in (("uv",RuntimeError),("position",SamplingArithmeticError)):
                    def evaluate(engine,*args,**kwargs):
                        child=original(engine,*args,**kwargs);children.append(child);download=child.download
                        def corrupt():
                            batch=download()
                            if fault=="uv":batch._uv[0]=.125
                            else:batch.data[0]+=.125
                            return batch
                        child.download=corrupt;return child
                    with patch.object(Engine,"evaluate",evaluate):
                        with self.assertRaises(error):sampler.sample(request)
                    self.assertFalse(children[-1].handle)
                qualify_batch(sampler.sample(request),request)


    def test_preparation_caps_refuse_without_invalidating_exact_result(self):
        from unittest.mock import patch
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_oracles import qualify_result,recipe_counts
        import brepgpu.intersection_sampling as sampling
        wall=make_body("cylinder")
        circle=make_body("box",Frame((0.,0.,-.5),1.)).intersect_faces(1,wall,3)
        with patch.object(sampling,"MAX_CANDIDATES",7):
            with self.assertRaises(ValueError):sampling.prepare_samples(circle,2)
        self.assertEqual(len(sampling.prepare_samples(circle,2).candidates),8)
        recipe_counts(circle,"circle")
        tiny=float.fromhex("0x0.0000000000001p-1022")
        narrow=make_body("plate",Frame((tiny,0.,-.5),1.)).intersect_faces(1,wall,3)
        qualify_result(narrow)
        # Scope only the actual admission budget; exact construction is intact.
        # This does not claim the production 4096-step cap is reached here.
        with patch.object(sampling,"MAX_SUBDIVISIONS",1):
            with self.assertRaises(sampling.SamplingPrecisionError):sampling.prepare_samples(narrow,3)
        self.assertTrue(sampling.prepare_samples(narrow,3).candidates)
        self.assertEqual(len(sampling.prepare_samples(circle,3).candidates),12)


    def test_sampling_consumes_generators_points_and_framed_faces(self):
        from brepgpu import Engine
        from brepgpu.body import Frame,make_body
        from brepgpu.intersection_sampling import prepare_samples,IntersectionSampler
        from brepgpu.intersection_oracles import qualify_batch,equal,Value
        wall=make_body("cylinder")
        # Literal complete counts are distinct from original-source map and
        # actual native consumption grades below. No constructor supplies truth.
        cases=(
            ("irrational_x",make_body("box",Frame((-.625,0.,0.),1.)),5,wall,(2,2,2,4),"segment",False,True),
            ("irrational_y",make_body("box",Frame((0.,-.625,0.),1.)),4,wall,(2,2,2,4),"segment",False,False),
            ("cardinal",make_body("box",Frame((-.75,0.,0.),1.)),5,wall,(2,4,2,4),"segment",False,False),
            ("carrier_tangent",make_body("box",Frame((-.5,0.,0.),1.)),5,wall,(1,2,1,2),"segment",True,False),
            ("height_point",make_body("box",Frame((-.5,0.,1.),1.)),5,wall,(1,2,1,1),"point",True,False),
            ("trim_point",make_body("cylinder",Frame((0.,0.,-.5),1.)),1,
             make_body("cylinder",Frame((.5,0.,0.),1.)),(1,2,1,1),"point",False,False),
            ("max_frame",make_body("box",Frame((13.5,16.,16.),4.)),5,
             make_body("cylinder",Frame((16.,16.,16.),4.)),(2,2,2,4),"segment",False,True))
        with Engine(**engine_arguments()) as engine:
            for name,plane,face,cylinder,counts,kind,tangent,reverse in cases:
                for swapped in ((False,True) if reverse else (False,)):
                    with self.subTest(case=name,swapped=swapped):
                        result=cylinder.intersect_faces(3,plane,face) if swapped else plane.intersect_faces(face,cylinder,3)
                        self.assertEqual((len(result.components),len(result.pieces),
                                          len({p.span_id for p in result.pieces}),len(result.events)),counts)
                        self.assertTrue(all(c.kind==kind and c.tangent==tangent and not c.closed for c in result.components))
                        if name in ("irrational_x","irrational_y"):
                            varying_axis=1 if name=="irrational_x" else 0
                            actual=[p.spatial_map.numerators[varying_axis][0] for p in result.pieces]
                            self.assertTrue(any(equal(v,Value(0,Fraction(1,8),3)) for v in actual))
                            self.assertTrue(any(equal(v,Value(0,Fraction(-1,8),3)) for v in actual))
                        elif name=="max_frame":
                            actual=[p.spatial_map.numerators[1][0] for p in result.pieces]
                            self.assertTrue(any(equal(v,Value(16,Fraction(1,2),3)) for v in actual))
                            self.assertTrue(any(equal(v,Value(16,Fraction(-1,2),3)) for v in actual))
                        request=prepare_samples(result,3)
                        with IntersectionSampler(engine,result) as sampler:batch=sampler.sample(request)
                        metrics=qualify_batch(batch,request)
                        expected_candidates=counts[1]*(1 if kind=="point" else 3)
                        self.assertEqual((metrics["count"],metrics["evaluated_count"]),
                                         (expected_candidates,2*expected_candidates))
                        self.assertTrue(all(row.left_status==row.right_status==0 for row in batch.rows))
        # Large transformed actual metadata/model remains readable after close.
        qualify_batch(batch,request)


if __name__ == "__main__":
    unittest.main()
