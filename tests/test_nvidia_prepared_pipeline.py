"""Direct reference preparation contracts; execution belongs on the fleet.

Required environment, all checked once in REQUIREMENTS before any test runs
(see docs/NVIDIA_REFERENCE_PREPARATION.md, "Running the preparation tests"):
  BREPGPU_NVIDIA_PACKAGE     extracted NVIDIA USD/BRep 1.0.0 package directory
  BREPGPU_REFERENCE_DISPLAY  built brepgpu-om10-reference-display
  OMNISOLID_PLUGIN_PATH      $BREPGPU_NVIDIA_PACKAGE/omniSolid/resources; the
                             SDK converter and producer refuse without it
  PYTHONPATH                 includes python/ and $BREPGPU_NVIDIA_PACKAGE/usdpy
  LD_LIBRARY_PATH            includes $BREPGPU_NVIDIA_PACKAGE/lib and extraLibs,
                             which the usdpy bindings load
  numactl on PATH            the caller binds itself and its children to a lane
  BREPGPU_DELIVERED_CLOCK    built delivered-clock probe for context rows; default
                             build/measurement/brepgpu-delivered-clock
                             (scripts/build_measurement_probe.sh)
  build/nvidia-ingress/brepgpu-nvidia-ingress
                             the minimal NVIDIA STEP ingress the caller runs
Optional executable overrides: BREPGPU_NVIDIA_CONVERTER, BREPGPU_REFERENCE_SOURCE,
BREPGPU_OM10_NVIDIA, BREPGPU_REFERENCE_PERIODIC, BREPGPU_NVIDIA_PLANE_ENVELOPE.
A missing requirement fails the suite with one message naming every one.
"""
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



def missing_requirements():
    """Every unmet requirement of this suite, each named with its fix."""
    missing=[]
    package=os.environ.get('BREPGPU_NVIDIA_PACKAGE')
    if not package or not Path(package,'lib').is_dir():
        missing.append('BREPGPU_NVIDIA_PACKAGE: set to the extracted NVIDIA USD/BRep 1.0.0 package directory')
    display=os.environ.get('BREPGPU_REFERENCE_DISPLAY')
    if not display or not Path(display).is_file():
        missing.append('BREPGPU_REFERENCE_DISPLAY: set to the built brepgpu-om10-reference-display')
    plugin=os.environ.get('OMNISOLID_PLUGIN_PATH')
    if not plugin or not Path(plugin).is_dir():
        missing.append('OMNISOLID_PLUGIN_PATH: set to $BREPGPU_NVIDIA_PACKAGE/omniSolid/resources; '
                       'the SDK converter and producer refuse without it')
    probe=subprocess.run((sys.executable,'-c','import brepgpu, pxr.Usd'),capture_output=True,text=True)
    if probe.returncode:
        missing.append('PYTHONPATH/LD_LIBRARY_PATH: python/ and $BREPGPU_NVIDIA_PACKAGE/usdpy on PYTHONPATH, '
                       '$BREPGPU_NVIDIA_PACKAGE/lib and extraLibs on LD_LIBRARY_PATH ('+
                       (probe.stderr.strip().splitlines() or ['import failed'])[-1]+')')
    import shutil
    if not shutil.which('numactl'):
        missing.append('numactl: not on PATH; the caller binds itself and its workers with it')
    probe=Path(os.environ.get('BREPGPU_DELIVERED_CLOCK',ROOT/'build/measurement/brepgpu-delivered-clock'))
    if not probe.is_file():
        missing.append(f'BREPGPU_DELIVERED_CLOCK: {probe} is not built; run scripts/build_measurement_probe.sh '
                       'or point BREPGPU_DELIVERED_CLOCK at it')
    ingress=ROOT/'build/nvidia-ingress/brepgpu-nvidia-ingress'
    if not ingress.is_file():
        missing.append(f'{ingress} is not built; build native/nvidia_ingress (docs/NVIDIA_REFERENCE_PREPARATION.md)')
    for variable,path in (('BREPGPU_NVIDIA_CONVERTER',CONVERTER),('BREPGPU_REFERENCE_SOURCE',EXPORTER),
                          ('BREPGPU_OM10_NVIDIA',PRODUCER),('BREPGPU_REFERENCE_PERIODIC',PERIODIC),
                          ('BREPGPU_NVIDIA_PLANE_ENVELOPE',PLANE_HELPER)):
        if not path.is_file():
            missing.append(f'{variable}: {path} is not built; build it or point {variable} at it')
    return missing


class PreparedPipeline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        missing=missing_requirements()
        if missing:
            raise AssertionError('NVIDIA preparation suite environment incomplete:\n  '+'\n  '.join(missing))

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

    def test_converter_service_failure_is_ours_not_an_nvidia_refusal(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            subprocess.run((str(EXPORTER),str(ROOT/'tests/fixtures/reference-source.step'),
                            str(root/'source')),check=True,capture_output=True)
            dying=root/'dying-converter'
            dying.write_text(f'#!{sys.executable}\nimport os,sys\nout=os.fdopen(int(sys.argv[2]),"w")\n'
                             'out.write("ready\\n");out.flush()\nsys.stdin.readline()\nos._exit(1)\n')
            dying.chmod(0o700)
            result=subprocess.run((sys.executable,'-B',str(ROOT/'scripts/prepare_nvidia_reference_worker.py'),
                '--converter',str(dying),'--periodic-preparer',str(PERIODIC),'--plane-helper',str(PLANE_HELPER),
                '--producer',str(PRODUCER),'--heal','0','--chord-height','.005','--curve-angle','25',
                '--surface-angle','25','--preparation-directory',str(root/'preparation'),
                '--output-directory',str(root/'mesh'),
                '--definition','41',str(root/'source/definition-41.brep'),'6'),capture_output=True,text=True)
            # Two dead servers on one job: ours (exit 5); no refusal inferred, no repair attempted.
            self.assertEqual(result.returncode,5,result.stderr)
            with (root/'preparation/definitions.tsv').open() as stream:
                report,=csv.DictReader(stream,delimiter='\t')
            self.assertEqual(report['status'],'FAIL')
            self.assertTrue(report['message'].startswith('ours:'))
            with (root/'preparation/stages.tsv').open() as stream:
                stages=[r['stage'] for r in csv.DictReader(stream,delimiter='\t')]
            self.assertNotIn('prepare_periodic_copy',stages)
            # Both failed requests, charged once, to ours.
            self.assertEqual(stages.count('converter_service_failure'),1)
            self.assertFalse((root/'mesh').exists())

    def test_caller_labels_a_failed_converter_service_ours(self):
        sys.path.insert(0,str(ROOT/'tests'))
        from test_measurement_lane import matched_fixture
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            matched_fixture(root)
            dying=root/'dying-converter'
            dying.write_text(f'#!{sys.executable}\nimport os,sys\nout=os.fdopen(int(sys.argv[2]),"w")\n'
                             'out.write("ready\\n");out.flush()\nsys.stdin.readline()\nos._exit(1)\n')
            dying.chmod(0o700)
            result=subprocess.run((sys.executable,'-B',str(ROOT/'examples/render_om10_nvidia.py'),
                '--step',str(ROOT/'tests/fixtures/reference-source.step'),'--matched',str(root/'matched'),
                '--output',str(root/'out'),'--heal','0','--workers','1','--allow-fixture',
                '--reference-preparation','periodic-plane','--exporter',str(EXPORTER),'--producer',str(PRODUCER),
                '--converter',str(dying),'--periodic-preparer',str(PERIODIC),'--plane-helper',str(PLANE_HELPER),
                '--package',os.environ['BREPGPU_NVIDIA_PACKAGE'],'--display',os.environ['BREPGPU_REFERENCE_DISPLAY']),
                capture_output=True,text=True)
            # Ours (exit 5), never NVIDIA's failure (1) or a refusal-driven repair.
            self.assertEqual(result.returncode,5,result.stderr)
            self.assertIn('converter service failed twice',result.stderr)

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
