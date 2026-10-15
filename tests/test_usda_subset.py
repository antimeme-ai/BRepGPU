"""Literal contracts for the bounded external ASCII boundary."""
from dataclasses import FrozenInstanceError
import struct
import unittest


def document(body, metadata=""):
    return ('#usda 1.0\n' + metadata + '\ndef Xform "World" { def BrepArray "shape" { '
            + body + ' } }').encode()


class StrictSourceTests(unittest.TestCase):
    def api(self):
        from brepgpu import usda_subset
        return usda_subset

    def test_entry_and_literal_types(self):
        from brepgpu import Patch
        self.assertEqual(Patch(0, 0, [(0, 0, 0, 1)]).controls, (0., 0., 0., 1.))
        p = self.api()
        value = p.parse(document('uniform uint[] ids = [0, 4294967295] '
            'uniform point3d[] xyz = [(1, -2.5, 3e-2)] '
            'uniform int2[] edge = [(-2147483648, 2147483647)] '
            'uniform token[] names = ["same", "opposite"] '
            'uniform bool[] flags = [true, false] uniform double[] empty = []'))
        self.assertEqual([(q.path,q.type_name) for q in value.prims],
                         [('/World','Xform'),('/World/shape','BrepArray')])
        a = value.prims[1].attributes
        self.assertEqual([(x.name,x.type_name,x.value) for x in a], [
            ('ids','uint',(0,4294967295)),('xyz','point3d',((1.,-2.5,.03),)),
            ('edge','int2',((-2147483648,2147483647),)),
            ('names','token',('same','opposite')),('flags','bool',(True,False)),
            ('empty','double',())])
        self.assertIs(type(a[0].value[0]), int)
        self.assertIs(type(a[1].value[0][0]), float)
        self.assertIs(type(a[4].value[0]), bool)
        with self.assertRaises(FrozenInstanceError): value.prims = ()
        with self.assertRaises(FrozenInstanceError): a[0].value = ()

    def test_metadata_paths_and_escapes(self):
        p=self.api()
        data=b'''#usda 1.0
(defaultPrim = "World" upAxis = "Z" metersPerUnit = 0.01)
def Xform "World" { def Scope "group" { def BrepArray "shape" (
 prepend apiSchemas = ["BrepSurfaceNurbAPI"]
 customData = { string source = "a\\\\b\\\"c\\n" }
) { uniform string[] a:b = ["x\\t", "y"] } } }
'''
        d=p.parse(data)
        self.assertEqual(d.metadata,(('defaultPrim','World'),('upAxis','Z'),('metersPerUnit',.01)))
        self.assertEqual(d.prims[-1].path,'/World/group/shape')
        self.assertEqual(d.prims[-1].metadata,
            (('apiSchemas',('BrepSurfaceNurbAPI',)),('customData',(('source','a\\b"c\n'),))))
        self.assertEqual(d.prims[-1].attributes[0].value,('x\t','y'))

    def test_numeric_widths_and_comments(self):
        p=self.api()
        d=p.parse(document('''# comment
uniform float[] f = [0.1, -0.0, 1e-45,]
uniform vector3d[] v = [(0.1, 2, +3)]
uniform double4[] d = [(0,1,2,3)]
uniform float2[] a = [(0.1, 2)] uniform int4[] b = [(0,1,-2,+3)]'''))
        attrs=d.prims[-1].attributes
        self.assertEqual(attrs[0].value[0],struct.unpack('<f',b'\xcd\xcc\xcc=')[0])
        self.assertEqual(struct.pack('<d',attrs[0].value[1]),b'\0\0\0\0\0\0\0\x80')
        self.assertEqual(attrs[0].value[2],2**-149)
        self.assertEqual(attrs[1].value,((.1,2.,3.),))

    def test_binary32_direct_decimal_midpoints(self):
        p=self.api()
        d=p.parse(document('uniform float[] values = ['
            '1.0000000596046447753906250000000000000000000001,'
            '1.0000000596046447753906249999999999999999999999,'
            '1.000000059604644775390625,'
            '-1.0000000596046447753906250000000000000000000001]'))
        self.assertEqual(d.prims[-1].attributes[0].value,
            (1+2**-23,1.,1.,-(1+2**-23)))

    def test_bad_shapes_and_numbers(self):
        p=self.api()
        for declaration in ('uint[] x = [-1]','int[] x = [2147483648]',
                'uint[] x = [1.0]','uint[] x = [true]','double[] x = [nan]',
                'double[] x = [inf]','double[] x = [1e999]',
                'float[] x = [1e39]','point3d[] x = [(1,2)]',
                'point3d[] x = [(1,2,3,4)]','point3d[] x = [1,2,3]',
                'token[] x = [same]','bool[] x = [1]','double[] x = [1 2]',
                'double[] x = [,1]','double[] x = [1,,2]'):
            with self.subTest(declaration=declaration):
                with self.assertRaises(p.SourceError): p.parse(document('uniform '+declaration))

    def test_duplicates_and_invalid_names(self):
        p=self.api()
        for data in (document('uniform double[] x=[] uniform double[] x=[]'),
                b'#usda 1.0\ndef Scope "a" {} def Scope "a" {}',
                b'#usda 1.0\ndef Xform "a" { def Scope "b" {} def Scope "b" {} }',
                b'#usda 1.0\n(defaultPrim="a" defaultPrim="b")',
                b'#usda 1.0\ndef BrepArray "a" (apiSchemas=[] prepend apiSchemas=[]) {}',
                b'#usda 1.0\ndef BrepArray "a" (customData={string source="a" string source="b"}) {}',
                b'#usda 1.0\ndef Scope "bad/name" {}',
                document('uniform double[] bad::name=[]'),
                document('uniform double[] 2bad=[]')):
            with self.subTest(data=data):
                with self.assertRaises(p.SourceError): p.parse(data)

    def test_unsupported_scene(self):
        p=self.api()
        for data in (b'#usda 1.0\n(subLayers=[@x.usda@])',
                b'#usda 1.0\ndef Xform "a" (references=@x@) {}',
                b'#usda 1.0\nover "a" {}',b'#usda 1.0\nclass "a" {}',
                b'#usda 1.0\ndef Mesh "a" {}',
                document('uniform matrix4d[] x=[]'),
                document('rel a = </World>'),
                document('double xformOp:translate = 1'),
                document('uniform double[] x.timeSamples = {}'),
                b'#usda 1.0\ndef Xform "a" (kind="component") {}'):
            with self.subTest(data=data):
                with self.assertRaises(p.UnsupportedSource): p.parse(data)

    def test_invalid_bytes_and_complete_consumption(self):
        p=self.api()
        for data in (b'',b'PXR-USDC',b'#usda 2.0\n',b'#usda 1.0\r\r\n',b'#usda 1.0\n\xff',
                document('')+b' garbage',document('')[:-2],
                document('uniform string[] x=["bad\\q"]'),
                document('uniform string[] x=["unfinished]')):
            with self.subTest(data=data):
                with self.assertRaises(p.SourceError): p.parse(data)
        with self.assertRaises(TypeError): p.parse('#usda 1.0\n')
        with self.assertRaises(TypeError): p.parse(bytearray(b'#usda 1.0\n'))

    def test_each_resource_budget(self):
        p=self.api()
        with self.assertRaises(p.SourceBudgetError):p.parse(b' '* (p.MAX_BYTES+1))
        # Mutate only declared resource constants so tiny literal inputs directly
        # reach each production admission path without giant artificial fixtures.
        from unittest.mock import patch
        cases=[('MAX_TOKENS',5,document('')),
               ('MAX_COMPONENTS',2,document('uniform double[] x=[1,2,3]')),
               ('MAX_PRIMS',1,document('')),
               ('MAX_DEPTH',1,document(''))]
        for name,limit,data in cases:
            with self.subTest(name=name):
                with patch.object(p,name,limit):
                    with self.assertRaises(p.SourceBudgetError):p.parse(data)

    def test_wrapper_authored_properties_rejected(self):
        p=self.api()
        with self.assertRaises(p.UnsupportedSource):
            p.parse(b'#usda 1.0\ndef Xform "a" { uniform double[] xformOp:translate=[1,2,3] }')
        with self.assertRaises(p.UnsupportedSource):
            p.parse(b'#usda 1.0\ndef BrepArray "a" { def Scope "child" {} }')


if __name__=='__main__':unittest.main()
