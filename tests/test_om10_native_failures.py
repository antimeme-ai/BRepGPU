"""Direct native input/arithmetic/resource failure contracts; fleet execution only."""
from pathlib import Path
import os
import struct
import subprocess
import tempfile
import unittest

from test_om10_native import plane_scene, views


class NativeFailureTests(unittest.TestCase):
    def invoke(self, directory, scene=None, request=None, *, backend='cpu',
               device='0', plugin=None, extra_env=None, output=None):
        directory = Path(directory)
        source = directory / 'scene.bgmesh'
        view = directory / 'views.bgview'
        source.write_bytes(plane_scene() if scene is None else scene)
        view.write_bytes(views(raster=True) if request is None else request)
        output = directory / 'output' if output is None else Path(output)
        binary = Path(os.environ.get('BREPGPU_OM10_NATIVE',
                                     'build/om10-release/brepgpu-om10-native')).resolve()
        command = [str(binary), '--scene', str(source), '--views', str(view),
                   '--output-directory', str(output), '--backend', backend,
                   '--threads', '4', '--dump-source-jets']
        if backend == 'cuda':
            command += ['--device', device]
        if plugin is not None:
            command += ['--plugin', str(plugin)]
        env = dict(os.environ)
        if extra_env:
            env.update(extra_env)
        return subprocess.run(command, env=env, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, text=True), output

    def assert_failed(self, result, output, text):
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn(text, result.stderr)
        self.assertNotIn('completed\t', result.stdout)
        self.assertFalse((output / 'timings.tsv').exists())

    def test_malformed_scene_rejected_before_output(self):
        baseline = plane_scene()
        cases = [('truncated', baseline[:-1]), ('trailing', baseline + b'!')]
        for offset, code, value in ((8, '<I', 99), (12, '<I', 1),
                                    (32, '<Q', (1 << 64) - 1),
                                    (72, '<Q', 1), (256, '<d', float('nan')),
                                    (280, '<d', 3.), (136, '<d', -1.)):
            data = bytearray(baseline)
            struct.pack_into(code, data, offset, value)
            cases.append((f'field-{offset}', data))
        for name, scene in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory() as directory:
                result, output = self.invoke(directory, scene)
                self.assert_failed(result, output, 'decode:')
                self.assertFalse(tuple(output.glob('frame-*')))

    def test_malformed_view_flags_camera_and_dimensions_rejected(self):
        baseline = views(raster=True)
        cases = []
        for offset, code, value in ((16, '<I', 0), (24, '<I', 3),
                                    (48, '<d', 2.), (184, '<I', 8),
                                    (192, '<d', 1.)):
            data = bytearray(baseline)
            struct.pack_into(code, data, offset, value)
            cases.append((offset, data))
        for offset, request in cases:
            with self.subTest(offset=offset), tempfile.TemporaryDirectory() as directory:
                result, output = self.invoke(directory, request=request)
                self.assert_failed(result, output, 'decode:')

    def test_finite_source_overflow_is_explicit_and_has_source_row(self):
        data = bytearray(plane_scene())
        # Source position O.x+U exceeds binary64 although every input is finite.
        struct.pack_into('<d', data, 280, 1.e308)  # first corner U
        struct.pack_into('<d', data, 464, 1.e308)  # carrier O.x
        struct.pack_into('<d', data, 584, 1.e308)  # carrier Umax
        with tempfile.TemporaryDirectory() as directory:
            result, output = self.invoke(directory, data, views(raster=False))
            self.assert_failed(result, output, 'source_evaluate:')
            self.assertIn('row=0', result.stderr)
            self.assertFalse((output / 'source-jets.bin').exists())

    def test_finite_projection_overflow_is_explicit_without_frame(self):
        request = bytearray(views(raster=True))
        struct.pack_into('<d', request, 152, 1.e308)  # finite pixels/unit
        with tempfile.TemporaryDirectory() as directory:
            result, output = self.invoke(directory, request=request)
            self.assert_failed(result, output, 'instance_transform_project:')
            self.assertFalse(tuple(output.glob('frame-*')))

    def test_cpu_runs_without_cuda_or_a_valid_plugin(self):
        with tempfile.TemporaryDirectory() as directory:
            result, output = self.invoke(directory, plugin='/does/not/exist/cuda.so',
                                          extra_env={'CUDA_VISIBLE_DEVICES': '',
                                                     'LD_DEBUG': 'libs'})
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((output / 'frame-000000.rgba').exists())
            self.assertNotIn('libcudart', result.stderr)
            self.assertNotIn('libcuda.so', result.stderr)

    def test_nonempty_output_is_not_reused(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'output'
            output.mkdir()
            sentinel = output / 'frame-000000.rgba'
            sentinel.write_bytes(b'previous output')
            result, output = self.invoke(directory, output=output)
            self.assert_failed(result, output, 'output: directory must be empty')
            self.assertEqual(sentinel.read_bytes(), b'previous output')

    def test_explicit_cuda_missing_plugin_does_not_fallback(self):
        with tempfile.TemporaryDirectory() as directory:
            result, output = self.invoke(directory, backend='cuda',
                                          plugin='/does/not/exist/cuda.so')
            self.assert_failed(result, output, 'CUDA plugin unavailable')
            self.assertFalse(tuple(output.glob('frame-*')))

    @unittest.skipUnless(os.environ.get('BREPGPU_OM10_CHECK_CUDA') == '1',
                         'explicit CUDA ownership checks require P100 fixture run')
    def test_invalid_cuda_device_and_arithmetic_error_close_without_success(self):
        plugin = Path(os.environ.get('BREPGPU_OM10_PLUGIN',
                                     'build/om10-release/libbrepgpu_om10_cuda.so')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            result, output = self.invoke(directory, backend='cuda', device='999',
                                          plugin=plugin)
            self.assert_failed(result, output, 'session_create:')
            self.assertFalse(tuple(output.glob('frame-*')))
        data = bytearray(plane_scene())
        struct.pack_into('<d', data, 280, 1.e308)
        struct.pack_into('<d', data, 464, 1.e308)
        struct.pack_into('<d', data, 584, 1.e308)
        with tempfile.TemporaryDirectory() as directory:
            result, output = self.invoke(directory, data, views(raster=False),
                                          backend='cuda', plugin=plugin)
            self.assert_failed(result, output, 'source_evaluate:')
            self.assertIn('row=0', result.stderr)
            self.assertFalse((output / 'source-jets.bin').exists())


if __name__ == '__main__':
    unittest.main()
