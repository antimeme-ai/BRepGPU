"""Direct numerical-policy checks against the actual isolated NVIDIA producer."""
import csv
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = Path(os.environ.get('BREPGPU_OM10_NVIDIA', ROOT/'build/om10-nvidia/brepgpu-om10-nvidia'))
EXPORTER = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE', ROOT/'build/om10-reference-source/brepgpu-reference-source'))
PACKAGE = Path(os.environ.get('BREPGPU_NVIDIA_PACKAGE', ROOT/'quarantine/nvidia-usd-brep-1.0.0-linux'))


class NvidiaReferenceControls(unittest.TestCase):
    def test_bad_or_duplicate_angles_refuse_before_output(self):
        for option in ('--curve-angle', '--surface-angle'):
            for values in (('-1',), ('nan',), ('inf',), ('25junk',), ('25', '25')):
                with self.subTest(option=option, values=values), tempfile.TemporaryDirectory() as d:
                    output = Path(d)/'new'
                    arguments = [str(PRODUCER), '--heal', '0', '--chord-height', '.005',
                                 '--definition', '1', str(Path(d)/'missing.brep'),
                                 '--output-directory', str(output)]
                    for value in values:
                        arguments.extend((option, value))
                    result = subprocess.run(arguments, capture_output=True, text=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse(output.exists(), result.stderr)

    def test_angular_policy_changes_actual_cylinder_sampling(self):
        from brepgpu.nvidia_reference import load_definition
        environment = dict(os.environ, CUDA_VISIBLE_DEVICES='', PXR_WORK_THREAD_LIMIT='1',
                           OMP_NUM_THREADS='1', OMNISOLID_PLUGIN_PATH=str(PACKAGE/'omniSolid/resources'))
        with tempfile.TemporaryDirectory() as d:
            directory = Path(d)
            source = directory/'source'
            subprocess.run((str(EXPORTER), str(ROOT/'tests/fixtures/reference-source.step'), str(source)),
                           check=True, capture_output=True)
            counts = []
            for angle in (0, 3):
                output = directory/f'angle{angle}'
                subprocess.run((str(PRODUCER), '--heal', '0', '--chord-height', '.005',
                    '--curve-angle', str(angle), '--surface-angle', str(angle),
                    '--definition', '395', str(source/'definition-395.brep'),
                    '--output-directory', str(output)), env=environment, check=True, capture_output=True)
                with (output/'parameters.tsv').open() as stream:
                    parameters = {r['parameter']:r['value'] for r in csv.DictReader(stream, delimiter='\t')}
                self.assertEqual(float(parameters['curve_angle_degrees']), angle)
                self.assertEqual(float(parameters['surface_angle_degrees']), angle)
                self.assertEqual(float(parameters['chord_height_mm']), .005)
                mesh = load_definition(output/'definition-395.bgrefmesh', expected_id=395)
                self.assertEqual(set(mesh.face_ids), {0, 1, 2})
                counts.append(len(set(mesh.physical_keys)))
            self.assertGreater(counts[1], counts[0], 'actual SDK sampling must consume the angular settings')
