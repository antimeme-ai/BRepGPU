"""Direct actual CutResult source/domain/rounded contracts; fleet execution only."""
from dataclasses import FrozenInstanceError,fields,replace
from decimal import Decimal
from fractions import Fraction as F
import math
import unittest


def changed(record,**values):
    result=object.__new__(type(record))
    for field in fields(record):
        object.__setattr__(result,field.name,values.get(field.name,getattr(record,field.name)))
    return result


def cut(scale=1.,origin=(0.,0.,0.)):
    from brepgpu.body import Frame,make_body
    from brepgpu.boolean import difference
    left=make_body('box',Frame(origin,scale))
    right=make_body('cylinder',Frame(origin,2*scale))
    return difference(left,right)


class CutMeshingTests(unittest.TestCase):
    def mesh(self,result,tolerance=F(1,2),**options):
        # Import the real future API; no fallback/proxy or skip can turn red green.
        from brepgpu.cut_meshing import tessellate
        from brepgpu.cut_meshing_oracles import qualify_mesh
        mesh=tessellate(result,tolerance=tolerance,**options)
        self.assertIs(mesh.cut_result,result)
        self.assertIs(mesh.body,result.solid)
        self.assertIs(mesh.history,result.history)
        self.assertIs(type(mesh.tolerance),F)
        self.assertEqual(mesh.tolerance,F(tolerance))
        self.assertEqual(qualify_mesh(mesh)['genus'],1)
        return mesh

    def reject(self,mesh,message,**changes):
        from brepgpu.cut_meshing_oracles import qualify_mesh
        with self.assertRaisesRegex(AssertionError,message):
            qualify_mesh(changed(mesh,**changes))

    def test_actual_result_coarse_complete_mesh(self):
        result=cut();before=(result.solid.vertices,result.history.origins)
        mesh=self.mesh(result,max_quarter_segments=1)
        self.assertEqual((mesh.quarter_segments,len(mesh.vertices),len(mesh.triangles)),(1,16,32))
        self.assertEqual((len(mesh.edge_polylines),len(mesh.boundary_uses)),(15,30))
        self.assertEqual({t.face_id for t in mesh.triangles},set(range(1,8)))
        self.assertEqual((mesh.curve_bound,mesh.rounding_bound),(F(1,4),F(0)))
        self.assertEqual((len(mesh.history.records),len(mesh.history.origins)),(85,81))
        self.assertEqual(before,(result.solid.vertices,result.history.origins))
        # Arbitrarily coarse tolerance cannot erase the genuine result hole.
        coarse=self.mesh(result,tolerance=100,max_quarter_segments=1)
        self.assertEqual((len(coarse.vertices),len(coarse.triangles)),(16,32))

    def test_frames_fine_and_original_source_band(self):
        for scale in (.25,.5,1.,2.):
            with self.subTest(scale=scale):
                result=cut(scale,(16.,-16.,.125));mesh=self.mesh(result,F(scale)/32,max_quarter_segments=4)
                self.assertEqual(mesh.quarter_segments,4)
                self.assertLessEqual(mesh.error_bound,F(scale)/32)
                wall=result.solid.faces[-1]
                self.assertEqual({c.uv[1] for t in mesh.triangles if t.face_id==wall.id for c in t.corners},
                                 {F(1,4),F(3,4)})
                self.assertTrue(all(result.solid.charts[c.chart_id-1].patch.scale==2*scale
                                    for t in mesh.triangles if t.face_id==wall.id for c in t.corners))
                self.assertEqual(mesh.vertices[0].position[2],F(1,8)-F(scale)/2)
        fine=self.mesh(cut(),F(1,1024),max_quarter_segments=32)
        self.assertEqual((fine.quarter_segments,len(fine.vertices),len(fine.triangles)),(32,264,528))
        self.assertGreater(fine.rounding_bound,0)

    def test_exact_threshold_and_immediately_lower_float(self):
        from brepgpu.cut_meshing import tessellate
        from brepgpu.meshing import MeshBudgetError
        result=cut();mesh=self.mesh(result,F(1,2),max_quarter_segments=1)
        self.assertEqual(mesh.quarter_segments,1)
        below=math.nextafter(.5,0.)
        self.assertEqual(self.mesh(result,below,max_quarter_segments=2).quarter_segments,2)
        with self.assertRaises(MeshBudgetError):tessellate(result,tolerance=below,max_quarter_segments=1)
        self.assertEqual(self.mesh(result,F(1,8),max_quarter_segments=2).quarter_segments,2)

    def test_subnormal_origin_and_actual_binary64_displacement(self):
        tiny=math.ulp(0.)
        mesh=self.mesh(cut(.25,(tiny,-tiny,tiny)),F(1,4096))
        self.assertGreater(mesh.rounding_bound,0)
        actual=max(sum((abs(F.from_float(b)-a) for a,b in zip(v.position,v.rounded_position)),F(0))
                   for v in mesh.vertices)
        self.assertEqual(mesh.rounding_bound,actual)
        self.assertEqual(mesh.error_bound,mesh.curve_bound+actual)

    def test_shared_rims_seam_lifts_and_history_namespace(self):
        from brepgpu.boolean import EntityKey
        mesh=self.mesh(cut(),F(1,8));body=mesh.body
        seams=[u for u in mesh.boundary_uses if u.edge_id==15]
        self.assertEqual(len(seams),2)
        self.assertEqual(seams[0].vertices,seams[1].vertices[::-1])
        self.assertEqual({uv[0] for u in seams for uv in u.uv},{F(0),F(4)})
        self.assertEqual({uv[1] for u in seams for uv in u.uv},{F(1,4),F(3,4)})
        for poly in mesh.edge_polylines:
            if body.curves[body.edges[poly.edge_id-1].curve_slot].kind=='circle':
                self.assertEqual(poly.vertices[0],poly.vertices[-1])
                uses=[u for u in mesh.boundary_uses if u.edge_id==poly.edge_id]
                self.assertEqual(len(uses),2)
                self.assertEqual(uses[0].vertices,uses[1].vertices[::-1])
        for kind,ids in (('vertex',{v.source[1] for v in mesh.vertices if v.source[0]=='vertex'}),
                         ('edge',{p.edge_id for p in mesh.edge_polylines}),
                         ('coedge',{u.coedge_id for u in mesh.boundary_uses}),
                         ('face',{t.face_id for t in mesh.triangles}),
                         ('chart',{c.chart_id for t in mesh.triangles for c in t.corners})):
            for entity_id in ids:
                origin=mesh.history.origin(EntityKey(body.body_id,body.revision,kind,entity_id))
                self.assertTrue(origin.parents)
                self.assertTrue(all(p.body_id in (mesh.cut_result.left.body_id,mesh.cut_result.right.body_id)
                                    for p in origin.parents))

    def test_invalid_requests_reject_and_preserve_prior_outputs(self):
        from brepgpu.body import make_body
        from brepgpu.cut_meshing import tessellate
        from brepgpu.meshing import MeshBudgetError
        result=cut();prior=self.mesh(result);before=(result.solid.vertices,result.history.origins,prior.vertices)
        for value in (True,False,0,-1,float('nan'),float('inf'),-float('inf'),Decimal('.1'),'0.1',None,()):
            with self.subTest(tolerance=value):
                with self.assertRaises((TypeError,ValueError)):tessellate(result,tolerance=value)
        for budget in (True,False,0,-1,3,1025,2048,1.,F(1),None):
            with self.subTest(budget=budget):
                with self.assertRaises((TypeError,ValueError)):tessellate(result,tolerance=F(1,2),max_quarter_segments=budget)
        for value in (result.solid,result.left,make_body('plate'),object(),None):
            with self.assertRaises(TypeError):tessellate(value,tolerance=F(1,2))
        with self.assertRaises(MeshBudgetError):tessellate(result,tolerance=F(1,1<<80),max_quarter_segments=1024)
        self.assertEqual(before,(result.solid.vertices,result.history.origins,prior.vertices))
        self.mesh(result,max_quarter_segments=1)
        # Existing canonical admission is deliberately unchanged.
        from brepgpu.meshing import tessellate as canonical
        with self.assertRaises(TypeError):canonical(result.solid,tolerance=F(1,2))

    def test_factory_only_adapter_and_recursive_immutability(self):
        from brepgpu.cut_meshing import CutMesh
        result=cut();mesh=self.mesh(result)
        self.assertEqual(result.tessellate(tolerance=F(1,2)),mesh)
        with self.assertRaises(TypeError):CutMesh()
        for record,name,value in ((mesh,'cut_result',cut()),(mesh,'body',result.left),
                                  (mesh,'history',None),(mesh.vertices[0],'position',(F(0),)*3),
                                  (mesh.triangles[0],'vertices',(0,0,0)),
                                  (mesh.boundary_uses[0],'uv',())):
            with self.assertRaises((FrozenInstanceError,AttributeError,TypeError)):setattr(record,name,value)
        with self.assertRaises(TypeError):mesh.vertices[0].position[0]=F(0)

    def test_missing_duplicate_and_flipped_actual_triangles(self):
        mesh=self.mesh(cut());triangles=mesh.triangles
        missing=tuple(replace(t,id=i) for i,t in enumerate(triangles[1:]))
        self.reject(mesh,'full positive triangle boundary chain|missing original',triangles=missing)
        same=[t for t in triangles if t.face_id==1];a,b=same[:2]
        duplicate=list(triangles);duplicate[b.id]=replace(a,id=b.id)
        self.reject(mesh,'full positive triangle boundary chain',triangles=tuple(duplicate))
        for selected in ((0,),tuple(range(len(triangles)))):
            flipped=list(triangles)
            for i in selected:
                t=flipped[i];flipped[i]=replace(t,vertices=t.vertices[:1]+t.vertices[:0:-1],
                                               corners=t.corners[:1]+t.corners[:0:-1])
            self.reject(mesh,'triangle orientation',triangles=tuple(flipped))

    def test_filled_hole_in_actual_cap_fails_geometric_oracle(self):
        from brepgpu.meshing import MeshCorner,MeshTriangle
        mesh=self.mesh(cut());body=mesh.body
        rim=next(u for u in mesh.boundary_uses if u.face_id==1 and u.edge_id==14)
        ids=rim.vertices[:3][::-1];chart=body.charts[0]
        corners=tuple(MeshCorner(chart.id,((mesh.vertices[i].position[0]+F(3,4))/F(3,2),
                                          (mesh.vertices[i].position[1]+F(3,4))/F(3,2))) for i in ids)
        extra=MeshTriangle(len(mesh.triangles),ids,1,corners)
        self.reject(mesh,'whole triangle violates inward radial|full positive triangle boundary chain',
                    triangles=mesh.triangles+(extra,))

    def test_actual_source_generator_uv_and_frame_faults(self):
        mesh=self.mesh(cut(),F(1,8))
        wall=next(t for t in mesh.triangles if t.face_id==7)
        for uv in ((wall.corners[0].uv[0],F(0)),(wall.corners[0].uv[0],F(1))):
            bad=replace(wall,corners=(replace(wall.corners[0],uv=uv),)+wall.corners[1:])
            ts=tuple(bad if t.id==wall.id else t for t in mesh.triangles)
            self.reject(mesh,'corner original patch/UV',triangles=ts)
        # A correct coordinate on another face must not impersonate the original chart.
        wrong=replace(mesh.triangles[0],corners=(replace(mesh.triangles[0].corners[0],chart_id=2),)+mesh.triangles[0].corners[1:])
        self.reject(mesh,'corner chart/face/domain',triangles=(wrong,)+mesh.triangles[1:])
        seam=next(u for u in mesh.boundary_uses if u.edge_id==15 and u.uv[0][0]==4)
        bad=replace(seam,uv=tuple((F(0),uv[1]) for uv in seam.uv))
        self.reject(mesh,'original lifted boundary UV',boundary_uses=tuple(bad if u.coedge_id==seam.coedge_id else u for u in mesh.boundary_uses))
        poly=next(p for p in mesh.edge_polylines if p.edge_id==13)
        wrongp=replace(poly,parameters=(poly.parameters[0],F(1,7))+poly.parameters[2:])
        self.reject(mesh,'source parameter chain',edge_polylines=tuple(wrongp if p.edge_id==poly.edge_id else p for p in mesh.edge_polylines))
        v=mesh.vertices[0];bad=replace(v,position=(v.position[0]+F(1,16),*v.position[1:]))
        self.reject(mesh,'vertex differs from original exact source',vertices=(bad,)+mesh.vertices[1:])

    def test_actual_rounded_and_claimed_budget_faults(self):
        mesh=self.mesh(cut(),F(1,1024))
        v=mesh.vertices[0];bad=replace(v,rounded_position=(math.nextafter(v.rounded_position[0],math.inf),*v.rounded_position[1:]))
        self.reject(mesh,'actual rounded coordinates',vertices=(bad,)+mesh.vertices[1:])
        self.reject(mesh,'conservative curve bound',curve_bound=mesh.curve_bound/2)
        self.reject(mesh,'rounded displacement bound',rounding_bound=F(0))
        self.reject(mesh,'total error',error_bound=F(0))
        # Split one physical rim identity without changing its exact position.
        repeated=replace(mesh.vertices[8],id=len(mesh.vertices))
        self.reject(mesh,'complete original-source vertex generators',vertices=mesh.vertices+(repeated,))

    def test_native_identity_alias_faults(self):
        mesh=self.mesh(cut(),F(1,8))
        for v,source in ((mesh.vertices[0],('vertex',True)),
                         (mesh.vertices[10],('edge',mesh.vertices[10].source[1],float(mesh.vertices[10].source[2])))):
            vs=tuple(replace(x,source=source) if x.id==v.id else x for x in mesh.vertices)
            self.reject(mesh,'source ID|source ID/parameter',vertices=vs)
        u=mesh.boundary_uses[0]
        for name,value in (('face_id',True),('edge_id',float(u.edge_id)),('orientation',float(u.orientation)),
                           ('vertices',(float(u.vertices[0]),)+u.vertices[1:])):
            bad=replace(u,**{name:value})
            self.reject(mesh,'typed ownership|parameter/identity',boundary_uses=(bad,)+mesh.boundary_uses[1:])


if __name__=='__main__':unittest.main()
