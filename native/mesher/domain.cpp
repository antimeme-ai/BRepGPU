#include "domain.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace bgmesh {
namespace {
constexpr uint32_t absent = UINT32_MAX;
[[noreturn]] void invalid(const char *message) {
  throw bgfd::Failure(BGFD_INVALID_DOMAIN, message);
}
[[noreturn]] void internal(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
struct Box final {
  std::array<double, 2> lo{}, hi{};
};
Box joined(Box a, const Box &b) {
  for (std::size_t i = 0; i < 2; ++i) {
    a.lo[i] = std::min(a.lo[i], b.lo[i]);
    a.hi[i] = std::max(a.hi[i], b.hi[i]);
  }
  return a;
}
bool overlaps(const Box &a, const Box &b) {
  return !(a.hi[0] < b.lo[0] || b.hi[0] < a.lo[0] || a.hi[1] < b.lo[1] || b.hi[1] < a.lo[1]);
}
template <class T> void reserve_more(std::pmr::vector<T> &v, std::size_t count) {
  if (count > v.max_size() - v.size())
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "domain capacity exhausted");
  const auto need = v.size() + count;
  if (need > v.capacity())
    v.reserve(std::max(need, v.capacity() <= v.max_size() / 2 ? v.capacity() * 2 : v.max_size()));
}
struct Segment final {
  SiteSlot a, b;
  uint32_t loop;
  Box box;
};
struct Event final {
  SiteSlot y;
  int64_t delta = 0, prefix = 0;
};
struct Node final {
  Box box;
  uint32_t left = absent, right = absent;
  uint64_t first = 0, count = 0, event_first = 0, event_count = 0;
};
class SegmentIndex final {
  bgfd::Budget &budget;
  MutableFaceTopology &mesh;
  DomainStats &stats;
  std::span<const Segment> segments;
  std::pmr::vector<uint32_t> order;
  std::pmr::vector<Node> nodes;
  std::pmr::vector<Event> events;
  std::pmr::vector<Box> point_boxes;
  int ycompare(SiteSlot a, SiteSlot b) const { return mesh.compare_axis(a, b, CoordinateAxis::y); }
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
    for (const auto axis : {CoordinateAxis::x, CoordinateAxis::y}) {
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
      const auto axis = mesh.compare_axis(p, *common, CoordinateAxis::x) != 0 ? CoordinateAxis::x
                                                                              : CoordinateAxis::y;
      return mesh.compare_axis(p, *common, axis) == mesh.compare_axis(q, *common, axis);
    }
    const int p = mesh.orient({a.a, a.b, b.a}), q = mesh.orient({a.a, a.b, b.b});
    const int r = mesh.orient({b.a, b.b, a.a}), s = mesh.orient({b.a, b.b, a.b});
    return (p * q < 0 && r * s < 0) || (p == 0 && between({a.a, a.b}, b.a)) ||
           (q == 0 && between({a.a, a.b}, b.b)) || (r == 0 && between({b.a, b.b}, a.a)) ||
           (s == 0 && between({b.a, b.b}, a.b));
  }
  int64_t ray_segment(const Segment &s, SiteSlot p) const {
    const int a = mesh.compare_axis(s.a, p, CoordinateAxis::y),
              b = mesh.compare_axis(s.b, p, CoordinateAxis::y);
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
  SegmentIndex(bgfd::Budget &b, MutableFaceTopology &m, DomainStats &s,
               std::span<const Segment> input)
      : budget(b), mesh(m), stats(s), segments(input), order(&b), nodes(&b), events(&b),
        point_boxes(&b) {
    order.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
      budget.charge(1);
      order.push_back(static_cast<uint32_t>(i));
    }
    point_boxes.reserve(static_cast<std::size_t>(mesh.stats().sites));
    for (uint32_t i = 0; i < mesh.stats().sites; ++i) {
      budget.charge(1);
      Box box;
      for (std::size_t axis = 0; axis < 2; ++axis) {
        const auto r =
            mesh.coordinate(SiteSlot{i}, axis == 0 ? CoordinateAxis::x : CoordinateAxis::y)
                .bounds();
        if (!std::isfinite(r.lower) || !std::isfinite(r.upper))
          throw Error(ErrorCode::unsupported, "working chart requires finite coordinate enclosure");
        box.lo[axis] = r.lower;
        box.hi[axis] = r.upper;
      }
      point_boxes.push_back(box);
    }
    nodes.reserve(input.size() * 2);
    if (!input.empty())
      (void)build(0, input.size());
    stats.event_entries = events.size();
  }
  void validate_crossings() {
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
            throw bgfd::Failure(BGFD_CROSSING,
                                "selected domain boundaries cross, overlap or touch nonincidently");
        }
      }
    }
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
};
uint64_t exponent(const Rational &r) {
  const auto bits = r.denominator().bit_length();
  const auto power = bits - 1;
  const std::array<uint32_t, 1> word{1};
  const auto one = bgfd::Integer::from_words(r.budget(), 1, word);
  if (r.denominator().compare(one.shift_left(power)) != 0)
    invalid("initial metric coordinate is not dyadic");
  return power;
}
bgfd_integer append_integer(bgfd::Budget &budget, const bgfd::Integer &value,
                            std::pmr::vector<uint32_t> &limbs) {
  const auto words = value.magnitude();
  if (words.size() > UINT32_MAX)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "initial metric limb count exhausted");
  const bgfd_integer result{limbs.size(), static_cast<uint32_t>(words.size()), value.sign()};
  budget.charge(words.size());
  limbs.insert(limbs.end(), words.begin(), words.end());
  return result;
}
std::pmr::vector<bgcdt::TriangleWord> construct_cdt(bgfd::Budget &budget, MutableFaceTopology &mesh,
                                                    std::span<const uint32_t> slots,
                                                    std::span<const bgfd_loop> loops, uint64_t seed,
                                                    bgcdt::CoreStats &stats) {
  const auto n = mesh.stats().sites;
  // One common dyadic scale for BOTH transformed coordinates preserves the
  // selected full metric. Independent axis scaling would change Delaunay tests.
  std::pmr::vector<std::array<Rational, 2>> transformed(&budget);
  transformed.reserve(static_cast<std::size_t>(n));
  uint64_t common_power = 0;
  for (uint32_t i = 0; i < n; ++i) {
    budget.charge(1);
    std::array<Rational, 2> point{
        mesh.metric_coordinate(SiteSlot{i}, CoordinateAxis::x)
            .subtract(mesh.metric_coordinate(SiteSlot{0}, CoordinateAxis::x)),
        mesh.metric_coordinate(SiteSlot{i}, CoordinateAxis::y)
            .subtract(mesh.metric_coordinate(SiteSlot{0}, CoordinateAxis::y))};
    common_power = std::max({common_power, exponent(point[0]), exponent(point[1])});
    transformed.push_back(std::move(point));
  }
  std::pmr::vector<uint32_t> limbs(&budget);
  limbs.push_back(1);
  std::pmr::vector<bgfd_node> nodes(&budget);
  nodes.reserve(static_cast<std::size_t>(n));
  for (uint32_t i = 0; i < n; ++i) {
    budget.charge(1);
    const auto &point = transformed[i];
    const auto x = point[0].numerator().shift_left(common_power - exponent(point[0]));
    const auto y = point[1].numerator().shift_left(common_power - exponent(point[1]));
    nodes.push_back(
        {uint64_t{i} + 1, append_integer(budget, x, limbs), append_integer(budget, y, limbs)});
  }
  bgfd_input input{};
  input.abi = BGFD_ABI;
  input.size = sizeof(input);
  input.nodes = nodes.data();
  input.node_count = nodes.size();
  input.limbs = limbs.data();
  input.limb_count = limbs.size();
  input.x_weight = {0, 1, 1};
  input.y_weight = {0, 1, 1};
  bgcdt::Predicates predicates(budget, input);
  return bgcdt::solve(predicates, budget, slots, loops, seed, stats);
}
} // namespace

InitialDomain initialize_face_domain(bgfd::Budget &budget, MutableFaceTopology &mesh,
                                     std::span<const DomainLoopView> original, uint64_t seed) {
  if (mesh.stats().live_triangles != 0 || mesh.stats().sites < 3)
    invalid("domain requires pristine boundary sites");
  const auto n = mesh.stats().sites;
  if (n > UINT32_MAX)
    invalid("domain site index exhausted");
  if (&mesh.coordinate(SiteSlot{0}, CoordinateAxis::x).budget() != &budget)
    internal("domain/topology budget mismatch");
  InitialDomain result(&budget);
  result.traversal_adapters.resize(original.size());
  std::size_t outer = original.size(), outer_count = 0;
  for (std::size_t i = 0; i < original.size(); ++i) {
    budget.charge(1);
    if (original[i].outer) {
      outer = i;
      ++outer_count;
    }
  }
  if (outer_count != 1)
    throw Error(ErrorCode::unsupported, "ordinary domain requires exactly one declared outer loop");
  std::pmr::vector<uint8_t> seen(static_cast<std::size_t>(n), 0, &budget);
  std::pmr::vector<uint32_t> loop_order(&budget);
  loop_order.reserve(original.size());
  loop_order.push_back(static_cast<uint32_t>(outer));
  for (std::size_t i = 0; i < original.size(); ++i)
    if (i != outer)
      loop_order.push_back(static_cast<uint32_t>(i));
  std::pmr::vector<uint32_t> slots(&budget);
  std::pmr::vector<bgfd_loop> loops(&budget);
  std::pmr::vector<BoundaryKey> keys(&budget);
  std::pmr::vector<Segment> segments(&budget);
  slots.reserve(static_cast<std::size_t>(n));
  keys.reserve(static_cast<std::size_t>(n));
  segments.reserve(static_cast<std::size_t>(n));
  for (const auto index : loop_order) {
    const auto &loop = original[index];
    if (loop.sites.size() < 3 || loop.constraints.size() != loop.sites.size())
      invalid("invalid original domain loop inventory");
    Rational area(budget);
    for (std::size_t i = 0; i < loop.sites.size(); ++i) {
      budget.charge(1);
      const auto a = loop.sites[i], b = loop.sites[(i + 1) % loop.sites.size()];
      if (a.value >= n || b.value >= n || seen[a.value] != 0)
        invalid("domain site appears zero/multiple times or is out of range");
      seen[a.value] = 1;
      area = area.add(mesh.coordinate(a, CoordinateAxis::x)
                          .multiply(mesh.coordinate(b, CoordinateAxis::y))
                          .subtract(mesh.coordinate(a, CoordinateAxis::y)
                                        .multiply(mesh.coordinate(b, CoordinateAxis::x))));
    }
    if (area.sign() == 0)
      invalid("domain loop has zero signed area");
    const int32_t adapter = (loop.outer ? 1 : -1) * area.sign();
    result.traversal_adapters[index] = adapter;
    const auto offset = slots.size();
    loops.push_back({offset, loop.sites.size()});
    for (std::size_t i = 0; i < loop.sites.size(); ++i) {
      budget.charge(1);
      const auto site = loop.sites[adapter == 1 ? i : (i == 0 ? 0 : loop.sites.size() - i)];
      slots.push_back(site.value);
      keys.push_back(loop.constraints[adapter == 1 ? i : loop.sites.size() - 1 - i]);
    }
    for (std::size_t i = 0; i < loop.sites.size(); ++i) {
      budget.charge(1);
      const SiteSlot a{slots[offset + i]}, b{slots[offset + (i + 1) % loop.sites.size()]};
      Box box;
      for (std::size_t axis = 0; axis < 2; ++axis) {
        const auto coordinate = axis == 0 ? CoordinateAxis::x : CoordinateAxis::y;
        const auto x = mesh.coordinate(a, coordinate).bounds(),
                   y = mesh.coordinate(b, coordinate).bounds();
        if (!std::isfinite(x.lower) || !std::isfinite(x.upper) || !std::isfinite(y.lower) ||
            !std::isfinite(y.upper))
          throw Error(ErrorCode::unsupported, "working chart requires finite coordinate enclosure");
        box.lo[axis] = std::min(x.lower, y.lower);
        box.hi[axis] = std::max(x.upper, y.upper);
      }
      segments.push_back({a, b, index, box});
    }
  }
  budget.charge(seen.size());
  if (std::find(seen.begin(), seen.end(), uint8_t{0}) != seen.end())
    invalid("unowned initial domain site");
  {
    SegmentIndex index(budget, mesh, result.stats, segments);
    index.validate_crossings();
    for (std::size_t h = 1; h < loops.size(); ++h) {
      const auto &loop = loops[h];
      const auto own = std::span<const Segment>(segments).subspan(
          static_cast<std::size_t>(loop.offset), static_cast<std::size_t>(loop.count));
      if (index.winding_except(SiteSlot{slots[static_cast<std::size_t>(loop.offset)]}, own) != 1)
        invalid("hole lies outside outer domain or inside another hole");
    }
  }
  const auto triangles = construct_cdt(budget, mesh, slots, loops, seed, result.stats.cdt);
  for (const auto &triangle : triangles) {
    budget.charge(1);
    (void)mesh.add_initial_triangle(
        {SiteSlot{triangle[0]}, SiteSlot{triangle[1]}, SiteSlot{triangle[2]}});
  }
  for (std::size_t i = 0; i < segments.size(); ++i) {
    budget.charge(1);
    const auto edge = mesh.find_edge(segments[i].a, segments[i].b);
    if (!edge)
      internal("initial CDT omitted a source constraint");
    mesh.protect_edge(*edge, keys[i]);
  }
  mesh.seal_initial();
  return result;
}
} // namespace bgmesh
