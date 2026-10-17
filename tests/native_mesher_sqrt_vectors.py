"""Independent Python integer-isqrt/Fraction oracle for dyadic root enclosures."""
from fractions import Fraction as F
from math import isqrt
from pathlib import Path
import random
import struct
import sys


def encode(value):
    value = F(value)
    def limbs(integer):
        result = []
        while integer:
            result.append(integer & 0xFFFFFFFF)
            integer >>= 32
        return result
    n, d = limbs(value.numerator), limbs(value.denominator)
    return (struct.pack("<iII", int(value != 0), len(n), len(d))
            + b"".join(struct.pack("<I", x) for x in n + d))


def main():
    cases = [(F(0), 0), (F(0), 2048), (F(1), 0), (F(1, 4), 1),
             (F(1, 4), 0), (F(2), 0), (F(2), 3), (F(169, 144), 96)]
    for bits in [0, 1, 64, 127, 2048]:
        cases += [(F(3 << 4096, 5), bits), (F(1, 3 << 4096), bits)]
    rng = random.Random(0xB6E55071)
    for i in range(96):
        width = [1, 31, 32, 63, 127, 256, 512, 1024, 2048, 4096][i % 10]
        value = F(rng.getrandbits(width), rng.getrandbits(width) | 1)
        bits = [0, 3, 17, 64, 127, 256, 512][i % 7]
        cases.append((value, bits))
    # Exact squares and both one-unit neighbors exercise Newton termination.
    for width in [32, 64, 127, 512, 2048, 4096]:
        root = (1 << width) - 1
        for delta in [-1, 0, 1]:
            cases.append((F(root * root + delta), 0))
    output = bytearray(b"BGMSQRT1" + struct.pack("<I", len(cases)))
    for value, bits in cases:
        scaled = value.numerator << (2 * bits)
        root = isqrt(scaled // value.denominator)
        lo = F(root, 1 << bits)
        hi = lo if root * root * value.denominator == scaled else F(root + 1, 1 << bits)
        output += encode(value) + struct.pack("<I", bits) + encode(lo) + encode(hi)
    Path(sys.argv[1]).write_bytes(output)
    print(f"sqrt_vectors={len(cases)} bytes={len(output)}")


if __name__ == "__main__":
    main()
