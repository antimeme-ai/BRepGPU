"""Independent binary/pixel/normal answers for the native source display profile."""
import math
import os
from pathlib import Path
import struct
import tempfile
import unittest
from test_om10_native import NativeDirectTests, plane_scene, views, scalar_pixels


def source_plane(*, vertex_flags=33):
    points=((0.,0.,.125),(2.,0.,.25),(0.,-2.,.5))
    matrix=(1.,0.,0.,0.,0.,1.,0.,0.,.25,0.,1.,1.)
    instances=((7,matrix,(0.,0.,0.)),)
    data=bytearray(plane_scene(points=points,instances=instances))
    struct.pack_into('<I',data,12,1)
    for i in range(3):struct.pack_into('<I',data,80+40+136+i*56+44,vertex_flags)
    carrier=80+40+136+3*56+24
    # Original axes remain almost orthogonal, without rewriting a 1e-9 skew.
    struct.pack_into('<d',data,carrier+16+6*8,1e-9)
    return bytes(data),points,instances


class NativeSourceDisplayTests(unittest.TestCase):
    run_native=NativeDirectTests.run_native

    def test_plane_unused_z_axis_does_not_change_regular_source(self):
        scene,_,_=source_plane()
        carrier=80+40+136+3*56+24
        for z in ((0.,0.,0.),(1e300,-1e300,1e300)):
            with self.subTest(z=z),tempfile.TemporaryDirectory() as directory:
                data=bytearray(scene)
                struct.pack_into('<3d',data,carrier+16+9*8,*z)
                out=self.run_native(directory,raster=False,scene_bytes=bytes(data))
                row=struct.unpack_from('<15dII',(out/'source-jets.bin').read_bytes(),16)
                self.assertEqual(row[9:12],(0.,0.,1.))

    def test_stored_xyz_and_actual_affine_cofactor(self):
        scene,points,instances=source_plane()
        with tempfile.TemporaryDirectory() as directory:
            out=self.run_native(directory,raster=True,scene_bytes=scene,
                                view_bytes=views(raster=True,flags=6))
            jets=(out/'source-jets.bin').read_bytes()
            for i,point in enumerate(points):
                values=struct.unpack_from('<15dII',jets,16+i*128)
                self.assertEqual(values[12:15],point)
                self.assertEqual(values[16]&2,2)
            projected=(out/'frame-000000.projected').read_bytes()
            expected=(-.25/math.sqrt(1.0625),0.,1/math.sqrt(1.0625))
            for i in range(3):
                row=struct.unpack_from('<3Q2q7d',projected,16+i*96)
                for actual,want in zip(row[-3:],expected):
                    self.assertAlmostEqual(actual,want,places=14)
            samples=(out/'frame-000000.samples').read_bytes()
            oracle=scalar_pixels(points,((0,1,2),),instances)
            for i,(depth,primitive,instance) in enumerate(oracle):
                actual=struct.unpack_from('<dQQ',samples,24+i*24)
                self.assertEqual(actual[1:],(primitive,instance))
                if math.isfinite(depth):self.assertAlmostEqual(actual[0],float(depth),places=14)

    def test_declared_sphere_pole_sheet_is_not_rounded_cosine(self):
        from brepgpu.imported_source import Carrier
        from test_om10_native import source_jet_scene
        frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
        carrier=Carrier(101,4,frame,(1.,0.),(0.,1.,-2.,2.))
        # Choose the finite double on the OTHER side of exact pi/2. Its normal
        # still follows the declared positive regular sheet at the north pole.
        uv=(.25,math.nextafter(math.pi/2,math.inf))
        south=(.25,math.nextafter(-math.pi/2,-math.inf))
        raw,_=source_jet_scene(((carrier,(uv,uv,south)),))
        data=bytearray(raw);struct.pack_into('<I',data,12,1)
        for i,flags in enumerate((37,45,53)):
            struct.pack_into('<I',data,80+40+136+i*56+44,flags)
        with tempfile.TemporaryDirectory() as directory:
            out=self.run_native(directory,raster=False,scene_bytes=bytes(data))
            jets=(out/'source-jets.bin').read_bytes()
            for i,want in enumerate(((0.,0.,1.),(0.,0.,-1.),(0.,0.,-1.))):
                row=struct.unpack_from('<15dII',jets,16+i*128)
                self.assertEqual(row[9:12],want)
                self.assertEqual(row[16]&3,3)

    def test_declared_cone_sheet_and_face_reversal(self):
        from brepgpu.imported_source import Carrier
        from test_om10_native import source_jet_scene
        angle=math.pi/6
        frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
        carrier=Carrier(102,3,frame,(1.,angle),(0.,1.,-3.,0.))
        uv=(0.,-1/math.sin(angle))
        raw,_=source_jet_scene(((carrier,(uv,uv)),))
        data=bytearray(raw);struct.pack_into('<I',data,12,1)
        for i,flags in enumerate((39,45)):
            struct.pack_into('<I',data,80+40+136+i*56+44,flags)
        with tempfile.TemporaryDirectory() as directory:
            out=self.run_native(directory,raster=False,scene_bytes=bytes(data))
            jets=(out/'source-jets.bin').read_bytes()
            for i in range(2):
                row=struct.unpack_from('<15dII',jets,16+i*128)
                for value,want in zip(row[9:12],(-math.cos(angle),0.,math.sin(angle))):
                    self.assertAlmostEqual(value,want,places=14)
                self.assertEqual(row[16]&3,3)

    def test_invalid_profile_flags_fail_before_output(self):
        from test_om10_native_failures import NativeFailureTests
        runner=NativeFailureTests()
        for flags in (1,32,33|8,33|16,33|4,33|64):
            with self.subTest(flags=flags),tempfile.TemporaryDirectory() as directory:
                scene,_,_=source_plane(vertex_flags=flags)
                result,out=runner.invoke(directory,scene)
                runner.assert_failed(result,out,'decode:')


if __name__=='__main__':unittest.main()
