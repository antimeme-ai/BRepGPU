"""Direct exact revolution construction/history contract; fleet execution only."""
from collections import Counter
from copy import copy
from dataclasses import FrozenInstanceError,replace
from fractions import Fraction as F
import math
import unittest
from unittest.mock import patch

from brepgpu.body import Frame,make_body,INVALID,BOUNDARY,INTERIOR
from brepgpu.extrusion import make_profile


TRIANGLE=((F(1,4),F(-1,2)),(F(3,4),F(-1,4)),(F(1,2),F(3,4)))
RECTANGLE=((F(1,4),F(-1,2)),(F(3,4),F(-1,2)),(F(3,4),F(1,2)),(F(1,4),F(1,2)))


def many32():
    base=((1,0),(3,1),(2,1),(3,2),(1,1),(2,3),(1,2),(1,3))
    directions=tuple(((a,b),(-b,a),(-a,-b),(b,-a))[q] for q in range(4) for a,b in base)
    points=[];x=y=0
    for dx,dy in directions:points.append((x,y));x+=4*dx;y+=4*dy
    assert (x,y)==(0,0)
    cx=F(min(a for a,b in points)+max(a for a,b in points),2)
    cy=F(min(b for a,b in points)+max(b for a,b in points),2)
    return tuple(((a-cx+128)/256,(b-cy)/256) for a,b in points)


def circle(t):
    q,s=(3,F(1)) if t==1 else (int(4*t),4*t-int(4*t))
    c,d=(1-s*s)/(1+s*s),2*s/(1+s*s)
    return ((c,d),(-d,c),(-c,-d),(d,-c))[q]


class RevolutionConstructionTests(unittest.TestCase):
    def test_entry_healthy_original_body_then_actual_revolution(self):
        self.assertEqual(make_body('box').classify_point((0,0,0)).label,'interior')
        p=make_profile(TRIANGLE)
        from brepgpu.revolution import revolve
        r=revolve(p);s=r.solid
        self.assertIs(r.profile,p);self.assertIs(s.profile,p);self.assertIs(s.frame,p.frame)
        self.assertNotEqual(s.body_id,p.body_id)
        self.assertEqual(tuple(map(len,(s.vertices,s.edges,s.coedges,s.loops,s.faces,s.charts))),(3,6,12,3,3,12))
        self.assertEqual((s.volume.rational,s.volume.pi),(F(0),F(9,32)))
        self.assertFalse(hasattr(s,'shape'));self.assertFalse(hasattr(s,'packed_tables'))

    def test_preserved_source_embedding_and_exact_quarter_controls(self):
        from brepgpu.revolution import revolve
        frame=Frame((1.25,-2.,3.),.5);p=make_profile(TRIANGLE,frame);s=revolve(p).solid
        self.assertEqual(s.vertices,p.vertices)
        for i,edge in enumerate(p.edges):
            self.assertEqual(s.curves[s.edges[3+i].curve_slot],p.curves[edge.curve_slot])
        for i in range(3):
            a,b=TRIANGLE[i],TRIANGLE[(i+1)%3]
            for q in range(4):
                cp=s.patches[4*i+q];controls=[]
                for A,B,W in ((1,0,1),(1,1,1),(0,2,2)):
                    A,B=((A,B),(-B,A),(-A,-B),(B,-A))[q]
                    for radius,axial in (a,b):controls.extend((float(radius*A),float(axial*W),float(-radius*B),float(W)))
                self.assertEqual(cp.controls,tuple(controls))
                self.assertEqual((cp.degree_u,cp.degree_v,cp.uv,cp.origin,cp.scale),(2,1,(q,q+1,0.,1.),frame.origin,frame.scale))
                self.assertEqual((cp.lineage.body_id,cp.lineage.entity_id,cp.lineage.chart_id),(s.body_id,i+1,4*i+q+1))
                self.assertEqual(s.charts[4*i+q].face_slot,i)
        self.assertEqual(s.curves[s.edges[0].curve_slot].kind,'circle_y')
        self.assertEqual(s.curves[s.edges[0].curve_slot].data,(F(1,4),F(-1,2)))

    def test_rational_ring_line_and_all_incident_uv_readers(self):
        from brepgpu.revolution import revolve
        s=revolve(make_profile(TRIANGLE)).solid
        for i,(radius,axial) in enumerate(TRIANGLE):
            for t in (F(0),F(1,7),F(1,4),F(1,2),F(3,4),F(1)):
                c,d=circle(t)
                self.assertEqual(s.edge_point(i+1,t),(radius*c,axial,-radius*d))
                a,b=TRIANGLE[i],TRIANGLE[(i+1)%3]
                self.assertEqual(s.edge_point(4+i,t),((1-t)*a[0]+t*b[0],(1-t)*a[1]+t*b[1],F(0)))
        for face,loop in enumerate(s.loops):
            uses=tuple(s.coedges[cs] for cs in loop.coedge_slots)
            self.assertEqual(tuple(c.edge_slot for c in uses),(face,3+face,(face+1)%3,3+face))
            self.assertEqual(tuple(c.orientation for c in uses),(1,1,-1,-1))
            for t in (F(0),F(1,7),F(1)):
                self.assertEqual(tuple(s.coedge_uv(c.id,t) for c in uses),((4*t,F(0)),(F(4),t),(4*t,F(1)),(F(0),t)))

    def test_closed_topology_same_face_seams_and_four_port_links(self):
        from brepgpu.revolution import revolve
        s=revolve(make_profile(TRIANGLE)).solid
        for es,e in enumerate(s.edges):
            uses=[c for c in s.coedges if c.edge_slot==es]
            self.assertEqual(len(uses),2);self.assertEqual(sum(c.orientation for c in uses),0)
            self.assertEqual(len({c.face_slot for c in uses}),2 if es<3 else 1)
            if es<3:self.assertEqual(e.start,e.end)
        for vertex in range(3):
            germs={(es,end) for es,e in enumerate(s.edges) for end,v in enumerate((e.start,e.end)) if v==vertex}
            self.assertEqual(len(germs),4)
            link={g:[] for g in germs}
            for loop in s.loops:
                word=loop.coedge_slots
                for prev,cur in zip(word[-1:]+word[:-1],word):
                    a,b=s.coedges[prev],s.coedges[cur];ea,eb=s.edges[a.edge_slot],s.edges[b.edge_slot]
                    end=1 if a.orientation==1 else 0;start=0 if b.orientation==1 else 1
                    if (ea.start,ea.end)[end]==vertex:
                        self.assertEqual((eb.start,eb.end)[start],vertex)
                        x,y=(a.edge_slot,end),(b.edge_slot,start);link[x].append(y);link[y].append(x)
            self.assertTrue(all(len(v)==2 for v in link.values()))
            seen=set();todo=[next(iter(germs))]
            while todo:
                g=todo.pop()
                if g not in seen:seen.add(g);todo.extend(link[g])
            self.assertEqual(seen,germs)
        data=s.topology_input();self.assertEqual((data.edge_count,data.face_count,data.coedge_count),(6,3,12));self.assertEqual(data.snapshots,())

    def test_exact_material_radical_signs_all_supports_and_trim_seams(self):
        from brepgpu.revolution import revolve
        s=revolve(make_profile(RECTANGLE)).solid
        cases=(((F(1,2),0,0),'interior',()),((0,0,0),'exterior',()),
               ((F(1,4),0,0),'boundary',(4,)),((F(3,4),F(1,2),0),'boundary',(2,3)),
               ((F(1,2),0,F(1,2)),'interior',()),((F(3,4),0,F(1,4)),'exterior',()))
        for point,label,faces in cases:
            a=s.classify_point(point);self.assertIs(a.solid,s)
            self.assertEqual((a.point,a.label,a.boundary_faces),(tuple(map(F,point)),label,faces))
        tri=revolve(make_profile(TRIANGLE)).solid
        self.assertEqual(tri.classify_point((F(3,10),F(-3,8),F(2,5))).boundary_faces,(1,))
        self.assertEqual(tri.classify_point((F(1,4),F(-1,2),0)).boundary_faces,(1,3))
        tiny=F(1,1<<1074)
        self.assertEqual(tri.classify_point((F(3,10),F(-3,8)-tiny,F(2,5))).label,'exterior')
        self.assertEqual(tri.classify_point((F(3,10),F(-3,8)+tiny,F(2,5))).label,'interior')
        for slot in range(16):
            q=slot%4
            self.assertEqual(s.classify(slot,0,F(1,3)),BOUNDARY if q==0 else INTERIOR)
            self.assertEqual(s.classify(slot,1,F(1,3)),BOUNDARY if q==3 else INTERIOR)
            self.assertEqual(s.classify(slot,F(1,3),0),BOUNDARY)
        self.assertEqual(s.classify(0,math.nan,.5),INVALID)
        self.assertEqual(s.classify(0,-1,.5),INVALID)
        with self.assertRaises(TypeError):s.classify(True,.5,.5)
        with self.assertRaises(TypeError):s.classify_point((True,0,0))

    def test_complete_consumed_modified_generated_history_and_exact_roles(self):
        from brepgpu.revolution import revolve,EntityKey,HistoryRecord,_validate_candidate
        p=make_profile(TRIANGLE);r=revolve(p);h=r.history;n=3
        self.assertEqual((len(h.records),len(h.origins)),(12,39))
        self.assertEqual(Counter(x.status for x in h.records),dict(modified=9,consumed=3))
        self.assertEqual(Counter(x.relation for x in h.origins),dict(modified=12,generated=27))
        for rec in h.records:
            self.assertEqual(len(rec.images),2 if rec.source.kind=='coedge' else 1 if rec.source.kind in ('vertex','edge') else 0)
            for image in rec.images+rec.generated:
                o=h.origin(image);self.assertIn(rec.source,o.parents)
                self.assertIn(image,rec.generated if o.relation=='generated' else rec.images)
        for o in h.origins:
            for parent in o.parents:self.assertIn(o.target,h.from_input(parent).generated if o.relation=='generated' else h.from_input(parent).images)
        for i in range(n):
            end=h.origin(EntityKey(r.solid.body_id,1,'coedge',4*i+2));start=h.origin(EntityKey(r.solid.body_id,1,'coedge',4*i+4))
            self.assertEqual((end.role,end.periodic,end.orientation),('seam_end',(F(4),F(0),F(0)),1))
            self.assertEqual((start.role,start.periodic,start.orientation),('seam_start',(F(0),F(0),F(0)),-1))
            self.assertEqual(end.parents,start.parents)
        origin=h.origin(EntityKey(r.solid.body_id,1,'face',1))
        self.assertEqual(origin.source_t,(F(0),F(0),F(1)));self.assertEqual(origin.periodic,(F(0),F(1),F(0)))
        bad_origins=tuple(replace(o,parents=(EntityKey(p.body_id,1,'edge',2),EntityKey(p.body_id,1,'coedge',2),EntityKey(p.body_id,1,'face',1))) if o.target.kind=='face' and o.target.entity_id==1 else o for o in h.origins)
        bad=copy(h);object.__setattr__(bad,'origins',bad_origins)
        object.__setattr__(bad,'records',tuple(HistoryRecord(rec.source,rec.status,rec.images,tuple(o.target for o in bad_origins if o.relation=='generated' and rec.source in o.parents)) for rec in h.records))
        with self.assertRaises(ArithmeticError):_validate_candidate(r.solid,bad,p)
        other=make_profile(TRIANGLE)
        with self.assertRaises(KeyError):h.from_input(EntityKey(other.body_id,1,'edge',1))
        with self.assertRaises(KeyError):h.origin(object())

    def test_verified_max32_tiny_profile_frames_and_source_cyclic_identity(self):
        from brepgpu.revolution import revolve
        points=many32();self.assertEqual(len(points),32)
        for i,a in enumerate(points):
            b=points[(i+1)%32]
            self.assertTrue(all((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])>0 for j,c in enumerate(points) if j not in (i,(i+1)%32)))
        result=revolve(make_profile(points));s=result.solid
        self.assertEqual((len(s.vertices),len(s.edges),len(s.coedges),len(s.faces),len(s.charts),len(result.history.origins)),(32,64,128,32,128,416))
        shifted=revolve(make_profile(points[7:]+points[:7]));self.assertEqual(shifted.solid.volume,s.volume)
        self.assertNotEqual(shifted.profile.body_id,result.profile.body_id)
        tiny=((F(1,4),0),(F(65,256),0),(F(1,4),F(1,256)))
        for scale in (.25,.5,1.,2.,4.):
            frame=Frame((16.,-16.,.125),scale);p=make_profile(TRIANGLE,frame);r=revolve(p)
            self.assertEqual(r.solid.volume.pi,F(9,32)*F(scale)**3)
            self.assertEqual(r.solid.edge_point(1,F(1,4)),(F(16),F(-16)-F(scale)/2,F(1,8)-F(scale)/4))
            self.assertEqual(len(revolve(make_profile(tiny,frame)).solid.faces),3)

    def test_rejected_axis_family_immutable_factories_and_late_failure(self):
        from brepgpu import revolution as m
        p=make_profile(TRIANGLE);r=m.revolve(p)
        for points in (((0,0),(F(1,2),0),(0,F(1,2))),((F(-1,2),0),(0,0),(F(-1,4),F(1,2))),((F(63,256),0),(F(1,2),0),(F(1,4),F(1,2)))):
            with self.assertRaises(m.UnsupportedRevolution):m.revolve(make_profile(points))
        for value in (object(),make_body('box'),r.solid):
            with self.assertRaises(TypeError):m.revolve(value)
        for cls in (m.RevolvedSolid,m.RevolutionResult,m.History,m.PointClassification):
            with self.assertRaises(TypeError):cls()
        with self.assertRaises((FrozenInstanceError,AttributeError)):r.solid.volume=None
        before=(p.vertices,p.curves,p.patches,r.solid.coedges,r.history.origins)
        with patch.object(m,'_validate_candidate',side_effect=RuntimeError('late revolution failure')):
            with self.assertRaisesRegex(RuntimeError,'late revolution failure'):m.revolve(p)
        self.assertEqual(before,(p.vertices,p.curves,p.patches,r.solid.coedges,r.history.origins))
        self.assertEqual(m.revolve(p).solid.volume,r.solid.volume)


if __name__=='__main__':unittest.main()
