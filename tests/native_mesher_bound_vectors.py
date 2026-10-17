"""Independent actual-cell bound oracle using Fraction POWER polynomials.

No geometry or native code imports. Tensor Bernstein source expressions are
expanded into power polynomials, substituted at the original subcell, and only
then converted to simplex Bernstein coefficients. Production uses Bernstein
restriction/convolution instead. Run on the fleet with an output path.
"""
from fractions import Fraction as F
from math import comb, isqrt
from pathlib import Path
import random
import struct
import sys


def add(a, b):
    result = dict(a)
    for key, value in b.items():
        result[key] = result.get(key, F(0)) + value
    return {k: v for k, v in result.items() if v}


def scale(a, factor):
    return {k: v * factor for k, v in a.items() if v * factor}


def mul(a, b):
    result = {}
    for (i, j), x in a.items():
        for (k, l), y in b.items():
            key = (i + k, j + l)
            result[key] = result.get(key, F(0)) + x * y
    return {k: v for k, v in result.items() if v}


def power(a, degree):
    result = {(0, 0): F(1)}
    for _ in range(degree):
        result = mul(result, a)
    return result


def basis(degree, i, argument):
    complement = add({(0, 0): F(1)}, scale(argument, -1))
    return scale(mul(power(argument, i), power(complement, degree - i)), comb(degree, i))


def simplex_coeff(poly, degree, i, j):
    # x^a*y^b has coefficient (i)_a*(j)_b/(degree)_(a+b).
    return sum((v * F(comb(i, a) * comb(j, b),
                      comb(degree, a + b) * comb(a + b, a))
                for (a, b), v in poly.items() if a <= i and b <= j), F(0))


def criterion(p, q, bounds, controls, uv, xyz, curve=False):
    if curve:
        begin, end = bounds[:2]
        t = {(0, 0): (uv[0] - begin) / (end - begin),
             (1, 0): (uv[1] - uv[0]) / (end - begin)}
        polys = [{} for _ in range(4)]
        for i in range(p + 1):
            b = basis(p, i, t)
            for axis in range(4):
                polys[axis] = add(polys[axis], scale(b, controls[4 * i + axis]))
        linear = [{(0, 0): xyz[axis], (1, 0): xyz[3 + axis] - xyz[axis]}
                  for axis in range(3)]
        degree = p + 1
        indices = [(i, 0) for i in range(degree + 1)]
        weight_indices = [(i, 0) for i in range(p + 1)]
    else:
        u0, u1, v0, v1 = bounds
        s = {(0, 0): (uv[0] - u0) / (u1 - u0),
             (1, 0): (uv[2] - uv[0]) / (u1 - u0),
             (0, 1): (uv[4] - uv[0]) / (u1 - u0)}
        t = {(0, 0): (uv[1] - v0) / (v1 - v0),
             (1, 0): (uv[3] - uv[1]) / (v1 - v0),
             (0, 1): (uv[5] - uv[1]) / (v1 - v0)}
        polys = [{} for _ in range(4)]
        for i in range(p + 1):
            for j in range(q + 1):
                b = mul(basis(p, i, s), basis(q, j, t))
                for axis in range(4):
                    polys[axis] = add(polys[axis], scale(b, controls[4 * (i * (q + 1) + j) + axis]))
        linear = [{(0, 0): xyz[axis], (1, 0): xyz[3 + axis] - xyz[axis],
                   (0, 1): xyz[6 + axis] - xyz[axis]} for axis in range(3)]
        degree = p + q + 1
        indices = [(i, j) for i in range(degree + 1) for j in range(degree - i + 1)]
        weight_indices = [(i, j) for i in range(degree) for j in range(degree - i)]
    residual = [add(polys[a], scale(mul(polys[3], linear[a]), -1)) for a in range(3)]
    wstar = min(simplex_coeff(polys[3], degree - 1, i, j) for i, j in weight_indices)
    assert wstar > 0
    return max(sum(simplex_coeff(r, degree, i, j)**2 for r in residual)
               for i, j in indices) / wstar**2


def answer(p, q, bounds, controls, uv, xyz, tolerance, curve=False):
    return criterion(p, q, bounds, controls, uv, xyz, curve) <= tolerance**2


def near_threshold(value):
    scale = 1 << 80
    scaled = value.numerator << 160
    root = isqrt(scaled // value.denominator)
    equality = root * root * value.denominator == scaled
    return F(root - int(equality), scale), F(root + int(not equality), scale)


def words(value):
    result = []
    while value:
        result.append(value & 0xFFFFFFFF)
        value >>= 32
    return result


def encode(value):
    value = F(value)
    n, d = words(abs(value.numerator)), words(value.denominator)
    return (struct.pack("<iII", (value > 0) - (value < 0), len(n), len(d))
            + b"".join(struct.pack("<I", x) for x in n + d))


def main():
    cases = []
    controls = [F(x) for x in [0, 0, 0, 1, 0, 1, 0, 1,
                                1, 0, 0, 1, 1, 1, 1, 1]]
    uv = list(map(F, [0, 0, 1, 0, 0, 1]))
    xyz = list(map(F, [0, 0, 0, 1, 0, 0, 0, 1, 0]))
    for tau in [F(1, 3), F((1 << 80) - 1, 3 << 80), F(1, 2), F(0)]:
        cases.append((0, 1, 1, list(map(F, [0, 1, 0, 1])), controls, uv, xyz, tau))
    tiny = F(1, 1 << 600)
    cases.append((0, 1, 1, list(map(F, [0, 1, 0, 1])), controls,
                  [F(0), F(0), tiny, F(0), F(0), tiny],
                  [F(0), F(0), F(0), tiny, F(0), F(0), F(0), tiny, F(0)],
                  F(1, 3 << 1200)))
    # Same actual affine plane far from the origin. XYZ values remain exactly
    # binary64 representable; root/task centering must avoid origin-sized widths.
    shift = F(1 << 40)
    plane = [shift, shift, shift, F(1), shift, shift+1, shift, F(1),
             shift+1, shift, shift, F(1), shift+1, shift+1, shift, F(1)]
    cases.append((0,1,1,list(map(F,[0,1,0,1])),plane,uv,
                  [shift,shift,shift,shift+1,shift,shift,shift,shift+1,shift],F(1,1024)))
    rng = random.Random(0xCE11B0A0)
    for p in range(4):
        for q in range(4):
            controls = []
            for _ in range((p + 1) * (q + 1)):
                w = F(rng.randint(1, 8), rng.randint(1, 5))
                controls += [w * F(rng.randint(-9, 9), 8) for _ in range(3)] + [w]
            xyz = [F(rng.randint(-4, 4), 4) for _ in range(9)]
            box = list(map(F, [-2, 3, 1, 5]))
            corners = [F(-1), F(2), F(1), F(5, 2), F(0), F(4)]
            limit = criterion(p, q, box, controls, corners, xyz)
            for tau in [F(1, 64), F(64), *near_threshold(limit)]:
                cases.append((0, p, q, list(map(F, [-2, 3, 1, 5])), controls,
                              corners, xyz, tau))
    curve_controls = list(map(F, [0, 0, 0, 1, F(1, 2), 0, 0, 1, 1, 1, 0, 1]))
    for tau in [F(1, 3), F((1 << 80) - 1, 3 << 80), F(1)]:
        cases.append((1, 2, 0, list(map(F, [0, 1, 0, 1])), curve_controls,
                      list(map(F, [0, 1, 0, 0, 0, 0])), list(map(F, [0, 0, 0, 1, 1, 0, 0, 0, 0])), tau))
    radius = F(1, 256)
    for tau in [2 * radius / 3, F(1, 1000), F(1, 200)]:
        cases.append((1, 2, 0, list(map(F, [0, 1, 0, 1])),
                      [radius, F(0), F(0), F(1), radius, radius, F(0), F(1),
                       F(0), 2 * radius, F(0), F(2)],
                      list(map(F, [0, 1, 0, 0, 0, 0])),
                      [radius, F(0), F(0), F(0), radius, F(0), F(0), F(0), F(0)], tau))
    for degree in [0, 1, 3, 7, 16, 32]:
        controls = []
        for i in range(degree + 1):
            w = F(1 + i % 3)
            controls += [w * F(i, max(1, degree)), w * F((i % 4) - 1, 8), F(0), w]
        for tau in [F(1, 256), F(16)]:
            cases.append((1, degree, 0, list(map(F, [-3, 2, 0, 1])), controls,
                          list(map(F, [-2, 1, 0, 0, 0, 0])),
                          list(map(F, [0, 0, 0, 1, 0, 0, 0, 0, 0])), tau))
    output = bytearray(b"BGBNDP01" + struct.pack("<I", len(cases)))
    accepted = 0
    for kind, p, q, bounds, controls, uv, xyz, tau in cases:
        expected = answer(p, q, bounds, controls, uv, xyz, tau, bool(kind))
        accepted += expected
        output += struct.pack("<IIII", kind, p, q, len(controls))
        for value in bounds + controls + uv + xyz + [tau]:
            output += encode(value)
        output += struct.pack("<I", int(expected))
    Path(sys.argv[1]).write_bytes(output)
    print(f"bound_vectors={len(cases)} accept={accepted} fail={len(cases)-accepted} bytes={len(output)}")


if __name__ == "__main__":
    main()
