#include "domain.hpp"
#include "face_seed.hpp"
#include "geometry_boundary.hpp"
#include "geometry_domain.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_profile.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
namespace bgmesh {
namespace {
using namespace geometry_detail;
using UV = std::array<Rational, 2>;
using E2 = std::array<ExactEnclosure, 2>;
using Box = std::array<Rational, 4>;
using DBox = std::array<double, 4>;
struct Split final {
  BoundaryKey feature;
};
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void unsupported(const char *message) { throw Error(ErrorCode::unsupported, message); }
Rational abs(const Rational &x) { return x.sign() < 0 ? x.negative() : Rational(x); }
Rational max(const Rational &a, const Rational &b) {
  return a.compare(b) > 0 ? Rational(a) : Rational(b);
}
Rational min(const Rational &a, const Rational &b) {
  return a.compare(b) < 0 ? Rational(a) : Rational(b);
}
Rational dot(const UV &a, const UV &b) { return a[0].multiply(b[0]).add(a[1].multiply(b[1])); }
Rational cross(const UV &a, const UV &b) {
  return a[0].multiply(b[1]).subtract(a[1].multiply(b[0]));
}
UV subtract(const UV &a, const UV &b) { return {a[0].subtract(b[0]), a[1].subtract(b[1])}; }
UV point(const std::array<Rational, 4> &q, std::size_t i) { return {q[2 * i], q[2 * i + 1]}; }
ExactEnclosure project(const UV &d, const E2 &p) {
  return sum(times(p[0], d[0]), times(p[1], d[1]));
}
Rational turn(const UV &a, const UV &b, const UV &c) {
  return cross(subtract(b, a), subtract(c, a));
}
ExactEnclosure turn(const UV &a, const UV &b, const E2 &c) {
  const auto d = subtract(b, a);
  return difference(times(difference(c[1], ExactEnclosure(a[1])), d[0]),
                    times(difference(c[0], ExactEnclosure(a[0])), d[1]));
}
void hull(E2 &a, const E2 &b) {
  for (std::size_t k = 0; k < 2; ++k) {
    if (b[k].lower.compare(a[k].lower) < 0)
      a[k].lower = b[k].lower;
    if (b[k].upper.compare(a[k].upper) > 0)
      a[k].upper = b[k].upper;
  }
}
Box box(const E2 &p) { return {p[0].lower, p[0].upper, p[1].lower, p[1].upper}; }
DBox outward(const Box &b) {
  return {b[0].bounds().lower, b[1].bounds().upper, b[2].bounds().lower, b[3].bounds().upper};
}
bool overlaps(const DBox &a, const DBox &b) {
  return a[0] <= b[1] && b[0] <= a[1] && a[2] <= b[3] && b[2] <= a[3];
}
bool overlaps(const Box &a, const Box &b) {
  return a[0].compare(b[1]) <= 0 && b[0].compare(a[1]) <= 0 && a[2].compare(b[3]) <= 0 &&
         b[2].compare(a[3]) <= 0;
}
class BoxIndex final {
  bgfd::Budget &budget_;
  std::span<const DBox> boxes_;
  uint32_t build(std::size_t first, std::size_t end, uint32_t depth) {
    budget_.charge(1);
    if (nodes.size() >= UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "domain index exhausted");
    const auto ordinal = static_cast<uint32_t>(nodes.size());
    auto b = boxes_[order[first]];
    for (auto i = first + 1; i < end; ++i)
      for (std::size_t a = 0; a < 2; ++a) {
        budget_.charge(1);
        const auto &q = boxes_[order[i]];
        b[2 * a] = std::min(b[2 * a], q[2 * a]);
        b[2 * a + 1] = std::max(b[2 * a + 1], q[2 * a + 1]);
      }
    nodes.push_back({b, 0, 0, static_cast<uint32_t>(first), static_cast<uint32_t>(end - first)});
    if (end - first <= 8)
      return ordinal;
    const auto mid = first + (end - first) / 2, axis = static_cast<std::size_t>(depth % 2) * 2;
    std::nth_element(order.begin() + static_cast<std::ptrdiff_t>(first),
                     order.begin() + static_cast<std::ptrdiff_t>(mid),
                     order.begin() + static_cast<std::ptrdiff_t>(end), [&](uint32_t a, uint32_t c) {
                       budget_.charge(1);
                       return boxes_[a][axis] == boxes_[c][axis]
                                  ? a < c
                                  : boxes_[a][axis] < boxes_[c][axis];
                     });
    const auto l = build(first, mid, depth + 1), r = build(mid, end, depth + 1);
    nodes[ordinal].left = l;
    nodes[ordinal].right = r;
    nodes[ordinal].count = 0;
    return ordinal;
  }

public:
  std::pmr::vector<uint32_t> order;
  std::pmr::vector<DomainSupportNode> nodes;
  BoxIndex(bgfd::Budget &b, std::span<const DBox> boxes)
      : budget_(b), boxes_(boxes), order(&b), nodes(&b) {
    if (boxes.size() > UINT32_MAX / 2)
      throw Error(ErrorCode::resource_exhausted, "domain index capacity exhausted");
    order.reserve(boxes.size());
    for (std::size_t i = 0; i < boxes.size(); ++i)
      order.push_back(static_cast<uint32_t>(i));
    if (!boxes.empty())
      static_cast<void>(build(0, boxes.size(), 0));
  }
  [[nodiscard]] std::pmr::vector<uint32_t> query(const DBox &b) const {
    std::pmr::vector<uint32_t> out(&budget_), stack(&budget_);
    if (!nodes.empty())
      stack.push_back(0);
    while (!stack.empty()) {
      const auto i = stack.back();
      stack.pop_back();
      budget_.charge(1);
      const auto &n = nodes[i];
      if (!overlaps(n.box, b))
        continue;
      if (n.count == 0) {
        stack.push_back(n.left);
        stack.push_back(n.right);
      } else
        for (uint32_t j = 0; j < n.count; ++j) {
          const auto k = order[n.first + j];
          if (overlaps(boxes_[k], b))
            out.push_back(k);
        }
    }
    return out;
  }
};
struct Fan final {
  Slot<CoedgeTag> use;
  Rational parameter;
  uint32_t site;
  UV q;
  E2 a;
  std::array<Rational, 8> polygon;
  BoundaryKey feature;
};
struct Strip final {
  uint32_t arc;
  UV origin, d, x, y;
  Rational slope, c, w0;
  std::array<Rational, 8> polygon;
};
Box polygon_box(const std::array<Rational, 8> &p) {
  Box b{p[0], p[0], p[1], p[1]};
  for (std::size_t i = 1; i < 4; ++i) {
    b[0] = min(b[0], p[2 * i]);
    b[1] = max(b[1], p[2 * i]);
    b[2] = min(b[2], p[2 * i + 1]);
    b[3] = max(b[3], p[2 * i + 1]);
  }
  return b;
}
UV vertex(const std::array<Rational, 8> &p, std::size_t i) { return {p[2 * i], p[2 * i + 1]}; }
bool equal(const UV &a, const UV &b) { return a[0].compare(b[0]) == 0 && a[1].compare(b[1]) == 0; }
bool in_polygon(const UV &p, const std::array<Rational, 8> &polygon, bool strict = false) {
  for (std::size_t i = 0; i < 4; ++i) {
    const auto s = turn(vertex(polygon, i), vertex(polygon, (i + 1) % 4), p).sign();
    if (strict ? s <= 0 : s < 0)
      return false;
  }
  return true;
}
bool on_segment(const UV &p, const UV &a, const UV &b) {
  if (turn(a, b, p).sign() != 0)
    return false;
  return p[0].compare(min(a[0], b[0])) >= 0 && p[0].compare(max(a[0], b[0])) <= 0 &&
         p[1].compare(min(a[1], b[1])) >= 0 && p[1].compare(max(a[1], b[1])) <= 0;
}
bool proper_cross(const UV &a, const UV &b, const UV &c, const UV &d) {
  return turn(a, b, c).sign() * turn(a, b, d).sign() < 0 &&
         turn(c, d, a).sign() * turn(c, d, b).sign() < 0;
}
bool allowed(const UV &p, std::span<const UV> permitted) {
  return std::any_of(permitted.begin(), permitted.end(),
                     [&](const auto &q) { return equal(p, q); });
}
bool segment_conflict(const UV &a, const UV &b, const std::array<Rational, 8> &p,
                      std::span<const UV> permitted) {
  for (const auto &q : {a, b})
    if (in_polygon(q, p) && (!allowed(q, permitted) || in_polygon(q, p, true)))
      return true;
  for (std::size_t i = 0; i < 4; ++i) {
    const auto c = vertex(p, i), d = vertex(p, (i + 1) % 4);
    if (proper_cross(a, b, c, d))
      return true;
    if (on_segment(c, a, b) && !allowed(c, permitted))
      return true;
  }
  return false;
}
bool polygon_conflict(const std::array<Rational, 8> &a, const std::array<Rational, 8> &b,
                      std::span<const UV> permitted = {}) {
  for (std::size_t i = 0; i < 4; ++i)
    if (segment_conflict(vertex(a, i), vertex(a, (i + 1) % 4), b, permitted) ||
        segment_conflict(vertex(b, i), vertex(b, (i + 1) % 4), a, permitted))
      return true;
  return false;
}
struct SourceCurve final {
  GeometryContext &ctx;
  const WorkingChart &chart;
  Slot<CoedgeTag> use;
  uint32_t precision_bits;
  [[nodiscard]] Slot<CurveTag> slot() const {
    const auto &s = chart.envelope.geometry.view().source.view();
    return s.curve_index.find(s.coedges[use.value].curve_id);
  }
  [[nodiscard]] E2 lift(const std::array<ExactEnclosure, 3> &coordinates) const {
    const auto &s = chart.envelope.geometry.view().source.view();
    const auto &u = s.coedges[use.value];
    auto &b = ctx.budget();
    const auto coordinate = [&](std::size_t k) {
      if (u.periodic_shift[k] == 0)
        return coordinates[k];
      const auto kind = s.carriers[chart.envelope.carrier.value].kind;
      if (!(k == 0 ? (kind >= 2 && kind <= 5) : kind == 5))
        unsupported("authoritative use lacks explicit source periodic metadata");
      const auto shift = Rational(b, static_cast<int64_t>(u.periodic_shift[k]) * 2);
      return sum(coordinates[k], times(ctx.pi(precision() + 64), shift));
    };
    return {coordinate(0), coordinate(1)};
  }
  [[nodiscard]] uint32_t precision() const { return precision_bits; }
  [[nodiscard]] E2 range(const Rational &a, const Rational &b) const {
    return lift(enclose_original_curve_range(ctx, chart.envelope.geometry, slot(),
                                             {min(a, b), max(a, b)}, precision())
                    .coordinates);
  }
  [[nodiscard]] std::array<Rational, 2> direction(const DomainArcInput &arc) const {
    return subtract(point(arc.selected, 1), point(arc.selected, 0));
  }
};
struct PieceJet final {
  E2 position, first;
};
PieceJet original_jet(const SourceCurve &curve, const Rational &a, const Rational &b) {
  const auto jet = enclose_original_curve_jet(curve.ctx, curve.chart.envelope.geometry,
                                              curve.slot(), a, b, curve.precision());
  return {curve.lift(jet.position), {jet.first[0], jet.first[1]}};
}
std::pmr::vector<Rational> partitions(const SourceCurve &curve, const DomainArcInput &arc) {
  auto &b = curve.ctx.budget();
  std::pmr::vector<Rational> cuts(&b);
  const auto lo = min(arc.pcurve_parameter[0], arc.pcurve_parameter[1]),
             hi = max(arc.pcurve_parameter[0], arc.pcurve_parameter[1]);
  if (lo.compare(hi) >= 0)
    throw Split{arc.feature};
  cuts.push_back(lo);
  cuts.push_back(hi);
  const auto &g = curve.chart.envelope.geometry;
  const auto &row = g.view().curves[curve.slot().value];
  if (row.kind >= 4)
    for (const auto slot : intersecting_curve_roots(b, g, curve.slot(), {lo, hi})) {
      const auto root = import_curve_root(b, g, slot);
      for (const auto &x : {root.begin, root.end})
        if (x.compare(lo) > 0 && x.compare(hi) < 0)
          cuts.push_back(x);
    }
  std::sort(cuts.begin(), cuts.end(),
            [](const auto &a, const auto &c) { return a.compare(c) < 0; });
  cuts.erase(std::unique(cuts.begin(), cuts.end(),
                         [](const auto &a, const auto &c) { return a.compare(c) == 0; }),
             cuts.end());
  return cuts;
}
E2 transformed_first(const PieceJet &jet, const std::pmr::vector<Fan> &fans,
                     const BoxIndex &index) {
  E2 out = jet.first;
  const auto candidates = index.query(outward(box(jet.position)));
  for (const auto ordinal : candidates) {
    const auto &fan = fans[ordinal];
    if (!overlaps(box(jet.position), polygon_box(fan.polygon)))
      continue;
    const E2 delta{difference(ExactEnclosure(fan.q[0]), fan.a[0]),
                   difference(ExactEnclosure(fan.q[1]), fan.a[1])};
    for (std::size_t i = 0; i < 4; ++i) {
      const auto a = vertex(fan.polygon, i), c = vertex(fan.polygon, (i + 1) % 4);
      const auto denominator = turn(a, c, fan.a);
      if (denominator.lower.sign() <= 0)
        throw Split{fan.feature};
      const UV gradient{a[1].subtract(c[1]), c[0].subtract(a[0])};
      const auto lambda = quotient(project(gradient, jet.first), denominator);
      const E2 moved{sum(jet.first[0], product(delta[0], lambda)),
                     sum(jet.first[1], product(delta[1], lambda))};
      hull(out, moved);
    }
  }
  return out;
}
bool line_image(const SourceCurve &curve, const DomainArcInput &arc) {
  const auto &geometry = curve.chart.envelope.geometry;
  const auto &source = geometry.view().source.view();
  const auto &use = source.coedges[arc.use.value];
  if (use.periodic_shift != std::array<int32_t, 2>{0, 0})
    return false;
  const auto &row = source.curves[curve.slot().value];
  const auto q = point(arc.selected, 0), d = subtract(point(arc.selected, 1), q);
  const auto norm = dot(d, d);
  if (norm.sign() == 0)
    return false;
  const UV normal{d[1].negative(), d[0]};
  const auto a = curve.range(arc.pcurve_parameter[0], arc.pcurve_parameter[0]),
             c = curve.range(arc.pcurve_parameter[1], arc.pcurve_parameter[1]);
  for (std::size_t k = 0; k < 2; ++k)
    if (a[k].lower.compare(a[k].upper) != 0 || c[k].lower.compare(c[k].upper) != 0)
      return false;
  if (dot(normal, subtract(UV{a[0].lower, a[1].lower}, q)).sign() != 0 ||
      dot(normal, subtract(UV{c[0].lower, c[1].lower}, q)).sign() != 0)
    return false;
  if (row.kind == 1)
    return true;
  if (row.kind < 4)
    return false;
  const auto cuts = partitions(curve, arc);
  auto &b = curve.ctx.budget();
  for (std::size_t k = 1; k < cuts.size(); ++k) {
    const auto mid = cuts[k - 1].add(cuts[k]).divide(Rational(b, 2));
    const auto root =
        restrict_curve(import_curve_root(b, geometry, find_curve_root(geometry, curve.slot(), mid)),
                       cuts[k - 1], cuts[k]);
    std::pmr::vector<Rational> scalar(&b), weights(&b);
    const auto stride = static_cast<std::size_t>(root.dimension) + 1;
    for (std::size_t i = 0; i < root.controls.size(); i += stride) {
      const auto &w = root.controls[i + 2];
      const UV delta{root.controls[i].subtract(w.multiply(q[0])),
                     root.controls[i + 1].subtract(w.multiply(q[1]))};
      if (dot(normal, delta).sign() != 0)
        return false;
      auto value = dot(d, delta);
      if (arc.pcurve_parameter[1].compare(arc.pcurve_parameter[0]) < 0)
        value = value.negative();
      scalar.push_back(std::move(value));
      weights.push_back(w);
    }
    if (!ratio_increasing(scalar, weights))
      return false;
  }
  return true;
}
struct Builder final {
  GeometryContext &ctx;
  const DomainMapRequest &request;
  bgfd::Budget &budget;
  std::pmr::vector<Fan> fans;
  std::pmr::vector<Strip> strips;
  std::pmr::vector<E2> source_boxes;
  std::pmr::vector<DBox> arc_boxes;
  std::pmr::vector<std::array<uint32_t, 2>> incident;
  uint64_t pieces = 0;
  Builder(GeometryContext &c, const DomainMapRequest &r)
      : ctx(c), request(r), budget(c.budget()), fans(&budget), strips(&budget),
        source_boxes(&budget), arc_boxes(&budget), incident(&budget) {}
  [[nodiscard]] const SourceData &source() const {
    return request.chart.envelope.geometry.view().source.view();
  }
  [[nodiscard]] bool authoritative(uint32_t arc) const {
    return !source().coedges[request.arcs[arc].use.value].reconstructed;
  }
  [[nodiscard]] SourceCurve curve(uint32_t arc) const {
    return {ctx, request.chart, request.arcs[arc].use, request.precision.transcendental_bits};
  }
  void admit() {
    const auto &r = request;
    if (!r.resource || r.chart.envelope.face.value >= source().faces.size() ||
        r.definition.value >= source().definitions.size() ||
        source().faces[r.chart.envelope.face.value].definition_id !=
            source().definitions[r.definition.value].id ||
        r.generations[0] != r.chart.envelope.geometry.view().table.key().generation ||
        r.generations[1] != r.chart.envelope.generation ||
        r.table.owner != OwnerKey{OwnerKind::face, r.definition.value,
                                  source().faces[r.chart.envelope.face.value].id.value} ||
        r.table.generation != r.generations[1] || r.arcs.size() > UINT32_MAX / 2)
      invalid("domain source/owner/generation admission failed");
    if (r.precision.transcendental_bits < 32 ||
        r.precision.transcendental_bits > ctx.maximum_bits() ||
        r.precision.norm_fractional_bits > ctx.maximum_bits())
      invalid("domain precision profile invalid");

    for (const auto &a : r.arcs) {
      budget.charge(1);
      if (a.use.value >= source().coedges.size() ||
          a.feature.kind != BoundaryOwnerKind::source_edge ||
          a.feature.owner != source().coedges[a.use.value].edge_id.value)
        invalid("domain arc source feature mismatch");
      const auto &u = source().coedges[a.use.value];
      if (source().loops[source().loop_index.find(u.loop_id).value].face_id !=
          source().faces[r.chart.envelope.face.value].id)
        invalid("domain arc belongs to foreign face");
      for (const auto &x : a.selected)
        if (&x.budget() != &budget)
          invalid("domain coordinate owner mismatch");
      for (const auto &x : a.pcurve_parameter)
        if (&x.budget() != &budget)
          invalid("domain pcurve parameter owner mismatch");
      for (const auto &x : a.source_parameter)
        if (&x.budget() != &budget)
          invalid("domain source parameter owner mismatch");
      if (a.source_parameter[0].compare(a.source_parameter[1]) == 0)
        invalid("ordinary domain arc has zero source extent");
      for (std::size_t endpoint = 0; endpoint < 2; ++endpoint)
        if (map_original_use_parameter(ctx, r.chart, {a.use, a.source_parameter[endpoint]})
                .compare(a.pcurve_parameter[endpoint]) != 0)
          invalid("domain mapped pcurve parameter differs from original source map");
    }
    // Arc rows follow the complete original face loop/coedge inventory.  A map
    // cannot silently omit a hole, reorder uses, or jump over a source-map piece.
    std::size_t cursor = 0;
    const auto &face = source().faces[r.chart.envelope.face.value];
    for (uint64_t li = 0; li < face.loops.count; ++li) {
      const auto &loop =
          source().loops[source().loop_index.find(source().loop_ids[face.loops.first + li]).value];
      const auto first_arc = cursor;
      for (uint64_t ui = 0; ui < loop.coedges.count; ++ui) {
        const auto slot = source().coedge_index.find(source().coedge_ids[loop.coedges.first + ui]);
        const auto &use = source().coedges[slot.value];
        const auto &edge = source().edges[source().edge_index.find(use.edge_id).value];
        if (use.sense != 1 && use.sense != -1)
          invalid("domain original use sense invalid");
        if (edge.degenerate)
          unsupported("domain quotient use requires explicit mapped coverage");
        const auto from = use.sense == 1 ? 0U : 1U;
        auto current = Rational::from_double(budget, edge.parameter_range[from]);
        const auto finish = Rational::from_double(budget, edge.parameter_range[1 - from]);
        const auto direction = current.compare(finish);
        if (direction == 0)
          invalid("domain ordinary source use has zero extent");
        const auto first = cursor;
        while (cursor < r.arcs.size() && r.arcs[cursor].use == slot) {
          const auto &arc = r.arcs[cursor];
          if (arc.source_parameter[0].compare(current) != 0 ||
              arc.source_parameter[1].compare(current) != -direction ||
              arc.source_parameter[1].compare(finish) * direction < 0)
            invalid("domain original use intervals are not contiguous full ordered coverage");
          if (use.parameter_map_kind == 2) {
            const auto map_direction = edge.parameter_range[0] < edge.parameter_range[1] ? 1 : -1;
            const auto lo = map_direction > 0 ? arc.source_parameter[0] : arc.source_parameter[1];
            const auto hi = map_direction > 0 ? arc.source_parameter[1] : arc.source_parameter[0];
            // Search by numerical t, accounting for descending original map rows.
            std::size_t left = 1, right = static_cast<std::size_t>(use.parameter_map.count) - 1;
            while (left < right) {
              budget.charge(1);
              const auto mid = left + (right - left) / 2;
              const auto row = map_direction > 0 ? mid : use.parameter_map.count - 1 - mid;
              const auto t = Rational::from_double(
                  budget, source().parameter_maps[use.parameter_map.first + row][0]);
              if (t.compare(min(lo, hi)) <= 0)
                left = mid + 1;
              else
                right = mid;
            }
            if (left + 1 < use.parameter_map.count) {
              const auto row = map_direction > 0 ? left : use.parameter_map.count - 1 - left;
              const auto t = Rational::from_double(
                  budget, source().parameter_maps[use.parameter_map.first + row][0]);
              if (t.compare(min(lo, hi)) > 0 && t.compare(max(lo, hi)) < 0)
                invalid("domain arc omits mandatory original parameter-map partition");
            }
          }
          current = arc.source_parameter[1];
          ++cursor;
        }
        if (cursor == first || current.compare(finish) != 0)
          invalid("domain omits or truncates an original face use");
      }
      if (cursor == first_arc)
        invalid("domain original loop has no ordinary coverage");
      for (auto i = first_arc; i < cursor; ++i) {
        const auto next = i + 1 == cursor ? first_arc : i + 1;
        if (r.arcs[i].end_site != r.arcs[next].begin_site)
          invalid("domain arcs reorder original source loop incidence");
      }
    }
    if (cursor != r.arcs.size())
      invalid("domain contains extra or reordered original face uses");
    incident.assign(r.arcs.size(), {UINT32_MAX, UINT32_MAX});
    for (uint32_t i = 0; i < r.arcs.size(); ++i) {
      const auto &a = r.arcs[i];
      if (a.begin_site >= incident.size() || a.end_site >= incident.size() ||
          incident[a.begin_site][0] != UINT32_MAX || incident[a.end_site][1] != UINT32_MAX)
        invalid("domain ordinary site incidence is not a complete disjoint loop inventory");
      incident[a.begin_site][0] = i;
      incident[a.end_site][1] = i;
    }
    for (const auto &site : incident)
      if (site[0] == UINT32_MAX || site[1] == UINT32_MAX)
        invalid("domain source site is unreferenced or open");
  }
  void source_index() {
    for (uint32_t i = 0; i < request.arcs.size(); ++i) {
      const auto &arc = request.arcs[i];
      if (authoritative(i))
        source_boxes.push_back(curve(i).range(arc.pcurve_parameter[0], arc.pcurve_parameter[1]));
      else {
        const auto a = point(arc.selected, 0), b = point(arc.selected, 1);
        source_boxes.push_back({ExactEnclosure(min(a[0], b[0]), max(a[0], b[0])),
                                ExactEnclosure(min(a[1], b[1]), max(a[1], b[1]))});
      }
      arc_boxes.push_back(outward(box(source_boxes.back())));
    }
  }
  bool original_outside(uint32_t arc, const Rational &a, const Rational &b, const Box &p,
                        uint32_t depth) {
    if (++pieces > request.limits.interval_pieces)
      throw Error(ErrorCode::resource_exhausted,
                  "domain original curve exclusion budget exhausted");
    const auto range = curve(arc).range(a, b);
    if (!overlaps(box(range), p))
      return true;
    if (depth >= request.limits.curve_subdivisions)
      return false;
    const auto mid = a.add(b).divide(Rational(budget, 2));
    if (mid.compare(a) == 0 || mid.compare(b) == 0)
      return false;
    return original_outside(arc, a, mid, p, depth + 1) &&
           original_outside(arc, mid, b, p, depth + 1);
  }
  void build_fans(const BoxIndex &index) {
    for (uint32_t site = 0; site < incident.size(); ++site) {
      const auto outgoing = incident[site][0];
      const auto &arc = request.arcs[outgoing];
      std::optional<std::pair<uint32_t, uint32_t>> authority;
      Rational radius(budget, 0);
      BoundaryKey feature = arc.feature;
      const auto q = point(arc.selected, 0);
      for (const auto j : incident[site]) {
        const auto &other = request.arcs[j];
        const auto end = other.end_site == site ? 1U : 0U;
        if (!equal(q, point(other.selected, end)))
          invalid("domain site has inconsistent selected coordinates");
        feature = std::min(feature, other.feature);
        const auto d = subtract(point(other.selected, 1 - end), q);
        const auto length = max(abs(d[0]), abs(d[1]));
        if (length.sign() > 0 && (radius.sign() == 0 || length.compare(radius) < 0))
          radius = length;
        if (authoritative(j) && !authority)
          authority.emplace(j, end);
      }
      if (!authority)
        continue;
      const auto [owner, end] = *authority;
      const auto &original = request.arcs[owner];
      const auto parameter = original.pcurve_parameter[end];
      for (const auto other : incident[site])
        if (authoritative(other)) {
          const auto &adjacent = request.arcs[other];
          const auto adjacent_end = adjacent.end_site == site ? 1U : 0U;
          require_authoritative_point_agreement(
              ctx, request.chart,
              {{{original.use, original.source_parameter[end]},
                {adjacent.use, adjacent.source_parameter[adjacent_end]}}},
              request.precision);
        }
      const auto a = curve(owner).range(parameter, parameter);
      bool exact = true;
      for (std::size_t k = 0; k < 2; ++k)
        exact = exact && a[k].lower.compare(q[k]) == 0 && a[k].upper.compare(q[k]) == 0;
      if (exact)
        continue;
      if (radius.sign() == 0)
        throw Split{feature};
      radius = radius.divide(Rational(budget, 16));
      bool accepted = false;
      for (uint32_t attempt = 0; attempt < request.limits.star_shrinks; ++attempt) {
        const Box p{q[0].subtract(radius), q[0].add(radius), q[1].subtract(radius),
                    q[1].add(radius)};
        if (a[0].lower.compare(p[0]) <= 0 || a[0].upper.compare(p[1]) >= 0 ||
            a[1].lower.compare(p[2]) <= 0 || a[1].upper.compare(p[3]) >= 0)
          break;
        const std::array<Rational, 8> polygon{p[0], p[2], p[1], p[2], p[1], p[3], p[0], p[3]};
        bool clear = true;
        if (clear)
          for (const auto j : index.query(outward(p))) {
            const auto &other = request.arcs[j];
            if (other.begin_site == site || other.end_site == site)
              continue;
            if (authoritative(j)) {
              if (!original_outside(j, other.pcurve_parameter[0], other.pcurve_parameter[1], p,
                                    0)) {
                clear = false;
                break;
              }
            } else if (segment_conflict(point(other.selected, 0), point(other.selected, 1), polygon,
                                        {})) {
              clear = false;
              break;
            }
          }
        if (clear) {
          fans.push_back({original.use, parameter, site, q, a, polygon, feature});
          accepted = true;
          break;
        }
        radius = radius.divide(Rational(budget, 2));
      }
      if (!accepted)
        throw Split{feature};
    }
  }
  void separate_fans() {
    for (uint32_t round = 0; round < request.limits.star_shrinks; ++round) {
      std::pmr::vector<DBox> boxes(&budget);
      for (const auto &fan : fans)
        boxes.push_back(outward(polygon_box(fan.polygon)));
      const BoxIndex index(budget, boxes);
      std::pmr::vector<uint8_t> shrink(fans.size(), 0, &budget);
      bool conflict = false;
      for (uint32_t i = 0; i < fans.size(); ++i)
        for (const auto j : index.query(boxes[i]))
          if (j > i && polygon_conflict(fans[i].polygon, fans[j].polygon)) {
            shrink[i] = 1;
            shrink[j] = 1;
            conflict = true;
          }
      if (!conflict)
        return;
      for (std::size_t i = 0; i < fans.size(); ++i)
        if (shrink[i]) {
          auto &fan = fans[i];
          const auto radius = fan.polygon[2].subtract(fan.polygon[0]).divide(Rational(budget, 4));
          const Box p{fan.q[0].subtract(radius), fan.q[0].add(radius), fan.q[1].subtract(radius),
                      fan.q[1].add(radius)};
          if (fan.a[0].lower.compare(p[0]) <= 0 || fan.a[0].upper.compare(p[1]) >= 0 ||
              fan.a[1].lower.compare(p[2]) <= 0 || fan.a[1].upper.compare(p[3]) >= 0)
            throw Split{fan.feature};
          fan.polygon = {p[0], p[2], p[1], p[2], p[1], p[3], p[0], p[3]};
        }
    }
    if (!fans.empty())
      throw Split{fans.front().feature};
  }
  Rational slope(uint32_t arc, const Rational &a, const Rational &b, const UV &x, const UV &y,
                 const BoxIndex &index, uint32_t depth) {
    if (++pieces > request.limits.interval_pieces)
      throw Error(ErrorCode::resource_exhausted, "domain derivative restriction budget exhausted");
    const auto jet = original_jet(curve(arc), a, b);
    auto first = transformed_first(jet, fans, index);
    const auto &row = request.arcs[arc];
    const auto sign =
        Rational(budget, row.pcurve_parameter[1].compare(row.pcurve_parameter[0]) > 0 ? 1 : -1);
    const auto dx = times(project(x, first), sign), dy = project(y, first);
    if (dx.lower.sign() > 0)
      return max(abs(dy.lower), abs(dy.upper)).divide(dx.lower);
    if (depth >= request.limits.curve_subdivisions)
      throw Split{row.feature};
    const auto middle = a.add(b).divide(Rational(budget, 2));
    return max(slope(arc, a, middle, x, y, index, depth + 1),
               slope(arc, middle, b, x, y, index, depth + 1));
  }
  void build_strips(const BoxIndex &index) {
    for (uint32_t i = 0; i < request.arcs.size(); ++i) {
      if (!authoritative(i))
        continue;
      const auto &arc = request.arcs[i];
      const auto a = point(arc.selected, 0), d = subtract(point(arc.selected, 1), a);
      const auto norm = dot(d, d);
      if (norm.sign() == 0)
        throw Split{arc.feature};
      if (line_image(curve(i), arc))
        continue;
      const UV x{d[0].divide(norm), d[1].divide(norm)},
          y{d[1].negative().divide(norm), d[0].divide(norm)};
      Rational m(budget, 0);
      const auto cuts = partitions(curve(i), arc);
      for (std::size_t k = 1; k < cuts.size(); ++k)
        m = max(m, slope(i, cuts[k - 1], cuts[k], x, y, index, 0));
      if (m.sign() == 0)
        continue;
      const auto c = m.multiply(Rational(budget, 2)), w0 = m;
      const auto half = Rational(budget, 1).divide(Rational(budget, 2));
      const UV middle{a[0].add(d[0].multiply(half)), a[1].add(d[1].multiply(half))};
      const UV off{d[1].negative().multiply(w0), d[0].multiply(w0)};
      strips.push_back(
          {i,
           a,
           d,
           x,
           y,
           m,
           c,
           w0,
           {a[0], a[1], middle[0].subtract(off[0]), middle[1].subtract(off[1]), a[0].add(d[0]),
            a[1].add(d[1]), middle[0].add(off[0]), middle[1].add(off[1])}});
    }
  }
  void check_strips() {
    std::pmr::vector<DBox> boxes(&budget);
    for (const auto &s : strips)
      boxes.push_back(outward(polygon_box(s.polygon)));
    const BoxIndex index(budget, boxes);
    std::pmr::vector<uint8_t> nontrivial(request.arcs.size(), 0, &budget);
    for (const auto &s : strips)
      nontrivial[s.arc] = 1;
    std::pmr::vector<DBox> selected_boxes(&budget);
    for (const auto &arc : request.arcs) {
      const auto a = point(arc.selected, 0), c = point(arc.selected, 1);
      selected_boxes.push_back(
          outward({min(a[0], c[0]), max(a[0], c[0]), min(a[1], c[1]), max(a[1], c[1])}));
    }
    const BoxIndex selected_index(budget, selected_boxes);
    for (uint32_t i = 0; i < strips.size(); ++i) {
      const auto &s = strips[i];
      const auto &arc = request.arcs[s.arc];
      for (const auto j : index.query(boxes[i]))
        if (j > i) {
          const auto &t = strips[j];
          const auto &other = request.arcs[t.arc];
          std::pmr::vector<UV> common(&budget);
          for (uint32_t a = 0; a < 2; ++a)
            for (uint32_t b = 0; b < 2; ++b)
              if ((a ? arc.end_site : arc.begin_site) == (b ? other.end_site : other.begin_site))
                common.push_back(point(arc.selected, a));
          if (polygon_conflict(s.polygon, t.polygon, common))
            throw Split{std::min(arc.feature, other.feature)};
        }
      // ALL identity reference segments, including authoritative geometric lines.
      for (const auto j : selected_index.query(boxes[i])) {
        if (j == s.arc || nontrivial[j])
          continue;
        const auto &other = request.arcs[j];
        std::pmr::vector<UV> common(&budget);
        for (uint32_t a = 0; a < 2; ++a)
          for (uint32_t b = 0; b < 2; ++b)
            if ((a ? arc.end_site : arc.begin_site) == (b ? other.end_site : other.begin_site))
              common.push_back(point(arc.selected, a));
        if (segment_conflict(point(other.selected, 0), point(other.selected, 1), s.polygon, common))
          throw Split{std::min(arc.feature, other.feature)};
      }
    }
  }
  bool source_inside(uint32_t arc, const Rational &a, const Rational &b, uint32_t depth) {
    if (++pieces > request.limits.interval_pieces)
      throw Error(ErrorCode::resource_exhausted, "domain carrier admission piece budget exhausted");
    const auto range = curve(arc).range(a, b);
    const auto &r = request.chart.envelope.uv;
    if (range[0].lower.compare(r[0]) >= 0 && range[0].upper.compare(r[1]) <= 0 &&
        range[1].lower.compare(r[2]) >= 0 && range[1].upper.compare(r[3]) <= 0)
      return true;
    if (depth >= request.limits.curve_subdivisions)
      return false;
    const auto mid = a.add(b).divide(Rational(budget, 2));
    return source_inside(arc, a, mid, depth + 1) && source_inside(arc, mid, b, depth + 1);
  }
  void reference_admission(const BoxIndex &index) {
    const auto &r = request.chart.envelope.uv;
    for (uint32_t i = 0; i < request.arcs.size(); ++i) {
      const auto &arc = request.arcs[i];
      for (std::size_t end = 0; end < 2; ++end)
        for (std::size_t a = 0; a < 2; ++a)
          if (arc.selected[2 * end + a].compare(r[2 * a]) < 0 ||
              arc.selected[2 * end + a].compare(r[2 * a + 1]) > 0)
            invalid("selected domain leaves prepared source rectangle");
      if (authoritative(i)) {
        if (!source_inside(i, arc.pcurve_parameter[0], arc.pcurve_parameter[1], 0))
          throw Split{arc.feature};
        continue;
      }
      const auto p = source_boxes[i];
      for (const auto j : index.query(outward(box(p)))) {
        const auto &fan = fans[j];
        // Inverse extensions only need hold on the intersection of this selected
        // segment with the convex P sector. Clip its exact parameter first.
        const auto a = point(arc.selected, 0), d = subtract(point(arc.selected, 1), a);
        for (std::size_t k = 0; k < 4; ++k) {
          const auto x = vertex(fan.polygon, k), y = vertex(fan.polygon, (k + 1) % 4);
          Rational lo(budget, 0), hi(budget, 1);
          const std::array<UV, 3> triangle{x, y, fan.q};
          bool empty = false;
          for (std::size_t edge = 0; edge < 3; ++edge) {
            const auto &u = triangle[edge], &v = triangle[(edge + 1) % 3];
            const auto base = turn(u, v, a), slope = cross(subtract(v, u), d);
            if (slope.sign() == 0) {
              if (base.sign() < 0) {
                empty = true;
                break;
              }
              continue;
            }
            const auto t = base.negative().divide(slope);
            if (slope.sign() > 0)
              lo = max(lo, t);
            else
              hi = min(hi, t);
            if (lo.compare(hi) > 0) {
              empty = true;
              break;
            }
          }
          if (empty)
            continue;
          const auto denominator = turn(x, y, fan.q);
          if (denominator.sign() <= 0)
            invalid("inverse fan target orientation invalid");
          for (const auto &t : {lo, hi}) {
            const UV q{a[0].add(d[0].multiply(t)), a[1].add(d[1].multiply(t))};
            const auto lambda = turn(x, y, q).divide(denominator);
            for (std::size_t axis = 0; axis < 2; ++axis) {
              const auto image =
                  sum(ExactEnclosure(q[axis]),
                      times(difference(fan.a[axis], ExactEnclosure(fan.q[axis])), lambda));
              if (image.lower.compare(r[2 * axis]) < 0 || image.upper.compare(r[2 * axis + 1]) > 0)
                throw Split{arc.feature};
            }
          }
        }
      }
    }
  }
  DomainMap seal() {
    ExactBuilder values(request.resource, request.table);
    const auto append = [&]<std::size_t N>(const std::array<Rational, N> &a) {
      std::array<ExactRef, N> out{};
      for (std::size_t i = 0; i < N; ++i)
        out[i] = values.append(a[i]);
      return out;
    };
    std::pmr::vector<FrozenDomainFan> frozen_fans(&budget);
    std::pmr::vector<FrozenDomainStrip> frozen_strips(&budget);
    std::pmr::vector<DBox> boxes(&budget);
    for (const auto &fan : fans) {
      frozen_fans.push_back({fan.use, values.append(fan.parameter), fan.site, false,
                             append(fan.polygon), append(box(fan.a)), fan.feature});
      boxes.push_back(outward(polygon_box(fan.polygon)));
    }
    for (const auto &strip : strips) {
      const auto &arc = request.arcs[strip.arc];
      frozen_strips.push_back({arc.use,
                               {0, fans.size()},
                               append(arc.source_parameter),
                               {arc.begin_site, arc.end_site},
                               append(strip.origin),
                               append(strip.x),
                               append(strip.y),
                               values.append(Rational(budget, 1)),
                               values.append(strip.c),
                               values.append(strip.w0),
                               values.append(strip.slope),
                               arc.feature});
      boxes.push_back(outward(polygon_box(strip.polygon)));
    }
    const auto original_rectangle = append(request.chart.envelope.uv);
    bool mixed = false;
    for (const auto &a : request.arcs)
      mixed = mixed || source().coedges[a.use.value].reconstructed;
    auto result = std::allocate_shared<DomainMapData>(
        RunAllocator<DomainMapData>(request.resource), request.resource,
        request.chart.envelope.geometry, request.definition, request.chart.envelope.face,
        request.generations,
        mixed ? DomainReferencePolicy::hybrid_pullback
              : DomainReferencePolicy::authoritative_original,
        request.precision, values.seal());
    result->fans.assign(frozen_fans.begin(), frozen_fans.end());
    result->strips.assign(frozen_strips.begin(), frozen_strips.end());
    result->original_rectangle = original_rectangle;
    result->chart_kind = request.chart.kind;
    const BoxIndex index(budget, boxes);
    result->support_order.assign(index.order.begin(), index.order.end());
    result->support_index.assign(index.nodes.begin(), index.nodes.end());
    return result;
  }
  DomainMap run() {
    source_index();
    const BoxIndex original(budget, arc_boxes);
    build_fans(original);
    separate_fans();
    std::pmr::vector<DBox> fan_boxes(&budget);
    for (const auto &f : fans)
      fan_boxes.push_back(outward(polygon_box(f.polygon)));
    const BoxIndex fan_index(budget, fan_boxes);
    build_strips(fan_index);
    check_strips();
    reference_admission(fan_index);
    return seal();
  }
};
template <std::size_t N>
std::array<Rational, N> load(bgfd::Budget &b, const ExactTable &table,
                             const std::array<ExactRef, N> &refs) {
  auto result = [&]<std::size_t... I>(std::index_sequence<I...>) {
    return std::array<Rational, N>{(static_cast<void>(I), Rational(b))...};
  }(std::make_index_sequence<N>{});
  for (std::size_t i = 0; i < N; ++i)
    result[i] = import_exact(b, table.view(refs[i]));
  return result;
}
std::pmr::vector<uint32_t> query(bgfd::Budget &b, const DomainMapData &map, const Box &rectangle) {
  const auto box = outward(rectangle);
  std::pmr::vector<uint32_t> out(&b), stack(&b);
  if (!map.support_index.empty())
    stack.push_back(0);
  while (!stack.empty()) {
    const auto i = stack.back();
    stack.pop_back();
    b.charge(1);
    const auto &n = map.support_index[i];
    if (!overlaps(box, n.box))
      continue;
    if (n.count == 0) {
      stack.push_back(n.left);
      stack.push_back(n.right);
    } else
      for (uint32_t j = 0; j < n.count; ++j)
        out.push_back(map.support_order[n.first + j]);
  }
  return out;
}
std::array<Rational, 8> strip_polygon(bgfd::Budget &b, const DomainMapData &map,
                                      const FrozenDomainStrip &strip) {
  const auto origin = load(b, map.values, strip.origin), x = load(b, map.values, strip.x_covector),
             y = load(b, map.values, strip.y_covector);
  const auto det = cross(x, y);
  if (det.sign() <= 0)
    invalid("stored domain chord covectors singular");
  const auto extent = import_exact(b, map.values.view(strip.extent)),
             w = import_exact(b, map.values.view(strip.w0));
  const UV d{y[1].multiply(extent).divide(det), y[0].negative().multiply(extent).divide(det)},
      off{x[1].negative().multiply(w).divide(det), x[0].multiply(w).divide(det)};
  const auto half = Rational(b, 1).divide(Rational(b, 2));
  const UV mid{origin[0].add(d[0].multiply(half)), origin[1].add(d[1].multiply(half))};
  return {
      origin[0],           origin[1],           mid[0].subtract(off[0]), mid[1].subtract(off[1]),
      origin[0].add(d[0]), origin[1].add(d[1]), mid[0].add(off[0]),      mid[1].add(off[1])};
}
} // namespace
DomainMapData::DomainMapData(std::shared_ptr<RunResource> owner, GeometryStore original,
                             Slot<DefinitionTag> definition_slot, Slot<FaceTag> face_slot,
                             std::array<uint64_t, 3> generation, DomainReferencePolicy reference,
                             GeometryBoundPrecision profile, ExactTable table)
    : resource(std::move(owner)), geometry(std::move(original)), definition(definition_slot),
      face(face_slot), generations(generation), policy(reference), precision(profile),
      carrier(geometry.view().source.view().carrier_index.find(
          geometry.view().source.view().faces[face.value].carrier_id)),
      chart_kind(WorkingChartKind::original_uv), values(std::move(table)), fans(resource.get()),
      strips(resource.get()), support_order(resource.get()), support_index(resource.get()) {}
DomainMapResult construct_domain_map(GeometryContext &ctx, const DomainMapRequest &request) {
  const auto &source = request.chart.envelope.geometry.view().source.view();
  bool any = false;
  for (const auto &arc : request.arcs) {
    if (arc.use.value >= source.coedges.size())
      invalid("domain source use slot out of range");
    any = any || !source.coedges[arc.use.value].reconstructed;
  }
  Builder builder(ctx, request);
  builder.admit();
  if (!any)
    return {};
  if (request.chart.kind == WorkingChartKind::physical_plane_convex_q)
    unsupported("authoritative domain construction uses original UV fallback before projected "
                "specialization");
  try {
    return {builder.run(), std::nullopt};
  } catch (const Split &split) {
    return {{}, split.feature};
  }
}
DomainDisplacement bound_domain_displacement(GeometryContext &ctx, const DomainMap &lease,
                                             std::span<const Rational, 6> triangle) {
  auto &b = ctx.budget();
  if (!lease)
    return {Rational(b, 0), std::nullopt};
  const auto &map = *lease;
  for (const auto &x : triangle)
    if (&x.budget() != &b)
      invalid("domain bound triangle owner mismatch");
  if (map.generations[0] != map.geometry.view().table.key().generation)
    invalid("domain bound source generation mismatch");
  Box selected{triangle[0], triangle[0], triangle[1], triangle[1]};
  for (std::size_t i = 1; i < 3; ++i) {
    selected[0] = min(selected[0], triangle[2 * i]);
    selected[1] = max(selected[1], triangle[2 * i]);
    selected[2] = min(selected[2], triangle[2 * i + 1]);
    selected[3] = max(selected[3], triangle[2 * i + 1]);
  }
  const auto r = load(b, map.values, map.original_rectangle);
  for (std::size_t axis = 0; axis < 2; ++axis)
    if (selected[2 * axis].compare(r[2 * axis]) < 0 ||
        selected[2 * axis + 1].compare(r[2 * axis + 1]) > 0)
      invalid("domain cell leaves admitted source rectangle");
  UV tmove{Rational(b), Rational(b)}, bmove{Rational(b), Rational(b)};
  std::optional<BoundaryKey> feature;
  const auto choose = [&](BoundaryKey key) {
    if (!feature || key < *feature)
      feature = key;
  };
  for (const auto ordinal : query(b, map, selected)) {
    if (ordinal < map.fans.size())
      continue;
    const auto &strip = map.strips[ordinal - map.fans.size()];
    if (!overlaps(selected, polygon_box(strip_polygon(b, map, strip))))
      continue;
    const auto x = load(b, map.values, strip.x_covector), y = load(b, map.values, strip.y_covector);
    const auto det = cross(x, y);
    const auto slope = import_exact(b, map.values.view(strip.slope_upper)).divide(Rational(b, 2));
    const UV move{abs(x[1].divide(det)).multiply(slope), abs(x[0].divide(det)).multiply(slope)};
    for (std::size_t axis = 0; axis < 2; ++axis)
      tmove[axis] = max(tmove[axis], move[axis]);
    choose(strip.feature);
  }
  Box before_b{selected[0].subtract(tmove[0]), selected[1].add(tmove[0]),
               selected[2].subtract(tmove[1]), selected[3].add(tmove[1])};
  for (const auto ordinal : query(b, map, before_b)) {
    if (ordinal >= map.fans.size())
      continue;
    const auto &fan = map.fans[ordinal];
    const auto p = load(b, map.values, fan.polygon);
    if (!overlaps(before_b, polygon_box(p)))
      continue;
    const auto a = load(b, map.values, fan.source_enclosure);
    const UV q{p[0].add(p[4]).divide(Rational(b, 2)), p[1].add(p[5]).divide(Rational(b, 2))};
    for (std::size_t axis = 0; axis < 2; ++axis)
      bmove[axis] = max(bmove[axis], max(abs(a[2 * axis].subtract(q[axis])),
                                         abs(a[2 * axis + 1].subtract(q[axis]))));
    choose(fan.feature);
  }
  const UV movement{tmove[0].add(bmove[0]), tmove[1].add(bmove[1])};
  if (movement[0].sign() == 0 && movement[1].sign() == 0)
    return {Rational(b, 0), std::nullopt};
  if (map.chart_kind == WorkingChartKind::physical_plane_convex_q)
    unsupported("projected authoritative domain bound not prepared");
  const Box hull{
      max(r[0], selected[0].subtract(movement[0])), min(r[1], selected[1].add(movement[0])),
      max(r[2], selected[2].subtract(movement[1])), min(r[3], selected[3].add(movement[1]))};
  Rational du(b), dv(b);
  const auto &source = map.geometry.view().source.view();
  const auto fold = [&](const SurfaceJetEnclosure &jet) {
    du = max(du, enclosure_norm_upper(b, jet.du, map.precision.norm_fractional_bits));
    dv = max(dv, enclosure_norm_upper(b, jet.dv, map.precision.norm_fractional_bits));
  };
  if (source.carriers[map.carrier.value].kind == 6) {
    for (const auto root : intersecting_surface_roots(b, map.geometry, map.carrier, hull)) {
      const auto original = import_surface_root(b, map.geometry, root);
      Box cut = hull;
      for (std::size_t axis = 0; axis < 2; ++axis) {
        cut[2 * axis] = max(cut[2 * axis], original.bounds[2 * axis]);
        cut[2 * axis + 1] = min(cut[2 * axis + 1], original.bounds[2 * axis + 1]);
      }
      // A boundary trace may query a degenerate rectangle.  Derivatives on
      // that line are enclosed using the same original root, with only each
      // zero-width axis widened to its original span.  Never drop its motion.
      for (std::size_t axis = 0; axis < 2; ++axis)
        if (cut[2 * axis].compare(cut[2 * axis + 1]) == 0) {
          cut[2 * axis] = original.bounds[2 * axis];
          cut[2 * axis + 1] = original.bounds[2 * axis + 1];
        }
      fold(enclose_rational_surface_jet(original, cut));
    }
  } else
    fold(enclose_analytic_surface_jet(ctx, source.carriers[map.carrier.value], hull,
                                      map.precision.transcendental_bits));
  return {du.multiply(movement[0]).add(dv.multiply(movement[1])), feature};
}
BoundaryKey classify_coarse_domain_issue(bgfd::Budget &budget, const FaceBoundarySnapshotData &seed,
                                         const DomainIssue &issue) {
  const auto &source = seed.geometry.view().source.view();
  if (seed.face.value >= source.faces.size() || seed.trace_maps.size() != seed.segments.size() ||
      seed.source_generation != seed.geometry.view().table.key().generation)
    invalid("coarse domain classification lacks complete source trace inventory");
  std::optional<BoundaryKey> requested;
  bool missing = false, declared = false;
  const auto inspect = [&](uint64_t ordinal) {
    budget.charge(1);
    if (ordinal >= seed.segments.size())
      invalid("coarse domain issue references foreign source segment");
    const auto &segment = seed.segments[ordinal];
    const auto &trace = seed.trace_maps[ordinal];
    if(segment.use.value>=source.coedges.size())invalid("coarse domain trace references foreign source use");
    if (!trace.trace_position_upper) {
      missing = true;
      return;
    }
    const auto upper = import_exact(budget, seed.values.view(*trace.trace_position_upper));
    if (upper.sign() < 0)
      invalid("negative canonical trace bound in coarse domain issue");
    if (upper.sign() > 0 && (!requested || segment.constraint < *requested))
      requested = segment.constraint;
  };
  const auto ordinal = [&](DomainSegmentLocation location) {
    if (location.loop >= seed.loops.size())
      invalid("coarse domain issue loop outside source inventory");
    const auto &loop = seed.loops[location.loop];
    if (location.segment >= loop.segments.count || loop.segments.first > seed.segments.size() ||
        loop.segments.count > seed.segments.size() - loop.segments.first)
      invalid("coarse domain issue segment outside source loop");
    return loop.segments.first + location.segment;
  };
  if (issue.kind == DomainIssueKind::boundary_contact) {
    if (issue.segment_count != 2)
      invalid("coarse boundary contact lacks original occurrence pair");
    const auto first = ordinal(issue.segments[0]), second = ordinal(issue.segments[1]);
    inspect(first);
    inspect(second);
    const auto &a = source.coedges[seed.segments[first].use.value];
    const auto &b = source.coedges[seed.segments[second].use.value];
    const auto &ea = source.edges[source.edge_index.find(a.edge_id).value];
    const auto &eb = source.edges[source.edge_index.find(b.edge_id).value];
    declared = a.edge_id == b.edge_id;
    for (const auto va : ea.vertex_ids)
      for (const auto vb : eb.vertex_ids)
        declared = declared || va == vb;
  } else if (issue.kind == DomainIssueKind::hole_placement) {
    // A coarse outer boundary can displace a straight hole; the selected
    // validator identifies the hole but not the responsible enclosing arc.
    static_cast<void>(ordinal(issue.segments[0]));
    for (uint64_t i = 0; i < seed.segments.size(); ++i)
      inspect(i);
  } else {
    // A representative first segment never decides a collapsed/zero-area loop.
    const auto location = issue.segments[0];
    static_cast<void>(ordinal(location));
    const auto &loop = seed.loops[location.loop];
    for (uint64_t i = 0; i < loop.segments.count; ++i)
      inspect(loop.segments.first + i);
  }
  if (requested)
    return *requested;
  if (missing)
    throw Error(ErrorCode::unresolved,
                "coarse source domain lacks retained whole-trace deviation evidence");
  if (declared)
    unsupported(
        "declared physical contact requires another admitted chart/occurrence representation");
  throw Error(ErrorCode::unresolved,"zero physical trace deviation does not establish a faithful source chart or hole ownership");
}
std::optional<BoundaryKey> protected_domain_point(bgfd::Budget &b, const DomainMap &lease,
                                                  const UV &point) {
  if (!lease)
    return std::nullopt;
  for (const auto &x : point)
    if (&x.budget() != &b)
      invalid("protected domain point owner mismatch");
  const auto &map = *lease;
  std::optional<BoundaryKey> feature;
  const Box rectangle{point[0], point[0], point[1], point[1]};
  const auto choose=[&](BoundaryKey key) {
    if(!feature)feature=key;
    else if(key<feature.value())feature=key;
  };
  for (const auto ordinal : query(b, map, rectangle)) {
    if (ordinal < map.fans.size()) {
      const auto &f = map.fans[ordinal];
      if (in_polygon(point, load(b, map.values, f.polygon), true))choose(f.feature);
    } else {
      const auto &s = map.strips[ordinal - map.fans.size()];
      if (in_polygon(point, strip_polygon(b, map, s), true))choose(s.feature);
    }
  }
  return feature;
}
} // namespace bgmesh
