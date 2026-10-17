#include "../native/mesher/contact.hpp"
#include <array>
#include <cfenv>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
bgmesh::ContactFacet facet(std::array<uint32_t, 3> ids, std::array<bgmesh::Vec3, 3> xyz) {
  return {0, {bgmesh::NodeSlot{ids[0]}, bgmesh::NodeSlot{ids[1]}, bgmesh::NodeSlot{ids[2]}}, xyz};
}
void exact_contacts() {
  bgfd::Budget budget({1u << 27, uint64_t{1} << 32});
  using D = bgmesh::ContactDimension;
  using S = bgmesh::SimplexDimension;
  const auto a = facet({0, 1, 2}, {{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}}});
  const bgmesh::AllowedSimplex none{S::none, {}};
  const auto separate = facet({3, 4, 5}, {{{0, 0, 1}, {2, 0, 1}, {0, 2, 1}}});
  const auto disjoint = bgmesh::classify_contact(budget, a, separate, none);
  require(disjoint.dimension == D::empty && disjoint.permitted, "separate parallel facets");
  const auto crossing = facet({3, 4, 5}, {{{0.5, 0.5, -1}, {0.5, 0.5, 1}, {1.5, 0.5, 0}}});
  const auto cut = bgmesh::classify_contact(budget, a, crossing, none);
  require(cut.dimension == D::segment && !cut.permitted, "proper crossing hidden");
  const auto vertex = facet({0, 4, 5}, {{{0, 0, 0}, {-1, 0, 1}, {0, -1, 1}}});
  const bgmesh::AllowedSimplex point{S::vertex, {bgmesh::NodeSlot{0}, bgmesh::NodeSlot{0}}};
  const auto touch = bgmesh::classify_contact(budget, a, vertex, point);
  require(touch.dimension == D::point && touch.permitted, "declared vertex contact");
  const auto foreign = facet({3, 4, 5}, vertex.xyz);
  require(!bgmesh::classify_contact(budget, a, foreign, none).permitted,
          "equal coordinates implicitly welded foreign nodes");
  const auto edge = facet({1, 0, 5}, {{{2, 0, 0}, {0, 0, 0}, {0, -2, 0}}});
  const bgmesh::AllowedSimplex common_edge{S::edge, {bgmesh::NodeSlot{0}, bgmesh::NodeSlot{1}}};
  const auto joined = bgmesh::classify_contact(budget, a, edge, common_edge);
  require(joined.dimension == D::segment && joined.permitted, "declared coplanar edge");
  const auto overlap = facet({1, 0, 5}, {{{2, 0, 0}, {0, 0, 0}, {0.5, 0.5, 0}}});
  const auto beyond = bgmesh::classify_contact(budget, a, overlap, common_edge);
  require(beyond.dimension == D::area && !beyond.permitted, "shared edge hid area overlap");
  const auto shared_cross = facet({0, 4, 5}, {{{0, 0, 0}, {1, 1, -1}, {1, 1, 1}}});
  const auto extra = bgmesh::classify_contact(budget, a, shared_cross, point);
  require(extra.dimension == D::segment && !extra.permitted, "shared vertex hid crossing");
  auto inconsistent = vertex;
  inconsistent.xyz[0][0] = 0.25;
  bool rejected = false;
  try {
    (void)bgmesh::classify_contact(budget, a, inconsistent, point);
  } catch (const bgfd::Failure &) {
    rejected = true;
  }
  require(rejected, "one node identity had inconsistent XYZ");

  const std::array<std::array<unsigned, 3>, 6> permutations{
      {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}}};
  const auto transformed = [](const bgmesh::ContactFacet &input, std::array<unsigned, 3> order,
                              unsigned axis) {
    auto output = input;
    const bgmesh::Vec3 translation{1000000, -2000000, 3000000};
    for (unsigned i = 0; i < 3; ++i) {
      output.nodes[i] = input.nodes[order[i]];
      for (unsigned j = 0; j < 3; ++j)
        output.xyz[i][j] = input.xyz[order[i]][(j + axis) % 3] + translation[j];
    }
    return output;
  };
  for (const auto &order : permutations)
    for (unsigned axis = 0; axis < 3; ++axis) {
      const auto first = transformed(a, order, axis);
      const auto second = transformed(shared_cross, order, axis);
      const auto answer = bgmesh::classify_contact(budget, first, second, point);
      const auto reversed = bgmesh::classify_contact(budget, second, first, point);
      require(answer.dimension == D::segment && !answer.permitted &&
                  reversed.dimension == answer.dimension && reversed.permitted == answer.permitted,
              "contact depends on corner order, plane axis, translation or pair order");
      const auto adjacent =
          bgmesh::classify_contact(budget, first, transformed(edge, order, axis), common_edge);
      require(adjacent.dimension == D::segment && adjacent.permitted,
              "coplanar shared-edge answer depends on orientation/axis");
    }
  const auto contained = facet({3, 4, 5}, {{{0.25, 0.25, 0}, {0.5, 0.25, 0}, {0.25, 0.5, 0}}});
  require(bgmesh::classify_contact(budget, a, contained, none).dimension == D::area,
          "coplanar containment omitted");
  const auto junction = facet({3, 4, 5}, {{{1, 0, 0}, {1, -1, 1}, {1, -1, 2}}});
  const auto tj = bgmesh::classify_contact(budget, a, junction, none);
  require(tj.dimension == D::point && !tj.permitted, "foreign-node T junction allowed");
}
void filtered_contacts() {
  bgfd::Budget budget({1u << 27, uint64_t{1} << 32});
  using S = bgmesh::SimplexDimension;
  const auto a = facet({0, 1, 2}, {{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}}});
  const auto prepared = bgmesh::prepare_contact_facet(budget, a);
  bgmesh::ContactPredicateStats stats;
  const auto check = [&](const bgmesh::ContactFacet &b, bgmesh::AllowedSimplex allowed) {
    return bgmesh::classify_contact(budget, prepared, bgmesh::prepare_contact_facet(budget, b),
                                    allowed, stats);
  };
  const bgmesh::AllowedSimplex none{S::none, {}};
  require(check(facet({3, 4, 5}, {{{0, 0, 1}, {2, 0, 1}, {0, 2, 1}}}), none).permitted &&
              stats.filtered_separation == 1,
          "strict plane separation did not take certified filter");
  const bgmesh::AllowedSimplex edge{S::edge, {bgmesh::NodeSlot{0}, bgmesh::NodeSlot{1}}};
  require(check(facet({1, 0, 5}, {{{2, 0, 0}, {0, 0, 0}, {0, 1, 1}}}), edge).permitted &&
              stats.filtered_shared_edge == 1,
          "noncoplanar declared edge missed certified fast case");
  require(check(facet({1, 0, 5}, {{{2, 0, 0}, {0, 0, 0}, {0, -2, 0}}}), edge).permitted &&
              stats.filtered_projected == 1,
          "coplanar opposite shared edge missed certified fast case");
  const bgmesh::AllowedSimplex vertex{S::vertex, {bgmesh::NodeSlot{0}, bgmesh::NodeSlot{0}}};
  require(check(facet({0, 4, 5}, {{{0, 0, 0}, {-1, 0, 1}, {0, -1, 1}}}), vertex).permitted &&
              stats.filtered_shared_vertex == 1,
          "declared vertex strict side missed certified fast case");
  require(check(facet({3, 4, 5}, {{{3, 0, 0}, {4, 0, 0}, {3, 1, 0}}}), none).permitted &&
              stats.filtered_separation == 2,
          "coplanar separating edge missed certified filter");
  require(check(facet({3, 4, 5}, {{{1, 2, -1}, {1, 3, 1}, {1, 4, -1}}}), none).permitted &&
              stats.filtered_separation == 3,
          "edge-cross-edge separating axis missed disjoint straddling planes");
  const auto crossing = check(facet({0, 4, 5}, {{{0, 0, 0}, {1, 1, -1}, {1, 1, 1}}}), vertex);
  require(!crossing.permitted && stats.exact_pairs == 1,
          "shared vertex shortcut hid an ambiguous extra crossing");
  const auto inclined = bgmesh::prepare_contact_facet(
      budget, facet({0, 1, 2}, {{{0, 0, 0}, {2, 0, 2}, {0, 2, 2}}}));
  const auto projected_before = stats.filtered_projected;
  const auto inclined_edge = bgmesh::prepare_contact_facet(
      budget, facet({1, 0, 5}, {{{2, 0, 2}, {0, 0, 0}, {0, -2, -2}}}));
  require(bgmesh::classify_contact(budget, inclined, inclined_edge, edge, stats).permitted &&
              stats.filtered_projected == projected_before + 1,
          "inclined shared edge unnecessarily imported exact rationals");
  const auto perturbed_edge = bgmesh::prepare_contact_facet(
      budget, facet({1, 0, 5}, {{{2, 0, 2}, {0, 0, 0}, {0, -2, std::nextafter(-2.0, 0.0)}}}));
  require(bgmesh::classify_contact(budget, inclined, perturbed_edge, edge, stats).permitted &&
              stats.filtered_projected == projected_before + 2,
          "opposite projection shortcut incorrectly requires coplanarity");
  const auto separation_before = stats.filtered_separation;
  const auto inclined_apart = bgmesh::prepare_contact_facet(
      budget, facet({3, 4, 5}, {{{3, 0, 3}, {4, 0, 4}, {3, 1, 4}}}));
  require(bgmesh::classify_contact(budget, inclined, inclined_apart, none, stats).permitted &&
              stats.filtered_separation == separation_before + 1,
          "inclined disjoint projections unnecessarily imported exact rationals");
  const auto before_environment = stats.exact_pairs;
  const auto parallel =
      bgmesh::prepare_contact_facet(budget, facet({3, 4, 5}, {{{0, 0, 1}, {2, 0, 1}, {0, 2, 1}}}));
  const int original_rounding = std::fegetround();
  require(std::fesetround(FE_UPWARD) == 0, "cannot select test rounding mode");
  const auto directed = bgmesh::classify_contact(budget, prepared, parallel, none, stats);
  require(std::fesetround(original_rounding) == 0, "cannot restore rounding mode");
  require(directed.dimension == bgmesh::ContactDimension::empty && directed.permitted &&
              stats.exact_pairs == before_environment + 1,
          "non-nearest environment used unproved interval decision");
  for (const double scale : {1e-200, 1e200}) {
    auto small_a = a;
    auto small_b = facet({3, 4, 5}, {{{0, 0, 1}, {2, 0, 1}, {0, 2, 1}}});
    for (auto *f : {&small_a, &small_b})
      for (auto &p : f->xyz)
        for (auto &x : p)
          x *= scale;
    const auto before = stats.exact_pairs;
    const auto answer =
        bgmesh::classify_contact(budget, bgmesh::prepare_contact_facet(budget, small_a),
                                 bgmesh::prepare_contact_facet(budget, small_b), none, stats);
    require(answer.dimension == bgmesh::ContactDimension::empty && answer.permitted &&
                stats.exact_pairs == before + 1,
            "underflow/overflow contact did not resolve through exact fallback");
  }
  std::cout << "contact_filtered_pairs\t"
            << stats.filtered_separation + stats.filtered_shared_edge +
                   stats.filtered_shared_vertex + stats.filtered_projected
            << "\ncontact_exact_pairs\t" << stats.exact_pairs << '\n';
}

bool overlap(const bgmesh::FacetBox &a, const bgmesh::FacetBox &b) {
  for (unsigned axis = 0; axis < 3; ++axis)
    if (a.upper[axis] < b.lower[axis] || b.upper[axis] < a.lower[axis])
      return false;
  return true;
}
void bvh_updates() {
  bgfd::Budget budget({1u << 28, uint64_t{1} << 34});
  bgmesh::FacetBVH tree(budget);
  std::vector<bgmesh::FacetBox> boxes;
  for (uint32_t i = 0; i < 80; ++i) {
    const double x = static_cast<double>(i % 10), y = static_cast<double>(i / 10);
    boxes.push_back({{bgmesh::FacetSlot{i}, 1}, {x, y, 0}, {x + 1.25, y + 1.25, 0}});
  }
  tree.build(boxes);
  const auto check = [&] {
    for (const auto &box : boxes) {
      const auto candidates = tree.query(box);
      for (const auto &other : boxes) {
        bool found = false;
        for (const auto candidate : candidates)
          found = found || candidate == other.facet;
        require(found == overlap(box, other), "BVH omitted or invented closed-box overlap");
      }
    }
  };
  check();
  for (uint32_t i = 0; i < 20; ++i) {
    tree.erase(boxes[i].facet);
    boxes[i].facet.generation = 2;
    boxes[i].lower[0] += 20;
    boxes[i].upper[0] += 20;
    tree.insert(boxes[i]);
  }
  check();
  bool stale = false;
  try {
    tree.erase({bgmesh::FacetSlot{0}, 1});
  } catch (const bgfd::Failure &) {
    stale = true;
  }
  require(stale && tree.stats().live_facets == boxes.size(), "stale facet erased new generation");
  std::cout << "bvh_box_tests\t" << tree.stats().box_tests << '\n';
  // Finite coordinates may overflow a doubled centroid or surface-area filter.
  // Equal point boxes also force the median fallback instead of an SAH split.
  std::vector<bgmesh::FacetBox> extreme;
  for (uint32_t i = 0; i < 24; ++i)
    extreme.push_back({{bgmesh::FacetSlot{i}, 3}, {1e308, 0, 0}, {1e308, 0, 0}});
  tree.build(extreme);
  require(tree.query(extreme.front()).size() == extreme.size(),
          "overflowing centroid or median fallback omitted identical point boxes");
  for (const auto &box : extreme)
    tree.erase(box.facet);
  require(tree.stats().live_facets == 0 && tree.query(extreme.front()).empty(),
          "last-leaf deletion left live records");
  tree.insert(extreme.front());
  require(tree.query(extreme.front()).size() == 1, "empty index did not accept a new root");
}
} // namespace
int main() {
  try {
    exact_contacts();
    filtered_contacts();
    bvh_updates();
    std::cout << "native exact contacts and mutable BVH PASS\n";
  } catch (const bgfd::Failure &failure) {
    std::cerr << failure.what() << '\n';
    return 1;
  } catch (const std::exception &failure) {
    std::cerr << failure.what() << '\n';
    return 1;
  }
}
