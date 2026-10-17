"""Author an all-section source input using the independent existing writer.

The native test checks literal field answers and incidence, not writer roundtrip.
This generator is research/test code and is never in the meshing operation.
"""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from brepgpu.imported_source import (  # noqa: E402
    ImportedSource, Product, SolidDefinition, Usage, Vertex, Edge, Coedge,
    Loop, Face, Carrier, Curve, SurfaceSpan, CurveSpan, Style, Diagnostic,
    ZERO_FRAME, IDENTITY, write_source,
)


def main():
    xyz = ((0., 0., 0.), (1., 0., 0.), (1., 1., 0.), (0., 1., 0.))
    vertices = tuple(Vertex(300+i, p, .0001*(i+1)) for i, p in enumerate(xyz))
    edges, uses, curves = [], [], []
    for i in range(4):
        a, b = xyz[i], xyz[(i+1) % 4]
        controls = (*a, 1., *b, 1.)
        curves.append(Curve(600+i, 3, 5, ZERO_FRAME, (0., 0.), (0., 1.),
                            1, (0., 1.), (2, 2), controls,
                            (CurveSpan((0., 1.), 1, controls),)))
        uv = (*a[:2], 1., *b[:2], 1.)
        curves.append(Curve(700+i, 2, 4, ZERO_FRAME, (0., 0.), (0., 1.),
                            1, controls=uv))
        edges.append(Edge(400+i, (300+i, 300+(i+1) % 4), 600+i,
                          (0., 1.), False, False, False, .0002))
        uses.append(Coedge(500+i, 400+i, 800, 1, 700+i, (0., 1.),
                           (i, -i), i % 2 == 0, 2 if i < 3 else 3,
                           ((0., 0.), (1., 1.))))
    net = (-0., 0., 0., 1., 1., 0., 0., 1., 0., 1., 0., 1., 1., 1., 0., 1.)
    carrier = Carrier(1000, 6, ZERO_FRAME, (0., 0.), (-2., 3., -4., 5.),
                      (1, 1), (0., 1.), (2, 2), (0., 1.), (2, 2), (2, 2),
                      net, (SurfaceSpan((0., 1., 0., 1.), (1, 1), net),))
    transform = list(IDENTITY)
    transform[3], transform[7], transform[11] = 13., -7., .5
    source = ImportedSource(
        "ab"*32, 25.4, .0007, 100, "literal all-section λ fixture",
        (Product(100, "root", "assembly"), Product(101, "sheet", "drawing", 200, 1100)),
        (SolidDefinition(200, 101, (900,), tuple(e.id for e in edges),
                         tuple(v.id for v in vertices)),),
        (Usage(201, 100, 101, "placed sheet", tuple(transform), 1100),),
        vertices, tuple(edges), tuple(uses), (Loop(800, 900, True, (500,501,502,503)),),
        (Face(900, 200, 1000, -1, (800,), .0003, 1100),),
        (carrier,), tuple(curves),
        (Style(1100, (42, 43), (.125, .25, .5, 1.), True,
               "brass λ", "g/mm³", .0085, True),),
        (Diagnostic("author λ", 1, (42,), "retained warning"),),
    )
    write_source(source, sys.argv[1])


if __name__ == "__main__":
    main()
