"""One actual original STEP -> OCCT triangles -> two native display frames."""
import argparse
import csv
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'python'))
from brepgpu.nvidia_reference import load_definition
from brepgpu.reference_source import load_reference_source
from brepgpu.imported_assembly import assemble


def rows(path):
    with path.open() as stream:
        return tuple(csv.DictReader(stream, delimiter='\t'))


def main():
    parser = argparse.ArgumentParser()
    for name in ('caller', 'producer', 'exporter', 'display'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--workspace', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(dir=args.workspace, prefix='occt-reference-') as directory:
        directory = Path(directory)
        output = directory/'actual'
        command = [sys.executable, '-B', str(args.caller), '--step',
                   str(ROOT/'tests/fixtures/reference-source.step'), '--producer', str(args.producer),
                   '--exporter', str(args.exporter), '--display', str(args.display),
                   '--output', str(output), '--allow-fixture', '--width', '800', '--height', '640',
                   '--display-threads', '4']
        completed = subprocess.run(command, capture_output=True, text=True, timeout=60)
        assert completed.returncode == 0, completed.stdout+completed.stderr
        source = load_reference_source(output/'source/source.bgocct')
        assembly = assemble(source)
        assert (len(source.definitions), len(source.faces), len(assembly.occurrences)) == (2, 9, 3)
        actual = rows(output/'meshes/faces.tsv')
        assert [(int(r['definition']), int(r['face'])) for r in actual] == [
            (d.id, f.id) for d in source.definitions for f in sorted(d.faces, key=lambda f:f.id)]
        assert all(int(r['triangles']) > 0 for r in actual)
        products = {p.id:p for p in source.products}
        for definition in source.definitions:
            mesh = load_definition(output/f'meshes/definition-{definition.id}.bgrefmesh',
                                   expected_id=definition.id)
            assert set(mesh.face_ids) == set(definition.face_ids)
            box = products[definition.product_id].name == 'fixture-box'
            for p,n in zip(mesh.points, mesh.normals):
                assert abs(math.hypot(*n)-1) < 1e-6
                if box:
                    lower,upper = (.125,.25,.5), (1.125,2.25,3.5)
                    axis = max(range(3), key=lambda k:abs(n[k]))
                    sign = 1 if n[axis] > 0 else -1
                    assert all(abs(n[k]-(sign if k == axis else 0)) < 1e-6 for k in range(3))
                    assert abs(p[axis]-(upper[axis] if sign > 0 else lower[axis])) < 1e-10
                    assert all(lower[k]-1e-10 <= p[k] <= upper[k]+1e-10 for k in range(3))
                elif abs(n[2]) > .5:
                    assert abs(n[2]-(1 if p[2] > 1 else -1)) < 1e-6
                    assert abs(p[2]-(2.125 if n[2] > 0 else .125)) < 1e-10
                else:
                    assert abs(math.hypot(p[0]+1.5,p[1]-2.25)-.75) < 1e-10
                    assert abs(n[0]-(p[0]+1.5)/.75) < 1e-6
                    assert abs(n[1]-(p[1]-2.25)/.75) < 1e-6
            for triangle in mesh.triangles:
                a,b,c = (mesh.points[i] for i in triangle)
                u,v = tuple(b[k]-a[k] for k in range(3)),tuple(c[k]-a[k] for k in range(3))
                cross = (u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
                assert sum(cross[k]*mesh.normals[triangle[0]][k] for k in range(3)) > 0
        raw = (output/'scene.bgrefmesh').read_bytes()
        header = struct.unpack_from('<8sII6Q',raw)
        assert header[:5] == (b'BGREFM01',1,0,2,3)
        for i,occurrence in enumerate(assembly.occurrences):
            row = struct.unpack_from('<QII15d',raw,64+40*2+136*i)
            assert row[0] == occurrence.id and row[3:15] == occurrence.transform
        for name in ('000-assembled','001-exploded'):
            assert (output/f'{name}.png').read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
            assert 'OCCT REFERENCE' in (output/f'{name}.svg').read_text()
        assert (output/'COMPLETE').is_file()
        duplicate = subprocess.run(command,capture_output=True,text=True,timeout=10)
        assert duplicate.returncode != 0 and 'already exists' in duplicate.stderr
        missing = command.copy()
        missing[missing.index('--step')+1] = str(directory/'missing.step')
        missing[missing.index('--output')+1] = str(directory/'missing-output')
        failed = subprocess.run(missing,capture_output=True,text=True,timeout=10)
        assert failed.returncode != 0 and 'source' in failed.stderr.lower()
        assert not tuple((directory/'missing-output').glob('*.png'))
        assert not (directory/'missing-output/COMPLETE').exists()
        print('OCCT original STEP/reference/display PASS: 2 definitions, 9 faces, 3 placements')


if __name__ == '__main__':
    main()
