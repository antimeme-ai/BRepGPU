"""Direct reference preparation contracts; execution belongs on the fleet."""
import csv
import os
from pathlib import Path
import subprocess
import signal
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
CONVERTER = Path(os.environ.get('BREPGPU_NVIDIA_CONVERTER',
    ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-convert'))
EXPORTER = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE',
    ROOT/'build/om10-reference-source/brepgpu-reference-source'))
PRODUCER = Path(os.environ.get('BREPGPU_OM10_NVIDIA',
    ROOT/'build/nvidia-reference-recovery/brepgpu-om10-nvidia'))
PERIODIC = Path(os.environ.get('BREPGPU_REFERENCE_PERIODIC',
    ROOT/'build/reference-periodic/brepgpu-reference-periodic'))
PLANE_HELPER = Path(os.environ.get('BREPGPU_NVIDIA_PLANE_ENVELOPE',
    ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-plane-envelope'))


class PreparedPipeline(unittest.TestCase):
    def test_reference_term_stops_the_exporter_and_its_child(self):
        package=Path(os.environ['BREPGPU_NVIDIA_PACKAGE'])
        display=Path(os.environ['BREPGPU_REFERENCE_DISPLAY'])
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);pid_file=root/'pids';exporter=root/'blocking-exporter'
            exporter.write_text('#!/usr/bin/python3\nimport os,signal,subprocess,sys\n'
                'from pathlib import Path\nchild=subprocess.Popen(["sleep","30"])\n'
                'def stop(*args):\n raise SystemExit(0)\n'
                'signal.signal(signal.SIGTERM,stop)\n'
                f'Path({str(pid_file)!r}).write_text(str(os.getpid())+" "+str(child.pid))\n'
                'try:\n child.wait()\nfinally:\n'
                ' if child.poll() is None:child.terminate()\n child.wait()\n')
            exporter.chmod(0o700)
            command=(sys.executable,'-B',str(ROOT/'examples/render_om10_nvidia.py'),
                '--step',str(ROOT/'tests/fixtures/reference-source.step'),'--matched',str(root/'unused'),
                '--output',str(root/'output'),'--heal','0','--exporter',str(exporter),
                '--producer',str(PRODUCER),'--display',str(display),'--package',str(package))
            process=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            pids=[]
            try:
                deadline=time.monotonic()+5
                while not pid_file.exists() and process.poll() is None and time.monotonic()<deadline:
                    time.sleep(.01)
                self.assertTrue(pid_file.exists(),'actual caller did not reach the owned exporter process')
                pids=list(map(int,pid_file.read_text().split()))
                process.terminate();process.communicate(timeout=5)
                self.assertNotEqual(process.returncode,0)
                for pid in pids:
                    state=Path(f'/proc/{pid}/stat')
                    self.assertTrue(not state.exists() or state.read_text().split()[2]=='Z',
                                    'TERM left an exporter/descendant running')
            finally:
                if process.poll() is None:process.kill();process.communicate()
                for pid in pids:
                    try:os.kill(pid,signal.SIGKILL)
                    except ProcessLookupError:pass

    def test_actual_prepared_worker_keeps_good_jobs_and_reports_source_inventory(self):
        from brepgpu.nvidia_reference import load_definition
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            subprocess.run((str(EXPORTER),str(ROOT/'tests/fixtures/reference-source.step'),
                            str(root/'source')),check=True,capture_output=True)
            common=(sys.executable,'-B',str(ROOT/'scripts/prepare_nvidia_reference_worker.py'),
                    '--converter',str(CONVERTER),'--periodic-preparer',str(PERIODIC),
                    '--plane-helper',str(PLANE_HELPER),'--producer',str(PRODUCER),
                    '--heal','0','--chord-height','.005','--curve-angle','25','--surface-angle','25')
            for bad in (False,True):
                with self.subTest(failed_input=bad):
                    folder=root/('with-failure' if bad else 'complete')
                    jobs=('--definition','41',str(root/'source/definition-41.brep'),'6',
                          '--definition','395',str(root/'source/definition-395.brep'),'3')
                    if bad:
                        jobs=('--definition','99',str(root/'missing.brep'),'1',*jobs)
                    result=subprocess.run((*common,'--preparation-directory',str(folder/'preparation'),
                        '--output-directory',str(folder/'mesh'),*jobs),capture_output=True,text=True)
                    self.assertEqual(result.returncode,1 if bad else 0,result.stderr)
                    with (folder/'preparation/definitions.tsv').open() as stream:
                        reports=list(csv.DictReader(stream,delimiter='\t'))
                    self.assertEqual({int(r['definition_id']):r['status'] for r in reports},
                                     {41:'PASS',395:'PASS',**({99:'FAIL'} if bad else {})})
                    for id,faces in ((41,6),(395,3)):
                        mesh=load_definition(folder/f'mesh/definition-{id}.bgrefmesh',expected_id=id)
                        self.assertEqual(set(mesh.face_ids),set(range(faces)))
                    self.assertFalse((folder/'mesh/definition-99.bgrefmesh').exists())
                    with (folder/'preparation/stages.tsv').open() as stream:
                        stages=list(csv.DictReader(stream,delimiter='\t'))
                    self.assertEqual(next(r['exit'] for r in stages if r['stage']=='complete_worker'),
                                     str(result.returncode))

    def test_converter_produces_the_literal_box_for_actual_sdk_import(self):
        from brepgpu.nvidia_reference import load_definition
        from brepgpu.imported_oracles import qualify_physical_solid
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            subprocess.run((str(EXPORTER), str(ROOT/'tests/fixtures/reference-source.step'),
                            str(root/'source')), check=True, capture_output=True)
            usd = root/'original.usdc'
            subprocess.run((str(CONVERTER), str(root/'source/definition-41.brep'), str(usd)),
                           check=True, capture_output=True)
            self.assertTrue(usd.is_file())
            subprocess.run((str(PRODUCER), '--heal', '0', '--chord-height', '.005',
                '--curve-angle', '25', '--surface-angle', '25', '--usd-definition', '41', str(usd),
                '--output-directory', str(root/'mesh')), check=True, capture_output=True)
            mesh = load_definition(root/'mesh/definition-41.bgrefmesh', expected_id=41)
            self.assertEqual(set(mesh.face_ids), set(range(6)))
            lower, upper = (.125, .25, .5), (1.125, 2.25, 3.5)
            self.assertEqual(set(mesh.points),
                {(x,y,z) for x in (lower[0],upper[0]) for y in (lower[1],upper[1])
                         for z in (lower[2],upper[2])})
            stats = qualify_physical_solid(dict(zip(mesh.physical_keys,mesh.points)),
                tuple(tuple(mesh.physical_keys[i] for i in t) for t in mesh.triangles))
            self.assertEqual(stats['euler'], 2)

    def test_converter_missing_input_and_existing_output_refuse(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            output = root/'new.usdc'
            result = subprocess.run((str(CONVERTER), str(root/'missing.brep'), str(output)),
                                    capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(output.exists())
            output.write_bytes(b'retain existing bytes')
            result = subprocess.run((str(CONVERTER), str(root/'missing.brep'), str(output)),
                                    capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), b'retain existing bytes')
            output.unlink()
            source = root/'regular.brep'
            source.write_bytes(b'not parsed when destination is occupied')
            pending = root/'new.usdc.pending.usdc'
            pending.write_bytes(b'retain existing temporary bytes')
            result = subprocess.run((str(CONVERTER), str(source), str(output)), capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(pending.read_bytes(), b'retain existing temporary bytes')
            self.assertFalse(output.exists())

    def test_no_plane_layer_is_byte_identical_without_hull_capture(self):
        from pxr import Gf, Sdf, Usd, Vt
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            original = root/'nonplane.usdc'
            stage = Usd.Stage.CreateNew(str(original))
            prim = stage.DefinePrim('/body', 'BrepArray')
            prim.CreateAttribute('face:surfaceType', Sdf.ValueTypeNames.TokenArray).Set(
                Vt.TokenArray(['BrepSurfaceSphereAPI']))
            prim.CreateAttribute('face:range', Sdf.ValueTypeNames.Double2Array).Set(
                Vt.Vec2dArray([Gf.Vec2d(0,-1),Gf.Vec2d(6,1)]))
            stage.GetRootLayer().Save()
            subprocess.run((sys.executable, '-B', str(ROOT/'scripts/prepare_nvidia_plane_envelope.py'),
                '--input-usd', str(original), '--output-directory', str(root/'prepared'),
                '--helper', str(root/'deliberately-no-helper')), check=True, capture_output=True)
            self.assertEqual(original.read_bytes(), (root/'prepared/corrected.usdc').read_bytes())
            with (root/'prepared/preparation.tsv').open() as stream:
                fields = {r['field']:r['value'] for r in csv.DictReader(stream,delimiter='\t')}
            self.assertEqual(fields['policy'], 'no_plane_passthrough_v1')
            self.assertEqual(fields['changed_attribute'], 'none')
            self.assertEqual(fields['plane_ranges'], '0')


if __name__ == '__main__':
    unittest.main()
