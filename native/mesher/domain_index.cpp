#include "domain_index.hpp"
#include <algorithm>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <limits>
#include <numeric>
#if defined(__SSE2__)
#include <xmmintrin.h>
#endif
namespace bgmesh {
namespace {
constexpr uint32_t absent = UINT32_MAX;
[[noreturn]] void internal(const char *m) { throw bgfd::Failure(BGFD_INTERNAL_ERROR, m); }
using Box = DomainBox;
using Segment = DomainSegment;
Box joined(Box a, const Box &b) {
  for (uint32_t j = 0; j < 2; ++j) {
    a.lo[j] = std::min(a.lo[j], b.lo[j]);
    a.hi[j] = std::max(a.hi[j], b.hi[j]);
  }
  return a;
}
bool overlaps(const Box &a, const Box &b) {
  return !(a.hi[0] < b.lo[0] || b.hi[0] < a.lo[0] || a.hi[1] < b.lo[1] || b.hi[1] < a.lo[1]);
}
enum class IndexAxis { x, y };
struct ScalarRange {
  double lo, hi;
};
ScalarRange difference(ScalarRange a, ScalarRange b) {
  return {std::nextafter(a.lo - b.hi, -INFINITY), std::nextafter(a.hi - b.lo, INFINITY)};
}
ScalarRange product(ScalarRange a, ScalarRange b) {
  const std::array<double, 4> p{a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
  if (!std::ranges::all_of(p, [](double x) { return std::isfinite(x); }))
    return {-INFINITY, INFINITY};
  return {std::nextafter(*std::min_element(p.begin(), p.end()), -INFINITY),
          std::nextafter(*std::max_element(p.begin(), p.end()), INFINITY)};
}
struct PointTable {
  bgfd::Budget &budget;
  std::span<const DomainPointRef> points;
  std::pmr::vector<std::array<ScalarRange, 2>> enclosures;
  DomainPointRef query{};
  bool filtering = false;
  PointTable(bgfd::Budget &b, std::span<const DomainPointRef> p)
      : budget(b), points(p), enclosures(&b) {
    filtering = FLT_EVAL_METHOD == 0 && std::fegetround() == FE_TONEAREST;
#if defined(__SSE2__)
    filtering = filtering && (_mm_getcsr() & ((1u << 15) | (1u << 6))) == 0;
#endif
    for (const auto &point : p) {
      std::array<ScalarRange, 2> row{};
      for (uint32_t j = 0; j < 2; ++j) {
        if (!point[j] || &point[j]->budget() != &b)
          throw Error(ErrorCode::invalid_source, "domain coordinate owner");
        auto q = point[j]->bounds();
        row[j] = {q.lower, q.upper};
      }
      enclosures.push_back(row);
    }
  }
  size_t size() const { return points.size(); }
  const Rational &coordinate(SiteSlot p, IndexAxis a) const {
    const auto j = a == IndexAxis::x ? 0u : 1u;
    return *(p.value == points.size() ? query[j] : points[p.value][j]);
  }
  ScalarRange enclosure(SiteSlot p, uint32_t j) const {
    if (p.value < points.size())
      return enclosures[p.value][j];
    auto r = query[j]->bounds();
    return {r.lower, r.upper};
  }
  int compare_axis(SiteSlot a, SiteSlot b, IndexAxis axis) const {
    return coordinate(a, axis).compare(coordinate(b, axis));
  }
  int orient(std::array<SiteSlot, 3> p) const {
    budget.charge(1);
    if (filtering) {
      auto x = difference(enclosure(p[1], 0), enclosure(p[0], 0)),
           y = difference(enclosure(p[1], 1), enclosure(p[0], 1));
      auto u = difference(enclosure(p[2], 0), enclosure(p[0], 0)),
           v = difference(enclosure(p[2], 1), enclosure(p[0], 1));
      auto d = difference(product(x, v), product(y, u));
      if (d.lo > 0)
        return 1;
      if (d.hi < 0)
        return -1;
    }
    return coordinate(p[1], IndexAxis::x)
        .subtract(coordinate(p[0], IndexAxis::x))
        .multiply(coordinate(p[2], IndexAxis::y).subtract(coordinate(p[0], IndexAxis::y)))
        .subtract(
            coordinate(p[1], IndexAxis::y)
                .subtract(coordinate(p[0], IndexAxis::y))
                .multiply(coordinate(p[2], IndexAxis::x).subtract(coordinate(p[0], IndexAxis::x))))
        .sign();
  }
};
template <class T> void reserve_more(std::pmr::vector<T> &v, std::size_t count) {
  if (count > v.max_size() - v.size())
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "domain capacity exhausted");
  const auto need = v.size() + count;
  if (need > v.capacity())
    v.reserve(std::max(need, v.capacity() <= v.max_size() / 2 ? v.capacity() * 2 : v.max_size()));
}
struct Event final {
  SiteSlot y;
  int64_t delta = 0, prefix = 0;
};
struct Node final {
  Box box;
  uint32_t left = absent, right = absent;
  uint64_t first = 0, count = 0, event_first = 0, event_count = 0;
};
} // namespace
struct DomainSegmentIndex::Impl final {
  bgfd::Budget &budget;
  PointTable mesh;
  DomainIndexStats &stats;
  std::span<const Segment> segments;
  std::pmr::vector<uint32_t> order;
  std::pmr::vector<Node> nodes;
  std::pmr::vector<Event> events;
  std::pmr::vector<Box> point_boxes;
  int ycompare(SiteSlot a, SiteSlot b) const { return mesh.compare_axis(a, b, IndexAxis::y); }
  void append_event(Event event, int64_t &prefix) {
    budget.charge(1);
    if (event.delta == 0)
      return;
    prefix += event.delta;
    event.prefix = prefix;
    events.push_back(event);
  }
  void build_events(uint32_t id) {
    auto &node = nodes[id];
    node.event_first = events.size();
    int64_t prefix = 0;
    if (node.left == absent) {
      std::array<Event, 16> scratch{};
      std::size_t count = 0;
      for (uint64_t i = 0; i < node.count; ++i) {
        budget.charge(1);
        const auto &s = segments[order[static_cast<std::size_t>(node.first + i)]];
        const int direction = ycompare(s.a, s.b);
        if (direction == 0)
          continue;
        const auto lo = direction < 0 ? s.a : s.b, hi = direction < 0 ? s.b : s.a;
        const int64_t sign = direction < 0 ? 1 : -1;
        scratch[count++] = {lo, sign, 0};
        scratch[count++] = {hi, -sign, 0};
      }
      std::sort(scratch.begin(), scratch.begin() + static_cast<std::ptrdiff_t>(count),
                [&](const auto &a, const auto &b) {
                  budget.charge(1);
                  const int c = ycompare(a.y, b.y);
                  return c < 0 || (c == 0 && a.y < b.y);
                });
      reserve_more(events, count);
      for (std::size_t i = 0; i < count;) {
        Event event = scratch[i++];
        while (i < count && ycompare(event.y, scratch[i].y) == 0) {
          event.delta += scratch[i++].delta;
        }
        append_event(event, prefix);
      }
    } else {
      const auto left = nodes[node.left], right = nodes[node.right];
      uint64_t i = 0, j = 0;
      reserve_more(events, static_cast<std::size_t>(left.event_count + right.event_count));
      while (i < left.event_count || j < right.event_count) {
        budget.charge(1);
        Event event{};
        if (i == left.event_count)
          event = events[static_cast<std::size_t>(right.event_first + j++)];
        else if (j == right.event_count)
          event = events[static_cast<std::size_t>(left.event_first + i++)];
        else {
          const auto a = events[static_cast<std::size_t>(left.event_first + i)],
                     b = events[static_cast<std::size_t>(right.event_first + j)];
          const int c = ycompare(a.y, b.y);
          if (c < 0) {
            event = a;
            ++i;
          } else if (c > 0) {
            event = b;
            ++j;
          } else {
            event = a;
            event.delta += b.delta;
            event.y = std::min(a.y, b.y);
            ++i;
            ++j;
          }
        }
        append_event(event, prefix);
      }
    }
    node.event_count = events.size() - node.event_first;
    if (prefix != 0)
      internal("domain crossing events do not close");
  }
  uint32_t build(uint64_t first, uint64_t count) {
    budget.charge(count);
    if (nodes.size() >= absent)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "domain BVH slot exhausted");
    const auto id = static_cast<uint32_t>(nodes.size());
    nodes.push_back({});
    Box box = segments[order[static_cast<std::size_t>(first)]].box;
    for (uint64_t i = 1; i < count; ++i)
      box = joined(box, segments[order[static_cast<std::size_t>(first + i)]].box);
    nodes[id].box = box;
    nodes[id].first = first;
    nodes[id].count = count;
    if (count > 8) {
      const auto axis =
          box.hi[1] - box.lo[1] > box.hi[0] - box.lo[0] ? std::size_t{1} : std::size_t{0};
      auto begin = order.begin() + static_cast<std::ptrdiff_t>(first),
           middle = begin + static_cast<std::ptrdiff_t>(count / 2),
           end = begin + static_cast<std::ptrdiff_t>(count);
      std::nth_element(begin, middle, end, [&](uint32_t a, uint32_t b) {
        budget.charge(1);
        const auto &x = segments[a].box;
        const auto &y = segments[b].box;
        const double cx = std::midpoint(x.lo[axis], x.hi[axis]),
                     cy = std::midpoint(y.lo[axis], y.hi[axis]);
        return cx < cy || (cx == cy && a < b);
      });
      const auto left = build(first, count / 2),
                 right = build(first + count / 2, count - count / 2);
      nodes[id].left = left;
      nodes[id].right = right;
    }
    build_events(id);
    return id;
  }
  bool between(std::array<SiteSlot, 2> endpoints, SiteSlot p) const {
    const auto a = endpoints[0], b = endpoints[1];
    for (const auto axis : {IndexAxis::x, IndexAxis::y}) {
      const int ca = mesh.compare_axis(p, a, axis), cb = mesh.compare_axis(p, b, axis);
      if ((ca < 0 && cb < 0) || (ca > 0 && cb > 0))
        return false;
    }
    return true;
  }
  bool bad_pair(const Segment &a, const Segment &b) const {
    std::optional<SiteSlot> common;
    if (a.a == b.a || a.a == b.b)
      common = a.a;
    else if (a.b == b.a || a.b == b.b)
      common = a.b;
    if (common) {
      const auto p = a.a == *common ? a.b : a.a, q = b.a == *common ? b.b : b.a;
      if (mesh.orient({*common, p, q}) != 0)
        return false;
      const auto axis =
          mesh.compare_axis(p, *common, IndexAxis::x) != 0 ? IndexAxis::x : IndexAxis::y;
      return mesh.compare_axis(p, *common, axis) == mesh.compare_axis(q, *common, axis);
    }
    const int p = mesh.orient({a.a, a.b, b.a}), q = mesh.orient({a.a, a.b, b.b});
    const int r = mesh.orient({b.a, b.b, a.a}), s = mesh.orient({b.a, b.b, a.b});
    return (p * q < 0 && r * s < 0) || (p == 0 && between({a.a, a.b}, b.a)) ||
           (q == 0 && between({a.a, a.b}, b.b)) || (r == 0 && between({b.a, b.b}, a.a)) ||
           (s == 0 && between({b.a, b.b}, a.b));
  }
  int64_t ray_segment(const Segment &s, SiteSlot p) const {
    const int a = mesh.compare_axis(s.a, p, IndexAxis::y),
              b = mesh.compare_axis(s.b, p, IndexAxis::y);
    if (a <= 0 && b > 0)
      return mesh.orient({s.a, s.b, p}) > 0 ? 1 : 0;
    if (b <= 0 && a > 0)
      return mesh.orient({s.a, s.b, p}) < 0 ? -1 : 0;
    return 0;
  }
  int64_t prefix(const Node &node, SiteSlot p) const {
    uint64_t lo = 0, hi = node.event_count;
    while (lo < hi) {
      budget.charge(1);
      const auto mid = lo + (hi - lo) / 2;
      if (ycompare(events[static_cast<std::size_t>(node.event_first + mid)].y, p) <= 0)
        lo = mid + 1;
      else
        hi = mid;
    }
    return lo == 0 ? 0 : events[static_cast<std::size_t>(node.event_first + lo - 1)].prefix;
  }

public:
  Impl(bgfd::Budget &b, std::span<const DomainPointRef> m, DomainIndexStats &s,
       std::span<const Segment> input)
      : budget(b), mesh(b, m), stats(s), segments(input), order(&b), nodes(&b), events(&b),
        point_boxes(&b) {
    order.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
      budget.charge(1);
      order.push_back(static_cast<uint32_t>(i));
    }
    point_boxes.reserve(static_cast<std::size_t>(mesh.size()));
    for (uint32_t i = 0; i < mesh.size(); ++i) {
      budget.charge(1);
      Box box;
      for (std::size_t axis = 0; axis < 2; ++axis) {
        const auto r =
            mesh.coordinate(SiteSlot{i}, axis == 0 ? IndexAxis::x : IndexAxis::y).bounds();
        if (!std::isfinite(r.lower) || !std::isfinite(r.upper))
          throw Error(ErrorCode::unsupported, "working chart requires finite coordinate enclosure");
        box.lo[axis] = r.lower;
        box.hi[axis] = r.upper;
      }
      point_boxes.push_back(box);
    }
    point_boxes.push_back({});
    nodes.reserve(input.size() * 2);
    if (!input.empty())
      (void)build(0, input.size());
    stats.event_entries = events.size();
  }
  std::optional<std::array<uint32_t, 2>> crossing() {
    std::pmr::vector<uint32_t> stack(&budget);
    for (uint32_t i = 0; i < segments.size(); ++i) {
      budget.charge(1);
      stack.clear();
      stack.push_back(0);
      while (!stack.empty()) {
        budget.charge(1);
        ++stats.bvh_visits;
        const auto id = stack.back();
        stack.pop_back();
        const auto &node = nodes[id];
        if (!overlaps(node.box, segments[i].box))
          continue;
        if (node.left != absent) {
          stack.push_back(node.right);
          stack.push_back(node.left);
          continue;
        }
        for (uint64_t at = 0; at < node.count; ++at) {
          budget.charge(1);
          const auto j = order[static_cast<std::size_t>(node.first + at)];
          if (j <= i || !overlaps(segments[i].box, segments[j].box))
            continue;
          ++stats.segment_candidates;
          if (bad_pair(segments[i], segments[j]))
            return std::array{i, j};
        }
      }
    }
    return std::nullopt;
  }
  int64_t winding_except(SiteSlot p, std::span<const Segment> own) {
    ++stats.ray_queries;
    const auto &point = point_boxes[p.value];
    int64_t total = 0;
    std::pmr::vector<uint32_t> stack(&budget);
    stack.push_back(0);
    while (!stack.empty()) {
      budget.charge(1);
      ++stats.bvh_visits;
      const auto id = stack.back();
      stack.pop_back();
      const auto &node = nodes[id];
      if (node.box.hi[0] < point.lo[0] || node.box.hi[1] < point.lo[1] ||
          node.box.lo[1] > point.hi[1])
        continue;
      if (node.box.lo[0] > point.hi[0]) {
        ++stats.winding_aggregates;
        total += prefix(node, p);
        continue;
      }
      if (node.left != absent) {
        stack.push_back(node.right);
        stack.push_back(node.left);
        continue;
      }
      for (uint64_t at = 0; at < node.count; ++at) {
        budget.charge(1);
        ++stats.ray_candidates;
        total += ray_segment(segments[order[static_cast<std::size_t>(node.first + at)]], p);
      }
    }
    for (const auto &s : own) {
      budget.charge(1);
      total -= ray_segment(s, p);
    }
    return total;
  }

  SiteSlot set_query(DomainPointRef point) {
    mesh.query = point;
    point_boxes.back() = domain_point_box(point);
    return SiteSlot{static_cast<uint32_t>(mesh.size())};
  }
  bool contact(DomainPointRef point) {
    const auto p = set_query(point);
    const auto box = point_boxes.back();
    std::pmr::vector<uint32_t> stack(&budget);
    if (nodes.empty())
      return false;
    stack.push_back(0);
    while (!stack.empty()) {
      budget.charge(1);
      auto id = stack.back();
      stack.pop_back();
      const auto &node = nodes[id];
      if (!overlaps(node.box, box))
        continue;
      if (node.left != absent) {
        stack.push_back(node.left);
        stack.push_back(node.right);
        continue;
      }
      for (uint64_t i = 0; i < node.count; ++i) {
        const auto &s = segments[order[node.first + i]];
        ++stats.segment_candidates;
        if (mesh.orient({s.a, s.b, p}) == 0 && between({s.a, s.b}, p))
          return true;
      }
    }
    return false;
  }
  bool rectangle(std::span<const Rational, 4> box) {
    const std::array<DomainPointRef, 4> corners{
        {{&box[0], &box[2]}, {&box[1], &box[2]}, {&box[1], &box[3]}, {&box[0], &box[3]}}};
    const auto querybox = domain_segment_box(corners[0], corners[2]);
    std::pmr::vector<uint32_t> stack(&budget);
    if (nodes.empty())
      return false;
    stack.push_back(0);
    while (!stack.empty()) {
      budget.charge(1);
      auto id = stack.back();
      stack.pop_back();
      const auto &node = nodes[id];
      if (!overlaps(node.box, querybox))
        continue;
      if (node.left != absent) {
        stack.push_back(node.left);
        stack.push_back(node.right);
        continue;
      }
      for (uint64_t i = 0; i < node.count; ++i) {
        const auto &s = segments[order[node.first + i]];
        if (!overlaps(s.box, querybox))
          continue;
        ++stats.segment_candidates;
        bool separated = false;
        for (uint32_t j = 0; j < 2; ++j) {
          const auto axis = j == 0 ? IndexAxis::x : IndexAxis::y;
          auto low = mesh.coordinate(s.a, axis), high = mesh.coordinate(s.b, axis);
          if (low.compare(high) > 0)
            std::swap(low, high);
          if (high.compare(box[2 * j]) < 0 || low.compare(box[2 * j + 1]) > 0)
            separated = true;
        }
        if (separated)
          continue;
        int sign = 0;
        bool opposite = false;
        for (const auto &corner : corners) {
          auto p = set_query(corner);
          int side = mesh.orient({s.a, s.b, p});
          if (!side) {
            opposite = true;
            break;
          }
          if (sign && side != sign) {
            opposite = true;
            break;
          }
          sign = side;
        }
        if (opposite)
          return true;
      }
    }
    return false;
  }
};

DomainBox domain_point_box(DomainPointRef point) {
  DomainBox result;
  for (uint32_t j = 0; j < 2; ++j) {
    if (!point[j])
      throw Error(ErrorCode::invalid_source, "null domain point");
    auto v = point[j]->bounds();
    if (!std::isfinite(v.lower) || !std::isfinite(v.upper))
      throw Error(ErrorCode::unsupported, "unbounded domain coordinate");
    result.lo[j] = v.lower;
    result.hi[j] = v.upper;
  }
  return result;
}
DomainBox domain_segment_box(DomainPointRef a, DomainPointRef b) {
  return joined(domain_point_box(a), domain_point_box(b));
}
DomainSegmentIndex::DomainSegmentIndex(bgfd::Budget &b, std::span<const DomainPointRef> p,
                                       DomainIndexStats &s, std::span<const DomainSegment> edges)
    : budget_(&b), impl_(nullptr) {
  if (p.size() >= UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "domain point capacity");
  auto memory = b.allocate(sizeof(Impl), alignof(Impl));
  try {
    impl_ = new (memory) Impl(b, p, s, edges);
  } catch (...) {
    b.deallocate(memory, sizeof(Impl), alignof(Impl));
    throw;
  }
}
DomainSegmentIndex::~DomainSegmentIndex() {
  if (impl_) {
    impl_->~Impl();
    budget_->deallocate(impl_, sizeof(Impl), alignof(Impl));
  }
}
std::optional<std::array<uint32_t, 2>> DomainSegmentIndex::crossing() { return impl_->crossing(); }
int64_t DomainSegmentIndex::winding_except(SiteSlot p, std::span<const DomainSegment> own) {
  return impl_->winding_except(p, own);
}
int64_t DomainSegmentIndex::winding(DomainPointRef p) {
  if (impl_->nodes.empty())
    return 0;
  return impl_->winding_except(impl_->set_query(p), {});
}
bool DomainSegmentIndex::boundary_contact(DomainPointRef p) { return impl_->contact(p); }
bool DomainSegmentIndex::touches_rectangle(std::span<const Rational, 4> box) {
  if (box[0].compare(box[1]) > 0 || box[2].compare(box[3]) > 0)
    throw Error(ErrorCode::invalid_source, "reversed domain rectangle");
  return impl_->rectangle(box);
}
} // namespace bgmesh
