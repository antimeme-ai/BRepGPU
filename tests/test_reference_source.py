"""Direct original-source export contracts; execute on the fleet only."""
import os
import dataclasses
import hashlib
import math
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class ReferenceSourceTests(unittest.TestCase):
    def export(self, directory, *, source=None, success=True):
        executable = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE',
            'build/om10-reference-source/brepgpu-reference-source')).resolve()
        source = source or Path(__file__).parent/'fixtures'/'reference-source.step'
        completed = subprocess.run((str(executable), str(source), str(directory)),
            capture_output=True, text=True)
        if success:
            self.assertEqual(completed.returncode, 0, completed.stderr)
        else:
            self.assertNotEqual(completed.returncode, 0, 'invalid export accepted')
        return completed

    def test_fresh_original_source_export(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(__file__).parent/'fixtures'/'reference-source.step'
            completed=self.export(Path(directory)/'export', source=source)
            self.assertIn('shape_process_before_transfer\t1\t0',completed.stdout)
            self.assertIn('shape_process_after_transfer\t1\t0',completed.stdout)
            from brepgpu.reference_source import load_reference_source
            from brepgpu.imported_assembly import assemble
            result = load_reference_source(Path(directory)/'export'/'source.bgocct')
            self.assertEqual(result.source_sha256, hashlib.sha256(source.read_bytes()).hexdigest())
            self.assertEqual(result.units_mm, 1.)
            self.assertEqual(result.root_product_id, 5)
            self.assertEqual(tuple(d.id for d in result.definitions), (41, 395))
            self.assertEqual(tuple(len(d.faces) for d in result.definitions), (6, 3))
            self.assertEqual(tuple(u.id for u in result.usages), (380, 386, 502))
            assembly = assemble(result)
            self.assertEqual(tuple(o.definition_id for o in assembly.occurrences), (41, 41, 395))
            self.assertEqual(assembly.occurrences[1].transform[3::4], (4., -3., 2.))
            for a, b in zip(assembly.occurrences[1].transform,
                    (0.,-1.,0.,4.,1.,0.,0.,-3.,0.,0.,1.,2.)):
                self.assertAlmostEqual(a, b, places=14)
            self.assertEqual(tuple(f.orientation for f in result.definitions[0].faces), (-1,1,-1,1,-1,1))
            self.assertTrue(all(math.isfinite(t) and t >= 0 for t in result.phase_seconds.values()))
            with self.assertRaises(dataclasses.FrozenInstanceError):
                result.units_mm = 10.
            with self.assertRaises(TypeError):
                result.phase_seconds['read_step'] = 0.
            with self.assertRaises(TypeError):
                dataclasses.replace(result, products=list(result.products))

    def test_v4_readback_geometry_orientation_and_absent_mesh(self):
        inspector = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE_FIXTURE',
            'build/om10-reference-source/brepgpu-reference-source-fixture')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/'export'
            self.export(output)
            for id in (41,395):
                path = output/f'definition-{id}.brep'
                self.assertTrue(path.read_bytes().startswith(b'\nOpen CASCADE Topology V4, (c) Open Cascade\n'))
                completed = subprocess.run((str(inspector),'--inspect',str(path)),capture_output=True,text=True)
                self.assertEqual(completed.returncode, 0, completed.stderr)
                rows = tuple(line.split() for line in completed.stdout.splitlines())
                points = {tuple(map(float,row[1:])) for row in rows if row[0]=='vertex'}
                planes = [tuple(map(float,row[1:])) for row in rows if row[0]=='plane']
                oriented = {(tuple(sign*n for n in normal),sign*sum(x*n for x,n in zip(origin,normal)))
                    for sign,*values in planes for origin,normal in [(values[:3],values[3:])]}
                if id == 41:
                    self.assertEqual(points,{(x,y,z) for x in (.125,1.125) for y in (.25,2.25) for z in (.5,3.5)})
                    self.assertEqual(oriented,{((-1.,0.,0.),-.125),((1.,0.,0.),1.125),
                        ((0.,-1.,0.),-.25),((0.,1.,0.),2.25),((0.,0.,-1.),-.5),((0.,0.,1.),3.5)})
                else:
                    self.assertEqual(points,{(-.75,2.25,.125),(-.75,2.25,2.125)})
                    self.assertEqual(oriented,{((0.,0.,-1.),-.125),((0.,0.,1.),2.125)})
                    self.assertEqual([tuple(map(float,row[1:])) for row in rows if row[0]=='cylinder'],
                        [(1.,-1.5,2.25,.125,0.,0.,1.,.75)])

    def test_malformed_manifest_graph_paths_and_partial_output_raise(self):
        from brepgpu.reference_source import load_reference_source
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/'export'
            self.export(output)
            path=output/'source.bgocct';raw=path.read_bytes();offsets={};at=88
            while at<len(raw):
                tag=raw[at:at+8];count,size=struct.unpack_from('<QQ',raw,at+8);p=at+24;entries=[]
                for _ in range(count):
                    n=struct.unpack_from('<Q',raw,p)[0];entries.append(p+8);p+=8+n
                offsets[tag]=entries;at+=24+size
            cases=[raw[:cut] for cut in (0,8,87,len(raw)-1)]+[raw+b'\0']
            def changed(offset,fmt,value):
                bad=bytearray(raw);struct.pack_into('<'+fmt,bad,offset,value);cases.append(bad)
            changed(8,'I',2);changed(12,'I',1);changed(48,'d',10.);changed(48,'d',float('nan'))
            changed(96,'Q',2**64-1)
            changed(offsets[b'PRODUCTS'][0]+8,'Q',999)
            meta=offsets[b'META____'][0];length=struct.unpack_from('<I',raw,meta)[0]
            changed(meta+4+length+8,'d',-1.);changed(meta+4+length+8,'d',float('nan'))
            definition=offsets[b'DEFS____'][0];length=struct.unpack_from('<I',raw,definition+16)[0]
            name=definition+20;firstface=name+length+8
            bad=bytearray(raw);bad[name:name+length]=b'../'+b'x'*(length-3);cases.append(bad)
            index=struct.unpack_from('<Q',raw,firstface+16)[0]
            changed(firstface+32+16,'Q',index);changed(firstface+24,'i',0)
            for bad in cases:
                path.write_bytes(bad)
                with self.assertRaises(ValueError):
                    load_reference_source(path)
            path.write_bytes(raw)
            (output/'definition-41.brep').unlink()
            with self.assertRaisesRegex(ValueError,'missing'):
                load_reference_source(path)
            with self.assertRaises(OSError):
                load_reference_source(Path(directory)/'never-published.bgocct')

    def test_native_non_mm_existing_directory_and_write_failure_raise(self):
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory)
            self.export(directory,success=False)
            self.export(directory/'absent-parent'/'output',success=False)
            source=directory/'cm.step'
            original=Path(__file__).parent/'fixtures'/'reference-source.step'
            source.write_bytes(original.read_bytes().replace(b'.MILLI.',b'.CENTI.'))
            self.export(directory/'cm-export',source=source,success=False)
            self.assertFalse((directory/'cm-export'/'source.bgocct').exists())


if __name__ == '__main__':
    unittest.main()
