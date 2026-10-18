import dataclasses
from pathlib import Path
import struct
import tempfile
import unittest

from brepgpu.imported_source import ImportedSource, Product, Usage, Curve, ZERO_FRAME, load_source, write_source, IDENTITY


def metadata_fixture():
    return ImportedSource('ab'*32,1.0,0.01,17,'independent metadata fixture',
                          (Product(17,'Movement','OM10'),),(),(),(),(),(),(),(),(),())


class ImportedSourceTests(unittest.TestCase):
    def test_typed_metadata_roundtrip_is_immutable(self):
        source=metadata_fixture()
        with tempfile.TemporaryDirectory() as td:
            path=Path(td)/'source.bin';write_source(source,path)
            restored=load_source(path)
        self.assertEqual(restored,source)
        self.assertEqual(restored.product(17).name,'Movement')
        with self.assertRaises(dataclasses.FrozenInstanceError):restored.products=()
        with self.assertRaises(TypeError):restored._indices['products'][18]=Product(18,'bad','bad')

    def test_wire_truncation_overflow_version_and_trailing_bytes_raise(self):
        with tempfile.TemporaryDirectory() as td:
            path=Path(td)/'source.bin';write_source(metadata_fixture(),path);raw=path.read_bytes()
            corruptions=[raw[:cut] for cut in (0,8,87,len(raw)-1)]
            for wrong in (1,3):  # the format is version 2 since B2-ingress
                version=bytearray(raw);struct.pack_into('<I',version,8,wrong);corruptions.append(version)
            rows=bytearray(raw);struct.pack_into('<Q',rows,96,2**64-1);corruptions.append(rows)
            corruptions.append(raw+b'\0')
            for data in corruptions:
                with self.subTest(length=len(data)):
                    path.write_bytes(data)
                    with self.assertRaises(ValueError):load_source(path)

    def test_dangling_nonrigid_and_nonfinite_source_values_raise(self):
        source=metadata_fixture()
        with tempfile.TemporaryDirectory() as td:
            path=Path(td)/'source.bin'
            for transform in (IDENTITY[:3]+(float('nan'),)+IDENTITY[4:],(2.0,)+IDENTITY[1:]):
                bad=dataclasses.replace(source,usages=(Usage(2,17,17,'bad',transform),))
                with self.assertRaises(ValueError):write_source(bad,path)
            bad=dataclasses.replace(source,products=(Product(17,'bad','bad',900),))
            with self.assertRaises(ValueError):write_source(bad,path)

    def test_direct_source_construction_cannot_smuggle_mutable_rows(self):
        source=metadata_fixture()
        with self.assertRaises(TypeError):
            dataclasses.replace(source,products=(Product(17,['mutable'],'bad'),))
        with self.assertRaises(TypeError):
            dataclasses.replace(source,products=[Product(17,'mutable','bad')])

    def test_bezier_carrier_domain_is_not_its_active_edge_interval(self):
        source=metadata_fixture()
        curve=Curve(1,3,4,ZERO_FRAME,(0.,0.),(.25,.75),1,
                    controls=(0.,0.,0.,1.,1.,0.,0.,1.))
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaisesRegex(ValueError,'natural'):
                write_source(dataclasses.replace(source,curves=(curve,)),Path(td)/'bad.bin')


if __name__=='__main__':unittest.main()
