"""Original-meridian complete geometry/history/native revolution truth."""
from dataclasses import fields,replace
from fractions import Fraction as F
import math
import os
import unittest
from unittest.mock import patch

TRIANGLE=((F(1,4),-F(1,4)),(F(3,4),-F(1,4)),(F(1,2),F(1,2)))
CHAMFER=tuple((F(r,256),F(a,256)) for r,a in ((64,-96),(128,-160),(192,-96),(192,96),(128,160),(64,96)))
TINY=((F(1,4),F(0)),(F(65,256),F(0)),(F(1,4),F(1,256)))


def many_sided(n=32):
    quadrant=((64,0),(63,12),(59,24),(53,36),(45,45),(36,53),(24,59),(12,63))
    points=[]
    for q in range(4):
        for x,y in quadrant:
            x,y=((x,y),(-y,x),(-x,-y),(y,-x))[q]
            points.append((F(128+x,256),F(2*y,256)))
    return tuple(points[::32//n])


def altered(value,**changes):
    result=object.__new__(type(value))
    for field in fields(value):object.__setattr__(result,field.name,changes.get(field.name,getattr(value,field.name)))
    return result


def world(result,p):
    return tuple(F(o)+F(result.profile.frame.scale)*F(x) for o,x in zip(result.profile.frame.origin,p))


class RevolutionTests(unittest.TestCase):
    def test_existing_healthy_body_then_actual_full_revolution(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import BodySampler,Candidate,SampleRequest
        body=make_body('cylinder')
        with Engine(backend='cpu',threads=1) as engine:
            with BodySampler(engine,body) as sampler:
                batch=sampler.sample(SampleRequest(body,(Candidate(2,0.,.5),)))
            self.assertEqual(batch.rows[0].native_status,0)
            self.assertEqual(batch.rows[0].position,(.25,0.,0.))
            self.assertEqual(batch.rows[0].normal,(1.,0.,0.))
        from brepgpu.extrusion import make_profile
        profile=make_profile(TRIANGLE)
        from brepgpu.revolution import revolve
        result=revolve(profile)
        self.assertIs(result.profile,profile);self.assertIs(result.solid.profile,profile)
        self.assertEqual(result.solid.classify_point((0,0,0)).label,'exterior')
        self.assertEqual(result.solid.classify_point((F(1,2),0,0)).label,'interior')

    def feature(self,points=TRIANGLE,frame=None):
        from brepgpu.body import Frame
        from brepgpu.extrusion import make_profile
        from brepgpu.revolution import revolve
        from brepgpu.revolution_oracles import qualify_result
        profile=make_profile(points,Frame() if frame is None else frame)
        self.assertEqual(tuple(v.position for v in profile.vertices),tuple((F(r),F(a),F(0)) for r,a in points))
        result=revolve(profile)
        self.assertIs(result.profile,profile);self.assertIs(result.solid.profile,profile);self.assertIs(result.solid.frame,profile.frame)
        qualify_result(result)
        return result

    def sampled(self,result,request=None):
        from brepgpu import Engine
        from brepgpu.revolution_sampling import prepare_grid,sample
        from brepgpu.revolution_oracles import qualify_samples
        request=prepare_grid(result,5) if request is None else request
        backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        with Engine(backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1'))) as engine:
            batch=sample(engine,request)
        self.assertIs(batch.result,result);self.assertIs(batch.request,request)
        self.assertEqual((batch.backend,batch.device),(backend,device))
        self.assertEqual(qualify_samples(batch)['count'],len(request.candidates))
        return batch

    def test_complete_bands_all_frames_and_exact_pi_volume(self):
        from brepgpu.body import Frame
        from brepgpu.revolution_oracles import qualify_result
        for scale in (.25,.5,1.,2.,4.):
            with self.subTest(scale=scale):
                result=self.feature(CHAMFER,Frame((16.,-16.,.125),scale));q=qualify_result(result);n=6
                self.assertEqual(tuple(q[k] for k in ('vertices','edges','coedges','loops','faces','charts')),(n,2*n,4*n,n,n,4*n))
                self.assertEqual((q['modified'],q['generated'],q['genus']),(4*n,9*n,1))
        result=self.feature();self.assertEqual(result.solid.volume.rational,0)
        # Triangle area3/16, centroid radius1/2: volume2*pi*area*radius.
        self.assertEqual(result.solid.volume.pi,F(3,16))

    def test_verified16_and_maximum32_full_history_and_cyclic_relabeling(self):
        from brepgpu.revolution_oracles import qualify_result
        for n in (16,32):
            points=many_sided(n);self.assertEqual(len(points),n)
            for i,a in enumerate(points):
                b=points[(i+1)%n]
                self.assertTrue(all((b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])>0 for j,p in enumerate(points) if j not in (i,(i+1)%n)))
            result=self.feature(points);q=qualify_result(result)
            self.assertEqual((q['history_inputs'],q['history_outputs'],q['charts']),(3*n+3,13*n,4*n))
        shifted=self.feature(points[7:]+points[:7]);self.assertEqual(result.solid.volume,shifted.solid.volume)
        self.assertEqual(set(v.position for v in result.solid.vertices),set(v.position for v in shifted.solid.vertices))
        self.assertNotEqual(result.profile.body_id,shifted.profile.body_id)

    def test_literal_material_radical_sign_branches_all_supports_and_offsets(self):
        result=self.feature();solid=result.solid
        cases=(((0,0,0),'exterior',()),((F(1,8),0,0),'exterior',()),
               ((F(1,2),0,0),'interior',()),((F(1,2),0,F(1,4)),'interior',()),
               ((F(3,4),0,F(1,4)),'exterior',()),((F(1,4),0,F(1,8)),'exterior',()),
               ((F(1,4),0,F(1,4)),'interior',()),
               ((F(1,2),-F(1,4),F(1,4)),'boundary',(1,)),
               ((F(1,4),-F(1,4),0),'boundary',(1,3)),
               ((F(3,4),-F(1,4),0),'boundary',(1,2)),
               ((F(1,2),F(1,2),0),'boundary',(2,3)),
               ((F(3,8),F(1,8),-F(1,2)),'boundary',(2,)),
               ((F(3,10),-F(1,4),-F(2,5)),'boundary',(1,)))
        for p,label,faces in cases:
            query=world(result,p);answer=solid.classify_point(query)
            self.assertIs(answer.solid,solid);self.assertEqual(answer.point,query)
            self.assertEqual((answer.label,answer.boundary_faces),(label,faces))
        tiny=F(1,1<<1074)
        for a,label in ((-F(1,4)-tiny,'exterior'),(-F(1,4)+tiny,'interior')):
            self.assertEqual(solid.classify_point((F(1,2),a,F(0))).label,label)
        for r,label in ((math.nextafter(.75,0.),'boundary'),(math.nextafter(.75,1.),'exterior')):
            answer=solid.classify_point((r,-F(1,4),0));self.assertEqual(answer.label,label)

    def test_exact_ring_seam_and_quarter_join_reader_occurrences(self):
        result=self.feature();solid=result.solid;n=3
        self.assertEqual(solid.edge_point(1,F(0)),(F(1,4),-F(1,4),F(0)))
        self.assertEqual(solid.edge_point(1,F(1)),solid.edge_point(1,F(0)))
        self.assertEqual(solid.edge_point(1,F(1,8)),(F(3,20),-F(1,4),-F(1,5)))
        for i in range(n):
            edge=solid.edges[n+i];self.assertEqual((edge.start,edge.end),(i,(i+1)%n))
            seam=[c for c in solid.coedges if c.edge_slot==n+i]
            self.assertEqual(len(seam),2);self.assertEqual({c.orientation for c in seam},{-1,1})
            self.assertEqual({solid.coedge_uv(c.id,F(1,7)) for c in seam},{(F(0),F(1,7)),(F(4),F(1,7))})
            for q in range(4):
                slot=4*i+q
                self.assertEqual(solid.classify(slot,F(1,7),F(2,3)),1)
                self.assertEqual(solid.classify(slot,F(1,7),F(0)),2)
                self.assertEqual(solid.classify(slot,F(0),F(2,3)),2 if q==0 else 1)
                self.assertEqual(solid.classify(slot,F(1),F(2,3)),2 if q==3 else 1)
                self.assertEqual(solid.classify(slot,math.nan,.5),3)
        for bad in (True,'0.5',object()):
            with self.assertRaises(TypeError):solid.classify(0,bad,F(1,2))

    def test_whole_consumed_history_exact_maps_and_foreign_source_identity(self):
        from brepgpu.boolean import EntityKey
        result=self.feature();other=self.feature();n=3
        self.assertEqual(tuple(v.position for v in result.solid.vertices),tuple(v.position for v in other.solid.vertices))
        statuses={}
        for record in result.history.records:
            statuses[record.source.kind]=record.status
            self.assertEqual(len(record.images),2 if record.source.kind=='coedge' else 1 if record.source.kind in ('vertex','edge') else 0)
        self.assertEqual(statuses,dict(vertex='modified',edge='modified',coedge='modified',loop='consumed',face='consumed',chart='consumed'))
        with self.assertRaises(KeyError):result.history.from_input(EntityKey(other.profile.body_id,1,'edge',1))
        origin=next(o for o in result.history.origins if o.role=='swept_chart')
        self.assertEqual(origin.source_t,(F(0),F(0),F(1)))
        self.assertEqual(origin.periodic,(F(0),F(1),F(0)))
        self.assertEqual(tuple(k.kind for k in origin.parents),('edge','coedge','chart'))
        end=next(o for o in result.history.origins if o.role=='ring_end')
        self.assertEqual(end.source_t,(F(1),F(0),F(0)));self.assertEqual(end.orientation,-1)

    def test_native_handedness_nonaxis_normals_actual_uv_and_tiny_edges(self):
        from brepgpu.body import Frame
        from brepgpu.body_sampling import Candidate
        from brepgpu.revolution_sampling import RevolutionRequest
        result=self.feature(CHAMFER,Frame((8.,-8.,.125),.5))
        # Source edge2 is the outer vertical wall, edge5 the inner vertical wall.
        candidates=(Candidate(8,0.,.5),Candidate(8,1.,.5),Candidate(9,0.,.5),Candidate(20,0.,.5),
                    Candidate(3,1.,.5),Candidate(1,F(1,7),F(2,3)),Candidate(0,math.nan,.5),Candidate(1,-.25,.5))
        batch=self.sampled(result,RevolutionRequest(result,candidates))
        self.assertEqual(tuple(r.trim for r in batch.rows),(2,1,1,2,2,1,3,3))
        self.assertEqual(batch.rows[0].position,(8.375,-8.,.125));self.assertEqual(batch.rows[0].normal,(1.,0.,0.))
        self.assertEqual(batch.rows[1].position,batch.rows[2].position);self.assertEqual(batch.rows[1].normal,batch.rows[2].normal)
        self.assertEqual(batch.rows[3].normal,(-1.,0.,0.))
        self.sampled(result)
        self.sampled(self.feature(many_sided()))
        for scale in (.25,4.):self.sampled(self.feature(TINY,Frame((16.,-16.,.125),scale)))

    def test_actual_abi5_ring_pairs_same_face_seams_and_full_permutations(self):
        from brepgpu.adjacency import AdjacencyLibrary
        from brepgpu.revolution_oracles import qualify_topology
        result=self.feature(CHAMFER);n=6;data=result.solid.topology_input()
        variants=(data,data.permuted(edge_order=tuple(reversed(range(2*n))),face_order=tuple(reversed(range(n))),coedge_order=tuple(range(1,4*n,2))+tuple(range(0,4*n,2))))
        library=AdjacencyLibrary();backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        for source in variants:
            actual=library.build(source,backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))
            self.assertIs(actual.input,source);q=qualify_topology(result,actual)
            self.assertEqual((q['paired'],q['same_face_pair']),(n,n))

    def test_unsupported_axis_profiles_types_and_existing_operation_guards(self):
        from brepgpu.extrusion import make_profile,extrude
        from brepgpu.revolution import revolve,UnsupportedRevolution
        from brepgpu.body import make_body
        for points in (((0,0),(.5,0),(.25,.5)),((-.25,0),(.5,0),(.25,.5)),((F(63,256),0),(.5,0),(.25,.5))):
            with self.assertRaises(UnsupportedRevolution):revolve(make_profile(points))
        for bad in (object(),None,make_body('cylinder'),extrude(make_profile(TRIANGLE),1)):
            with self.assertRaises(TypeError):revolve(bad)
        result=self.feature()
        from brepgpu.body_sampling import SampleRequest
        from brepgpu.extrusion_sampling import ExtrusionRequest
        with self.assertRaises(TypeError):SampleRequest(result.solid,())
        with self.assertRaises(TypeError):ExtrusionRequest(result,())
        with self.assertRaises((TypeError,ValueError)):result.solid.classify_point((True,0,0))
        with self.assertRaises((TypeError,ValueError)):result.solid.classify_point((math.inf,0,0))
        with self.assertRaises((TypeError,ValueError)):result.solid.edge_point(1,True)

    def test_falsified_controls_maps_seams_typed_alias_and_consistent_wrong_ancestry(self):
        from brepgpu.revolution_oracles import qualify_result
        result=self.feature();solid=result.solid;bad=[]
        patches=list(solid.patches);controls=list(patches[0].controls);controls[2]=F(1,256)
        patches[0]=replace(patches[0],controls=controls);bad.append(altered(result,solid=altered(solid,patches=tuple(patches))))
        bad.append(altered(result,solid=altered(solid,faces=solid.faces[:-1])))
        c=solid.coedges[1];mapping=replace(c.uv_map,data=tuple(F(v) for v in (0,0,0,1)))
        bad.append(altered(result,solid=altered(solid,coedges=solid.coedges[:1]+(replace(c,uv_map=mapping),)+solid.coedges[2:])))
        bad.append(altered(result,solid=altered(solid,revision=True)))
        bad.append(altered(result,solid=altered(solid,loops=(replace(solid.loops[0],coedge_slots=(False,1,2,3)),)+solid.loops[1:])))
        origins=list(result.history.origins);i=next(i for i,o in enumerate(origins) if o.role=='swept_face');original=origins[i]
        for changed in (replace(original,source_t=(F(0),F(1),F(0))),altered(original,orientation=True)):
            values=list(origins);values[i]=changed;bad.append(altered(result,history=altered(result.history,origins=tuple(values))))
        # A fully reciprocal wrong edge parent must still fail the literal role.
        parents=list(original.parents);old=parents[0];parents[0]=replace(old,entity_id=2)
        origins[i]=replace(original,parents=tuple(parents));records=[]
        for record in result.history.records:
            generated=record.generated
            if record.source==old:generated=tuple(k for k in generated if k!=original.target)
            if record.source==parents[0]:generated=generated+(original.target,)
            records.append(replace(record,generated=generated))
        bad.append(altered(result,history=altered(result.history,origins=tuple(origins),records=tuple(records))))
        for value in bad:
            with self.assertRaises(AssertionError):qualify_result(value)

    def test_falsified_native_output_and_late_publication_rollback_retry(self):
        from brepgpu.revolution import revolve
        from brepgpu.revolution_oracles import qualify_samples,qualify_result
        result=self.feature();batch=self.sampled(result);rows=list(batch.rows);index=next(i for i,r in enumerate(rows) if r.patch_slot==4 and r.native_status==0);row=rows[index]
        for changed in (replace(row,normal=tuple(-v for v in row.normal)),replace(row,position=(row.position[0]+.01,*row.position[1:])),replace(row,u=math.nextafter(row.u,math.inf)),replace(row,face_id=1)):
            values=list(rows);values[index]=changed
            with self.assertRaises(AssertionError):qualify_samples(altered(batch,rows=tuple(values)))
        profile=result.profile;before=tuple(getattr(profile,n) for n in ('vertices','curves','edges','coedges','loops','faces','charts','patches'));history=(result.history.records,result.history.origins)
        sentinel=RuntimeError('independent late revolution failure')
        with patch('brepgpu.revolution._validate_candidate',side_effect=sentinel) as boundary:
            with self.assertRaises(RuntimeError) as caught:revolve(profile)
            self.assertIs(caught.exception,sentinel);self.assertEqual(boundary.call_count,1)
        self.assertEqual(tuple(getattr(profile,n) for n in ('vertices','curves','edges','coedges','loops','faces','charts','patches')),before)
        self.assertEqual((result.history.records,result.history.origins),history);qualify_result(result)
        healthy=revolve(profile);qualify_result(healthy);self.assertNotEqual(healthy.solid.body_id,result.solid.body_id)
        self.sampled(result)


if __name__=='__main__':unittest.main()
