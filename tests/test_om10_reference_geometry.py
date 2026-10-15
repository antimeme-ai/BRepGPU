"""Finite authored source-boundary/triangle checks; no watch or render."""
import csv
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
from test_om10_reference import mesh

BUILD=Path(os.environ.get('BREPGPU_OM10_REFERENCE_GEOMETRY_BUILD','build/om10-reference-geometry')).resolve()


def records(path):
    with path.open() as stream:return tuple(csv.DictReader(stream,delimiter='\t'))


class GeometryTests(unittest.TestCase):
    def fixture(self,root):
        result=subprocess.run((str(BUILD/'brepgpu-reference-geometry-fixture'),str(root/'source')),
            text=True,capture_output=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def query(self,root,kind,points,witnesses,*,tolerance='.005',request_bytes=None,triangles=((0,1,2),),mesh_bytes=None,file_limit=None):
        raw=root/'mesh.bgrefmesh';data=mesh(points,triangles=triangles)
        # This is a definition-only producer file: remove its sole136-byte instance.
        data=bytearray(data[:104]+data[240:]);struct.pack_into('<Q',data,24,0)
        raw.write_bytes(data if mesh_bytes is None else mesh_bytes)
        probe=root/'source.bgprobe'
        probe.write_bytes(request_bytes if request_bytes is not None else struct.pack('<8sQ',b'BGRPROB1',len(witnesses))+
            b''.join(struct.pack('<QQ3d',19,i,*point) for i,point in enumerate(witnesses)))
        output=root/'output'
        def limit_file_size():
            import resource
            import signal
            signal.signal(signal.SIGXFSZ,signal.SIG_IGN)
            resource.setrlimit(resource.RLIMIT_FSIZE,(file_limit,file_limit))
        result=subprocess.run((str(BUILD/'brepgpu-om10-reference-geometry'),'--source',str(root/'source'/(kind+'.brep')),
            '--mesh',str(raw),'--witnesses',str(probe),'--tolerance',tolerance,'--output-directory',str(output)),
            text=True,capture_output=True,preexec_fn=limit_file_size if file_limit is not None else None)
        return output,result

    def test_sphere_center_measures_boundary_radius_and_rejects_coarse_chord(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            points=((1.,0.,0.),(-.5,math.sqrt(3)/2,0.),(-.5,-math.sqrt(3)/2,0.))
            out,result=self.query(root,'sphere',points,((1.,0.,0.),))
            self.assertEqual(result.returncode,1,result.stderr)
            forward=records(out/'forward.tsv');self.assertEqual(len(forward),4)
            center=next(r for r in forward if r['probe']=='centroid')
            self.assertAlmostEqual(float(center['distance_mm']),1.,places=10)
            summary={r['field']:r['value'] for r in records(out/'summary.tsv')}
            self.assertEqual(summary['status'],'FAIL')
            self.assertGreaterEqual(int(summary['forward_failures']),1)

    def test_literal_reverse_interior_edge_vertex_closest_points(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            points=((0.,0.,0.),(2.,0.,0.),(0.,2.,0.))
            out,result=self.query(root,'box',points,((.5,.5,1.),(1.5,1.5,0.),(-1.,-1.,0.)))
            self.assertEqual(result.returncode,1,result.stderr)
            answers=((1.,(.5,.25,.25),(.5,.5,0.)),
                     (math.sqrt(.5),(0.,.5,.5),(1.,1.,0.)),
                     (math.sqrt(2),(1.,0.,0.),(0.,0.,0.)))
            for row,(distance,bary,closest) in zip(records(out/'reverse.tsv'),answers):
                self.assertEqual(row['closest_nv_triangle_index'],'0')
                self.assertAlmostEqual(float(row['distance_mm']),distance,places=14)
                self.assertEqual(tuple(float(row[k]) for k in ('b0','b1','b2')),bary)
                self.assertEqual(tuple(float(row[k]) for k in ('closest_x_mm','closest_y_mm','closest_z_mm')),closest)

    def test_finite_pass_selection_union_and_lowest_triangle_tie(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            # N65 global64 skips32; the sole NVface's middle32 adds it back.
            out,result=self.query(root,'box',((0.,0.,0.),(2.,0.,0.),(0.,2.,0.)),((.5,.5,0.),),triangles=((0,1,2),)*65)
            self.assertEqual(result.returncode,0,result.stderr)
            summary={r['field']:r['value'] for r in records(out/'summary.tsv')}
            self.assertEqual(summary['status'],'PASS')
            self.assertEqual(summary['selected_nv_triangles'],'65')
            self.assertEqual(summary['forward_queries'],'260')
            self.assertEqual(summary['original_faces'],'6')
            self.assertEqual(float(summary['solver_deflection_mm']),2**-40)
            self.assertEqual(float(summary['numerical_grade_mm']),2**-32)
            self.assertEqual({int(r['nv_triangle_index']) for r in records(out/'forward.tsv')},set(range(65)))
            self.assertEqual(records(out/'reverse.tsv')[0]['closest_nv_triangle_index'],'0')

    def test_annular_trim_hole_is_not_its_untrimmed_plane(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            points=((2.,0.,0.),(-1.,math.sqrt(3),0.),(-1.,-math.sqrt(3),0.))
            out,result=self.query(root,'annulus',points,((2.,0.,0.),))
            self.assertEqual(result.returncode,1,result.stderr)
            center=next(r for r in records(out/'forward.tsv') if r['probe']=='centroid')
            self.assertAlmostEqual(float(center['distance_mm']),1.,places=10)

    def test_missing_nonlinear_pcurve_is_source_capability_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            out,result=self.query(root,'sphere-missing',((1.,0.,0.),(0.,1.,0.),(0.,0.,1.)),((1.,0.,0.),))
            self.assertEqual(result.returncode,1)
            self.assertIn('SOURCE_QUERY_PREPARATION',result.stderr)
            self.assertIn('pcurve',result.stderr)
            self.assertFalse((out/'summary.tsv').exists())

    def test_malformed_witness_mesh_tolerance_and_stale_output_fail_closed(self):
        points=((0.,0.,0.),(2.,0.,0.),(0.,2.,0.))
        header=struct.pack('<8sQ',b'BGRPROB1',1)
        bad=(b'',header,header+struct.pack('<QQ3d',19,0,math.inf,0.,0.),
             struct.pack('<8sQ',b'BGRPROB1',0),struct.pack('<8sQ',b'BGRPROB1',(1<<64)-1))
        for data in bad:
            with self.subTest(length=len(data)),tempfile.TemporaryDirectory() as directory:
                root=Path(directory);self.fixture(root)
                out,result=self.query(root,'box',points,(),request_bytes=data)
                self.assertEqual(result.returncode,1)
                self.assertFalse((out/'summary.tsv').exists())
        for value in ('0','-1','nan','inf','1e308'):
            with self.subTest(tolerance=value),tempfile.TemporaryDirectory() as directory:
                root=Path(directory);self.fixture(root)
                out,result=self.query(root,'box',points,((.5,.5,0.),),tolerance=value)
                self.assertEqual(result.returncode,1)
                self.assertFalse((out/'summary.tsv').exists())
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            out,result=self.query(root,'box',points,((.5,.5,0.),),mesh_bytes=mesh(points))
            self.assertEqual(result.returncode,1)
            self.assertIn('zero occurrences',result.stderr)
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root);(root/'output').mkdir();(root/'output'/'sentinel').write_text('retain')
            out,result=self.query(root,'box',points,((.5,.5,0.),))
            self.assertEqual(result.returncode,1)
            self.assertEqual((out/'sentinel').read_text(),'retain')

    def test_final_report_write_failure_never_publishes_success(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            out,result=self.query(root,'box',((0.,0.,0.),(2.,0.,0.),(0.,2.,0.)),((.5,.5,0.),),file_limit=512)
            self.assertEqual(result.returncode,1,result.stderr)
            self.assertIn('write/close failed',result.stderr)
            self.assertTrue((out/'summary.tsv.tmp').exists(),'did not reach final report publication')
            self.assertTrue((out/'metadata.tsv').exists())
            self.assertFalse((out/'summary.tsv').exists(),'partial execution published a successful summary')


if __name__=='__main__':unittest.main()
