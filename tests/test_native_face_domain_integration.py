"""Actual imported mesh consumers of the native domain engine; fleet only."""
from unittest import TestCase, mock
import math

from brepgpu import face_domain
from brepgpu.imported_meshing import mesh_imported, MeshQuality
from brepgpu.imported_oracles import qualify_domain, qualify_mesh
from tests.test_imported_geometry import pierced_prism_source
from tests.test_imported_meshing import (tangent_circle_sheet_fixture,
                                        sphere_cap_fixture, spline_column_fixture)


class NativeFaceDomainIntegration(TestCase):
    def test_actual_two_hole_solid_qualifies_with_all_explicit_engines(self):
        source = pierced_prism_source()
        quality = MeshQuality(tolerance_mm=.005, source_residual_mm=1.e-8)
        for backend in ('native', 'python', 'direct'):
            with self.subTest(backend=backend):
                mesh = mesh_imported(source, quality, domain_backend=backend)
                answer = qualify_mesh(mesh)
                self.assertEqual((answer['definitions'], answer['faces'], answer['solid_eulers']),
                                 (1, 16, (-2,)))
                self.assertEqual(mesh.bounds(9000), (0., 0., 0., 4., 4., .25))

    def test_default_native_routes_circle_preflight_and_actual_triangulation(self):
        source = tangent_circle_sheet_fixture()
        with mock.patch.object(face_domain, 'first_crossing', wraps=face_domain.first_crossing) as crossing, \
             mock.patch.object(face_domain, 'triangulate', wraps=face_domain.triangulate) as triangulate:
            mesh = mesh_imported(source)
        self.assertEqual(len(mesh.faces), 1)
        self.assertGreaterEqual(crossing.call_count, 1)
        self.assertEqual(triangulate.call_count, 1)

    def test_direct_routes_circle_preflight_metric_and_skips_old_lawson(self):
        source = tangent_circle_sheet_fixture()
        with mock.patch.object(face_domain, 'first_crossing', wraps=face_domain.first_crossing) as crossing, \
             mock.patch.object(face_domain, 'constrained_delaunay', wraps=face_domain.constrained_delaunay) as construct, \
             mock.patch('brepgpu.imported_meshing._improve_connectivity', side_effect=AssertionError('old Lawson called')):
            mesh = mesh_imported(source, domain_backend='direct')
        self.assertEqual(len(mesh.faces), 1)
        qualify_domain(tuple(tuple(o.uv for o in loop) for loop in mesh.faces[0].boundary_loops),
                       tuple(tuple(c.uv for c in t.corners) for t in mesh.triangles))
        self.assertGreaterEqual(crossing.call_count, 1)
        self.assertEqual(construct.call_count, 1)
        self.assertEqual(construct.call_args.kwargs['seed'], 0)
        self.assertEqual(len(construct.call_args.kwargs['metric']), 2)

    def test_direct_keeps_curved_pole_and_spline_domains_with_original_metrics(self):
        for source, expected_metric in ((sphere_cap_fixture(), (2., 3.)),
                                        (spline_column_fixture(), (math.sqrt(2.), 2.))):
            with self.subTest(carrier=source.carriers[0].kind), \
                 mock.patch.object(face_domain, 'constrained_delaunay', wraps=face_domain.constrained_delaunay) as construct, \
                 mock.patch('brepgpu.imported_meshing._improve_connectivity', side_effect=AssertionError('old Lawson called')):
                mesh = mesh_imported(source, MeshQuality(tolerance_mm=.05), domain_backend='direct')
                self.assertEqual(construct.call_args.kwargs['metric'], expected_metric)
                qualify_domain(tuple(tuple(o.uv for o in loop) for loop in mesh.faces[0].boundary_loops),
                               tuple(tuple(c.uv for c in t.corners) for t in mesh.triangles))
                if source.carriers[0].kind == 4:
                    self.assertEqual(len(mesh.faces[0].chart_closures), 1)
                    self.assertTrue(any(t.collapsed for t in mesh.triangles))
                    for vertex in mesh.vertices:
                        self.assertAlmostEqual(math.sqrt(sum(x*x for x in vertex.xyz)), 1., places=12)
                else:
                    self.assertEqual(len(mesh.faces[0].joint_recoveries), 4)

    def test_direct_preflights_both_libraries_and_propagates_resource_failure(self):
        source = pierced_prism_source()
        for loader in ('direct_cdt_library_path', 'library_path'):
            with self.subTest(loader=loader), \
                 mock.patch.object(face_domain, loader, side_effect=RuntimeError('selected library unavailable')), \
                 mock.patch('brepgpu.imported_meshing.validate_source', side_effect=AssertionError('preparation started')):
                with self.assertRaisesRegex(RuntimeError, 'selected library unavailable'):
                    mesh_imported(source, domain_backend='direct')
        with mock.patch.object(face_domain, 'constrained_delaunay', side_effect=face_domain.ResourceError('work exhausted')):
            with self.assertRaisesRegex(face_domain.ResourceError, 'work exhausted'):
                mesh_imported(source, domain_backend='direct')

    def test_unknown_backend_and_unavailable_native_engine_fail_visibly(self):
        source = pierced_prism_source()
        with self.assertRaisesRegex(ValueError, 'domain_backend'):
            mesh_imported(source, domain_backend='unselected')
        with mock.patch.object(face_domain, 'library_path', side_effect=RuntimeError('native engine unavailable')):
            with self.assertRaisesRegex(RuntimeError, 'native engine unavailable'):
                mesh_imported(source, domain_backend='native')
