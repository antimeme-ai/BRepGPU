#include "domain.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) {
  throw bgfd::Failure(BGFD_INVALID_DOMAIN, message);
}
[[noreturn]] void internal(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
using Box = DomainBox;
using Segment = DomainSegment;
using SegmentIndex = DomainSegmentIndex;
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
                                     std::span<const DomainLoopView> original, uint64_t seed,
                                     DomainFailureMode mode, DomainInitialSeal seal) {
  if (mode != DomainFailureMode::reject && mode != DomainFailureMode::report_geometry)
    invalid("unknown selected-domain failure mode");
  if (seal != DomainInitialSeal::complete && seal != DomainInitialSeal::deferred)
    invalid("unknown initial domain sealing policy");
  if (mesh.stats().live_triangles != 0 || mesh.stats().sites == 0 ||
      (mesh.stats().sites < 3 && mode == DomainFailureMode::reject))
    invalid("domain requires pristine boundary sites");
  const auto n = mesh.stats().sites;
  if (n > UINT32_MAX)
    invalid("domain site index exhausted");
  if (&mesh.coordinate(SiteSlot{0}, CoordinateAxis::x).budget() != &budget)
    internal("domain/topology budget mismatch");
  InitialDomain result(&budget);
  std::optional<DomainIssue> geometric_issue;
  const auto report = [&](DomainIssue issue) {
    if (mode == DomainFailureMode::reject) {
      if (issue.kind == DomainIssueKind::boundary_contact)
        throw bgfd::Failure(BGFD_CROSSING,
                            "selected domain boundaries cross, overlap or touch nonincidently");
      invalid("selected domain loop is collapsed or has invalid hole placement");
    }
    result.issue = issue;
  };
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
    if (loop.sites.empty() || loop.constraints.size() != loop.sites.size())
      invalid("invalid original domain loop inventory");
    if (loop.sites.size() < 3 && !geometric_issue)
      geometric_issue = DomainIssue{DomainIssueKind::too_few_sites, {{{index, 0}, {}}}, 1};
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
    if (area.sign() == 0 && !geometric_issue)
      geometric_issue = DomainIssue{DomainIssueKind::zero_area, {{{index, 0}, {}}}, 1};
    const int32_t adapter = area.sign() == 0 ? 1 : (loop.outer ? 1 : -1) * area.sign();
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
      const auto occurrence = adapter == 1 ? i : loop.sites.size() - 1 - i;
      segments.push_back({a, b, {index, occurrence}, box});
    }
  }
  budget.charge(seen.size());
  if (std::find(seen.begin(), seen.end(), uint8_t{0}) != seen.end())
    invalid("unowned initial domain site");
  if (geometric_issue) {
    report(*geometric_issue);
    return result;
  }
  {
    std::pmr::vector<DomainPointRef> points(&budget);
    points.reserve(static_cast<size_t>(n));
    for (uint32_t i = 0; i < n; ++i)
      points.push_back({&mesh.coordinate(SiteSlot{i}, CoordinateAxis::x),
                        &mesh.coordinate(SiteSlot{i}, CoordinateAxis::y)});
    SegmentIndex index(budget, points, result.stats, segments);
    if (const auto pair = index.crossing()) {
      report({DomainIssueKind::boundary_contact,
              {segments[(*pair)[0]].original, segments[(*pair)[1]].original},
              2});
      return result;
    }
    for (std::size_t h = 1; h < loops.size(); ++h) {
      const auto &loop = loops[h];
      const auto own = std::span<const Segment>(segments).subspan(
          static_cast<std::size_t>(loop.offset), static_cast<std::size_t>(loop.count));
      if (index.winding_except(SiteSlot{slots[static_cast<std::size_t>(loop.offset)]}, own) != 1) {
        report({DomainIssueKind::hole_placement, {own.front().original, {}}, 1});
        return result;
      }
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
  if (seal == DomainInitialSeal::complete)
    mesh.seal_initial();
  return result;
}
} // namespace bgmesh
