"""Independent complete through-bore geometry/history/consumer qualification."""
from dataclasses import fields,replace
from fractions import Fraction as F
import math
import os
import unittest
from unittest.mock import patch


def altered(value,**changes):
    result=object.__new__(type(value))
    for field in fields(value):object.__setattr__(result,field.name,changes.get(field.name,getattr(value,field.name)))
    return result


def world(result,point):
    return tuple(F(o)+F(result.left.frame.scale)*F(v) for o,v in zip(result.left.frame.origin,point))



class BooleanTests(unittest.TestCase):
    def test_existing_healthy_body_then_actual_difference(self):
        from brepgpu import Engine
        from brepgpu.body import Frame,make_body
        from brepgpu.body_sampling import BodySampler,Candidate,SampleRequest
        box=make_body('box');cutter=make_body('cylinder',Frame(scale=2.))
        with Engine(backend='cpu',threads=1) as engine:
            with BodySampler(engine,box) as sampler:
                batch=sampler.sample(SampleRequest(box,(Candidate(0,0.,0.),)))
            self.assertEqual(batch.rows[0].native_status,0)
            self.assertEqual(batch.rows[0].position,(-.75,-.75,.5))
        self.assertEqual(cutter.classify_point((0,0,0)).label,'interior')
        from brepgpu.boolean import difference
        result=difference(box,cutter)
        self.assertIs(result.left,box);self.assertIs(result.right,cutter)
        self.assertEqual(result.solid.classify_point((0,0,0)).label,'exterior')
        self.assertEqual(result.solid.classify_point((F(1,2),0,0)).label,'boundary')

    def cut(self,scale=1.,origin=(0.,0.,0.)):
        from brepgpu.body import Frame,make_body
        from brepgpu.boolean import difference
        from brepgpu.boolean_oracles import qualify_result
        left=make_body('box',Frame(origin,scale));right=make_body('cylinder',Frame(origin,2*scale))
        result=difference(left,right)
        self.assertIs(result.left,left);self.assertIs(result.right,right)
        quality=qualify_result(result)
        self.assertEqual((quality['history_inputs'],quality['history_outputs']),(85,81))
        return result

    def sample(self,result,request=None):
        from brepgpu import Engine
        from brepgpu.boolean_sampling import sample,prepare_grid
        from brepgpu.boolean_oracles import qualify_samples
        request=prepare_grid(result,5) if request is None else request
        backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        with Engine(backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1'))) as engine:
            batch=sample(engine,request)
        self.assertIs(batch.result,result);self.assertIs(batch.request,request)
        self.assertEqual((batch.backend,batch.device),(backend,device))
        quality=qualify_samples(batch)
        self.assertEqual(quality['count'],len(request.candidates))
        return batch

    def test_all_scales_translations_complete_boundary_and_all_kind_history(self):
        from brepgpu.boolean_oracles import qualify_result
        for scale in (.25,.5,1.,2.):
            with self.subTest(scale=scale):
                result=self.cut(scale,(8.,-4.,.125))
                q=qualify_result(result)
                self.assertEqual((q['vertices'],q['edges'],q['coedges'],q['loops'],q['faces'],q['charts']),(10,15,30,9,7,10))
                self.assertEqual((q['retained'],q['modified'],q['deleted'],q['generated'],q['genus']),(58,13,14,10,1))
                self.assertEqual((result.solid.volume.rational,result.solid.volume.pi),(F(9,4)*F(scale)**3,-F(1,4)*F(scale)**3))
                self.assertEqual(result.solid.patches[0].scale,scale)
                self.assertEqual(result.solid.patches[6].scale,2*scale)
                for out,source in zip(result.solid.patches[:6],result.left.patches):self.assertEqual(out.controls,source.controls)
                self.assertEqual(result.solid.patches[6].controls,result.right.patches[2].controls)

    def test_exact_regularized_material_every_support_stratum_and_ulp_sides(self):
        result=self.cut(.5,(2.,-1.,.125));solid=result.solid
        cases=[((0,0,0),'exterior',()),((F(5,8),0,0),'interior',()),
               ((F(1,2),0,0),'boundary',(7,)),((0,F(1,2),0),'boundary',(7,)),
               ((F(1,2),0,F(1,2)),'boundary',(1,7)),((F(1,2),0,-F(1,2)),'boundary',(2,7)),
               ((0,0,F(1,2)),'exterior',()),((0,0,-F(1,2)),'exterior',()),
               ((F(3,4),F(3,4),F(1,2)),'boundary',(1,4,5)),
               ((-F(3,4),-F(3,4),-F(1,2)),'boundary',(2,3,6)),
               ((F(5,8),0,F(1,2)),'boundary',(1,)),((F(5,8),0,-F(1,2)),'boundary',(2,)),
               ((0,-F(3,4),0),'boundary',(3,)),((0,F(3,4),0),'boundary',(4,)),
               ((F(3,4),0,0),'boundary',(5,)),((-F(3,4),0,0),'boundary',(6,)),
               ((F(1,2),0,1),'exterior',())]
        for point,label,faces in cases:
            query=world(result,point);answer=solid.classify_point(query)
            self.assertIs(answer.solid,solid);self.assertEqual(answer.point,query)
            self.assertEqual((answer.label,answer.boundary_faces),(label,faces))
        for x,label in ((math.nextafter(.5,0.),'exterior'),(math.nextafter(.5,1.),'interior')):
            self.assertEqual(solid.classify_point(world(result,(F(x),0,0))).label,label)
        tiny=F(1,1<<1074)
        self.assertEqual(solid.classify_point(world(result,(F(1,2)-tiny,0,0))).label,'exterior')
        self.assertEqual(solid.classify_point(world(result,(F(1,2)+tiny,0,0))).label,'interior')

    def test_exact_cap_rims_original_wall_band_and_internal_chart_joins(self):
        result=self.cut();solid=result.solid
        self.assertEqual(solid.classify(0,F(5,6),F(1,2)),2)
        self.assertEqual(solid.classify(0,float(F(5,6)),.5),1)
        self.assertEqual(solid.classify(0,math.nextafter(float(F(5,6)),0.),.5),0)
        for slot in (0,1):
            self.assertEqual(solid.classify(slot,F(1,2),F(1,2)),0)
            self.assertEqual(solid.classify(slot,F(0),F(0)),2)
        for slot in range(6,10):
            self.assertEqual(solid.classify(slot,F(1,2),F(1,4)),2)
            self.assertEqual(solid.classify(slot,F(1,2),F(3,4)),2)
            self.assertEqual(solid.classify(slot,F(1,2),F(1,8)),0)
            self.assertEqual(solid.classify(slot,F(1,2),F(7,8)),0)
            self.assertEqual(solid.classify(slot,F(1,2),F(1,2)),1)
            self.assertEqual(solid.classify(slot,math.nan,.5),3)
        self.assertEqual((solid.classify(6,0,.5),solid.classify(9,1,.5)),(2,2))
        self.assertEqual((solid.classify(6,1,.5),solid.classify(7,0,.5)),(1,1))
        for invalid in (True,'0.5',object()):
            with self.assertRaises((TypeError,ValueError)):solid.classify(0,invalid,F(1,2))

    def test_actual_source_rim_events_same_face_seam_and_exact_reader_maps(self):
        result=self.cut();solid=result.solid
        seam=[c for c in solid.coedges if c.edge_slot==14]
        self.assertEqual(len(seam),2)
        self.assertEqual({c.orientation for c in seam},{-1,1})
        self.assertEqual({solid.coedge_uv(c.id,F(1,7))[0] for c in seam},{F(0),F(4)})
        self.assertEqual({solid.coedge_uv(c.id,F(1,7))[1] for c in seam},{F(9,28)})
        self.assertEqual(solid.edge_point(15,F(1,7)),(F(1,2),F(0),-F(5,14)))
        lower=solid.edge_point(13,F(1,28));upper=solid.edge_point(14,F(1,28))
        self.assertEqual(lower[:2],upper[:2]);self.assertEqual((lower[2],upper[2]),(-F(1,2),F(1,2)))
        for section in result.sections:
            self.assertIs(section.left.snapshot,result.left);self.assertIs(section.right.snapshot,result.right)
            self.assertEqual((len(section.components),len(section.pieces),len(section.events)),(1,4,4))
        self.assertTrue(all(o.parents for o in result.history.origins))
        generated=[o for o in result.history.origins if o.relation=='generated']
        self.assertEqual(len(generated),10)
        self.assertTrue(all(len(o.parents)==(3 if o.target.kind=='vertex' else 2) for o in generated))

    def test_native_mixed_source_frames_normals_rows_and_actual_float_binding(self):
        from brepgpu.body_sampling import Candidate
        from brepgpu.boolean_sampling import CutRequest
        result=self.cut(.5,(8.,-8.,.125))
        candidates=(Candidate(6,0.,.25),Candidate(9,1.,.75),Candidate(6,1.,.5),Candidate(7,0.,.5),
                    Candidate(0,.5,.5),Candidate(0,F(5,6),.5),Candidate(6,.5,.125),Candidate(6,math.nan,.5),
                    Candidate(5,.125,.75),Candidate(6,0.,.25))
        request=CutRequest(result,candidates);batch=self.sample(result,request)
        self.assertEqual(tuple(r.trim for r in batch.rows),(2,2,1,1,0,1,0,3,1,2))
        self.assertEqual(batch.rows[0].position,(8.25,-8.,-.125))
        self.assertEqual(batch.rows[0].normal,(-1.,0.,0.))
        self.assertEqual(batch.rows[0].position,batch.rows[9].position)
        self.sample(result) # A complete ten-carrier grid, not only hand-selected rows.

    def test_actual_topology_reader_permutations_and_same_face_mates(self):
        from brepgpu.adjacency import AdjacencyLibrary
        from brepgpu.boolean_oracles import qualify_topology
        result=self.cut();source=result.solid.topology_input()
        self.assertEqual(source.snapshots,())
        variants=(source,source.permuted(edge_order=tuple(reversed(range(15))),
                    face_order=(6,2,4,0,5,1,3),coedge_order=tuple(range(1,30,2))+tuple(range(0,30,2))))
        backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu');device=int(os.environ.get('BREPGPU_TEST_DEVICE','0'))
        library=AdjacencyLibrary()
        for data in variants:
            actual=library.build(data,backend=backend,device=device,threads=int(os.environ.get('BREPGPU_TEST_THREADS','1')))
            self.assertIs(actual.input,data)
            quality=qualify_topology(result,actual)
            self.assertEqual((quality['paired'],quality['same_face_pair']),(14,1))

    def test_falsified_domain_shell_source_and_history_outputs_rejected(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.boolean_oracles import qualify_result
        result=self.cut();solid=result.solid
        wrongleft=make_body('box',result.left.frame);wrongright=make_body('cylinder',result.right.frame)
        failures=[altered(result,left=wrongleft),altered(result,right=wrongright)]
        wall=solid.faces[6]
        failures.append(altered(result,solid=altered(solid,faces=solid.faces[:6]+(replace(wall,orientation=1),))))
        patches=list(solid.patches);patches[6]=replace(patches[6],scale=solid.frame.scale)
        failures.append(altered(result,solid=altered(solid,patches=tuple(patches))))
        coedges=list(solid.coedges);seam=next(i for i,c in enumerate(coedges) if c.edge_slot==14)
        coedges[seam]=replace(coedges[seam],orientation=-coedges[seam].orientation)
        failures.append(altered(result,solid=altered(solid,coedges=tuple(coedges))))
        face=solid.faces[0];loop=solid.loops[0];chart=solid.charts[0]
        aliases=(altered(solid,revision=True),
                 altered(solid,faces=(replace(face,loop_slots=(False,)+face.loop_slots[1:]),)+solid.faces[1:]),
                 altered(solid,loops=(replace(loop,coedge_slots=(False,)+loop.coedge_slots[1:]),)+solid.loops[1:]),
                 altered(solid,charts=(replace(chart,face_slot=False),)+solid.charts[1:]),
                 altered(solid,vertices=(replace(solid.vertices[0],position=tuple(map(F,solid.vertices[0].position))),)+solid.vertices[1:]),
                 altered(solid,curves=(replace(solid.curves[0],data=tuple(map(F,solid.curves[0].data))),)+solid.curves[1:]))
        failures.extend(altered(result,solid=value) for value in aliases)
        failures.append(altered(result,history=altered(result.history,records=result.history.records[:-1])))
        origins=list(result.history.origins);index=next(i for i,o in enumerate(origins) if o.relation=='generated' and o.target.kind=='edge')
        origins[index]=replace(origins[index],parents=origins[index].parents[:1])
        failures.append(altered(result,history=altered(result.history,origins=tuple(origins))))
        origins=list(result.history.origins);index=next(i for i,o in enumerate(origins) if o.target.kind=='face' and o.target.entity_id==7)
        origins[index]=replace(origins[index],relation='retained')
        failures.append(altered(result,history=altered(result.history,origins=tuple(origins))))
        for bad in failures:
            with self.assertRaises(AssertionError):qualify_result(bad)

    def test_falsified_actual_sample_geometry_normal_and_query_metadata_rejected(self):
        from brepgpu.boolean_oracles import qualify_samples
        batch=self.sample(self.cut());rows=list(batch.rows)
        index=next(i for i,row in enumerate(rows) if row.native_status==0 and row.patch_slot==6)
        original=rows[index]
        changes=(replace(original,position=tuple(x+.01 for x in original.position)),
                 replace(original,normal=tuple(-x for x in original.normal)),
                 replace(original,u=math.nextafter(original.u,math.inf)),replace(original,face_id=1))
        for changed in changes:
            alteredrows=list(rows);alteredrows[index]=changed
            with self.assertRaises(AssertionError):qualify_samples(altered(batch,rows=tuple(alteredrows)))

    def test_late_candidate_failure_preserves_both_inputs_and_existing_result(self):
        from brepgpu.boolean import difference
        from brepgpu.boolean_oracles import qualify_result
        result=self.cut();left,right=result.left,result.right
        sourcebytes=(left.packed_tables,right.packed_tables)
        oldhistory=(result.history.records,result.history.origins)
        failure=RuntimeError('deliberate late independent Boolean witness')
        with patch('brepgpu.boolean._validate_candidate',side_effect=failure) as boundary:
            with self.assertRaises(RuntimeError) as caught:difference(left,right)
            self.assertIs(caught.exception,failure)
            self.assertEqual(boundary.call_count,1)
            self.assertEqual(len(boundary.call_args.args[4]),2)
            self.assertEqual(len(boundary.call_args.args[5]),2)
        self.assertEqual((left.packed_tables,right.packed_tables),sourcebytes)
        self.assertEqual((result.history.records,result.history.origins),oldhistory)
        self.assertEqual(result.solid.classify_point(world(result,(0,0,0))).label,'exterior')
        healthy=difference(left,right);qualify_result(healthy)
        self.assertNotEqual(healthy.solid.body_id,result.solid.body_id)
        self.sample(result)


if __name__=='__main__':unittest.main()
