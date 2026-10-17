#include "adaptive_region_cdt.hpp"
#include "domain_index.hpp"
#include <algorithm>
#include <limits>
#include <numeric>
#include <optional>
namespace bgmesh::adaptive {
namespace {
constexpr uint32_t missing = UINT32_MAX;
void check(bool condition, const char *message) {
  if (!condition)
    throw bgfd::Failure(BGFD_INVALID_DOMAIN, message);
}
struct HalfEdge final {
  uint32_t a, b, triangle;
};
std::array<uint32_t, 2> edge_key(const HalfEdge &edge) {
  return {std::min(edge.a, edge.b), std::max(edge.a, edge.b)};
}
bgfd_integer append_integer(const bgfd::Integer &value, std::pmr::vector<uint32_t> &limbs,
                            bgfd::Budget &budget) {
  const auto words = value.magnitude();
  if (words.size() > UINT32_MAX)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "region limb count");
  bgfd_integer descriptor{limbs.size(), static_cast<uint32_t>(words.size()), value.sign()};
  budget.charge(words.size());
  limbs.insert(limbs.end(), words.begin(), words.end());
  return descriptor;
}
} // namespace
RegionCDT construct_region_cdt(bgfd::Budget &budget,
                               std::span<const std::array<Rational, 2>> points,
                               std::span<const RegionCycleView> cycles,
                               const std::array<Rational, 4> &factor, uint64_t seed,
                               bool filter_nonmaterial_anchors) {
  check(points.size() >= 3 && points.size() < missing && !cycles.empty(),
        "empty region sites/cycles");
  for (const auto &point : points)
    for (const auto &q : point)
      check(&q.budget() == &budget, "region site owner");
  for (const auto &entry : factor)
    check(&entry.budget() == &budget, "region metric owner");
  check(factor[0].multiply(factor[3]).subtract(factor[1].multiply(factor[2])).sign() > 0,
        "region metric determinant must be positive");
  RegionCDT result(&budget);
  uint64_t positive = 0, negative = 0;
  {
    std::pmr::vector<uint32_t> active(&budget), slots(&budget);
    std::pmr::vector<bgfd_loop> loops(&budget);
    {
      // The index borrows these admission tables only within this phase.
      std::pmr::vector<uint8_t> boundary(points.size(), uint8_t{0}, &budget);
      for (const auto &cycle : cycles) {
        check(cycle.sites.size() >= 3, "region cycle has fewer than three sites");
        Rational area(budget);
        const auto first = slots.size();
        for (size_t i = 0; i < cycle.sites.size(); ++i) {
          budget.charge(1);
          const auto a = cycle.sites[i], b = cycle.sites[(i + 1) % cycle.sites.size()];
          check(a < points.size() && b < points.size() && boundary[a] == 0,
                "repeated/outside boundary site");
          boundary[a] = 1;
          slots.push_back(a);
          for (const auto &q : points[a])
            check(&q.budget() == &budget, "region site owner");
          area = area.add(
              points[a][0].multiply(points[b][1]).subtract(points[a][1].multiply(points[b][0])));
        }
        check(area.sign() != 0, "region cycle has zero area");
        if (area.sign() > 0)
          ++positive;
        else
          ++negative;
        loops.push_back({first, cycle.sites.size()});
      }
      check(positive != 0, "region has no positive cycle");
      std::pmr::vector<DomainPointRef> point_refs(&budget);
      for (const auto &point : points)
        point_refs.push_back({&point[0], &point[1]});
      std::pmr::vector<DomainSegment> segments(&budget);
      for (uint32_t l = 0; l < cycles.size(); ++l) {
        const auto &cycle = cycles[l];
        for (uint32_t j = 0; j < cycle.sites.size(); ++j) {
          const auto a = cycle.sites[j], b = cycle.sites[(j + 1) % cycle.sites.size()];
          segments.push_back(
              {SiteSlot{a}, SiteSlot{b}, {l, j}, domain_segment_box(point_refs[a], point_refs[b])});
        }
      }
      DomainIndexStats index_stats;
      DomainSegmentIndex index(budget, point_refs, index_stats, segments);
      check(!index.crossing(), "region cycles cross, overlap or touch");
      for (size_t l = 0; l < loops.size(); ++l) {
        const auto &loop = loops[l];
        auto own = std::span<const DomainSegment>(segments).subspan(loop.offset, loop.count);
        Rational area(budget);
        for (const auto &edge : own)
          area = area.add(points[edge.a.value][0]
                              .multiply(points[edge.b.value][1])
                              .subtract(points[edge.a.value][1].multiply(points[edge.b.value][0])));
        check(index.winding_except(own.front().a, own) == (area.sign() > 0 ? 0 : 1),
              "region cycle placement/orientation differs");
      }
      std::pmr::vector<uint32_t> compact(points.size(), missing, &budget);
      for (uint32_t i = 0; i < points.size(); ++i) {
        const bool inside = boundary[i] || (!index.boundary_contact(point_refs[i]) &&
                                            index.winding(point_refs[i]) == 1);
        if (!inside) {
          check(filter_nonmaterial_anchors, "region anchor is not strictly inside material");
          continue;
        }
        compact[i] = static_cast<uint32_t>(active.size());
        active.push_back(i);
      }
      std::pmr::vector<uint32_t> unique_order(&budget);
      unique_order = active;
      std::sort(unique_order.begin(), unique_order.end(), [&](uint32_t a, uint32_t b) {
        budget.charge(1);
        auto x = points[a][0].compare(points[b][0]);
        return x != 0 ? x < 0 : points[a][1].compare(points[b][1]) < 0;
      });
      for (size_t j = 1; j < unique_order.size(); ++j) {
        const auto &a = points[unique_order[j - 1]], &b = points[unique_order[j]];
        check(a[0].compare(b[0]) != 0 || a[1].compare(b[1]) != 0, "duplicate region chart sites");
      }

      for (auto &slot : slots)
        slot = compact[slot];
    }
    // Predicates owns decoded coordinates/lifts; no encoder buffers are
    // borrowed.
    std::optional<bgcdt::Predicates> predicates;
    {
      std::pmr::vector<std::array<Rational, 2>> transformed(&budget);
      transformed.reserve(points.size());
      const std::array<uint32_t, 1> one_word{1};
      const auto one = bgfd::Integer::from_words(budget, 1, one_word);
      auto denominator = one;
      bool dyadic = true;
      uint64_t power = 0;
      for (uint32_t index_value : active) {
        const auto &point = points[index_value];
        for (const auto &q : point)
          check(&q.budget() == &budget, "region site owner");
        const auto x = point[0].subtract(points[0][0]), y = point[1].subtract(points[0][1]);
        transformed.push_back({factor[0].multiply(x).add(factor[1].multiply(y)),
                               factor[2].multiply(x).add(factor[3].multiply(y))});
        for (const auto &q : transformed.back()) {
          budget.charge(1);
          const auto exponent = q.denominator().bit_length() - 1;
          if (q.denominator().compare(one.shift_left(exponent)) != 0)
            dyadic = false;
          power = std::max(power, exponent);
        }
      }
      if (dyadic)
        denominator = one.shift_left(power);
      else
        for (const auto &point : transformed)
          for (const auto &q : point) {
            const auto gcd = bgfd::Integer::gcd_nonnegative(denominator, q.denominator());
            denominator = denominator.divide_exact(gcd).multiply(q.denominator());
          }
      result.denominator_bits = denominator.bit_length();
      std::pmr::vector<uint32_t> limbs(&budget);
      limbs.push_back(1);
      std::pmr::vector<bgfd_node> nodes(&budget);
      nodes.reserve(active.size());
      for (uint32_t i = 0; i < active.size(); ++i) {
        const auto x = transformed[i][0].numerator().multiply(
            denominator.divide_exact(transformed[i][0].denominator()));
        const auto y = transformed[i][1].numerator().multiply(
            denominator.divide_exact(transformed[i][1].denominator()));
        nodes.push_back({i, append_integer(x, limbs, budget), append_integer(y, limbs, budget)});
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
      predicates.emplace(budget, input);
    }
    result.triangles = bgcdt::solve_directed_regions(
        *predicates, budget, slots, loops, {slots.size(), positive, negative}, seed, result.stats);
    predicates.reset();
    for (auto &triangle : result.triangles)
      for (auto &slot : triangle)
        slot = active[slot];
  }
  std::pmr::vector<HalfEdge> edges(&budget);
  for (uint32_t i = 0; i < result.triangles.size(); ++i)
    for (uint32_t j = 0; j < 3; ++j) {
      const auto &triangle = result.triangles[i];
      edges.push_back({triangle[j], triangle[(j + 1) % 3], i});
    }
  std::sort(edges.begin(), edges.end(), [&](const auto &a, const auto &b) {
    budget.charge(1);
    const auto ka = edge_key(a), kb = edge_key(b);
    return ka != kb ? ka < kb : a.triangle < b.triangle;
  });
  std::pmr::vector<std::array<uint32_t, 3>> adjacent(
      result.triangles.size(), std::array<uint32_t, 3>{missing, missing, missing}, &budget);
  std::pmr::vector<uint32_t> degree(result.triangles.size(), 0, &budget);
  for (size_t i = 0; i < edges.size();) {
    auto end = i + 1;
    while (end < edges.size() && edge_key(edges[end]) == edge_key(edges[i]))
      ++end;
    check(end - i <= 2, "region CDT edge has too many incidents");
    if (end - i == 2) {
      const auto &a = edges[i], &b = edges[i + 1];
      check(a.a == b.b && a.b == b.a, "region CDT interior edge orientation differs");
      adjacent[a.triangle][degree[a.triangle]++] = b.triangle;
      adjacent[b.triangle][degree[b.triangle]++] = a.triangle;
    }
    i = end;
  }
  result.components.assign(result.triangles.size(), missing);
  std::pmr::vector<uint32_t> queue(&budget);
  for (uint32_t start = 0; start < result.triangles.size(); ++start) {
    if (result.components[start] != missing)
      continue;
    const auto component = result.component_count++;
    queue.clear();
    queue.push_back(start);
    result.components[start] = component;
    for (size_t at = 0; at < queue.size(); ++at) {
      budget.charge(1);
      const auto triangle = queue[at];
      for (uint32_t j = 0; j < degree[triangle]; ++j) {
        const auto other = adjacent[triangle][j];
        if (result.components[other] != missing)
          continue;
        result.components[other] = component;
        queue.push_back(other);
      }
    }
  }
  check(result.component_count == positive, "region positive cycle/component inventory differs");
  std::pmr::vector<std::array<uint32_t, 2>> minimum(
      result.component_count, std::array<uint32_t, 2>{missing, missing}, &budget);
  for (const auto &cycle : cycles) {
    uint32_t owner = missing;
    for (size_t j = 0; j < cycle.sites.size(); ++j) {
      const auto a = cycle.sites[j], b = cycle.sites[(j + 1) % cycle.sites.size()];
      const std::array<uint32_t, 2> key{std::min(a, b), std::max(a, b)};
      auto hit = std::lower_bound(edges.begin(), edges.end(), key,
                                  [&](const auto &edge, const auto &needle) {
                                    budget.charge(1);
                                    return edge_key(edge) < needle;
                                  });
      check(hit != edges.end() && edge_key(*hit) == key && hit->a == a && hit->b == b,
            "region loop has no retained left incidence");
      const auto component = result.components[hit->triangle];
      check(owner == missing || owner == component, "region loop straddles components");
      owner = component;
      minimum[component] = std::min(minimum[component], std::array{a, b});
    }
    result.loop_components.push_back(owner);
  }
  std::pmr::vector<uint32_t> order(result.component_count, &budget),
      remap(result.component_count, &budget);
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
    budget.charge(1);
    return minimum[a] < minimum[b];
  });
  for (uint32_t i = 0; i < order.size(); ++i)
    remap[order[i]] = i;
  for (auto &component : result.components)
    component = remap[component];
  for (auto &component : result.loop_components)
    component = remap[component];
  return result;
}
} // namespace bgmesh::adaptive
