#include "../native/mesher/topology.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
template <class T> T required(std::optional<T> value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
  return *value;
}
using bgmesh::SiteSlot;
using bgmesh::TriangleHandle;

void check_links(const bgmesh::MutableFaceTopology &mesh) {
  for (const auto handle : mesh.triangles()) {
    const auto triangle = mesh.triangle(handle);
    for (uint32_t side = 0; side < 3; ++side) {
      const auto edge = mesh.edge(triangle.edges[side]);
      require(edge.incident_count >= 1 && edge.incident_count <= 2, "edge incidence");
      unsigned hits = 0;
      for (uint32_t i = 0; i < edge.incident_count; ++i)
        hits += edge.incidents[i] == handle ? 1u : 0u;
      require(hits == 1, "triangle absent from its edge");
      if (const auto neighbor = triangle.neighbors[side]) {
        const auto other = mesh.triangle(*neighbor);
        unsigned reciprocal = 0;
        for (uint32_t j = 0; j < 3; ++j)
          if (other.edges[j] == triangle.edges[side] && other.neighbors[j] == handle)
            ++reciprocal;
        require(reciprocal == 1, "nonreciprocal neighbor");
      } else {
        require(edge.incident_count == 1, "lost internal neighbor");
      }
    }
  }
}

void midpoint_and_patch() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget);
  const auto a = mesh.append_site({1, 0});
  const auto b = mesh.append_site({std::nextafter(1.0, 2.0), 0});
  const auto c = mesh.append_site({1, 1});
  const auto d = mesh.append_site({std::nextafter(1.0, 2.0), -1});
  const auto left = mesh.add_initial_triangle({a, b, c});
  const auto right = mesh.add_initial_triangle({b, a, d});
  mesh.seal_initial();
  check_links(mesh);
  const auto shared = required(mesh.find_edge(a, b), "initial diagonal missing");
  const auto p = mesh.bisect_internal_edge(shared);
  require(mesh.compare_axis(a, p, bgmesh::CoordinateAxis::x) < 0 &&
              mesh.compare_axis(p, b, bgmesh::CoordinateAxis::x) < 0,
          "midpoint rounded to an endpoint");
  require(!mesh.is_live(left) && !mesh.is_live(right), "stale parent handle still live");
  require(mesh.triangles().size() == 4, "both incidents were not split");
  require(!mesh.find_edge(a, b), "old unsplit edge remains");
  require(mesh.find_edge(a, p) && mesh.find_edge(p, b), "split edge chain missing");
  check_links(mesh);
}

void terminal_tie() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget);
  const auto a = mesh.append_site({0, 0});
  const auto d = mesh.append_site({4, -3});
  const auto b = mesh.append_site({5, 0});
  const auto c = mesh.append_site({2, 1});
  const auto target = mesh.add_initial_triangle({a, b, c});
  (void)mesh.add_initial_triangle({b, a, d});
  mesh.seal_initial();
  // AB=AD=5 in the neighbor. Its ordinary endpoint tie chooses AD, but the
  // incoming AB must immediately terminate the LEPP pair.
  const auto result = mesh.refine_longest(target);
  require(result.kind == bgmesh::RefineKind::target_bisected, "incoming longest tie walked on");
  require(result.splits == 1 && mesh.triangles().size() == 4, "tie did not split one pair");
  require(!mesh.find_edge(a, b), "wrong tied edge split");
  check_links(mesh);
}

void anisotropic_choice() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget, {1, -1, 0.125, 0.125});
  const auto a = mesh.append_site({0, 0});
  const auto b = mesh.append_site({1, -1});
  const auto c = mesh.append_site({4.5, 3.5});
  const auto d = mesh.append_site({-3.5, -4.5});
  const auto target = mesh.add_initial_triangle({a, b, c});
  (void)mesh.add_initial_triangle({b, a, d});
  mesh.seal_initial();
  // In UV the ruling edges are much longer. Under the fixed rotated metric,
  // AB has length²4 and the others length²2, so AB is the terminal pair.
  const auto result = mesh.refine_longest(target);
  require(result.kind == bgmesh::RefineKind::target_bisected && result.splits == 1,
          "LEPP used UV or an evolving metric");
  require(!mesh.find_edge(a, b), "anisotropic longest edge was retained");
  check_links(mesh);
}

void sticky_lineage() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget);
  const auto a = mesh.append_site({0, 0});
  const auto b = mesh.append_site({2, 0});
  const auto c = mesh.append_site({0, 2});
  const auto target = mesh.add_initial_triangle({a, b, c});
  mesh.seal_initial();
  const auto inserted = mesh.insert_heuristic(target, {0.5, 0.5});
  require(inserted.has_value(), "valid heuristic rejected");
  const auto p = *inserted;
  require(mesh.triangles().size() == 3, "interior insertion patch");
  for (const auto child : mesh.triangles()) {
    require(mesh.triangle(child).heuristic_spent, "child lost spent ancestry");
    mesh.mark_accepted(child);
    require(mesh.triangle(child).heuristic_spent, "acceptance reset spent ancestry");
  }
  const auto shared = mesh.find_edge(a, p);
  require(shared.has_value(), "new spoke missing");
  (void)mesh.bisect_internal_edge(*shared);
  for (const auto child : mesh.triangles())
    require(mesh.triangle(child).heuristic_spent, "conformity split reset ancestry");
  const auto live = mesh.triangles().front();
  const bool rejected = !mesh.insert_heuristic(live, {0.5, 0.5});
  require(rejected && mesh.is_usable(), "spent attempt did not reject before mutation");
  check_links(mesh);
  bgmesh::MutableFaceTopology overflow(budget);
  const auto oa=overflow.append_site({0,0}),ob=overflow.append_site({2,0}),oc=overflow.append_site({0,2});
  const auto failed=overflow.add_initial_triangle({oa,ob,oc});overflow.seal_initial();
  const auto no_site=overflow.insert_heuristic(failed,{std::numeric_limits<double>::infinity(),0});
  require(!no_site && overflow.triangle(failed).heuristic_spent && overflow.stats().sites==3 &&
              !overflow.insert_heuristic(failed,{.5,.5}),
          "nonfinite rounded circumcenter failed to spend/reject without publishing");
}

void boundary_walk() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget);
  const auto a = mesh.append_site({0, 0});
  const auto b = mesh.append_site({1, 0});
  const auto c = mesh.append_site({0.5, 0.25});
  const auto d = mesh.append_site({0, -2});
  const auto target = mesh.add_initial_triangle({a, b, c});
  (void)mesh.add_initial_triangle({b, a, d});
  mesh.seal_initial();
  const auto result = mesh.refine_longest(target);
  require(result.kind == bgmesh::RefineKind::boundary_request && result.boundary.has_value(),
          "longer neighbor boundary not requested");
  const auto edge = mesh.edge(required(result.boundary, "missing boundary handle"));
  require((edge.sites[0] == b && edge.sites[1] == d) || (edge.sites[0] == d && edge.sites[1] == b),
          "LEPP forced the neighbor's nonlongest incoming edge");
  require(result.splits == 0 && mesh.is_live(target), "boundary was privately bisected");
  check_links(mesh);
}

bgmesh::Rational diameter_squared(const bgmesh::MutableFaceTopology &mesh,
                                  std::array<SiteSlot, 3> sites) {
  auto result = mesh.coordinate(sites[0], bgmesh::CoordinateAxis::x)
                    .subtract(mesh.coordinate(sites[0], bgmesh::CoordinateAxis::x));
  for (unsigned side = 0; side < 3; ++side) {
    const auto a = sites[side], b = sites[(side + 1) % 3];
    const auto x = mesh.coordinate(a, bgmesh::CoordinateAxis::x)
                       .subtract(mesh.coordinate(b, bgmesh::CoordinateAxis::x));
    const auto y = mesh.coordinate(a, bgmesh::CoordinateAxis::y)
                       .subtract(mesh.coordinate(b, bgmesh::CoordinateAxis::y));
    const auto squared = x.multiply(x).add(y.multiply(y));
    if (squared.compare(result) > 0)
      result = squared;
  }
  return result;
}

void nested_shrink() {
  bgfd::Budget budget({1u << 28, uint64_t{1} << 33});
  bgmesh::MutableFaceTopology mesh(budget);
  std::array<SiteSlot, 81> sites{};
  for (unsigned y = 0; y <= 8; ++y)
    for (unsigned x = 0; x <= 8; ++x)
      sites[y * 9 + x] = mesh.append_site({static_cast<double>(x), static_cast<double>(y)});
  uint64_t root = UINT64_MAX;
  for (unsigned y = 0; y < 8; ++y)
    for (unsigned x = 0; x < 8; ++x) {
      const auto a = sites[y * 9 + x], b = sites[y * 9 + x + 1];
      const auto c = sites[(y + 1) * 9 + x], d = sites[(y + 1) * 9 + x + 1];
      const auto target = mesh.add_initial_triangle({a, b, d});
      (void)mesh.add_initial_triangle({a, d, c});
      if (x == 3 && y == 3)
        root = mesh.triangle(target).lineage;
    }
  mesh.seal_initial();
  const bgmesh::Rational three(budget, 3), four(budget, 4);
  unsigned checked = 0;
  for (unsigned pass = 0; pass < 16; ++pass) {
    std::optional<TriangleHandle> target;
    bgmesh::Rational largest(budget);
    for (const auto live : mesh.triangles()) {
      const auto row = mesh.triangle(live);
      auto ancestor = row.lineage;
      while (mesh.lineage(ancestor).parent != UINT64_MAX)
        ancestor = mesh.lineage(ancestor).parent;
      if (ancestor != root)
        continue;
      const auto size = diameter_squared(mesh, row.sites);
      if (!target || size.compare(largest) > 0) {
        target = live;
        largest = size;
      }
    }
    require(target.has_value(), "lost marked lineage");
    require(mesh.refine_longest(*target).kind == bgmesh::RefineKind::target_bisected,
            "interior grid closure escaped to boundary");
    for (const auto live : mesh.triangles()) {
      const auto &child = mesh.lineage(mesh.triangle(live).lineage);
      if (child.operation != bgmesh::LineageOperation::bisection || child.parent == UINT64_MAX)
        continue;
      const auto &parent = mesh.lineage(child.parent);
      if (parent.operation != bgmesh::LineageOperation::bisection || parent.parent == UINT64_MAX)
        continue;
      const auto &grandparent = mesh.lineage(parent.parent);
      const auto actual = diameter_squared(mesh, child.sites).multiply(four);
      const auto limit = diameter_squared(mesh, grandparent.sites).multiply(three);
      require(actual.compare(limit) <= 0, "two-level longest-edge contraction violated");
      ++checked;
    }
  }
  require(checked > 0 && mesh.stats().edge_splits > 16, "closure/descendant paths unexercised");
  check_links(mesh);
}

void spent_neighbor() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget);
  const auto a = mesh.append_site({0, 0});
  const auto b = mesh.append_site({2, 0});
  const auto c = mesh.append_site({0, 2});
  const auto d = mesh.append_site({2, -2});
  const auto target = mesh.add_initial_triangle({a, b, c});
  const auto neighbor = mesh.add_initial_triangle({b, a, d});
  mesh.seal_initial();
  require(mesh.insert_heuristic(neighbor, {1, -0.5}).has_value(), "neighbor insertion rejected");
  const auto before = mesh.triangles().size();
  const auto sites_before = mesh.stats().sites;
  const bool rejected = !mesh.insert_heuristic(target, {1, 0});
  require(rejected && mesh.is_live(target) && mesh.triangles().size() == before,
          "heuristic split an already-spent neighbor ancestry");
  require(mesh.triangle(target).heuristic_spent, "rejected attempt was not spent");
  require(mesh.stats().sites == sites_before, "rejected candidate published an orphan");
  const auto edge = required(mesh.find_edge(a, b), "rejection lost needed midpoint edge");
  const auto midpoint = mesh.bisect_internal_edge(edge);
  require(mesh.stats().sites == sites_before + 1 &&
              mesh.coordinate(midpoint, bgmesh::CoordinateAxis::x).to_double() == 1 &&
              mesh.coordinate(midpoint, bgmesh::CoordinateAxis::y).sign() == 0,
          "rejected proposal obstructed the later exact midpoint");
  check_links(mesh);
}

void constraints_and_rejection() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget);
  const auto a = mesh.append_site({0, 0});
  const auto b = mesh.append_site({2, 0});
  const auto c = mesh.append_site({0, 2});
  const auto target = mesh.add_initial_triangle({a, b, c});
  mesh.seal_initial();
  const auto old_edge = required(mesh.find_edge(a, b), "missing source constraint edge");
  const bgmesh::BoundaryKey key{7, 11};
  mesh.protect_edge(old_edge, key);
  require(mesh.insert_heuristic(target, {0.5, 0.5}).has_value(),
          "constraint fixture insertion rejected");
  const auto new_edge = mesh.find_edge(a, b);
  require(new_edge && mesh.edge(*new_edge).constraint == key,
          "local replacement lost source constraint metadata");
  bool stale_rejected = false;
  try {
    (void)mesh.edge(old_edge);
  } catch (const bgfd::Failure &) {
    stale_rejected = true;
  }
  require(stale_rejected, "recycled edge accepted stale generation");
  bool duplicate_rejected = false;
  try {
    (void)mesh.append_site({0, 0});
  } catch (const bgfd::Failure &) {
    duplicate_rejected = true;
  }
  require(duplicate_rejected && mesh.is_usable(), "duplicate site poisoned valid topology");
  check_links(mesh);
}

void resource_failure() {
  bool poisoned_observed = false, precommit_observed = false;
  constexpr uint64_t work_limit = uint64_t{1} << 28;
  for (uint64_t allowance = 1; allowance <= 2048; allowance += 47) {
    bgfd::Budget budget({1u << 24, work_limit});
    bgmesh::MutableFaceTopology mesh(budget);
    const auto a = mesh.append_site({0, 0});
    const auto b = mesh.append_site({1, 1});
    const auto c = mesh.append_site({0, 1});
    const auto d = mesh.append_site({1, 0});
    const auto left = mesh.add_initial_triangle({a, b, c});
    const auto right = mesh.add_initial_triangle({b, a, d});
    mesh.seal_initial();
    const auto shared = required(mesh.find_edge(a, b), "resource fixture edge missing");
    budget.charge(work_limit - budget.work() - allowance);
    try {
      (void)mesh.bisect_internal_edge(shared);
    } catch (const bgfd::Failure &failure) {
      require(failure.status == BGFD_RESOURCE_EXHAUSTED, "wrong resource error");
      if (mesh.is_usable()) {
        require(mesh.is_live(left) && mesh.is_live(right), "precommit failure changed parent rows");
        precommit_observed = true;
      } else {
        bool inaccessible = false;
        try {
          (void)mesh.triangle(left);
        } catch (const bgfd::Failure &unusable) {
          inaccessible = unusable.status == BGFD_INTERNAL_ERROR;
        }
        require(inaccessible, "partial failed generation remained readable");
        poisoned_observed = true;
      }
    }
  }
  require(precommit_observed && poisoned_observed, "resource fixture missed transaction phases");
}

void local_edit_log() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 30});
  bgmesh::MutableFaceTopology mesh(budget);
  const auto a = mesh.append_site({0, 0});
  const auto b = mesh.append_site({1, 0});
  const auto c = mesh.append_site({0.5, 0.25});
  const auto d = mesh.append_site({0, -2});
  const auto e = mesh.append_site({2, -2});
  const auto target = mesh.add_initial_triangle({a, b, c});
  const auto second = mesh.add_initial_triangle({b, a, d});
  const auto third = mesh.add_initial_triangle({b, d, e});
  mesh.seal_initial();
  require(mesh.take_changes().empty(), "initial enumeration emitted edit deltas");
  const auto result = mesh.refine_longest(target);
  require(result.kind == bgmesh::RefineKind::boundary_request && result.splits == 1,
          "partial closure boundary case unexercised");
  auto expected = std::vector<TriangleHandle>{target, second, third};
  const auto changes = mesh.take_changes();
  require(changes.size() == result.splits, "terminal split log incomplete");
  for (const auto &change : changes) {
    require(change.new_site.has_value(), "split did not report appended site");
    for (uint32_t i = 0; i < change.removed_count; ++i) {
      const auto at = std::find(expected.begin(), expected.end(), change.removed[i]);
      require(at != expected.end(), "removed handle/generation absent from replay");
      expected.erase(at);
    }
    for (uint32_t i = 0; i < change.inserted_count; ++i)
      expected.push_back(change.inserted[i]);
  }
  std::sort(expected.begin(), expected.end());
  auto actual = mesh.triangles();
  std::sort(actual.begin(), actual.end());
  require(expected.size() == actual.size() && std::equal(expected.begin(), expected.end(), actual.begin()),
          "local edit replay disagrees with final active mesh");
  require(mesh.take_changes().empty(), "edit log was not consumed");
  check_links(mesh);

  bgmesh::MutableFaceTopology duplicate(budget);
  const auto x = duplicate.append_site({0, 0});
  const auto y = duplicate.append_site({1, 0});
  const auto z = duplicate.append_site({0, 1});
  const auto tri = duplicate.add_initial_triangle({x, y, z});
  duplicate.seal_initial();
  require(!duplicate.insert_heuristic(tri, {0, 0}) && duplicate.stats().sites == 3 &&
              duplicate.is_live(tri) && duplicate.take_changes().empty(),
          "connected coordinate proposal was reused as a new site");
}
} // namespace

int main() {
  try {
    midpoint_and_patch();
    terminal_tie();
    anisotropic_choice();
    sticky_lineage();
    boundary_walk();
    nested_shrink();
    spent_neighbor();
    constraints_and_rejection();
    resource_failure();
    local_edit_log();
    std::cout << "native mutable topology: 10 semantic cases PASS\n";
  } catch (const bgfd::Failure &failure) {
    std::cerr << "native failure: " << failure.what() << '\n';
    return 1;
  } catch (const std::exception &failure) {
    std::cerr << failure.what() << '\n';
    return 1;
  }
}
