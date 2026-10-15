"""Complete literal/source-domain arrangement tests; execute only on fleet."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
from itertools import repeat
import math
import unittest


class ArrangementTests(unittest.TestCase):
    def face(self, face_id=1, origin=(0.,0.,-.5), scale=1., shape="box"):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersections import FaceRef
        return FaceRef(make_body(shape,Frame(origin,scale)),face_id)

    def circle(self, face, center=(0.,0.), scale=1., reverse=False):
        from brepgpu.body import Frame,make_body
        from brepgpu.intersections import FaceRef,intersect_faces
        height = F(face.snapshot.frame.origin[2])+F(face.snapshot.frame.scale)*(F(1,2) if face.face_id == 1 else -F(1,2))
        wall = FaceRef(make_body("cylinder",Frame((float(center[0]),float(center[1]),float(height)),scale)),3)
        return intersect_faces(wall,face) if reverse else intersect_faces(face,wall)

    def arranged(self, face, sections=(), u_cuts=(), v_cuts=()):
        from brepgpu.arrangements import arrange_face
        from brepgpu.arrangement_oracles import qualify_arrangement
        result = arrange_face(face,sections,u_cuts=u_cuts,v_cuts=v_cuts)
        self.assertIs(result.face,face)
        self.assertEqual(len(result.sections),len(sections))
        for original,retained in zip(sections,result.sections): self.assertIs(retained,original)
        self.assertEqual(result.u_cuts,tuple(sorted(F(x) for x in u_cuts)))
        self.assertEqual(result.v_cuts,tuple(sorted(F(x) for x in v_cuts)))
        return result,qualify_arrangement(result)

    def locations(self, result, queries, expected=None):
        from brepgpu.arrangement_oracles import qualify_locations
        outputs = tuple(result.locate_uv(*query) for query in queries)
        qualify_locations(result,queries,outputs)
        if expected is not None:self.assertEqual(outputs,expected)
        return outputs

    def test_bare_rectangle_complete_graph_and_exterior(self):
        face = self.face()
        result,counts = self.arranged(face)
        self.assertEqual((counts["vertices"],counts["edges"],counts["uses"],counts["loops"],counts["regions"]),(4,4,8,2,2))
        self.assertEqual(result.regions[1].holes,())
        self.assertEqual((result.regions[1].area.rational,result.regions[1].area.pi),(F(1),F(0)))
        self.locations(result,((F(1,2),F(1,2)),(0,0),(1,F(1,2)),(-1,0),(2,2)),
                       ((1,),(0,1),(0,1),(0,),(0,)))
        via_body = face.snapshot.arrange_face(face.face_id)
        from brepgpu.arrangement_oracles import qualify_arrangement
        qualify_arrangement(via_body)
        self.assertIs(via_body.face.snapshot,face.snapshot)
        self.assertEqual(via_body.face.face_id,face.face_id)

    def test_grid_shared_twins_and_closed_lookup(self):
        result,counts = self.arranged(self.face(),u_cuts=(F(1,2),),v_cuts=(F(1,2),))
        self.assertEqual((counts["vertices"],counts["edges"],counts["uses"],counts["regions"]),(9,12,24,5))
        self.assertEqual(tuple(r.cell for r in result.regions[1:]),((0,0),(1,0),(0,1),(1,1)))
        self.locations(result,((F(1,2),F(1,2)),(F(1,2),0),(0,F(1,2)),(F(1,4),F(1,4)),(F(3,4),F(3,4))),
                       ((1,2,3,4),(0,1,2),(0,1,3),(1,),(4,)))
        center = next(v for v in result.vertices if v.source == ("grid",1,1))
        incident = [edge for edge in result.edges if center.id in (edge.start,edge.end)]
        self.assertEqual(len(incident),4)
        self.assertEqual({use.orientation for use in result.uses},{-1,1})
        for use in result.uses:
            self.assertEqual(result.uses[use.twin].twin,use.id)
            self.assertEqual(result.uses[use.twin].orientation,-use.orientation)

    def test_original_boundary_parameter_direction_all_rectangles(self):
        directions=set()
        for shape,ids in (("box",range(1,7)),("plate",range(3,7))):
            for fid in ids:
                with self.subTest(shape=shape,face=fid):
                    result,counts = self.arranged(self.face(fid,origin=(1.,-2.,.125),scale=.5,shape=shape),
                                                  u_cuts=(F(3,4),F(1,4)),v_cuts=(F(1,3),))
                    self.assertEqual(counts["cells"],6)
                    for edge in result.edges:
                        if edge.source[0] == "boundary":
                            t0,t1=edge.source[3:]
                            directions.add(1 if t1>t0 else -1)
                            self.assertNotEqual(t0,t1)
        self.assertEqual(directions,{-1,1})

    def test_circle_source_order_side_and_curve_readers(self):
        face=self.face()
        for reverse in (False,True):
            with self.subTest(reverse=reverse):
                section=self.circle(face,reverse=reverse)
                result,counts=self.arranged(face,(section,))
                self.assertEqual((counts["vertices"],counts["edges"],counts["uses"],counts["loops"],counts["regions"]),(5,5,10,4,3))
                self.assertEqual(tuple(r.kind for r in result.regions),("outside","cell","disk"))
                self.assertEqual((result.regions[1].area.rational,result.regions[1].area.pi),(F(1),-F(1,36)))
                self.assertEqual((result.regions[2].area.rational,result.regions[2].area.pi),(F(0),F(1,36)))
                edge=next(e for e in result.edges if e.kind == "circle")
                self.assertEqual(edge.start,edge.end)
                self.assertEqual(edge.piece_ids,section.components[0].piece_ids)
                self.assertCountEqual(edge.event_ids,tuple(e.id for e in section.events))
                expected=(((F(1,4),F(0),F(0)),(F(2,3),F(1,2))),
                          ((F(3,20),F(1,5),F(0)),(F(3,5),F(19,30))),
                          ((F(0),F(1,4),F(0)),(F(1,2),F(2,3))),
                          ((-F(1,4),F(0),F(0)),(F(1,3),F(1,2))),
                          ((F(0),-F(1,4),F(0)),(F(1,2),F(1,3))),
                          ((F(1,4),F(0),F(0)),(F(2,3),F(1,2))))
                for t,(world,uv) in zip((F(0),F(1,8),F(1,4),F(1,2),F(3,4),F(1)),expected):
                    self.assertEqual(result.edge_point(edge.id,t),world)
                    self.assertEqual(result.edge_uv(edge.id,t),uv)
                self.locations(result,((F(1,2),F(1,2)),(F(2,3),F(1,2)),(F(1,4),F(1,2))),((2,),(1,2),(1,)))

    def test_multi_hole_cell_complete_region_bijection(self):
        face=self.face()
        sections=(self.circle(face,(-.25,0.),.25),self.circle(face,(.25,0.),.25))
        result,counts=self.arranged(face,sections)
        self.assertEqual((counts["cells"],counts["disks"],counts["holes"],counts["regions"]),(1,2,2,4))
        self.assertEqual(len(result.regions[1].holes),2)
        self.assertEqual((result.regions[1].area.rational,result.regions[1].area.pi),(F(1),-F(1,288)))
        self.locations(result,((F(1,3),F(1,2)),(F(2,3),F(1,2)),(F(1,2),F(1,2))),((2,),(3,),(1,)))
        for section,uv in ((0,(F(3,8),F(1,2))),(1,(F(17,24),F(1,2)))):
            self.assertEqual(result.locate_uv(*uv),(1,2+section))

    def test_mixed_cells_circle_holes_areas_and_sources(self):
        face=self.face()
        sections=tuple(self.circle(face,center,.25,reverse=index%2==1)
                       for index,center in enumerate(((-.5,-.5),(-.25,-.25),(.375,.375))))
        result,counts=self.arranged(face,sections,u_cuts=(F(1,2),),v_cuts=(F(1,2),))
        self.assertEqual((counts["vertices"],counts["edges"],counts["uses"],counts["loops"],counts["regions"]),(12,15,30,11,8))
        self.assertEqual(tuple(len(r.holes) for r in result.regions[1:5]),(2,0,0,1))
        self.assertEqual(tuple(r.cell for r in result.regions[5:]),((0,0),(0,0),(1,1)))
        self.assertEqual(tuple(r.section for r in result.regions[5:]),(0,1,2))
        self.assertEqual(sum(r.area.rational for r in result.regions[1:]),F(1))
        self.assertEqual(sum(r.area.pi for r in result.regions[1:]),F(0))
        self.locations(result,tuple(r.witness_uv for r in result.regions),tuple((r.id,) for r in result.regions))

    def test_source_identity_bottom_frames_and_input_permutations(self):
        for fid in (1,2):
            face=self.face(fid,origin=(16.,-16.,.125),scale=4.)
            sections=(self.circle(face,(16.,-16.),.25,reverse=True),)
            result,counts=self.arranged(face,sections,u_cuts=(F(3,4),F(1,4)),v_cuts=(F(3,4),F(1,4)))
            self.assertEqual(counts["cells"],9)
            self.assertEqual(result.regions[-1].cell,(1,1))
            self.assertEqual(result.regions[-1].witness_uv,(F(1,2),F(1,2)))
            self.assertEqual(result.regions[-1].area.pi,F(1,9216))
        face=self.face()
        a,b=self.circle(face,(-.25,0.),.25),self.circle(face,(.25,0.),.25,reverse=True)
        first,_=self.arranged(face,(a,b));second,_=self.arranged(face,(b,a))
        def key(section):
            return tuple((ref.snapshot.body_id,ref.snapshot.revision,ref.face_id) for ref in (section.left,section.right))
        first_labels={key(first.sections[r.section]):r.witness_uv for r in first.regions if r.kind=="disk"}
        second_labels={key(second.sections[r.section]):r.witness_uv for r in second.regions if r.kind=="disk"}
        self.assertEqual(first_labels,second_labels)

    def test_exact_cut_separation_and_limits(self):
        from brepgpu.arrangements import arrange_face,UnsupportedArrangement
        tiny=F.from_float(math.nextafter(0.,1.))
        result,counts=self.arranged(self.face(),u_cuts=(2*tiny,tiny))
        self.assertEqual((counts["vertices"],counts["cells"]),(8,3))
        self.assertTrue(all(r.area.rational>0 for r in result.regions[1:]))
        self.locations(result,((tiny,F(1,2)),(2*tiny,F(1,2)),(tiny,0)),((1,2),(2,3),(0,1,2)))
        face=self.face();section=self.circle(face)
        below=F.from_float(float(F(1,3)))
        self.assertLess(below,F(1,3))
        self.arranged(face,(section,),u_cuts=(below,))
        for cut in (F(1,3),F.from_float(math.nextafter(float(F(1,3)),math.inf))):
            with self.assertRaises(UnsupportedArrangement):arrange_face(face,(section,),u_cuts=(cut,))
        large,metrics=self.arranged(self.face(),u_cuts=tuple(F(i,65) for i in range(1,65)))
        self.assertEqual(metrics["cells"],65)

    def test_unsupported_touching_overlapping_and_clipped_sections(self):
        from brepgpu.arrangements import arrange_face,UnsupportedArrangement
        from brepgpu.intersections import intersect_faces,FaceRef
        from brepgpu.body import make_body,Frame
        face=self.face()
        tangent=(self.circle(face,(-.0625,0.),.25),self.circle(face,(.0625,0.),.25))
        overlap=(self.circle(face,(-.03125,0.),.25),self.circle(face,(.03125,0.),.25))
        duplicate=(self.circle(face,(0.,0.),.25),self.circle(face,(0.,0.),.25))
        nested=(self.circle(face,(0.,0.),.5),self.circle(face,(0.,0.),.25))
        for sections in (tangent,overlap,duplicate,nested,(self.circle(face,(.625,0.),1.),),(self.circle(face,(4.,0.),1.),)):
            with self.assertRaises(UnsupportedArrangement):arrange_face(face,sections)
        circle=self.circle(face)
        with self.assertRaises(UnsupportedArrangement):arrange_face(face,(circle,),u_cuts=(F(2,3),))
        with self.assertRaises(UnsupportedArrangement):arrange_face(self.face(),(circle,))
        with self.assertRaises(UnsupportedArrangement):arrange_face(self.face(2), (circle,))
        vertical=self.face(3)
        wall=FaceRef(make_body("cylinder",Frame((0.,-.75,-.5))),3)
        section=intersect_faces(vertical,wall)
        with self.assertRaises(UnsupportedArrangement):arrange_face(vertical,(section,))
        for unsupported in (self.face(shape="cylinder"),self.face(shape="plate"),self.face(7,shape="plate")):
            with self.assertRaises(UnsupportedArrangement):arrange_face(unsupported)

    def test_invalid_iterators_readers_and_immutable_results(self):
        from brepgpu.arrangements import arrange_face,Arrangement
        face=self.face();section=self.circle(face)
        for cuts in ((0,), (1,), (-1,), (F(1,2),F(1,2)), (True,), (float("nan"),), (float("inf"),)):
            with self.assertRaises((TypeError,ValueError)):arrange_face(face,u_cuts=cuts)
        for field,limit,value in (("u_cuts",64,F(1,3)),("sections",256,section)):
            visited=[]
            def endless():
                while True:
                    visited.append(len(visited))
                    yield value
            with self.assertRaises(ValueError):arrange_face(face,**{field:endless()})
            self.assertEqual(len(visited),limit+1)
        def failed():
            yield F(1,3)
            raise RuntimeError("arrangement source iterator failure")
        with self.assertRaisesRegex(RuntimeError,"arrangement source iterator failure"):
            arrange_face(face,u_cuts=failed())
        sections=[section];cuts=[F(1,4)]
        # The cut is left of the circle, retaining a valid full disk in cell1.
        result=arrange_face(face,sections,u_cuts=cuts)
        sections.clear();cuts.clear()
        self.assertIs(result.sections[0],section)
        self.assertEqual(result.u_cuts,(F(1,4),))
        from brepgpu.arrangement_oracles import qualify_arrangement
        qualify_arrangement(result)
        for record,field in ((result,"vertices"),(result.vertices[0],"uv"),(result.edges[0],"source"),
                             (result.uses[0],"twin"),(result.loops[0],"uses"),(result.regions[1],"holes"),
                             (result.regions[1].area,"pi")):
            with self.assertRaises((FrozenInstanceError,AttributeError,TypeError)):setattr(record,field,None)
        with self.assertRaises(TypeError):Arrangement()
        for reader in (result.edge_uv,result.edge_point):
            for eid in (-1,len(result.edges),True,.5):
                with self.assertRaises((ValueError,TypeError,IndexError)):reader(eid,F(1,2))
            for parameter in (-1,2,True,float("nan"),float("inf")):
                with self.assertRaises((ValueError,TypeError,IndexError)):reader(0,parameter)
        for value in (True,float("nan"),float("inf")):
            with self.assertRaises((ValueError,TypeError)):result.locate_uv(value,F(1,2))
        with self.assertRaises(TypeError):arrange_face(None)
        with self.assertRaises(TypeError):arrange_face(face,(None,))


if __name__ == "__main__":unittest.main()
