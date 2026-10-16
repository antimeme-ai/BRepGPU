#include "geometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace bgmesh {
namespace {
using Net = std::pmr::vector<Rational>;
struct ControlShape final {
  std::size_t count;
  uint32_t dimension;
};
struct BasisShape final {
  uint32_t degree;
  std::size_t count;
};
struct PolygonShape final {
  uint32_t degree;
  std::size_t stride;
};
struct BinomialIndex final {
  uint32_t degree, index;
};

[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
void owner(const Rational &value, bgfd::Budget &budget) {
  if (&value.budget() != &budget)
    throw Error(ErrorCode::internal_error, "geometry rational owner mismatch");
}
std::size_t product(std::size_t left, std::size_t right) {
  if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right)
    invalid("geometry control count overflow");
  return left * right;
}
Net zeros(bgfd::Budget &budget, std::size_t count) {
  Net result(&budget);
  result.reserve(count);
  for (std::size_t i = 0; i < count; ++i)
    result.emplace_back(budget);
  budget.charge(static_cast<uint64_t>(count));
  return result;
}
Net import(bgfd::Budget &budget, std::span<const double> values) {
  Net result(&budget);
  result.reserve(values.size());
  for (double value : values) {
    if (!std::isfinite(value))
      invalid("nonfinite original geometry value");
    result.push_back(Rational::from_double(budget, value));
  }
  return result;
}
void validate_knots(bgfd::Budget &budget, std::span<const double> knots, uint32_t degree,
                    std::size_t count) {
  if (count < static_cast<std::size_t>(degree) + 1 ||
      count > std::numeric_limits<std::size_t>::max() - degree - 1 ||
      knots.size() != count + degree + 1)
    invalid("original knot/control/degree mismatch");
  std::size_t repeated = 0;
  for (std::size_t i = 0; i < knots.size(); ++i) {
    budget.charge(1);
    if (!std::isfinite(knots[i]) || (i != 0 && knots[i] < knots[i - 1]))
      invalid("nonfinite or unordered original knot vector");
    repeated = i != 0 && knots[i] == knots[i - 1] ? repeated + 1 : 1;
    if (repeated > static_cast<std::size_t>(degree) + 1)
      invalid("original knot multiplicity exceeds degree plus one");
  }
  if (!(knots[degree] < knots[count]))
    invalid("empty original natural parameter interval");
}
void validate_controls(bgfd::Budget &budget, std::span<const double> controls, ControlShape shape) {
  const auto [count, dimension] = shape;
  const std::size_t stride = static_cast<std::size_t>(dimension) + 1;
  if (controls.size() != product(count, stride))
    invalid("original homogeneous control count mismatch");
  for (std::size_t i = 0; i < controls.size(); ++i) {
    budget.charge(1);
    if (!std::isfinite(controls[i]))
      invalid("nonfinite original homogeneous control");
    if (i % stride == dimension && controls[i] <= 0)
      invalid("nonpositive original homogeneous weight");
  }
}
void validate_local_controls(bgfd::Budget &budget, std::span<const Rational> controls,
                             uint32_t dimension) {
  const auto stride = static_cast<std::size_t>(dimension) + 1;
  for (std::size_t i = 0; i < controls.size(); ++i) {
    owner(controls[i], budget);
    if (i % stride == dimension && controls[i].sign() <= 0)
      invalid("nonpositive owner-local homogeneous weight");
  }
}
bgfd::Budget &validate_curve(const BezierCurve &curve) {
  auto &budget = curve.begin.budget();
  owner(curve.end, budget);
  if ((curve.dimension != 2 && curve.dimension != 3) || curve.degree > 32 ||
      curve.controls.size() != product(static_cast<std::size_t>(curve.degree) + 1,
                                       static_cast<std::size_t>(curve.dimension) + 1) ||
      curve.begin.compare(curve.end) >= 0)
    invalid("malformed owner-local rational curve");
  validate_local_controls(budget, curve.controls, curve.dimension);
  return budget;
}
bgfd::Budget &validate_surface(const BezierSurface &surface) {
  auto &budget = surface.bounds[0].budget();
  for (const auto &bound : surface.bounds)
    owner(bound, budget);
  if (surface.p > 3 || surface.q > 3 ||
      surface.controls.size() != product(product(static_cast<std::size_t>(surface.p) + 1,
                                                 static_cast<std::size_t>(surface.q) + 1),
                                         4) ||
      surface.bounds[0].compare(surface.bounds[1]) >= 0 ||
      surface.bounds[2].compare(surface.bounds[3]) >= 0)
    invalid("malformed owner-local rational surface");
  validate_local_controls(budget, surface.controls, 3);
  return budget;
}

struct SpanBasis final {
  std::size_t first;
  Rational begin, end;
  Net coefficients;
  SpanBasis(bgfd::Budget &budget, std::size_t first_value, const std::array<Rational, 2> &range)
      : first(first_value), begin(range[0]), end(range[1]), coefficients(&budget) {}
};

// Add the Bernstein product of an affine endpoint pair and a degree(r-1)
// polynomial to a degree-r destination. All values belong to one owner.
void add_affine_product(Net &destination, std::size_t offset, std::span<const Rational> previous,
                        const Rational &a0, const Rational &a1, uint32_t degree) {
  auto &budget = a0.budget();
  const Rational divisor(budget, static_cast<int64_t>(degree));
  for (uint32_t j = 0; j <= degree; ++j) {
    if (j < degree) {
      const auto factor = Rational(budget, static_cast<int64_t>(degree - j)).divide(divisor);
      const auto term = factor.multiply(a0).multiply(previous[j]);
      destination[offset + j] = destination[offset + j].add(term);
    }
    if (j != 0) {
      const auto factor = Rational(budget, static_cast<int64_t>(j)).divide(divisor);
      const auto term = factor.multiply(a1).multiply(previous[j - 1]);
      destination[offset + j] = destination[offset + j].add(term);
    }
  }
}

std::pmr::vector<SpanBasis> extract_basis(bgfd::Budget &budget, const Net &knots,
                                          BasisShape shape) {
  const auto [degree, count] = shape;
  std::pmr::vector<SpanBasis> result(&budget);
  for (std::size_t k = degree; k < count; ++k) {
    budget.charge(1);
    if (knots[k].compare(knots[k + 1]) == 0)
      continue;
    Net previous(&budget);
    previous.emplace_back(budget, 1);
    for (uint32_t r = 1; r <= degree; ++r) {
      const auto width = static_cast<std::size_t>(r) + 1;
      auto next = zeros(budget, product(width, width));
      const auto previous_first = k - (r - 1);
      for (std::size_t position = 0; position < width; ++position) {
        const auto i = k - r + position;
        if (i >= previous_first) {
          const auto denominator = knots[i + r].subtract(knots[i]);
          if (denominator.sign() != 0) {
            const auto a0 = knots[k].subtract(knots[i]).divide(denominator);
            const auto a1 = knots[k + 1].subtract(knots[i]).divide(denominator);
            const auto input =
                std::span<const Rational>(previous).subspan(product(i - previous_first, r), r);
            add_affine_product(next, position * width, input, a0, a1, r);
          }
        }
        if (i + 1 <= k) {
          const auto denominator = knots[i + r + 1].subtract(knots[i + 1]);
          if (denominator.sign() != 0) {
            const auto a0 = knots[i + r + 1].subtract(knots[k]).divide(denominator);
            const auto a1 = knots[i + r + 1].subtract(knots[k + 1]).divide(denominator);
            const auto input =
                std::span<const Rational>(previous).subspan(product(i + 1 - previous_first, r), r);
            add_affine_product(next, position * width, input, a0, a1, r);
          }
        }
      }
      previous = std::move(next);
    }
    SpanBasis basis(budget, k - degree, {knots[k], knots[k + 1]});
    basis.coefficients = std::move(previous);
    result.push_back(std::move(basis));
  }
  return result;
}

bool same_point(std::span<const Rational> left, std::span<const Rational> right,
                uint32_t dimension) {
  for (uint32_t axis = 0; axis < dimension; ++axis)
    if (left[axis].multiply(right[dimension]).compare(right[axis].multiply(left[dimension])) != 0)
      return false;
  return true;
}
uint64_t binomial(BinomialIndex entry) {
  const auto [n, k] = entry;
  uint64_t result = 1;
  for (uint32_t i = 1; i <= k; ++i)
    result = result * (n + 1 - i) / i;
  return result;
}
// Full rational boundary-curve identity, including different homogeneous
// scales.
bool same_boundary(std::span<const Rational> left, std::span<const Rational> right,
                   uint32_t degree) {
  auto &budget = left[0].budget();
  for (uint32_t axis = 0; axis < 3; ++axis)
    for (uint32_t k = 0; k <= 2 * degree; ++k) {
      Rational sum(budget);
      for (uint32_t i = 0; i <= degree; ++i) {
        if (k < i || k - i > degree)
          continue;
        const uint32_t j = k - i;
        const auto factor =
            Rational(budget, static_cast<int64_t>(binomial({.degree = degree, .index = i}) *
                                                  binomial({.degree = degree, .index = j})));
        // The common positive binomial(2*degree,k) denominator cancels for
        // zero.
        const auto term = left[4 * i + axis]
                              .multiply(right[4 * j + 3])
                              .subtract(right[4 * i + axis].multiply(left[4 * j + 3]));
        sum = sum.add(term.multiply(factor));
      }
      if (sum.sign() != 0)
        return false;
    }
  return true;
}
Rational normalized(const Rational &value, const Rational &begin, const Rational &end) {
  owner(value, begin.budget());
  owner(end, begin.budget());
  if (begin.compare(end) >= 0 || value.compare(begin) < 0 || value.compare(end) > 0)
    invalid("geometry query outside original span");
  return value.subtract(begin).divide(end.subtract(begin));
}
Net evaluate_homogeneous(std::span<const Rational> controls, PolygonShape shape,
                         const Rational &parameter) {
  const auto [degree, stride] = shape;
  auto &budget = parameter.budget();
  Net current(controls.begin(), controls.end(), &budget);
  const auto one_minus = Rational(budget, 1).subtract(parameter);
  for (uint32_t level = degree; level != 0; --level)
    for (uint32_t i = 0; i < level; ++i)
      for (std::size_t axis = 0; axis < stride; ++axis) {
        const auto index = stride * i + axis;
        current[index] =
            current[index].multiply(one_minus).add(current[index + stride].multiply(parameter));
      }
  current.erase(current.begin() + static_cast<std::ptrdiff_t>(stride), current.end());
  return current;
}
struct Split final {
  Net left, right;
};
Split split(std::span<const Rational> controls, uint32_t degree, std::size_t stride,
            const Rational &parameter) {
  auto &budget = parameter.budget();
  Net current(controls.begin(), controls.end(), &budget);
  auto left = zeros(budget, controls.size());
  auto right = zeros(budget, controls.size());
  const auto one_minus = Rational(budget, 1).subtract(parameter);
  for (uint32_t level = 0; level <= degree; ++level) {
    for (std::size_t axis = 0; axis < stride; ++axis) {
      left[stride * level + axis] = current[axis];
      right[stride * (degree - level) + axis] = current[stride * (degree - level) + axis];
    }
    for (uint32_t i = 0; i < degree - level; ++i)
      for (std::size_t axis = 0; axis < stride; ++axis) {
        const auto index = stride * i + axis;
        current[index] =
            current[index].multiply(one_minus).add(current[index + stride].multiply(parameter));
      }
  }
  return {std::move(left), std::move(right)};
}
Net restrict_polygon(std::span<const Rational> controls, uint32_t degree, std::size_t stride,
                     const Rational &begin, const Rational &end) {
  if (begin.compare(end) >= 0)
    invalid("empty or reversed geometry restriction");
  auto first = split(controls, degree, stride, begin);
  const auto parameter = end.subtract(begin).divide(Rational(begin.budget(), 1).subtract(begin));
  return split(first.right, degree, stride, parameter).left;
}

std::size_t triangular_count(uint32_t degree) {
  return (static_cast<std::size_t>(degree) + 1) * (degree + 2) / 2;
}
std::size_t triangular_index(uint32_t degree, uint32_t i, uint32_t j) {
  return static_cast<std::size_t>(i) * (2 * degree + 3 - i) / 2 + j;
}
uint64_t multinomial(uint32_t degree, uint32_t i, uint32_t j) {
  return binomial({.degree = degree, .index = i}) * binomial({.degree = degree - i, .index = j});
}
struct ScalarTriangle final {
  uint32_t degree;
  Net coefficients;
  ScalarTriangle(bgfd::Budget &budget, uint32_t degree_value)
      : degree(degree_value), coefficients(zeros(budget, triangular_count(degree_value))) {}
};
ScalarTriangle multiply(const ScalarTriangle &left, const ScalarTriangle &right) {
  auto &budget = left.coefficients[0].budget();
  const uint32_t degree = left.degree + right.degree;
  ScalarTriangle result(budget, degree);
  for (uint32_t i = 0; i <= left.degree; ++i)
    for (uint32_t j = 0; j <= left.degree - i; ++j) {
      const auto &a = left.coefficients[triangular_index(left.degree, i, j)];
      if (a.sign() == 0)
        continue;
      for (uint32_t k = 0; k <= right.degree; ++k)
        for (uint32_t l = 0; l <= right.degree - k; ++l) {
          const auto &b = right.coefficients[triangular_index(right.degree, k, l)];
          if (b.sign() == 0)
            continue;
          const auto numerator = multinomial(left.degree, i, j) * multinomial(right.degree, k, l);
          const auto denominator = multinomial(degree, i + k, j + l);
          const auto factor = Rational(budget, static_cast<int64_t>(numerator))
                                  .divide(Rational(budget, static_cast<int64_t>(denominator)));
          const auto index = triangular_index(degree, i + k, j + l);
          result.coefficients[index] =
              result.coefficients[index].add(a.multiply(b).multiply(factor));
        }
    }
  return result;
}
std::pmr::vector<ScalarTriangle> affine_powers(bgfd::Budget &budget, const ScalarTriangle &affine,
                                               uint32_t degree) {
  std::pmr::vector<ScalarTriangle> result(&budget);
  result.reserve(static_cast<std::size_t>(degree) + 1);
  ScalarTriangle one(budget, 0);
  one.coefficients[0] = Rational(budget, 1);
  result.push_back(std::move(one));
  for (uint32_t i = 1; i <= degree; ++i)
    result.push_back(multiply(result.back(), affine));
  return result;
}
std::pmr::vector<ScalarTriangle> composed_basis(bgfd::Budget &budget,
                                                const ScalarTriangle &coordinate, uint32_t degree) {
  ScalarTriangle complement(budget, 1);
  for (std::size_t i = 0; i < 3; ++i)
    complement.coefficients[i] = Rational(budget, 1).subtract(coordinate.coefficients[i]);
  const auto positive = affine_powers(budget, coordinate, degree);
  const auto negative = affine_powers(budget, complement, degree);
  std::pmr::vector<ScalarTriangle> result(&budget);
  result.reserve(static_cast<std::size_t>(degree) + 1);
  for (uint32_t i = 0; i <= degree; ++i) {
    auto item = multiply(positive[i], negative[degree - i]);
    const Rational factor(budget, static_cast<int64_t>(binomial({.degree = degree, .index = i})));
    for (auto &coefficient : item.coefficients)
      coefficient = coefficient.multiply(factor);
    result.push_back(std::move(item));
  }
  return result;
}
} // namespace

BezierCurve::BezierCurve(bgfd::Budget &budget, CurveShape shape,
                         const std::array<Rational, 2> &domain)
    : dimension(shape.dimension), degree(shape.degree), begin(domain[0].clone_into(budget)),
      end(domain[1].clone_into(budget)), controls(&budget) {}
BezierCurve::BezierCurve(const BezierCurve &other)
    : dimension(other.dimension), degree(other.degree), begin(other.begin), end(other.end),
      controls(other.controls, &other.begin.budget()) {}
BezierSurface::BezierSurface(bgfd::Budget &budget, uint32_t p_value, uint32_t q_value,
                             const std::array<Rational, 4> &bounds_value)
    : p(p_value), q(q_value),
      bounds{bounds_value[0].clone_into(budget), bounds_value[1].clone_into(budget),
             bounds_value[2].clone_into(budget), bounds_value[3].clone_into(budget)},
      controls(&budget) {}
BezierSurface::BezierSurface(const BezierSurface &other)
    : p(other.p), q(other.q), bounds(other.bounds),
      controls(other.controls, &other.bounds[0].budget()) {}

std::pmr::vector<BezierCurve> extract_curve(bgfd::Budget &budget, BSplineCurveView source) {
  if ((source.dimension != 2 && source.dimension != 3) || source.degree > 32)
    invalid("unsupported original rational curve dimension or degree");
  const std::size_t stride = static_cast<std::size_t>(source.dimension) + 1;
  if (source.controls.size() % stride != 0)
    invalid("original curve homogeneous stride mismatch");
  const auto count = source.controls.size() / stride;
  validate_knots(budget, source.knots, source.degree, count);
  validate_controls(budget, source.controls, {.count = count, .dimension = source.dimension});
  const auto knots = import(budget, source.knots);
  const auto original = import(budget, source.controls);
  const auto basis = extract_basis(budget, knots, {.degree = source.degree, .count = count});
  std::pmr::vector<BezierCurve> result(&budget);
  result.reserve(basis.size());
  const auto width = static_cast<std::size_t>(source.degree) + 1;
  for (const auto &item : basis) {
    BezierCurve curve(budget, {.dimension = source.dimension, .degree = source.degree},
                      {item.begin, item.end});
    curve.controls = zeros(budget, product(width, stride));
    for (std::size_t input = 0; input < width; ++input)
      for (std::size_t output = 0; output < width; ++output)
        for (std::size_t axis = 0; axis < stride; ++axis) {
          const auto index = output * stride + axis;
          curve.controls[index] =
              curve.controls[index].add(original[(item.first + input) * stride + axis].multiply(
                  item.coefficients[input * width + output]));
        }
    if (!result.empty() &&
        !same_point(std::span<const Rational>(result.back().controls).last(stride),
                    std::span<const Rational>(curve.controls).first(stride), source.dimension))
      invalid("positionally discontinuous original rational curve");
    result.push_back(std::move(curve));
  }
  return result;
}

std::pmr::vector<BezierSurface> extract_surface(bgfd::Budget &budget, BSplineSurfaceView source) {
  if (source.p > 3 || source.q > 3)
    invalid("unsupported original rational surface degree");
  validate_knots(budget, source.knots_u, source.p, source.nu);
  validate_knots(budget, source.knots_v, source.q, source.nv);
  validate_controls(budget, source.controls,
                    {.count = product(source.nu, source.nv), .dimension = 3});
  const auto ku = import(budget, source.knots_u), kv = import(budget, source.knots_v);
  const auto original = import(budget, source.controls);
  const auto ub = extract_basis(budget, ku, {.degree = source.p, .count = source.nu});
  const auto vb = extract_basis(budget, kv, {.degree = source.q, .count = source.nv});
  const auto uw = static_cast<std::size_t>(source.p) + 1;
  const auto vw = static_cast<std::size_t>(source.q) + 1;
  std::pmr::vector<BezierSurface> result(&budget);
  result.reserve(product(ub.size(), vb.size()));
  for (const auto &u : ub)
    for (const auto &v : vb) {
      BezierSurface surface(budget, source.p, source.q, {u.begin, u.end, v.begin, v.end});
      surface.controls = zeros(budget, product(product(uw, vw), 4));
      for (std::size_t i = 0; i < uw; ++i)
        for (std::size_t j = 0; j < vw; ++j)
          for (std::size_t a = 0; a < uw; ++a)
            for (std::size_t b = 0; b < vw; ++b) {
              const auto factor = u.coefficients[i * uw + a].multiply(v.coefficients[j * vw + b]);
              for (std::size_t axis = 0; axis < 4; ++axis) {
                const auto index = (a * vw + b) * 4 + axis;
                surface.controls[index] = surface.controls[index].add(
                    original[((u.first + i) * source.nv + v.first + j) * 4 + axis].multiply(
                        factor));
              }
            }
      result.push_back(std::move(surface));
    }
  for (std::size_t i = 0; i < ub.size(); ++i)
    for (std::size_t j = 0; j < vb.size(); ++j) {
      const auto &current = result[i * vb.size() + j];
      if (i != 0) {
        const auto &previous = result[(i - 1) * vb.size() + j];
        const auto left = std::span<const Rational>(previous.controls).last(4 * vw);
        const auto right = std::span<const Rational>(current.controls).first(4 * vw);
        if (!same_boundary(left, right, source.q))
          invalid("positionally discontinuous original rational surface U "
                  "boundary");
      }
      if (j != 0) {
        const auto &previous = result[i * vb.size() + j - 1];
        Net left(&budget), right(&budget);
        left.reserve(4 * uw);
        right.reserve(4 * uw);
        for (std::size_t a = 0; a < uw; ++a)
          for (std::size_t axis = 0; axis < 4; ++axis) {
            left.push_back(previous.controls[(a * vw + vw - 1) * 4 + axis]);
            right.push_back(current.controls[a * vw * 4 + axis]);
          }
        if (!same_boundary(left, right, source.p))
          invalid("positionally discontinuous original rational surface V "
                  "boundary");
      }
    }
  return result;
}

std::pmr::vector<Rational> evaluate_curve(const BezierCurve &curve, const Rational &parameter) {
  auto &budget = validate_curve(curve);
  const auto normalized_parameter = normalized(parameter, curve.begin, curve.end);
  const auto stride = static_cast<std::size_t>(curve.dimension) + 1;
  const auto value = evaluate_homogeneous(
      curve.controls, {.degree = curve.degree, .stride = stride}, normalized_parameter);
  if (value[curve.dimension].sign() <= 0)
    invalid("nonpositive evaluated rational curve weight");
  Net result(&budget);
  result.reserve(curve.dimension);
  for (uint32_t axis = 0; axis < curve.dimension; ++axis)
    result.push_back(value[axis].divide(value[curve.dimension]));
  return result;
}

std::array<Rational, 3> evaluate_surface(const BezierSurface &surface, const Rational &u,
                                         const Rational &v) {
  auto &budget = validate_surface(surface);
  const auto su = normalized(u, surface.bounds[0], surface.bounds[1]);
  const auto sv = normalized(v, surface.bounds[2], surface.bounds[3]);
  const auto uw = static_cast<std::size_t>(surface.p) + 1;
  const auto vw = static_cast<std::size_t>(surface.q) + 1;
  Net rows(&budget);
  rows.reserve(4 * uw);
  for (std::size_t i = 0; i < uw; ++i) {
    const auto row = evaluate_homogeneous(
        std::span<const Rational>(surface.controls).subspan(i * vw * 4, vw * 4),
        {.degree = surface.q, .stride = 4}, sv);
    rows.insert(rows.end(), row.begin(), row.end());
  }
  const auto value = evaluate_homogeneous(rows, {.degree = surface.p, .stride = 4}, su);
  if (value[3].sign() <= 0)
    invalid("nonpositive evaluated rational surface weight");
  return {value[0].divide(value[3]), value[1].divide(value[3]), value[2].divide(value[3])};
}

BezierCurve restrict_curve(const BezierCurve &curve, const Rational &begin, const Rational &end) {
  static_cast<void>(validate_curve(curve));
  const auto a = normalized(begin, curve.begin, curve.end);
  const auto b = normalized(end, curve.begin, curve.end);
  BezierCurve result(curve.begin.budget(), {.dimension = curve.dimension, .degree = curve.degree},
                     {begin, end});
  result.controls = restrict_polygon(curve.controls, curve.degree,
                                     static_cast<std::size_t>(curve.dimension) + 1, a, b);
  return result;
}

BezierSurface restrict_surface(const BezierSurface &surface,
                               const std::array<Rational, 4> &bounds) {
  auto &budget = validate_surface(surface);
  const auto ua = normalized(bounds[0], surface.bounds[0], surface.bounds[1]);
  const auto ub = normalized(bounds[1], surface.bounds[0], surface.bounds[1]);
  const auto va = normalized(bounds[2], surface.bounds[2], surface.bounds[3]);
  const auto vb = normalized(bounds[3], surface.bounds[2], surface.bounds[3]);
  const auto uw = static_cast<std::size_t>(surface.p) + 1;
  const auto vw = static_cast<std::size_t>(surface.q) + 1;
  auto intermediate = zeros(budget, surface.controls.size());
  for (std::size_t j = 0; j < vw; ++j) {
    Net column(&budget);
    column.reserve(4 * uw);
    for (std::size_t i = 0; i < uw; ++i)
      for (std::size_t axis = 0; axis < 4; ++axis)
        column.push_back(surface.controls[(i * vw + j) * 4 + axis]);
    const auto cut = restrict_polygon(column, surface.p, 4, ua, ub);
    for (std::size_t i = 0; i < uw; ++i)
      for (std::size_t axis = 0; axis < 4; ++axis)
        intermediate[(i * vw + j) * 4 + axis] = cut[i * 4 + axis];
  }
  BezierSurface result(budget, surface.p, surface.q, bounds);
  result.controls.reserve(surface.controls.size());
  for (std::size_t i = 0; i < uw; ++i) {
    const auto cut = restrict_polygon(
        std::span<const Rational>(intermediate).subspan(i * vw * 4, vw * 4), surface.q, 4, va, vb);
    result.controls.insert(result.controls.end(), cut.begin(), cut.end());
  }
  return result;
}

BezierTriangle restrict_to_triangle(const BezierSurface &surface,
                                    std::span<const Rational, 6> original_uv) {
  auto &budget = validate_surface(surface);
  ScalarTriangle s(budget, 1), t(budget, 1);
  // Affine Bernstein order is lambda0,lambda2,lambda1, not input corner order.
  constexpr std::array<std::size_t, 3> corner{0, 2, 1};
  for (std::size_t i = 0; i < 3; ++i) {
    s.coefficients[i] =
        normalized(original_uv[2 * corner[i]], surface.bounds[0], surface.bounds[1]);
    t.coefficients[i] =
        normalized(original_uv[2 * corner[i] + 1], surface.bounds[2], surface.bounds[3]);
  }
  const auto u_basis = composed_basis(budget, s, surface.p);
  const auto v_basis = composed_basis(budget, t, surface.q);
  BezierTriangle result(budget, surface.p + surface.q);
  result.controls = zeros(budget, product(triangular_count(result.degree), 4));
  for (uint32_t i = 0; i <= surface.p; ++i)
    for (uint32_t j = 0; j <= surface.q; ++j) {
      const auto basis = multiply(u_basis[i], v_basis[j]);
      const auto source = (static_cast<std::size_t>(i) * (surface.q + 1) + j) * 4;
      for (std::size_t coefficient = 0; coefficient < basis.coefficients.size(); ++coefficient) {
        if (basis.coefficients[coefficient].sign() == 0)
          continue;
        for (std::size_t axis = 0; axis < 4; ++axis) {
          const auto target = coefficient * 4 + axis;
          result.controls[target] = result.controls[target].add(
              surface.controls[source + axis].multiply(basis.coefficients[coefficient]));
        }
      }
    }
  return result;
}
} // namespace bgmesh
