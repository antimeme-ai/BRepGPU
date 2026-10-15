"""Direct contracts at the real isolated NVIDIA reference producer boundary."""
import os
import math
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

    def test_real_box_and_cylinder_both_heals_geometry_normals_and_keys(self):
        from brepgpu.nvidia_reference import load_definition
        from brepgpu.imported_oracles import qualify_physical_solid
        root=Path(__file__).resolve().parents[1]
        exporter=root/'build/om10-reference-source/brepgpu-reference-source'
        package=root/'quarantine/nvidia-usd-brep-1.0.0-linux'
        env=dict(os.environ,CUDA_VISIBLE_DEVICES='',PXR_WORK_THREAD_LIMIT='1',
                 OMNISOLID_PLUGIN_PATH=str(package/'omniSolid/resources'))
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory);source=directory/'source'
            subprocess.run([str(exporter),str(root/'tests/fixtures/reference-source.step'),str(source)],
                           check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            for heal in (0,1):
                with self.subTest(heal=heal):
                    output=directory/f'heal{heal}'
                    result=subprocess.run([str(EXECUTABLE),'--heal',str(heal),'--chord-height','.005',
                        '--output-directory',str(output),'--definition','41',str(source/'definition-41.brep'),
                        '--definition','395',str(source/'definition-395.brep')],env=env,
                        stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
                    self.assertEqual(result.returncode,0,result.stderr)
                    for id,faces in ((41,6),(395,3)):
                        m=load_definition(output/f'definition-{id}.bgrefmesh',expected_id=id)
                        self.assertEqual(set(m.face_ids),set(range(faces)))
                        stats=qualify_physical_solid(dict(zip(m.physical_keys,m.points)),
                            tuple(tuple(m.physical_keys[i] for i in t) for t in m.triangles))
                        self.assertEqual(stats['euler'],2)
                        for triangle in m.triangles:
                            a,b,c=(m.points[i] for i in triangle)
                            ab=tuple(b[k]-a[k] for k in range(3));ac=tuple(c[k]-a[k] for k in range(3))
                            cross=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
                            self.assertGreater(sum(cross[k]*m.normals[triangle[0]][k] for k in range(3)),0)
                        if id==41:
                            lower=(.125,.25,.5);upper=(1.125,2.25,3.5)
                            observed=set()
                            for p,n in zip(m.points,m.normals):
                                axis=max(range(3),key=lambda k:abs(n[k]));sign=1 if n[axis]>0 else -1
                                observed.add((axis,sign))
                                for k in range(3):self.assertAlmostEqual(n[k],sign if k==axis else 0.,places=12)
                                self.assertAlmostEqual(p[axis],upper[axis] if sign>0 else lower[axis],places=12)
                                self.assertTrue(all(lower[k]-1.e-12<=p[k]<=upper[k]+1.e-12 for k in range(3)))
                            self.assertEqual(observed,{(axis,sign) for axis in range(3) for sign in (-1,1)})
                        else:
                            for p,n in zip(m.points,m.normals):
                                self.assertGreaterEqual(p[2],.125-1.e-12);self.assertLessEqual(p[2],2.125+1.e-12)
                                if abs(n[2])>.5:
                                    self.assertAlmostEqual(n[2],1. if p[2]>1 else -1.,places=12)
                                    self.assertAlmostEqual(n[0],0.,places=12);self.assertAlmostEqual(n[1],0.,places=12)
                                else:
                                    self.assertAlmostEqual(math.hypot(p[0]+1.5,p[1]-2.25),.75,places=10)
                                    self.assertAlmostEqual(n[0],(p[0]+1.5)/.75,places=10)
                                    self.assertAlmostEqual(n[1],(p[1]-2.25)/.75,places=10)
                                    self.assertAlmostEqual(n[2],0.,places=12)
                            self.assertTrue(any(x!=struct.unpack('<f',struct.pack('<f',x))[0] for p in m.points for x in p))

    def test_failed_definition_is_retained_without_skipping_remaining_jobs(self):
        import csv
        root=Path(__file__).resolve().parents[1]
        env=dict(os.environ,CUDA_VISIBLE_DEVICES='',PXR_WORK_THREAD_LIMIT='1',
                 OMNISOLID_PLUGIN_PATH=str(root/'quarantine/nvidia-usd-brep-1.0.0-linux/omniSolid/resources'))
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory);source=directory/'source'
            subprocess.run([str(root/'build/om10-reference-source/brepgpu-reference-source'),
                str(root/'tests/fixtures/reference-source.step'),str(source)],check=True,
                stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            output=directory/'actual'
            result=subprocess.run([str(EXECUTABLE),'--heal','0','--chord-height','.005',
                '--output-directory',str(output),'--definition','17',str(directory/'missing.brep'),
                '--definition','41',str(source/'definition-41.brep')],env=env,
                stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            self.assertNotEqual(result.returncode,0)
            with (output/'definitions.tsv').open() as f:
                rows=tuple(csv.DictReader(f,delimiter='\t'))
            self.assertEqual(tuple((r['definition_id'],r['status']) for r in rows),(('17','FAIL'),('41','PASS')))
            self.assertFalse((output/'definition-17.bgrefmesh').exists())
            self.assertTrue((output/'definition-41.bgrefmesh').is_file())


class ReferenceMeshTests(unittest.TestCase):
    def test_owned_witness_gate_requires_completion_and_unchanged_inputs(self):
        root=Path(__file__).resolve().parents[1]
        import sys
        sys.path.insert(0,str(root/'scripts'));sys.path.insert(0,str(root/'examples'))
        from qualify_om10_nvidia import qualified_owned_inputs
        from render_om10_nvidia import tsv
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory)
            tsv(directory/'timings.tsv',('stage','seconds','scope'),(('partial',1.,'incomplete fixture'),))
            with self.assertRaisesRegex(ValueError,'incomplete'):
                qualified_owned_inputs(directory,'1'*64,.005)
            tsv(directory/'timings.tsv',('stage','seconds','scope'),
                (('complete_nonvisual_qualification',1.,'synthetic negative input'),))
            tsv(directory/'inputs.tsv',('field','value'),(('source_sha256','1'*64),
                ('tolerance_mm','.005'),('scene_sha256','0'*64),('views_sha256','0'*64)))
            (directory/'scene.bgmesh').write_bytes(b'changed scene')
            (directory/'views.bgview').write_bytes(b'changed views')
            with self.assertRaisesRegex(ValueError,'input bytes changed'):
                qualified_owned_inputs(directory,'1'*64,.005)

    def test_owned_reverse_witness_reads_xyz_not_original_uv(self):
        from types import SimpleNamespace
        from brepgpu.nvidia_reference import write_source_witnesses
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory)
            points=((1.,2.,3.),(4.,2.,3.),(1.,8.,3.))
            data=(struct.pack('<8sII8Q',b'BGMESH01',1,0,1,0,3,1,0,1,0,0)
                +struct.pack('<5Q',41,0,3,0,1)
                +b''.join(struct.pack('<5dIIQ',*p,17.+i,29.+i,0,1,i) for i,p in enumerate(points))
                +struct.pack('<4IQ',0,1,2,0,901)+struct.pack('<4d',.5,.5,.5,.5))
            scene=directory/'qualified.bgmesh';scene.write_bytes(data)
            definitions=(SimpleNamespace(id=41,face_ids=(901,)),)
            self.assertEqual(write_source_witnesses(scene,directory/'probes',definitions),((41,1),))
            raw=(directory/'probes/definition-41.bgprobe').read_bytes()
            self.assertEqual(raw[:16],struct.pack('<8sQ',b'BGRPROB1',1))
            self.assertEqual(struct.unpack_from('<QQ3d',raw,16),(901,0,2.,4.,3.))
            bad=bytearray(data);struct.pack_into('<Q',bad,120+3*56+16,902);scene.write_bytes(bad)
            with self.assertRaisesRegex(ValueError,'source face absent'):
                write_source_witnesses(scene,directory/'bad',definitions)

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

    def test_fresh_authored_batch_packs_original_occurrences_and_genuine_bytes(self):
        from brepgpu.reference_source import load_reference_source
        from brepgpu.imported_assembly import assemble
        from brepgpu.nvidia_reference import load_definition
        root=Path(__file__).resolve().parents[1]
        import sys
        sys.path.insert(0,str(root/'examples'))
        from om10_scene import View,pack_views,authored_material
        from render_om10_nvidia import tsv,matched_inputs,digest
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory);control=directory/'control'
            subprocess.run([str(root/'build/om10-reference-source/brepgpu-reference-source'),
                str(root/'tests/fixtures/reference-source.step'),str(control)],check=True,
                stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            original=load_reference_source(control/'source.bgocct');assembly=assemble(original)
            matched=directory/'matched';matched.mkdir()
            metadata=(('source_sha256',digest(root/'tests/fixtures/reference-source.step')),('tolerance_mm',.005))
            tsv(matched/'metadata.tsv',('field','value'),metadata)
            tsv(matched/'bom.tsv',('occurrence_id','source_usage_path','product_id','solid_id',
                *(f'matrix_{i}' for i in range(12))),
                ((o.id,'/'.join(map(str,o.path)),o.product_id,o.definition_id,*o.transform) for o in assembly.occurrences))
            tsv(matched/'explosion.tsv',('occurrence_id','world_offset_x_mm','world_offset_y_mm','world_offset_z_mm'),
                ((o.id,0.,0.,4.*(o.id-1)) for o in assembly.occurrences))
            views=tuple(View(4,4,1,float(i),(1.,0.,0.,0.,1.,0.,0.,0.,1.),
                            (0.,0.,0.),(0.,0.),1.) for i in range(2))
            (matched/'views.bgview').write_bytes(pack_views(views))
            with self.assertRaisesRegex(ValueError,'source SHA'):
                matched_inputs(matched,assembly,'0'*64,.005)
            with self.assertRaisesRegex(ValueError,'chord height'):
                matched_inputs(matched,assembly,metadata[0][1],.01)
            output=directory/'actual'
            result=subprocess.run([sys.executable,str(root/'examples/render_om10_nvidia.py'),
                '--step',str(root/'tests/fixtures/reference-source.step'),'--matched',str(matched),
                '--output',str(output),'--heal','0','--workers','2','--mode','render','--allow-fixture'],
                stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            data=(output/'scene.bgrefmesh').read_bytes()
            h=struct.unpack_from('<8sII6Q',data)
            self.assertEqual(h[3:5],(2,3))
            at=64+2*40
            for i,o in enumerate(assembly.occurrences):
                row=struct.unpack_from('<QII15d',data,at+i*136)
                self.assertEqual(row[0],o.id);self.assertEqual(row[3:15],o.transform)
                self.assertEqual(row[15:18],(0.,0.,4.*i))
            meshes={}
            for path in output.glob('worker-*/*.bgrefmesh'):
                mesh=load_definition(path);meshes[mesh.id]=mesh
            start=64+2*40+3*136
            self.assertEqual(data[start:start+56*h[5]],meshes[41].vertex_bytes+meshes[395].vertex_bytes)
            triangle_at=start+56*h[5]
            for index,d in enumerate(original.definitions):
                expected=authored_material(original.product(d.product_id).name)
                for _ in meshes[d.id].triangles:
                    self.assertEqual(struct.unpack_from('<I',data,triangle_at+12)[0],expected)
                    triangle_at+=24
            self.assertEqual((output/'views.bgview').read_bytes(),(matched/'views.bgview').read_bytes())
            for i,name in enumerate(('assembled','exploded')):
                self.assertEqual(len((output/'native'/f'frame-{i:06d}.rgba').read_bytes()),64)
                self.assertTrue((output/f'{i:03d}-{name}.svg').is_file())
                svg=(output/f'{i:03d}-{name}.svg').read_text()
                self.assertIn('Genuine NVIDIA USD/BRep CPU tessellation',svg)
                self.assertNotIn('Owned tessellation',svg)


if __name__ == '__main__':
    unittest.main()
