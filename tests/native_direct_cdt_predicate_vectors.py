"""Write bounded native inputs with independent original-rational answers."""
from fractions import Fraction as F
import math
from pathlib import Path
import random
import struct
import sys

from direct_cdt_oracle import _determinant


def write_vectors(path):
    rng = random.Random(0xCD72026)
    cases = []
    for _ in range(160):
        points = tuple((F(rng.randrange(-10000, 10000), rng.choice((3, 7, 11, 17))),
                        F(rng.randrange(-10000, 10000), rng.choice((5, 13, 19))))
                       for _ in range(4))
        metric = (2.**rng.randrange(-20, 21), 2.**rng.randrange(-20, 21))
        cases.append((points, metric))
    for exponent in (1, 53, 180, 1000, 2200):
        epsilon = F(1, 2**exponent)
        for direction in (-1, 0, 1):
            points = ((F(0), F(0)), (F(1), F(0)), (F(1), F(1)),
                      (F(0), F(1)+direction*epsilon))
            cases.append((points, (1., 1.)))
            offset = (F(2**(exponent+300), 3), -F(2**(exponent+200), 7))
            translated = tuple((x+offset[0], y+offset[1]) for x, y in points)
            cases.append((translated, (2.**-500, 2.**500)))
        cases.append((((F(0), F(0)), (F(1), epsilon),
                       (F(2), F(0)), (F(2**(exponent+100)), F(3))), (1., 1.)))
    output = bytearray(b'BGCDTP01' + struct.pack('<I', len(cases)))

    def integer(value):
        magnitude = abs(value)
        limbs = []
        while magnitude:
            limbs.append(magnitude & 0xffffffff)
            magnitude >>= 32
        output.extend(struct.pack('<iI', (value > 0)-(value < 0), len(limbs)))
        output.extend(struct.pack('<' + 'I'*len(limbs), *limbs))

    for points, metric in cases:
        scales = tuple(map(F, metric))
        matrix = []
        for (u, v) in points:
            x, y = scales[0]*u, scales[1]*v
            matrix.append((x, y, x*x+y*y, F(1)))
        circle = _determinant(matrix)
        a, b, c, _ = points
        orientation = (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
        denominators = tuple(math.lcm(*(p[k].denominator for p in points)) for k in (0, 1))
        weights = tuple(scales[k]**2 / denominators[k]**2 for k in (0, 1))
        common = math.lcm(*(weight.denominator for weight in weights))
        for weight in weights:
            integer(int(weight*common))
        for point in points:
            for k in (0, 1):
                integer(int(point[k]*denominators[k]))
        output.extend(struct.pack('<ii', (orientation > 0)-(orientation < 0),
                                  (circle > 0)-(circle < 0)))
    Path(path).write_bytes(output)
    print(f'{len(cases)} independent rational predicate vectors written')


if __name__ == '__main__':
    write_vectors(sys.argv[1])
