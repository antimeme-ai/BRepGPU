"""Independent tiny raw-mesh inputs; fleet-only execution, no watch raster."""
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

MAX = (1 << 64)-1
MATRIX = (1.,0.,0.,0., 0.,1.,0.,0., 0.,0.,1.,0.)
POINTS = ((0.,0.,1.), (2.,0.,1.), (0.,-2.,1.))


def mesh(points=POINTS, normals=None, triangles=((0,1,2),), instances=None, keys=None):
    normals = normals or ((0.,0.,1.),)*len(points)
    instances = instances or ((7,MATRIX,(0.,0.,0.)),)
    keys=tuple(range(len(points))) if keys is None else keys
    return (struct.pack('<8sII6Q',b'BGREFM01',1,0,1,len(instances),len(points),len(triangles),1,0)
        +struct.pack('<5Q',41,0,len(points),0,len(triangles))
        +b''.join(struct.pack('<QII15d',id,0,0,*matrix,*offset) for id,matrix,offset in instances)
        +b''.join(struct.pack('<6dQ',*p,*n,key) for p,n,key in zip(points,normals,keys))
        +b''.join(struct.pack('<4IQ',*t,0,0) for t in triangles)
        +struct.pack('<4d',.2,.4,.8,.5))


def views(*, alpha=0., supersample=1, flags=0):
    return struct.pack('<8sII',b'BGVIEW01',1,1)+struct.pack('<4I19dII2d',
        4,4,supersample,0,alpha,1.,0.,0.,0.,1.,0.,0.,0.,1.,
        0.,0.,0.,0.,0.,1.,0.,0.,0.,flags,0,0.,0.)


class RawDisplayTests(unittest.TestCase):
    def run_display(self, directory, data=None, request=None, *, success=True):
        directory=Path(directory);source=directory/'scene.bgrefmesh';view=directory/'views.bgview'
        source.write_bytes(mesh() if data is None else data)
        view.write_bytes(views() if request is None else request)
        output=directory/'output'
        executable=Path(os.environ.get('BREPGPU_OM10_REFERENCE',
            'build/om10-reference/brepgpu-om10-reference-display')).resolve()
        process=subprocess.run((str(executable),'--mesh',str(source),'--views',str(view),
            '--threads',os.environ.get('BREPGPU_OM10_THREADS','1'),
            '--output-directory',str(output)),capture_output=True,text=True)
        if success:self.assertEqual(process.returncode,0,process.stderr)
        else:self.assertNotEqual(process.returncode,0,'invalid raw input accepted')
        return output,process

    def test_exact_raw_triangle_pixels_ids_depth(self):
        with tempfile.TemporaryDirectory() as directory:
            out,_=self.run_display(directory)
            ids=struct.unpack('<16Q',(out/'frame-000000.primitive').read_bytes())
            instances=struct.unpack('<16Q',(out/'frame-000000.instance').read_bytes())
            depths=struct.unpack('<16d',(out/'frame-000000.depth').read_bytes())
            # The down-left hypotenuse is not top-left: pixel centers1/4
            # lie exactly on it and are excluded. Only(.5,.5) is interior.
            self.assertEqual(ids,tuple(0 if i==0 else MAX for i in range(16)))
            self.assertEqual(instances,tuple(7 if i==0 else MAX for i in range(16)))
            self.assertEqual(depths,tuple(1. if i==0 else -math.inf for i in range(16)))
            rgba=(out/'frame-000000.rgba').read_bytes()
            self.assertEqual(rgba[3::4],bytes([255]*16))
            self.assertEqual(rgba[8:12],bytes((0,0,0,255)))
            self.assertFalse((out/'source-jets.bin').exists())

    def test_shared_edge_opposite_winding_and_supersample(self):
        points=((0.,0.,1.),(2.,0.,1.),(2.,-2.,1.),(0.,-2.,1.))
        for triangles in (((0,1,2),(0,2,3)),((0,2,1),(0,3,2))):
            with self.subTest(triangles=triangles),tempfile.TemporaryDirectory() as directory:
                out,_=self.run_display(directory,mesh(points,triangles=triangles),views(supersample=2,flags=4))
                final=struct.unpack('<16Q',(out/'frame-000000.primitive').read_bytes())
                self.assertEqual(final,tuple(({0:0,1:0,4:1,5:0}).get(i,MAX) for i in range(16)))
                data=(out/'frame-000000.samples').read_bytes()
                self.assertEqual(data[:24],struct.pack('<8sQQ',b'BGSAMP01',8,8))
                samples=tuple(struct.iter_unpack('<dQQ',data[24:]))
                for y in range(8):
                    for x in range(8):
                        expected=(1.,0 if x>=y else 1,7) if x<4 and y<4 else (-math.inf,MAX,MAX)
                        self.assertEqual(samples[8*y+x],expected)

    def test_front_tie_explosion_and_projection_normals(self):
        lifted=(*MATRIX[:11],2.)
        for instances,alpha,primitive,instance,depth in (
            (((7,MATRIX,(0.,0.,0.)),(9,lifted,(0.,0.,0.))),0.,1,9,3.),
            (((7,MATRIX,(0.,0.,0.)),(9,MATRIX,(0.,0.,0.))),0.,0,7,1.),
            (((7,MATRIX,(0.,0.,4.)),(9,lifted,(0.,0.,0.))),1.,0,7,5.)):
            with self.subTest(alpha=alpha,depth=depth),tempfile.TemporaryDirectory() as directory:
                out,_=self.run_display(directory,mesh(instances=instances),views(alpha=alpha))
                self.assertEqual(struct.unpack('<16Q',(out/'frame-000000.primitive').read_bytes())[0],primitive)
                self.assertEqual(struct.unpack('<16Q',(out/'frame-000000.instance').read_bytes())[0],instance)
                self.assertEqual(struct.unpack('<16d',(out/'frame-000000.depth').read_bytes())[0],depth)
        with tempfile.TemporaryDirectory() as directory:
            two_views=struct.pack('<8sII',b'BGVIEW01',1,2)+views()[16:]+views(alpha=1.)[16:]
            out,_=self.run_display(directory,mesh(instances=((7,MATRIX,(0.,0.,4.)),)),two_views)
            self.assertEqual(struct.unpack('<16d',(out/'frame-000000.depth').read_bytes())[0],1.)
            self.assertEqual(struct.unpack('<16d',(out/'frame-000001.depth').read_bytes())[0],5.)
        rotate=(0.,-1.,0.,0.,1.,0.,0.,0.,0.,0.,1.,0.)
        with tempfile.TemporaryDirectory() as directory:
            normals=((2.,0.,0.),)*3
            out,_=self.run_display(directory,mesh(normals=normals,instances=((7,rotate,(1.,2.,3.)),)),views(alpha=1.,flags=3))
            self.assertFalse(tuple(out.glob('*.rgba')))
            data=(out/'frame-000000.projected').read_bytes()
            self.assertEqual(data[:16],struct.pack('<8sQ',b'BGPROJ01',3))
            rows=tuple(struct.iter_unpack('<3Q2q7d',data[16:]))
            for i,row in enumerate(rows):
                self.assertEqual(row[:3],(7,41,i))
                self.assertEqual(row[-3:],(0.,1.,0.))
                x,y,z=POINTS[i]
                self.assertEqual(row[6:9],(1.-y,2.+x,3.+z))

    def test_duplicate_physical_corner_keeps_normal_and_source_keys(self):
        points=POINTS+POINTS
        normals=((0.,0.,1.),)*3+((0.,0.,-2.),)*3
        with tempfile.TemporaryDirectory() as directory:
            out,_=self.run_display(directory,mesh(points,normals,((0,1,2),(3,4,5)),keys=(0,1,2,0,1,2)),views(flags=3))
            rows=tuple(struct.iter_unpack('<3Q2q7d',(out/'frame-000000.projected').read_bytes()[16:]))
            self.assertEqual(tuple(r[-3:] for r in rows),((0.,0.,1.),)*3+((0.,0.,-1.),)*3)
            self.assertEqual(tuple(r[2] for r in rows),tuple(range(6)))

    def test_reject_bad_records_and_nonfinite_projection(self):
        valid=mesh();invalid=[]
        invalid.extend((valid[:-1],valid+b'X'))
        changed=bytearray(valid);struct.pack_into('<Q',changed,16,MAX);invalid.append(changed)
        invalid.append(mesh(normals=((0.,0.,0.),)*3))
        invalid.append(mesh(points=((math.nan,0.,1.),*POINTS[1:])))
        invalid.append(mesh(triangles=((0,1,99),)))
        invalid.append(mesh(keys=(0,0,2)))
        invalid.append(mesh(instances=((MAX,MATRIX,(0.,0.,0.)),)))
        reflected=(-1.,*MATRIX[1:]);invalid.append(mesh(instances=((7,reflected,(0.,0.,0.)),)))
        huge=(1.,0.,0.,1e308,0.,1.,0.,0.,0.,0.,1.,0.)
        invalid.append(mesh(instances=((7,huge,(0.,0.,0.)),)))
        for i,data in enumerate(invalid):
            with self.subTest(i=i),tempfile.TemporaryDirectory() as directory:
                _,process=self.run_display(directory,data,success=False)
                self.assertTrue(process.stderr.strip())
        with tempfile.TemporaryDirectory() as directory:
            Path(directory,'output').mkdir()
            Path(directory,'output','sentinel').write_text('retain')
            _,process=self.run_display(directory,success=False)
            self.assertIn('destination must not exist',process.stderr)
            self.assertEqual(Path(directory,'output','sentinel').read_text(),'retain')

    def test_identical_plane_matches_qualified_cpu_outputs(self):
        from test_om10_native import plane_scene, views as owned_views, POINTS as owned_points, MATRIX as owned_matrix
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);reference=root/'reference';reference.mkdir()
            request=owned_views(raster=True)
            out,_=self.run_display(reference,mesh(owned_points,((0.,0.,-1.),)*3,
                instances=((7,owned_matrix,(0.,0.,0.)),)),request)
            (root/'owned.bgmesh').write_bytes(plane_scene())
            (root/'views.bgview').write_bytes(request)
            owned=Path(os.environ.get('BREPGPU_OM10_NATIVE','build/om10-release/brepgpu-om10-native')).resolve()
            result=subprocess.run((str(owned),'--scene',str(root/'owned.bgmesh'),
                '--views',str(root/'views.bgview'),'--output-directory',str(root/'owned-output'),
                '--backend','cpu','--threads',os.environ.get('BREPGPU_OM10_THREADS','1')),
                capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            for suffix in ('rgba','depth','primitive','instance','samples'):
                self.assertEqual((out/('frame-000000.'+suffix)).read_bytes(),
                    (root/'owned-output'/('frame-000000.'+suffix)).read_bytes(),suffix)


if __name__=='__main__':unittest.main()
