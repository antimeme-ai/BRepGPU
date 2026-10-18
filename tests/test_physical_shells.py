"""Physical admission of solids with voids, and whose errors count as whose (review rounds 3, 4).

qualify_physical_solid splits a mesh into its edge-connected shells; each must be closed
and edge-consistent, the outer shell (largest volume by magnitude) must enclose a
positive volume, and every other shell is a void: negative and inside the outer shell by
point containment. Euler is taken over all shells. Owned and NVIDIA meshes use the one
function. And physical_row scores only an admission verdict as an NVIDIA FAIL; an
exception in our admission code is ours (OursFailure, exit 5).

Oracle: authored meshes of a 10 mm cube with a 4 mm cube void (volume 1000 - 64 = 936),
wound by hand.
"""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# A unit cube's eight corners (x, y, z in {0, 1}) and its twelve triangles (two per side).
CORNERS = tuple((x, y, z) for x in (0, 1) for y in (0, 1) for z in (0, 1))
OUTWARD = ((0, 2, 3), (0, 3, 1), (4, 5, 7), (4, 7, 6), (0, 1, 5), (0, 5, 4),
           (2, 6, 7), (2, 7, 3), (0, 4, 6), (0, 6, 2), (1, 3, 7), (1, 7, 5))


def cube(origin, size, start, inward=False):
    """Vertices keyed from `start` and the twelve triangles of an axis-aligned cube, each
    wound so its normal points away from the centre (towards it when inward)."""
    vertices = {start+i: tuple(origin[k]+size*c[k] for k in range(3)) for i, c in enumerate(CORNERS)}
    centre = tuple(origin[k]+size/2 for k in range(3))
    triangles = []
    for a, b, c in OUTWARD:
        p, q, r = (vertices[start+i] for i in (a, b, c))
        u, v = tuple(q[k]-p[k] for k in range(3)), tuple(r[k]-p[k] for k in range(3))
        n = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
        away = sum(n[k]*(p[k]-centre[k]) for k in range(3)) > 0
        triangles.append(tuple(start+i for i in ((a, b, c) if away != inward else (a, c, b))))
    return vertices, tuple(triangles)


def block(outer_inward=False, void_inward=True, void_origin=(3., 3., 3.)):
    outer, a = cube((0., 0., 0.), 10., 0, outer_inward)
    void, b = cube(void_origin, 4., 8, void_inward)
    return {**outer, **void}, a+b


def mesh_volume(vertices, triangles):
    total = 0.
    for t in triangles:
        a, b, c = (vertices[k] for k in t)
        total += (a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6
    return total


class PhysicalShellTests(unittest.TestCase):

    def test_a_cube_with_an_inward_void_passes(self):
        from brepgpu.imported_oracles import qualify_physical_solid
        vertices, triangles = block()
        self.assertAlmostEqual(mesh_volume(vertices, triangles), 936.)
        stats = qualify_physical_solid(vertices, triangles)
        self.assertEqual((stats['shells'], stats['euler']), (2, 4))

    def test_inside_out_shells_fail_by_name(self):
        from brepgpu.imported_oracles import AdmissionVerdict, qualify_physical_solid
        for name, (vertices, triangles), message in (
                ('outer inside out', block(outer_inward=True, void_inward=True), 'outer shell mesh volume -1000 is not positive'),
                ('void outward', block(void_inward=False), 'void shell mesh volume 64 is not negative'),
                ('both inside out', block(outer_inward=True, void_inward=False), 'outer shell mesh volume -1000 is not positive'),
                ('void outside the outer shell', block(void_origin=(20., 3., 3.)), 'void shell is not inside the outer shell'),
                ('lone cube inside out', cube((0., 0., 0.), 10., 0, inward=True), 'outer shell mesh volume -1000 is not positive')):
            with self.subTest(name), self.assertRaisesRegex(AdmissionVerdict, message):
                qualify_physical_solid(vertices, triangles)

    def test_shell_configurations_keep_or_gain_their_outcome(self):
        """Review round 4, item 1 (attack a10): shells must be pairwise disjoint, every
        void inside the outer shell, and no void inside another void. The three that
        passed before (a void inside a void, overlapping voids, a void crossing the
        outer face) now fail by name; every correct case still passes, and every other
        malformed case keeps its failure."""
        from brepgpu.imported_oracles import AdmissionVerdict, qualify_physical_solid
        def boxes(*parts):
            vertices, triangles, start = {}, (), 0
            for origin, size, inward in parts:
                v, t = cube(origin, size, start, inward)
                vertices.update(v); triangles += t; start += 8
            return vertices, triangles
        O = ((0., 0., 0.), 10., False)
        cases = (
            ('one void', (O, ((3.,)*3, 4., True)), None),
            ('three disjoint voids', (O, ((1.,)*3, 2., True), ((4.,)*3, 2., True), ((7.,)*3, 2., True)), None),
            ('thin skin around a large void', (O, ((.001,)*3, 9.998, True)), None),
            ('void on a diagonal plane of the outer shell', (O, ((1.,)*3, 1., True)), None),
            ('void inside a void', (O, ((2.,)*3, 6., True), ((4.,)*3, 2., True)), 'void shell lies inside another void shell'),
            ('island in a cavity', (O, ((2.,)*3, 6., True), ((4.,)*3, 2., False)), 'void shell mesh volume 8 is not negative'),
            ('overlapping voids', (O, ((2.,)*3, 4., True), ((4.,)*3, 4., True)), r'physical shells \d and \d intersect'),
            ('void crossing the outer face', (O, ((8.,)*3, 4., True)), r'physical shells \d and \d intersect'),
            ('void entirely outside', (O, ((12.,)*3, 2., True)), 'void shell is not inside the outer shell'),
            ('void outside on the origin side', (O, ((-4.,)*3, 2., True)), 'void shell is not inside the outer shell'),
            ('two lumps', (O, ((12.,)*3, 2., False)), 'void shell mesh volume 8 is not negative'),
            ('outer inside out', (((0.,)*3, 10., True), ((3.,)*3, 4., False)), 'outer shell mesh volume -1000 is not positive'),
            ('void inside out', (O, ((3.,)*3, 4., False)), 'void shell mesh volume 64 is not negative'),
            ('the void is the larger shell', (((3.,)*3, 4., False), ((0.,)*3, 10., True)), 'outer shell mesh volume -1000 is not positive'))
        for name, parts, message in cases:
            with self.subTest(name):
                vertices, triangles = boxes(*parts)
                if message is None:
                    self.assertEqual(qualify_physical_solid(vertices, triangles)['shells'], len(parts))
                else:
                    with self.assertRaisesRegex(AdmissionVerdict, message):
                        qualify_physical_solid(vertices, triangles)

    def test_a_natural_torus_face_expects_euler_zero(self):
        """Review round 4, item 2: the expected Euler number comes from each face's own
        topology, chi = 2 - 2g - b - p. A one-face solid whose face is a natural torus
        (carrier kind 5, no bounds) expects 0, which a closed torus mesh has; the disc
        formula expected 2 and failed it. A natural sphere (no bounds) still expects 2."""
        import math
        from types import SimpleNamespace
        from brepgpu.imported_source import DEFINITION_COMPLETE
        from brepgpu.nvidia_reference import qualify_physical_definition
        def torus(n=12, m=8, R=5., r=2.):
            keys = {(i, j): i*m+j for i in range(n) for j in range(m)}
            points = {keys[i, j]: ((R+r*math.cos(2*math.pi*j/m))*math.cos(2*math.pi*i/n),
                                   (R+r*math.cos(2*math.pi*j/m))*math.sin(2*math.pi*i/n), r*math.sin(2*math.pi*j/m))
                      for (i, j) in keys}
            triangles = []
            for i in range(n):
                for j in range(m):
                    a, b, c, d = keys[i, j], keys[(i+1) % n, j], keys[(i+1) % n, (j+1) % m], keys[i, (j+1) % m]
                    triangles += [(a, b, c), (a, c, d)]
            return points, triangles
        def mesh(points, triangles):
            order = tuple(sorted(points))
            return SimpleNamespace(physical_keys=order, points=tuple(points[k] for k in order),
                                   triangles=tuple(tuple(order.index(k) for k in t) for t in triangles))
        def one_face(kind):
            face = SimpleNamespace(id=1, carrier_id=1, loop_ids=())
            definition = SimpleNamespace(id=7, status=DEFINITION_COMPLETE, vertex_ids=(), edge_ids=(), face_ids=(1,))
            return SimpleNamespace(face=lambda i: face, carrier=lambda i: SimpleNamespace(kind=kind),
                                   loop=lambda i: None, coedge=lambda i: None), definition
        points, triangles = torus()
        if mesh_volume(points, triangles) < 0:
            triangles = [(a, c, b) for a, b, c in triangles]
        source, definition = one_face(5)
        stats = qualify_physical_definition(mesh(points, triangles), source, definition)
        self.assertEqual(stats['euler'], 0)
        vertices, cube_triangles = cube((0., 0., 0.), 1., 0)
        source, definition = one_face(4)
        self.assertEqual(qualify_physical_definition(mesh(vertices, cube_triangles), source, definition)['euler'], 2)

    def test_a_decode_error_is_ours_unless_it_is_nvidias_mesh(self):
        """Review round 4, item 3: our wrapper's .bgrefmesh. A broken container (header)
        is our producer wrapper's fault: OursFailure (exit 5). A nonfinite point in the
        mesh NVIDIA produced is NVIDIA's: a FAIL row's message."""
        import struct
        import tempfile
        sys.path.insert(0, str(ROOT/'examples'))
        from render_om10_nvidia import OursFailure, decode_reference_mesh
        def bgrefmesh(points, magic=b'BGREFM01'):
            data = struct.pack('<8sII6Q', magic, 1, 0, 1, 0, len(points), 1, 1, 0)
            data += struct.pack('<5Q', 7, 0, len(points), 0, 1)
            for i, p in enumerate(points):
                data += struct.pack('<6dQ', *p, 0., 0., 1., i)
            data += struct.pack('<4IQ', 0, 1, 2, 0, 0)
            return data + struct.pack('<4d', .5, .5, .5, 1.)
        good = ((0., 0., 0.), (1., 0., 0.), (0., 1., 0.))
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'definition-7.bgrefmesh'
            path.write_bytes(bgrefmesh(good))
            mesh, fault = decode_reference_mesh(path, 7)
            self.assertIsNone(fault)
            self.assertEqual(mesh.points, good)
            path.write_bytes(bgrefmesh(good, magic=b'BGREFMXX'))
            with self.assertRaisesRegex(OursFailure, "our producer wrapper's mesh of definition 7 does not decode"):
                decode_reference_mesh(path, 7)
            path.write_bytes(bgrefmesh(((0., 0., 0.), (float('nan'), 0., 0.), (0., 1., 0.))))
            mesh, fault = decode_reference_mesh(path, 7)
            self.assertIsNone(mesh)
            self.assertEqual(fault, 'NVIDIA mesh content: reference point/normal is nonfinite or zero')

    def test_an_error_in_our_admission_is_ours_not_an_nvidia_fail(self):
        """A forced exception inside our admission (a mesh object without the fields the
        admission reads) is OursFailure; a verdict on the mesh is a FAIL row."""
        from types import SimpleNamespace
        sys.path.insert(0, str(ROOT/'examples'))
        from render_om10_nvidia import OursFailure, physical_row
        broken = SimpleNamespace(points=((0., 0., 0.),), triangles=((0, 0, 0),))  # no physical_keys
        with self.assertRaisesRegex(OursFailure, 'our physical admission of definition 7 failed in our code'):
            physical_row(broken, None, 7)
        vertices, triangles = cube((0., 0., 0.), 1., 0, inward=True)
        keys = tuple(sorted(vertices))
        inverted = SimpleNamespace(physical_keys=keys, points=tuple(vertices[k] for k in keys),
                                   triangles=tuple(tuple(keys.index(k) for k in t) for t in triangles))
        row = physical_row(inverted, None, 7)
        self.assertEqual(row[1], 'FAIL')
        self.assertIn('outer shell mesh volume -1 is not positive', row[-1])


if __name__ == '__main__':
    unittest.main()
