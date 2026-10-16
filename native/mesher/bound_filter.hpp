#ifndef BREPGPU_MESHER_BOUND_FILTER_HPP
#define BREPGPU_MESHER_BOUND_FILTER_HPP
#include "interval.hpp"
#if defined(__CUDACC__)
#define BGMB_HD __host__ __device__
#else
#define BGMB_HD
#endif
namespace bgmesh_filter {
using namespace bgmesh_interval;
// Conservative interval-operation reservation, not elapsed-time units.
// Validated supported degrees bound every product well within uint64.
BGMB_HD inline uint64_t work(const bgmb_root &root) {
  const uint64_t p = root.p, q = root.q;
  if (root.kind == 1)
    return 64 * (4 * (p + 1) * (p + 2) + 24 * (p + 2) + 32);
  const auto d = p + q, count = (d + 1) * (d + 2) / 2, parts = (p + 1) * (p + 2) / 2;
  return 64 * (4 * count * parts * (p + 1) * (q + 1) * (d + 1) + 36 * (d + 2) * (d + 3) / 2 + 32);
}
struct Binomial {
  uint32_t n, k;
};
BGMB_HD inline uint32_t choose(Binomial input) {
  const auto n = input.n, k = input.k;
  uint32_t result = 1;
  for (uint32_t i = 1; i <= k; ++i)
    result = result * (n + 1 - i) / i;
  return result;
}
BGMB_HD inline uint32_t multinomial(uint32_t n, uint32_t i, uint32_t j) {
  return choose({n, i}) * choose({n - i, j});
}
BGMB_HD inline uint32_t index(uint32_t d, uint32_t i, uint32_t j) {
  return i * (d + 1) - i * (i == 0 ? 0 : i - 1) / 2 + j;
}
BGMB_HD inline bool coordinates(const bgmb_root &root, const bgmb_task &task, Interval *out) {
  const uint32_t axes = root.kind == 0 ? 2 : 1, points = root.kind == 0 ? 3 : 2;
  for (uint32_t axis = 0; axis < axes; ++axis) {
    const auto origin = root.bounds[axis * 2];
    const auto width = subtract(root.bounds[axis * 2 + 1], origin);
    for (uint32_t point = 0; point < points; ++point) {
      const auto at = point * axes + axis;
      out[at] = divide_positive(subtract(task.parameter[at], origin), width);
      if (!valid(out[at]))
        return false;
    }
  }
  return valid(task.tolerance) && task.tolerance.lower >= 0;
}
struct Coefficient {
  uint32_t i, j, axis;
};
template <unsigned P, unsigned Q>
BGMB_HD Interval surface_coefficient(const Interval *source, Coefficient target,
                                     const Interval *uv) {
  const auto i = target.i, j = target.j, axis = target.axis;
  constexpr uint32_t degree = P + Q;
  const uint32_t alpha[3]{degree - i - j, i, j};
  Interval result{0, 0};
  for (uint32_t bi = 0; bi <= P; ++bi)
    for (uint32_t bj = 0; bj <= P - bi; ++bj) {
      const uint32_t beta[3]{P - bi - bj, bi, bj};
      if (beta[0] > alpha[0] || beta[1] > alpha[1] || beta[2] > alpha[2])
        continue;
      const uint32_t gamma[3]{alpha[0] - beta[0], alpha[1] - beta[1], alpha[2] - beta[2]};
      Interval work[(P + 1) * (Q + 1)];
      for (uint32_t a = 0; a <= P; ++a)
        for (uint32_t b = 0; b <= Q; ++b)
          work[a * (Q + 1) + b] = source[4 * (a * (Q + 1) + b) + axis];
      for (uint32_t step = 0; step != P; ++step) {
        const auto u = uv[(step < beta[0] ? 0 : step < beta[0] + beta[1] ? 1 : 2) * 2];
        for (uint32_t a = 0; a < P - step; ++a)
          for (uint32_t b = 0; b <= Q; ++b)
            work[a * (Q + 1) + b] = lerp(work[a * (Q + 1) + b], work[(a + 1) * (Q + 1) + b], u);
      }
      for (uint32_t step = 0; step != Q; ++step) {
        const auto v = uv[(step < gamma[0] ? 0 : step < gamma[0] + gamma[1] ? 1 : 2) * 2 + 1];
        for (uint32_t b = 0; b < Q - step; ++b)
          work[b] = lerp(work[b], work[b + 1], v);
      }
      const auto factor = ratio(multinomial(P, bi, bj) * multinomial(Q, gamma[1], gamma[2]),
                                multinomial(degree, i, j));
      result = add(result, multiply(work[0], factor));
    }
  return result;
}
template <unsigned N>
BGMB_HD Interval curve_coefficient(const Interval *source, Coefficient target,
                                   const Interval *parameter) {
  const auto i = target.i, axis = target.axis;
  Interval work[N + 1];
  for (uint32_t j = 0; j <= N; ++j)
    work[j] = source[j * 4 + axis];
  for (uint32_t step = 0; step != N; ++step) {
    const auto t = parameter[step < N - i ? 0 : 1];
    for (uint32_t j = 0; j < N - step; ++j)
      work[j] = lerp(work[j], work[j + 1], t);
  }
  return work[0];
}
BGMB_HD inline Interval limit(const Interval *net, uint32_t count, Interval tolerance) {
  Interval w = net[3];
  if (!valid(w))
    return invalid();
  for (uint32_t i = 1; i < count; ++i) {
    const auto next = net[i * 4 + 3];
    if (!valid(next))
      return invalid();
    w = {minimum(w.lower, next.lower), minimum(w.upper, next.upper)};
  }
  if (w.lower <= 0)
    return invalid();
  return multiply(square(tolerance), square(w));
}
BGMB_HD inline uint32_t classify(Interval squared, Interval threshold) {
  const auto difference = subtract(squared, threshold);
  if (!valid(difference))
    return 2;
  if (difference.upper <= 0)
    return 0;
  if (difference.lower > 0)
    return 1;
  return 2;
}
BGMB_HD inline uint32_t triangle_decision(const Interval *net, uint32_t d, const bgmb_task &task,
                                          Interval threshold, uint32_t i, uint32_t j) {
  const auto degree = d + 1;
  const uint32_t beta[3]{degree - i - j, i, j};
  Interval squared{0, 0};
  for (uint32_t axis = 0; axis < 3; ++axis) {
    Interval residual{0, 0};
    for (uint32_t k = 0; k < 3; ++k) {
      if (beta[k] == 0)
        continue;
      const auto at =
          index(d, i - static_cast<uint32_t>(k == 1), j - static_cast<uint32_t>(k == 2)) * 4;
      const auto term =
          subtract(net[at + axis], multiply(net[at + 3], task.linear_xyz[k * 3 + axis]));
      residual = add(residual, multiply(term, ratio(beta[k], degree)));
    }
    squared = add(squared, square(residual));
  }
  return classify(squared, threshold);
}
BGMB_HD inline uint32_t curve_decision(const Interval *net, uint32_t n, const bgmb_task &task,
                                       Interval threshold, uint32_t i) {
  const auto degree = n + 1;
  Interval squared{0, 0};
  for (uint32_t axis = 0; axis < 3; ++axis) {
    Interval residual{0, 0};
    if (i < degree)
      residual =
          multiply(subtract(net[i * 4 + axis], multiply(net[i * 4 + 3], task.linear_xyz[axis])),
                   ratio(degree - i, degree));
    if (i != 0)
      residual = add(residual,
                     multiply(subtract(net[(i - 1) * 4 + axis],
                                       multiply(net[(i - 1) * 4 + 3], task.linear_xyz[3 + axis])),
                              ratio(i, degree)));
    squared = add(squared, square(residual));
  }
  return classify(squared, threshold);
}
} // namespace bgmesh_filter
#undef BGMB_HD
#endif
