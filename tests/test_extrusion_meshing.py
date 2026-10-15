"""Direct actual extrusion-result mesh obligations; fleet execution only."""
from dataclasses import FrozenInstanceError,fields,replace
from decimal import Decimal
from fractions import Fraction as F
import math
import unittest

TRIANGLE=((-F(5,8),-F(1,2)),(F(3,4),-F(3,8)),(-F(1,8),F(3,4)))
CHAMFER=tuple((F(x,256),F(y,256)) for x,y in ((-160,-128),(64,-160),(192,-32),(128,128),(-32,192),(-192,32)))
MANY16=tuple((F(x,256),F(y,256)) for x,y in ((176,0),(160,64),(128,128),(64,160),(0,176),(-64,160),(-128,128),(-160,64),(-176,0),(-160,-64),(-128,-128),(-64,-160),(0,-176),(64,-160),(128,-128),(160,-64)))


def maximum32():
    quadrant=((192,0),(188,37),(177,73),(160,107),(136,136),(107,160),(73,177),(37,188))
    return tuple(tuple(F(v,256) for v in ((x,y),(-y,x),(-x,-y),(y,-x))[q])
                 for q in range(4) for x,y in quadrant)


def changed(record,**values):
    result=object.__new__(type(record))
    for field in fields(record):object.__setattr__(result,field.name,values.get(field.name,getattr(record,field.name)))
    return result


def feature(points=TRIANGLE,height=F(3,4),scale=1.,origin=(0.,0.,0.)):
    from brepgpu.body import Frame
    from brepgpu.extrusion import make_profile,extrude
    return extrude(make_profile(points,Frame(origin,scale)),height)


class ExtrusionMeshingTests(unittest.TestCase):
    def mesh(self,result,tolerance=F(1,1024)):
        from brepgpu.extrusion_meshing import tessellate
        from brepgpu.extrusion_meshing_oracles import qualify_mesh
        mesh=tessellate(result,tolerance=tolerance)
        self.assertIs(mesh.feature_result,result);self.assertIs(mesh.body,result.solid)
        self.assertIs(mesh.history,result.history)
        self.assertEqual(mesh.tolerance,F(tolerance));self.assertIs(type(mesh.tolerance),F)
        self.assertEqual(qualify_mesh(mesh)['genus'],0)
        return mesh

    def reject(self,mesh,message,**changes):
        from brepgpu.extrusion_meshing_oracles import qualify_mesh
        with self.assertRaisesRegex(AssertionError,message):qualify_mesh(changed(mesh,**changes))

    def test_complete_distinct_profile_families_and_source_history(self):
        from brepgpu.extrusion import EntityKey
        for points in (TRIANGLE,CHAMFER,MANY16,maximum32()):
            with self.subTest(vertices=len(points)):
                result=feature(points);mesh=self.mesh(result);n=len(points)
                self.assertEqual((len(mesh.vertices),len(mesh.triangles),len(mesh.edge_polylines),len(mesh.boundary_uses)),
                                 (2*n,4*n-4,3*n,6*n))
                self.assertEqual((mesh.quarter_segments,mesh.curve_bound,mesh.rounding_bound,mesh.error_bound),(0,F(0),F(0),F(0)))
                self.assertEqual((len(mesh.history.records),len(mesh.history.origins)),(3*n+3,14*n+6))
                for kind,ids in (('vertex',(v.source[1] for v in mesh.vertices)),
                                 ('edge',(p.edge_id for p in mesh.edge_polylines)),
                                 ('coedge',(u.coedge_id for u in mesh.boundary_uses)),
                                 ('face',{t.face_id for t in mesh.triangles}),
                                 ('chart',{c.chart_id for t in mesh.triangles for c in t.corners})):
                    for entity_id in ids:
                        origin=mesh.history.origin(EntityKey(mesh.body.body_id,mesh.body.revision,kind,entity_id))
                        self.assertTrue(origin.parents)
                        self.assertTrue(all(p.body_id==result.profile.body_id for p in origin.parents))

    def test_frame_extremes_heights_and_narrow_nonaxis_profile(self):
        narrow=((F(0),F(0)),(F(191,256),F(190,256)),(F(192,256),F(191,256)))
        for scale in (.25,.5,1.,2.,4.):
            for height in (F(1,4),F(2)):
                with self.subTest(scale=scale,height=height):
                    mesh=self.mesh(feature(narrow,height,scale,(16.,-16.,.125)))
                    self.assertEqual(len(mesh.triangles),8)
                    self.assertEqual(mesh.error_bound,0)
        shifted=TRIANGLE[1:]+TRIANGLE[:1]
        self.mesh(feature(shifted))

    def test_subnormal_actual_rounding_bound_and_exact_threshold(self):
        from brepgpu.extrusion_meshing import tessellate
        from brepgpu.meshing import MeshBudgetError
        tiny=math.ulp(0.);result=feature(origin=(tiny,-tiny,tiny))
        mesh=self.mesh(result)
        rho=max(sum((abs(F.from_float(y)-x) for x,y in zip(v.position,v.rounded_position)),F(0)) for v in mesh.vertices)
        self.assertEqual(mesh.rounding_bound,rho);self.assertEqual(mesh.error_bound,rho)
        self.assertGreater(rho,0)
        self.assertEqual(self.mesh(result,rho).error_bound,rho)
        with self.assertRaises(MeshBudgetError):tessellate(result,tolerance=rho/2)
        self.mesh(result,rho)
        # A normal, non-dyadic-origin mantissa loses bits under translation too.
        self.assertGreater(self.mesh(feature(CHAMFER,origin=(.1,-.2,.3))).rounding_bound,0)

    def test_directed_shared_boundaries_and_oblique_original_uv(self):
        mesh=self.mesh(feature());body=mesh.body
        for poly in mesh.edge_polylines:
            uses=[u for u in mesh.boundary_uses if u.edge_id==poly.edge_id]
            self.assertEqual(len(uses),2)
            self.assertEqual(uses[0].vertices,uses[1].vertices[::-1])
            self.assertEqual(uses[0].parameters,uses[1].parameters[::-1])
        lower=[u for u in mesh.boundary_uses if u.face_id==1]
        self.assertTrue(all(u.orientation==-1 and u.parameters==(F(1),F(0)) for u in lower))
        for t in mesh.triangles:
            for i,c in zip(t.vertices,t.corners):
                if t.face_id<=2:
                    self.assertEqual(c.uv,tuple((x+1)/2 for x in body.vertices[i].position[:2]))
                else:self.assertTrue(all(x in (F(0),F(1)) for x in c.uv))

    def test_invalid_requests_preserve_source_and_prior_output(self):
        from brepgpu.extrusion_meshing import tessellate
        from brepgpu.meshing import tessellate as canonical
        from brepgpu.cut_meshing import tessellate as cut
        from brepgpu.body import make_body
        result=feature();prior=self.mesh(result);before=(result.solid.vertices,result.history.origins,prior.vertices,prior.triangles)
        for value in (True,False,0,-1,float('nan'),float('inf'),-float('inf'),Decimal('.1'),'0.1',None,()):
            with self.subTest(tolerance=value):
                with self.assertRaises((TypeError,ValueError)):tessellate(result,tolerance=value)
        for value in (result.solid,result.profile,make_body('box'),object(),None):
            with self.assertRaises(TypeError):tessellate(value,tolerance=1)
        with self.assertRaises(TypeError):tessellate(result,tolerance=1,max_quarter_segments=1)
        with self.assertRaises(TypeError):canonical(result.solid,tolerance=1)
        with self.assertRaises(TypeError):cut(result,tolerance=1)
        self.assertEqual(before,(result.solid.vertices,result.history.origins,prior.vertices,prior.triangles))
        self.assertEqual(self.mesh(result,1.).triangles,prior.triangles)
        self.assertEqual(self.mesh(result,1).vertices,prior.vertices)

    def test_factory_only_lazy_adapter_and_recursive_immutability(self):
        from brepgpu.extrusion_meshing import ExtrusionMesh
        result=feature();mesh=self.mesh(result)
        self.assertEqual(result.tessellate(tolerance=F(1,1024)),mesh)
        with self.assertRaises(TypeError):ExtrusionMesh()
        for record,name,value in ((mesh,'feature_result',None),(mesh,'body',None),(mesh,'history',None),
                                  (mesh,'curve_bound',F(1)),(mesh,'quarter_segments',1),
                                  (mesh.vertices[0],'position',(F(0),)*3),
                                  (mesh.triangles[0],'vertices',(0,0,0)),(mesh.boundary_uses[0],'uv',())):
            with self.assertRaises((FrozenInstanceError,AttributeError,TypeError)):setattr(record,name,value)
        with self.assertRaises(TypeError):mesh.vertices[0].position[0]=F(0)

    def test_missing_duplicate_and_reversed_triangles(self):
        mesh=self.mesh(feature(CHAMFER));triangles=mesh.triangles
        missing=tuple(replace(t,id=i) for i,t in enumerate(triangles[1:]))
        self.reject(mesh,'full positive triangle boundary chain|missing original',triangles=missing)
        a,b=[t for t in triangles if t.face_id==1][:2];duplicate=list(triangles);duplicate[b.id]=replace(a,id=b.id)
        self.reject(mesh,'full positive triangle boundary chain',triangles=tuple(duplicate))
        for selected in ((0,),tuple(range(len(triangles)))):
            flipped=list(triangles)
            for i in selected:
                t=flipped[i];flipped[i]=replace(t,vertices=t.vertices[:1]+t.vertices[:0:-1],corners=t.corners[:1]+t.corners[:0:-1])
            self.reject(mesh,'triangle orientation',triangles=tuple(flipped))

    def test_source_chart_uv_and_shared_identity_faults(self):
        mesh=self.mesh(feature());t=mesh.triangles[0];v=mesh.vertices[0]
        wrong=replace(t,corners=(replace(t.corners[0],chart_id=2),)+t.corners[1:])
        self.reject(mesh,'corner chart/face',triangles=(wrong,)+mesh.triangles[1:])
        wrong=replace(t,corners=(replace(t.corners[0],uv=(F(0),F(0))),)+t.corners[1:])
        self.reject(mesh,'corner original patch/UV',triangles=(wrong,)+mesh.triangles[1:])
        self.reject(mesh,'original vertex source identity',vertices=(replace(v,source=('vertex',2)),)+mesh.vertices[1:])
        self.reject(mesh,'vertex differs from original',vertices=(replace(v,position=(v.position[0]+F(1,256),*v.position[1:])),)+mesh.vertices[1:])
        self.reject(mesh,'complete original-source vertex',vertices=mesh.vertices+(replace(v,id=len(mesh.vertices)),))

    def test_source_parameters_uv_occurrences_and_native_types(self):
        mesh=self.mesh(feature());poly=mesh.edge_polylines[0];use=mesh.boundary_uses[0]
        for bad in (replace(poly,parameters=(F(0),F(1,2))),replace(poly,parameters=(0.,1.)),
                    replace(poly,vertices=(float(poly.vertices[0]),poly.vertices[1]))):
            self.reject(mesh,'parameter chain|edge endpoints',edge_polylines=(bad,)+mesh.edge_polylines[1:])
        for bad in (replace(use,orientation=-use.orientation),replace(use,face_id=True),
                    replace(use,parameters=use.parameters[::-1]),replace(use,vertices=use.vertices[::-1]),
                    replace(use,uv=use.uv[::-1])):
            self.reject(mesh,'ownership/sign|parameter/identity|directed UV',boundary_uses=(bad,)+mesh.boundary_uses[1:])
        self.reject(mesh,'original vertex source identity',vertices=(replace(mesh.vertices[0],source=('vertex',True)),)+mesh.vertices[1:])

    def test_actual_float_error_and_retained_identity_faults(self):
        mesh=self.mesh(feature(origin=(.1,-.2,.3)));v=mesh.vertices[0]
        bad=replace(v,rounded_position=(math.nextafter(v.rounded_position[0],math.inf),*v.rounded_position[1:]))
        self.reject(mesh,'actual rounded coordinates',vertices=(bad,)+mesh.vertices[1:])
        self.reject(mesh,'rounded displacement bound',rounding_bound=F(0))
        self.reject(mesh,'whole-triangle error',error_bound=F(0))
        self.reject(mesh,'retained actual result/body/history',feature_result=feature(origin=(.1,-.2,.3)))


if __name__=='__main__':unittest.main()
