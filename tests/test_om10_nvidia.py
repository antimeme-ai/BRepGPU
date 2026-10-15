"""Direct contracts at the real isolated NVIDIA reference producer boundary."""
import os
from pathlib import Path
import subprocess
import struct
import tempfile
import unittest


EXECUTABLE = Path(os.environ.get('BREPGPU_OM10_NVIDIA',
    'build/om10-nvidia/brepgpu-om10-nvidia')).resolve()


class NvidiaProducerTests(unittest.TestCase):
    def test_actual_cli_help_names_reference_scope(self):
        result = subprocess.run([str(EXECUTABLE), '--help'], text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('--definition', result.stdout)
        self.assertIn('--heal', result.stdout)
        self.assertIn('CPU', result.stdout)

    def test_missing_input_fails_without_a_mesh(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/'new'
            result = subprocess.run([str(EXECUTABLE), '--heal', '0',
                '--chord-height', '.005', '--definition', '17',
                str(Path(directory)/'missing.brep'), '--output-directory', str(output)],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(tuple(output.glob('*.bgrefmesh')))


class ReferenceMeshTests(unittest.TestCase):
    def test_definition_reader_retains_exact_supplied_corners_and_keys(self):
        from brepgpu.nvidia_reference import load_definition
        from tests.test_om10_reference import mesh
        data=mesh(instances=((7,(1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.),(0.,0.,0.)),))
        # Native producer emits zero placements for a definition-local export.
        header=list(struct.unpack_from('<8sII6Q',data));header[4]=0
        data=struct.pack('<8sII6Q',*header)+data[64:104]+data[240:]
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'one.bgrefmesh';path.write_bytes(data)
            result=load_definition(path,expected_id=41)
            self.assertEqual(result.points,((0.,0.,1.),(2.,0.,1.),(0.,-2.,1.)))
            self.assertEqual(result.physical_keys,(0,1,2))
            self.assertEqual(result.face_ids,(0,))
            with self.assertRaises(ValueError):load_definition(path,expected_id=42)


if __name__ == '__main__':
    unittest.main()
