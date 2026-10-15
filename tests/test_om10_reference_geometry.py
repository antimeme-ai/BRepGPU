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

    def query(self,root,kind,points,witnesses,*,tolerance='.005',request_bytes=None):
        raw=root/'mesh.bgrefmesh';data=mesh(points)
        # This is a definition-only producer file: remove its sole136-byte instance.
        data=bytearray(data[:104]+data[240:]);struct.pack_into('<Q',data,24,0)
        raw.write_bytes(data)
        probe=root/'source.bgprobe'
        probe.write_bytes(request_bytes if request_bytes is not None else struct.pack('<8sQ',b'BGRPROB1',len(witnesses))+
            b''.join(struct.pack('<QQ3d',19,i,*point) for i,point in enumerate(witnesses)))
        output=root/'output'
        result=subprocess.run((str(BUILD/'brepgpu-om10-reference-geometry'),'--source',str(root/'source'/(kind+'.brep')),
            '--mesh',str(raw),'--witnesses',str(probe),'--tolerance',tolerance,'--output-directory',str(output)),
            text=True,capture_output=True)
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


if __name__=='__main__':unittest.main()
