"""Whole canonical exact/rounded tessellation contracts; product runs on fleet."""
from dataclasses import FrozenInstanceError,fields,replace
from decimal import Decimal
from fractions import Fraction as F
import math
import unittest


def altered(mesh,**changes):
    """Test-only altered real output; bypass publication, preserve public schema."""
    result=object.__new__(type(mesh))
    for field in fields(mesh):object.__setattr__(result,field.name,changes.get(field.name,getattr(mesh,field.name)))
    return result


class MeshingTests(unittest.TestCase):
    def mesh(self,body,tolerance=F(1,4),**options):
        from brepgpu.meshing import tessellate
        from brepgpu.meshing_oracles import qualify_mesh
        mesh=tessellate(body,tolerance=tolerance,**options)
        self.assertIs(mesh.body,body)
        self.assertEqual(mesh.tolerance,F(tolerance))
        self.assertIs(type(mesh.tolerance),F)
        quality=qualify_mesh(mesh)
        self.assertEqual(quality['faces'],len(body.faces))
        return mesh

    def test_literal_coarse_closed_shells_and_preserved_hole(self):
        from brepgpu.body import make_body
        from brepgpu.meshing_oracles import qualify_mesh
        for shape,vertices,triangles,edges,genus in (('box',8,12,18,0),('cylinder',10,16,24,0),('plate',16,32,48,1)):
            with self.subTest(shape=shape):
                body=make_body(shape);mesh=self.mesh(body,max_quarter_segments=1)
                self.assertEqual((len(mesh.vertices),len(mesh.triangles)),(vertices,triangles))
                quality=qualify_mesh(mesh)
                self.assertEqual((quality['mesh_edges'],quality['genus']),(edges,genus))
                self.assertEqual(set(t.face_id for t in mesh.triangles),set(f.id for f in body.faces))
                self.assertEqual(mesh.quarter_segments,0 if shape=='box' else 1)
                self.assertEqual(mesh.rounding_bound,F(0))
        plate=self.mesh(make_body('plate'),tolerance=4,max_quarter_segments=1)
        self.assertEqual((len(plate.vertices),len(plate.triangles)),(16,32))
        self.assertEqual(sum(t.face_id==1 for t in plate.triangles),8)
        self.assertEqual(sum(t.face_id==2 for t in plate.triangles),8)
        # Even an enormous geometric tolerance keeps the complete through-hole.
        self.assertEqual(len([u for u in plate.boundary_uses if u.face_id in (1,2) and len(u.parameters)>2]),2)

    def test_all_frame_scales_translations_and_world_chord_budget(self):
        from brepgpu.body import Frame,make_body
        for scale in (.25,.5,1.,2.,4.):
            for shape in ('box','cylinder','plate'):
                with self.subTest(shape=shape,scale=scale):
                    body=make_body(shape,Frame((16.,-16.,.125),scale))
                    mesh=self.mesh(body,tolerance=F(scale)/16,max_quarter_segments=2)
                    self.assertEqual(mesh.quarter_segments,0 if shape=='box' else 2)
                    self.assertLessEqual(mesh.error_bound,F(scale)/16)
                    self.assertEqual(mesh.vertices[0].position[2],F(1,8)-F(scale)/2)

    def test_exact_threshold_just_below_and_fine_mesh(self):
        from brepgpu.body import make_body
        from brepgpu.meshing import MeshBudgetError,tessellate
        body=make_body('cylinder')
        exact=self.mesh(body,tolerance=F(1,16),max_quarter_segments=2)
        self.assertEqual(exact.quarter_segments,2)
        self.assertEqual(exact.curve_bound,F(1,32))
        below=math.nextafter(1/16,0.)
        finer=self.mesh(body,tolerance=below,max_quarter_segments=4)
        self.assertEqual(finer.tolerance,F.from_float(below))
        self.assertEqual(finer.quarter_segments,4)
        with self.assertRaises(MeshBudgetError):tessellate(body,tolerance=below,max_quarter_segments=2)
        fine=self.mesh(make_body('plate'),tolerance=F(1,512),max_quarter_segments=16)
        self.assertEqual((fine.quarter_segments,len(fine.vertices),len(fine.triangles)),(16,136,272))
        self.assertGreater(fine.rounding_bound,0)
        self.assertLessEqual(fine.error_bound,F(1,512))

    def test_subnormal_original_frames_and_actual_rounding_budget(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.meshing import MeshBudgetError,tessellate
        tiny=math.ulp(0.)
        for shape in ('box','cylinder','plate'):
            body=make_body(shape,Frame((tiny,-tiny,tiny),1.))
            mesh=self.mesh(body)
            self.assertGreater(mesh.rounding_bound,0)
            self.assertTrue(any(tuple(F(x) for x in v.rounded_position)!=v.position for v in mesh.vertices))
            self.assertEqual(mesh.error_bound,mesh.curve_bound+mesh.rounding_bound)
        box=make_body('box',Frame((tiny,0.,0.),1.))
        exact=self.mesh(box,tolerance=F(tiny))
        self.assertEqual(exact.rounding_bound,F(tiny))
        with self.assertRaises(MeshBudgetError):tessellate(box,tolerance=F(tiny)/2)

    def test_original_rims_seam_occurrences_and_chart_local_corners(self):
        from brepgpu.body import make_body
        for shape in ('cylinder','plate'):
            body=make_body(shape);mesh=self.mesh(body,tolerance=F(1,16),max_quarter_segments=2)
            wall=body.faces[-1];seam=[u for u in mesh.boundary_uses if body.coedges[u.coedge_id-1].uv_map.kind=='seam_wall']
            self.assertEqual(len(seam),2)
            self.assertEqual(seam[0].edge_id,seam[1].edge_id)
            self.assertEqual(seam[0].vertices,seam[1].vertices[::-1])
            self.assertEqual({uv[0] for use in seam for uv in use.uv},{F(0),F(4)})
            self.assertEqual({use.orientation for use in seam},{-1,1})
            for edge in mesh.edge_polylines:
                if body.curves[body.edges[edge.edge_id-1].curve_slot].kind=='circle':
                    self.assertEqual(edge.vertices[0],edge.vertices[-1])
                    self.assertEqual((edge.parameters[0],edge.parameters[-1]),(F(0),F(1)))
                    uses=[u for u in mesh.boundary_uses if u.edge_id==edge.edge_id]
                    self.assertEqual(len(uses),2)
                    self.assertEqual(set(uses[0].vertices),set(uses[1].vertices))
            seam_ids=set(seam[0].vertices)
            observed={vid:set() for vid in seam_ids}
            for t in mesh.triangles:
                if t.face_id==wall.id:
                    for vid,c in zip(t.vertices,t.corners):
                        self.assertTrue(0<=c.uv[0]<=1 and 0<=c.uv[1]<=1)
                        if vid in observed:observed[vid].add((body.charts[c.chart_id-1].quadrant,c.uv[0]))
            self.assertTrue(all({(0,F(0)),(3,F(1))}<=values for values in observed.values()))

    def test_source_line_parameters_and_all_original_face_orientations(self):
        from brepgpu.body import make_body
        mesh=self.mesh(make_body('plate'),tolerance=F(1,16))
        self.assertEqual({u.coedge_id for u in mesh.boundary_uses},set(range(1,31)))
        self.assertEqual({t.face_id for t in mesh.triangles},set(range(1,8)))
        for use in mesh.boundary_uses:
            if use.orientation>0:self.assertEqual((use.parameters[0],use.parameters[-1]),(F(0),F(1)))
            else:self.assertEqual((use.parameters[0],use.parameters[-1]),(F(1),F(0)))
        bottom=[t for t in mesh.triangles if t.face_id==2]
        for triangle in bottom:
            a,b,c=[mesh.vertices[i].position for i in triangle.vertices]
            self.assertLess((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]),0)

    def test_invalid_requests_budget_failure_and_healthy_retry(self):
        from brepgpu.body import make_body
        from brepgpu.meshing import MeshBudgetError,tessellate
        body=make_body('plate')
        for value in (True,False,0,-1,float('nan'),float('inf'),-float('inf'),Decimal('.1'),'0.1',None,()):
            with self.subTest(tolerance=value):
                with self.assertRaises((TypeError,ValueError)):tessellate(body,tolerance=value)
        for budget in (True,False,0,-1,3,1025,2048,1.,F(1),None):
            with self.subTest(budget=budget):
                with self.assertRaises((TypeError,ValueError)):tessellate(body,tolerance=F(1,4),max_quarter_segments=budget)
        with self.assertRaises(MeshBudgetError):tessellate(body,tolerance=F(1,2**80),max_quarter_segments=1024)
        with self.assertRaises(TypeError):tessellate(object(),tolerance=F(1,4))
        healthy=self.mesh(body,max_quarter_segments=1)
        self.assertIs(healthy.body,body)

    def test_factory_only_recursive_immutability_and_body_adapter(self):
        from brepgpu.body import make_body
        from brepgpu.meshing import Mesh,tessellate
        from brepgpu.meshing_oracles import qualify_mesh
        body=make_body('cylinder');tol=F(1,16)
        mesh=body.tessellate(tolerance=tol,max_quarter_segments=2)
        self.assertIs(mesh.body,body);self.assertEqual(mesh.tolerance,tol);qualify_mesh(mesh)
        direct=tessellate(body,tolerance=tol,max_quarter_segments=2)
        self.assertEqual(mesh,direct)
        with self.assertRaises(TypeError):Mesh()
        for obj,name,value in ((mesh,'tolerance',F(1)),(mesh.vertices[0],'position',(F(0),)*3),
                               (mesh.triangles[0],'vertices',(0,0,0)),(mesh.triangles[0].corners[0],'uv',(F(0),F(0))),
                               (mesh.edge_polylines[0],'parameters',()),(mesh.boundary_uses[0],'uv',())):
            with self.subTest(record=type(obj).__name__):
                with self.assertRaises((FrozenInstanceError,AttributeError,TypeError)):setattr(obj,name,value)
        with self.assertRaises(TypeError):mesh.vertices[0].position[0]=F(1)

    def test_actual_missing_duplicate_reversed_and_filled_hole_outputs(self):
        from brepgpu.body import make_body
        from brepgpu.meshing import MeshCorner,MeshTriangle
        from brepgpu.meshing_oracles import qualify_mesh
        mesh=self.mesh(make_body('plate'),max_quarter_segments=1)
        missing=tuple(replace(t,id=i) for i,t in enumerate(mesh.triangles[1:]))
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,triangles=missing))
        same=[t for t in mesh.triangles if t.face_id==1];a,b=same[:2]
        duplicate=list(mesh.triangles);duplicate[b.id]=replace(a,id=b.id)
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,triangles=tuple(duplicate)))
        flip=list(mesh.triangles);t=flip[0];flip[0]=replace(t,vertices=t.vertices[:1]+t.vertices[:0:-1],corners=t.corners[:1]+t.corners[:0:-1])
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,triangles=tuple(flip)))
        rim=next(u for u in mesh.boundary_uses if u.face_id==1 and len(u.parameters)>2)
        ids=rim.vertices[:3]
        # Hole use is clockwise on the top; reverse it to add a positive material triangle.
        ids=ids[::-1];chart=mesh.body.charts[mesh.body.faces[0].chart_slots[0]]
        corners=tuple(MeshCorner(chart.id,((mesh.vertices[i].position[0]+F(3,4))/F(3,2),
                                          (mesh.vertices[i].position[1]+F(3,4))/F(3,2))) for i in ids)
        extra=MeshTriangle(len(mesh.triangles),ids,1,corners)
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,triangles=mesh.triangles+(extra,)))

    def test_actual_source_seam_and_rounded_output_corruptions(self):
        from brepgpu.body import make_body
        from brepgpu.meshing_oracles import qualify_mesh
        mesh=self.mesh(make_body('cylinder'),tolerance=F(1,16))
        use=next(u for u in mesh.boundary_uses if any(uv[0]==4 for uv in u.uv))
        wrong=replace(use,uv=tuple((F(0),uv[1]) for uv in use.uv))
        uses=tuple(wrong if u.coedge_id==use.coedge_id else u for u in mesh.boundary_uses)
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,boundary_uses=uses))
        poly=next(p for p in mesh.edge_polylines if len(p.parameters)>2)
        parameters=list(poly.parameters);parameters[1]=F(1,7)
        wrong_poly=replace(poly,parameters=tuple(parameters))
        polylines=tuple(wrong_poly if p.edge_id==poly.edge_id else p for p in mesh.edge_polylines)
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,edge_polylines=polylines))
        bad=list(mesh.vertices);v=bad[2]
        bad[2]=replace(v,rounded_position=(math.nextafter(v.rounded_position[0],math.inf),*v.rounded_position[1:]))
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,vertices=tuple(bad)))
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,error_bound=F(0)))
        # Source duplicate at a shared rim is rejected even if its position is identical.
        repeated=replace(mesh.vertices[0],id=len(mesh.vertices))
        with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,vertices=mesh.vertices+(repeated,)))
        # Native typed identities reject Python's bool/int/float equality aliases.
        aliases=[(mesh.vertices[0],('vertex',True))]
        edge_vertex=next(v for v in mesh.vertices if v.source[0]=='edge')
        aliases.append((edge_vertex,('edge',edge_vertex.source[1],float(edge_vertex.source[2]))))
        for vertex,source in aliases:
            bad=tuple(replace(v,source=source) if v.id==vertex.id else v for v in mesh.vertices)
            with self.assertRaises(AssertionError):qualify_mesh(altered(mesh,vertices=bad))
        p=mesh.edge_polylines[0]
        bad_poly=replace(p,vertices=(float(p.vertices[0]),)+p.vertices[1:])
        with self.assertRaises(AssertionError):
            qualify_mesh(altered(mesh,edge_polylines=(bad_poly,)+mesh.edge_polylines[1:]))
        u=mesh.boundary_uses[0]
        for name,value in (('face_id',True),('edge_id',float(u.edge_id)),('orientation',float(u.orientation)),
                           ('vertices',(float(u.vertices[0]),)+u.vertices[1:])):
            bad_use=replace(u,**{name:value})
            with self.assertRaises(AssertionError):
                qualify_mesh(altered(mesh,boundary_uses=(bad_use,)+mesh.boundary_uses[1:]))


if __name__=='__main__':unittest.main()
