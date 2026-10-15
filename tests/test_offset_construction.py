"""Complete exact signed sharp-cylinder offset contract; fleet execution only."""
from collections import Counter
from copy import copy
from dataclasses import FrozenInstanceError,replace
from fractions import Fraction as F
import math
import struct
import unittest
from unittest.mock import patch

from brepgpu.body import Frame,make_body,INVALID,REJECTED,INTERIOR,BOUNDARY
from brepgpu.boolean import EntityKey


def circle(t):
    q,u=(3,F(1)) if t==1 else (int(4*t),4*t-int(4*t))
    x,y=(1-u*u)/(1+u*u),2*u/(1+u*u)
    return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def world(frame,p):return tuple(F(o)+F(frame.scale)*x for o,x in zip(frame.origin,p))


class OffsetConstructionTests(unittest.TestCase):
    def test_entry_healthy_source_then_complete_actual_offset(self):
        source=make_body('cylinder')
        self.assertEqual(source.classify_point((0,0,0)).label,'interior')
        from brepgpu.offset import offset_solid
        result=offset_solid(source,F(1,8));solid=result.solid
        self.assertIs(result.source,source);self.assertIs(solid.source,source);self.assertIs(solid.frame,source.frame)
        self.assertEqual((result.distance,solid.distance),(F(1,8),F(1,8)))
        self.assertEqual((result.join,solid.join),('intersection','intersection'))
        self.assertNotEqual(solid.body_id,source.body_id)
        self.assertEqual(tuple(map(len,(solid.vertices,solid.edges,solid.coedges,solid.loops,solid.faces,solid.charts))),(2,3,6,3,3,6))
        self.assertEqual((solid.volume.rational,solid.volume.pi),(F(0),F(45,256)))
        self.assertFalse(hasattr(solid,'shape'));self.assertFalse(hasattr(solid,'packed_tables'))

    def test_exact_changed_carriers_all_signed_distances_and_source_preservation(self):
        from brepgpu.offset import offset_solid
        for d in (F(-1,8),F(-17,256),F(-1,256),F(1,256),F(17,256),F(1,8)):
            source=make_body('cylinder',Frame((1.25,-2.,3.),.5));before=(source.vertices,source.curves,source.patches,source.packed_tables)
            solid=offset_solid(source,d).solid;R,H=F(1,4)+d,F(1,2)+d
            self.assertEqual(tuple(v.position for v in solid.vertices),((R,F(0),-H),(R,F(0),H)))
            self.assertTrue(all(type(x) is F for v in solid.vertices for x in v.position))
            for slot,p in enumerate(solid.patches):
                if slot<2:
                    expected=tuple(float(x) for u in (-R,R) for v in (-R,R) for x in (u,v,H if slot==0 else -H,F(1)))
                    self.assertEqual((p.degree_u,p.degree_v),(1,1))
                else:
                    q=slot-2;controls=[]
                    for a,b,w in ((1,0,1),(1,1,1),(0,2,2)):
                        a,b=((a,b),(-b,a),(-a,-b),(b,-a))[q]
                        for z in (-H,H):controls.extend(map(float,(R*a,R*b,z*w,w)))
                    expected=tuple(controls);self.assertEqual((p.degree_u,p.degree_v),(2,1))
                self.assertEqual(p.controls,expected)
                old=source.patches[slot]
                for name in ('uv','origin'):
                    a,b=getattr(old,name),getattr(p,name)
                    self.assertEqual(struct.pack('<'+'d'*len(a),*a),struct.pack('<'+'d'*len(b),*b))
                self.assertEqual(struct.pack('<d',p.scale),struct.pack('<d',old.scale))
                self.assertEqual((p.lineage.body_id,p.lineage.entity_id,p.lineage.chart_id),(solid.body_id,1 if slot==0 else 2 if slot==1 else 3,slot+1))
                for i in range(0,len(p.controls),4):self.assertTrue(all(abs(x)<=p.controls[i+3] for x in p.controls[i:i+3]))
            self.assertEqual(before,(source.vertices,source.curves,source.patches,source.packed_tables))

    def test_exact_ring_seam_readers_closed_words_and_three_port_links(self):
        from brepgpu.offset import offset_solid
        source=make_body('cylinder');solid=offset_solid(source,F(17,256)).solid;R,H=F(81,256),F(145,256)
        self.assertEqual(tuple((e.start,e.end) for e in solid.edges),((0,0),(1,1),(0,1)))
        for t in (F(0),F(1,7),F(1,4),F(1,2),F(3,4),F(6,7),F(1)):
            c,s=circle(t)
            self.assertEqual(solid.edge_point(1,t),(R*c,R*s,-H))
            self.assertEqual(solid.edge_point(2,t),(R*c,R*s,H))
            self.assertEqual(solid.edge_point(3,t),(R,F(0),-H+2*H*t))
            self.assertEqual(solid.coedge_uv(1,t),((1+c)/2,(1+s)/2))
            self.assertEqual(solid.coedge_uv(2,t),((1+c)/2,(1+s)/2))
            self.assertEqual(tuple(solid.coedge_uv(i,t) for i in (3,4,5,6)),((4*t,F(0)),(F(4),t),(4*t,F(1)),(F(0),t)))
        self.assertEqual(tuple(c.orientation for c in solid.coedges),(1,-1,1,1,-1,-1))
        self.assertEqual(tuple(l.coedge_slots for l in solid.loops),((0,),(1,),(2,3,4,5)))
        for es in range(3):
            uses=[c for c in solid.coedges if c.edge_slot==es]
            self.assertEqual(len(uses),2);self.assertEqual(sum(c.orientation for c in uses),0)
            self.assertEqual(len({c.face_slot for c in uses}),1 if es==2 else 2)
        for vertex in range(2):
            germs={(es,end) for es,e in enumerate(solid.edges) for end,v in enumerate((e.start,e.end)) if v==vertex};self.assertEqual(len(germs),3);link={g:[] for g in germs}
            for loop in solid.loops:
                word=loop.coedge_slots
                for prev,cur in zip(word[-1:]+word[:-1],word):
                    a,b=solid.coedges[prev],solid.coedges[cur];ea,eb=solid.edges[a.edge_slot],solid.edges[b.edge_slot];end=1 if a.orientation==1 else 0;start=0 if b.orientation==1 else 1
                    self.assertEqual((ea.start,ea.end)[end],(eb.start,eb.end)[start])
                    if (ea.start,ea.end)[end]==vertex:
                        x,y=(a.edge_slot,end),(b.edge_slot,start);link[x].append(y);link[y].append(x)
            self.assertTrue(all(len(v)==2 for v in link.values()))
        data=solid.topology_input();self.assertEqual((data.edge_count,data.face_count,data.coedge_count,data.snapshots),(3,3,6,()))
        self.assertTrue(all(s.body_id==solid.body_id and s.revision==1 for s in data.edge_sources))
        self.assertEqual(2-3+3,2)

    def test_exact_material_erosion_and_sharp_outward_counterexample(self):
        from brepgpu.offset import offset_solid
        source=make_body('cylinder');out=offset_solid(source,F(1,8)).solid
        for p,label,faces in (((0,0,0),'interior',()),((F(3,8),0,0),'boundary',(3,)),
                              ((F(9,40),F(3,10),F(5,8)),'boundary',(1,3)),
                              ((0,0,F(-5,8)),'boundary',(2,)),((F(3,8),0,F(5,8)),'boundary',(1,3)),
                              ((F(1,2),0,0),'exterior',()),((0,0,F(3,4)),'exterior',())):
            a=out.classify_point(p);self.assertIs(a.solid,out);self.assertEqual((a.point,a.label,a.boundary_faces),(tuple(map(F,p)),label,faces))
        # A sharp new rim is farther than d from the original bounded source.
        displacement=(F(1,8),F(0),F(1,8));self.assertGreater(sum(x*x for x in displacement),F(1,8)**2)
        inset=offset_solid(source,F(-1,8)).solid
        self.assertEqual((inset.volume.rational,inset.volume.pi),(F(0),F(3,256)))
        self.assertEqual(inset.classify_point((F(1,8),0,F(3,8))).boundary_faces,(1,3))
        self.assertEqual(inset.classify_point((F(1,4),0,0)).label,'exterior')
        tiny=F(1,1<<1074)
        self.assertEqual(out.classify_point((F(3,8)+tiny,0,0)).label,'exterior')
        self.assertEqual(out.classify_point((F(3,8)-tiny,0,0)).label,'interior')
        self.assertEqual(out.classify_point((0,0,math.nextafter(.625,math.inf))).label,'exterior')
        self.assertEqual(out.classify(0,0,0),REJECTED);self.assertEqual(out.classify(0,1,F(1,2)),BOUNDARY)
        for q in range(4):
            self.assertEqual(out.classify(q+2,0,F(1,2)),BOUNDARY if q==0 else INTERIOR)
            self.assertEqual(out.classify(q+2,1,F(1,2)),BOUNDARY if q==3 else INTERIOR)
            self.assertEqual(out.classify(q+2,F(1,2),0),BOUNDARY)
        self.assertEqual(out.classify(0,math.nan,.5),INVALID)

    def test_complete_normal_feet_original_square_disk_and_axial_extensions(self):
        from brepgpu.offset import offset_solid
        source=make_body('cylinder');solid=offset_solid(source,F(1,8)).solid
        p=solid.source_foot(0,F(3,4),F(3,4))
        self.assertIs(p.solid,solid);self.assertEqual((p.chart_id,p.u,p.v),(1,F(3,4),F(3,4)))
        self.assertEqual((p.source_face,p.source_chart),(EntityKey(source.body_id,1,'face',1),EntityKey(source.body_id,1,'chart',1)))
        self.assertEqual((p.source_uv,p.source_point,p.point,p.normal,p.distance),((F(7,8),F(7,8)),(F(3,16),F(3,16),F(1,2)),(F(3,16),F(3,16),F(5,8)),(F(0),F(0),F(1)),F(1,8)))
        self.assertIs(p.in_source_square,True);self.assertEqual(p.source_trim,REJECTED)
        rim=solid.source_foot(0,1,F(1,2));self.assertEqual(rim.source_uv,(F(5,4),F(1,2)));self.assertIs(rim.in_source_square,False);self.assertEqual(rim.source_trim,INVALID)
        lower=solid.source_foot(2,F(1,2),0)
        self.assertEqual((lower.source_uv,lower.normal),((F(1,2),F(-1,8)),(F(3,5),F(4,5),F(0))))
        self.assertIs(lower.in_source_square,False);self.assertEqual(lower.source_trim,INVALID)
        for d in (F(-1,8),F(17,256),F(1,8)):
            s=offset_solid(source,d).solid
            for slot in range(6):
                for u,v in ((F(1,2),F(1,2)),(F(1),F(1,2)),(F(1,2),F(0))):
                    if s.classify(slot,u,v)==REJECTED:continue
                    foot=s.source_foot(slot,u,v)
                    self.assertEqual(tuple(a+foot.distance*n for a,n in zip(foot.source_point,foot.normal)),foot.point)
                    self.assertEqual(sum(x*x for x in foot.normal),1)
                    self.assertTrue(all(type(x) is F for x in (*foot.source_uv,*foot.source_point,*foot.point,*foot.normal,foot.distance)))
                    if d<0:self.assertIs(foot.in_source_square,True);self.assertIn(foot.source_trim,(INTERIOR,BOUNDARY))
        with self.assertRaises(ValueError):solid.source_foot(0,0,0)

    def test_all23_modified_origins_complete_parameter_owners_and_domains(self):
        from brepgpu.offset import offset_solid
        source=make_body('cylinder');r=offset_solid(source,F(1,8));h=r.history;s=r.solid
        self.assertEqual((len(h.records),len(h.origins)),(23,23));self.assertEqual(Counter(x.status for x in h.records),dict(modified=23))
        tables={'vertex':s.vertices,'edge':s.edges,'coedge':s.coedges,'loop':s.loops,'face':s.faces,'chart':s.charts}
        for rec in h.records:
            self.assertEqual(rec.images,(EntityKey(s.body_id,1,rec.source.kind,rec.source.entity_id),));self.assertEqual(rec.generated,());self.assertIs(h.from_input(rec.source),rec)
            o=h.origin(rec.images[0]);self.assertEqual((o.relation,o.parents,o.role),('modified',(rec.source,),'source_'+rec.source.kind))
            self.assertEqual(o.source_t,(F(0),F(1),F(0)) if rec.source.kind in ('edge','coedge') else None)
            self.assertEqual(o.source_t_owner,rec.source if rec.source.kind in ('edge','coedge') else None)
            self.assertEqual(o.source_t_relation,'boundary_parameter' if rec.source.kind in ('edge','coedge') else None)
            if rec.source.kind in ('vertex','edge'):self.assertIsNone(o.source_uv);self.assertIsNone(o.source_uv_owner);self.assertIsNone(o.source_uv_relation)
            else:
                self.assertEqual(o.source_uv_relation,'normal_foot');self.assertEqual(o.source_uv_owner.body_id,source.body_id)
                self.assertTrue(all(type(x) is F for x in o.source_uv.domain))
            kind,id=rec.source.kind,rec.source.entity_id
            expected=1 if kind in ('vertex','edge') else s.coedges[id-1].orientation if kind=='coedge' else s.faces[(s.loops[id-1].face_slot if kind=='loop' else s.charts[id-1].face_slot if kind=='chart' else id-1)].orientation
            self.assertEqual(o.orientation,expected)
        cap=h.origin(EntityKey(s.body_id,1,'face',1));self.assertEqual(cap.source_uv.data,(F(-1,4),F(3,2),F(0),F(-1,4),F(0),F(3,2)))
        wall=h.origin(EntityKey(s.body_id,1,'face',3));self.assertEqual(wall.source_uv.domain,(F(0),F(4),F(0),F(1)));self.assertEqual(wall.source_uv.evaluate(4,0),(F(4),F(-1,8)))
        ch=h.origin(EntityKey(s.body_id,1,'chart',6));self.assertEqual(ch.source_uv.domain,(F(0),F(1),F(0),F(1)));self.assertEqual(ch.source_uv.evaluate(F(1,2),1),(F(7,2),F(9,8)))
        co=h.origin(EntityKey(s.body_id,1,'coedge',1));self.assertEqual((co.source_uv.kind,co.source_uv.data),('circle',(F(3,2),)))
        self.assertEqual(co.source_uv.evaluate(F(1,8),0),(F(19,20),F(11,10)))
        with self.assertRaises(KeyError):h.from_input(EntityKey(make_body('cylinder').body_id,1,'face',1))
        alias=copy(h.records[0].source);object.__setattr__(alias,'entity_id',float(alias.entity_id))
        with self.assertRaises(KeyError):h.from_input(alias)
        with self.assertRaises(KeyError):h.origin(object())

    def test_all_frame_scales_actual_world_maps_and_signed_normal_identity(self):
        from brepgpu.offset import offset_solid
        for scale in (.25,.5,1.,2.,4.):
            for d in (F(-1,8),F(17,256)):
                source=make_body('cylinder',Frame((16.,-16.,.125),scale));s=offset_solid(source,d).solid;R,H=F(1,4)+d,F(1,2)+d
                self.assertEqual(s.volume.pi,2*R*R*H*F(scale)**3)
                self.assertEqual(s.edge_point(2,F(1,8)),world(source.frame,(R*F(3,5),R*F(4,5),H)))
                for slot in range(6):
                    foot=s.source_foot(slot,F(1,2),F(1,2));self.assertEqual(foot.distance,d*F(scale))
                    self.assertEqual(foot.point,tuple(x+foot.distance*n for x,n in zip(foot.source_point,foot.normal)))
                    self.assertEqual((foot.source_face.body_id,foot.source_chart.body_id),(source.body_id,source.body_id))
                self.assertEqual(s.classify_point(source.frame.origin).label,'interior')

    def test_bounded_admission_strict_source_immutable_publication_and_retry(self):
        from brepgpu import offset as m
        unit=(F(0),F(1),F(0),F(1))
        for kind,width in (('affine',7),('circle',2)):
            seen=[]
            def data():
                for i in range(width):seen.append(i);yield F(1)
                raise RuntimeError('read beyond fixed map data budget')
            with self.assertRaises(ValueError):m.UVMap2(kind,data(),unit)
            self.assertEqual(len(seen),width)
        def domain():
            for i in range(5):yield F(i)
            raise RuntimeError('read beyond fixed domain budget')
        with self.assertRaises(ValueError):m.UVMap2('circle',(F(1),),domain())
        source=make_body('cylinder');r=m.offset_solid(source,F(1,8))
        for d in (0,F(33,256),F(-33,256),F(1,257),math.inf,math.nan,True,'0.125'):
            with self.assertRaises((TypeError,ValueError)):m.offset_solid(source,d)
        for other in (object(),make_body('box'),r.solid):
            with self.assertRaises((TypeError,ValueError)):m.offset_solid(other,F(1,8))
        bad=copy(source);object.__setattr__(bad,'edges',(replace(source.edges[0],start=False),)+source.edges[1:])
        with self.assertRaises((ArithmeticError,ValueError)):m.offset_solid(bad,F(1,8))
        bad=copy(source);object.__setattr__(bad,'vertices',(replace(source.vertices[0],position=tuple(map(F,source.vertices[0].position))),)+source.vertices[1:])
        with self.assertRaises((ArithmeticError,ValueError)):m.offset_solid(bad,F(1,8))
        for cls in (m.OffsetSolid,m.OffsetResult,m.History,m.Footpoint,m.PointClassification):
            with self.assertRaises(TypeError):cls()
        with self.assertRaises((FrozenInstanceError,AttributeError)):r.solid.distance=None
        before=(source.vertices,source.curves,source.packed_tables,r.solid.coedges,r.history.origins)
        with patch.object(m,'_validate_candidate',side_effect=RuntimeError('late offset failure')):
            with self.assertRaisesRegex(RuntimeError,'late offset failure'):m.offset_solid(source,F(-1,8))
        self.assertEqual(before,(source.vertices,source.curves,source.packed_tables,r.solid.coedges,r.history.origins))
        self.assertEqual(m.offset_solid(source,F(1,8)).solid.volume,r.solid.volume)


if __name__=='__main__':unittest.main()
