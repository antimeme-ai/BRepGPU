#include "adaptive_geometry.hpp"
#include "adaptive_packets.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <optional>

namespace bgmesh::adaptive {
namespace {
constexpr uint32_t none = UINT32_MAX;
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
uint32_t slot_size(std::size_t count) {
  if (count >= none)
    throw Error(ErrorCode::resource_exhausted, "packet index capacity exhausted");
  return static_cast<uint32_t>(count);
}
struct Side final {
  uint32_t leaf, ordinal, axis;
  PointSlot low, high;
  ExactRef constant;
  bool reverse;
};
struct Event final {
  uint32_t position, side;
  bool opening;
};
struct Sector final {
  PointSlot point;
  uint32_t leaf;
  uint32_t mask;
};
class Builder final {
  bgfd::Budget &budget_;
  const PatchSelection &selection_;
  std::pmr::vector<uint32_t> active_, demoted_;
  std::pmr::vector<Side> sides_;
  std::pmr::vector<PacketSide> uses_;
  std::pmr::vector<PacketLine> lines_;
  std::pmr::vector<PointSlot> line_points_;
  std::pmr::vector<PacketRim> rims_;
  PacketStats stats_;

  ExactView view(ExactRef ref) {
    budget_.charge(1);
    if (ref.table != selection_.values.key() || ref.index >= selection_.values.size())
      invalid("packet coordinate has a foreign exact table");
    return selection_.values.view(ref);
  }
  bool same_words(std::span<const uint32_t> a, std::span<const uint32_t> b) {
    if (a.size() != b.size())
      return false;
    budget_.charge(static_cast<uint64_t>(a.size()));
    return std::equal(a.begin(), a.end(), b.begin());
  }
  int magnitude(std::span<const uint32_t> a, std::span<const uint32_t> b) {
    if (a.size() != b.size())
      return a.size() < b.size() ? -1 : 1;
    for (std::size_t i = a.size(); i > 0; --i) {
      budget_.charge(1);
      if (a[i - 1] != b[i - 1])
        return a[i - 1] < b[i - 1] ? -1 : 1;
    }
    return 0;
  }
  std::optional<uint64_t> power(std::span<const uint32_t> words) {
    std::optional<uint64_t> exponent;
    for (std::size_t i = 0; i < words.size(); ++i) {
      budget_.charge(1);
      if (!words[i])
        continue;
      if (exponent || !std::has_single_bit(words[i]))
        return {};
      exponent = uint64_t{i} * 32 + static_cast<uint64_t>(std::countr_zero(words[i]));
    }
    return exponent;
  }
  struct ShiftedMagnitude final {
    std::span<const uint32_t> words;
    uint64_t shift;
  };
  static uint32_t shifted_word(ShiftedMagnitude value, uint64_t index) {
    const auto words = value.words;
    const auto shift = value.shift;
    const auto whole = shift / 32;
    if (index < whole)
      return 0;
    const auto at = index - whole;
    const auto bits = static_cast<uint32_t>(shift % 32);
    const auto low = at < words.size() ? words[static_cast<std::size_t>(at)] << bits : 0U;
    const auto high = bits && at && at - 1 < words.size()
                          ? words[static_cast<std::size_t>(at - 1)] >> (32 - bits)
                          : 0U;
    return low | high;
  }
  int compare(ExactRef left, ExactRef right) {
    budget_.charge(1);
    ++stats_.comparisons;
    if (left == right)
      return 0;
    const auto a = view(left), b = view(right);
    if (a.sign != b.sign)
      return a.sign < b.sign ? -1 : 1;
    if (!a.sign)
      return 0;
    if (same_words(a.denominator, b.denominator))
      return a.sign * magnitude(a.numerator, b.numerator);
    const auto ap = power(a.denominator), bp = power(b.denominator);
    if (ap && bp) {
      ++stats_.dyadic_comparisons;
      const auto words = std::max((uint64_t{a.numerator.size()} * 32 + *bp + 31) / 32,
                                  (uint64_t{b.numerator.size()} * 32 + *ap + 31) / 32);
      for (auto i = words; i > 0; --i) {
        budget_.charge(1);
        const auto aw = shifted_word({a.numerator, *bp}, i - 1),
                   bw = shifted_word({b.numerator, *ap}, i - 1);
        if (aw != bw)
          return a.sign * (aw < bw ? -1 : 1);
      }
      return 0;
    }
    ++stats_.rational_comparisons;
    const auto an = bgfd::Integer::from_words(budget_, a.sign, a.numerator),
               bn = bgfd::Integer::from_words(budget_, b.sign, b.numerator),
               ad = bgfd::Integer::from_words(budget_, 1, a.denominator),
               bd = bgfd::Integer::from_words(budget_, 1, b.denominator);
    return an.multiply(bd).compare(bn.multiply(ad));
  }
  ExactRef coordinate(PointSlot point, uint32_t axis) const {
    return selection_.points[point.value].chart_xy[axis];
  }
  int compare_point(PointSlot a, PointSlot b, uint32_t axis) {
    return compare(coordinate(a, axis), coordinate(b, axis));
  }
  void validate() {
    std::pmr::vector<uint32_t> points(&budget_);
    for (const auto leaf : active_) {
      budget_.charge(1);
      if (leaf >= selection_.leaves.size())
        invalid("packet leaf slot out of range");
      const auto &record = selection_.leaves[leaf];
      for (const auto point : record.corners) {
        budget_.charge(1);
        if (point.value >= selection_.points.size())
          invalid("packet corner slot out of range");
        for (const auto value : selection_.points[point.value].actual_xyz)
          if (!std::isfinite(value))
            invalid("packet corner XYZ is not finite");
        points.push_back(point.value);
      }
      const auto &c = record.corners;
      if (compare_point(c[0], c[1], 0) >= 0 || compare_point(c[0], c[2], 1) >= 0 ||
          compare_point(c[0], c[2], 0) || compare_point(c[1], c[3], 0) ||
          compare_point(c[0], c[1], 1) || compare_point(c[2], c[3], 1))
        invalid("packet corners do not form a positive exact rectangle");
      const std::array expected{coordinate(c[0], 0), coordinate(c[1], 0), coordinate(c[0], 1),
                                coordinate(c[2], 1)};
      for (std::size_t i = 0; i < expected.size(); ++i)
        if (compare(expected[i], record.box[i]))
          invalid("packet corners disagree with selected exact box");
    }
    std::sort(points.begin(), points.end(), [&](uint32_t a, uint32_t b) {
      budget_.charge(1);
      const auto x = compare_point({a}, {b}, 0);
      if (x)
        return x < 0;
      const auto y = compare_point({a}, {b}, 1);
      return y ? y < 0 : a < b;
    });
    for (std::size_t i = 1; i < points.size(); ++i) {
      budget_.charge(1);
      if (points[i] != points[i - 1] && !compare_point({points[i]}, {points[i - 1]}, 0) &&
          !compare_point({points[i]}, {points[i - 1]}, 1))
        invalid("common packet point lacks canonical PointPool identity");
    }
  }
  PointSlot side_point(const PacketSide &side, uint32_t at) const {
    const auto &line = lines_[side.line];
    const auto relative = side.reverse ? side.count - 1 - at : at;
    return line_points_[line.events.first + side.first + relative];
  }
  uint32_t find_event(const std::pmr::vector<PointSlot> &events, PointSlot point, uint32_t axis) {
    uint32_t low = 0, high = slot_size(events.size());
    while (low < high) {
      const auto mid = low + (high - low) / 2;
      if (compare_point(events[mid], point, axis) < 0)
        low = mid + 1;
      else
        high = mid;
    }
    if (low == events.size() || events[low] != point)
      invalid("packet side endpoint missing from common schedule");
    return low;
  }
  void build_lines() {
    sides_.clear();
    uses_.clear();
    lines_.clear();
    line_points_.clear();
    rims_.clear();
    for (const auto leaf : active_) {
      budget_.charge(1);
      const auto &c = selection_.leaves[leaf].corners;
      sides_.push_back({leaf, 0, 0, c[0], c[1], coordinate(c[0], 1), false});
      sides_.push_back({leaf, 1, 1, c[1], c[3], coordinate(c[1], 0), false});
      sides_.push_back({leaf, 2, 0, c[2], c[3], coordinate(c[2], 1), true});
      sides_.push_back({leaf, 3, 1, c[0], c[2], coordinate(c[0], 0), true});
    }
    slot_size(sides_.size());
    uses_.resize(sides_.size());
    std::pmr::vector<uint32_t> order(&budget_);
    order.reserve(sides_.size());
    for (uint32_t i = 0; i < sides_.size(); ++i)
      order.push_back(i);
    std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
      budget_.charge(1);
      const auto &left = sides_[a], &right = sides_[b];
      if (left.axis != right.axis)
        return left.axis < right.axis;
      const auto c = compare(left.constant, right.constant);
      if (c)
        return c < 0;
      const auto p = compare_point(left.low, right.low, left.axis);
      return p ? p < 0 : a < b;
    });
    for (std::size_t first = 0; first < order.size();) {
      std::size_t end = first + 1;
      const auto &base = sides_[order[first]];
      while (end < order.size() && sides_[order[end]].axis == base.axis &&
             !compare(sides_[order[end]].constant, base.constant))
        ++end;
      std::pmr::vector<PointSlot> endpoints(&budget_);
      for (auto i = first; i < end; ++i) {
        budget_.charge(1);
        endpoints.push_back(sides_[order[i]].low);
        endpoints.push_back(sides_[order[i]].high);
      }
      std::sort(endpoints.begin(), endpoints.end(), [&](PointSlot a, PointSlot b) {
        const auto value = compare_point(a, b, base.axis);
        return value ? value < 0 : a < b;
      });
      budget_.charge(static_cast<uint64_t>(endpoints.size()));
      endpoints.erase(std::unique(endpoints.begin(), endpoints.end()), endpoints.end());
      const auto line = slot_size(lines_.size());
      const auto point_first = line_points_.size();
      line_points_.insert(line_points_.end(), endpoints.begin(), endpoints.end());
      lines_.push_back({base.axis, base.constant, {point_first, endpoints.size()}});
      stats_.side_events += endpoints.size();
      std::pmr::vector<Event> events(&budget_);
      for (auto i = first; i < end; ++i) {
        const auto at = order[i];
        const auto &side = sides_[at];
        const auto lo = find_event(endpoints, side.low, base.axis),
                   hi = find_event(endpoints, side.high, base.axis);
        uses_[at] = {line, lo, hi - lo + 1, side.reverse};
        events.push_back({lo, at, true});
        events.push_back({hi, at, false});
      }
      std::sort(events.begin(), events.end(), [&](const Event &a, const Event &b) {
        budget_.charge(1);
        if (a.position != b.position)
          return a.position < b.position;
        if (a.opening != b.opening)
          return !a.opening;
        return a.side < b.side;
      });
      std::array<uint32_t, 2> incident{none, none};
      for (std::size_t i = 0; i < events.size();) {
        const auto position = events[i].position;
        auto next = i;
        while (next < events.size() && events[next].position == position) {
          budget_.charge(1);
          const auto &event = events[next++];
          const auto direction = static_cast<uint32_t>(sides_[event.side].reverse);
          if (event.opening) {
            if (incident[direction] != none)
              invalid("same-direction packet sides overlap");
            incident[direction] = event.side;
          } else {
            if (incident[direction] != event.side)
              invalid("packet side close does not match its open interval");
            incident[direction] = none;
          }
        }
        if (next < events.size() && (incident[0] == none) != (incident[1] == none)) {
          const auto direction = incident[0] == none ? 1U : 0U;
          const auto &side = sides_[incident[direction]];
          const auto a = endpoints[position], b = endpoints[events[next].position];
          rims_.push_back({side.leaf, direction ? std::array{b, a} : std::array{a, b}});
        }
        i = next;
      }
      if (incident[0] != none || incident[1] != none)
        invalid("packet line has an unclosed side");
      first = end;
    }
  }
  bool demote_pinches() {
    std::pmr::vector<Sector> sectors(&budget_);
    constexpr std::array<uint32_t, 4> middle{9, 12, 6, 3};
    constexpr std::array<uint32_t, 4> begin{1, 8, 4, 2};
    constexpr std::array<uint32_t, 4> finish{8, 4, 2, 1};
    for (std::size_t i = 0; i < sides_.size(); ++i) {
      const auto &use = uses_[i];
      for (uint32_t j = 0; j < use.count; ++j) {
        budget_.charge(1);
        const auto mask = j == 0               ? begin[sides_[i].ordinal]
                          : j + 1 == use.count ? finish[sides_[i].ordinal]
                                               : middle[sides_[i].ordinal];
        sectors.push_back({side_point(use, j), sides_[i].leaf, mask});
      }
    }
    std::sort(sectors.begin(), sectors.end(), [&](const Sector &a, const Sector &b) {
      budget_.charge(1);
      return a.point != b.point ? a.point < b.point : a.leaf < b.leaf;
    });
    std::pmr::vector<uint32_t> marked(&budget_);
    for (std::size_t i = 0; i < sectors.size();) {
      auto end = i;
      std::array<uint32_t, 4> owners{none, none, none, none};
      uint32_t occupied = 0;
      while (end < sectors.size() && sectors[end].point == sectors[i].point) {
        const auto &sector = sectors[end++];
        budget_.charge(1);
        occupied |= sector.mask;
        for (uint32_t q = 0; q < 4; ++q)
          if (sector.mask & (1U << q)) {
            if (owners[q] != none && owners[q] != sector.leaf)
              invalid("packet rectangles overlap at a side event");
            owners[q] = sector.leaf;
          }
      }
      if (occupied == 5 || occupied == 10)
        for (auto j = i; j < end; ++j)
          marked.push_back(sectors[j].leaf);
      i = end;
    }
    if (marked.empty())
      return false;
    std::sort(marked.begin(), marked.end(), [&](uint32_t a, uint32_t b) {
      budget_.charge(1);
      return a < b;
    });
    budget_.charge(static_cast<uint64_t>(marked.size()));
    marked.erase(std::unique(marked.begin(), marked.end()), marked.end());
    std::pmr::vector<uint32_t> retained(&budget_);
    budget_.charge(static_cast<uint64_t>(active_.size() + marked.size()));
    std::set_difference(active_.begin(), active_.end(), marked.begin(), marked.end(),
                        std::back_inserter(retained));
    demoted_.insert(demoted_.end(), marked.begin(), marked.end());
    active_ = std::move(retained);
    ++stats_.demotion_rounds;
    stats_.demoted_packets += marked.size();
    return true;
  }
  double score(PointSlot a, PointSlot b) {
    budget_.charge(1);
    double result = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto delta =
          selection_.points[a.value].actual_xyz[axis] - selection_.points[b.value].actual_xyz[axis];
      result = result + delta * delta;
    }
    return result;
  }
  void zip(PacketMeshData &out, uint32_t leaf, const PacketSide &a_side, const PacketSide &b_side) {
    uint32_t ai = 0, bi = 0;
    const auto a_last = a_side.count - 1, b_last = b_side.count - 1;
    while (ai + 1 < a_last || bi + 1 < b_last) {
      budget_.charge(1);
      const auto a = side_point(a_side, ai), b = side_point(b_side, bi);
      bool take_a = ai + 1 < a_last;
      if (take_a && bi + 1 < b_last) {
        const auto next_a = side_point(a_side, ai + 1), next_b = side_point(b_side, bi + 1);
        const auto as = score(next_a, b), bs = score(a, next_b);
        take_a = as < bs || (as == bs && next_a < next_b);
      }
      if (take_a) {
        const auto next = side_point(a_side, ++ai);
        out.triangles.push_back({{a, next, b}, leaf});
      } else {
        const auto next = side_point(b_side, ++bi);
        out.triangles.push_back({{a, next, b}, leaf});
      }
    }
    const auto common = side_point(a_side, a_last);
    if (common != side_point(b_side, b_last))
      invalid("packet zipper chains do not meet at the common corner");
    out.triangles.push_back({{side_point(a_side, ai), common, side_point(b_side, bi)}, leaf});
  }

public:
  Builder(bgfd::Budget &budget, const PatchSelection &selection, std::span<const uint32_t> enabled)
      : budget_(budget), selection_(selection), active_(&budget), demoted_(&budget),
        sides_(&budget), uses_(&budget), lines_(&budget), line_points_(&budget), rims_(&budget) {
    budget_.charge(1);
    if (enabled.size() >= none / 4)
      throw Error(ErrorCode::resource_exhausted, "packet side capacity exhausted");
    active_.assign(enabled.begin(), enabled.end());
    std::sort(active_.begin(), active_.end(), [&](uint32_t a, uint32_t b) {
      budget_.charge(1);
      return a < b;
    });
    for (std::size_t i = 1; i < active_.size(); ++i) {
      budget_.charge(1);
      if (active_[i] == active_[i - 1])
        invalid("duplicate enabled packet leaf");
    }
    validate();
  }
  PacketMesh run(const std::shared_ptr<RunResource> &resource,
                 const std::shared_ptr<const PatchSelection> &selection) {
    do {
      build_lines();
    } while (demote_pinches());
    uint64_t triangles = 0;
    for (std::size_t i = 0; i < active_.size(); ++i) {
      budget_.charge(1);
      uint64_t samples = 0;
      for (uint32_t side = 0; side < 4; ++side)
        samples += uses_[i * 4 + side].count - 1;
      triangles += samples - 2;
      if (triangles >= none)
        throw Error(ErrorCode::resource_exhausted, "packet triangle capacity exhausted");
    }
    auto out = std::allocate_shared<PacketMeshData>(RunAllocator<PacketMeshData>(resource),
                                                    resource, selection);
    out->lines.reserve(lines_.size());
    out->line_points.reserve(line_points_.size());
    out->sides.reserve(uses_.size());
    out->packets.reserve(active_.size());
    out->triangles.reserve(static_cast<std::size_t>(triangles));
    out->rims.reserve(rims_.size());
    out->demoted_leaf_slots.reserve(demoted_.size());
    budget_.charge(static_cast<uint64_t>(lines_.size() + line_points_.size() + uses_.size() +
                                         rims_.size() + demoted_.size()));
    out->lines.assign(lines_.begin(), lines_.end());
    out->line_points.assign(line_points_.begin(), line_points_.end());
    out->sides.assign(uses_.begin(), uses_.end());
    out->rims.assign(rims_.begin(), rims_.end());
    std::sort(demoted_.begin(), demoted_.end(), [&](uint32_t a, uint32_t b) {
      budget_.charge(1);
      return a < b;
    });
    out->demoted_leaf_slots.assign(demoted_.begin(), demoted_.end());
    for (std::size_t i = 0; i < active_.size(); ++i) {
      const auto first = out->triangles.size();
      const auto &side = uses_[i * 4];
      auto reverse_right = uses_[i * 4 + 1], reverse_left = uses_[i * 4 + 3];
      reverse_right.reverse = !reverse_right.reverse;
      reverse_left.reverse = !reverse_left.reverse;
      zip(*out, active_[i], side, reverse_right);
      zip(*out, active_[i], uses_[i * 4 + 2], reverse_left);
      out->packets.push_back({active_[i], {i * 4, 4}, {first, out->triangles.size() - first}});
    }
    out->stats = stats_;
    return out;
  }
};
} // namespace
PacketMesh build_packet_mesh(bgfd::Budget &scratch, const std::shared_ptr<RunResource> &resource,
                             const std::shared_ptr<const PatchSelection> &selection,
                             std::span<const uint32_t> enabled) {
  if (!selection || !resource || selection->resource != resource ||
      selection->values.resource() != resource)
    invalid("packet selection/result resource mismatch");
  admit_numeric_environment();
  return Builder(scratch, *selection, enabled).run(resource, selection);
}
} // namespace bgmesh::adaptive
