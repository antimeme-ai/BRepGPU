"""Literal C0 original source and independent Fraction POWER-cell oracle."""
from fractions import Fraction as F
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from brepgpu.imported_source import (  # noqa: E402
    ImportedSource, Product, SolidDefinition, Usage, Vertex, Edge, Coedge,
    Loop, Face, Carrier, Curve, SurfaceSpan, ZERO_FRAME, IDENTITY, write_source,
)


def exact(value):
    value = F(value)
    def words(n):
        result = []
        while n:
            result.append(n & 0xFFFFFFFF)
            n >>= 32
        return result
    a, b = words(abs(value.numerator)), words(value.denominator)
    return (struct.pack("<iII", (value > 0) - (value < 0), len(a), len(b))
            + b"".join(struct.pack("<I", n) for n in a + b))


def affine(triangle, xyz):
    # Independent Gaussian solve for POWER coefficients c0+c1*u+c2*v.
    result = []
    for axis in range(3):
        rows = [[F(1), *point, xyz[i][axis]] for i, point in enumerate(triangle)]
        for column in range(3):
            pivot = next(i for i in range(column, 3) if rows[i][column])
            rows[column], rows[pivot] = rows[pivot], rows[column]
            divisor = rows[column][column]
            rows[column] = [x / divisor for x in rows[column]]
            for i in range(3):
                if i != column:
                    factor = rows[i][column]
                    rows[i] = [a - factor*b for a, b in zip(rows[i], rows[column])]
        result.append(tuple(rows[i][3] for i in range(3)))
    return result


def evaluate(coefficients, point):
    u, v = point
    return tuple(a + b*u + c*v for a, b, c in coefficients)


def main():
    uv = ((0., 0.), (1., 0.), (1., 1.), (0., 1.))
    xyz = ((0., 0., 0.), (1., 0., 1.), (1., 1., 1.), (0., 1., 0.))
    curves, edges, uses = [], [], []
    bottom = (0., 0., 0., 1., .5, 0., 0., 1., 1., 0., 1., 1.)
    top = (1., 1., 1., 1., .5, 1., 0., 1., 0., 1., 0., 1.)
    for i in range(4):
        controls = bottom if i == 0 else top if i == 2 else (*xyz[i], 1., *xyz[(i+1) % 4], 1.)
        knots = (0., .5, 1.) if i in (0, 2) else (0., 1.)
        mult = (2, 1, 2) if i in (0, 2) else (2, 2)
        curves.append(Curve(2000+i, 3, 5, ZERO_FRAME, (0., 0.), (0., 1.), 1,
                            knots, mult, controls))
        curves.append(Curve(3000+i, 2, 4, ZERO_FRAME, (0., 0.), (0., 1.), 1,
                            controls=(*uv[i], 1., *uv[(i+1) % 4], 1.)))
        edges.append(Edge(400+i, (300+i, 300+(i+1) % 4), 2000+i, (0., 1.),
                          True, True, False, 0.))
        uses.append(Coedge(500+i, 400+i, 800, 1, 3000+i, (0., 1.), (0, 0)))
    # S=(u,v,max(0,2u-1)), exact C0 crease, independent of extracted hints.
    net = tuple(x for u, z in ((0., 0.), (.5, 0.), (1., 1.))
                for v in (0., 1.) for x in (u, v, z, 1.))
    carrier = Carrier(9, 6, ZERO_FRAME, (0., 0.), (0., 1., 0., 1.),
                      (1, 1), (0., .5, 1.), (2, 1, 2),
                      (0., 1.), (2, 2), (3, 2), net,
                      tuple(SurfaceSpan((a,b,0.,1.), (1,1),
                            tuple(x for u in (a,b) for v in (0.,1.)
                                  for x in (u,v,max(0.,2*u-1.),1.)))
                            for a,b in ((0.,.5),(.5,1.))))
    plane = Carrier(10, 1, (2.,-1.,.5, 2.,0.,0., 1.,3.,0., 0.,0.,1.),
                    (0.,0.), (0.,1.,0.,1.))
    cylinder = Carrier(11, 2, (0.,0.,0., 1.,0.,0., 0.,1.,0., 0.,0.,1.),
                       (1.,0.), (0.,1.,0.,1.))
    grid_n = 16
    axis = tuple(i/grid_n for i in range(grid_n+1))
    grid = Carrier(12, 6, ZERO_FRAME, (0.,0.), (0.,1.,0.,1.), (1,1),
                   axis, (2,)+(1,)*(grid_n-1)+(2,),
                   axis, (2,)+(1,)*(grid_n-1)+(2,), (grid_n+1,grid_n+1),
                   tuple(x for u in axis for v in axis for x in (u,v,0.,1.)),
                   tuple(SurfaceSpan((a,b,c,d),(1,1),
                         tuple(x for u in (a,b) for v in (c,d) for x in (u,v,0.,1.)))
                         for a,b in zip(axis,axis[1:]) for c,d in zip(axis,axis[1:])))
    source = ImportedSource("cd"*32, 1., 0., 100, "literal C0 actual-cell fixture",
        (Product(100, "assembly", "root"), Product(101, "creased sheet", "part", 200)),
        (SolidDefinition(200, 101, (900,), tuple(e.id for e in edges), (300,301,302,303)),),
        (Usage(201, 100, 101, "sheet", IDENTITY),),
        tuple(Vertex(300+i, p, 0.) for i, p in enumerate(xyz)), tuple(edges), tuple(uses),
        (Loop(800, 900, True, (500,501,502,503)),),
        (Face(900, 200, 9, 1, (800,), 0.),), (carrier,plane,cylinder,grid), tuple(curves))
    write_source(source, sys.argv[1])
    a, b, c = (F(0), F(0)), (F(1), F(0)), (F(0), F(1))
    d, e = (F(1, 2), F(0)), (F(1, 2), F(1, 2))
    partition = [(0, (a, d, e)), (0, (a, e, c)), (1, (d, b, e))]
    true_xyz = ((F(0), F(0), F(0)), (F(1), F(0), F(1)), (F(0), F(1), F(0)))
    offset = tuple((x, y, z+F(1, 8)) for x, y, z in true_xyz)
    cases = [((a,b,c), true_xyz, F(1,2), partition),
             ((a,b,c), true_xyz, F(1,4), partition),
             ((a,b,c), offset, F(1,2), partition),
             ((a,b,c), offset, F(5,8), partition)]
    # Exact crease touch: one positive root and one zero-area root candidate.
    touch = (d, b, (F(1,2), F(1)))
    cases.append((touch, tuple((u,v,2*u-1) for u,v in touch), F(0), [(1,touch)]))
    left = (a,e,c)
    cases.append((left, tuple((u,v,F(0)) for u,v in left), F(0), [(0,left)]))
    # Nondyadic virtual intersections 7/40 and9/16, literal geometry not clipped
    # by the implementation's Sutherland-Hodgman algorithm.
    aa, bb, cc = (F(1,4),F(1,8)), (F(7,8),F(1,4)), (F(1,8),F(7,8))
    dd, ee = (F(1,2),F(7,40)), (F(1,2),F(9,16))
    parts = [(0,(cc,aa,dd)), (0,(cc,dd,ee)), (1,(dd,bb,ee))]
    nodes = tuple((u,v,F(0)) for u,v in (aa,bb,cc))
    cases += [((aa,bb,cc), nodes, F(3,4), parts), ((aa,bb,cc), nodes, F(1,2), parts)]
    output = bytearray(b"BGMCELL1" + struct.pack("<I", len(cases)))
    for triangle, nodes, tolerance, pieces in cases:
        output += b"".join(exact(x) for p in triangle for x in p)
        output += b"".join(exact(x) for p in nodes for x in p) + exact(tolerance)
        output += struct.pack("<I", len(pieces))
        coefficients = affine(triangle, nodes)
        decisions = []
        for root, corners in pieces:
            linear = [evaluate(coefficients, p) for p in corners]
            squared = []
            for p, q in zip(corners, linear):
                u,v = p
                original = (u,v,F(0) if root == 0 else 2*u-1)
                squared.append(sum((x-y)**2 for x,y in zip(original,q)))
            # On EACH literal root, original S-L is affine POWER polynomial.
            # Squared norm is convex, so vertex maximum is its exact maximum.
            # This derives the decision without the production Bernstein net.
            decision = int(max(squared) > tolerance*tolerance)
            decisions.append(decision)
            output += struct.pack("<II", root, decision)
            output += b"".join(exact(x) for p in corners for x in p)
            output += b"".join(exact(x) for q in linear for x in q)
        output += struct.pack("<I", int(any(decisions)))
    Path(sys.argv[2]).write_bytes(output)
    print(f"actual_cell_cases={len(cases)} oracle_bytes={len(output)}")


if __name__ == "__main__":
    main()
