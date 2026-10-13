"""Owned analytic fixtures and exact expectations for binary64 patch inputs.

This is a qualification helper, not the production geometry evaluator. Exact
arithmetic uses supplied float ratios and a general differentiated product
identity, independently of the native floating-point execution strategies.
"""

from dataclasses import dataclass
from fractions import Fraction
from functools import lru_cache
import math


TERMS = ((0, 0), (1, 0), (0, 1), (2, 0), (1, 1), (0, 2))
TERM_NAMES = ("S", "Ss", "St", "Sss", "Sst", "Stt")
TERM_COUNTS = (1, 3, 6)
ERROR_TARGETS = (Fraction(1, 1 << 40), Fraction(1, 1 << 30), Fraction(1, 1 << 20))


@dataclass(frozen=True)
class Fixture:
    """A named mathematical test input; controls are the supplied float bits."""

    name: str
    degree_u: int
    degree_v: int
    controls: tuple
    analytic_kind: str = ""

    @property
    def control_count(self):
        return (self.degree_u + 1) * (self.degree_v + 1)


def _ratio(value):
    if isinstance(value, Fraction):
        return value
    return Fraction.from_float(float(value))


def _falling(power, order):
    if power < order:
        return 0
    return math.factorial(power) // math.factorial(power - order)


@lru_cache(maxsize=4096)
def _bernstein_derivative(degree, index, value, order):
    """Differentiate the exact expanded Bernstein polynomial, including ends."""
    if order > degree:
        return Fraction(0)
    return math.comb(degree, index) * sum(
        ((-1) ** k) * math.comb(degree - index, k)
        * _falling(index + k, order) * value ** (index + k - order)
        for k in range(degree - index + 1)
        if index + k >= order
    )


@lru_cache(maxsize=4096)
def _exact_full_jet(degree_u, degree_v, controls, s, t):
    coefficients = tuple(_ratio(x) for x in controls)
    homogeneous = {}
    for a, b in TERMS:
        homogeneous[a, b] = tuple(sum(
            coefficients[4 * (i * (degree_v + 1) + j) + axis]
            * _bernstein_derivative(degree_u, i, s, a)
            * _bernstein_derivative(degree_v, j, t, b)
            for i in range(degree_u + 1)
            for j in range(degree_v + 1)
        ) for axis in range(4))
    denominator = homogeneous[0, 0][3]
    if denominator <= 0:
        raise ValueError("exact rational denominator must be positive")
    cartesian = {}
    # General Leibniz identity A = W*S, solved in increasing total order.
    # It does not repeat native pure/mixed second-order formula branches.
    for a, b in TERMS:
        vector = []
        for axis in range(3):
            rest = sum(
                math.comb(a, i) * math.comb(b, j)
                * homogeneous[i, j][3] * cartesian[a - i, b - j][axis]
                for i in range(a + 1)
                for j in range(b + 1)
                if i or j
            )
            vector.append((homogeneous[a, b][axis] - rest) / denominator)
        cartesian[a, b] = tuple(vector)
    return tuple(cartesian[term] for term in TERMS)


def exact_jet(fixture, s, t, order=2):
    """Exact jet for the binary64 controls/parameters actually sent to native."""
    if order not in (0, 1, 2):
        raise ValueError("order must be 0, 1 or 2")
    if len(fixture.controls) != fixture.control_count * 4:
        raise ValueError("fixture control count differs from degree")
    return _exact_full_jet(
        fixture.degree_u, fixture.degree_v, tuple(fixture.controls),
        _ratio(s), _ratio(t),
    )[:TERM_COUNTS[order]]


def _polynomial_jet(components, s, t):
    """Differentiate author polynomial maps directly, without control nets."""
    s, t = _ratio(s), _ratio(t)
    return tuple(tuple(sum(
        _ratio(coefficient) * _falling(i, a) * _falling(j, b)
        * s ** (i - a) * t ** (j - b)
        for i, j, coefficient in terms
        if i >= a and j >= b
    ) for terms in components) for a, b in TERMS)


PLANE_MAP = (((1, 0, 1),), ((0, 1, 1),), ((1, 0, Fraction(1, 4)), (0, 1, Fraction(1, 2))))
BICUBIC_MAP = (
    ((1, 0, 1),), ((0, 1, 1),),
    ((3, 2, Fraction(1, 8)), (2, 1, Fraction(1, 4)),
     (1, 3, Fraction(3, 8)), (1, 1, Fraction(1, 8))),
)
SINGULAR_MAP = (((2, 0, 1),), ((1, 1, 1),), ((0, 2, 1),))


def _polynomial_fixture(name, degree_u, degree_v, components, kind):
    values = []
    for i in range(degree_u + 1):
        for j in range(degree_v + 1):
            point = []
            for terms in components:
                point.append(sum(
                    _ratio(coefficient) * Fraction(math.comb(i, a), math.comb(degree_u, a))
                    * Fraction(math.comb(j, b), math.comb(degree_v, b))
                    for a, b, coefficient in terms if i >= a and j >= b
                ))
            values.extend(float(x) for x in (*point, Fraction(1)))
    return Fixture(name, degree_u, degree_v, tuple(values), kind)


def plane():
    return _polynomial_fixture("plane", 1, 1, PLANE_MAP, "plane")


def asymmetric_bicubic():
    return _polynomial_fixture("asymmetric_bicubic", 3, 3, BICUBIC_MAP, "bicubic")


def singular_chart():
    return _polynomial_fixture("singular_chart", 2, 2, SINGULAR_MAP, "singular")


def rational_cylinder():
    controls = []
    for x, y, weight in ((1, 0, 1), (1, 1, 1), (0, 2, 2)):
        for j in (0, 1):
            controls.extend((float(x), float(y), float(j * weight), float(weight)))
    return Fixture("rational_quarter_cylinder", 2, 1, tuple(controls), "cylinder")


def weighted_bilinear():
    return Fixture(
        "weighted_bilinear", 1, 1,
        (0.0, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 2.0,
         2.0, 0.0, 0.0, 2.0, 3.0, 3.0, 3.0, 3.0), "bilinear",
    )


def analytic_jet(fixture, s, t, order=2):
    """Independently derived analytic maps; polynomial author rounding is separate."""
    kind = fixture.analytic_kind
    s, t = _ratio(s), _ratio(t)
    if kind in ("plane", "bicubic", "singular"):
        result = _polynomial_jet({"plane": PLANE_MAP, "bicubic": BICUBIC_MAP, "singular": SINGULAR_MAP}[kind], s, t)
    elif kind == "cylinder":
        d = 1 + s * s
        zero = Fraction(0)
        result = (
            ((1 - s * s) / d, 2 * s / d, t),
            (-4 * s / d ** 2, 2 * (1 - s * s) / d ** 2, zero),
            (zero, zero, Fraction(1)),
            ((-4 + 12 * s * s) / d ** 3, (-12 * s + 4 * s ** 3) / d ** 3, zero),
            (zero, zero, zero), (zero, zero, zero),
        )
    elif kind == "bilinear":
        # Analytic A(u,v)/(1+u+v), differentiated by the generic product identity.
        # At the midpoint a separately derived literal dyadic table is also used.
        d = 1 + s + t
        numerator = ((2 * s + s * t, 2 * t + s * t, 3 * s * t),
                     (2 + t, t, 3 * t), (s, 2 + s, 3 * s),
                     (0, 0, 0), (1, 1, 3), (0, 0, 0))
        result = []
        for k, (a, b) in enumerate(TERMS):
            result.append(tuple((numerator[k][axis]
                                 - (a * result[TERMS.index((a - 1, b))][axis] if a else 0)
                                 - (b * result[TERMS.index((a, b - 1))][axis] if b else 0)) / d
                                for axis in range(3)))
        result = tuple(result)
    else:
        raise ValueError("fixture has no independent analytic map")
    return result[:TERM_COUNTS[order]]


BILINEAR_MIDPOINT_JET = tuple(tuple(Fraction(n, d) for n, d in row) for row in (
    ((5, 8), (5, 8), (3, 8)),
    ((15, 16), (-1, 16), (9, 16)),
    ((-1, 16), (15, 16), (9, 16)),
    ((-15, 16), (1, 16), (-9, 16)),
    ((1, 16), (1, 16), (15, 16)),
    ((1, 16), (-15, 16), (-9, 16)),
))


def degree_fixture(p, q, pattern):
    controls = []
    for i in range(p + 1):
        for j in range(q + 1):
            if pattern == "constant":
                point = (Fraction(1, 4), Fraction(-1, 2), Fraction(3, 4))
                weight = 1 if (i + j) % 2 else 256
            elif pattern == "asymmetric":
                point = tuple(Fraction(((i * (11 + axis) + j * (7 + 2 * axis) + axis) % 17) - 8, 8)
                              for axis in range(3))
                weight = (1, 256, 3, 128)[(i + 3 * j) % 4]
            elif pattern == "cancellation":
                point = (Fraction((-1) ** (i + j)), Fraction((-1) ** i), Fraction((-1) ** j))
                weight = 1 if (i + j) % 2 else 256
            else:
                raise ValueError("unknown degree fixture pattern")
            controls.extend(float(x * weight) for x in point)
            controls.append(float(weight))
    return Fixture(f"degree_{p}_{q}_{pattern}", p, q, tuple(controls))


def qualification_fixtures():
    return (plane(), asymmetric_bicubic(), rational_cylinder(), weighted_bilinear(), singular_chart()) + tuple(
        degree_fixture(p, q, pattern)
        for p in range(4) for q in range(4)
        for pattern in ("constant", "asymmetric", "cancellation")
    )


def qualification_parameters():
    inward_zero = math.nextafter(0.0, 1.0)
    inward_one = math.nextafter(1.0, 0.0)
    return ((0.0, 0.0), (1.0, 0.0), (0.0, 1.0), (1.0, 1.0),
            (0.5, 0.5), (0.25, 0.75), (0.37, 0.61),
            (inward_zero, 0.5), (inward_one, 0.5),
            (0.5, inward_zero), (0.5, inward_one))


def exact_component_error(observed, expected):
    if not math.isfinite(observed):
        return None
    return abs(Fraction.from_float(float(observed)) - expected)


def exact_conversion(jet, origin, scale, du, dv):
    """Truth for conversion of actual local jet doubles and transform doubles."""
    scale, du, dv = _ratio(scale), _ratio(du), _ratio(dv)
    factors = (scale, scale / du, scale / dv, scale / du ** 2,
               scale / (du * dv), scale / dv ** 2)
    return tuple(tuple(_ratio(value) * factors[k]
                       + (_ratio(origin[axis]) if k == 0 else 0)
                       for axis, value in enumerate(vector))
                 for k, vector in enumerate(jet))
