"""Independent source writer input for the native original-geometry consumer."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from brepgpu.imported_source import (  # noqa: E402
    ImportedSource, Product, SolidDefinition, Usage, Vertex, Edge, Coedge,
    Loop, Face, Carrier, Curve, SurfaceSpan, ZERO_FRAME, IDENTITY, write_source,
)


def main():
    curves = [
        Curve(7, 2, 5, ZERO_FRAME, (0., 0.), (0., 1.), 2,
              (0., .5, 1.), (3, 2, 3),
              (0., 0., 1., 1., 0., 1., 1., 1., 1., 2., 1., 1., 2., 2., 1.)),
        Curve(11, 3, 1, (0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0., 1.),
              (0., 0.), (-2., 5.)),
    ]
    xyz = ((0., 0., 0.), (1., 0., 0.), (1., 1., 1.), (0., 1., 0.))
    uv = ((0., 0.), (1., 0.), (1., 1.), (0., 1.))
    boundary = (
        (0., 0., 0., 1., 1., 0., 0., 1.),
        (1., 0., 0., 1., 2., 2., 2., 2.),
        (2., 2., 2., 2., 0., 2., 0., 2.),
        (0., 2., 0., 2., 0., 0., 0., 1.),
    )
    edges, uses = [], []
    for i in range(4):
        curves.append(Curve(2000+i, 3, 4, ZERO_FRAME, (0., 0.), (0., 1.),
                            1, controls=boundary[i]))
        curves.append(Curve(3000+i, 2, 4, ZERO_FRAME, (0., 0.), (0., 1.),
                            1, controls=(*uv[i], 1., *uv[(i+1) % 4], 1.)))
        edges.append(Edge(400+i, (300+i, 300+(i+1) % 4), 2000+i, (0., 1.),
                          True, True, False, 0.))
        uses.append(Coedge(500+i, 400+i, 800, 1, 3000+i, (0., 1.), (0, 0)))
    net = (0., 0., 0., 1., 0., 2., 0., 2., 1., 0., 0., 1., 2., 2., 2., 2.)
    wrong_hint = (100., 100., 100., 1.) * 4
    carrier = Carrier(9, 6, ZERO_FRAME, (0., 0.), (0., .125, 0., .125),
                      (1, 1), (0., 1.), (2, 2), (0., 1.), (2, 2), (2, 2), net,
                      (SurfaceSpan((0., 1., 0., 1.), (1, 1), wrong_hint),))
    grid = tuple(value for i in range(3) for j in range(3)
                 for value in (i/2., j/2., i*j/4., 1.))
    grid_spans = tuple(SurfaceSpan((i/2., (i+1)/2., j/2., (j+1)/2.), (1, 1),
                       tuple(value for u in (i/2., (i+1)/2.)
                             for v in (j/2., (j+1)/2.) for value in (u,v,u*v,1.)))
                       for i in range(2) for j in range(2))
    multispan = Carrier(12, 6, ZERO_FRAME, (0., 0.), (0., 1., 0., 1.),
                        (1, 1), (0., .5, 1.), (2, 1, 2),
                        (0., .5, 1.), (2, 1, 2), (3, 3), grid, grid_spans)
    source = ImportedSource(
        "ef"*32, 1., 0., 100, "original-geometry literal fixture",
        (Product(100, "assembly", "root"), Product(101, "rational sheet", "part", 200)),
        (SolidDefinition(200, 101, (900,), tuple(e.id for e in edges), (300,301,302,303)),),
        (Usage(201, 100, 101, "sheet", IDENTITY),),
        tuple(Vertex(300+i, point, 0.) for i, point in enumerate(xyz)),
        tuple(edges), tuple(uses), (Loop(800, 900, True, (500,501,502,503)),),
        (Face(900, 200, 9, 1, (800,), 0.),), (carrier, multispan), tuple(curves),
    )
    write_source(source, sys.argv[1])


if __name__ == "__main__":
    main()
