"""Direct original-UV and pixel answers for the owned native display; fleet only."""
from pathlib import Path
from fractions import Fraction as F
import math
import os
import struct
import subprocess
import tempfile
import unittest


U64_MAX = (1 << 64) - 1
POINTS = ((0., 0., 0.), (2., 0., 0.), (0., -2., 0.))
MATRIX = (1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 1.)
INSTANCES = ((7, MATRIX, (0., 0., 0.)),)


def plane_scene(points=POINTS, triangles=((0, 1, 2),), instances=INSTANCES):
    """One source plane, reversed normal, one actual rigidly placed triangle."""
    header = struct.pack('<8sII8Q', b'BGMESH01', 1, 0, 1, len(instances), len(points),
                         len(triangles), 1, 1, 0, 0)
    definition = struct.pack('<5Q', 41, 0, len(points), 0, len(triangles))
    instance = b''.join(struct.pack('<QII15d', id, 0, 0, *matrix, *offset)
                        for id, matrix, offset in instances)
    vertices = b''.join(struct.pack('<5dIIQ', x, y, z, x, y, 0, 2, i)
                        for i, (x, y, z) in enumerate(points, 1))
    triangle = b''.join(struct.pack('<4IQ', *indices, 0, 901 + i)
                        for i, indices in enumerate(triangles))
    frame = (0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0., 1.)
    carrier = (struct.pack('<QII', 101, 1, 0) +
               struct.pack('<18d', *frame, 0., 0., min(p[0] for p in points),
                           max(p[0] for p in points), min(p[1] for p in points),
                           max(p[1] for p in points)) +
               struct.pack('<QQIIQ', 0, 0, 0, 0, 0))
    material = struct.pack('<4d', .2, .4, .8, .5)
    return header + definition + instance + vertices + triangle + carrier + material


def views(*, raster, width=4, height=4, supersample=1, alpha=0., flags=4):
    header = struct.pack('<8sII', b'BGVIEW01', 1, int(raster))
    if not raster:
        return header
    basis = (1., 0., 0., 0., 1., 0., 0., 0., 1.)
    request = (struct.pack('<4I', width, height, supersample, 0) +
               struct.pack('<19d', alpha, *basis, 0., 0., 0., .5, .5, 1., .01, .01, .01) +
               struct.pack('<II2d', flags, 0, 0., 0.))
    return header + request


def scalar_pixels(points, triangles, instances, *, width=4, height=4, supersample=1, alpha=0.):
    """Independent tiny rational image model; no bins or native raster helper."""
    primitives = []
    for instance_index, (id, matrix, offset) in enumerate(instances):
        world = []
        for point in points:
            value = tuple(sum(F(matrix[4*i+j]) * F(point[j]) for j in range(3)) +
                          F(matrix[4*i+3]) + F(alpha) * F(offset[i]) for i in range(3))
            projected = ((value[0] + F(1, 2)) * supersample,
                         (-value[1] + F(1, 2)) * supersample)
            snapped = []
            for x in projected:
                x *= 256
                snapped.append(math.floor(x + F(1, 2)) if x >= 0 else math.ceil(x - F(1, 2)))
            world.append((*snapped, value[2]))
        for triangle_index, indices in enumerate(triangles):
            primitives.append((instance_index * len(triangles) + triangle_index, id,
                               tuple(world[i] for i in indices)))
    def side(a, b, p):
        return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
    output = []
    for y in range(height * supersample):
        for x in range(width * supersample):
            sample = (256*x + 128, 256*y + 128)
            best = (-float('inf'), U64_MAX, U64_MAX)
            for primitive, instance, corners in primitives:
                a, b, c = corners
                area = side(a, b, c)
                if not area:
                    continue
                if area < 0:
                    b, c = c, b
                    area = -area
                edges = ((b, c), (c, a), (a, b))
                weights = tuple(side(p, q, sample) for p, q in edges)
                owned = tuple(q[1] < p[1] or (q[1] == p[1] and q[0] > p[0]) for p, q in edges)
                if any(weight < 0 or (weight == 0 and not owner) for weight, owner in zip(weights, owned)):
                    continue
                depth = sum(F(w, area) * p[2] for w, p in zip(weights, (a, b, c)))
                if depth > best[0] or (depth == best[0] and primitive < best[1]):
                    best = (depth, primitive, instance)
            output.append(best)
    return tuple(output)


class NativeDirectTests(unittest.TestCase):
    def run_native(self, directory, *, raster, scene_bytes=None, view_bytes=None):
        directory = Path(directory)
        source = directory / 'scene.bgmesh'
        request = directory / 'views.bgview'
        output = directory / 'output'
        source.write_bytes(plane_scene() if scene_bytes is None else scene_bytes)
        request.write_bytes(views(raster=raster) if view_bytes is None else view_bytes)
        executable = Path(os.environ.get('BREPGPU_OM10_NATIVE',
                                        'build/om10-release/brepgpu-om10-native')).resolve()
        backend = os.environ.get('BREPGPU_OM10_BACKEND', 'cpu')
        arguments = [str(executable), '--scene', str(source), '--views', str(request),
                     '--output-directory', str(output), '--backend', backend,
                     '--threads', os.environ.get('BREPGPU_OM10_THREADS', '1'), '--dump-source-jets']
        if backend == 'cuda':
            arguments += ['--device', os.environ.get('BREPGPU_OM10_DEVICE', '0'), '--plugin',
                          os.environ.get('BREPGPU_OM10_PLUGIN',
                                         str(executable.parent / 'libbrepgpu_om10_cuda.so'))]
        process = subprocess.run(arguments,
                                 text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEqual(process.returncode, 0, process.stderr)
        return output

    def test_original_plane_jets_and_reversed_normal_without_a_render(self):
        with tempfile.TemporaryDirectory() as directory:
            output = self.run_native(directory, raster=False)
            data = (output / 'source-jets.bin').read_bytes()
            self.assertEqual(data[:16], struct.pack('<8sQ', b'BGJETS01', 3))
            self.assertEqual(len(data), 16 + 3 * 128)
            for index, (x, y) in enumerate(((0., 0.), (2., 0.), (0., -2.))):
                row = struct.unpack_from('<15dII', data, 16 + index * 128)
                self.assertEqual(row[:3], (x, y, 0.))
                self.assertEqual(row[3:6], (1., 0., 0.))
                self.assertEqual(row[6:9], (0., 1., 0.))
                self.assertEqual(row[9:12], (0., 0., -1.))
                self.assertEqual(row[12:15], (x, y, 0.))
                self.assertEqual(row[15:], (0, 0))
            self.assertFalse(tuple(output.glob('frame-*')), 'evaluate-only emitted a product frame')

    def test_known_triangle_pixels_original_instance_and_depth(self):
        with tempfile.TemporaryDirectory() as directory:
            output = self.run_native(directory, raster=True)
            primitive = struct.unpack('<16Q', (output / 'frame-000000.primitive').read_bytes())
            instance = struct.unpack('<16Q', (output / 'frame-000000.instance').read_bytes())
            depth = struct.unpack('<16d', (output / 'frame-000000.depth').read_bytes())
            # In image coordinates, positive-area AB is the top edge and CA the
            # left edge. The hypotenuse owns none of its exact-center samples.
            inside = {0, 1, 4}
            self.assertEqual(primitive, tuple(0 if i in inside else U64_MAX for i in range(16)))
            self.assertEqual(instance, tuple(7 if i in inside else U64_MAX for i in range(16)))
            self.assertEqual(depth, tuple(1. if i in inside else -float('inf') for i in range(16)))
            rgba = (output / 'frame-000000.rgba').read_bytes()
            self.assertEqual(len(rgba), 64)
            self.assertEqual(rgba[3::4], bytes([255] * 16))
            samples = (output / 'frame-000000.samples').read_bytes()
            self.assertEqual(samples[:24], struct.pack('<8sQQ', b'BGSAMP01', 4, 4))
            self.assertEqual(len(samples), 24 + 16 * 24)
            for i in range(16):
                self.assertEqual(struct.unpack_from('<dQQ', samples, 24 + i * 24),
                                 (depth[i], primitive[i], instance[i]))

    def test_shared_edge_opposite_winding_and_offscreen_exact_pixel_model(self):
        points = ((-1., 1., 0.), (3., 1., 0.), (3., -3., 0.), (-1., -3., 0.))
        triangles = ((0, 1, 2), (0, 3, 2))
        expected = scalar_pixels(points, triangles, INSTANCES)
        with tempfile.TemporaryDirectory() as directory:
            output = self.run_native(directory, raster=True,
                                     scene_bytes=plane_scene(points, triangles))
            self.assertEqual(struct.unpack('<16Q', (output / 'frame-000000.primitive').read_bytes()),
                             tuple(row[1] for row in expected))
            self.assertEqual(struct.unpack('<16d', (output / 'frame-000000.depth').read_bytes()),
                             tuple(float(row[0]) for row in expected))

    def test_front_back_tie_instances_and_explosion_exact_sample_model(self):
        front = MATRIX[:11] + (3.,)
        cases = ((INSTANCES + ((8, front, (0., 0., 0.)),), 0.),
                 (((8, front, (0., 0., 0.)),) + INSTANCES, 0.),
                 (INSTANCES + ((8, MATRIX, (0., 0., 0.)),), 0.),
                 (((7, MATRIX, (0., 0., 0.)), (8, front, (3., 0., 0.))), 1.))
        for instances, alpha in cases:
            with self.subTest(instances=instances, alpha=alpha), tempfile.TemporaryDirectory() as directory:
                expected = scalar_pixels(POINTS, ((0, 1, 2),), instances, alpha=alpha)
                output = self.run_native(directory, raster=True,
                                         scene_bytes=plane_scene(instances=instances),
                                         view_bytes=views(raster=True, alpha=alpha))
                self.assertEqual(struct.unpack('<16Q', (output / 'frame-000000.primitive').read_bytes()),
                                 tuple(row[1] for row in expected))
                self.assertEqual(struct.unpack('<16Q', (output / 'frame-000000.instance').read_bytes()),
                                 tuple(row[2] for row in expected))

    def test_supersample_visibility_and_upper_left_final_identity(self):
        expected = scalar_pixels(POINTS, ((0, 1, 2),), INSTANCES, supersample=2)
        with tempfile.TemporaryDirectory() as directory:
            output = self.run_native(directory, raster=True,
                                     view_bytes=views(raster=True, supersample=2))
            sample_bytes = (output / 'frame-000000.samples').read_bytes()
            self.assertEqual(sample_bytes[:24], struct.pack('<8sQQ', b'BGSAMP01', 8, 8))
            for index, row in enumerate(expected):
                self.assertEqual(struct.unpack_from('<dQQ', sample_bytes, 24 + index * 24),
                                 (float(row[0]), row[1], row[2]))
            final = tuple(expected[(2*y)*8 + 2*x][1] for y in range(4) for x in range(4))
            self.assertEqual(struct.unpack('<16Q', (output / 'frame-000000.primitive').read_bytes()), final)

    def test_project_only_rotates_actual_normals_and_preserves_source_mapping(self):
        rotation = (0., 0., 1., 3., 0., 1., 0., 4., -1., 0., 0., 5.)
        with tempfile.TemporaryDirectory() as directory:
            output = self.run_native(directory, raster=True,
                                     scene_bytes=plane_scene(instances=((19, rotation, (0., 0., 0.)),)),
                                     view_bytes=views(raster=True, flags=3))
            data = (output / 'frame-000000.projected').read_bytes()
            self.assertEqual(data[:16], struct.pack('<8sQ', b'BGPROJ01', 3))
            self.assertEqual(len(data), 16 + 3 * 96)
            for index, (x, y, z) in enumerate(POINTS):
                row = struct.unpack_from('<3Q2q7d', data, 16 + index * 96)
                self.assertEqual(row[:3], (19, 41, index))
                self.assertEqual(row[5], 5.-x)
                self.assertEqual(row[6:9], (3.+z, 4.+y, 5.-x))
                self.assertEqual(row[9:12], (-1., 0., 0.))
            self.assertFalse(tuple(output.glob('*.rgba')), 'project-only emitted image output')


if __name__ == '__main__':
    unittest.main()
