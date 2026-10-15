"""Independent complete convex-profile extrusion qualification."""
from fractions import Fraction as F
from dataclasses import fields,replace
import math
import os
import unittest
from unittest.mock import patch


TRIANGLE=((-F(1,2),-F(1,4)),(F(1,2),-F(1,4)),(F(1,4),F(1,2)))
CHAMFER=tuple((F(x,256),F(y,256)) for x,y in ((-160,-112),(96,-160),(176,-64),(144,128),(-80,176),(-176,64)))


def many_sided():
    quadrant=((192,0),(188,37),(177,73),(160,107),(136,136),(107,160),(73,177),(37,188))
    points=[]
    for q in range(4):
        for x,y in quadrant:
            xy=((x,y),(-y,x),(-x,-y),(y,-x))[q]
            points.append(tuple(F(v,256) for v in xy))
    return tuple(points)


def altered(value,**changes):
    result=object.__new__(type(value))
    for field in fields(value):object.__setattr__(result,field.name,changes.get(field.name,getattr(value,field.name)))
    return result


class ExtrusionTests(unittest.TestCase):
    def test_existing_healthy_body_then_actual_extrusion(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import BodySampler,Candidate,SampleRequest
        body=make_body('box')
        with Engine(backend='cpu',threads=1) as engine:
            with BodySampler(engine,body) as sampler:
                batch=sampler.sample(SampleRequest(body,(Candidate(0,0.,0.),)))
            self.assertEqual(batch.rows[0].native_status,0)
            self.assertEqual(batch.rows[0].position,(-.75,-.75,.5))
        from brepgpu.extrusion import make_profile,extrude
        points=((-F(1,2),-F(1,4)),(F(1,2),-F(1,4)),(F(1,4),F(1,2)))
        profile=make_profile(points);result=extrude(profile,F(3,4))
        self.assertIs(result.profile,profile);self.assertIs(result.solid.profile,profile)
        self.assertEqual(result.height,F(3,4))
        self.assertEqual(result.solid.classify_point((0,0,0)).label,'interior')

    def feature(self,points=TRIANGLE,height=F(3,4),frame=None):
        from brepgpu.extrusion import make_profile,extrude
        from brepgpu.extrusion_oracles import qualify_profile,qualify_result
        from brepgpu.body import Frame
        profile=make_profile(points,frame=Frame() if frame is None else frame)
        self.assertEqual(tuple(v.position for v in profile.vertices),tuple((F(x),F(y),F(0)) for x,y in points))
        qualify_profile(profile)
        result=extrude(profile,height)
        self.assertIs(result.profile,profile);self.assertIs(result.solid.profile,profile)
        self.assertEqual(result.height,F(height));qualify_result(result)
        return result

    def sampled(self,result,request=None):
        from brepgpu import Engine
        from brepgpu.extrusion_sampling import prepare_grid,sample
        from brepgpu.extrusion_oracles import qualify_samples
        backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        request=prepare_grid(result,5) if request is None else request
        with Engine(backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1'))) as engine:
            batch=sample(engine,request)
        self.assertIs(batch.result,result);self.assertIs(batch.request,request)
        self.assertEqual((batch.backend,batch.device),(backend,device))
        self.assertEqual(qualify_samples(batch)['count'],len(request.candidates))
        return batch

    def test_complete_nonrectangular_families_all_frames_and_exact_extremes(self):
        from brepgpu.body import Frame
        from brepgpu.extrusion_oracles import qualify_result
        for scale in (.25,.5,1.,2.,4.):
            with self.subTest(scale=scale):
                result=self.feature(CHAMFER,F(511,256),Frame((16.,-16.,.125),scale))
                q=qualify_result(result);n=6
                self.assertEqual(tuple(q[k] for k in ('vertices','edges','coedges','loops','faces','charts')),(2*n,3*n,6*n,n+2,n+2,n+2))
                self.assertEqual((q['modified'],q['generated'],q['genus']),(6*n+6,8*n,0))
        self.feature(((F(0),F(0)),(F(1,256),F(0)),(F(0),F(1,256))),F(1,4))
        self.feature(((-F(3,4),-F(3,4)),(F(3,4),-F(3,4)),(F(3,4),F(3,4)),(-F(3,4),F(3,4))),F(2),Frame(scale=4.))

    def test_verified_maximum32_and_cyclic_source_order_complete_history(self):
        from brepgpu.extrusion_oracles import qualify_result
        points=many_sided();self.assertEqual(len(points),32)
        for i,a in enumerate(points):
            b=points[(i+1)%32]
            self.assertTrue(all((b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])>0 for j,p in enumerate(points) if j not in (i,(i+1)%32)))
        result=self.feature(points,F(257,256));q=qualify_result(result)
        self.assertEqual((q['vertices'],q['edges'],q['coedges'],q['faces'],q['history_inputs'],q['history_outputs']),(64,96,192,34,99,454))
        shifted=self.feature(points[7:]+points[:7],F(257,256))
        self.assertEqual(result.solid.volume,shifted.solid.volume)
        self.assertEqual(set(v.position for v in result.solid.vertices),set(v.position for v in shifted.solid.vertices))
        self.assertNotEqual(result.profile.body_id,shifted.profile.body_id)

    def test_all_material_supports_exact_offsets_and_nondyadic_trim_readers(self):
        result=self.feature();solid=result.solid;h=result.height
        # First side is horizontal y=-1/4: its midpoint at the lower cap has
        # both supports, while a vertex has the cap and both adjacent sides.
        cases=(((F(0),-F(1,4),-h/2),'boundary',(1,3)),
               ((-F(1,2),-F(1,4),h/2),'boundary',(2,3,5)),
               ((F(0),F(0),F(0)),'interior',()),
               ((F(0),F(0),h/2+F(1,1<<1074)),'exterior',()))
        for query,label,faces in cases:
            answer=solid.classify_point(query)
            self.assertIs(answer.solid,solid);self.assertEqual(answer.point,query)
            self.assertEqual((answer.label,answer.boundary_faces),(label,faces))
        a,b=TRIANGLE[1],TRIANGLE[2];t=F(1,7)
        point=tuple(x+t*(y-x) for x,y in zip(a,b));uv=tuple((v+1)/2 for v in point)
        self.assertEqual(result.profile.classify(*uv),2)
        self.assertEqual(solid.classify(0,*uv),2)
        self.assertEqual(solid.edge_point(2,t),(*point,-h/2))
        lower=next(c for c in solid.coedges if c.edge_slot==1 and c.face_slot==0)
        self.assertEqual(solid.coedge_uv(lower.id,t),uv)
        self.assertEqual(lower.orientation,-1)
        for slot in range(2,5):
            self.assertEqual(solid.classify(slot,F(1,7),F(2,3)),1)
            self.assertEqual(solid.classify(slot,F(0),F(2,3)),2)
            self.assertEqual(solid.classify(slot,F(-1,256),F(2,3)),3)
        for u in (math.nextafter(float(uv[0]),-math.inf),math.nextafter(float(uv[0]),math.inf)):
            self.assertIn(solid.classify(0,u,float(uv[1])),(0,1,2))

    def test_exact_affine_history_and_geometry_equal_source_identity(self):
        from brepgpu.extrusion import extrude,make_profile,EntityKey
        from brepgpu.extrusion_oracles import qualify_result
        result=self.feature();other=make_profile(TRIANGLE);second=extrude(other,result.height);qualify_result(second)
        self.assertEqual(tuple(v.position for v in result.solid.vertices),tuple(v.position for v in second.solid.vertices))
        self.assertNotEqual(result.profile.body_id,other.body_id)
        for history,foreign in ((result.history,other),(second.history,result.profile)):
            with self.assertRaises(KeyError):history.from_input(EntityKey(foreign.body_id,1,'edge',1))
        n=len(TRIANGLE);origins=result.history.origins
        vertical=next(o for o in origins if o.target.kind=='edge' and o.target.entity_id==2*n+1)
        self.assertEqual((vertical.role,vertical.axial,vertical.source_t),('swept_vertex',(-result.height/2,result.height,F(0)),None))
        side=next(o for o in origins if o.target.kind=='face' and o.target.entity_id==3)
        self.assertEqual(side.source_t,(F(0),F(1),F(0)))
        self.assertEqual(side.axial,(-result.height/2,F(0),result.height))
        self.assertEqual(tuple(k.kind for k in side.parents),('edge','coedge'))

    def test_actual_native_nonaxis_normals_copied_float_uv_and_complete_grid(self):
        from brepgpu.body import Frame
        from brepgpu.body_sampling import Candidate
        from brepgpu.extrusion_sampling import ExtrusionRequest
        result=self.feature(TRIANGLE,F(3,4),Frame((8.,-8.,.125),.5))
        candidates=(Candidate(0,.5,.5),Candidate(1,.5,.5),Candidate(3,F(1,7),F(2,3)),
                    Candidate(4,0.,0.),Candidate(0,0.,0.),Candidate(3,math.nan,.5),
                    Candidate(2,.5,.5),Candidate(3,F(1,7),F(2,3)))
        batch=self.sampled(result,ExtrusionRequest(result,candidates))
        self.assertEqual(tuple(row.trim for row in batch.rows),(1,1,1,2,0,3,1,1))
        self.assertEqual(batch.rows[0].normal,(0.,0.,-1.));self.assertEqual(batch.rows[1].normal,(0.,0.,1.))
        self.assertEqual(batch.rows[6].normal,(0.,-1.,0.))
        self.assertEqual(batch.rows[2].position,batch.rows[7].position)
        self.sampled(result)
        self.sampled(self.feature(many_sided(),F(1,4)))
        tiny=((F(0),F(0)),(F(1,256),F(0)),(F(0),F(1,256)))
        for scale in (.25,4.):
            self.sampled(self.feature(tiny,F(1,4),Frame((16.,-16.,.125),scale)))

    def test_actual_abi5_closed_mates_under_full_slot_permutations(self):
        from brepgpu.adjacency import AdjacencyLibrary
        from brepgpu.extrusion_oracles import qualify_topology
        result=self.feature(CHAMFER);data=result.solid.topology_input();n=len(CHAMFER)
        variants=(data,data.permuted(edge_order=tuple(reversed(range(3*n))),face_order=tuple(reversed(range(n+2))),coedge_order=tuple(range(1,6*n,2))+tuple(range(0,6*n,2))))
        library=AdjacencyLibrary();backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        for source in variants:
            actual=library.build(source,backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))
            self.assertIs(actual.input,source);self.assertEqual(qualify_topology(result,actual)['paired'],3*n)

    def test_invalid_profiles_heights_hostile_winding_star_and_bounded_iterators(self):
        from brepgpu.extrusion import make_profile,extrude,UnsupportedExtrusion
        pentagon=tuple((F(x,256),F(y,256)) for x,y in ((0,192),(-183,59),(-113,-155),(113,-155),(183,59)))
        star=tuple(pentagon[i] for i in (0,2,4,1,3))
        self.assertTrue(all((star[(i+1)%5][0]-star[i][0])*(star[(i+2)%5][1]-star[(i+1)%5][1])-(star[(i+1)%5][1]-star[i][1])*(star[(i+2)%5][0]-star[(i+1)%5][0])>0 for i in range(5)))
        invalid=((),TRIANGLE[:2],TRIANGLE[::-1],TRIANGLE+(TRIANGLE[0],),star,
                 ((0,0),(F(1,2),0),(F(1,4),0)),((0,0),(1,0),(0,1)),
                 ((0,0),(F(1,257),0),(0,F(1,2))),((0,0),(math.inf,0),(0,.5)),
                 ((False,0),(.5,0),(0,.5)),((0,0,0),(.5,0),(0,.5)),((0,0),(.5,0),(0,)),
                 ((0,0),(.5,0),(.25,.125),(0,.5)))
        for points in invalid:
            with self.assertRaises((TypeError,ValueError)):make_profile(points)
        consumed=[]
        def endless():
            while True:consumed.append(1);yield (0,0)
        with self.assertRaises((TypeError,ValueError)):make_profile(endless())
        self.assertLessEqual(len(consumed),33)
        coords=[]
        def badvertex():
            while True:coords.append(1);yield 0
        with self.assertRaises((TypeError,ValueError)):make_profile((badvertex(),(.5,0),(0,.5)))
        self.assertLessEqual(len(coords),3)
        sentinel=RuntimeError('independent source iterator failure')
        def broken():
            yield (0,0)
            raise sentinel
        with self.assertRaises(RuntimeError) as caught:make_profile(broken())
        self.assertIs(caught.exception,sentinel)
        profile=make_profile(TRIANGLE)
        for height in (0,-1,F(1,8),F(513,256),F(1,257),math.nan,math.inf,True,'1'):
            with self.assertRaises((TypeError,ValueError)):extrude(profile,height)
        with self.assertRaises(TypeError):extrude(object(),1)
        for invalidvalue in (True,'0.5',object()):
            with self.assertRaises(TypeError):profile.classify(invalidvalue,F(1,2))

    def test_falsified_complete_geometry_typed_alias_and_history_maps_rejected(self):
        from brepgpu.extrusion_oracles import qualify_result
        result=self.feature();solid=result.solid;bad=[]
        patches=list(solid.patches);controls=list(patches[2].controls);controls[0]+=1/256
        patches[2]=replace(patches[2],controls=tuple(controls));bad.append(altered(result,solid=altered(solid,patches=tuple(patches))))
        bad.append(altered(result,solid=altered(solid,faces=solid.faces[:-1])))
        face=solid.faces[0];loop=solid.loops[0];use=solid.coedges[0]
        bad.extend(altered(result,solid=s) for s in (
            altered(solid,revision=True),altered(solid,height=float(solid.height)),
            altered(solid,faces=(replace(face,orientation=1),)+solid.faces[1:]),
            altered(solid,faces=(replace(face,loop_slots=(False,)),)+solid.faces[1:]),
            altered(solid,loops=(replace(loop,coedge_slots=(False,)+loop.coedge_slots[1:]),)+solid.loops[1:]),
            altered(solid,coedges=(replace(use,orientation=1),)+solid.coedges[1:]),
            altered(solid,vertices=(replace(solid.vertices[0],position=tuple(map(float,solid.vertices[0].position))),)+solid.vertices[1:])))
        origins=list(result.history.origins);index=next(i for i,o in enumerate(origins) if o.role=='side' and o.target.kind=='face')
        old=origins[index]
        for changed in (replace(old,source_t=(F(0),F(0),F(1))),replace(old,parents=old.parents[:1]),altered(old,orientation=True),replace(old,axial=(F(0),F(0),result.height))):
            values=list(origins);values[index]=changed;bad.append(altered(result,history=altered(result.history,origins=tuple(values))))
        bad.append(altered(result,history=altered(result.history,records=result.history.records[:-1])))
        for value in bad:
            with self.assertRaises(AssertionError):qualify_result(value)

    def test_falsified_actual_sample_query_normal_and_world_position_rejected(self):
        from brepgpu.extrusion_oracles import qualify_samples
        batch=self.sampled(self.feature());rows=list(batch.rows)
        index=next(i for i,row in enumerate(rows) if row.patch_slot==3 and row.native_status==0);row=rows[index]
        for changed in (replace(row,position=(row.position[0]+.001,*row.position[1:])),
                        replace(row,normal=tuple(-v for v in row.normal)),replace(row,u=math.nextafter(row.u,math.inf)),
                        replace(row,face_id=1),replace(row,index=True)):
            values=list(rows);values[index]=changed
            with self.assertRaises(AssertionError):qualify_samples(altered(batch,rows=tuple(values)))

    def test_late_validation_failure_preserves_source_prior_result_and_retry(self):
        from brepgpu.extrusion import extrude
        from brepgpu.extrusion_oracles import qualify_result
        result=self.feature();profile=result.profile
        before=tuple(getattr(profile,name) for name in ('vertices','curves','edges','coedges','loops','faces','charts','patches'))
        history=(result.history.records,result.history.origins);sentinel=RuntimeError('independent late extrusion failure')
        with patch('brepgpu.extrusion._validate_candidate',side_effect=sentinel) as final:
            with self.assertRaises(RuntimeError) as caught:extrude(profile,F(1))
            self.assertIs(caught.exception,sentinel);self.assertEqual(final.call_count,1)
        self.assertEqual(tuple(getattr(profile,name) for name in ('vertices','curves','edges','coedges','loops','faces','charts','patches')),before)
        self.assertEqual((result.history.records,result.history.origins),history);qualify_result(result)
        nextresult=extrude(profile,F(1));qualify_result(nextresult)
        self.assertNotEqual(nextresult.solid.body_id,result.solid.body_id)
        self.sampled(result)


if __name__=='__main__':unittest.main()
