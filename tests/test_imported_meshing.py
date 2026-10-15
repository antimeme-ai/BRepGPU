from fractions import Fraction
from dataclasses import replace
import math
import unittest

from brepgpu.imported_source import (Curve,CurveSpan,Carrier,SurfaceSpan,ZERO_FRAME,ImportedSource,
    Product,SolidDefinition,Vertex,Edge,Coedge,Loop,Face)
from brepgpu.imported_meshing import evaluate_curve,evaluate_carrier,_triangulate,_orient,_surface_error,_positive_midpoint,_recover_global_uv,_uv_rounding_bound,_guard_carrier_refinement,_cylinder_uv,_curve_derivative_bounds,_cylinder_trim_bound,_curve_interval_span,_plane_uv,_cone_uv,_cone_generator,_source_uv,_curve_scalar_range,_cone_v_envelope,MeshQuality,mesh_imported


def sphere_cap_fixture():
    frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
    circle=Curve(101,3,2,frame,(1.,1.),(0.,2*math.pi))
    meridian=Curve(102,3,2,(0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,-1.,0.),(1.,1.),(0.,math.pi/2))
    def line(identifier,origin,direction,interval):
        return Curve(identifier,2,1,(*origin,0.,*direction,0.,0.,0.,0.,0.,0.,0.),(0.,0.),interval)
    uv=(line(2**63+11,(0.,0.),(1.,0.),(0.,2*math.pi)),
        line(2**63+12,(2*math.pi,0.),(0.,1.),(0.,math.pi/2)),
        line(2**63+13,(0.,0.),(0.,1.),(0.,math.pi/2)))
    return ImportedSource('cd'*32,1.,0.,10,'independently authored upper hemisphere',
        (Product(10,'hemisphere fixture','fixture',20),),
        (SolidDefinition(20,10,(30,),(40,41),(50,51)),),(),
        (Vertex(50,(1.,0.,0.),0.),Vertex(51,(0.,0.,1.),0.)),
        (Edge(40,(50,50),101,(0.,2*math.pi),True,True,False,0.),
         Edge(41,(50,51),102,(0.,math.pi/2),True,True,False,0.)),
        (Coedge(11,40,60,1,uv[0].id,(0.,2*math.pi)),
         Coedge(12,41,60,1,uv[1].id,(0.,math.pi/2)),
         Coedge(13,41,60,-1,uv[2].id,(0.,math.pi/2))),
        (Loop(60,30,True,(11,12,13)),),
        (Face(30,20,70,1,(60,),0.),),
        (Carrier(70,4,frame,(1.,0.),(0.,2*math.pi,0.,math.pi/2)),),
        (circle,meridian,*uv))


def exact_triangle_area(points,triangles):
    total=Fraction(0)
    for tri in triangles:
        a,b,c=[tuple(Fraction(x) for x in points[i]) for i in tri]
        area=((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))/2
        if area<=0:raise AssertionError('nonpositive actual triangle')
        total+=area
    return total


class ImportedMeshingTests(unittest.TestCase):
    def test_degree_five_boundary_is_not_reduced_to_cubic(self):
        # Bernstein controls [0,0,0,0,0,1] are exactly x(t)=t**5.
        controls=tuple(v for i in range(6) for v in (float(i==5),0.0,0.0,1.0))
        c=Curve(1,3,4,ZERO_FRAME,(0,0),(0,1),5,controls=controls)
        for t in (0.0,0.125,0.5,0.875,1.0):
            self.assertEqual(evaluate_curve(c,t),(t**5,0.0,0.0))

    def test_original_span_parameter_and_partials_survive(self):
        def controls(a,b,za,zb):
            return tuple(x for u,z in ((a,za),(b,zb)) for v in (0.,1.) for x in (u,v,z,1.))
        left=SurfaceSpan((0.,0.5,0.,1.),(1,1),controls(0.,0.5,0.,0.))
        right=SurfaceSpan((0.5,1.,0.,1.),(1,1),controls(0.5,1.,0.,0.5))
        c=Carrier(1,6,ZERO_FRAME,(0,0),(0,1,0,1),(1,1),spans=(left,right))
        for u,v,z,slope in ((0.25,0.75,0.,0.),(0.75,0.25,0.25,1.)):
            p,du,dv=evaluate_carrier(c,u,v)
            self.assertEqual(p,(u,v,z));self.assertEqual(du,(1.,0.,slope));self.assertEqual(dv,(0.,1.,0.))

    def test_visible_bridge_handles_eberly_nearest_reflex_counterexample(self):
        pts=[(0.,5.),(9.,5.),(7.,4.),(8.,4.),(10.,0.),(10.,8.),(0.,8.),(1.,6.),(1.,7.),(2.,6.)]
        points=dict(enumerate(pts));triangles=_triangulate([list(range(7)),[7,8,9]],points)
        self.assertEqual(exact_triangle_area(points,triangles),Fraction(71,2))
        # The hole cannot be swallowed despite the concave outer notch.
        for tri in triangles:
            p=(Fraction(4,3),Fraction(19,3));a,b,c=[points[i] for i in tri]
            self.assertFalse(_orient(a,b,p)>0 and _orient(b,c,p)>0 and _orient(c,a,p)>0)

    def test_many_holes_are_preserved_at_coarse_quality(self):
        points={0:(0.,0.),1:(20.,0.),2:(20.,20.),3:(0.,20.)};loops=[[0,1,2,3]]
        for i in range(55):
            x,y=1.+3*(i%6),1.+2*(i//6);k=len(points)
            for j,p in enumerate(((x-.125,y-.125),(x-.125,y+.125),(x+.125,y+.125),(x+.125,y-.125))):points[k+j]=p
            loops.append([k,k+1,k+2,k+3])
        triangles=_triangulate(loops,points)
        self.assertEqual(exact_triangle_area(points,triangles),Fraction(6345,16))
        self.assertEqual(len(triangles),332)

    def test_invalid_quality_raises(self):
        for kwargs in ({'tolerance_mm':0},{'source_residual_mm':-1},{'max_refinement':True},{'max_edge_samples':3}):
            with self.assertRaises(ValueError):MeshQuality(**kwargs)

    def test_sharp_interpolation_bound_reaches_quadratic_and_mixed_limits(self):
        quadratic=SurfaceSpan((0.,1.,0.,1.),(2,1),
            tuple(x for u,z in ((0.,0.),(.5,0.),(1.,1.)) for v in (0.,1.) for x in (u,v,z,1.)))
        q=Carrier(1,6,ZERO_FRAME,(0.,0.),(0.,1.,0.,1.),spans=(quadratic,))
        self.assertEqual(_surface_error(q,((0.,0.),(1.,0.),(0.,1.)),{}),.25)
        mixed=SurfaceSpan((0.,1.,0.,1.),(1,1),
            tuple(x for u in (0.,1.) for v in (0.,1.) for x in (u,v,u*v,1.)))
        m=replace(q,spans=(mixed,))
        self.assertEqual(_surface_error(m,((0.,0.),(1.,1.),(1.,0.)),{}),.25)

    def test_unproved_continuation_map_is_rejected(self):
        source=sphere_cap_fixture()
        source=replace(source,edges=(replace(source.edges[0],same_parameter=False),source.edges[1]),
            coedges=(replace(source.coedges[0],parameter_map_kind=3,
                             parameter_map=((0.,0.),(2*math.pi,2*math.pi))),*source.coedges[1:]))
        with self.assertRaisesRegex(ValueError,'unproved non-SameParameter'):
            mesh_imported(source)

    def test_mesh_does_not_assume_non_mm_parameters_are_millimetres(self):
        with self.assertRaisesRegex(ValueError,'millimetre source parameters'):
            mesh_imported(replace(sphere_cap_fixture(),units_mm=10.))

    def test_c1_crossed_knot_uses_uniform_hessian_without_uv_snapping(self):
        def controls(xs,zs):
            return tuple(x for u,z in zip(xs,zs) for v in (0.,1.) for x in (u,v,z,1.))
        a=SurfaceSpan((-2.,-1.,0.,1.),(2,1),controls((-2.,-1.5,-1.),(4.,2.,1.)))
        b=SurfaceSpan((-1.,0.,0.,1.),(2,1),controls((-1.,-.5,0.),(1.,0.,0.)))
        c=Carrier(1,6,ZERO_FRAME,(0.,0.),(-2.,0.,0.,1.),degrees=(2,1),
                  knot_u=(-2.,-1.,0.),mult_u=(3,1,3),knot_v=(0.,1.),mult_v=(2,2),spans=(a,b))
        uvs=((-1.25,.15),(-1.+2**-48,.15),(-1.1,.157))
        bound=_surface_error(c,uvs,{})
        self.assertAlmostEqual(bound,(uvs[1][0]-uvs[0][0])**2/4,places=15)
        self.assertGreater(uvs[1][0],-1.)

    def test_genuine_c0_crossed_knot_retains_nonzero_bound(self):
        def controls(a,b,za,zb):
            return tuple(x for u,z in ((a,za),(b,zb)) for v in (0.,1.) for x in (u,v,z,1.))
        a=SurfaceSpan((0.,.5,0.,1.),(1,1),controls(0.,.5,0.,0.))
        b=SurfaceSpan((.5,1.,0.,1.),(1,1),controls(.5,1.,0.,.5))
        c=Carrier(1,6,ZERO_FRAME,(0.,0.),(0.,1.,0.,1.),degrees=(1,1),
                  knot_u=(0.,.5,1.),mult_u=(2,1,2),knot_v=(0.,1.),mult_v=(2,2),spans=(a,b))
        self.assertGreater(_surface_error(c,((.25,0.),(.75,0.),(.5,1.)),{}),.1)

    def test_adjacent_binary64_rows_require_exact_positive_midpoint(self):
        low=math.nextafter(.15,-math.inf)
        a=(-8.55796106604621e-15,.15);b=(1.4999999999999991,low)
        c=(.999999999999999,low);d=(1.9999999999999991,low)
        parents=((a,c,b),(a,b,d))
        self.assertTrue(all(_orient(*p)>0 for p in parents))
        rounded=((a[0]+b[0])/2,(a[1]+b[1])/2)
        self.assertLessEqual(_orient(a,c,rounded),0)
        midpoint=_positive_midpoint(a,b,((b,a,c),(a,b,d)))
        self.assertTrue(any(isinstance(x,Fraction) for x in midpoint))
        for x,y,z in ((b,a,c),(a,b,d)):
            self.assertGreater(_orient(x,midpoint,z),0)
            self.assertGreater(_orient(midpoint,y,z),0)
        self.assertEqual(_orient(a,midpoint,b),0)
        self.assertGreater(_uv_rounding_bound(midpoint,(1.,1.)),0.)
        self.assertLess(_uv_rounding_bound(midpoint,(1.,1.)),1e-15)

    def test_foreign_global_end_recovery_is_bounded_and_never_snaps_interior(self):
        carrier=Carrier(1,6,ZERO_FRAME,(0.,0.),(0.,2.,0.,1.),degrees=(2,1),
                        knot_u=(0.,1.,2.),mult_u=(3,1,3),knot_v=(0.,1.),mult_v=(2,2))
        raw=(-8.55796106604621e-15,.15)
        self.assertEqual(_recover_global_uv(carrier,raw),(0.,.15))
        self.assertLess(raw[0],0.)
        near=math.nextafter(1.,math.inf)
        self.assertEqual(_recover_global_uv(carrier,(near,.15)),(near,.15))
        with self.assertRaisesRegex(ValueError,'outside global recovery grade'):
            _recover_global_uv(carrier,(-1e-10,.15))
        with self.assertRaisesRegex(ValueError,'outside global recovery grade'):
            _recover_global_uv(carrier,(1.,1.+1e-10))

    def test_metric_resolution_and_independent_work_cap_fail_closed(self):
        _guard_carrier_refinement(1.,16.,20,100)
        with self.assertRaisesRegex(ValueError,'metric resolution exhausted'):
            _guard_carrier_refinement(math.ldexp(16.,-20),16.,20,0)
        with self.assertRaisesRegex(ValueError,'work cap exhausted'):
            _guard_carrier_refinement(1.,16.,20,2**20)
        with self.assertRaisesRegex(ValueError,'invalid carrier refinement metric'):
            _guard_carrier_refinement(1.,math.inf,20,0)

    def test_original_cylinder_inverse_retains_branch_and_rejects_ambiguity(self):
        frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
        cylinder=Carrier(1,2,frame,(2.,0.),(0.,4*math.pi,-1.,1.))
        uv=_cylinder_uv(cylinder,(0.,2.,.25),(5*math.pi/2,.3))
        self.assertEqual(uv,(5*math.pi/2,.25))
        skew=replace(cylinder,frame=(0.,0.,0.,2.,0.,0.,1.,1.,0.,.5,0.,-3.))
        point=evaluate_carrier(skew,.7,.2)[0]
        actual=_cylinder_uv(skew,point,(.7,.2))
        self.assertAlmostEqual(actual[0],.7,places=15);self.assertAlmostEqual(actual[1],.2,places=15)
        with self.assertRaisesRegex(ValueError,'axis ambiguity'):_cylinder_uv(cylinder,(0.,0.,0.),(0.,0.))
        with self.assertRaisesRegex(ValueError,'branch ambiguity'):_cylinder_uv(cylinder,(2.,0.,0.),(math.pi,0.))

    def test_original_degree_five_derivative_bounds_and_cylinder_trim(self):
        controls=tuple(v for i in range(6) for v in (float(i==5),0.,0.,1.))
        curve=Curve(1,3,4,ZERO_FRAME,(0.,0.),(0.,1.),5,controls=controls)
        self.assertEqual(_curve_derivative_bounds(curve,0.,1.),(5.,20.))
        self.assertEqual(_curve_derivative_bounds(curve,0.,.5),(5./16,2.5))
        frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
        cylinder=Carrier(2,2,frame,(2.,0.),(0.,2*math.pi,-1.,1.))
        circle=Curve(3,3,2,frame,(2.,2.),(0.,2*math.pi))
        a,b=0.,.01;pa,pb=evaluate_curve(circle,a),evaluate_curve(circle,b)
        bound=_cylinder_trim_bound(circle,cylinder,a,b,(a,0.),(b,0.),pa,pb,(4.,1.))
        self.assertGreater(bound,0.)
        self.assertLess(bound,.001)
        # A full turn cannot be justified by a single local inverse interval.
        self.assertEqual(_cylinder_trim_bound(circle,cylinder,0.,2*math.pi,(0.,0.),(2*math.pi,0.),pa,pa,(4.,1.)),math.inf)
        # Same XYZ on an incompatible source lift cannot pass branch continuity.
        self.assertEqual(_cylinder_trim_bound(circle,cylinder,a,b,(a,0.),(b+2*math.pi,0.),pa,pb,(4.,1.)),math.inf)

    def test_adjacent_double_interval_selects_containing_original_curve_span(self):
        left=CurveSpan((0.,1.),1,(0.,0.,0.,1.,1.,0.,0.,1.))
        right=CurveSpan((1.,2.),1,(1.,0.,0.,1.,2.,1.,0.,1.))
        curve=Curve(1,3,5,ZERO_FRAME,(0.,0.),(0.,2.),1,spans=(left,right))
        a,b=1.,math.nextafter(1.,math.inf)
        self.assertEqual((a+b)/2,a)
        self.assertIs(_curve_interval_span(curve,a,b),right)
        with self.assertRaisesRegex(ValueError,'crosses original spans'):
            _curve_interval_span(curve,.5,1.5)

    def test_exact_oblique_plane_inverse_preserves_original_collinearity(self):
        c,s=math.cos(.3),math.sin(.3)
        plane=Carrier(1,1,(1.7,.5,-2.1,c,0.,s,0.,1.,0.,-s,0.,c),(0.,0.),(-20.,20.,-20.,20.))
        points=((3.14824785219045,-.87,-11.7860620895614),
                (3.14824785219045,6.6441495727456e-17,-11.7860620895614),
                (3.14824785219045,-.3,-11.7860620895614))
        uv=tuple(_plane_uv(plane,p) for p in points)
        self.assertTrue(all(type(x) is Fraction for p in uv for x in p))
        self.assertEqual(_orient(*uv),0)
        self.assertEqual(uv[0][0],uv[1][0]);self.assertEqual(uv[1][0],uv[2][0])

    def test_cone_inverse_retains_slant_signed_sheet_and_zero_reference_radius(self):
        frame=(0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,1.,0.)
        cone=Carrier(1,3,frame,(.3,math.pi/3),(0.,2*math.pi,-1.,1.))
        for u,v in ((.7,-.1),(.7,-1.),(.7,.2)):
            p=evaluate_carrier(cone,u,v)[0];actual=_cone_uv(cone,p,(u,v))
            self.assertAlmostEqual(actual[0],u,places=14);self.assertAlmostEqual(actual[1],v,places=14)
        zero=replace(cone,radii=(0.,math.pi/3))
        actual=_cone_uv(zero,evaluate_carrier(zero,.7,.2)[0],(.7,.2))
        self.assertAlmostEqual(actual[0],.7,places=14);self.assertAlmostEqual(actual[1],.2,places=14)
        with self.assertRaisesRegex(ValueError,'axis ambiguity'):_cone_uv(zero,(0.,0.,0.),(0.,0.))

    def test_source_active_axial_ranges_ignore_foreign_cone_v_bounds(self):
        frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
        circle=Curve(1,3,2,frame,(2.,2.),(0.,2*math.pi))
        lo,hi=_curve_scalar_range(circle,math.pi/4,3*math.pi/4,(0.,1.,0.),(0.,0.,0.))
        self.assertLessEqual(lo,math.sqrt(2));self.assertGreaterEqual(hi,2.)
        self.assertLess(hi,2.00000000000001)
        source=sphere_cap_fixture()
        cone=Carrier(70,3,frame,(.3,math.pi/3),(0.,2*math.pi,-1.,-.9))
        v0,v1=_cone_v_envelope(source,cone,(40,41))
        self.assertLessEqual(v0,0.);self.assertGreaterEqual(v1,1/math.cos(math.pi/3))
        self.assertGreater(v1,cone.uv_bounds[3])

    def test_original_line_cone_generator_has_whole_edge_endpoint_residual_proof(self):
        source=sphere_cap_fixture();frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
        cone=Carrier(70,3,frame,(.3,math.pi/3),(0.,2*math.pi,-1.,1.))
        u=.7;v0=.2;v1=-.3/math.sin(math.pi/3)
        a,b=[evaluate_carrier(cone,u,v)[0] for v in (v0,v1)]
        direction=tuple(y-x for x,y in zip(a,b))
        line=Curve(999,3,1,(*a,*direction,0.,0.,0.,0.,0.,0.),(0.,0.),(0.,1.))
        edge=Edge(777,(50,51),999,(0.,1.),True,True,False,0.)
        source=replace(source,curves=(*source.curves,line))
        profile=_cone_generator(source,edge,cone,MeshQuality(),{})
        self.assertIsNotNone(profile);self.assertLess(profile[1],2**-32)
        for t in (.125,.5,.875):
            xyz=evaluate_curve(line,t)
            uv,kind=_source_uv(source,edge,cone,t,xyz,(u,0.),MeshQuality(),{})
            self.assertEqual(kind,'cone_source_generator')
            self.assertLess(math.dist(evaluate_carrier(cone,*uv)[0],xyz),2**-32)
        apex_uv,kind=_source_uv(source,edge,cone,1.,b,(u,0.),MeshQuality(),{})
        self.assertEqual(kind,'cone_source_generator')
        self.assertEqual(apex_uv[1],-.3/math.sin(math.pi/3))
        bad=replace(line,frame=(*line.frame[:3],direction[0]+.1,*direction[1:],*line.frame[6:]))
        self.assertIsNone(_cone_generator(replace(source,curves=tuple(bad if c.id==999 else c for c in source.curves)),edge,cone,MeshQuality(),{}))

    def test_original_sphere_pole_needs_generated_chart_connector(self):
        mesh=mesh_imported(sphere_cap_fixture(),MeshQuality(tolerance_mm=.05))
        self.assertEqual(len(mesh.faces[0].chart_closures),1)
        closure=mesh.faces[0].chart_closures[0]
        self.assertEqual((closure.source_vertex_id,closure.incoming_coedge_id,closure.outgoing_coedge_id),(51,12,13))
        self.assertEqual(closure.kind,'sphere_pole')
        self.assertTrue(any(t.collapsed for t in mesh.triangles))
        self.assertTrue(all(len(set(t.vertex_keys))==3 for t in mesh.triangles if not t.collapsed))
        self.assertLessEqual(dict(mesh.error_metrics)['max_surface_interpolation_bound_mm'],.05*.75)
        # Complete UV rectangle despite its top boundary's physical quotient.
        area=math.fsum(abs((t.corners[1].uv[0]-t.corners[0].uv[0])*(t.corners[2].uv[1]-t.corners[0].uv[1])
                          -(t.corners[1].uv[1]-t.corners[0].uv[1])*(t.corners[2].uv[0]-t.corners[0].uv[0]))/2
                        for t in mesh.triangles)
        self.assertAlmostEqual(area,math.pi**2,places=11)


if __name__=='__main__':unittest.main()
