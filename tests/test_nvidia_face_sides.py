"""NVIDIA's meshes of corrected and inside-out definitions under our physical admission;
fleet only, with the NVIDIA package (authored fixtures, one definition; not a benchmark).

B2-reader-export review round 2, item 3: physical admission requires each shell's mesh
volume to have the sign its role requires, for both arms and our owned meshes alike, so
NVIDIA's mesh of an inside-out solid no longer passes (the review measured -3.521 for the
cylinder part), and a face our translation-only export corrects (S1) is NVIDIA's input as
our corrected .brep and is not scored as an NVIDIA failure.

Fixture: the cylinder part (#395) of reference-source.step. Oracles: the mesh's own signed
volume, the cylinder's volume pi 0.75^2 2.
"""
import math
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT/'tests/fixtures'
PACKAGE = Path(os.environ.get('BREPGPU_NVIDIA_PACKAGE', ROOT/'quarantine/nvidia-usd-brep-1.0.0-linux'))
PRODUCER = Path(os.environ.get('BREPGPU_OM10_NVIDIA', ROOT/'build/om10-nvidia/brepgpu-om10-nvidia'))
EXPORTER = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE', ROOT/'build/om10-reference-source/brepgpu-reference-source'))
INGRESS = Path(os.environ.get('BREPGPU_NVIDIA_INGRESS', ROOT/'build/nvidia-ingress/brepgpu-nvidia-ingress'))


def flipped(same=(), bound=()):
    """reference-source.step with the cylinder part's faces' same_sense and bounds flipped."""
    text = (FIXTURES/'reference-source.step').read_text()
    bounds = {397: 398, 485: 486, 489: 490}
    for face in same:
        text = re.sub(rf"(#{face} = ADVANCED_FACE\('',\(#\d+\),#\d+,)(\.[TF]\.)",
                      lambda m: m.group(1)+('.F.' if m.group(2) == '.T.' else '.T.'), text)
    for face in bound:
        text = re.sub(rf"(#{bounds[face]} = FACE_BOUND\('',#\d+,)(\.[TF]\.)",
                      lambda m: m.group(1)+('.F.' if m.group(2) == '.T.' else '.T.'), text)
    return text


@unittest.skipUnless(sys.platform == 'linux' and PRODUCER.is_file() and EXPORTER.is_file() and INGRESS.is_file()
                     and PACKAGE.is_dir(), 'needs the fleet NVIDIA package, producer, exporter and ingress')
class NvidiaFaceSideTests(unittest.TestCase):

    def mesh(self, brep, directory):
        from brepgpu.nvidia_reference import load_definition
        env = dict(os.environ, CUDA_VISIBLE_DEVICES='', PXR_WORK_THREAD_LIMIT='1',
                   OMNISOLID_PLUGIN_PATH=str(PACKAGE/'omniSolid/resources'),
                   LD_LIBRARY_PATH=f"{PACKAGE/'lib'}:{PACKAGE/'extraLibs'}")
        out = Path(directory)/'nvidia'
        run = subprocess.run((str(PRODUCER), '--heal', '0', '--chord-height', '.005', '--output-directory', str(out),
                              '--definition', '395', str(brep)), env=env, capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stderr[-400:])
        return load_definition(out/'definition-395.bgrefmesh', expected_id=395)

    def admit(self, mesh):
        from brepgpu.imported_oracles import qualify_physical_solid
        return qualify_physical_solid(dict(zip(mesh.physical_keys, mesh.points)),
                                      tuple(tuple(mesh.physical_keys[i] for i in t) for t in mesh.triangles))

    def test_an_inside_out_mesh_fails_physical_admission(self):
        """Every face's same_sense and bound flipped: OCCT's translation is the cylinder
        inside out, and NVIDIA meshes it so (negative volume). Our corrected export (S2)
        of the same file meshes outward and passes."""
        A = (397, 485, 489)
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'inside-out.step'
            step.write_text(flipped(same=A, bound=A))
            subprocess.run((str(INGRESS), '--shape-processing', 'none', str(step), str(Path(directory)/'minimal')),
                           check=True, capture_output=True)
            with tempfile.TemporaryDirectory() as d:
                with self.assertRaisesRegex(AssertionError, r'mesh volume -3\.\d+ is not positive'):
                    self.admit(self.mesh(Path(directory)/'minimal/definition-395.brep', d))
            subprocess.run((str(EXPORTER), '--shape-processing', 'translation-only', str(step),
                            str(Path(directory)/'ours')), check=True, capture_output=True)
            with tempfile.TemporaryDirectory() as d:
                self.admit(self.mesh(Path(directory)/'ours/definition-395.brep', d))

    def test_a_corrected_s1_face_is_not_an_nvidia_failure(self):
        """The seam face #397's same_sense flipped (S1): through the caller on the
        translation-only arm, NVIDIA's input for #395 is our corrected .brep (after our
        uncorrected translation matched the minimal ingress's bytes), its mesh passes
        physical admission, and the run completes. The legacy matched mode compares
        every face with the adapter's source of the same file."""
        sys.path.insert(0, str(ROOT/'tests'))
        sys.path.insert(0, str(ROOT/'examples'))
        from test_measurement_lane import matched_fixture, rows
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            step = root/'s1.step'
            step.write_text(flipped(same=(397,)))
            _, matched = matched_fixture(root, step)
            output = root/'out'
            result = subprocess.run((sys.executable, '-B', str(ROOT/'examples/render_om10_nvidia.py'),
                '--step', str(step), '--matched', str(matched), '--output', str(output), '--heal', '0',
                '--workers', '1', '--allow-fixture', '--mode', 'render', '--exporter', str(EXPORTER),
                '--nvidia-ingress', str(INGRESS), '--producer', str(PRODUCER), '--package', str(PACKAGE)),
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr[-600:])
            metadata = {r['field']: r['value'] for r in rows(output/'metadata.tsv')}
            self.assertEqual(metadata['nvidia_input_ours_corrected'], '395')
            self.assertEqual(metadata['exporter_raw_entity_corrections'], 'S1')
            self.assertEqual({r['definition_id']: r['status'] for r in rows(output/'physical.tsv')},
                             {'41': 'PASS', '395': 'PASS'})


@unittest.skipUnless(sys.platform == 'linux' and PRODUCER.is_file() and INGRESS.is_file() and PACKAGE.is_dir(),
                     'needs the fleet NVIDIA package, producer and ingress')
class NvidiaVoidPartTests(unittest.TestCase):
    """Review round 3, item 1: what NVIDIA itself makes of a correct void part."""

    def test_nvidias_mesh_of_the_void_block_fails_admission_on_correct_input(self):
        """void-block.step (a 10 mm cube with a 4 mm cube void, volume 936). OCCT's
        translation, the file NVIDIA meshes, verifies (BRepCheck valid, outer shell
        positive, void negative, 936). NVIDIA's mesh of it, in both ingress arms and with
        both heal settings, winds some void faces wrongly: an edge is used twice the same
        way, so our admission fails it by name, and its signed volume is not 936. This is
        NVIDIA's own output on correct input, a genuine NVIDIA FAIL on this fixture; the
        same admission passes a correctly wound mesh of the same solid
        (tests/test_physical_shells.py)."""
        inspector = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE_FIXTURE',
                                        ROOT/'build/om10-reference-source/brepgpu-reference-source-fixture'))
        env = dict(os.environ, CUDA_VISIBLE_DEVICES='', PXR_WORK_THREAD_LIMIT='1',
                   OMNISOLID_PLUGIN_PATH=str(PACKAGE/'omniSolid/resources'),
                   LD_LIBRARY_PATH=f"{PACKAGE/'lib'}:{PACKAGE/'extraLibs'}")
        from brepgpu.nvidia_reference import load_definition
        from brepgpu.imported_oracles import AdmissionVerdict, qualify_physical_solid
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            for policy in ('none', 'default'):
                subprocess.run((str(INGRESS), '--shape-processing', policy, str(FIXTURES/'void-block.step'),
                                str(directory/policy)), check=True, capture_output=True)
                brep = directory/policy/'definition-15.brep'
                verified = subprocess.run((str(inspector), '--verify', str(brep)), capture_output=True, text=True,
                                          check=True).stdout
                self.assertIn('verify ok\n', verified, policy)
                self.assertAlmostEqual(float(verified.split('completed_volume ')[1]), 936., places=6)
                for heal in ('0', '1'):
                    with self.subTest(policy=policy, heal=heal):
                        out = directory/f'nvidia-{policy}-{heal}'
                        run = subprocess.run((str(PRODUCER), '--heal', heal, '--chord-height', '.005',
                                              '--output-directory', str(out), '--definition', '15', str(brep)),
                                             env=env, capture_output=True, text=True)
                        self.assertEqual(run.returncode, 0, run.stderr[-300:])
                        mesh = load_definition(out/'definition-15.bgrefmesh', expected_id=15)
                        vertices = dict(zip(mesh.physical_keys, mesh.points))
                        triangles = tuple(tuple(mesh.physical_keys[i] for i in t) for t in mesh.triangles)
                        with self.assertRaisesRegex(AdmissionVerdict, 'physical edge needs two opposite distinct uses'):
                            qualify_physical_solid(vertices, triangles)
                        volume = sum((a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6
                                     for a, b, c in (tuple(vertices[k] for k in t) for t in triangles))
                        self.assertGreater(abs(volume-936.), 1.)


if __name__ == '__main__':
    unittest.main()
