#include "bound_tasks.hpp"
#include "bound_filter.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cfloat>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw bgfd::Failure(BGFD_INVALID_INPUT, message); }
void same_owner(const Rational &value, bgfd::Budget &budget) {
  if (&value.budget() != &budget)
    throw bgfd::Failure(BGFD_INTERNAL_ERROR, "bound task input allocator mismatch");
}
void validate(std::span<const Rational> controls, const Rational &tolerance, std::size_t count,
              std::span<const Rational> linear) {
  if (controls.size() != count * 4 || tolerance.sign() < 0)
    invalid("invalid residual net size or negative tolerance");
  auto &budget = tolerance.budget();
  for (const auto &value : controls)
    same_owner(value, budget);
  for (const auto &value : linear)
    same_owner(value, budget);
  for (std::size_t i = 3; i < controls.size(); i += 4)
    if (controls[i].sign() <= 0)
      invalid("nonpositive residual weight coefficient");
}
Rational threshold(std::span<const Rational> controls, const Rational &tolerance) {
  Rational minimum(controls[3]);
  for (std::size_t i = 7; i < controls.size(); i += 4)
    if (controls[i].compare(minimum) < 0)
      minimum = controls[i];
  return tolerance.multiply(minimum).multiply(tolerance.multiply(minimum));
}
std::pmr::vector<Rational> factors(bgfd::Budget &budget, uint32_t degree) {
  std::pmr::vector<Rational> result(&budget);
  result.reserve(static_cast<std::size_t>(degree) + 1);
  const Rational denominator(budget, degree);
  for (uint32_t i = 0; i <= degree; ++i)
    result.push_back(Rational(budget, i).divide(denominator));
  return result;
}
std::size_t index(uint32_t degree, uint32_t i, uint32_t j) {
  return static_cast<std::size_t>(i) * (degree + 1) -
         static_cast<std::size_t>(i) * (i == 0 ? 0 : i - 1) / 2 + j;
}
} // namespace

BoundAnswer triangle_residual_exact(BoundTaskKey key, TriangleResidualView input) {
  if (input.degree > 6)
    invalid("unsupported triangle residual degree");
  const auto d = input.degree, degree = d + 1;
  validate(input.homogeneous, input.tolerance, static_cast<std::size_t>(d + 1) * (d + 2) / 2,
           input.linear_xyz);
  auto &budget = input.tolerance.budget();
  const auto limit = threshold(input.homogeneous, input.tolerance);
  const auto factor = factors(budget, degree);
  for (uint32_t i = 0; i <= degree; ++i)
    for (uint32_t j = 0; j <= degree - i; ++j) {
      const uint32_t beta[]{degree - i - j, i, j};
      Rational squared(budget);
      for (uint32_t axis = 0; axis < 3; ++axis) {
        Rational residual(budget);
        for (uint32_t k = 0; k < 3; ++k) {
          if (beta[k] == 0)
            continue;
          const auto at =
              index(d, i - static_cast<uint32_t>(k == 1), j - static_cast<uint32_t>(k == 2)) * 4;
          const auto term = input.homogeneous[at + axis].subtract(
              input.homogeneous[at + 3].multiply(input.linear_xyz[k * 3 + axis]));
          residual = residual.add(term.multiply(factor[beta[k]]));
        }
        squared = squared.add(residual.multiply(residual));
      }
      if (squared.compare(limit) > 0)
        return {key, BoundDecision::bound_fail};
    }
  return {key, BoundDecision::accept};
}

BoundAnswer surface_bound_exact(BoundTaskKey key, const BezierSurface &source,
                                std::span<const Rational, 6> original_uv,
                                std::span<const Rational, 9> linear_xyz,
                                const Rational &tolerance) {
  if (source.p > 3 || source.q > 3)
    invalid("unsupported source surface bound degree");
  auto &budget = tolerance.budget();
  for (const auto &value : source.bounds)
    same_owner(value, budget);
  for (const auto &value : original_uv)
    same_owner(value, budget);
  validate(source.controls, tolerance, static_cast<std::size_t>(source.p + 1) * (source.q + 1),
           linear_xyz);
  const auto triangle = restrict_to_triangle(source, original_uv);
  return triangle_residual_exact(key, {triangle.degree, triangle.controls, linear_xyz, tolerance});
}

BoundAnswer curve_bound_exact(BoundTaskKey key, const BezierCurve &source, const Rational &begin,
                              const Rational &end, std::span<const Rational, 6> endpoint_xyz,
                              const Rational &tolerance) {
  if (source.degree > 32 || source.dimension != 3)
    invalid("unsupported source curve bound degree or dimension");
  auto &budget = tolerance.budget();
  same_owner(source.begin, budget);
  same_owner(source.end, budget);
  same_owner(begin, budget);
  same_owner(end, budget);
  validate(source.controls, tolerance, static_cast<std::size_t>(source.degree) + 1, endpoint_xyz);
  const auto curve = restrict_curve(source, begin, end);
  const auto degree = source.degree + 1;
  const auto factor = factors(budget, degree);
  const auto limit = threshold(curve.controls, tolerance);
  for (uint32_t i = 0; i <= degree; ++i) {
    Rational squared(budget);
    for (uint32_t axis = 0; axis < 3; ++axis) {
      Rational residual(budget);
      if (i < degree) {
        const auto at = i * 4;
        residual = curve.controls[at + axis]
                       .subtract(curve.controls[at + 3].multiply(endpoint_xyz[axis]))
                       .multiply(factor[degree - i]);
      }
      if (i != 0) {
        const auto at = (i - 1) * 4;
        const auto term = curve.controls[at + axis]
                              .subtract(curve.controls[at + 3].multiply(endpoint_xyz[3 + axis]))
                              .multiply(factor[i]);
        residual = residual.add(term);
      }
      squared = squared.add(residual.multiply(residual));
    }
    if (squared.compare(limit) > 0)
      return {key, BoundDecision::bound_fail};
  }
  return {key, BoundDecision::accept};
}
namespace {
bgmb_interval enclose(const Rational &value) {
  // An unrepresentable endpoint is numerical uncertainty, while a real Budget
  // failure must propagate. Precheck overflow exactly, not by catching the
  // shared resource-exhausted status of arbitrary arithmetic allocations.
  const auto n = value.numerator().bit_length(), d = value.denominator().bit_length();
  if (n > d && n - d >= 1023) {
    const auto maximum =
        Rational::from_double(value.budget(), std::bit_cast<double>(uint64_t{0x7fefffffffffffff}));
    const auto positive = value.sign() < 0 ? value.negative() : Rational(value);
    if (positive.compare(maximum) > 0)
      return bgmesh_interval::invalid();
  }
  const auto result = value.bounds();
  return {result.lower, result.upper};
}
bgmb_key pack_key(BoundTaskKey key) { return {key.owner, key.cell, key.generation, key.ordinal}; }
BoundTaskKey unpack_key(bgmb_key key) { return {key.owner, key.cell, key.generation, key.ordinal}; }
std::array<Rational, 3> root_anchor(std::span<const Rational> controls) {
  if (controls.size() < 4 || controls[3].sign() <= 0)
    invalid("invalid bound anchor weight");
  return {controls[0].divide(controls[3]), controls[1].divide(controls[3]),
          controls[2].divide(controls[3])};
}
bool environment() {
#if FLT_EVAL_METHOD != 0 || !defined(__x86_64__)
  return false;
#else
  if (std::fegetround() != FE_TONEAREST)
    return false;
#if defined(__x86_64__)
  if ((_mm_getcsr() & uint32_t{0xe040}) != 0)
    return false;
#endif
  return true;
#endif
}
template <unsigned P, unsigned Q = 0>
uint32_t filter_surface(uint32_t q, const bgmb_interval *source, const bgmb_task &task,
                        const bgmb_interval *uv) {
  if (q == Q) {
    constexpr uint32_t degree = P + Q, count = (degree + 1) * (degree + 2) / 2;
    bgmb_interval net[count * 4];
    for (uint32_t i = 0; i <= degree; ++i)
      for (uint32_t j = 0; j <= degree - i; ++j)
        for (uint32_t axis = 0; axis < 4; ++axis)
          net[bgmesh_filter::index(degree, i, j) * 4 + axis] =
              bgmesh_filter::surface_coefficient<P, Q>(source, {i, j, axis}, uv);
    const auto limit = bgmesh_filter::limit(net, count, task.tolerance);
    bool unresolved = false;
    for (uint32_t i = 0; i <= degree + 1; ++i)
      for (uint32_t j = 0; j <= degree + 1 - i; ++j) {
        const auto decision = bgmesh_filter::triangle_decision(net, degree, task, limit, i, j);
        if (decision == 1)
          return 1;
        unresolved = unresolved || decision == 2;
      }
    return unresolved ? 2 : 0;
  }
  if constexpr (Q < 3)
    return filter_surface<P, Q + 1>(q, source, task, uv);
  return 2;
}
template <unsigned N = 0>
uint32_t filter_curve(uint32_t degree, const bgmb_interval *source, const bgmb_task &task,
                      const bgmb_interval *parameter) {
  if (degree == N) {
    bgmb_interval net[(N + 1) * 4];
    for (uint32_t i = 0; i <= N; ++i)
      for (uint32_t axis = 0; axis < 4; ++axis)
        net[i * 4 + axis] = bgmesh_filter::curve_coefficient<N>(source, {i, 0, axis}, parameter);
    const auto limit = bgmesh_filter::limit(net, N + 1, task.tolerance);
    bool unresolved = false;
    for (uint32_t i = 0; i <= N + 1; ++i) {
      const auto decision = bgmesh_filter::curve_decision(net, N, task, limit, i);
      if (decision == 1)
        return 1;
      unresolved = unresolved || decision == 2;
    }
    return unresolved ? 2 : 0;
  }
  if constexpr (N < 32)
    return filter_curve<N + 1>(degree, source, task, parameter);
  return 2;
}
} // namespace
bgmb_root pack_bound_root(const BezierSurface &source, std::span<bgmb_interval> output,
                          uint64_t offset) {
  if (source.p > 3 || source.q > 3 ||
      source.controls.size() != 4 * (source.p + 1) * (source.q + 1) ||
      output.size() != source.controls.size())
    invalid("invalid surface bound root");
  bgmb_root result{0, source.p, source.q, 0, offset, output.size(), {}};
  for (const auto &value : source.controls)
    same_owner(value, source.bounds[0].budget());
  const auto anchor = root_anchor(source.controls);
  for (std::size_t i = 0; i < 4; ++i) {
    same_owner(source.bounds[i], source.bounds[0].budget());
    result.bounds[i] = enclose(source.bounds[i]);
  }
  if (source.bounds[0].compare(source.bounds[1]) >= 0 ||
      source.bounds[2].compare(source.bounds[3]) >= 0)
    invalid("invalid original surface root domain");
  for (std::size_t i = 0; i < output.size(); ++i) {
    same_owner(source.controls[i], source.bounds[0].budget());
    if (i % 4 == 3 && source.controls[i].sign() <= 0)
      invalid("nonpositive source root weight");
    output[i] = i % 4 == 3 ? enclose(source.controls[i])
                           : enclose(source.controls[i].subtract(
                                 source.controls[(i / 4) * 4 + 3].multiply(anchor[i % 4])));
  }
  return result;
}
bgmb_root pack_bound_root(const BezierCurve &source, std::span<bgmb_interval> output,
                          uint64_t offset) {
  if (source.degree > 32 || source.dimension != 3 ||
      source.controls.size() != 4 * (source.degree + 1) || output.size() != source.controls.size())
    invalid("invalid curve bound root");
  bgmb_root result{1, source.degree, 0, 0, offset, output.size(), {}};
  for (const auto &value : source.controls)
    same_owner(value, source.begin.budget());
  const auto anchor = root_anchor(source.controls);
  same_owner(source.end, source.begin.budget());
  if (source.begin.compare(source.end) >= 0)
    invalid("invalid original curve root domain");
  result.bounds[0] = enclose(source.begin);
  result.bounds[1] = enclose(source.end);
  for (std::size_t i = 0; i < output.size(); ++i) {
    same_owner(source.controls[i], source.begin.budget());
    if (i % 4 == 3 && source.controls[i].sign() <= 0)
      invalid("nonpositive source root weight");
    output[i] = i % 4 == 3 ? enclose(source.controls[i])
                           : enclose(source.controls[i].subtract(
                                 source.controls[(i / 4) * 4 + 3].multiply(anchor[i % 4])));
  }
  return result;
}
bgmb_task pack_bound_task(BoundTaskKey key, uint32_t root, const BezierSurface &source,
                          std::span<const Rational, 6> uv, std::span<const Rational, 9> xyz,
                          const Rational &tau) {
  if (source.p > 3 || source.q > 3)
    invalid("unsupported surface bound task degree");
  validate(source.controls, tau, static_cast<std::size_t>(source.p + 1) * (source.q + 1), xyz);
  bgmb_task result{};
  result.key = pack_key(key);
  result.root = root;
  const auto anchor = root_anchor(source.controls);
  for (std::size_t i = 0; i < 6; ++i) {
    same_owner(uv[i], tau.budget());
    const auto axis = i % 2;
    if (uv[i].compare(source.bounds[axis * 2]) < 0 ||
        uv[i].compare(source.bounds[axis * 2 + 1]) > 0)
      invalid("surface bound subcell leaves original span");
    result.parameter[i] = enclose(uv[i]);
  }
  for (std::size_t i = 0; i < 9; ++i)
    result.linear_xyz[i] = enclose(xyz[i].subtract(anchor[i % 3]));
  result.tolerance = enclose(tau);
  return result;
}
bgmb_task pack_bound_task(BoundTaskKey key, uint32_t root, const BezierCurve &source,
                          const Rational &begin, const Rational &end,
                          std::span<const Rational, 6> xyz, const Rational &tau) {
  if (source.degree > 32 || source.dimension != 3)
    invalid("unsupported curve bound task degree/dimension");
  validate(source.controls, tau, static_cast<std::size_t>(source.degree) + 1, xyz);
  same_owner(begin, tau.budget());
  same_owner(end, tau.budget());
  if (begin.compare(end) >= 0 || begin.compare(source.begin) < 0 || end.compare(source.end) > 0)
    invalid("invalid original curve bound interval");
  bgmb_task result{};
  result.key = pack_key(key);
  result.root = root;
  const auto anchor = root_anchor(source.controls);
  result.parameter[0] = enclose(begin);
  result.parameter[1] = enclose(end);
  for (std::size_t i = 0; i < 6; ++i)
    result.linear_xyz[i] = enclose(xyz[i].subtract(anchor[i % 3]));
  result.tolerance = enclose(tau);
  return result;
}
BoundAnswer filter_bound_cpu(bgfd::Budget &budget, const bgmb_root &root,
                             std::span<const bgmb_interval> controls, const bgmb_task &task) {
  const auto key = unpack_key(task.key);
  if (!environment())
    return {key, BoundDecision::unresolved};
  if (root.reserved != 0 || task.reserved != 0 || root.kind > 1 ||
      root.p > (root.kind == 0 ? 3u : 32u) || root.q > (root.kind == 0 ? 3u : 0u) ||
      root.offset > controls.size() || root.count > controls.size() - root.offset ||
      root.count != 4 * (root.p + 1) * (root.kind == 0 ? root.q + 1 : 1))
    invalid("invalid packed bound root/task");
  budget.charge(bgmesh_filter::work(root));
  bgmb_interval parameters[6]{};
  if (!bgmesh_filter::coordinates(root, task, parameters))
    return {key, BoundDecision::unresolved};
  const auto *source = controls.data() + root.offset;
  uint32_t result = 2;
  if (root.kind == 1)
    result = filter_curve(root.p, source, task, parameters);
  else
    switch (root.p) {
    case 0:
      result = filter_surface<0>(root.q, source, task, parameters);
      break;
    case 1:
      result = filter_surface<1>(root.q, source, task, parameters);
      break;
    case 2:
      result = filter_surface<2>(root.q, source, task, parameters);
      break;
    case 3:
      result = filter_surface<3>(root.q, source, task, parameters);
      break;
    default:
      break;
    }
  return {key, static_cast<BoundDecision>(result)};
}

bgmb_task pack_prepared_bound_task(BoundTaskKey key, uint32_t root, PreparedBoundFrame frame,
                                   std::span<const Rational> parameters,
                                   std::span<const Rational> linear, const Rational &tau) {
  const bool surface = frame.kind == 0;
  if (frame.kind > 1 || frame.domain.size() != (surface ? 4u : 2u) ||
      parameters.size() != (surface ? 6u : 2u) || linear.size() != (surface ? 9u : 6u) ||
      tau.sign() < 0)
    invalid("invalid prepared bound frame/task");
  auto &budget = tau.budget();
  for (const auto &value : frame.anchor)
    same_owner(value, budget);
  for (const auto &value : frame.domain)
    same_owner(value, budget);
  if (frame.domain[0].compare(frame.domain[1]) >= 0 ||
      (surface && frame.domain[2].compare(frame.domain[3]) >= 0))
    invalid("invalid prepared bound domain");
  bgmb_task result{};
  result.key = pack_key(key);
  result.root = root;
  for (std::size_t i = 0; i < parameters.size(); ++i) {
    same_owner(parameters[i], budget);
    const auto axis = surface ? i % 2 : 0;
    if (parameters[i].compare(frame.domain[axis * 2]) < 0 ||
        parameters[i].compare(frame.domain[axis * 2 + 1]) > 0)
      invalid("prepared bound task leaves original span");
    result.parameter[i] = enclose(parameters[i]);
  }
  if (!surface && parameters[0].compare(parameters[1]) >= 0)
    invalid("invalid prepared curve interval");
  for (std::size_t i = 0; i < linear.size(); ++i) {
    same_owner(linear[i], budget);
    result.linear_xyz[i] = enclose(linear[i].subtract(frame.anchor[i % 3]));
  }
  result.tolerance = enclose(tau);
  return result;
}
} // namespace bgmesh
