"""Complete source-box fillet CPU contract; fleet execution only."""
from collections import Counter
from copy import copy
from dataclasses import FrozenInstanceError, replace
from fractions import Fraction as F
from math import comb
import math
import struct
import unittest
from unittest.mock import patch

from brepgpu.body import Frame, make_body, INVALID, REJECTED, BOUNDARY, INTERIOR
from brepgpu.boolean import EntityKey


def selected(source,edge=11):
    return EntityKey(source.body_id,source.revision,'edge',edge)


def rotate(x,y,q):
    return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def world(frame,p):
    return tuple(F(o)+F(frame.scale)*x for o,x in zip(frame.origin,p))


def plane(p,u,v):
    # Exact independent original degree(1,1) carrier evaluation.
    cp=tuple(tuple(F(x) for x in p.controls[i:i+4]) for i in range(0,16,4))
    return tuple((1-u)*(1-v)*cp[0][i]+(1-u)*v*cp[1][i]+u*(1-v)*cp[2][i]+u*v*cp[3][i] for i in range(3))


def rational_patch(p,u,v):
    terms=[]
    for i in range(p.degree_u+1):
        for j in range(p.degree_v+1):
            b=F(comb(p.degree_u,i))*u**i*(1-u)**(p.degree_u-i)*F(comb(p.degree_v,j))*v**j*(1-v)**(p.degree_v-j)
            off=4*(i*(p.degree_v+1)+j)
            terms.append((b,tuple(F(x) for x in p.controls[off:off+4])))
    values=tuple(sum(b*c[k] for b,c in terms) for k in range(4))
    return tuple(x/values[3] for x in values[:3])


class FilletConstructionTests(unittest.TestCase):
    def test_entry_actual_source_selected_edge_and_complete_graph(self):
        source=make_body('box')
        self.assertEqual(source.classify_point((0,0,0)).label,'interior')
        from brepgpu.fillet import fillet_edge
        key=selected(source);r=fillet_edge(source,key,F(3,8));s=r.solid
        self.assertIs(r.source,source);self.assertIs(s.source,source)
        self.assertIs(r.edge,key);self.assertIs(s.edge,key)
        self.assertEqual((r.radius,s.radius),(F(3,8),F(3,8)))
        self.assertNotEqual(s.body_id,source.body_id);self.assertIs(s.frame,source.frame)
        self.assertEqual(tuple(map(len,(s.vertices,s.edges,s.coedges,s.loops,s.faces,s.charts))),(10,15,30,7,7,7))
        self.assertFalse(hasattr(s,'shape'));self.assertFalse(hasattr(s,'packed_tables'))
        self.assertEqual((s.volume.rational,s.volume.pi),(F(135,64),F(9,256)))
        self.assertEqual(len(r.contacts),2)

    def test_exact_quarter_controls_all_corners_and_original_carrier_bits(self):
        from brepgpu.fillet import fillet_edge
        for edge in range(9,13):
            for radius in (F(1,16),F(17,256),F(1,2)):
                source=make_body('box',Frame((1.25,-2.,3.),.5));s=fillet_edge(source,selected(source,edge),radius).solid
                q=(edge-9+2)%4;cx,cy=rotate(F(3,4)-radius,F(3,4)-radius,q)
                controls=[]
                for a,b,w in ((1,0,1),(1,1,1),(0,2,2)):
                    a,b=rotate(a,b,q)
                    for z in (F(-1,2),F(1,2)):
                        controls.extend(map(float,(cx*w+radius*a,cy*w+radius*b,z*w,w)))
                p=s.patches[6]
                self.assertEqual((p.degree_u,p.degree_v,p.controls,p.uv),(2,1,tuple(controls),(0.,1.,0.,1.)))
                self.assertEqual(s.charts[6].quadrant,q)
                for slot in range(6):
                    a,b=source.patches[slot],s.patches[slot]
                    self.assertEqual(struct.pack('<'+str(len(a.controls))+'d',*a.controls),struct.pack('<'+str(len(b.controls))+'d',*b.controls))
                    self.assertEqual((a.uv,a.origin,a.scale),(b.uv,b.origin,b.scale))
                for slot,p in enumerate(s.patches):
                    self.assertEqual((p.lineage.body_id,p.lineage.entity_id,p.lineage.chart_id),(s.body_id,slot+1,slot+1))
                old_positions=tuple(v.position for i,v in enumerate(source.vertices) if i not in (edge-9,edge-5))
                self.assertEqual(tuple(v.position for v in s.vertices[:6]),old_positions)
                self.assertTrue(all(type(x) is float for v in s.vertices[:6] for x in v.position))
                for t in (F(0),F(1,7),F(1)):
                    c,d=(1-t*t)/(1+t*t),2*t/(1+t*t);x,y=rotate(c,d,q)
                    self.assertEqual(rational_patch(p,t,F(2,5)),(cx+radius*x,cy+radius*y,F(-1,10)))

    def test_source_subranges_actual_arc_pcurves_and_every_incident_reader(self):
        from brepgpu.fillet import fillet_edge
        source=make_body('box');r=fillet_edge(source,selected(source),F(17,256));s=r.solid
        edges=[o for o in r.history.origins if o.target.kind=='edge' and o.relation=='modified']
        self.assertEqual(len(edges),4)
        delta=F(17,384)
        for o in edges:
            self.assertEqual(o.source_t_relation,'coincident');self.assertEqual(o.source_t_owner,o.parents[0])
            self.assertIn(o.source_t,((delta,1-delta,F(0)),(F(0),1-delta,F(0))))
            e=source.edges[o.source_t_owner.entity_id-1];curve=source.curves[e.curve_slot]
            for t in (F(0),F(1,7),F(1)):
                a,b,c=o.source_t;old=a+b*t
                expected=tuple((1-old)*F(x)+old*F(y) for x,y in zip(curve.data[:3],curve.data[3:]))
                self.assertEqual(s.edge_point(o.target.entity_id,t),expected)
        for coedge in s.coedges:
            for t in (F(0),F(1,7),F(1)):
                uv=s.coedge_uv(coedge.id,t);point=s.edge_point(s.edges[coedge.edge_slot].id,t)
                p=s.patches[s.faces[coedge.face_slot].chart_slots[0]]
                self.assertEqual(world(s.frame,rational_patch(p,*uv)),point)
        for o in r.history.origins:
            if o.role in ('lower_arc','upper_arc'):
                self.assertEqual(s.curves[s.edges[o.target.entity_id-1].curve_slot].kind,'quarter_arc')
            if o.source_uv is not None and o.target.kind in ('edge','coedge','vertex'):
                for t in (F(0),F(1,7),F(1)):
                    args=(0,0) if o.target.kind=='vertex' else (t,0)
                    uv=o.source_uv.evaluate(*args);owner=o.source_uv_owner
                    face=source.faces[owner.entity_id-1] if owner.kind=='face' else source.faces[source.charts[owner.entity_id-1].face_slot]
                    expected=world(source.frame,plane(source.patches[face.chart_slots[0]],*uv))
                    actual=world(s.frame,tuple(map(F,s.vertices[o.target.entity_id-1].position))) if o.target.kind=='vertex' else s.edge_point(o.target.entity_id if o.target.kind=='edge' else s.edges[s.coedges[o.target.entity_id-1].edge_slot].id,t)
                    self.assertEqual(actual,expected)

    def test_complete_two_rail_contact_maps_and_tangent_normals(self):
        from brepgpu.fillet import fillet_edge
        for edge in range(9,13):
            source=make_body('box');r=fillet_edge(source,selected(source,edge),F(3,8));s=r.solid
            q=(edge-9+2)%4
            self.assertEqual(tuple(c.blend_u for c in r.contacts),(0,1))
            for contact in r.contacts:
                self.assertEqual(contact.blend_face,EntityKey(s.body_id,1,'face',7))
                old=source.coedges[contact.source_coedge.entity_id-1]
                self.assertEqual((old.edge_slot,old.face_slot),(edge-1,contact.source_face.entity_id-1))
                nx,ny=rotate(1,0,q) if contact.blend_u==0 else rotate(0,1,q)
                self.assertEqual(contact.normal,(F(nx),F(ny),F(0)))
                self.assertEqual(contact.source_t,(F(0),F(1),F(0)))
                for t in (F(0),F(1,7),F(1,2),F(1)):
                    uv=contact.source_uv.evaluate(t,0);buv=contact.blend_uv.evaluate(t,0)
                    self.assertEqual(buv,(F(contact.blend_u),t))
                    p=source.patches[old.face_slot]
                    self.assertEqual(world(source.frame,plane(p,*uv)),s.edge_point(contact.edge.entity_id,t))
                    self.assertEqual(world(s.frame,rational_patch(s.patches[6],*buv)),s.edge_point(contact.edge.entity_id,t))

    def test_closed_words_opposed_pairs_links_and_typed_topology(self):
        from brepgpu.fillet import fillet_edge
        source=make_body('box');s=fillet_edge(source,selected(source),F(1,4)).solid
        self.assertEqual(len(s.vertices)-len(s.edges)+len(s.faces),2)
        for es,e in enumerate(s.edges):
            uses=[c for c in s.coedges if c.edge_slot==es]
            self.assertEqual(len(uses),2);self.assertEqual(sum(c.orientation for c in uses),0)
            self.assertEqual(len({c.face_slot for c in uses}),2)
        for vertex in range(10):
            germs={(es,end) for es,e in enumerate(s.edges) for end,v in enumerate((e.start,e.end)) if v==vertex}
            self.assertEqual(len(germs),3);link={g:[] for g in germs}
            for loop in s.loops:
                word=loop.coedge_slots
                for prev,cur in zip(word[-1:]+word[:-1],word):
                    a,b=s.coedges[prev],s.coedges[cur];ea,eb=s.edges[a.edge_slot],s.edges[b.edge_slot]
                    end=1 if a.orientation==1 else 0;start=0 if b.orientation==1 else 1
                    self.assertEqual((ea.start,ea.end)[end],(eb.start,eb.end)[start])
                    if (ea.start,ea.end)[end]==vertex:
                        x,y=(a.edge_slot,end),(b.edge_slot,start);link[x].append(y);link[y].append(x)
            self.assertTrue(all(len(v)==2 for v in link.values()))
        data=s.topology_input();self.assertEqual((data.edge_count,data.face_count,data.coedge_count),(15,7,30));self.assertEqual(data.snapshots,())
        self.assertTrue(all(x.body_id==s.body_id and x.revision==s.revision for x in data.edge_sources))

    def test_material_all_bounded_supports_artificial_lines_and_nonbinary_trims(self):
        from brepgpu.fillet import fillet_edge
        source=make_body('box');s=fillet_edge(source,selected(source),F(1,4)).solid
        for point,label,faces in (((0,0,0),'interior',()),((F(3,4),F(3,4),0),'exterior',()),
                                  ((F(1,2),F(1,2),0),'interior',()),((F(1,4),F(1,2),0),'interior',()),
                                  ((F(3,4),F(1,2),0),'boundary',(5,7)),
                                  ((F(1,2),F(3,4),F(1,2)),'boundary',(1,4,7)),
                                  ((F(7,10),F(13,20),F(-1,2)),'boundary',(2,7))):
            a=s.classify_point(point);self.assertEqual((a.label,a.boundary_faces),(label,faces));self.assertIs(a.solid,s)
        tiny=F(1,1<<1074)
        self.assertEqual(s.classify_point((F(7,10)+tiny,F(13,20),0)).label,'exterior')
        self.assertEqual(s.classify_point((F(7,10)-tiny,F(13,20),0)).label,'interior')
        self.assertEqual(s.classify(0,1,1),REJECTED)
        self.assertEqual(s.classify(6,F(1,7),F(1,3)),INTERIOR)
        self.assertEqual(s.classify(6,0,F(1,3)),BOUNDARY)
        self.assertEqual(s.classify(6,math.nan,.5),INVALID)
        self.assertEqual(s.classify(6,-1,.5),INVALID)
        for value in (True,'0'):
            with self.assertRaises(TypeError):s.classify_point((value,0,0))

    def test_complete_history_roles_owners_and_exact_forward_reverse_maps(self):
        from brepgpu.fillet import fillet_edge,_validate_candidate
        source=make_body('box');r=fillet_edge(source,selected(source),F(17,256));s=r.solid;h=r.history
        self.assertEqual((len(h.records),len(h.origins)),(62,76))
        self.assertEqual(Counter(x.status for x in h.records),dict(retained=33,modified=24,deleted=5))
        self.assertEqual(Counter(x.relation for x in h.origins),dict(retained=33,modified=24,generated=19))
        for rec in h.records:
            self.assertEqual(h.from_input(rec.source),rec)
            self.assertEqual(len(rec.images),0 if rec.status=='deleted' else 1)
            for key in rec.images+rec.generated:
                origin=h.origin(key);self.assertIn(rec.source,origin.parents)
                self.assertIn(key,rec.generated if origin.relation=='generated' else rec.images)
        for o in h.origins:
            self.assertEqual(h.origin(o.target),o)
            for p in o.parents:self.assertIn(o.target,h.from_input(p).generated if o.relation=='generated' else h.from_input(p).images)
            if o.relation=='generated':self.assertIn(r.edge,o.parents)
            self.assertEqual(o.orientation,1 if o.target.kind in ('vertex','edge') else s.coedges[o.target.entity_id-1].orientation if o.target.kind=='coedge' else s.faces[(s.loops[o.target.entity_id-1].face_slot if o.target.kind=='loop' else s.charts[o.target.entity_id-1].face_slot if o.target.kind=='chart' else o.target.entity_id-1)].orientation)
            self.assertEqual(o.source_t is None,o.source_t_owner is None)
            self.assertEqual(o.source_t is None,o.source_t_relation is None)
            self.assertEqual(o.source_uv is None,o.source_uv_owner is None)
            if o.role in ('band_face','band_loop','band_chart'):
                self.assertEqual((o.source_t_owner,o.source_t_relation,o.source_t,o.source_uv),(r.edge,'spine',(F(0),F(0),F(1)),None))
            if o.target.kind=='coedge' and o.role=='source_coedge':
                old=source.coedges[o.parents[0].entity_id-1]
                self.assertEqual(o.source_uv_owner,EntityKey(source.body_id,1,'face',old.face_slot+1))
        charts=[rec for rec in h.records if rec.source.kind=='chart']
        self.assertEqual(Counter(rec.status for rec in charts),dict(retained=2,modified=4))
        alias=copy(h.records[0].source);object.__setattr__(alias,'entity_id',float(alias.entity_id))
        malformed=copy(h);object.__setattr__(malformed,'records',(replace(h.records[0],source=alias),)+h.records[1:])
        with self.assertRaises(ArithmeticError):_validate_candidate(s,malformed,r.contacts)
        contact=copy(r.contacts[0]);object.__setattr__(contact,'normal',tuple(float(x) for x in contact.normal))
        with self.assertRaises(ArithmeticError):_validate_candidate(s,h,(contact,r.contacts[1]))
        with self.assertRaises(KeyError):h.from_input(EntityKey(make_body('box').body_id,1,'edge',11))
        with self.assertRaises(KeyError):h.origin(object())

    def test_frames_source_alias_rejection_immutable_publication_and_retry(self):
        from brepgpu import fillet as m
        for kind,limit in (('affine',7),('arc',5)):
            consumed=[]
            def endless():
                for i in range(limit):consumed.append(i);yield F(0)
                raise RuntimeError('fixed-size map read past its admission budget')
            with self.assertRaises(ValueError):m.Map2(kind,endless())
            self.assertEqual(len(consumed),limit)
        for scale in (.25,.5,1.,2.,4.):
            source=make_body('box',Frame((16.,-16.,.125),scale));r=m.fillet_edge(source,selected(source,9),F(1,2))
            self.assertEqual((r.solid.volume.rational,r.solid.volume.pi),(2*F(scale)**3,F(1,16)*F(scale)**3))
            self.assertEqual(r.solid.classify_point(source.frame.origin).label,'interior')
        source=make_body('box');r=m.fillet_edge(source,selected(source),F(1,4))
        for radius in (F(15,256),F(129,256),F(1,3),math.inf,math.nan):
            with self.assertRaises((ValueError,TypeError)):m.fillet_edge(source,selected(source),radius)
        for radius in (True,'0.25'):
            with self.assertRaises(TypeError):m.fillet_edge(source,selected(source),radius)
        for key in (selected(make_body('box')),EntityKey(source.body_id,2,'edge',11),EntityKey(source.body_id,1,'edge',1),EntityKey(source.body_id,1,'face',1)):
            with self.assertRaises((ValueError,KeyError)):m.fillet_edge(source,key,F(1,4))
        for other in (object(),make_body('cylinder'),r.solid):
            with self.assertRaises((TypeError,ValueError)):m.fillet_edge(other,selected(source),F(1,4))
        malformed=copy(source);object.__setattr__(malformed,'edges',(replace(source.edges[0],start=False),)+source.edges[1:])
        with self.assertRaises((ArithmeticError,ValueError)):m.fillet_edge(malformed,selected(malformed),F(1,4))
        for cls in (m.FilletedSolid,m.FilletResult,m.History,m.PointClassification):
            with self.assertRaises(TypeError):cls()
        with self.assertRaises((FrozenInstanceError,AttributeError)):r.solid.radius=None
        before=(source.vertices,source.curves,source.packed_tables,r.solid.coedges,r.history.origins)
        with patch.object(m,'_validate_candidate',side_effect=RuntimeError('late fillet failure')):
            with self.assertRaisesRegex(RuntimeError,'late fillet failure'):m.fillet_edge(source,selected(source),F(1,4))
        self.assertEqual(before,(source.vertices,source.curves,source.packed_tables,r.solid.coedges,r.history.origins))
        self.assertEqual(m.fillet_edge(source,selected(source),F(1,4)).solid.volume,r.solid.volume)


if __name__=='__main__':unittest.main()
