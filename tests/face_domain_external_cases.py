"""Owned adapters for pinned external polygon inputs; no foreign algorithms."""
from __future__ import annotations

from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path


FIXTURES = Path(__file__).resolve().parent / 'fixtures' / 'face-domain'
MANIFOLD_CASES = ('SimpleHole', 'Eberly', 'Concave2', 'AlignedHoles')


@dataclass(frozen=True)
class ExternalCase:
    name: str
    loops: tuple
    nodes: tuple
    upstream_triangles: int | None = None
    upstream_epsilon: float | None = None

    @property
    def points(self):
        return dict(self.nodes)


def _case(name, coordinates, expected=None, epsilon=None):
    nodes, loops = [], []
    for loop in coordinates:
        word = []
        for point in loop:
            identifier = len(nodes)
            word.append(identifier)
            nodes.append((identifier, tuple(Fraction(x) for x in point)))
        loops.append(tuple(word))
    return ExternalCase(name, tuple(loops), tuple(nodes), expected, epsilon)


def manifold_cases():
    """Parse original decimal bytes via binary64, as upstream's reader does."""
    tokens = iter((FIXTURES / 'manifold-polygon-corpus.txt').read_text().split())
    selected = {}
    while True:
        name = next(tokens, None)
        if name is None:
            break
        expected, epsilon, count = int(next(tokens)), float(next(tokens)), int(next(tokens))
        loops = []
        for _ in range(count):
            loops.append(tuple((float(next(tokens)), float(next(tokens)))
                               for _ in range(int(next(tokens)))))
        if name in MANIFOLD_CASES:
            if name in selected:
                raise ValueError(f'duplicate original external case {name}')
            selected[name] = _case('manifold/' + name, loops, expected, epsilon)
    if set(selected) != set(MANIFOLD_CASES):
        raise ValueError('missing selected original Manifold case')
    return tuple(selected[name] for name in MANIFOLD_CASES)


def nvidia_case():
    """Complete original planar recipe; circle has declared exact chord schedule."""
    outer, hole, circle = [], [], None
    for line in (FIXTURES / 'nvidia-planar-trim.tsv').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        kind, *values = line.split()
        values = tuple(Fraction(value) for value in values)
        if kind == 'outer':
            outer.append(values)
        elif kind == 'hole':
            hole.append(values)
        elif kind == 'circle' and circle is None:
            circle = values
        else:
            raise ValueError('invalid external NVIDIA recipe row')
    if len(outer) != 6 or len(hole) != 3 or circle is None or len(circle) != 3:
        raise ValueError('incomplete original NVIDIA planar recipe')
    cx, cy, radius = circle
    sampled = []
    for quadrant in range(4):
        for k in range(8):
            t = Fraction(k, 8)
            x, y = (1 - t*t) / (1 + t*t), -2*t / (1 + t*t)
            for _ in range(quadrant):
                x, y = y, -x
            sampled.append((cx + radius*x, cy + radius*y))
    return _case('nvidia/TestSmTrimSurfaceWith3dCurves/circle32',
                 (tuple(outer), tuple(hole), tuple(sampled)))


def external_cases():
    return (nvidia_case(), *manifold_cases())
