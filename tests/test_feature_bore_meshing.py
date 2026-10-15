"""Whole exact/rounded actual pierced-feature mesh obligations; fleet only."""
from dataclasses import FrozenInstanceError,replace
from decimal import Decimal
from fractions import Fraction as F
import math
import unittest

from test_feature_bore import operands,changed,TRIANGLE,SQUARE,CHAMFER,maximum32


class FeatureBoreMeshingTests(unittest.TestCase):
    def mesh(self,points=CHAMFER,tolerance=F(1,32),**kwargs):
        from brepgpu.feature_bore import difference
        from brepgpu.feature_bore_meshing import tessellate
        from brepgpu.feature_bore_meshing_oracles import qualify_mesh
        result=difference(*operands(points,**kwargs));mesh=tessellate(result,tolerance=tolerance)
        self.assertEqual(qualify_mesh(mesh)['genus'],1)
        return mesh

    def reject(self,mesh,message,**changes):
        from brepgpu.feature_bore_meshing_oracles import qualify_mesh
        with self.assertRaisesRegex(AssertionError,message):qualify_mesh(changed(mesh,**changes))

    def test_actual_chain_mesh_retains_both_histories(self):
        from brepgpu.body import make_body
        from brepgpu.extrusion import make_profile,extrude
        from brepgpu.feature_bore import difference
        from brepgpu.feature_bore_meshing import tessellate
        from brepgpu.feature_bore_meshing_oracles import qualify_mesh
        profile=make_profile(tuple((F(x,2),F(y,2)) for x,y in ((-1,-1),(1,-1),(1,1),(-1,1))))
        result=difference(extrude(profile,F(1,2)),make_body('cylinder'))
        mesh=tessellate(result,tolerance=F(1,32))
        self.assertIs(mesh.bore_result,result);self.assertIs(mesh.history,result.history)
        self.assertIs(mesh.composed_history,result.composed_history)
        self.assertEqual(qualify_mesh(mesh)['genus'],1)

    def test_whole_source_profiles_coarse_refined_and_rotated_bridge_words(self):
        for points in (TRIANGLE,SQUARE,CHAMFER,maximum32()[::2],maximum32()):
            for p in (points,points[1:]+points[:1]):
                with self.subTest(n=len(p),first=p[0]):
                    mesh=self.mesh(p,tolerance=F(1,4));n=len(p);m=4*mesh.quarter_segments
                    self.assertEqual(len(mesh.vertices),2*n+2*m)
                    self.assertEqual(len(mesh.triangles),4*n+4*m)
        self.mesh(CHAMFER,tolerance=F(1,512))

    def test_aligned_hole_outer_vertices_and_nearly_tangent_bridge(self):
        # Multiple max-X vertices, same-Y bridge, and exact radial alignments.
        self.mesh(SQUARE,tolerance=1)
        self.mesh(maximum32(),tolerance=F(1,16),offset=(.0625,0.))
        self.mesh(SQUARE,tolerance=F(1,16),offset=(.375-2.**-40,0.))
        diamond=tuple((F(x,2),F(y,2)) for x,y in ((1,0),(0,1),(-1,0),(0,-1)))
        self.mesh(diamond,tolerance=F(1,16))

    def test_frames_actual_float_rounding_and_original_wall_band(self):
        for scale in (.25,.5,1.,2.,4.):
            with self.subTest(scale=scale):
                self.mesh(SQUARE,tolerance=F(1,32),height=F(1,4),scale=scale,origin=(.1,-.2,.3),
                          offset=(.0625,-.03125))
        tiny=math.ulp(0.)
        mesh=self.mesh(origin=(tiny,-tiny,tiny))
        rho=max(sum((abs(F.from_float(y)-x) for x,y in zip(v.position,v.rounded_position)),F(0)) for v in mesh.vertices)
        self.assertEqual(mesh.rounding_bound,rho);self.assertGreater(rho,0)
        mesh=self.mesh(maximum32(),height=F(511,256),scale=.5,cutter_scale=1.)
        self.assertEqual(mesh.body.wall_band,(F(1,1024),F(1023,1024)))

    def test_shared_boundary_original_cap_uv_and_both_seam_lifts(self):
        mesh=self.mesh(TRIANGLE);n=len(mesh.bore_result.feature.profile.vertices)
        for poly in mesh.edge_polylines:
            uses=[u for u in mesh.boundary_uses if u.edge_id==poly.edge_id]
            self.assertEqual(len(uses),2)
            self.assertEqual(uses[0].vertices,uses[1].vertices[::-1])
            self.assertEqual(uses[0].parameters,uses[1].parameters[::-1])
        seam=[u for u in mesh.boundary_uses if u.edge_id==3*n+3]
        self.assertEqual({uv[0] for u in seam for uv in u.uv},{F(0),F(4)})
        for t in mesh.triangles:
            for vid,corner in zip(t.vertices,t.corners):
                if t.face_id<=2:
                    p=mesh.vertices[vid].position;frame=mesh.body.frame
                    local=tuple((x-F(o))/F(frame.scale) for x,o in zip(p,frame.origin))
                    self.assertEqual(corner.uv,tuple((x+1)/2 for x in local[:2]))
                elif t.face_id==n+3:self.assertIn(corner.uv[1],mesh.body.wall_band)

    def test_budget_threshold_failure_retry_and_unchanged_sources(self):
        from brepgpu.feature_bore import difference
        from brepgpu.feature_bore_meshing import tessellate
        from brepgpu.meshing import MeshBudgetError
        result=difference(*operands());mesh=tessellate(result,tolerance=F(1,4))
        self.assertEqual(mesh.quarter_segments,1)
        refined=tessellate(result,tolerance=math.nextafter(.25,-math.inf))
        self.assertEqual(refined.quarter_segments,2)
        before=(result.feature.solid.vertices,result.cutter.patches,result.history.origins,mesh.vertices,mesh.triangles)
        with self.assertRaises(MeshBudgetError):tessellate(result,tolerance=F(1,4096),max_quarter_segments=1)
        for tol in (True,False,0,-1,math.nan,math.inf,Decimal('.1'),'1',None):
            with self.assertRaises((TypeError,ValueError)):tessellate(result,tolerance=tol)
        for budget in (True,0,-1,3,.5,None):
            with self.assertRaises((TypeError,ValueError)):tessellate(result,tolerance=1,max_quarter_segments=budget)
        self.assertEqual(before,(result.feature.solid.vertices,result.cutter.patches,result.history.origins,mesh.vertices,mesh.triangles))
        self.assertEqual(tessellate(result,tolerance=1).triangles,mesh.triangles)
        self.assertEqual(result.tessellate(tolerance=1).vertices,mesh.vertices)

    def test_factory_types_and_recursive_immutability(self):
        from brepgpu.feature_bore_meshing import BoreMesh,tessellate
        mesh=self.mesh()
        with self.assertRaises(TypeError):BoreMesh()
        for source in (mesh.body,mesh.bore_result.feature,mesh.bore_result.cutter,None,object()):
            with self.assertRaises(TypeError):tessellate(source,tolerance=1)
        for obj,name in ((mesh,'bore_result'),(mesh,'history'),(mesh.vertices[0],'position'),
                         (mesh.triangles[0],'vertices'),(mesh.boundary_uses[0],'uv')):
            with self.assertRaises((FrozenInstanceError,AttributeError,TypeError)):setattr(obj,name,None)

    def test_missing_duplicate_reversed_and_filled_hole_triangles(self):
        mesh=self.mesh(SQUARE,tolerance=1);ts=mesh.triangles
        self.reject(mesh,'complete source-domain triangulation count',triangles=ts[1:])
        a,b=[t for t in ts if t.face_id==2][:2];bad=list(ts);bad[b.id]=replace(a,id=b.id)
        self.reject(mesh,'full positive triangle boundary chain',triangles=tuple(bad))
        a=ts[0];flipped=replace(a,vertices=a.vertices[:1]+a.vertices[:0:-1],corners=a.corners[:1]+a.corners[:0:-1])
        self.reject(mesh,'triangle orientation/collapse',triangles=(flipped,)+ts[1:])
        # Replace a top-cap triangle by an exactly sourced triangle covering hole center.
        poly=mesh.edge_polylines[3*len(SQUARE)+1];ids=poly.vertices[:3]
        corners=tuple(replace(a.corners[0],chart_id=2,uv=tuple((x+1)/2 for x in mesh.vertices[i].position[:2])) for i in ids)
        fill=replace(b,vertices=ids,corners=corners);bad=list(ts);bad[b.id]=fill
        self.reject(mesh,'unbounded analytic hole',triangles=tuple(bad))

    def test_source_corner_seam_and_float_faults(self):
        mesh=self.mesh();t=mesh.triangles[0];v=mesh.vertices[0]
        bad=replace(t,corners=(replace(t.corners[0],chart_id=2),)+t.corners[1:])
        self.reject(mesh,'corner chart/face/domain',triangles=(bad,)+mesh.triangles[1:])
        bad=replace(t,corners=(replace(t.corners[0],uv=(F(0),F(0))),)+t.corners[1:])
        self.reject(mesh,'patch/corner UV',triangles=(bad,)+mesh.triangles[1:])
        self.reject(mesh,'original vertex source identity',vertices=(replace(v,source=('vertex',True)),)+mesh.vertices[1:])
        self.reject(mesh,'source generator differs from original',vertices=(replace(v,position=(v.position[0]+F(1,256),*v.position[1:])),)+mesh.vertices[1:])
        seam=next(u for u in mesh.boundary_uses if u.coedge_id==6*len(CHAMFER)+5)
        bad=replace(seam,uv=tuple((F(0),uv[1]) for uv in seam.uv))
        self.reject(mesh,'original cap/side/cutter occurrence UV',boundary_uses=tuple(bad if u is seam else u for u in mesh.boundary_uses))
        bad=replace(v,rounded_position=(math.nextafter(v.rounded_position[0],math.inf),*v.rounded_position[1:]))
        self.reject(mesh,'actual rounded vertex coordinates',vertices=(bad,)+mesh.vertices[1:])
        self.reject(mesh,'whole actual output error bound',error_bound=F(0))
        self.reject(mesh,'retained actual result/body/both histories',bore_result=self.mesh().bore_result)


if __name__=='__main__':unittest.main()
