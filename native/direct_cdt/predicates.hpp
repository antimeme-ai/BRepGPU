#ifndef BREPGPU_DIRECT_CDT_PREDICATES_HPP
#define BREPGPU_DIRECT_CDT_PREDICATES_HPP
#include "../face_domain/exact.hpp"
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <vector>

namespace bgcdt {
using Slot = uint32_t;
enum class Axis { x, y };
struct PredicateStats final {
  uint64_t axis_compares = 0, orientations = 0, incircles = 0, line_distance_compares = 0;
  uint64_t scalar_signs = 0, interval_signs = 0, exact_signs = 0, exact_zeros = 0;
  uint64_t floating_disabled = 0, enclosure_values = 0, exact_lifts = 0;
};
struct Interval final {
  double lo, hi;
};
class Predicates final {
  struct Point final {
    bgfd::Integer x, y, lift;
    Interval ix{}, iy{};
    double ax = 0, ay = 0;
    Point(bgfd::Integer px, bgfd::Integer py, bgfd::Budget &budget)
        : x(std::move(px)), y(std::move(py)), lift(budget) {}
  };
  bgfd::Budget *budget_;
  bgfd::Integer wx_, wy_;
  std::pmr::vector<Point> points_;
  Interval iwx_{}, iwy_{};
  double awx_ = 0, awy_ = 0;
  bool filtering_;
  PredicateStats stats_;
  [[nodiscard]] const Point &point(Slot slot) const;
  [[nodiscard]] bgfd::Integer orientation(Slot a, Slot b, Slot c) const;
  [[nodiscard]] int exact_sign(const bgfd::Integer &value);
  [[nodiscard]] bool filters_enabled();
  [[nodiscard]] Interval enclosure(const bgfd::Integer &value, int64_t power);

public:
  explicit Predicates(bgfd::Budget &budget, const bgfd_input &metric_input, bool filtering = true);
  Predicates(const Predicates &) = delete;
  Predicates &operator=(const Predicates &) = delete;
  [[nodiscard]] std::size_t size() const noexcept { return points_.size(); }
  [[nodiscard]] int compare_axis(Slot a, Slot b, Axis axis);
  [[nodiscard]] int orient(Slot a, Slot b, Slot c);
  [[nodiscard]] int incircle(Slot a, Slot b, Slot c, Slot d);
  [[nodiscard]] int compare_line_distance(Slot s0, Slot s1, Slot a, Slot b);
  [[nodiscard]] const PredicateStats &stats() const noexcept { return stats_; }
};
} // namespace bgcdt
#endif
