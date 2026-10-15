"""Exact whole profile/solid/history specifications; product runs fleet only."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
from unittest.mock import patch
import math
import unittest

from brepgpu.body import Frame, make_body, INVALID, REJECTED, BOUNDARY, INTERIOR


TRIANGLE=((F(-1,2),F(-1,2)),(F(3,4),F(-1,4)),(F(-1,4),F(3,4)))


def lattice32():
    quarter=((1,0),(3,1),(2,1),(3,2),(1,1),(2,3),(1,2),(1,3))
    directions=[]
    for q in range(4):
        for a,b in quarter:
            directions.append(((a,b),(-b,a),(-a,-b),(b,-a))[q])
    points=[];x=y=0
    for dx,dy in directions:
        points.append((x,y));x+=8*dx;y+=8*dy
    assert (x,y)==(0,0)
    cx=F(min(p[0] for p in points)+max(p[0] for p in points),2)
    cy=F(min(p[1] for p in points)+max(p[1] for p in points),2)
    return tuple(((x-cx)/256,(y-cy)/256) for x,y in points)


class ExtrusionConstructionTests(unittest.TestCase):
    def test_entry_healthy_original_body_then_actual_profile_feature(self):
        self.assertEqual(make_body('box').classify_point((0,0,0)).label,'interior')
        from brepgpu.extrusion import make_profile,extrude
        p=make_profile(TRIANGLE);r=extrude(p,F(3,4));s=r.solid
        self.assertIs(r.profile,p);self.assertIs(s.profile,p)
        self.assertEqual((r.height,s.height),(F(3,4),F(3,4)))
        self.assertNotEqual(p.body_id,s.body_id)
        self.assertEqual(tuple(map(len,(s.vertices,s.edges,s.coedges,s.loops,s.faces,s.charts))),(6,9,18,5,5,5))
        self.assertFalse(hasattr(s,'shape'));self.assertFalse(hasattr(s,'packed_tables'))
        self.assertEqual(p.area,F(3,4));self.assertEqual(s.volume,F(9,16))

    def test_profile_exact_source_carrier_and_grid_bits(self):
        from brepgpu.extrusion import make_profile,extrude
        frame=Frame((1.25,-2.,3.),.5);p=make_profile(TRIANGLE,frame)
        self.assertIs(p.frame,frame)
        self.assertEqual(tuple(v.position for v in p.vertices),tuple((*a,F(0)) for a in TRIANGLE))
        self.assertEqual(p.patches[0].controls,(-1.,-1.,0.,1.,-1.,1.,0.,1.,1.,-1.,0.,1.,1.,1.,0.,1.))
        self.assertEqual(p.classify(F(1,4),F(1,4)),BOUNDARY)
        self.assertEqual(p.classify(0,0),REJECTED)
        self.assertEqual(p.classify(F(1,2),F(1,2)),INTERIOR)
        self.assertEqual(p.classify(math.inf,.5),INVALID)
        s=extrude(p,F(3,4)).solid
        for slot,cp in enumerate(s.patches):
            self.assertEqual((cp.origin,cp.scale),(frame.origin,frame.scale))
            self.assertEqual((cp.lineage.body_id,cp.lineage.entity_id,cp.lineage.chart_id),(s.body_id,slot+1,slot+1))
            self.assertTrue(all(abs(v)<=1 for v in cp.controls))
        self.assertEqual(s.vertices[0].position,(*TRIANGLE[0],F(-3,8)))
        self.assertEqual(s.patches[2].controls,tuple(float(x) for a in TRIANGLE[:2] for z in (F(-3,8),F(3,8)) for x in (*a,z,F(1))))

    def test_material_and_full_supporting_boundary(self):
        from brepgpu.extrusion import make_profile,extrude
        s=extrude(make_profile(TRIANGLE),1).solid
        cases=(((0,0,0),'interior',()),((0,0,F(1,2)),'boundary',(2,)),
               ((F(-1,2),F(-1,2),0),'boundary',(3,5)),
               ((F(-1,2),F(-1,2),F(-1,2)),'boundary',(1,3,5)),
               ((F(3,4),F(3,4),0),'exterior',()),((0,0,1),'exterior',()))
        for point,label,faces in cases:
            a=s.classify_point(point)
            self.assertIs(a.solid,s);self.assertEqual((a.label,a.boundary_faces),(label,faces))
            self.assertTrue(all(type(x) is F for x in a.point))
        self.assertEqual(s.classify(0,F(1,4),F(1,4)),BOUNDARY)
        self.assertEqual(s.classify(0,0,0),REJECTED)
        self.assertEqual(s.classify(2,F(1,3),F(1,3)),INTERIOR)
        self.assertEqual(s.classify(2,0,F(1,3)),BOUNDARY)
        self.assertEqual(s.classify(2,-1,.5),INVALID)
        with self.assertRaises(TypeError):s.classify(True,.5,.5)
        with self.assertRaises(TypeError):s.classify_point((True,0,0))

    def test_complete_exact_edge_use_maps_and_oriented_words(self):
        from brepgpu.extrusion import make_profile,extrude
        s=extrude(make_profile(TRIANGLE),1).solid;n=3
        self.assertEqual(tuple(s.coedges[c].edge_slot for c in s.loops[0].coedge_slots),(2,1,0))
        self.assertEqual(tuple(s.coedges[c].orientation for c in s.loops[0].coedge_slots),(-1,-1,-1))
        for edge in s.edges:
            uses=tuple(c for c in s.coedges if c.edge_slot==edge.id-1)
            self.assertEqual(len(uses),2);self.assertEqual(sum(c.orientation for c in uses),0)
            for c in uses:
                for t in (F(0),F(1,3),F(1)):
                    u,v=s.coedge_uv(c.id,t);face=c.face_slot
                    if face<2:local=(2*u-1,2*v-1,F(-1,2) if face==0 else F(1,2))
                    else:
                        a,b=TRIANGLE[face-2],TRIANGLE[(face-1)%n]
                        local=((1-u)*a[0]+u*b[0],(1-u)*a[1]+u*b[1],v-F(1,2))
                    self.assertEqual(s.edge_point(edge.id,t),local)
        for loop in s.loops:
            ends=[]
            for cs in loop.coedge_slots:
                c=s.coedges[cs];e=s.edges[c.edge_slot]
                ends.append((e.start,e.end) if c.orientation==1 else (e.end,e.start))
            self.assertTrue(all(a[1]==b[0] for a,b in zip(ends,ends[1:]+ends[:1])))

    def test_complete_affine_origins_and_reciprocal_history(self):
        from brepgpu.extrusion import make_profile,extrude,EntityKey
        p=make_profile(TRIANGLE);r=extrude(p,1);h=r.history
        self.assertEqual((len(h.records),len(h.origins)),(12,48))
        self.assertEqual(sum(o.relation=='modified' for o in h.origins),24)
        self.assertEqual(sum(o.relation=='generated' for o in h.origins),24)
        for record in h.records:
            self.assertEqual((record.status,len(record.images)),('modified',2))
            for target in record.images+record.generated:
                o=h.origin(target);self.assertIn(record.source,o.parents)
                self.assertIn(target,record.generated if o.relation=='generated' else record.images)
        for o in h.origins:
            for key in o.parents:
                self.assertIn(o.target,h.from_input(key).generated if o.relation=='generated' else h.from_input(key).images)
        edge=h.origin(EntityKey(r.solid.body_id,1,'edge',7))
        self.assertEqual((edge.role,edge.axial,edge.orientation),('swept_vertex',(F(-1,2),F(1),F(0)),1))
        lower=h.origin(EntityKey(r.solid.body_id,1,'coedge',1))
        self.assertEqual((lower.parents,lower.orientation,lower.source_t),((EntityKey(p.body_id,1,'coedge',3),),-1,(F(0),F(1),F(0))))
        other=make_profile(TRIANGLE)
        with self.assertRaises(KeyError):h.from_input(EntityKey(other.body_id,1,'edge',1))

    def test_actual_max32_and_positive_turn_winding_star(self):
        from brepgpu.extrusion import make_profile,extrude,UnsupportedExtrusion
        points=lattice32();self.assertEqual(len(points),32)
        for i,a in enumerate(points):
            b=points[(i+1)%32]
            for j,c in enumerate(points):
                if j not in (i,(i+1)%32):self.assertGreater((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]),0)
        s=extrude(make_profile(points),F(1,4)).solid
        self.assertEqual((len(s.vertices),len(s.edges),len(s.coedges),len(s.faces)),(64,96,192,34))
        pent=((0,128),(-128,32),(-80,-128),(80,-128),(128,32))
        star=tuple((F(pent[i][0],256),F(pent[i][1],256)) for i in (0,2,4,1,3))
        for i,a in enumerate(star):
            b,c=star[(i+1)%5],star[(i+2)%5]
            self.assertGreater((b[0]-a[0])*(c[1]-b[1])-(b[1]-a[1])*(c[0]-b[0]),0)
        with self.assertRaises(UnsupportedExtrusion):make_profile(star)

    def test_admission_limits_and_immutable_publication(self):
        from brepgpu.extrusion import make_profile,extrude,UnsupportedExtrusion,PlanarProfile,ExtrudedSolid,ExtrusionResult
        for points in (TRIANGLE[::-1],TRIANGLE+TRIANGLE[:1],TRIANGLE[:2],((0,0),(1,0),(0,1)),((0,0),(F(1,257),0),(0,F(1,2))),((0,0),(0,0),(F(1,2),F(1,2))),((0,0),(F(1,4),0),(F(1,2),0)),[(0,0)]*33):
            with self.assertRaises((ValueError,UnsupportedExtrusion)):make_profile(points)
        for points in (((True,0),*TRIANGLE[1:]),((math.inf,0),*TRIANGLE[1:])):
            with self.assertRaises((TypeError,ValueError)):make_profile(points)
        p=make_profile(TRIANGLE)
        for height in (0,-1,3,F(1,257),F(1,8),math.nan):
            with self.assertRaises(ValueError):extrude(p,height)
        with self.assertRaises(TypeError):extrude(p,True)
        with self.assertRaises(TypeError):make_profile(TRIANGLE,object())
        with self.assertRaises(TypeError):extrude(make_body('box'),1)
        for cls in (PlanarProfile,ExtrudedSolid,ExtrusionResult):
            with self.assertRaises(TypeError):cls()
        with self.assertRaises((FrozenInstanceError,AttributeError)):p.area=F(0)

    def test_frames_topology_and_transaction_failure(self):
        from brepgpu import extrusion as m
        for scale in (.25,.5,1.,2.,4.):
            frame=Frame((1.25,-2.,3.),scale);p=m.make_profile(TRIANGLE,frame);r=m.extrude(p,1);s=r.solid
            self.assertEqual(s.edge_point(1,F(1,3)),tuple(F(o)+F(scale)*x for o,x in zip(frame.origin,(F(-1,12),F(-5,12),F(-1,2)))))
            self.assertEqual(s.volume,F(3,4)*F(scale)**3)
            t=s.topology_input();self.assertEqual((t.edge_count,t.face_count,t.coedge_count),(9,5,18));self.assertEqual(t.snapshots,())
        before=(p.vertices,p.patches,r.solid.coedges,r.history.origins)
        with patch.object(m,'_validate_candidate',side_effect=RuntimeError('late exact failure')):
            with self.assertRaisesRegex(RuntimeError,'late exact failure'):m.extrude(p,1)
        self.assertEqual(before,(p.vertices,p.patches,r.solid.coedges,r.history.origins))
        self.assertEqual(m.extrude(p,1).solid.volume,r.solid.volume)


if __name__=='__main__':unittest.main()
