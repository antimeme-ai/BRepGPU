"""Independent whole one-edge fillet geometry/history/contact/native truth."""
from dataclasses import fields,replace
from fractions import Fraction as F
import math
import os
import struct
import unittest
from unittest.mock import patch


def altered(value,**changes):
    result=object.__new__(type(value))
    for field in fields(value):object.__setattr__(result,field.name,changes.get(field.name,getattr(value,field.name)))
    return result


def world(result,point):return tuple(F(o)+F(result.source.frame.scale)*F(x) for o,x in zip(result.source.frame.origin,point))


class FilletTests(unittest.TestCase):
    def test_existing_healthy_box_then_actual_complete_edge_fillet(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import BodySampler,Candidate,SampleRequest
        source=make_body('box')
        with Engine(backend='cpu',threads=1) as engine:
            with BodySampler(engine,source) as sampler:
                batch=sampler.sample(SampleRequest(source,(Candidate(0,0.,0.),)))
            self.assertEqual(batch.rows[0].native_status,0)
            self.assertEqual(batch.rows[0].position,(-.75,-.75,.5))
        from brepgpu.boolean import EntityKey
        edge=EntityKey(source.body_id,1,'edge',11)
        from brepgpu.fillet import fillet_edge
        result=fillet_edge(source,edge,F(3,8))
        self.assertIs(result.source,source);self.assertIs(result.edge,edge)
        self.assertEqual(result.solid.classify_point((F(3,4),F(3,4),0)).label,'exterior')
        self.assertEqual(result.solid.classify_point((0,0,0)).label,'interior')

    def feature(self,edge_id=11,radius=F(3,8),frame=None):
        from brepgpu.body import Frame,make_body
        from brepgpu.boolean import EntityKey
        from brepgpu.fillet import fillet_edge
        from brepgpu.fillet_oracles import qualify_result
        source=make_body('box',Frame() if frame is None else frame);edge=EntityKey(source.body_id,source.revision,'edge',edge_id)
        result=fillet_edge(source,edge,radius)
        self.assertIs(result.source,source);self.assertIs(result.solid.source,source)
        self.assertIs(result.edge,edge);self.assertIs(result.solid.edge,edge)
        self.assertEqual(result.radius,F(radius));self.assertEqual(result.solid.radius,F(radius));self.assertIs(result.solid.frame,source.frame)
        quality=qualify_result(result)
        self.assertEqual((quality['history_inputs'],quality['history_outputs'],quality['retained'],quality['modified'],quality['deleted'],quality['generated']),(62,76,33,24,5,19))
        return result

    def sampled(self,result,request=None):
        from brepgpu import Engine
        from brepgpu.fillet_sampling import prepare_grid,sample
        from brepgpu.fillet_oracles import qualify_samples
        backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        request=prepare_grid(result,5) if request is None else request
        with Engine(backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1'))) as engine:
            batch=sample(engine,request)
        self.assertIs(batch.result,result);self.assertIs(batch.request,request);self.assertEqual((batch.backend,batch.device),(backend,device))
        self.assertEqual(qualify_samples(batch)['count'],len(request.candidates))
        return batch

    def test_all_four_corners_radii_frames_complete_domains_and_exact_volume(self):
        from brepgpu.body import Frame
        from brepgpu.fillet_oracles import qualify_result
        for edge in range(9,13):
            for radius in (F(1,16),F(17,256),F(1,2)):
                with self.subTest(edge=edge,radius=radius):
                    result=self.feature(edge,radius);q=qualify_result(result)
                    self.assertEqual(tuple(q[k] for k in ('vertices','edges','coedges','loops','faces','charts','genus')),(10,15,30,7,7,7,0))
                    self.assertEqual((result.solid.volume.rational,result.solid.volume.pi),(F(9,4)-radius*radius,radius*radius/4))
        for scale in (.25,.5,1.,2.,4.):
            result=self.feature(10,F(85,256),Frame((16.,-16.,.125),scale))
            self.assertEqual(result.solid.volume.rational,(F(9,4)-F(85,256)**2)*F(scale)**3)

    def test_literal_material_all_supports_cap_closures_and_unused_circle(self):
        result=self.feature();solid=result.solid
        cases=(((F(3,4),F(3,4),0),'exterior',()),((F(3,4),F(3,4),F(1,2)),'exterior',()),
               ((F(3,8),F(3,8),0),'interior',()),((0,F(3,8),0),'interior',()),
               ((F(3,8),F(1,2),0),'interior',()),((F(3,5),F(27,40),0),'boundary',(7,)),
               ((F(3,5),F(27,40),F(1,2)),'boundary',(1,7)),
               ((F(3,5),F(27,40),-F(1,2)),'boundary',(2,7)),
               ((F(3,4),F(3,8),0),'boundary',(5,7)),((F(3,8),F(3,4),0),'boundary',(4,7)),
               ((F(3,4),F(3,8),F(1,2)),'boundary',(1,5,7)),
               ((F(3,8),F(3,4),-F(1,2)),'boundary',(2,4,7)),
               ((-F(3,4),-F(3,4),F(1,2)),'boundary',(1,3,6)),
               ((F(21,32),F(21,32),0),'exterior',()),((F(1,2),F(1,2),0),'interior',()))
        for local,label,faces in cases:
            query=world(result,local);answer=solid.classify_point(query)
            self.assertIs(answer.solid,solid);self.assertEqual(answer.point,query)
            self.assertEqual((answer.label,answer.boundary_faces),(label,faces))
        tiny=F(1,1<<1074)
        for delta,label in ((tiny,'exterior'),(-tiny,'interior')):
            self.assertEqual(solid.classify_point((F(3,5)+delta,F(27,40)+delta,0)).label,label)
        for z,label in ((math.nextafter(.5,math.inf),'exterior'),(math.nextafter(.5,0.),'interior')):
            self.assertEqual(solid.classify_point((0,0,z)).label,label)

    def test_retained_source_bits_exact_shortened_ranges_and_nondyadic_trim(self):
        result=self.feature(radius=F(17,256));solid=result.solid;d=1-F(2,3)*result.radius
        self.assertEqual(solid.classify(4,d,F(1,2)),2)
        actual=float(d);self.assertNotEqual(F(actual),d)
        self.assertEqual(solid.classify(4,actual,.5),0 if F(actual)>d else 1)
        self.assertEqual(solid.classify(4,math.nextafter(actual,-math.inf),.5),1)
        self.assertEqual(solid.classify(4,math.nextafter(actual,math.inf),.5),0)
        for patch,old in zip(solid.patches[:6],result.source.patches):
            for name in ('controls','origin','uv'):
                self.assertEqual(tuple(struct.pack('<d',v) for v in getattr(patch,name)),tuple(struct.pack('<d',v) for v in getattr(old,name)))
            self.assertEqual(struct.pack('<d',patch.scale),struct.pack('<d',old.scale))
        source_edge=next(r for r in result.history.records if r.source.kind=='edge' and r.source.entity_id==2)
        origin=result.history.origin(source_edge.images[0])
        self.assertEqual((origin.source_t_owner,origin.source_t_relation,origin.source_t),(source_edge.source,'coincident',(F(0),d,F(0))))
        self.assertIsNone(origin.source_uv_owner);self.assertIsNone(origin.source_uv)
        source_edge3=next(r for r in result.history.records if r.source.kind=='edge' and r.source.entity_id==3)
        origin3=result.history.origin(source_edge3.images[0])
        self.assertEqual(origin3.source_t,(F(2,3)*result.radius,d,F(0)))
        for bad in (True,'0.5',object()):
            with self.assertRaises(TypeError):solid.classify(0,bad,F(1,2))

    def test_contact_rails_whole_exact_maps_normals_and_source_occurrences(self):
        result=self.feature();solid=result.solid
        self.assertEqual(len(result.contacts),2)
        for rail,edgeid,faceid,coedgeid,normal,blend_u in zip(result.contacts,(14,15),(5,4),(18,15),((1,0,0),(0,1,0)),(0,1)):
            self.assertEqual((rail.edge.body_id,rail.edge.kind,rail.edge.entity_id),(solid.body_id,'edge',edgeid))
            self.assertEqual((rail.source_face.body_id,rail.source_face.entity_id),(result.source.body_id,faceid))
            self.assertEqual((rail.source_coedge.entity_id,rail.blend_face.entity_id,rail.blend_u),(coedgeid,7,blend_u))
            self.assertEqual(rail.normal,tuple(map(F,normal)));self.assertEqual(rail.source_t,(F(0),F(1),F(0)))
            for t in (F(0),F(1,7),F(2,3),F(1)):
                sourceuv=rail.source_uv.evaluate(t,F(0));blenduv=rail.blend_uv.evaluate(t,F(0))
                self.assertEqual(sourceuv,(F(3,4),t));self.assertEqual(blenduv,(F(blend_u),t))
                expected=(F(3,4),F(3,8),t-F(1,2)) if blend_u==0 else (F(3,8),F(3,4),t-F(1,2))
                self.assertEqual(solid.edge_point(edgeid,t),expected)
        self.assertEqual(solid.edge_point(12,F(1,2)),(F(3,5),F(27,40),-F(1,2)))
        self.assertEqual(solid.edge_point(13,F(1,2)),(F(3,5),F(27,40),F(1,2)))
        # New quarter is finite: its endpoints have separate physical vertices.
        self.assertNotEqual(solid.edges[11].start,solid.edges[11].end)
        self.assertEqual(tuple(c.orientation for c in solid.coedges[-4:]),(1,1,-1,-1))

    def test_complete_history_owners_deleted_births_and_foreign_identity(self):
        from brepgpu.boolean import EntityKey
        result=self.feature();other=self.feature()
        self.assertNotEqual(result.source.body_id,other.source.body_id)
        with self.assertRaises(KeyError):result.history.from_input(EntityKey(other.source.body_id,1,'edge',11))
        deleted={(r.source.kind,r.source.entity_id) for r in result.history.records if r.status=='deleted'}
        self.assertEqual(deleted,{('vertex',3),('vertex',7),('edge',11),('coedge',15),('coedge',18)})
        selected=result.history.from_input(result.edge);self.assertEqual(selected.images,());self.assertTrue(selected.generated)
        arc=next(o for o in result.history.origins if o.role=='lower_arc')
        self.assertEqual(tuple((k.kind,k.entity_id) for k in arc.parents),(('edge',11),('vertex',3),('face',2)))
        self.assertEqual((arc.source_t_owner,arc.source_t_relation,arc.source_t),(result.edge,'spine',(F(0),F(0),F(0))))
        self.assertEqual((arc.source_uv_owner.kind,arc.source_uv_owner.entity_id,arc.source_uv.kind),('face',2,'arc'))
        self.assertEqual(arc.source_uv.evaluate(F(1,2),F(0)),(F(9,10),F(19,20)))
        band=next(o for o in result.history.origins if o.role=='band_face')
        self.assertEqual(tuple(k.entity_id for k in band.parents),(11,5,4));self.assertEqual(band.source_t,(F(0),F(0),F(1)))
        self.assertIsNone(band.source_uv_owner);self.assertIsNone(band.source_uv)
        bottom_chart=next(o for o in result.history.origins if o.target.kind=='chart' and o.target.entity_id==2)
        self.assertEqual(bottom_chart.orientation,-1)

    def test_actual_native_dyadic_contact_pairs_nonaxis_and_all_corners(self):
        from brepgpu.body import Frame
        from brepgpu.body_sampling import Candidate
        from brepgpu.fillet_sampling import FilletRequest
        result=self.feature()
        candidates=(Candidate(4,.75,.5),Candidate(6,0.,.5),Candidate(3,.75,.5),Candidate(6,1.,.5),
                    Candidate(6,F(1,2),F(1,3)),Candidate(0,1.,1.),Candidate(6,math.nan,.5),Candidate(6,-1.,.5))
        batch=self.sampled(result,FilletRequest(result,candidates))
        self.assertEqual(tuple(r.trim for r in batch.rows),(2,2,2,2,1,0,3,3))
        for a,b in ((0,1),(2,3)):
            self.assertEqual(batch.rows[a].position,batch.rows[b].position);self.assertEqual(batch.rows[a].normal,batch.rows[b].normal)
        for actual,expected in zip(batch.rows[4].normal,(.6,.8,0.)):
            self.assertAlmostEqual(actual,expected,delta=2.**-30)
        for edge in range(9,13):self.sampled(self.feature(edge,F(17,256)))
        for scale in (.25,4.):self.sampled(self.feature(12,F(1,16),Frame((16.,-16.,.125),scale)))

    def test_native_nonbinary_contacts_copy_actual_float_before_admission(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.fillet_sampling import FilletRequest
        result=self.feature(radius=F(17,256));d=1-F(2,3)*result.radius;actual=float(d)
        request=FilletRequest(result,(Candidate(4,d,.5),Candidate(6,0.,.5),Candidate(4,math.nextafter(actual,math.inf),.5),Candidate(4,math.nextafter(actual,-math.inf),.5)))
        batch=self.sampled(result,request)
        self.assertEqual(batch.rows[0].u,actual)
        self.assertEqual(tuple(r.trim for r in batch.rows),(0 if F(actual)>d else 1,2,0,1))
        self.assertEqual(struct.pack('<d',request.candidates[0].u),struct.pack('<d',actual))

    def test_actual_abi5_all_paired_mates_under_full_source_permutations(self):
        from brepgpu.adjacency import AdjacencyLibrary
        from brepgpu.fillet_oracles import qualify_topology
        result=self.feature(9,F(1,2));data=result.solid.topology_input()
        variants=(data,data.permuted(edge_order=tuple(reversed(range(15))),face_order=tuple(reversed(range(7))),coedge_order=tuple(range(1,30,2))+tuple(range(0,30,2))))
        library=AdjacencyLibrary();backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        for source in variants:
            actual=library.build(source,backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))
            self.assertIs(actual.input,source);q=qualify_topology(result,actual)
            self.assertEqual((q['paired'],q['same_face_pair']),(15,0))

    def test_wrong_source_keys_radii_and_existing_operation_guards(self):
        from brepgpu.body import make_body
        from brepgpu.boolean import EntityKey
        from brepgpu.fillet import fillet_edge
        source=make_body('box');other=make_body('box');good=EntityKey(source.body_id,1,'edge',11)
        for key in (object(),11,EntityKey(other.body_id,1,'edge',11),EntityKey(source.body_id,2,'edge',11),EntityKey(source.body_id,1,'face',5),EntityKey(source.body_id,1,'edge',1)):
            with self.assertRaises((TypeError,ValueError,KeyError)):fillet_edge(source,key,F(1,4))
        for radius in (0,-1,F(15,256),F(129,256),F(1,257),math.inf,math.nan,True,'0.25'):
            with self.assertRaises((TypeError,ValueError)):fillet_edge(source,good,radius)
        with self.assertRaises((TypeError,ValueError)):fillet_edge(make_body('cylinder'),good,F(1,4))
        result=self.feature()
        from brepgpu.body_sampling import SampleRequest
        from brepgpu.revolution_sampling import RevolutionRequest
        with self.assertRaises(TypeError):SampleRequest(result.solid,())
        with self.assertRaises(TypeError):RevolutionRequest(result,())
        with self.assertRaises((TypeError,ValueError)):result.solid.classify_point((True,0,0))
        with self.assertRaises((TypeError,ValueError)):result.solid.edge_point(12,True)

    def test_falsified_geometry_contact_owner_and_consistently_wrong_history_rejected(self):
        from brepgpu.fillet_oracles import qualify_result
        result=self.feature();solid=result.solid;bad=[]
        patches=list(solid.patches);values=list(patches[6].controls);values[0]+=1/256;patches[6]=replace(patches[6],controls=values)
        bad.append(altered(result,solid=altered(solid,patches=tuple(patches))))
        bad.append(altered(result,solid=altered(solid,faces=solid.faces[:-1])))
        bad.append(altered(result,solid=altered(solid,revision=True)))
        bad.append(altered(result,solid=altered(solid,loops=(replace(solid.loops[0],coedge_slots=(False,)+solid.loops[0].coedge_slots[1:]),)+solid.loops[1:])))
        rail=result.contacts[0]
        bad.extend(altered(result,contacts=(changed,result.contacts[1])) for changed in (replace(rail,normal=(F(-1),F(0),F(0))),replace(rail,source_face=result.contacts[1].source_face),altered(rail,blend_u=False)))
        origins=list(result.history.origins);i=next(i for i,o in enumerate(origins) if o.role=='lower_arc');old=origins[i]
        values=list(origins);values[i]=replace(old,source_uv_owner=replace(old.source_uv_owner,entity_id=1));bad.append(altered(result,history=altered(result.history,origins=tuple(values))))
        values=list(origins);values[i]=altered(old,orientation=True);bad.append(altered(result,history=altered(result.history,origins=tuple(values))))
        # Preserve reciprocal generated links while lying about the actual cap parent.
        parents=list(old.parents);wrong=replace(parents[2],entity_id=1);previous=parents[2];parents[2]=wrong;origins[i]=replace(old,parents=tuple(parents));records=[]
        for record in result.history.records:
            generated=record.generated
            if record.source==previous:generated=tuple(v for v in generated if v!=old.target)
            if record.source==wrong:generated=generated+(old.target,)
            records.append(replace(record,generated=generated))
        bad.append(altered(result,history=altered(result.history,origins=tuple(origins),records=tuple(records))))
        for value in bad:
            with self.assertRaises(AssertionError):qualify_result(value)

    def test_falsified_native_geometry_and_late_publication_rollback_retry(self):
        from brepgpu.fillet import fillet_edge
        from brepgpu.fillet_oracles import qualify_result,qualify_samples
        result=self.feature();batch=self.sampled(result);rows=list(batch.rows);index=next(i for i,r in enumerate(rows) if r.patch_slot==6 and r.native_status==0);row=rows[index]
        for changed in (replace(row,normal=tuple(-v for v in row.normal)),replace(row,position=(row.position[0]+.01,*row.position[1:])),replace(row,u=math.nextafter(row.u,math.inf)),replace(row,face_id=1)):
            values=list(rows);values[index]=changed
            with self.assertRaises(AssertionError):qualify_samples(altered(batch,rows=tuple(values)))
        source=result.source;packed=source.packed_tables;history=(result.history.records,result.history.origins);failure=RuntimeError('independent late fillet publication failure')
        with patch('brepgpu.fillet._validate_candidate',side_effect=failure) as final:
            with self.assertRaises(RuntimeError) as caught:fillet_edge(source,result.edge,F(1,4))
            self.assertIs(caught.exception,failure);self.assertEqual(final.call_count,1)
        self.assertEqual(source.packed_tables,packed);self.assertEqual((result.history.records,result.history.origins),history);qualify_result(result)
        healthy=fillet_edge(source,result.edge,F(1,4));qualify_result(healthy);self.assertNotEqual(healthy.solid.body_id,result.solid.body_id)
        self.sampled(result)


if __name__=='__main__':unittest.main()
