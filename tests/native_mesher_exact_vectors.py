"""Independent original Fraction/integer/IEEE vectors for the owned native unit.

Run only on the fleet, passing the destination vector file. No native machinery
or geometry implementation is imported by this producer.
"""
from fractions import Fraction as F
import math
from pathlib import Path
import random
import struct
import sys


def u32(value):
    return struct.pack("<I", value)


def limbs(value):
    result = []
    while value:
        result.append(value & 0xFFFFFFFF)
        value >>= 32
    return result


def rational(value):
    value = F(value)
    n, d = limbs(abs(value.numerator)), limbs(value.denominator)
    return (struct.pack("<iII", (value > 0) - (value < 0), len(n), len(d))
            + b"".join(u32(x) for x in n + d))


def double_word(value):
    return struct.pack("<d", value)


def conversion(value):
    try:
        nearest = float(value)
    except OverflowError:
        return u32(1) + bytes(8) + u32(1) + bytes(16)
    if not math.isfinite(nearest):
        return u32(1) + bytes(8) + u32(1) + bytes(16)
    exact = F.from_float(nearest)
    lo = math.nextafter(nearest, -math.inf) if exact > value else nearest
    hi = math.nextafter(nearest, math.inf) if exact < value else nearest
    overflow = not (math.isfinite(lo) and math.isfinite(hi))
    return (u32(0) + double_word(nearest) + u32(int(overflow))
            + (bytes(16) if overflow else double_word(lo) + double_word(hi)))


def finite_binary64_words():
    fractions = (0, 1, 2, 3, 1 << 51, (1 << 52)-1, 0x5555555555555, 0xAAAAAAAAAAAAA)
    words = {sign | (exponent << 52) | fraction
             for sign in (0,1 << 63) for exponent in range(2047) for fraction in fractions}
    for sign in (0,1 << 63):
        for exponent in (0,1,1022,1023,1074,1075,2046):
            for bit in range(52):
                words.add(sign | (exponent << 52) | (1 << bit))
                words.add(sign | (exponent << 52) | ((1 << 52)-(1 << bit)))
    return tuple(sorted(words))


def main():
    rng = random.Random(0xB6E0AC71)
    special = [F(0), F(1), F(-1), F(3, 2), F(-7, 15), F(-(1 << 63)),
               F((1 << 53) + 1), F((1 << 53) + 3),
               F(1, 1 << 1075), F(3, 1 << 1075), F(5, 1 << 1075),
               F(1, 1 << 2000), F((1 << 52) - 1, 1 << 1074),
               F((1 << 53) - 1, 1 << 1075), F(1, 1 << 1022),
               F((1 << 53) - 1) * (1 << 971),
               F((1 << 54) - 1) * (1 << 970),
               F(1 << 1024), F(1 << 1500)]
    special += [-x for x in special if x]
    # Midpoints immediately beside each IEEE regime and both ties-even parities.
    bits = [1, 2, 3, (1 << 52) - 1, 1 << 52, (1023 << 52),
            (1023 << 52) + 1, (2046 << 52) + (1 << 52) - 2]
    for bit in bits:
        a = F.from_float(struct.unpack("<d", struct.pack("<Q", bit))[0])
        b = F.from_float(struct.unpack("<d", struct.pack("<Q", bit + 1))[0])
        midpoint = (a + b) / 2
        delta = (b - a) / 16
        special += [midpoint - delta, midpoint, midpoint + delta,
                    -midpoint - delta, -midpoint, -midpoint + delta]
    pairs = [(value, F((i % 9) - 4, (i % 7) + 1)) for i, value in enumerate(special)]
    for i in range(192):
        width = [1, 31, 32, 33, 63, 64, 65, 128, 257, 512, 1024, 2048][i % 12]
        a = F(rng.getrandbits(width) * (-1 if i & 1 else 1), rng.getrandbits(width) | 1)
        b = F(rng.getrandbits(width) * (-1 if i & 2 else 1), rng.getrandbits(width) | 1)
        if i % 7 == 0:
            b = -a
        pairs.append((a, b))
    integers = [(0, 1), (1, 1), (1, 2), ((1 << 64) - 1, (1 << 32) - 1),
                ((1 << 96) - 1, (1 << 64) + 1),
                ((1 << 160) - (1 << 96), (1 << 96) - 1)]
    radix = 1 << 32
    # Lower divisor limbs force trial correction and D6 addback respectively.
    for d in (radix**2 // 2 + radix - 1, radix**3 // 2 + 1):
        integers.append((d * radix - 1, d))
    # Dense/sparse wide divisors exercise normalization and trial-quotient correction.
    for i in range(160):
        width = [33, 64, 96, 127, 256, 512, 1024, 2048][i % 8]
        n = rng.getrandbits(width * 2)
        d = rng.getrandbits(width) | 1
        if i % 5 == 0:
            n *= d
        integers.append((n, d))
    x, y = 0, 1
    for i in range(12000):
        x, y = y, x + y
        if i in (92, 511, 2047, 8191, 11999):
            integers.append((y, x))
    double_inputs = finite_binary64_words()
    payload = bytearray(b"BGEXACT2" + u32(len(pairs)) + u32(len(integers)) + u32(len(double_inputs)))
    for a, b in pairs:
        for value in (a, b, a + b, a - b, a * b, a / b if b else 0):
            payload += rational(value)
        payload += struct.pack("<i", (a > b) - (a < b)) + conversion(a)
        raw = rng.getrandbits(64)
        if (raw >> 52) & 0x7FF == 0x7FF:
            raw ^= 1 << 52
        decoded = struct.unpack("<d", struct.pack("<Q", raw))[0]
        payload += struct.pack("<Q", raw) + rational(F.from_float(decoded))
    for n, d in integers:
        for value in (n, d, n // d, n % d, math.gcd(n, d)):
            payload += rational(value)
    for raw in double_inputs:
        decoded = struct.unpack("<d", struct.pack("<Q", raw))[0]
        numerator,denominator = decoded.as_integer_ratio()
        payload += struct.pack("<Q",raw) + rational(F(numerator,denominator))
    Path(sys.argv[1]).write_bytes(payload)
    if len(sys.argv) == 3:
        Path(sys.argv[2]).write_bytes(b"BGDBL001"+struct.pack("<Q",len(double_inputs))+
                                      b"".join(struct.pack("<Q",raw) for raw in double_inputs))
    print(f"rational_vectors={len(pairs)} integer_vectors={len(integers)} double_vectors={len(double_inputs)} bytes={len(payload)}")


if __name__ == "__main__":
    main()
