#include "geometry_domain.hpp"
#include "geometry_protection.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
std::array<double, 6> segment_box(bgfd::Budget &budget, const std::array<double, 6> &xyz) {
  std::array<Rational, 3> a{Rational::from_double(budget, xyz[0]),
                            Rational::from_double(budget, xyz[1]),
                            Rational::from_double(budget, xyz[2])};
  std::array<Rational, 3> b{Rational::from_double(budget, xyz[3]),
                            Rational::from_double(budget, xyz[4]),
                            Rational::from_double(budget, xyz[5])};
  Rational radius(budget, 0);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto delta = b[axis].subtract(a[axis]);
    radius = radius.add(delta.sign() < 0 ? delta.negative() : delta);
  }
  radius = radius.divide(Rational(budget, 2));
  std::array<double, 6> box{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto center = a[axis].add(b[axis]).divide(Rational(budget, 2));
    box[2 * axis] = center.subtract(radius).bounds().lower;
    box[2 * axis + 1] = center.add(radius).bounds().upper;
  }
  return box;
}
uint32_t build(bgfd::Budget &budget, BoundaryProtectionData &index, Range<uint32_t> range,
               uint32_t depth) {
  budget.charge(1);
  if (index.nodes.size() >= UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "protected feature index node count exhausted");
  const auto ordinal = static_cast<uint32_t>(index.nodes.size());
  const auto first = static_cast<std::size_t>(range.first),
             end = first + static_cast<std::size_t>(range.count);
  auto box = index.segments[index.order[first]].box;
  for (std::size_t i = first + 1; i < end; ++i)
    for (std::size_t axis = 0; axis < 3; ++axis) {
      budget.charge(1);
      const auto &next = index.segments[index.order[i]].box;
      box[2 * axis] = std::min(box[2 * axis], next[2 * axis]);
      box[2 * axis + 1] = std::max(box[2 * axis + 1], next[2 * axis + 1]);
    }
  index.nodes.push_back(
      {box, 0, 0, static_cast<uint32_t>(first), static_cast<uint32_t>(range.count)});
  if (range.count <= 8)
    return ordinal;
  const auto middle = first + static_cast<std::size_t>(range.count) / 2,
             axis = static_cast<std::size_t>(depth % 3);
  std::nth_element(index.order.begin() + static_cast<std::ptrdiff_t>(first),
                   index.order.begin() + static_cast<std::ptrdiff_t>(middle),
                   index.order.begin() + static_cast<std::ptrdiff_t>(end),
                   [&](uint32_t a, uint32_t b) {
                     budget.charge(1);
                     const auto &x = index.segments[a], &y = index.segments[b];
                     if (x.box[2 * axis] != y.box[2 * axis])
                       return x.box[2 * axis] < y.box[2 * axis];
                     if (x.feature != y.feature)
                       return x.feature < y.feature;
                     return a < b;
                   });
  const auto left = build(budget, index, {first, middle - first}, depth + 1);
  const auto right = build(budget, index, {middle, end - middle}, depth + 1);
  index.nodes[ordinal].left = left;
  index.nodes[ordinal].right = right;
  index.nodes[ordinal].count = 0;
  return ordinal;
}
bool contains(const std::array<double, 6> &box, const Vec3 &point) {
  for (std::size_t axis = 0; axis < 3; ++axis)
    if (point[axis] < box[2 * axis] || point[axis] > box[2 * axis + 1])
      return false;
  return true;
}
} // namespace
BoundaryProtectionData::BoundaryProtectionData(const FaceBoundarySnapshotData &seed)
    : resource(seed.resource), geometry(seed.geometry), definition(seed.definition),
      face(seed.face), source_generation(seed.source_generation),
      face_generation(seed.face_generation), boundary_generation(seed.boundary_generation),
      segments(resource.get()), order(resource.get()), nodes(resource.get()) {}
std::shared_ptr<const BoundaryProtectionData>
prepare_boundary_protection(bgfd::Budget &budget, const FaceBoundarySnapshotData &seed) {
  const auto &source = seed.geometry.view().source.view();
  if (seed.face.value >= source.faces.size() ||
      seed.definition.value >= source.definitions.size() || seed.segments.empty())
    invalid("protected boundary index lacks a complete source face");
  auto index = std::allocate_shared<BoundaryProtectionData>(
      RunAllocator<BoundaryProtectionData>(seed.resource), seed);
  for (const auto &loop : seed.loops) {
    if (loop.segments.first > seed.segments.size() ||
        loop.segments.count > seed.segments.size() - loop.segments.first ||
        loop.segments.count == 0)
      invalid("protected boundary loop range invalid");
    for (uint64_t i = 0; i < loop.segments.count; ++i) {
      budget.charge(1);
      const auto position = loop.segments.first + i;
      const auto &segment = seed.segments[static_cast<std::size_t>(position)];
      const auto &next = seed.segments[static_cast<std::size_t>(loop.segments.first +
                                                                (i + 1) % loop.segments.count)];
      if (segment.site >= seed.sites.size() || next.site >= seed.sites.size() ||
          segment.use.value >= source.coedges.size())
        invalid("protected boundary site/use slot invalid");
      const auto &use = source.coedges[segment.use.value];
      if (!use.reconstructed) {
        const auto &map = seed.domain_map;
        if (!map)
          throw Error(ErrorCode::unsupported,
                      "authoritative protection requires its actual star/strip map");
        if (&map->geometry.view() != &seed.geometry.view() || map->definition != seed.definition ||
            map->face != seed.face ||
            map->generations != std::array<uint64_t, 3>{seed.source_generation,
                                                        seed.face_generation,
                                                        seed.boundary_generation})
          invalid("authoritative protection map belongs to foreign generation");
      }
      if (segment.constraint.kind == BoundaryOwnerKind::source_edge) {
        if (segment.constraint.owner != use.edge_id.value)
          invalid("protected source edge key differs from original use");
      } else if (segment.constraint.kind != BoundaryOwnerKind::face_cut)
        invalid("unknown protected source feature kind");
      else if (segment.constraint.owner != source.faces[seed.face.value].id.value)
        invalid("protected face cut belongs to another face");
      const auto &a = seed.sites[segment.site].node.xyz, &b = seed.sites[next.site].node.xyz;
      const std::array<double, 6> xyz{a[0], a[1], a[2], b[0], b[1], b[2]};
      for (const auto coordinate : xyz)
        if (!std::isfinite(coordinate))
          invalid("nonfinite protected endpoint");
      if (index->segments.size() >= UINT32_MAX / 2)
        throw Error(ErrorCode::resource_exhausted, "protected boundary segment count exhausted");
      index->order.push_back(static_cast<uint32_t>(index->segments.size()));
      index->segments.push_back({segment.constraint, xyz, segment_box(budget, xyz)});
    }
  }
  if (index->segments.size() != seed.segments.size())
    invalid("protected source loop inventory differs from segments");
  static_cast<void>(build(budget, *index, {0, index->order.size()}, 0));
  return index;
}
std::optional<BoundaryKey>
encroached_boundary(bgfd::Budget &budget, const BoundaryProtectionData &index, const Vec3 &point) {
  for (const auto value : point)
    if (!std::isfinite(value))
      invalid("nonfinite protected candidate");
  if (index.nodes.empty())
    invalid("missing protected source feature index");
  const std::array<Rational, 3> p{Rational::from_double(budget, point[0]),
                                  Rational::from_double(budget, point[1]),
                                  Rational::from_double(budget, point[2])};
  std::pmr::vector<uint32_t> stack(&budget);
  stack.push_back(0);
  std::optional<BoundaryKey> selected;
  while (!stack.empty()) {
    const auto ordinal = stack.back();
    stack.pop_back();
    budget.charge(1);
    const auto &node = index.nodes[ordinal];
    if (!contains(node.box, point))
      continue;
    if (node.count == 0) {
      stack.push_back(node.left);
      stack.push_back(node.right);
      continue;
    }
    for (uint32_t i = 0; i < node.count; ++i) {
      const auto &segment = index.segments[index.order[node.first + i]];
      if (!contains(segment.box, point))
        continue;
      Rational product(budget, 0);
      for (std::size_t axis = 0; axis < 3; ++axis)
        product = product.add(p[axis]
                                  .subtract(Rational::from_double(budget, segment.endpoints[axis]))
                                  .multiply(p[axis].subtract(
                                      Rational::from_double(budget, segment.endpoints[axis + 3]))));
      if (product.sign() < 0 && (!selected || segment.feature < *selected))
        selected.emplace(segment.feature);
    }
  }
  return selected;
}
} // namespace bgmesh
