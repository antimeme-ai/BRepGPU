"""Literal and original-source exact classifier truth; execute on the fleet."""
from dataclasses import FrozenInstanceError
from decimal import Decimal
from fractions import Fraction as F
from itertools import repeat
import math
import sys
import unittest


class ClassificationTests(unittest.TestCase):
    def point(self, body, query, label=None, faces=None):
        from brepgpu.classification import classify_point
        from brepgpu.classification_oracles import qualify_point
        result = classify_point(body, query)
        self.assertIs(result.snapshot, body)
        self.assertEqual(result.point, tuple(F(value) for value in query))
        qualify_point(result)
        if label is not None:
            self.assertEqual(result.label, label)
        if faces is not None:
            self.assertEqual(result.boundary_faces, faces)
        return result

    def contact(self, left, right, dimension=None, relation=None):
        from brepgpu.classification import contact_faces
        from brepgpu.classification_oracles import qualify_contact
        result = contact_faces(left, right)
        self.assertIs(result.left, left)
        self.assertIs(result.right, right)
        qualify_contact(result)
        if dimension is not None:
            self.assertEqual(result.dimension, dimension)
        if relation is not None:
            self.assertEqual(result.relation, relation)
        return result

    def face(self, shape="box", face=1, origin=(0., 0., 0.), scale=1.):
        from brepgpu.body import Frame, make_body
        from brepgpu.intersections import FaceRef
        return FaceRef(make_body(shape, Frame(origin, scale)), face)

    def test_literal_material_and_all_original_supporting_faces(self):
        from brepgpu.body import make_body
        from brepgpu.body_oracles import qualify_snapshot
        cases = {
            "box": (((0,0,0),"interior",()),((F(1,4),0,0),"interior",()),
                    ((0,0,F(1,2)),"boundary",(1,)),((F(3,4),F(3,4),F(1,2)),"boundary",(1,4,5)),
                    ((-F(3,4),-F(3,4),-F(1,2)),"boundary",(2,3,6)),
                    ((F(3,4),0,1),"exterior",())),
            "cylinder": (((0,0,0),"interior",()),((F(1,2),0,0),"exterior",()),
                         ((F(1,4),0,0),"boundary",(3,)),((F(1,4),0,F(1,2)),"boundary",(1,3)),
                         ((F(1,4),0,-F(1,2)),"boundary",(2,3)),((0,0,F(1,2)),"boundary",(1,)),
                         ((F(1,4),0,1),"exterior",())),
            "plate": (((0,0,0),"exterior",()),((F(1,2),0,0),"interior",()),
                      ((F(1,4),0,0),"boundary",(7,)),((F(1,4),0,F(1,2)),"boundary",(1,7)),
                      ((F(1,4),0,-F(1,2)),"boundary",(2,7)),((0,0,F(1,2)),"exterior",()),
                      ((F(3,4),F(3,4),F(1,2)),"boundary",(1,4,5)),((F(1,4),0,1),"exterior",())),
        }
        for shape, queries in cases.items():
            body = make_body(shape)
            qualify_snapshot(body)
            for query, label, faces in queries:
                with self.subTest(shape=shape, query=query):
                    self.point(body, query, label, faces)

    def test_exact_rational_circle_and_subnormal_radial_trap(self):
        from brepgpu.body import make_body
        tiny = math.nextafter(0., 1.)
        cylinder, plate = make_body("cylinder"), make_body("plate")
        self.point(cylinder, (F(3,20), F(1,5), 0), "boundary", (3,))
        self.point(plate, (F(3,20), F(1,5), 0), "boundary", (7,))
        # y² is strictly positive despite ordinary binary64 underflow/rounding.
        self.point(cylinder, (.25, tiny, 0), "exterior", ())
        self.point(plate, (.25, tiny, 0), "interior", ())
        for x, cylindrical, holed in ((math.nextafter(.25, 0.), "interior", "exterior"),
                                       (.25, "boundary", "boundary"),
                                       (math.nextafter(.25, 1.), "exterior", "interior")):
            self.point(cylinder, (x, 0, 0), cylindrical, (3,) if cylindrical=="boundary" else ())
            self.point(plate, (x, 0, 0), holed, (7,) if holed=="boundary" else ())
        # Decimal-looking floats are their actual binary values, not exact3/20,1/5.
        rounded = self.point(cylinder, (.15, .2, 0))
        self.assertNotEqual(rounded.label, "boundary")
        self.assertEqual(rounded.point, (F.from_float(.15), F.from_float(.2), F(0)))

    def test_one_ulp_slab_boundaries_all_axis_sides(self):
        from brepgpu.body import make_body
        box = make_body("box")
        for axis, extent, negative_face, positive_face in ((0,.75,6,5),(1,.75,3,4),(2,.5,2,1)):
            for sign, face in ((-1,negative_face),(1,positive_face)):
                boundary = sign*extent
                for value, label, faces in ((math.nextafter(boundary,0.),"interior",()),
                                             (boundary,"boundary",(face,)),
                                             (math.nextafter(boundary,sign*math.inf),"exterior",())):
                    query = [0,0,0]
                    query[axis] = value
                    self.point(box, query, label, faces)

    def test_original_tiny_frame_offsets_never_round_membership(self):
        from brepgpu.body import Frame, make_body
        tiny = math.nextafter(0., 1.)
        box = make_body("box", Frame((tiny,0.,0.),1.))
        self.point(box, (.75,0,0), "interior", ())
        self.point(box, (-.75,0,0), "exterior", ())
        self.point(box, (F(3,4)+F.from_float(tiny),0,0), "boundary", (5,))
        for shape in ("box","cylinder","plate"):
            for scale in (.25,1.,4.):
                frame = Frame((16.,-16.,.125),scale)
                body = make_body(shape,frame)
                local = (F(1,4),0,F(1,2))
                world = tuple(F.from_float(o)+F.from_float(scale)*value for o,value in zip(frame.origin,local))
                self.point(body, world, "boundary", (1,) if shape=="box" else (1,3) if shape=="cylinder" else (1,7))

    def test_extreme_scalar_values_and_owned_ordered_batches(self):
        from brepgpu.body import make_body
        from brepgpu.classification import classify_points
        from brepgpu.classification_oracles import qualify_point
        body = make_body("cylinder")
        queries = ((0,-0.,0), (sys.float_info.max,0,0), (-(1<<2048),0,0),
                   (F(1,4)+F(1,1<<2048),0,0), (F(1,4)-F(1,1<<2048),0,0),
                   (sys.float_info.min, math.nextafter(0.,1.),0))
        result = classify_points(body, iter(queries))
        self.assertIsInstance(result, tuple)
        self.assertEqual(tuple(r.label for r in result), ("interior","exterior","exterior","exterior","interior","interior"))
        for original, actual in zip(queries,result):
            self.assertEqual(actual.point, tuple(F(x) for x in original))
            self.assertIs(actual.snapshot,body)
            qualify_point(actual)

    def test_closed_coplanar_point_segment_area_and_same_domain(self):
        left = self.face()
        examples = ((self.face(origin=(0.,0.,0.)),2,"coincident",True),
                    (self.face(origin=(1.,0.,0.)),2,"overlap",False),
                    (self.face(origin=(1.5,0.,0.)),1,"touch",False),
                    (self.face(origin=(1.5,1.5,0.)),0,"touch",False),
                    (self.face(origin=(math.nextafter(1.5,math.inf),0.,0.)),-1,"disjoint",False))
        for right, dimension, relation, same in examples:
            with self.subTest(dimension=dimension, relation=relation):
                result = self.contact(left,right,dimension,relation)
                self.assertEqual(result.same_domain,same)
                self.assertEqual(result.carrier_relation,"coplanar")
                self.assertEqual(result.normal_relation,"same")
                if dimension==1:
                    self.assertEqual(tuple(v.position for v in result.vertices),
                                     ((F(3,4),-F(3,4),F(1,2)),(F(3,4),F(3,4),F(1,2))))
                if dimension==0:
                    self.assertEqual(result.vertices[0].position,(F(3,4),F(3,4),F(1,2)))
                if relation=="overlap":
                    self.assertEqual({v.position for v in result.vertices},
                                     {(x,y,F(1,2)) for x in (F(1,4),F(3,4)) for y in (-F(3,4),F(3,4))})

    def test_parallel_original_subnormal_planes_and_opposed_normals(self):
        tiny = math.nextafter(0.,1.)
        left = self.face()
        separated = self.contact(left,self.face(origin=(0.,0.,tiny)),-1,"disjoint")
        self.assertEqual(separated.carrier_relation,"parallel_disjoint")
        self.assertFalse(separated.same_domain)
        opposed = self.contact(left,self.face(face=2,origin=(0.,0.,1.)),2,"coincident")
        self.assertEqual(opposed.normal_relation,"opposite")
        self.assertTrue(opposed.same_domain)
        self.assertNotEqual((opposed.left.snapshot.body_id,opposed.left.face_id),
                            (opposed.right.snapshot.body_id,opposed.right.face_id))
        flat_disjoint = self.contact(left,self.face(origin=(2.,0.,0.)),-1,"disjoint")
        self.assertEqual(flat_disjoint.carrier_relation,"coplanar")
        self.assertFalse(flat_disjoint.same_domain)

    def test_transverse_crossing_versus_finite_boundary_touch(self):
        left = self.face()
        # Translated front lies on y=0 and extends through z=.5 internally.
        right = self.face(face=3,origin=(0.,.75,.5))
        crossing = self.contact(left,right,1,"crossing")
        self.assertEqual(crossing.normal_relation,"orthogonal")
        self.assertEqual(crossing.carrier_relation,"transverse")
        self.assertFalse(crossing.same_domain)
        self.assertEqual(tuple(v.position for v in crossing.vertices),
                         ((-F(3,4),F(0),F(1,2)),(F(3,4),F(0),F(1,2))))
        self.assertEqual(tuple(v.left_uv for v in crossing.vertices),((F(0),F(1,2)),(F(1),F(1,2))))
        self.assertEqual(tuple(v.right_uv for v in crossing.vertices),((F(0),F(1,2)),(F(1),F(1,2))))
        from brepgpu.classification_oracles import qualify_contact
        via_body = left.snapshot.contact_faces(1,right.snapshot,3)
        qualify_contact(via_body)
        self.assertIs(via_body.left.snapshot,left.snapshot)
        self.assertIs(via_body.right.snapshot,right.snapshot)
        self.assertEqual((via_body.left.face_id,via_body.right.face_id),(1,3))
        self.assertEqual(via_body.vertices,crossing.vertices)
        edge_touch = self.contact(left,self.face(face=5),1,"touch")
        self.assertEqual(edge_touch.carrier_relation,"transverse")
        self.contact(left,self.face(face=3,origin=(1.5,0.,0.)),0,"touch")
        self.contact(left,self.face(face=3,origin=(2.,0.,0.)),-1,"disjoint")

    def test_all_rectangular_face_orientations_source_reversal_and_frames(self):
        from brepgpu.body import Frame, make_body
        from brepgpu.intersections import FaceRef
        for shape, ids in (("box",range(1,7)),("plate",range(3,7))):
            a = make_body(shape,Frame((16.,-16.,.125),.25))
            b = make_body(shape,Frame((16.,-16.,.125),.25))
            for face in ids:
                result = self.contact(FaceRef(a,face),FaceRef(b,face),2,"coincident")
                swapped = self.contact(result.right,result.left,2,"coincident")
                by_world = {v.position:(v.left_uv,v.right_uv) for v in result.vertices}
                self.assertEqual({v.position:(v.right_uv,v.left_uv) for v in swapped.vertices},by_world)
        for left_face in range(1,7):
            for right_face in range(1,7):
                result = self.contact(self.face(face=left_face),self.face(face=right_face,origin=(.25,.5,.125),scale=.5))
                swapped = self.contact(result.right,result.left)
                self.assertEqual((swapped.dimension,swapped.relation,swapped.same_domain,swapped.normal_relation),
                                 (result.dimension,result.relation,result.same_domain,result.normal_relation))
                self.assertEqual({v.position:(v.right_uv,v.left_uv) for v in swapped.vertices},
                                 {v.position:(v.left_uv,v.right_uv) for v in result.vertices})

    def test_unsupported_faces_are_not_empty_contacts(self):
        from brepgpu.classification import UnsupportedContact, contact_faces
        left = self.face()
        for face in (self.face("cylinder",1),self.face("cylinder",2),self.face("cylinder",3),
                     self.face("plate",1),self.face("plate",2),self.face("plate",7)):
            with self.assertRaises(UnsupportedContact):
                contact_faces(left,face)
            with self.assertRaises(UnsupportedContact):
                contact_faces(face,left)
        for wrong in (None,left.snapshot,1):
            with self.assertRaises(TypeError):
                contact_faces(left,wrong)

    def test_invalid_queries_atomic_batch_and_factory_immutability(self):
        from brepgpu.body import make_body
        from brepgpu.classification import classify_point, classify_points, PointClassification, FaceContact, ContactVertex
        body = make_body("box")
        for query in ((),(0,0),(0,0,0,0),(True,0,0),(float("nan"),0,0),
                      (0,float("inf"),0),(Decimal("0"),0,0),("0",0,0),None,repeat(0)):
            with self.subTest(query_type=type(query).__name__):
                with self.assertRaises((ValueError,TypeError)):
                    classify_point(body,query)
        with self.assertRaises((ValueError,TypeError)):
            classify_points(body,((0,0,0),(0,0,float("nan")),(0,0,0)))
        def failed_input():
            yield (0,0,0)
            raise RuntimeError("classification input iterator failure")
        with self.assertRaisesRegex(RuntimeError,"classification input iterator failure"):
            classify_points(body,failed_input())
        self.assertEqual(classify_points(body,()),())
        self.assertEqual(classify_points(body,((0,0,0),))[0].label,"interior")
        with self.assertRaises(TypeError):
            classify_point(None,(0,0,0))
        point = self.point(body,(0,0,0))
        contact = self.contact(self.face(),self.face())
        for value,field in ((point,"label"),(contact,"dimension"),(contact.vertices[0],"left_uv")):
            with self.assertRaises((FrozenInstanceError,AttributeError,TypeError)):
                setattr(value,field,None)
        for cls in (PointClassification,FaceContact,ContactVertex):
            with self.assertRaises(TypeError):
                cls()


if __name__ == "__main__":
    unittest.main()
