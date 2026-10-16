#include "../native/mesher/domain.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void hole_domain() {
  bgfd::Budget budget({1u << 28, uint64_t{1} << 33});
  bgmesh::MutableFaceTopology mesh(budget, {1, -1, .125, .125});
  const std::array<std::array<double, 2>, 8> xy{
      {{0, 0}, {0, 8}, {8, 8}, {8, 0}, {2, 2}, {6, 2}, {6, 6}, {2, 6}}};
  for (const auto &p : xy)
    (void)mesh.append_site(p);
  const std::array<bgmesh::SiteSlot, 4> outer{{{0}, {1}, {2}, {3}}}, hole{{{4}, {5}, {6}, {7}}};
  const std::array<bgmesh::BoundaryKey, 4> a{{{10, 0}, {11, 0}, {12, 0}, {13, 0}}},
      b{{{20, 0}, {21, 0}, {22, 0}, {23, 0}}};
  const std::array<bgmesh::DomainLoopView, 2> loops{{{false, hole, b}, {true, outer, a}}};
  const auto result = bgmesh::initialize_face_domain(budget, mesh, loops, 17);
  require(result.traversal_adapters == std::pmr::vector<int32_t>({-1, -1}, &budget),
          "original winding not adapted independently of source sense");
  require(mesh.triangles().size() == 8 && mesh.stats().sites == 8,
          "hole triangulation dropped sites/hole");
  for (const auto handle : mesh.triangles()) {
    const auto triangle = mesh.triangle(handle);
    std::cout << "domain_triangle\t" << triangle.sites[0].value << '\t' << triangle.sites[1].value
              << '\t' << triangle.sites[2].value << '\n';
  }
  for (unsigned loop = 0; loop < 2; ++loop)
    for (unsigned i = 0; i < 4; ++i) {
      const auto edge = mesh.find_edge(loops[loop].sites[i], loops[loop].sites[(i + 1) % 4]);
      if (!edge)
        throw std::runtime_error("original source constraint missing");
      const auto row = mesh.edge(*edge);
      require(row.constraint == loops[loop].constraints[i] && row.incident_count == 1,
              "source constraint identity/protection lost under loop reversal");
    }
  std::cout << "domain_candidate_tests\t" << result.stats.segment_candidates
            << "\ndomain_ray_tests\t" << result.stats.ray_candidates << '\n';
}
void invalid_domains() {
  using P = std::array<double, 2>;
  const auto rejected = [](std::span<const P> points, std::span<const unsigned> sizes,
                           bool extra_outer) {
    bgfd::Budget budget({1u << 26, uint64_t{1} << 31});
    bgmesh::MutableFaceTopology mesh(budget);
    std::pmr::vector<bgmesh::SiteSlot> sites(&budget);
    std::pmr::vector<bgmesh::BoundaryKey> keys(&budget);
    for (const auto &p : points) {
      sites.push_back(mesh.append_site(p));
      keys.push_back({77, sites.size()});
    }
    std::pmr::vector<bgmesh::DomainLoopView> loops(&budget);
    unsigned at = 0;
    for (unsigned i = 0; i < sizes.size(); ++i) {
      loops.push_back({i == 0 || extra_outer,
                       std::span<const bgmesh::SiteSlot>(sites).subspan(at, sizes[i]),
                       std::span<const bgmesh::BoundaryKey>(keys).subspan(at, sizes[i])});
      at += sizes[i];
    }
    try {
      (void)bgmesh::initialize_face_domain(budget, mesh, loops, 0);
    } catch (const bgfd::Failure &) {
      return true;
    } catch (const bgmesh::Error &) {
      return true;
    }
    return false;
  };
  const std::array<P, 5> crossed{{{0, 0}, {4, 0}, {0, 4}, {4, 4}, {5, 2}}};
  const std::array<unsigned, 1> five{5};
  require(rejected(crossed, five, false), "nonzero-area crossing admitted");
  const std::array<P, 5> backtrack{{{0, 0}, {4, 0}, {2, 0}, {4, 4}, {0, 4}}};
  require(rejected(backtrack, five, false), "adjacent collinear overlap admitted");
  const std::array<P, 6> touching{{{0, 0}, {4, 0}, {4, 4}, {0, 4}, {2, 0}, {2, 2}}};
  const std::array<unsigned, 1> six{6};
  require(rejected(touching, six, false), "nonincident endpoint-on-edge admitted");
  const std::array<P, 12> nested{{{0, 0},
                                  {10, 0},
                                  {10, 10},
                                  {0, 10},
                                  {2, 2},
                                  {2, 8},
                                  {8, 8},
                                  {8, 2},
                                  {3, 3},
                                  {3, 4},
                                  {4, 4},
                                  {4, 3}}};
  const std::array<unsigned, 3> fours{4, 4, 4};
  require(rejected(nested, fours, false), "nested holes admitted");
  const std::array<P, 8> outside{{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {3, 0}, {3, 1}, {4, 1}, {4, 0}}};
  const std::array<unsigned, 2> two{4, 4};
  require(rejected(outside, two, false), "outside hole admitted");
  require(rejected(outside, two, true), "second outer silently discarded");
}
void protected_collinear() {
  bgfd::Budget budget({1u << 26, uint64_t{1} << 31});
  bgmesh::MutableFaceTopology mesh(budget);
  const std::array<std::array<double, 2>, 5> points{{{0, 0}, {1, 0}, {2, 0}, {2, 2}, {0, 2}}};
  std::array<bgmesh::SiteSlot, 5> sites{};
  std::array<bgmesh::BoundaryKey, 5> keys{};
  for (uint32_t i = 0; i < 5; ++i) {
    sites[i] = mesh.append_site(points[i]);
    keys[i] = {88, i};
  }
  const std::array<bgmesh::DomainLoopView, 1> loops{{{true, sites, keys}}};
  (void)bgmesh::initialize_face_domain(budget, mesh, loops, 3);
  require(mesh.triangles().size() == 3 && mesh.stats().sites == 5,
          "protected collinear site removed or rejected");
}
void reported_geometry() {
  using namespace bgmesh;
  const auto check = [](std::span<const std::array<double, 2>> points,
                        std::span<const uint32_t> sizes, DomainIssueKind expected) {
    bgfd::Budget budget({1u << 26, uint64_t{1} << 31});
    MutableFaceTopology mesh(budget);
    std::pmr::vector<SiteSlot> sites(&budget);
    std::pmr::vector<BoundaryKey> keys(&budget);
    for (const auto &point : points) {
      sites.push_back(mesh.append_site(point));
      keys.push_back({900, sites.size()});
    }
    std::pmr::vector<DomainLoopView> loops(&budget);
    std::size_t first = 0;
    for (const auto size : sizes) {
      loops.push_back({loops.empty(), std::span<const SiteSlot>(sites).subspan(first, size),
                       std::span<const BoundaryKey>(keys).subspan(first, size)});
      first += size;
    }
    const auto admission =
        initialize_face_domain(budget, mesh, loops, 0, DomainFailureMode::report_geometry);
    require(admission.issue && admission.issue->kind == expected,
            "selected geometry issue was thrown or lost");
    require(mesh.stats().live_triangles == 0 && mesh.is_usable(),
            "geometric admission report mutated or poisoned pristine connectivity");
    for (uint32_t i = 0; i < admission.issue->segment_count; ++i) {
      const auto source = admission.issue->segments[i];
      require(source.loop < loops.size() && source.segment < loops[source.loop].sites.size(),
              "reported witness is not an original loop/segment occurrence");
    }
    if (expected == DomainIssueKind::boundary_contact) {
      require(admission.issue->segment_count == 2,
              "crossing lacks the two original source segment witnesses");
      const auto a = admission.issue->segments[0], b = admission.issue->segments[1];
      require(a.loop == 0 && b.loop == 0 &&
                  ((a.segment == 1 && b.segment == 4) || (a.segment == 4 && b.segment == 1)),
              "reversed normalization changed original crossing occurrence identities");
    }
  };
  const std::array<std::array<double, 2>, 5> crossing{{{0, 0}, {4, 0}, {0, 4}, {4, 4}, {5, 2}}};
  const std::array<uint32_t, 1> five{5};
  check(crossing, five, DomainIssueKind::boundary_contact);
  const std::array<std::array<double, 2>, 3> flat{{{0, 0}, {1, 0}, {2, 0}}};
  const std::array<uint32_t, 1> three{3};
  check(flat, three, DomainIssueKind::zero_area);
  const std::array<std::array<double, 2>, 5> tiny_hole{{{0, 0}, {4, 0}, {4, 4}, {0, 4}, {2, 2}}};
  const std::array<uint32_t, 2> four_one{4, 1};
  check(tiny_hole, four_one, DomainIssueKind::too_few_sites);
  const std::array<std::array<double, 2>, 8> outside{
      {{0, 0}, {2, 0}, {2, 2}, {0, 2}, {3, 0}, {3, 1}, {4, 1}, {4, 0}}};
  const std::array<uint32_t, 2> fours{4, 4};
  check(outside, fours, DomainIssueKind::hole_placement);
  bgfd::Budget budget({1u << 26, uint64_t{1} << 31});
  MutableFaceTopology mesh(budget);
  for (const auto &point : flat)
    static_cast<void>(mesh.append_site(point));
  const std::array<SiteSlot, 3> repeated{{{0}, {1}, {1}}};
  const std::array<BoundaryKey, 3> keys{{{900, 1}, {900, 2}, {900, 3}}};
  const std::array<DomainLoopView, 1> malformed{{{true, repeated, keys}}};
  bool structural_error = false;
  try {
    static_cast<void>(
        initialize_face_domain(budget, mesh, malformed, 0, DomainFailureMode::report_geometry));
  } catch (const bgfd::Failure &error) {
    structural_error = error.status == BGFD_INVALID_DOMAIN;
  }
  require(structural_error, "report mode converted a broken source inventory into refinement");
}
void aligned_holes(uint32_t count) {
  bgfd::Budget budget({1u << 28, uint64_t{1} << 36});
  bgmesh::MutableFaceTopology mesh(budget);
  std::pmr::vector<bgmesh::SiteSlot> sites(&budget);
  std::pmr::vector<bgmesh::BoundaryKey> keys(&budget);
  const auto append = [&](std::array<double, 2> p) {
    sites.push_back(mesh.append_site(p));
    keys.push_back({500, sites.size()});
  };
  const double width = 3.0 * count + 1;
  append({0, 0});
  append({width, 0});
  append({width, 3});
  append({0, 3});
  for (uint32_t i = 0; i < count; ++i) {
    const double x = 3.0 * i + 1;
    // Alternating lower/upper-Y anchors exercise both ends of [minY,maxY).
    const std::array<std::array<double, 2>, 4> points{{{x, 1}, {x, 2}, {x + 1, 2}, {x + 1, 1}}};
    for (uint32_t j = 0; j < 4; ++j)
      append(points[(j + (i % 2)) % 4]);
  }
  std::pmr::vector<bgmesh::DomainLoopView> loops(&budget);
  for (uint32_t i = 0; i <= count; ++i)
    loops.push_back({i == 0, std::span<const bgmesh::SiteSlot>(sites).subspan(4 * i, 4),
                     std::span<const bgmesh::BoundaryKey>(keys).subspan(4 * i, 4)});
  const auto result = bgmesh::initialize_face_domain(budget, mesh, loops, 7);
  require(mesh.triangles().size() == 6u * count + 2u && result.stats.ray_queries == count &&
              result.stats.winding_aggregates != 0,
          "aligned/shared-Y holes not admitted through actual prefix aggregation");
  std::cout << "aligned_holes\t" << count << "\tray_candidates\t" << result.stats.ray_candidates
            << "\taggregates\t" << result.stats.winding_aggregates << "\tevent_entries\t"
            << result.stats.event_entries << '\n';
}
} // namespace
int main() {
  try {
    hole_domain();
    invalid_domains();
    protected_collinear();
    reported_geometry();
    aligned_holes(16);
    aligned_holes(64);
    std::cout << "native indexed domain and typed metric CDT PASS\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
