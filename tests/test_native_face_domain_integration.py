"""Actual imported mesh consumers of the native domain engine; fleet only."""
from unittest import TestCase, mock

from brepgpu import face_domain
from brepgpu.imported_meshing import mesh_imported, MeshQuality
from brepgpu.imported_oracles import qualify_mesh
from tests.test_imported_geometry import pierced_prism_source
from tests.test_imported_meshing import tangent_circle_sheet_fixture


class NativeFaceDomainIntegration(TestCase):
    def test_actual_two_hole_solid_qualifies_with_both_explicit_engines(self):
        source = pierced_prism_source()
        quality = MeshQuality(tolerance_mm=.005, source_residual_mm=1.e-8)
        for backend in ('native', 'python'):
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

    def test_unknown_backend_and_unavailable_native_engine_fail_visibly(self):
        source = pierced_prism_source()
        with self.assertRaisesRegex(ValueError, 'domain_backend'):
            mesh_imported(source, domain_backend='unselected')
        with mock.patch.object(face_domain, 'library_path', side_effect=RuntimeError('native engine unavailable')):
            with self.assertRaisesRegex(RuntimeError, 'native engine unavailable'):
                mesh_imported(source, domain_backend='native')
