"""Complete independent sharp cylinder offset/source-foot/native contracts."""
from dataclasses import fields,replace
from fractions import Fraction as F
import math
import os
import struct
import unittest
from unittest.mock import patch


def altered(value,**changes):
    answer=object.__new__(type(value))
    for field in fields(value):object.__setattr__(answer,field.name,changes.get(field.name,getattr(value,field.name)))
    return answer


def world(result,local):return tuple(F(o)+F(result.source.frame.scale)*F(x) for o,x in zip(result.source.frame.origin,local))


class OffsetTests(unittest.TestCase):
    def feature(self,distance=F(1,8),frame=None):
        from brepgpu.body import Frame,make_body
        from brepgpu.offset import offset_solid
        from brepgpu.offset_oracles import qualify_result
        source=make_body('cylinder',Frame() if frame is None else frame)
        result=offset_solid(source,distance)
        self.assertIs(result.source,source);self.assertIs(result.solid.source,source);self.assertIs(result.solid.frame,source.frame)
        self.assertEqual((result.distance,result.solid.distance,result.join,result.solid.join),(F(distance),F(distance),'intersection','intersection'))
        quality=qualify_result(result)
        self.assertEqual(tuple(quality[k] for k in ('vertices','edges','coedges','loops','faces','charts','genus')),(2,3,6,3,3,6,0))
        self.assertEqual(tuple(quality[k] for k in ('history_inputs','history_outputs','modified','retained','deleted','generated')),(23,23,23,0,0,0))
        return result

    def sampled(self,result,request=None):
        from brepgpu import Engine
        from brepgpu.offset_sampling import prepare_grid,sample
        from brepgpu.offset_oracles import qualify_samples
        request=prepare_grid(result,5) if request is None else request
        backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        with Engine(backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1'))) as engine:batch=sample(engine,request)
        self.assertIs(batch.result,result);self.assertIs(batch.request,request);self.assertEqual((batch.backend,batch.device),(backend,device))
        self.assertEqual(qualify_samples(batch)['count'],len(request.candidates))
        return batch

    def test_existing_healthy_cylinder_then_actual_signed_offset(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import BodySampler,Candidate,SampleRequest
        source=make_body('cylinder')
        with Engine(backend='cpu',threads=1) as engine:
            with BodySampler(engine,source) as sampler:batch=sampler.sample(SampleRequest(source,(Candidate(2,.5,.5),)))
            self.assertEqual(batch.rows[0].native_status,0)
            for a,b in zip(batch.rows[0].position,(.15,.2,0.)):self.assertAlmostEqual(a,b,delta=2.**-36)
        from brepgpu.offset import offset_solid
        result=offset_solid(source,F(1,8))
        self.assertIs(result.source,source)
        self.assertEqual(result.solid.classify_point((F(3,8),0,F(5,8))).boundary_faces,(1,3))

    def test_complete_signed_family_frames_original_bits_and_exact_volume(self):
        from brepgpu.body import Frame
        for distance in (-F(1,8),-F(17,256),-F(1,256),F(1,256),F(17,256),F(1,8)):
            result=self.feature(distance);R=F(1,4)+distance;H=F(1,2)+distance
            self.assertEqual((result.solid.volume.rational,result.solid.volume.pi),(F(0),2*H*R*R))
            self.assertEqual(tuple(v.position for v in result.solid.vertices),((R,F(0),-H),(R,F(0),H)))
            self.assertEqual(tuple((e.start,e.end) for e in result.solid.edges),((0,0),(1,1),(0,1)))
            self.assertEqual(result.solid.loops[2].coedge_slots,(2,3,4,5))
        for scale in (.25,.5,1.,2.,4.):
            result=self.feature(F(17,256),Frame((16.,-16.,.125),scale))
            self.assertEqual(result.solid.volume.pi,2*(F(1,2)+F(17,256))*(F(1,4)+F(17,256))**2*F(scale)**3)
            for old,new in zip(result.source.patches,result.solid.patches):
                for name in ('origin','uv'):self.assertEqual(tuple(struct.pack('<d',v) for v in getattr(old,name)),tuple(struct.pack('<d',v) for v in getattr(new,name)))
                self.assertEqual(struct.pack('<d',old.scale),struct.pack('<d',new.scale))

    def test_literal_complete_material_supports_sharp_rim_and_inward_erosion(self):
        result=self.feature();solid=result.solid
        cases=(((0,0,0),'interior',()),((0,0,F(5,8)),'boundary',(1,)),((0,0,-F(5,8)),'boundary',(2,)),
               ((F(3,8),0,0),'boundary',(3,)),((F(9,40),F(3,10),0),'boundary',(3,)),
               ((F(9,40),F(3,10),F(5,8)),'boundary',(1,3)),((F(3,8),0,-F(5,8)),'boundary',(2,3)),
               ((F(3,8),F(3,8),0),'exterior',()),((0,0,F(3,4)),'exterior',()))
        for point,label,faces in cases:
            query=world(result,point);answer=solid.classify_point(query)
            self.assertIs(answer.solid,solid);self.assertEqual(answer.point,query);self.assertEqual((answer.label,answer.boundary_faces),(label,faces))
        # The sharp new rim exceeds the Euclidean dilation distance: squared distance=2*d².
        rim=(F(3,8),0,F(5,8));self.assertEqual(solid.classify_point(rim).label,'boundary')
        self.assertGreater((rim[0]-F(1,4))**2+(rim[2]-F(1,2))**2,result.distance**2)
        tiny=F(1,1<<1074)
        for delta,label in ((tiny,'exterior'),(-tiny,'interior')):self.assertEqual(solid.classify_point((F(9,40)+delta,F(3,10),0)).label,label)
        for z,label in ((math.nextafter(.625,math.inf),'exterior'),(math.nextafter(.625,0.),'interior')):self.assertEqual(solid.classify_point((0,0,z)).label,label)
        inward=self.feature(-F(1,8));r=F(1,8);h=F(3,8)
        self.assertEqual(inward.solid.classify_point((r,0,h)).boundary_faces,(1,3))
        self.assertEqual(min(F(1,4)-r,F(1,2)-h),-inward.distance)
        self.assertEqual(inward.solid.classify_point((F(3,16),0,0)).label,'exterior')

    def test_exact_trim_boundaries_rounding_and_internal_chart_joins(self):
        result=self.feature(F(17,256));solid=result.solid
        self.assertEqual(solid.classify(0,F(4,5),F(9,10)),2)
        for u in (float(F(4,5)),math.nextafter(float(F(4,5)),0.),math.nextafter(float(F(4,5)),1.)):
            v=float(F(9,10));q=(2*F(u)-1)**2+(2*F(v)-1)**2
            self.assertEqual(solid.classify(0,u,v),0 if q>1 else 2 if q==1 else 1)
        self.assertEqual(solid.classify(0,0,0),0);self.assertEqual(solid.classify(0,F(1,2),F(1,2)),1)
        for q in range(4):
            self.assertEqual(solid.classify(2+q,F(1,2),0),2)
            self.assertEqual(solid.classify(2+q,F(1,2),1),2)
            self.assertEqual(solid.classify(2+q,0,F(1,2)),2 if q==0 else 1)
            self.assertEqual(solid.classify(2+q,1,F(1,2)),2 if q==3 else 1)
        for invalid in (-1,2,math.inf,math.nan):self.assertEqual(solid.classify(2,invalid,.5),3)

    def test_source_foot_exact_extensions_disk_vs_square_and_signed_normal_identity(self):
        result=self.feature();solid=result.solid
        cases=((0,F(3,4),F(3,4),(F(7,8),F(7,8)),True,0),
               (0,F(1),F(1,2),(F(5,4),F(1,2)),False,3),
               (2,F(1,2),F(0),(F(1,2),-F(1,8)),False,3),
               (2,F(1,2),F(1,2),(F(1,2),F(1,2)),True,1),
               (5,F(1),F(1,2),(F(4),F(1,2)),True,2))
        for slot,u,v,uv,inside,trim in cases:
            foot=solid.source_foot(slot,u,v)
            self.assertIs(foot.solid,solid);self.assertEqual((foot.chart_id,foot.u,foot.v),(slot+1,u,v))
            self.assertEqual((foot.source_uv,foot.in_source_square,foot.source_trim),(uv,inside,trim))
            self.assertEqual((foot.source_face.body_id,foot.source_chart.body_id),(result.source.body_id,result.source.body_id))
            self.assertEqual(foot.source_chart.entity_id,slot+1)
            self.assertEqual(foot.distance,result.distance)
            self.assertEqual(foot.point,tuple(a+foot.distance*n for a,n in zip(foot.source_point,foot.normal)))
            self.assertEqual(sum(n*n for n in foot.normal),1)
        inward=self.feature(-F(1,8))
        self.assertEqual(inward.solid.source_foot(0,1,F(1,2)).source_trim,1)
        self.assertEqual(inward.solid.source_foot(2,F(1,2),0).source_trim,1)
        for args in ((0,0,0),(2,-1,.5),(0,math.nan,.5)):
            with self.assertRaises((TypeError,ValueError)):solid.source_foot(*args)
        # Both incident carrier feet at a sharp rim satisfy their own identity.
        cap=solid.source_foot(0,1,F(1,2));wall=solid.source_foot(2,0,1)
        self.assertEqual(cap.point,wall.point);self.assertNotEqual(cap.source_point,wall.source_point)
        self.assertEqual((cap.normal,wall.normal),((F(0),F(0),F(1)),(F(1),F(0),F(0))))

    def test_complete_history_original_map_owners_and_full_ring_seam_parameters(self):
        from brepgpu.boolean import EntityKey
        result=self.feature();solid=result.solid
        for record in result.history.records:
            self.assertEqual((record.status,len(record.images),record.generated),('modified',1,()))
            origin=result.history.origin(record.images[0]);self.assertEqual((origin.parents,origin.relation,origin.role),((record.source,),'modified','source_'+record.source.kind))
            self.assertEqual(result.history.from_input(record.source),record)
            if record.source.kind in ('edge','coedge'):
                self.assertEqual((origin.source_t_owner,origin.source_t_relation,origin.source_t),(record.source,'boundary_parameter',(F(0),F(1),F(0))))
        def origin(kind,id):return result.history.origin(EntityKey(solid.body_id,solid.revision,kind,id))
        cap=origin('chart',1);self.assertEqual((cap.source_uv_owner.kind,cap.source_uv_owner.entity_id,cap.source_uv_relation),('chart',1,'normal_foot'))
        self.assertEqual(cap.source_uv.evaluate(F(3,4),F(3,4)),(F(7,8),F(7,8)))
        wall=origin('chart',6);self.assertEqual(wall.source_uv.evaluate(F(1,2),F(0)),(F(7,2),-F(1,8)))
        ring=origin('coedge',1);self.assertEqual(ring.source_uv.evaluate(F(0),F(0)),(F(5,4),F(1,2)))
        for id,tau in ((4,4),(6,0)):
            item=origin('coedge',id);self.assertEqual(item.source_uv.evaluate(F(1,3),F(0)),(F(tau),F(7,24)))
        self.assertEqual(solid.edge_point(1,F(1,8)),(F(9,40),F(3,10),-F(5,8)))
        self.assertEqual(solid.edge_point(1,0),solid.edge_point(1,1))
        self.assertEqual(solid.coedge_uv(4,F(1,3)),(F(4),F(1,3)))
        self.assertEqual(solid.coedge_uv(6,F(1,3)),(F(0),F(1,3)))
        other=self.feature()
        with self.assertRaises(KeyError):result.history.from_input(EntityKey(other.source.body_id,1,'face',1))

    def test_actual_native_nonaxis_normals_copied_query_bits_and_extreme_frames(self):
        from brepgpu.body import Frame
        from brepgpu.body_sampling import Candidate
        from brepgpu.offset_sampling import OffsetRequest
        result=self.feature();u=F(4,5);v=F(9,10)
        candidates=tuple(Candidate(2+q,.5,.5) for q in range(4))+(Candidate(0,u,v),Candidate(0,0.,0.),Candidate(2,math.nan,.5),Candidate(2,-1.,.5))
        request=OffsetRequest(result,candidates);batch=self.sampled(result,request)
        for row,normal in zip(batch.rows[:4],((.6,.8,0.),(-.8,.6,0.),(-.6,-.8,0.),(.8,-.6,0.))):
            for actual,expected in zip(row.normal,normal):self.assertAlmostEqual(actual,expected,delta=2.**-30)
        self.assertEqual(struct.pack('<d',batch.rows[4].u),struct.pack('<d',float(u)))
        q=(2*F(float(u))-1)**2+(2*F(float(v))-1)**2
        self.assertEqual(batch.rows[4].trim,0 if q>1 else 2 if q==1 else 1)
        self.assertEqual(tuple(r.trim for r in batch.rows[-3:]),(0,3,3))
        for d in (-F(1,8),F(17,256)):
            for scale in (.25,4.):self.sampled(self.feature(d,Frame((16.,-16.,.125),scale)))

    def test_actual_abi5_closed_ring_and_sameface_seam_full_permutations(self):
        from brepgpu.adjacency import AdjacencyLibrary
        from brepgpu.offset_oracles import qualify_topology
        result=self.feature(-F(17,256));data=result.solid.topology_input();library=AdjacencyLibrary()
        variants=(data,data.permuted(edge_order=(2,0,1),face_order=(2,1,0),coedge_order=(5,2,0,4,1,3)))
        for source in variants:
            actual=library.build(source,backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu'),device=int(os.environ.get('BREPGPU_TEST_DEVICE','0')),threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))
            self.assertIs(actual.input,source);q=qualify_topology(result,actual)
            self.assertEqual((q['paired'],q['same_face_pair']),(2,1))
            seam=next(i for i,e in enumerate(source.edge_sources) if e.entity_id==3)
            self.assertEqual(actual.edge_kind(seam),3)
            uses=actual.edge_uses(seam);self.assertEqual({u.entity_id for u in uses},{4,6})
            for use in uses:
                slot=source.coedge_sources.index(use);self.assertEqual(actual.neighbor(slot).entity_id,3)
                self.assertEqual(actual.mate(slot).entity_id,6 if use.entity_id==4 else 4)

    def test_distance_source_admission_and_existing_operation_guards(self):
        from brepgpu.body import make_body
        from brepgpu.offset import offset_solid
        source=make_body('cylinder');before=source.packed_tables
        for d in (0,F(1,257),F(33,256),-F(33,256),-F(1,4),True,math.inf,math.nan,'0.125'):
            with self.assertRaises((TypeError,ValueError)):offset_solid(source,d)
            self.assertEqual(source.packed_tables,before)
        for source in (make_body('box'),make_body('plate'),object()):
            with self.assertRaises((TypeError,ValueError)):offset_solid(source,F(1,8))
        result=self.feature()
        from brepgpu.body_sampling import SampleRequest
        from brepgpu.fillet_sampling import FilletRequest
        with self.assertRaises(TypeError):SampleRequest(result.solid,())
        with self.assertRaises(TypeError):FilletRequest(result,())
        with self.assertRaises((TypeError,ValueError)):result.solid.classify_point((True,0,0))

    def test_falsified_geometry_typed_source_and_reciprocal_wrong_ancestry_rejected(self):
        from brepgpu.offset_oracles import qualify_result
        result=self.feature();solid=result.solid;bad=[]
        patches=list(solid.patches);values=list(patches[2].controls);values[0]+=1/256;patches[2]=replace(patches[2],controls=values)
        bad.append(altered(result,solid=altered(solid,patches=tuple(patches))))
        bad.append(altered(result,solid=altered(solid,revision=True)))
        vertices=list(result.source.vertices);vertices[0]=replace(vertices[0],position=tuple(map(F,vertices[0].position)))
        sourcealias=altered(result.source,vertices=tuple(vertices));bad.append(altered(result,source=sourcealias,solid=altered(solid,source=sourcealias)))
        loops=list(solid.loops);loops[0]=replace(loops[0],coedge_slots=(False,));bad.append(altered(result,solid=altered(solid,loops=tuple(loops))))
        origins=list(result.history.origins);i=next(i for i,o in enumerate(origins) if o.target.kind=='chart' and o.target.entity_id==1);old=origins[i]
        changed=list(origins);changed[i]=altered(old,orientation=True);bad.append(altered(result,history=altered(result.history,origins=tuple(changed))))
        changed=list(origins);changed[i]=replace(old,source_uv_owner=replace(old.source_uv_owner,entity_id=2));bad.append(altered(result,history=altered(result.history,origins=tuple(changed))))
        # Swap complete reciprocal vertex histories; graph/reciprocity alone still pass.
        a,b=0,1;oa,ob=origins[a],origins[b];origins[a]=replace(oa,parents=ob.parents);origins[b]=replace(ob,parents=oa.parents)
        records=[]
        for r in result.history.records:
            images=(ob.target,) if r.source==oa.parents[0] else (oa.target,) if r.source==ob.parents[0] else r.images
            records.append(replace(r,images=images))
        bad.append(altered(result,history=altered(result.history,origins=tuple(origins),records=tuple(records))))
        for value in bad:
            with self.assertRaises(AssertionError):qualify_result(value)

    def test_falsified_source_foot_owner_extension_label_and_public_readers_rejected(self):
        from brepgpu.offset import OffsetSolid
        from brepgpu.offset_oracles import qualify_result
        result=self.feature();method=OffsetSolid.source_foot
        for field in ('source_face','source_trim','normal'):
            def false_foot(solid,*args,field=field):
                foot=method(solid,*args)
                if field=='source_face':return altered(foot,source_face=replace(foot.source_face,entity_id=2 if foot.source_face.entity_id!=2 else 1))
                if field=='source_trim':return altered(foot,source_trim=1 if foot.source_trim!=1 else 0)
                return altered(foot,normal=tuple(-v for v in foot.normal))
            with patch.object(OffsetSolid,'source_foot',false_foot):
                with self.assertRaises(AssertionError):qualify_result(result)
        for method_name in ('edge_point','coedge_uv'):
            original=getattr(OffsetSolid,method_name)
            def shifted(solid,*args,original=original):
                value=original(solid,*args);return (value[0]+F(1,256),)+value[1:]
            with patch.object(OffsetSolid,method_name,shifted):
                with self.assertRaises(AssertionError):qualify_result(result)

    def test_falsified_native_rows_and_late_publication_rollback_retry(self):
        from brepgpu.offset import offset_solid
        from brepgpu.offset_oracles import qualify_result,qualify_samples
        result=self.feature();batch=self.sampled(result);rows=list(batch.rows);index=next(i for i,r in enumerate(rows) if r.patch_slot==2 and r.native_status==0);row=rows[index]
        for changed in (replace(row,normal=tuple(-v for v in row.normal)),replace(row,position=(row.position[0]+.01,*row.position[1:])),replace(row,u=math.nextafter(row.u,math.inf)),replace(row,native_status=1),replace(row,face_id=1)):
            values=list(rows);values[index]=changed
            with self.assertRaises(AssertionError):qualify_samples(altered(batch,rows=tuple(values)))
        source=result.source;before=source.packed_tables;history=(result.history.records,result.history.origins);sentinel=RuntimeError('independent late offset publication failure')
        with patch('brepgpu.offset._validate_candidate',side_effect=sentinel) as boundary:
            with self.assertRaises(RuntimeError) as caught:offset_solid(source,-F(1,8))
            self.assertIs(caught.exception,sentinel);self.assertEqual(boundary.call_count,1)
        self.assertEqual(source.packed_tables,before);self.assertEqual((result.history.records,result.history.origins),history);qualify_result(result)
        healthy=offset_solid(source,-F(1,8));qualify_result(healthy);self.assertNotEqual(healthy.solid.body_id,result.solid.body_id)
        self.sampled(result)


if __name__=='__main__':unittest.main()
