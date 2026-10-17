"""One retained source bank, bounded face owners and immutable old host results."""
from dataclasses import replace
from fractions import Fraction as F
from pathlib import Path
import argparse
import math
import subprocess
import tempfile

from native_adaptive_curves_caller import source as original_plane, read as read_curves
from native_adaptive_trimmed_caller import read_face, qualify_face
from brepgpu.imported_source import (Vertex, Edge, Coedge, Loop, Face, Curve, Carrier,
                                     write_source)


def source():
    base = original_plane('plain')
    vertices, edges, uses, loops, faces, curves, carriers = ([] for _ in range(7))
    for i in range(4):
        offset, shift = 10000 * i, float(3 * i)
        vertices.extend(replace(v, id=v.id+offset, xyz=(v.xyz[0]+shift, *v.xyz[1:]))
                        for v in base.vertices)
        for curve in base.curves:
            controls = list(curve.controls)
            for at in range(0, len(controls), curve.dimension+1):
                controls[at] += shift * controls[at+curve.dimension]
            curves.append(replace(curve, id=curve.id+offset, controls=tuple(controls)))
        edges.extend(replace(e, id=e.id+offset,
                             vertex_ids=tuple(v+offset for v in e.vertex_ids),
                             curve_id=e.curve_id+offset) for e in base.edges)
        uses.extend(replace(u, id=u.id+offset, edge_id=u.edge_id+offset,
                            loop_id=u.loop_id+offset, curve_id=u.curve_id+offset)
                    for u in base.coedges)
        loops.extend(replace(loop, id=loop.id+offset, face_id=900+i,
                             coedge_ids=tuple(u+offset for u in loop.coedge_ids))
                     for loop in base.loops)
        faces.append(replace(base.faces[0], id=900+i, carrier_id=9+i,
                             loop_ids=tuple(loop.id+offset for loop in base.loops)))
        carriers.append(replace(base.carriers[0], id=9+i,
                                uv_bounds=(shift, shift+1., 0., 1.)))
    # A valid original closed circle accepted at loose positional quality has a
    # one-chord selected graph. The real domain producer must request refinement.
    unit = base.carriers[0].frame
    vertices.append(Vertex(99000, (1., 0., 0.), 0.))
    curves.extend((Curve(99001, 3, 2, unit, (1., 1.), (0., math.tau)),
                   Curve(99002, 2, 2, unit, (1., 1.), (0., math.tau))))
    edges.append(Edge(99003, (99000, 99000), 99001, (0., math.tau), True, True, False, 0.))
    uses.append(Coedge(99004, 99003, 99005, 1, 99002, (0., math.tau), reconstructed=True))
    loops.append(Loop(99005, 904, True, (99004,)))
    faces.append(Face(904, 200, 13, 1, (99005,), 0.))
    carriers.append(Carrier(13, 1, unit, (0., 0.), (-2., 2., -2., 2.)))
    definition = replace(base.definitions[0], face_ids=tuple(f.id for f in faces),
                         edge_ids=tuple(e.id for e in edges), vertex_ids=tuple(v.id for v in vertices))
    return replace(base, definitions=(definition,), vertices=tuple(vertices), edges=tuple(edges),
                   coedges=tuple(uses), loops=tuple(loops), faces=tuple(faces),
                   carriers=tuple(carriers), curves=tuple(curves))


def run(options, root):
    src = source()
    path = root / 'original.bgsource'
    write_source(src, path)
    output = root / 'output'
    output.mkdir()
    command = [str(options.executable.resolve()), str(path), str(output), str(options.threads)]
    if options.module:
        command.extend((str(options.module.resolve()), str(options.device)))
    result = subprocess.run(command, text=True, capture_output=True, timeout=60)
    (root / 'stdout.txt').write_text(result.stdout)
    (root / 'stderr.txt').write_text(result.stderr)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    for query in (1, 2):
        for i, face in enumerate(src.faces[:4]):
            chosen = replace(src, faces=(face, *[f for f in src.faces if f != face]))
            stem = f'{query}-{i}'
            mesh = read_face(output / (stem+'.bgatrm'))
            carrier = next(c for c in src.carriers if c.id == face.carrier_id)
            assert len(mesh['roots']) == 1 and (
                mesh['roots'][0]['carrier'], tuple(map(F, mesh['roots'][0]['box']))) == (
                    carrier.id, tuple(map(F, carrier.uv_bounds))), 'wrong original face-root closure'
            qualify_face(chosen, read_curves(output / (stem+'.bgacrv')), mesh, .01, 1.e-8)
    for suffix in ('bgacrv', 'bgatrm'):
        assert (output / ('1-0.'+suffix)).read_bytes() == (
            output / ('first-after-close.'+suffix)).read_bytes(), 'old immutable host data changed'
    print(result.stdout.strip())
    print('existing original-source/chain reader PASS\t8 real faces')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--threads', type=int, choices=(1, 4), default=1)
    parser.add_argument('--module', type=Path)
    parser.add_argument('--device', type=int, choices=(0, 1), default=0)
    parser.add_argument('--workspace', type=Path)
    options = parser.parse_args()
    if options.workspace:
        options.workspace.mkdir(parents=True, exist_ok=False)
        run(options, options.workspace.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='adaptive-face-assignments-') as temporary:
            run(options, Path(temporary))


if __name__ == '__main__':
    main()
