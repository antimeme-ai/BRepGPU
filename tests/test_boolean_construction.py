"""Whole CPU cut construction/history specifications; fleet only."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
import math
from unittest.mock import patch
import unittest

from brepgpu.body import Frame, make_body, REJECTED, INTERIOR, BOUNDARY, INVALID


def operands(scale=1.,origin=(0.,0.,0.)):
    return make_body('box',Frame(origin,scale)),make_body('cylinder',Frame(origin,2*scale))


class BooleanConstructionTests(unittest.TestCase):
    def test_entry_actual_sections_and_new_hole_not_canonical_plate(self):
        left,right=operands()
        self.assertEqual(left.classify_point((0,0,0)).label,'interior')
        self.assertEqual(right.classify_point((0,0,0)).label,'interior')
        from brepgpu.boolean import difference
        result=difference(left,right); solid=result.solid
        self.assertIs(result.left,left);self.assertIs(result.right,right)
        self.assertEqual((solid.radius,solid.wall_band),(F(1,2),(F(1,4),F(3,4))))
        self.assertNotIn(solid.body_id,(left.body_id,right.body_id))
        self.assertFalse(hasattr(solid,'shape'))
        self.assertEqual(tuple(map(len,(solid.vertices,solid.edges,solid.faces,solid.loops,solid.coedges,solid.charts))),
                         (10,15,7,9,30,10))
        self.assertEqual(len(result.sections),2);self.assertEqual(len(result.arrangements),2)
        self.assertEqual(len(result.selections),13)
        for i,(section,arrangement) in enumerate(zip(result.sections,result.arrangements)):
            self.assertIs(section.left.snapshot,left);self.assertIs(section.right.snapshot,right)
            self.assertIs(arrangement.sections[0],section)
            self.assertEqual(section.left.face.name,'bottom' if i==0 else 'top')
            self.assertEqual((len(section.components),len(section.pieces),len(section.events)),(1,4,4))
        self.assertEqual(solid.classify_point((F(3,8),0,0)).label,'exterior')
        self.assertEqual(make_body('plate').classify_point((F(3,8),0,0)).label,'interior')
        self.assertEqual((solid.volume.rational,solid.volume.pi),(F(9,4),F(-1,4)))

    def test_source_carrier_bits_original_frames_and_new_lineage(self):
        from brepgpu.boolean import difference
        left,right=operands(.5,(2.,-1.,.25)); solid=difference(left,right).solid
        original=left.patches+tuple(right.patches[i] for i in right.faces[2].chart_slots)
        for slot,(actual,source) in enumerate(zip(solid.patches,original)):
            self.assertEqual((actual.controls,actual.uv,actual.origin,actual.scale),
                             (source.controls,source.uv,source.origin,source.scale))
            self.assertEqual((actual.lineage.body_id,actual.lineage.entity_id,actual.lineage.chart_id),
                             (solid.body_id,solid.charts[slot].face_slot+1,slot+1))
        self.assertEqual(solid.patches[-1].scale,2*solid.frame.scale)
        self.assertEqual(tuple(v.position for v in solid.vertices[:8]),tuple(v.position for v in left.vertices))

    def test_exact_material_and_trim_queries(self):
        from brepgpu.boolean import difference
        solid=difference(*operands()).solid
        for p,label,faces in (((0,0,0),'exterior',()),((F(5,8),0,0),'interior',()),
                              ((F(1,2),0,0),'boundary',(7,)),
                              ((F(1,2),0,F(1,2)),'boundary',(1,7)),
                              ((F(3,4),F(3,4),F(1,2)),'boundary',(1,4,5)),
                              ((0,0,F(1,2)),'exterior',())):
            answer=solid.classify_point(p)
            self.assertIs(answer.solid,solid);self.assertEqual((answer.label,answer.boundary_faces),(label,faces))
            self.assertTrue(all(type(x) is F for x in answer.point))
        self.assertEqual(solid.classify(0,F(5,6),F(1,2)),BOUNDARY)
        self.assertEqual(solid.classify(0,.5,.5),REJECTED)
        for u in (math.nextafter(5/6,-math.inf),5/6):
            expected=REJECTED if (F(u)-F(1,2))**2<F(1,9) else INTERIOR
            self.assertEqual(solid.classify(0,u,.5),expected)
        self.assertEqual(solid.classify(7,0,.5),INTERIOR)
        self.assertEqual(solid.classify(6,0,.5),BOUNDARY)
        self.assertEqual(solid.classify(9,1,.5),BOUNDARY)
        self.assertEqual(solid.classify(7,.25,.25),BOUNDARY)
        self.assertEqual(solid.classify(7,.25,.125),REJECTED)
        self.assertEqual(solid.classify(0,math.nan,.5),INVALID)
        for p in ((True,0,0),(math.inf,0,0),('0',0,0)):
            with self.assertRaises((TypeError,ValueError)):solid.classify_point(p)
        with self.assertRaises(TypeError):solid.classify(0,True,.5)

    def test_exact_ring_seam_maps_and_opposite_uses(self):
        from brepgpu.boolean import difference
        solid=difference(*operands()).solid
        self.assertEqual(solid.edge_point(13,F(1,8)),(F(3,10),F(2,5),F(-1,2)))
        self.assertEqual(solid.edge_point(13,0),solid.edge_point(13,1))
        for edge in solid.edges:
            uses=tuple(c for c in solid.coedges if c.edge_slot==edge.id-1)
            self.assertEqual(len(uses),2);self.assertEqual(sum(c.orientation for c in uses),0)
            for c in uses:
                for t in (F(0),F(1,8),F(1,2),F(1)):
                    uv=solid.coedge_uv(c.id,t)
                    self.assertTrue(all(type(x) is F for x in uv))
        seam=tuple(c for c in solid.coedges if c.edge_slot==14)
        self.assertTrue(all(c.face_slot==6 for c in seam))
        self.assertEqual({solid.coedge_uv(c.id,F(1,2)) for c in seam},{(F(0),F(1,2)),(F(4),F(1,2))})
        self.assertEqual(solid.edge_point(15,F(1,3)),(F(1,2),F(0),F(-1,6)))

    def test_complete_typed_forward_reverse_history(self):
        from brepgpu.boolean import difference,EntityKey
        result=difference(*operands());h=result.history
        self.assertEqual((len(h.records),len(h.origins)),(85,81))
        self.assertEqual({s:sum(r.status==s for r in h.records) for s in ('retained','modified','deleted')},
                         {'retained':58,'modified':13,'deleted':14})
        self.assertEqual(sum(o.relation=='generated' for o in h.origins),10)
        self.assertEqual(len({r.source for r in h.records}),85)
        self.assertEqual(len({o.target for o in h.origins}),81)
        for r in h.records:
            self.assertIs(h.from_input(r.source),r)
            for target in r.images:
                o=h.origin(target);self.assertIn(r.source,o.parents)
                self.assertEqual((o.relation,target.kind),(r.status,r.source.kind))
            for target in r.generated:
                o=h.origin(target);self.assertEqual(o.relation,'generated');self.assertIn(r.source,o.parents)
        for o in h.origins:
            self.assertIs(h.origin(o.target),o)
            self.assertTrue(o.parents)
            for parent in o.parents:
                self.assertIn(o.target,h.from_input(parent).generated if o.relation=='generated' else h.from_input(parent).images)
        seam=h.origin(EntityKey(result.solid.body_id,1,'edge',15))
        self.assertEqual((seam.relation,seam.source_range,seam.orientation),('modified',(F(1,4),F(3,4)),1))
        with self.assertRaises(KeyError):h.origin(EntityKey(result.left.body_id,1,'edge',1))
        with self.assertRaises((TypeError,ValueError)):EntityKey(True,1,'face',1)

    def test_all_scales_translations_and_topology_adapter(self):
        from brepgpu.boolean import difference
        for scale in (.25,.5,1.,2.):
            left,right=operands(scale,(1.25,-2.,3.));result=difference(left,right);s=result.solid
            self.assertEqual(s.edge_point(15,0),(F(1.25)+F(scale)/2,F(-2),F(3)-F(scale)/2))
            self.assertEqual((s.volume.rational,s.volume.pi),(F(9,4)*F(scale)**3,F(-1,4)*F(scale)**3))
            top=s.topology_input()
            self.assertEqual((top.edge_count,top.face_count,top.coedge_count),(15,7,30))
            self.assertEqual(top.snapshots,())
            self.assertTrue(all(x.body_id==s.body_id and x.revision==1 for table in (top.edge_sources,top.face_sources,top.coedge_sources) for x in table))

    def test_unsupported_family_and_immutability(self):
        from brepgpu.boolean import difference,UnsupportedBoolean,CutSolid,CutResult
        left,right=operands();before=(left.packed_tables,right.packed_tables)
        for a,b in ((right,left),(left,make_body('cylinder')),
                    (left,make_body('cylinder',Frame((0,0,.125),2))),
                    (left,make_body('cylinder',Frame((.125,0,0),2))),
                    (make_body('plate'),right)):
            with self.assertRaises(UnsupportedBoolean):difference(a,b)
        for a,b in ((None,right),(left,object())):
            with self.assertRaises(TypeError):difference(a,b)
        self.assertEqual(before,(left.packed_tables,right.packed_tables))
        result=difference(left,right)
        with self.assertRaises((FrozenInstanceError,AttributeError)):result.solid.radius=F(1,4)
        for factory in (CutSolid,CutResult):
            with self.assertRaises(TypeError):factory()

    def test_late_candidate_failure_preserves_inputs_and_prior_result(self):
        from brepgpu import boolean
        left,right=operands();prior=boolean.difference(left,right)
        before=(left.packed_tables,right.packed_tables,prior.solid.vertices,prior.history.origins)
        called=[]
        def fail(solid,history,*args):
            called.append((len(solid.coedges),len(history.origins)))
            raise RuntimeError('late candidate validation failure')
        with patch.object(boolean,'_validate_candidate',side_effect=fail):
            with self.assertRaisesRegex(RuntimeError,'late candidate'):boolean.difference(left,right)
        self.assertEqual(called,[(30,81)])
        self.assertEqual(before,(left.packed_tables,right.packed_tables,prior.solid.vertices,prior.history.origins))
        self.assertEqual(prior.solid.classify_point((.5,0,0)).boundary_faces,(7,))
        self.assertNotEqual(boolean.difference(left,right).solid.body_id,prior.solid.body_id)


if __name__=='__main__':unittest.main()
