#include "contact.hpp"
#include "interval.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#if defined(__x86_64__)
#include <immintrin.h>
#endif

namespace bgmesh {
namespace {
using Point = std::array<Rational, 3>;
[[noreturn]] void invalid(const char *message) { throw bgfd::Failure(BGFD_INVALID_INPUT, message); }
[[noreturn]] void internal(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
[[nodiscard]] Point exact_point(bgfd::Budget &budget, Vec3 value) {
  return {Rational::from_double(budget, value[0]), Rational::from_double(budget, value[1]),
          Rational::from_double(budget, value[2])};
}
[[nodiscard]] Point difference(const Point &a, const Point &b) {
  return {a[0].subtract(b[0]), a[1].subtract(b[1]), a[2].subtract(b[2])};
}
[[nodiscard]] Point cross(const Point &a, const Point &b) {
  return {a[1].multiply(b[2]).subtract(a[2].multiply(b[1])),
          a[2].multiply(b[0]).subtract(a[0].multiply(b[2])),
          a[0].multiply(b[1]).subtract(a[1].multiply(b[0]))};
}
[[nodiscard]] Rational dot(const Point &a, const Point &b) {
  return a[0].multiply(b[0]).add(a[1].multiply(b[1])).add(a[2].multiply(b[2]));
}
[[nodiscard]] bool equal(const Point &a, const Point &b) {
  return a[0].compare(b[0]) == 0 && a[1].compare(b[1]) == 0 && a[2].compare(b[2]) == 0;
}
[[nodiscard]] bool zero(const Point &a) {
  return a[0].sign() == 0 && a[1].sign() == 0 && a[2].sign() == 0;
}
[[nodiscard]] std::size_t dominant(const Point &a) {
  std::size_t best = 0;
  for (std::size_t i = 1; i < 3; ++i) {
    const auto x = a[i].sign() < 0 ? a[i].negative() : a[i];
    const auto y = a[best].sign() < 0 ? a[best].negative() : a[best];
    if (x.compare(y) > 0)
      best = i;
  }
  return best;
}
[[nodiscard]] Point interpolate(const Point &a, const Point &b, const Rational &t) {
  return {a[0].add(b[0].subtract(a[0]).multiply(t)), a[1].add(b[1].subtract(a[1]).multiply(t)),
          a[2].add(b[2].subtract(a[2]).multiply(t))};
}
[[nodiscard]] std::array<Point, 3> exact_facet(bgfd::Budget &budget, const ContactFacet &facet) {
  for (std::size_t i = 0; i < 3; ++i) {
    for (const auto value : facet.xyz[i])
      if (!std::isfinite(value))
        invalid("contact facet has nonfinite XYZ");
    for (std::size_t j = 0; j < i; ++j)
      if (facet.nodes[i] == facet.nodes[j])
        invalid("contact facet repeats a physical node");
  }
  std::array<Point, 3> result{exact_point(budget, facet.xyz[0]), exact_point(budget, facet.xyz[1]),
                              exact_point(budget, facet.xyz[2])};
  if (zero(cross(difference(result[1], result[0]), difference(result[2], result[0]))))
    invalid("contact facet has zero physical area");
  return result;
}
[[nodiscard]] Rational orient2(const Point &a, const Point &b, const Point &p, std::size_t drop) {
  const auto x = (drop + 1) % 3, y = (drop + 2) % 3;
  return b[x]
      .subtract(a[x])
      .multiply(p[y].subtract(a[y]))
      .subtract(b[y].subtract(a[y]).multiply(p[x].subtract(a[x])));
}
[[nodiscard]] bool strict_side(const std::array<Rational, 3> &d) {
  return (d[0].sign() > 0 && d[1].sign() > 0 && d[2].sign() > 0) ||
         (d[0].sign() < 0 && d[1].sign() < 0 && d[2].sign() < 0);
}
void append_unique(std::pmr::vector<Point> &points, const Point &point) {
  if (points.empty() || !equal(points.back(), point))
    points.push_back(point);
}
[[nodiscard]] std::pmr::vector<Point> plane_cut(bgfd::Budget &budget,
                                                const std::array<Point, 3> &triangle,
                                                const std::array<Rational, 3> &distances) {
  std::pmr::vector<Point> result(&budget);
  result.reserve(3);
  for (std::size_t i = 0; i < 3; ++i) {
    const auto j = (i + 1) % 3;
    if (distances[i].sign() == 0)
      append_unique(result, triangle[i]);
    if (distances[i].sign() * distances[j].sign() < 0)
      append_unique(result, interpolate(triangle[i], triangle[j],
                                        distances[i].divide(distances[i].subtract(distances[j]))));
  }
  if (result.size() > 1 && equal(result.front(), result.back()))
    result.pop_back();
  if (result.size() > 2)
    internal("nonparallel triangle plane cut has more than two points");
  return result;
}
[[nodiscard]] bool on_segment(const Point &p, const Point &a, const Point &b) {
  const auto direction = difference(b, a), offset = difference(p, a);
  if (!zero(cross(direction, offset)))
    return false;
  const auto axis = dominant(direction);
  const auto lo =
      std::min(a[axis], b[axis], [](const auto &x, const auto &y) { return x.compare(y) < 0; });
  const auto hi =
      std::max(a[axis], b[axis], [](const auto &x, const auto &y) { return x.compare(y) < 0; });
  return p[axis].compare(lo) >= 0 && p[axis].compare(hi) <= 0;
}
struct AllowedPoints final {
  SimplexDimension dimension;
  Point a, b;
  bool contains(const Point &p) const {
    if (dimension == SimplexDimension::none)
      return false;
    return dimension == SimplexDimension::vertex ? equal(p, a) : on_segment(p, a, b);
  }
};
[[nodiscard]] AllowedPoints allowed_points(bgfd::Budget &budget, const ContactFacet &a,
                                           const ContactFacet &b, AllowedSimplex allowed) {
  auto locate = [&](NodeSlot node) {
    std::size_t i = 0;
    while (i < 3 && a.nodes[i] != node)
      ++i;
    bool present = false;
    for (const auto other : b.nodes)
      present = present || other == node;
    if (i == 3 || !present)
      invalid("allowed contact node is not shared by both facets");
    return exact_point(budget, a.xyz[i]);
  };
  const auto origin = exact_point(budget, {0, 0, 0});
  if (allowed.dimension == SimplexDimension::none)
    return {allowed.dimension, origin, origin};
  if (allowed.dimension == SimplexDimension::vertex) {
    const auto p = locate(allowed.nodes[0]);
    return {allowed.dimension, p, p};
  }
  if (allowed.dimension != SimplexDimension::edge || allowed.nodes[0] == allowed.nodes[1])
    invalid("invalid allowed contact simplex");
  return {allowed.dimension, locate(allowed.nodes[0]), locate(allowed.nodes[1])};
}
[[nodiscard]] FacetContact interval_contact(bgfd::Budget &budget, const std::pmr::vector<Point> &a,
                                            const std::pmr::vector<Point> &b, std::size_t axis,
                                            const AllowedPoints &allowed) {
  if (a.empty() || b.empty())
    return {ContactDimension::empty, true};
  const auto less = [axis](const Point &x, const Point &y) { return x[axis].compare(y[axis]) < 0; };
  const auto pa = std::minmax_element(a.begin(), a.end(), less);
  const auto pb = std::minmax_element(b.begin(), b.end(), less);
  const auto lo =
      pa.first->at(axis).compare(pb.first->at(axis)) >= 0 ? pa.first->at(axis) : pb.first->at(axis);
  const auto hi = pa.second->at(axis).compare(pb.second->at(axis)) <= 0 ? pa.second->at(axis)
                                                                        : pb.second->at(axis);
  const int comparison = lo.compare(hi);
  if (comparison > 0)
    return {ContactDimension::empty, true};
  auto point_at = [&](const Rational &coordinate) {
    if (a.size() == 1)
      return a.front();
    return interpolate(a[0], a[1],
                       coordinate.subtract(a[0][axis]).divide(a[1][axis].subtract(a[0][axis])));
  };
  (void)budget;
  const auto first = point_at(lo);
  if (comparison == 0)
    return {ContactDimension::point, allowed.contains(first)};
  return {ContactDimension::segment, allowed.contains(first) && allowed.contains(point_at(hi))};
}
struct ExactFacetPair final {
  const std::array<Point, 3> &a, &b;
};
[[nodiscard]] FacetContact coplanar_contact(bgfd::Budget &budget, ExactFacetPair pair,
                                            const Point &normal, const AllowedPoints &allowed) {
  const auto &a = pair.a, &b = pair.b;
  const auto axis = dominant(normal);
  const auto sign = orient2(b[0], b[1], b[2], axis).sign();
  std::pmr::vector<Point> polygon(a.begin(), a.end(), &budget);
  for (std::size_t edge = 0; edge < 3 && !polygon.empty(); ++edge) {
    std::pmr::vector<Point> clipped(&budget);
    clipped.reserve(9);
    const auto &p = b[edge], &q = b[(edge + 1) % 3];
    auto previous = polygon.back();
    auto d0 = orient2(p, q, previous, axis);
    if (sign < 0)
      d0 = d0.negative();
    for (const auto &current : polygon) {
      auto d1 = orient2(p, q, current, axis);
      if (sign < 0)
        d1 = d1.negative();
      if ((d0.sign() >= 0) != (d1.sign() >= 0))
        append_unique(clipped, interpolate(previous, current, d0.divide(d0.subtract(d1))));
      if (d1.sign() >= 0)
        append_unique(clipped, current);
      previous = current;
      d0 = std::move(d1);
    }
    if (clipped.size() > 1 && equal(clipped.front(), clipped.back()))
      clipped.pop_back();
    if (clipped.size() > 9)
      internal("coplanar triangle clipping exceeded fixed bound");
    polygon = std::move(clipped);
  }
  if (polygon.empty())
    return {ContactDimension::empty, true};
  Rational area(budget);
  const auto x = (axis + 1) % 3, y = (axis + 2) % 3;
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const auto &p = polygon[i], &q = polygon[(i + 1) % polygon.size()];
    area = area.add(p[x].multiply(q[y]).subtract(p[y].multiply(q[x])));
  }
  if (area.sign() != 0)
    return {ContactDimension::area, false};
  const auto less = [](const Point &p, const Point &q) {
    for (std::size_t i = 0; i < 3; ++i) {
      const int value = p[i].compare(q[i]);
      if (value != 0)
        return value < 0;
    }
    return false;
  };
  const auto extremes = std::minmax_element(polygon.begin(), polygon.end(), less);
  if (equal(*extremes.first, *extremes.second))
    return {ContactDimension::point, allowed.contains(*extremes.first)};
  return {ContactDimension::segment,
          allowed.contains(*extremes.first) && allowed.contains(*extremes.second)};
}
} // namespace

static FacetContact classify_exact(bgfd::Budget &budget, const ContactFacet &a,
                                   const ContactFacet &b, AllowedSimplex allowed) {
  if (a.definition != b.definition)
    invalid("contact pair spans definitions");
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j)
      if (a.nodes[i] == b.nodes[j])
        for (std::size_t axis = 0; axis < 3; ++axis)
          if (std::bit_cast<uint64_t>(a.xyz[i][axis]) != std::bit_cast<uint64_t>(b.xyz[j][axis]))
            invalid("shared physical node has inconsistent XYZ words");
  const auto pa = exact_facet(budget, a), pb = exact_facet(budget, b);
  const auto permission = allowed_points(budget, a, b, allowed);
  const auto na = cross(difference(pa[1], pa[0]), difference(pa[2], pa[0]));
  const auto nb = cross(difference(pb[1], pb[0]), difference(pb[2], pb[0]));
  std::array<Rational, 3> da{dot(nb, difference(pa[0], pb[0])), dot(nb, difference(pa[1], pb[0])),
                             dot(nb, difference(pa[2], pb[0]))};
  std::array<Rational, 3> db{dot(na, difference(pb[0], pa[0])), dot(na, difference(pb[1], pa[0])),
                             dot(na, difference(pb[2], pa[0]))};
  if (strict_side(da) || strict_side(db))
    return {ContactDimension::empty, true};
  const auto direction = cross(na, nb);
  if (zero(direction)) {
    if (db[0].sign() != 0)
      return {ContactDimension::empty, true};
    return coplanar_contact(budget, {pa, pb}, na, permission);
  }
  return interval_contact(budget, plane_cut(budget, pa, da), plane_cut(budget, pb, db),
                          dominant(direction), permission);
}
namespace {
namespace iv = bgmesh_interval;
using Enclosure = std::array<bgmb_interval, 3>;
[[nodiscard]] bool predicate_environment() noexcept {
#if defined(__x86_64__) && FLT_EVAL_METHOD == 0
  return std::fegetround() == FE_TONEAREST && (_mm_getcsr() & uint32_t{0xE040}) == 0;
#else
  return false;
#endif
}
[[nodiscard]] Enclosure enclosed_difference(Vec3 a, Vec3 b) {
  Enclosure result;
  for (std::size_t i = 0; i < 3; ++i)
    result[i] = a[i] == b[i] ? bgmb_interval{0, 0} : iv::subtract({a[i], a[i]}, {b[i], b[i]});
  return result;
}
[[nodiscard]] Enclosure enclosed_cross(const Enclosure &a, const Enclosure &b) {
  return {iv::subtract(iv::multiply(a[1], b[2]), iv::multiply(a[2], b[1])),
          iv::subtract(iv::multiply(a[2], b[0]), iv::multiply(a[0], b[2])),
          iv::subtract(iv::multiply(a[0], b[1]), iv::multiply(a[1], b[0]))};
}
[[nodiscard]] bgmb_interval enclosed_dot(const Enclosure &a, const Enclosure &b) {
  return iv::add(iv::add(iv::multiply(a[0], b[0]), iv::multiply(a[1], b[1])),
                 iv::multiply(a[2], b[2]));
}
[[nodiscard]] int certified_sign(bgmb_interval value) {
  if (!iv::valid(value))
    return 0;
  return value.lower > 0 ? 1 : (value.upper < 0 ? -1 : 0);
}
[[nodiscard]] bool strict_interval_side(const std::array<bgmb_interval, 3> &d) {
  const auto sign = certified_sign(d[0]);
  return sign != 0 && certified_sign(d[1]) == sign && certified_sign(d[2]) == sign;
}
void validate_pair(const ContactFacet &a, const ContactFacet &b, AllowedSimplex allowed) {
  if (a.definition != b.definition)
    invalid("contact pair spans definitions");
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j)
      if (a.nodes[i] == b.nodes[j])
        for (std::size_t axis = 0; axis < 3; ++axis)
          if (std::bit_cast<uint64_t>(a.xyz[i][axis]) != std::bit_cast<uint64_t>(b.xyz[j][axis]))
            invalid("shared physical node has inconsistent XYZ words");
  const auto present = [](const ContactFacet &f, NodeSlot node) {
    return std::find(f.nodes.begin(), f.nodes.end(), node) != f.nodes.end();
  };
  if (allowed.dimension == SimplexDimension::none)
    return;
  if (allowed.dimension != SimplexDimension::vertex && allowed.dimension != SimplexDimension::edge)
    invalid("invalid allowed contact simplex");
  if (!present(a, allowed.nodes[0]) || !present(b, allowed.nodes[0]))
    invalid("allowed contact node is not shared by both facets");
  if (allowed.dimension == SimplexDimension::edge &&
      (allowed.nodes[0] == allowed.nodes[1] || !present(a, allowed.nodes[1]) ||
       !present(b, allowed.nodes[1])))
    invalid("invalid allowed contact edge");
}
[[nodiscard]] std::optional<FacetContact> filter_projected(const ContactFacet &a,
                                                          const ContactFacet &b, std::size_t drop,
                                                          AllowedSimplex allowed,
                                                          ContactPredicateStats &stats) {
  const auto turn = [drop](std::array<Vec3, 3> points) {
    const auto x = (drop + 1) % 3, y = (drop + 2) % 3;
    const auto v = enclosed_difference(points[1], points[0]),
               w = enclosed_difference(points[2], points[0]);
    return iv::subtract(iv::multiply(v[x], w[y]), iv::multiply(v[y], w[x]));
  };
  const auto separating = [&](const ContactFacet &first, const ContactFacet &second) {
    const int orientation = certified_sign(turn(first.xyz));
    if (orientation == 0)
      return false;
    for (std::size_t edge = 0; edge < 3; ++edge) {
      bool outside = true;
      for (const auto &p : second.xyz)
        outside =
            outside &&
            certified_sign(turn({first.xyz[edge], first.xyz[(edge + 1) % 3], p})) == -orientation;
      if (outside)
        return true;
    }
    return false;
  };
  if (separating(a, b) || separating(b, a)) {
    ++stats.filtered_separation;
    return FacetContact{ContactDimension::empty, true};
  }
  if (allowed.dimension == SimplexDimension::edge) {
    const auto corner = [&](const ContactFacet &f, NodeSlot id) {
      return static_cast<std::size_t>(std::find(f.nodes.begin(), f.nodes.end(), id) -
                                      f.nodes.begin());
    };
    const auto i = corner(a, allowed.nodes[0]), j = corner(a, allowed.nodes[1]);
    std::size_t k = 0;
    while (b.nodes[k] == allowed.nodes[0] || b.nodes[k] == allowed.nodes[1])
      ++k;
    const int sa = certified_sign(turn({a.xyz[i], a.xyz[j], a.xyz[3 - i - j]}));
    const int sb = certified_sign(turn({a.xyz[i], a.xyz[j], b.xyz[k]}));
    if (sa != 0 && sb == -sa) {
      ++stats.filtered_projected;
      return FacetContact{ContactDimension::segment, true};
    }
  }
  return std::nullopt;
}
} // namespace

PreparedContactFacet prepare_contact_facet(bgfd::Budget &budget, const ContactFacet &facet) {
  budget.charge(1);
  PreparedContactFacet result;
  result.facet_ = facet;
  for (std::size_t i = 0; i < 3; ++i) {
    for (const auto value : facet.xyz[i])
      if (!std::isfinite(value))
        invalid("contact facet has nonfinite XYZ");
    for (std::size_t j = 0; j < i; ++j)
      if (facet.nodes[i] == facet.nodes[j])
        invalid("contact facet repeats a physical node");
  }
  if (predicate_environment()) {
    for (std::size_t i = 0; i < 3; ++i)
      result.edges_[i] = enclosed_difference(facet.xyz[(i + 1) % 3], facet.xyz[i]);
    const auto second = enclosed_difference(facet.xyz[2], facet.xyz[0]);
    result.normal_ = enclosed_cross(result.edges_[0], second);
    result.filtered_ = true;
    for (const auto value : result.normal_)
      if (certified_sign(value) != 0)
        return result;
  }
  // Uncertain normal is not treated as zero. Exact preparation also rejects
  // degenerate facets before any pair can use a separating shortcut.
  (void)exact_facet(budget, facet);
  return result;
}

FacetContact classify_contact(bgfd::Budget &budget, const PreparedContactFacet &first,
                              const PreparedContactFacet &second, AllowedSimplex allowed,
                              ContactPredicateStats &stats) {
  budget.charge(1);
  const auto &a = first.facet_, &b = second.facet_;
  validate_pair(a, b, allowed);
  if (first.filtered_ && second.filtered_ && predicate_environment()) {
    std::array<bgmb_interval, 3> da{}, db{};
    for (std::size_t i = 0; i < 3; ++i) {
      da[i] = enclosed_dot(second.normal_, enclosed_difference(a.xyz[i], b.xyz[0]));
      db[i] = enclosed_dot(first.normal_, enclosed_difference(b.xyz[i], a.xyz[0]));
    }
    if (strict_interval_side(da) || strict_interval_side(db)) {
      ++stats.filtered_separation;
      return {ContactDimension::empty, true};
    }
    if (allowed.dimension == SimplexDimension::edge) {
      for (std::size_t i = 0; i < 3; ++i)
        if (b.nodes[i] != allowed.nodes[0] && b.nodes[i] != allowed.nodes[1] &&
            certified_sign(db[i]) != 0) {
          ++stats.filtered_shared_edge;
          return {ContactDimension::segment, true};
        }
    }
    if (allowed.dimension == SimplexDimension::vertex) {
      const auto only_vertex = [&](const ContactFacet &f,
                                   const std::array<bgmb_interval, 3> &distances) {
        int sign = 0;
        for (std::size_t i = 0; i < 3; ++i)
          if (f.nodes[i] != allowed.nodes[0]) {
            const int current = certified_sign(distances[i]);
            if (current == 0 || (sign != 0 && current != sign))
              return false;
            sign = current;
          }
        return true;
      };
      if (only_vertex(a, da) || only_vertex(b, db)) {
        ++stats.filtered_shared_vertex;
        return {ContactDimension::point, true};
      }
    }
    // Projection separation is sufficient in 3D without assuming coplanarity.
    // Likewise, opposite projected third vertices confine both triangles' full
    // intersection to their actual declared common edge.
    for (std::size_t axis = 0; axis < 3; ++axis)
      if (certified_sign(first.normal_[axis]) != 0) {
        if (const auto answer = filter_projected(a, b, axis, allowed, stats))
          return *answer;
        break;
      }
    // The nine edge-cross-edge SAT axes need not be normalized. Project about
    // one actual vertex to avoid paying for a large world-space origin.
    for (const auto &edge_a : first.edges_)
      for (const auto &edge_b : second.edges_) {
        const auto axis = enclosed_cross(edge_a, edge_b);
        double alo = INFINITY, ahi = -INFINITY, blo = INFINITY, bhi = -INFINITY;
        bool valid = true;
        for (std::size_t i = 0; i < 3; ++i) {
          const auto pa = enclosed_dot(axis, enclosed_difference(a.xyz[i], a.xyz[0]));
          const auto pb = enclosed_dot(axis, enclosed_difference(b.xyz[i], a.xyz[0]));
          valid = valid && iv::valid(pa) && iv::valid(pb);
          alo = std::min(alo, pa.lower);
          ahi = std::max(ahi, pa.upper);
          blo = std::min(blo, pb.lower);
          bhi = std::max(bhi, pb.upper);
        }
        if (valid && (ahi < blo || bhi < alo)) {
          ++stats.filtered_separation;
          return {ContactDimension::empty, true};
        }
      }
  }
  ++stats.exact_pairs;
  return classify_exact(budget, a, b, allowed);
}

FacetContact classify_contact(bgfd::Budget &budget, const ContactFacet &a, const ContactFacet &b,
                              AllowedSimplex allowed) {
  const auto first = prepare_contact_facet(budget, a), second = prepare_contact_facet(budget, b);
  ContactPredicateStats stats;
  return classify_contact(budget, first, second, allowed, stats);
}
FacetBox facet_box(bgfd::Budget &budget, FacetHandle handle, const ContactFacet &facet) {
  return facet_box(handle, prepare_contact_facet(budget, facet));
}
FacetBox facet_box(FacetHandle handle, const PreparedContactFacet &prepared) {
  const auto &facet = prepared.facet();
  FacetBox box{handle, facet.xyz[0], facet.xyz[0]};
  for (const auto &point : facet.xyz)
    for (std::size_t axis = 0; axis < 3; ++axis) {
      box.lower[axis] = std::min(box.lower[axis], point[axis]);
      box.upper[axis] = std::max(box.upper[axis], point[axis]);
    }
  return box;
}
} // namespace bgmesh

namespace bgmesh {
namespace {
constexpr uint32_t missing = UINT32_MAX;
struct Box final {
  Vec3 lower{}, upper{};
};
[[nodiscard]] Box joined(const Box &a, const Box &b) {
  Box result;
  for (std::size_t i = 0; i < 3; ++i) {
    result.lower[i] = std::min(a.lower[i], b.lower[i]);
    result.upper[i] = std::max(a.upper[i], b.upper[i]);
  }
  return result;
}
[[nodiscard]] bool overlaps(const Box &a, const Box &b) {
  for (std::size_t i = 0; i < 3; ++i)
    if (a.upper[i] < b.lower[i] || b.upper[i] < a.lower[i])
      return false;
  return true;
}
void validate_box(const Box &box) {
  for (std::size_t i = 0; i < 3; ++i)
    if (!std::isfinite(box.lower[i]) || !std::isfinite(box.upper[i]) || box.lower[i] > box.upper[i])
      invalid("invalid physical facet box");
}
[[nodiscard]] uint32_t checked_slot(std::size_t value) {
  if (value >= missing)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "facet BVH slot range exhausted");
  return static_cast<uint32_t>(value);
}
[[nodiscard]] std::size_t hash_slot(uint32_t value) noexcept {
  uint64_t word = value;
  word = (word ^ (word >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  word = (word ^ (word >> 27)) * UINT64_C(0x94d049bb133111eb);
  return static_cast<std::size_t>(word ^ (word >> 31));
}
template <class T> void grow(std::pmr::vector<T> &v, std::size_t count) {
  if (count > v.max_size() - v.size())
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "facet BVH storage overflow");
  const auto need = v.size() + count;
  if (need > v.capacity())
    v.reserve(std::max(need, v.capacity() <= v.max_size() / 2 ? v.capacity() * 2 : v.max_size()));
}
struct I final {
  double lo, hi;
};
[[nodiscard]] I sum(I a, I b) {
  return {std::nextafter(a.lo + b.lo, -INFINITY), std::nextafter(a.hi + b.hi, INFINITY)};
}
[[nodiscard]] I sub(I a, I b) {
  return {std::nextafter(a.lo - b.hi, -INFINITY), std::nextafter(a.hi - b.lo, INFINITY)};
}
[[nodiscard]] I product(I a, I b) {
  const std::array<double, 4> values{a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
  for (const auto x : values)
    if (!std::isfinite(x))
      return {-INFINITY, INFINITY};
  const auto p = std::minmax_element(values.begin(), values.end());
  return {std::nextafter(*p.first, -INFINITY), std::nextafter(*p.second, INFINITY)};
}
[[nodiscard]] bool filtered_environment() noexcept {
#if defined(__x86_64__) && FLT_EVAL_METHOD == 0
  return std::fegetround() == FE_TONEAREST && (_mm_getcsr() & uint32_t{0xE040}) == 0;
#else
  return false;
#endif
}
[[nodiscard]] I area_interval(const Box &box) {
  std::array<I, 3> widths{};
  for (std::size_t i = 0; i < 3; ++i) {
    widths[i] = sub({box.upper[i], box.upper[i]}, {box.lower[i], box.lower[i]});
    widths[i].lo = std::max(0.0, widths[i].lo);
  }
  return sum(sum(product(widths[0], widths[1]), product(widths[1], widths[2])),
             product(widths[2], widths[0]));
}
} // namespace

struct FacetBVH::Impl final {
  struct Record final {
    FacetHandle handle{};
    Box box;
    uint32_t node = missing;
    bool live = false;
  };
  struct Node final {
    Box box;
    uint32_t parent = missing, left = missing, right = missing, used = 0, height = 0, count = 0;
    std::array<uint32_t, 4> records{missing, missing, missing, missing};
    bool live = false;
    [[nodiscard]] bool leaf() const noexcept { return left == missing; }
  };
  struct Entry final {
    uint32_t slot = missing, record = missing;
    uint8_t state = 0;
  };
  struct Cost final {
    Box a, b;
    uint32_t n = 0, m = 0;
    bool minus = false;
  };
  struct Bin final {
    Box box;
    uint32_t count = 0;
  };
  struct BuildPosition final {
    uint32_t parent, depth;
  };
  bgfd::Budget &budget;
  std::pmr::vector<Record> records;
  std::pmr::vector<Node> nodes;
  std::pmr::vector<uint32_t> free_records, free_nodes;
  std::pmr::vector<Entry> index;
  uint32_t root = missing;
  std::size_t indexed = 0, dead = 0;
  uint64_t since_build = 0;
  bool usable = true;
  BVHStats counters;
  explicit Impl(bgfd::Budget &owner)
      : budget(owner), records(&owner), nodes(&owner), free_records(&owner), free_nodes(&owner),
        index(&owner) {
    index.resize(16);
  }
  void check() const {
    if (!usable)
      internal("facet BVH generation failed");
  }
  uint32_t lookup(uint32_t key) {
    const auto mask = index.size() - 1;
    auto at = hash_slot(key) & mask;
    for (std::size_t i = 0; i < index.size(); ++i) {
      budget.charge(1);
      const auto &e = index[at];
      if (e.state == 0)
        return missing;
      if (e.state == 1 && e.slot == key)
        return e.record;
      at = (at + 1) & mask;
    }
    internal("facet index lookup cycle");
  }
  void place(uint32_t key, uint32_t record) {
    const auto mask = index.size() - 1;
    auto at = hash_slot(key) & mask;
    auto first = index.size();
    for (std::size_t i = 0; i < index.size(); ++i) {
      budget.charge(1);
      if (index[at].state == 2 && first == index.size())
        first = at;
      if (index[at].state == 0) {
        if (first != index.size()) {
          at = first;
          --dead;
        }
        index[at] = {key, record, 1};
        ++indexed;
        return;
      }
      if (index[at].state == 1 && index[at].slot == key)
        internal("duplicate facet index");
      at = (at + 1) & mask;
    }
    internal("facet index full");
  }
  void erase_index(uint32_t key) {
    const auto mask = index.size() - 1;
    auto at = hash_slot(key) & mask;
    for (std::size_t i = 0; i < index.size(); ++i) {
      budget.charge(1);
      if (index[at].state == 0)
        internal("facet index erase missing");
      if (index[at].state == 1 && index[at].slot == key) {
        index[at].state = 2;
        --indexed;
        ++dead;
        return;
      }
      at = (at + 1) & mask;
    }
    internal("facet index erase cycle");
  }
  void prepare_index() {
    if (indexed + 1 < index.size() / 2 && dead <= index.size() / 4)
      return;
    auto capacity = index.size();
    if (indexed + 1 >= capacity / 2) {
      if (capacity > index.max_size() / 2)
        throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "facet index capacity exhausted");
      capacity *= 2;
    }
    std::pmr::vector<Entry> fresh(capacity, Entry{}, &budget);
    const auto mask = capacity - 1;
    for (const auto &e : index)
      if (e.state == 1) {
        auto at = hash_slot(e.slot) & mask;
        for (std::size_t probes = 0;; ++probes) {
          budget.charge(1);
          if (probes >= fresh.size())
            internal("facet rehash cycle");
          if (fresh[at].state == 0)
            break;
          at = (at + 1) & mask;
        }
        fresh[at] = e;
      }
    index.swap(fresh);
    dead = 0;
  }
  Rational area(const Box &box) {
    std::array<Rational, 3> width{Rational::from_double(budget, box.upper[0])
                                      .subtract(Rational::from_double(budget, box.lower[0])),
                                  Rational::from_double(budget, box.upper[1])
                                      .subtract(Rational::from_double(budget, box.lower[1])),
                                  Rational::from_double(budget, box.upper[2])
                                      .subtract(Rational::from_double(budget, box.lower[2]))};
    return width[0]
        .multiply(width[1])
        .add(width[1].multiply(width[2]))
        .add(width[2].multiply(width[0]));
  }
  Rational exact_cost(const Cost &cost) {
    ++counters.exact_costs;
    const auto a = area(cost.a).multiply(Rational(budget, cost.n));
    const auto b = area(cost.b).multiply(Rational(budget, cost.m));
    return cost.minus ? a.subtract(b) : a.add(b);
  }
  I approximate_cost(const Cost &cost) const {
    const double n = cost.n, m = cost.m;
    const auto a = product(area_interval(cost.a), {n, n}),
               b = product(area_interval(cost.b), {m, m});
    return cost.minus ? sub(a, b) : sum(a, b);
  }
  bool lower_cost(const Cost &a, const Cost &b) {
    budget.charge(1);
    if (filtered_environment()) {
      const auto x = approximate_cost(a), y = approximate_cost(b);
      if (x.hi < y.lo)
        return true;
      if (x.lo > y.hi)
        return false;
    }
    return exact_cost(a).compare(exact_cost(b)) < 0;
  }
  int center_compare(const Box &a, const Box &b, std::size_t axis) {
    if (filtered_environment()) {
      const auto x = sum({a.lower[axis], a.lower[axis]}, {a.upper[axis], a.upper[axis]});
      const auto y = sum({b.lower[axis], b.lower[axis]}, {b.upper[axis], b.upper[axis]});
      if (x.hi < y.lo)
        return -1;
      if (x.lo > y.hi)
        return 1;
    }
    return Rational::from_double(budget, a.lower[axis])
        .add(Rational::from_double(budget, a.upper[axis]))
        .compare(Rational::from_double(budget, b.lower[axis])
                     .add(Rational::from_double(budget, b.upper[axis])));
  }
  uint32_t bin(const Box &value, const Box &parent, std::size_t axis) {
    uint32_t lo = 0, hi = 16;
    while (hi - lo > 1) {
      budget.charge(1);
      const auto k = (lo + hi) / 2;
      int comparison = 0;
      bool resolved = false;
      if (filtered_environment()) {
        const auto center =
            sum({value.lower[axis], value.lower[axis]}, {value.upper[axis], value.upper[axis]});
        const double factor = static_cast<double>(k) / 8;
        const auto boundary = sum(product({parent.lower[axis], parent.lower[axis]}, {2, 2}),
                                  product(sub({parent.upper[axis], parent.upper[axis]},
                                              {parent.lower[axis], parent.lower[axis]}),
                                          {factor, factor}));
        if (center.hi < boundary.lo) {
          comparison = -1;
          resolved = true;
        } else if (center.lo > boundary.hi) {
          comparison = 1;
          resolved = true;
        }
      }
      if (!resolved) {
        const auto p = Rational::from_double(budget, parent.lower[axis]);
        const auto cut = p.multiply(Rational(budget, 2))
                             .add(Rational::from_double(budget, parent.upper[axis])
                                      .subtract(p)
                                      .multiply(Rational(budget, k))
                                      .divide(Rational(budget, 8)));
        comparison = Rational::from_double(budget, value.lower[axis])
                         .add(Rational::from_double(budget, value.upper[axis]))
                         .compare(cut);
      }
      if (comparison >= 0)
        lo = k;
      else
        hi = k;
    }
    return std::min(lo, uint32_t{15});
  }
  std::size_t longest_axis(const Box &box) {
    std::size_t best = 0;
    for (std::size_t i = 1; i < 3; ++i) {
      const auto a = Rational::from_double(budget, box.upper[i])
                         .subtract(Rational::from_double(budget, box.lower[i]));
      const auto b = Rational::from_double(budget, box.upper[best])
                         .subtract(Rational::from_double(budget, box.lower[best]));
      if (a.compare(b) > 0)
        best = i;
    }
    return best;
  }
  uint32_t allocate_node() {
    uint32_t slot;
    if (free_nodes.empty()) {
      slot = checked_slot(nodes.size());
      nodes.emplace_back();
    } else {
      slot = free_nodes.back();
      free_nodes.pop_back();
      nodes[slot] = Node{};
    }
    nodes[slot].live = true;
    return slot;
  }
  void leaf(uint32_t node, std::span<const uint32_t> values) {
    if (values.empty() || values.size() > 4)
      internal("invalid BVH leaf size");
    auto &n = nodes[node];
    n.used = static_cast<uint32_t>(values.size());
    n.count = n.used;
    n.height = 0;
    n.left = n.right = missing;
    n.box = records[values[0]].box;
    for (std::size_t i = 0; i < values.size(); ++i) {
      n.records[i] = values[i];
      records[values[i]].node = node;
      n.box = joined(n.box, records[values[i]].box);
    }
  }
  uint32_t depth_limit() const {
    auto n = std::max(uint64_t{2}, counters.live_facets) - 1;
    uint32_t bits = 0;
    while (n != 0) {
      ++bits;
      n >>= 1;
    }
    return bits * 2 + 8;
  }
  uint32_t build_node(std::span<uint32_t> values, BuildPosition position) {
    const auto parent = position.parent, depth = position.depth;
    budget.charge(values.size());
    const auto node = allocate_node();
    nodes[node].parent = parent;
    if (values.size() <= 4) {
      leaf(node, values);
      return node;
    }
    Box box = records[values[0]].box;
    for (const auto value : values)
      box = joined(box, records[value].box);
    bool found = false;
    Cost best{};
    std::size_t chosen_axis = 0;
    uint32_t chosen_bin = 0;
    if (depth < depth_limit())
      for (std::size_t axis = 0; axis < 3; ++axis) {
        std::array<Bin, 16> bins{};
        for (const auto value : values) {
          auto &b = bins[bin(records[value].box, box, axis)];
          b.box = b.count == 0 ? records[value].box : joined(b.box, records[value].box);
          ++b.count;
        }
        std::array<Bin, 16> left{}, right{};
        Bin aggregate;
        for (std::size_t i = 0; i < 16; ++i) {
          if (bins[i].count != 0) {
            aggregate.box = aggregate.count == 0 ? bins[i].box : joined(aggregate.box, bins[i].box);
            aggregate.count += bins[i].count;
          }
          left[i] = aggregate;
        }
        aggregate = Bin{};
        for (std::size_t i = 16; i > 0; --i) {
          const auto j = i - 1;
          if (bins[j].count != 0) {
            aggregate.box = aggregate.count == 0 ? bins[j].box : joined(aggregate.box, bins[j].box);
            aggregate.count += bins[j].count;
          }
          right[j] = aggregate;
        }
        for (uint32_t cut = 0; cut < 15; ++cut)
          if (left[cut].count != 0 && right[cut + 1].count != 0) {
            const Cost cost{left[cut].box, right[cut + 1].box, left[cut].count,
                            right[cut + 1].count, false};
            if (!found || lower_cost(cost, best)) {
              found = true;
              best = cost;
              chosen_axis = axis;
              chosen_bin = cut;
            }
          }
      }
    auto middle = values.begin();
    if (found)
      middle = std::partition(values.begin(), values.end(), [&](uint32_t value) {
        return bin(records[value].box, box, chosen_axis) <= chosen_bin;
      });
    if (!found || middle == values.begin() || middle == values.end()) {
      const auto axis = longest_axis(box);
      middle = values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
      std::nth_element(values.begin(), middle, values.end(), [&](uint32_t a, uint32_t b) {
        budget.charge(1);
        const int c = center_compare(records[a].box, records[b].box, axis);
        return c < 0 || (c == 0 && records[a].handle < records[b].handle);
      });
    }
    const auto count = static_cast<std::size_t>(middle - values.begin());
    const auto left = build_node(values.first(count), {node, depth + 1}),
               right = build_node(values.subspan(count), {node, depth + 1});
    nodes[node].left = left;
    nodes[node].right = right;
    nodes[node].box = joined(nodes[left].box, nodes[right].box);
    nodes[node].count = nodes[left].count + nodes[right].count;
    nodes[node].height = std::max(nodes[left].height, nodes[right].height) + 1;
    return node;
  }
  void rebuild() {
    std::pmr::vector<uint32_t> order(&budget);
    order.reserve(static_cast<std::size_t>(counters.live_facets));
    for (uint32_t i = 0; i < records.size(); ++i) {
      budget.charge(1);
      if (records[i].live)
        order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
      budget.charge(1);
      return records[a].handle < records[b].handle;
    });
    nodes.clear();
    free_nodes.clear();
    root = missing;
    if (!order.empty())
      root = build_node(order, {missing, 0});
    since_build = 0;
    ++counters.rebuilds;
  }
  void refit(uint32_t node) {
    uint64_t steps = 0;
    while (node != missing) {
      budget.charge(1);
      if (++steps > nodes.size())
        internal("BVH parent cycle");
      auto &n = nodes[node];
      if (n.leaf()) {
        if (n.used == 0)
          internal("empty live BVH leaf");
        n.box = records[n.records[0]].box;
        for (uint32_t i = 1; i < n.used; ++i)
          n.box = joined(n.box, records[n.records[i]].box);
        n.count = n.used;
        n.height = 0;
      } else {
        n.box = joined(nodes[n.left].box, nodes[n.right].box);
        n.count = nodes[n.left].count + nodes[n.right].count;
        n.height = std::max(nodes[n.left].height, nodes[n.right].height) + 1;
      }
      node = n.parent;
    }
  }
  void maybe_rebuild() {
    if (root != missing && (nodes[root].height > depth_limit() ||
                            since_build >= std::max(uint64_t{1}, (counters.live_facets + 1) / 2)))
      rebuild();
  }
  uint32_t create_record(const FacetBox &box) {
    Record row{box.facet, {box.lower, box.upper}, missing, true};
    uint32_t slot;
    if (free_records.empty()) {
      slot = checked_slot(records.size());
      records.push_back(row);
    } else {
      slot = free_records.back();
      free_records.pop_back();
      records[slot] = row;
    }
    place(box.facet.slot.value, slot);
    ++counters.live_facets;
    return slot;
  }
};

FacetBVH::FacetBVH(bgfd::Budget &budget) : budget_(&budget), impl_(nullptr) {
  std::pmr::polymorphic_allocator<Impl> a(&budget);
  impl_ = a.allocate(1);
  try {
    std::construct_at(impl_, budget);
  } catch (...) {
    a.deallocate(impl_, 1);
    throw;
  }
}
FacetBVH::~FacetBVH() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(budget_).deallocate(impl_, 1);
}
void FacetBVH::build(std::span<const FacetBox> boxes) {
  impl_->check();
  try {
    impl_->records.clear();
    impl_->nodes.clear();
    impl_->free_records.clear();
    impl_->free_nodes.clear();
    impl_->index.assign(16, Impl::Entry{});
    impl_->indexed = impl_->dead = 0;
    impl_->root = missing;
    impl_->counters.live_facets = 0;
    for (const auto &box : boxes) {
      validate_box({box.lower, box.upper});
      if (impl_->lookup(box.facet.slot.value) != missing)
        invalid("duplicate facet slot in BVH build");
      impl_->prepare_index();
      (void)impl_->create_record(box);
    }
    impl_->rebuild();
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
void FacetBVH::insert(const FacetBox &box) {
  impl_->check();
  validate_box({box.lower, box.upper});
  if (impl_->lookup(box.facet.slot.value) != missing)
    invalid("facet slot already live");
  impl_->prepare_index();
  grow(impl_->records, 1);
  grow(impl_->nodes, 2);
  try {
    const auto record = impl_->create_record(box);
    ++impl_->since_build;
    ++impl_->counters.updates;
    if (impl_->root == missing) {
      impl_->root = impl_->allocate_node();
      const std::array<uint32_t, 1> entry{record};
      impl_->leaf(impl_->root, entry);
      return;
    }
    auto node = impl_->root;
    uint64_t steps = 0;
    while (!impl_->nodes[node].leaf()) {
      impl_->budget.charge(1);
      if (++steps > impl_->nodes.size())
        internal("BVH descent cycle");
      const auto left = impl_->nodes[node].left, right = impl_->nodes[node].right;
      const auto &a = impl_->nodes[left], &b = impl_->nodes[right];
      const Impl::Cost ca{joined(a.box, impl_->records[record].box), a.box, a.count + 1, a.count,
                          true};
      const Impl::Cost cb{joined(b.box, impl_->records[record].box), b.box, b.count + 1, b.count,
                          true};
      node = impl_->lower_cost(cb, ca) ? right : left;
    }
    if (impl_->nodes[node].used < 4) {
      auto &leaf = impl_->nodes[node];
      leaf.records[leaf.used++] = record;
      impl_->records[record].node = node;
      impl_->refit(node);
    } else {
      std::array<uint32_t, 5> values{};
      for (uint32_t i = 0; i < 4; ++i)
        values[i] = impl_->nodes[node].records[i];
      values[4] = record;
      const auto whole = joined(impl_->nodes[node].box, impl_->records[record].box);
      const auto axis = impl_->longest_axis(whole);
      std::sort(values.begin(), values.end(), [&](uint32_t a, uint32_t b) {
        impl_->budget.charge(1);
        const int c = impl_->center_compare(impl_->records[a].box, impl_->records[b].box, axis);
        return c < 0 || (c == 0 && impl_->records[a].handle < impl_->records[b].handle);
      });
      const auto left = impl_->allocate_node(), right = impl_->allocate_node();
      impl_->nodes[left].parent = node;
      impl_->nodes[right].parent = node;
      impl_->leaf(left, std::span<const uint32_t>(values).first(2));
      impl_->leaf(right, std::span<const uint32_t>(values).subspan(2));
      impl_->nodes[node].used = 0;
      impl_->nodes[node].left = left;
      impl_->nodes[node].right = right;
      impl_->refit(node);
    }
    impl_->maybe_rebuild();
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
void FacetBVH::erase(FacetHandle handle) {
  impl_->check();
  const auto record = impl_->lookup(handle.slot.value);
  if (record == missing || impl_->records[record].handle != handle)
    invalid("stale facet handle");
  grow(impl_->free_nodes, 2);
  grow(impl_->free_records, 1);
  try {
    const auto node = impl_->records[record].node;
    auto &leaf = impl_->nodes[node];
    uint32_t at = 0;
    while (at < leaf.used && leaf.records[at] != record)
      ++at;
    if (at == leaf.used)
      internal("facet absent from BVH leaf");
    leaf.records[at] = leaf.records[--leaf.used];
    impl_->records[record].live = false;
    impl_->free_records.push_back(record);
    impl_->erase_index(handle.slot.value);
    --impl_->counters.live_facets;
    ++impl_->counters.updates;
    ++impl_->since_build;
    if (leaf.used != 0)
      impl_->refit(node);
    else {
      const auto parent = leaf.parent;
      leaf.live = false;
      impl_->free_nodes.push_back(node);
      if (parent == missing)
        impl_->root = missing;
      else {
        const auto grand = impl_->nodes[parent].parent;
        const auto sibling = impl_->nodes[parent].left == node ? impl_->nodes[parent].right
                                                               : impl_->nodes[parent].left;
        impl_->nodes[sibling].parent = grand;
        if (grand == missing)
          impl_->root = sibling;
        else {
          if (impl_->nodes[grand].left == parent)
            impl_->nodes[grand].left = sibling;
          else if (impl_->nodes[grand].right == parent)
            impl_->nodes[grand].right = sibling;
          else
            internal("BVH lost parent link");
        }
        impl_->nodes[parent].live = false;
        impl_->free_nodes.push_back(parent);
        if (grand != missing)
          impl_->refit(grand);
      }
    }
    impl_->maybe_rebuild();
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
std::pmr::vector<FacetHandle> FacetBVH::query(const FacetBox &box) const {
  impl_->check();
  validate_box({box.lower, box.upper});
  std::pmr::vector<FacetHandle> result(budget_);
  std::pmr::vector<uint32_t> stack(budget_);
  if (impl_->root != missing)
    stack.push_back(impl_->root);
  const Box query{box.lower, box.upper};
  uint64_t visits = 0;
  while (!stack.empty()) {
    budget_->charge(1);
    const auto index = stack.back();
    stack.pop_back();
    if (++visits > impl_->nodes.size() || index >= impl_->nodes.size() || !impl_->nodes[index].live)
      internal("BVH query cycle");
    ++impl_->counters.visits;
    const auto &node = impl_->nodes[index];
    ++impl_->counters.box_tests;
    if (!overlaps(query, node.box))
      continue;
    if (node.leaf()) {
      for (uint32_t i = 0; i < node.used; ++i) {
        budget_->charge(1);
        const auto &row = impl_->records[node.records[i]];
        ++impl_->counters.box_tests;
        if (overlaps(query, row.box))
          result.push_back(row.handle);
      }
    } else {
      stack.push_back(node.right);
      stack.push_back(node.left);
    }
  }
  std::sort(result.begin(), result.end(), [&](const auto &a, const auto &b) {
    budget_->charge(1);
    return a < b;
  });
  return result;
}
BVHStats FacetBVH::stats() const {
  impl_->check();
  return impl_->counters;
}
} // namespace bgmesh
