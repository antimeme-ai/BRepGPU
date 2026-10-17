#include "adaptive_trimmed_geometry.hpp"
#include "adaptive_trimmed_numeric.hpp"
#include "geometry_enclosure.hpp"
#include "geometry_jets.hpp"
#include <algorithm>
#include <numeric>
#include <set>

namespace bgmesh::adaptive {
namespace {
const RootDescriptor &admit_triangle(const SourceCache &cache, const SourceTriangleInput &input) {
  admit_numeric_environment();
  if (input.original.value >= cache.original_roots().size())
    throw Error(ErrorCode::invalid_source, "foreign original triangle root");
  for (const auto &value : input.normalized_uv)
    if (!numeric::valid(value) || value.lower < 0. || value.upper > 1.)
      throw Error(ErrorCode::invalid_source, "triangle parameter outside original root");
  for (const auto &value : input.affine_xyz)
    if (!numeric::valid(value))
      throw Error(ErrorCode::invalid_source, "nonfinite same affine triangle");
  if (!numeric::finite(input.allowance_down) || input.allowance_down < 0.)
    throw Error(ErrorCode::invalid_source, "invalid triangle allowance filter");
  return cache.original_roots()[input.original.value];
}
} // namespace
SourceTriangleBounds bound_source_triangle(const SourceCache &cache,
                                           const SourceTriangleInput &input) {
  const auto &root = admit_triangle(cache, input);
  const auto controls = cache.controls();
  if (root.controls.first > controls.size() ||
      root.controls.count > controls.size() - root.controls.first)
    throw Error(ErrorCode::invalid_source, "foreign original triangle controls");
  return numeric::bound_source_triangle_rn64(
      root, root.controls.count ? controls.data() + root.controls.first : nullptr, input);
}
SourceTriangleBounds bound_original_plane(const SourceCache &cache,
                                          const SourceTriangleInput &input) {
  return numeric::bound_original_plane_rn64(admit_triangle(cache, input), input);
}

struct TrimFacetData final {
  SurfaceBoundView surface;
  FaceConstructionResult construction;
  FaceDomainLease domain;
  FacetAllowances allowances;
  ExactTable values;
  TrimFacetWorkKey key;
  TrimFacetLimits limits;
  TrimPreparationStats stats;
  std::pmr::vector<TrimFacetRecord> facets;
  std::pmr::vector<TrimPieceRecord> pieces;
  TrimFacetData(SurfaceBoundView view, FaceConstructionResult mesh, FaceDomainLease policy,
                FacetAllowances quality, ExactTable exact, TrimFacetLimits caps)
      : surface(std::move(view)), construction(std::move(mesh)), domain(std::move(policy)),
        allowances(std::move(quality)), values(std::move(exact)),
        key{surface.key(), construction->key, values.key()}, limits(caps),
        facets(surface.resource().get()), pieces(surface.resource().get()) {}
};
const TrimFacetData &TrimFacetWork::data() const { return *data_; }
const TrimFacetWorkKey &TrimFacetWork::key() const { return data_->key; }
const ExactTable &TrimFacetWork::values() const { return data_->values; }
std::span<const TrimFacetRecord> TrimFacetWork::facets() const { return data_->facets; }
std::span<const TrimPieceRecord> TrimFacetWork::pieces() const { return data_->pieces; }
TrimPreparationStats TrimFacetWork::stats() const { return data_->stats; }

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw Error(ErrorCode::invalid_source, message);
}
Rational exact(bgfd::Budget &budget, const ExactTable &values, uint64_t entry) {
  return import_exact(budget, values.view({values.key(), entry}));
}
std::array<Rational, 6> facet_uv(bgfd::Budget &budget, const FaceConstructionResultData &mesh,
                                 uint32_t facet) {
  require(facet < mesh.facets.size(), "foreign selected facet");
  const auto &ids = mesh.facets[facet].sites;
  for (const auto id : ids)
    require(id < mesh.sites.size(), "foreign facet site");
  const auto value = [&](uint32_t site, uint32_t axis) {
    return import_exact(budget, mesh.values.view(mesh.sites[site].uv[axis]));
  };
  return {value(ids[0], 0), value(ids[0], 1), value(ids[1], 0),
          value(ids[1], 1), value(ids[2], 0), value(ids[2], 1)};
}
std::array<double, 9> facet_xyz(const FaceConstructionResultData &mesh, uint32_t facet) {
  std::array<double, 9> result{};
  const auto &ids = mesh.facets[facet].sites;
  for (uint32_t vertex = 0; vertex < 3; ++vertex)
    for (uint32_t axis = 0; axis < 3; ++axis)
      result[vertex * 3 + axis] = mesh.sites[ids[vertex]].actual_xyz[axis];
  return result;
}
Rational orientation(std::span<const Rational, 6> uv) {
  return uv[2]
      .subtract(uv[0])
      .multiply(uv[5].subtract(uv[1]))
      .subtract(uv[3].subtract(uv[1]).multiply(uv[4].subtract(uv[0])));
}
struct ClipPoint final {
  std::array<Rational, 2> uv;
  std::array<Rational, 3> barycentric;
};
bool same_point(const ClipPoint &a, const ClipPoint &b) {
  return a.uv[0].compare(b.uv[0]) == 0 && a.uv[1].compare(b.uv[1]) == 0;
}
void append_point(std::pmr::vector<ClipPoint> &out, ClipPoint value) {
  if (!out.empty() && same_point(out.back(), value))
    return;
  if (out.size() >= 7)
    throw Error(ErrorCode::internal_error, "triangle rectangle vertex bound");
  out.push_back(std::move(value));
}
struct HalfPlane final {
  uint32_t axis;
  const Rational &coordinate;
  bool lower;
};
void clip_polygon(std::pmr::vector<ClipPoint> &polygon, HalfPlane plane, bgfd::Budget &budget) {
  if (polygon.empty())
    return;
  std::pmr::vector<ClipPoint> next(&budget);
  next.reserve(7);
  const auto distance = [&](const ClipPoint &point) {
    return plane.lower ? point.uv[plane.axis].subtract(plane.coordinate)
                       : plane.coordinate.subtract(point.uv[plane.axis]);
  };
  const auto lerp = [](const Rational &a, const Rational &b, const Rational &t) {
    return a.add(t.multiply(b.subtract(a)));
  };
  for (size_t i = 0; i < polygon.size(); ++i) {
    budget.charge(1);
    const auto &a = polygon[i], &b = polygon[(i + 1) % polygon.size()];
    const auto da = distance(a), db = distance(b);
    if (da.sign() >= 0)
      append_point(next, a);
    if ((da.sign() < 0 && db.sign() >= 0) || (da.sign() >= 0 && db.sign() < 0)) {
      const auto t = da.divide(da.subtract(db));
      append_point(next, {{lerp(a.uv[0], b.uv[0], t), lerp(a.uv[1], b.uv[1], t)},
                          {lerp(a.barycentric[0], b.barycentric[0], t),
                           lerp(a.barycentric[1], b.barycentric[1], t),
                           lerp(a.barycentric[2], b.barycentric[2], t)}});
    }
  }
  if (next.size() > 1 && same_point(next.front(), next.back()))
    next.pop_back();
  polygon = std::move(next);
}
std::array<Rational, 6> triangle_uv(const ClipPoint &a, const ClipPoint &b, const ClipPoint &c) {
  return {a.uv[0], a.uv[1], b.uv[0], b.uv[1], c.uv[0], c.uv[1]};
}
struct LeafBox final {
  uint32_t leaf;
  std::array<double, 4> bounds;
};
struct BoxNode final {
  std::array<double, 4> bounds;
  uint32_t first, count, left, right;
};
class LeafIndex final {
  std::pmr::vector<LeafBox> leaves_;
  std::pmr::vector<BoxNode> nodes_;
  bgfd::Budget &budget_;
  uint32_t build(uint32_t first, uint32_t count) {
    budget_.charge(1);
    auto bounds = leaves_[first].bounds;
    for (uint32_t i = first + 1; i < first + count; ++i)
      for (uint32_t axis = 0; axis < 2; ++axis) {
        bounds[axis * 2] = std::min(bounds[axis * 2], leaves_[i].bounds[axis * 2]);
        bounds[axis * 2 + 1] = std::max(bounds[axis * 2 + 1], leaves_[i].bounds[axis * 2 + 1]);
      }
    const auto at = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back({bounds, first, count, UINT32_MAX, UINT32_MAX});
    if (count <= 8)
      return at;
    const uint32_t axis = bounds[1] - bounds[0] >= bounds[3] - bounds[2] ? 0U : 1U;
    const auto middle = first + count / 2;
    std::nth_element(leaves_.begin() + first, leaves_.begin() + middle,
                     leaves_.begin() + first + count, [&](const auto &a, const auto &b) {
                       budget_.charge(1);
                       const auto ac = a.bounds[axis * 2] * .5 + a.bounds[axis * 2 + 1] * .5,
                                  bc = b.bounds[axis * 2] * .5 + b.bounds[axis * 2 + 1] * .5;
                       return ac < bc || (ac == bc && a.leaf < b.leaf);
                     });
    const auto left = build(first, middle - first), right = build(middle, first + count - middle);
    nodes_[at].left = left;
    nodes_[at].right = right;
    return at;
  }

public:
  LeafIndex(const PatchSelection &selection, bgfd::Budget &budget)
      : leaves_(&budget), nodes_(&budget), budget_(budget) {
    require(selection.leaves.size() <= UINT32_MAX / 2, "selected leaf index capacity");
    leaves_.reserve(selection.leaves.size());
    nodes_.reserve(selection.leaves.size() * 2);
    for (uint32_t i = 0; i < selection.leaves.size(); ++i) {
      LeafBox box{i, {}};
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto lo =
            import_exact(budget, selection.values.view(selection.leaves[i].box[axis * 2]));
        const auto hi =
            import_exact(budget, selection.values.view(selection.leaves[i].box[axis * 2 + 1]));
        require(lo.compare(hi) < 0, "nonpositive selected leaf rectangle");
        box.bounds[axis * 2] = lo.bounds().lower;
        box.bounds[axis * 2 + 1] = hi.bounds().upper;
      }
      leaves_.push_back(box);
    }
    if (!leaves_.empty())
      static_cast<void>(build(0, static_cast<uint32_t>(leaves_.size())));
  }
  std::pmr::vector<uint32_t> candidates(const std::array<double, 4> &box,
                                        TrimPreparationStats &stats, uint64_t visit_limit) {
    std::pmr::vector<uint32_t> result(&budget_), pending(&budget_);
    if (!nodes_.empty())
      pending.push_back(0);
    const auto disjoint = [&](const auto &other) {
      return box[1] < other[0] || other[1] < box[0] || box[3] < other[2] || other[3] < box[2];
    };
    while (!pending.empty()) {
      budget_.charge(1);
      if (++stats.candidate_visits > visit_limit)
        throw Error(ErrorCode::resource_exhausted, "trim facet candidate work");
      const auto &node = nodes_[pending.back()];
      pending.pop_back();
      if (disjoint(node.bounds))
        continue;
      if (node.left == UINT32_MAX) {
        for (uint32_t i = node.first; i < node.first + node.count; ++i) {
          budget_.charge(1);
          if (++stats.candidate_visits > visit_limit)
            throw Error(ErrorCode::resource_exhausted, "trim facet leaf candidates");
          if (!disjoint(leaves_[i].bounds))
            result.push_back(leaves_[i].leaf);
        }
      } else {
        pending.push_back(node.right);
        pending.push_back(node.left);
      }
    }
    std::sort(result.begin(), result.end(), [&](uint32_t a, uint32_t b) {
      budget_.charge(1);
      return a < b;
    });
    return result;
  }
};
bool same_domain_generation(const FaceConstructionKey &a, const FaceConstructionKey &b) {
  return a.definition == b.definition && a.face == b.face &&
         a.source_generation == b.source_generation && a.query == b.query &&
         a.boundary_generation == b.boundary_generation && a.chart_revision == b.chart_revision &&
         a.profile_revision == b.profile_revision && a.assignment_epoch == b.assignment_epoch &&
         effective_face_generation(a) == effective_face_generation(b) &&
         a.domain_revision == b.domain_revision;
}
Interval exact_interval(const Rational &value) {
  const auto bounds = value.bounds();
  return {bounds.lower, bounds.upper};
}
Interval normalized_interval(const Rational &value, const Rational &begin, const Rational &end) {
  const auto normalized = value.subtract(begin).divide(end.subtract(begin));
  require(normalized.sign() >= 0 && normalized.compare(Rational(value.budget(), 1)) <= 0,
          "trim piece outside its exact source box");
  auto interval = exact_interval(normalized);
  // Exact source admission, not unproved interval clipping.
  interval.lower = std::max(0., interval.lower);
  interval.upper = std::min(1., interval.upper);
  return interval;
}
} // namespace

namespace {
std::shared_ptr<const TrimFacetData>
prepare_work_data(JobContext &context, SurfaceBoundView surface,
                  FaceConstructionResult construction, FaceDomainLease domain,
                  FacetAllowances allowances, std::span<const uint32_t> selected_facets,
                  ExactTableKey result_key, TrimFacetLimits limits, bool candidates_only) {
  admit_numeric_environment();
  require(surface && construction && domain, "missing trimmed source/domain/view lease");
  auto &budget = context.budget;
  const auto work_start = budget.work();
  const auto check_work = [&] {
    if (budget.work() - work_start > limits.work)
      throw Error(ErrorCode::resource_exhausted, "trimmed facet preparation work");
  };
  const auto resource = surface.resource();
  const auto selection = surface.selection();
  require(resource && selection && construction->resource == resource &&
              domain->resource == resource && allowances.values.resource() == resource &&
              &construction->source.view() == &surface.source().source().view() &&
              &domain->source.view() == &construction->source.view() &&
              &construction->surface_source.view() == &surface.source().view(),
          "trim facet source owner mismatch");
  const auto &identity = construction->key;
  const auto view_key = surface.key();
  require(
      construction->surface && construction->surface.key() == view_key &&
          construction->surface.selection() == selection && surface.classified_domain() == domain &&
          same_domain_generation(identity, domain->key) && domain == construction->domain &&
          identity.definition == view_key.face.definition && identity.face == view_key.face.face &&
          identity.source_generation == view_key.face.source_generation &&
          identity.query == view_key.selection.query &&
          identity.boundary_generation == view_key.face.boundary_generation &&
          effective_face_generation(identity) == view_key.face.face_generation &&
          identity.domain_revision == view_key.face.domain_revision &&
          identity.chart_revision == view_key.face.chart_revision &&
          identity.profile_revision == view_key.selection.profile_revision &&
          identity.assignment_epoch == view_key.face.assignment_epoch,
      "stale trim facet source/domain/view generation");
  require(limits.pieces && limits.exact_entries && limits.batch_records &&
              limits.candidate_visits && limits.work && limits.precise_tasks &&
              selected_facets.size() <= UINT32_MAX && result_key != construction->values.key() &&
              result_key != selection->values.key() && result_key != domain->values.key() &&
              result_key != allowances.values.key(),
          "invalid trim work limits/key");
  const auto work_scope = budget.scoped_work(limits.work);
  const auto allowance = import_exact(budget, allowances.values.view(allowances.face_position));
  require(allowance.sign() > 0, "nonpositive original facet positional allowance");
  const auto &source = construction->source.view();
  require(identity.face.value < source.faces.size() &&
              identity.definition.value < source.definitions.size(),
          "foreign trimmed face/definition");
  const auto &face = source.faces[identity.face.value];
  require(face.definition_id == source.definitions[identity.definition.value].id,
          "trimmed face belongs to another definition");
  const auto carrier = source.carrier_index.find(face.carrier_id);
  // The construction partitions the selected domain, and conservative outside
  // pruning removes no material. Admit the COMPLETE prepared carrier closure
  // once, rather than recomputing a fraction area certificate for every facet.
  const auto closure = surface.prepared_roots();
  for (size_t i = 0; i < closure.size(); ++i) {
    budget.charge(1);
    require(closure[i].value < surface.source().roots().size() &&
                (i == 0 || closure[i - 1].value < closure[i].value),
            "invalid surface source root closure");
  }
  for (uint32_t i = 0; i < surface.source().roots().size(); ++i) {
    budget.charge(1);
    if (surface.source().roots()[i].source_slot == carrier.value)
      require(std::binary_search(closure.begin(), closure.end(), RootSlot{i},
                                 [&](RootSlot a, RootSlot b) {
                                   budget.charge(1);
                                   return a.value < b.value;
                                 }),
              "trim view omitted a prepared face carrier panel");
  }
  check_work();
  size_t use_index = 0;
  for (const auto loop_id :
       std::span(source.loop_ids).subspan(face.loops.first, face.loops.count)) {
    const auto uses = source.loops[source.loop_index.find(loop_id).value].coedges;
    for (const auto use_id : std::span(source.coedge_ids).subspan(uses.first, uses.count)) {
      budget.charge(1);
      require(use_index < domain->uses.size(), "domain omitted original face use");
      const auto use = source.coedge_index.find(use_id);
      const auto &policy = domain->uses[use_index++];
      require(policy.use == use &&
                  ((source.coedges[use.value].reconstructed &&
                    policy.policy == DomainPolicy::reconstructed_selected) ||
                   (!source.coedges[use.value].reconstructed &&
                    policy.policy == DomainPolicy::authoritative_exact_image && policy.image)),
              "missing original authoritative face-domain map");
    }
  }
  require(use_index == domain->uses.size(), "extra domain source use");
  for (const auto &leaf : selection->leaves) {
    budget.charge(1);
    require(leaf.key.root.value < surface.source().roots().size() &&
                surface.source().roots()[leaf.key.root.value].source_slot == carrier.value,
            "foreign selected facet carrier root");
  }
  std::pmr::vector<uint32_t> requested(selected_facets.begin(), selected_facets.end(), &budget);
  std::sort(requested.begin(), requested.end(), [&](uint32_t a, uint32_t b) {
    budget.charge(1);
    require(a < construction->facets.size() && b < construction->facets.size(),
            "foreign facet subset");
    return construction->facets[a].key < construction->facets[b].key;
  });
  for (size_t i = 0; i < requested.size(); ++i)
    require(requested[i] < construction->facets.size() &&
                (i == 0 || construction->facets[requested[i - 1]].key !=
                               construction->facets[requested[i]].key),
            "duplicate/foreign requested facet");
  ExactBuilder builder(resource, result_key);
  std::pmr::vector<TrimFacetRecord> facets(resource.get());
  std::pmr::vector<TrimPieceRecord> pieces(resource.get());
  facets.reserve(requested.size());
  TrimPreparationStats stats;
  std::optional<LeafIndex> index;
  uint64_t exact_entries = 0;
  for (const auto facet_id : requested) {
    require(construction->facets[facet_id].key.generator != FacetGenerator::mapped_cap &&
                !construction->facets[facet_id].mapped,
            "mapped cap facet entered ordinary source-triangle work");
    const auto uv = facet_uv(budget, *construction, facet_id);
    const auto xyz = facet_xyz(*construction, facet_id);
    const auto parent_area = orientation(uv);
    require(parent_area.sign() > 0, "nonpositive chosen-chart facet");
    for (uint32_t vertex = 0; vertex < 3; ++vertex) {
      const std::array point{uv[vertex * 2], uv[vertex * 2 + 1]};
      (void)find_carrier_root_slot(surface.source().roots(), carrier, point, RootBoxKind::prepared);
    }
    check_work();
    TrimFacetRecord parent{
        facet_id, {pieces.size(), 0}, false, actual_facet_nonzero(xyz, budget), {}, {}};
    const std::array first_point{uv[0], uv[1]};
    const auto original = find_original_carrier_root(surface.source(), carrier, first_point);
    parent.original = original;
    const auto &root = surface.source().original_roots()[original.value];
    if (root.family == GeometryFamily::plane) {
      for (uint32_t vertex = 0; vertex < 3; ++vertex)
        for (uint32_t axis = 0; axis < 2; ++axis)
          require(uv[vertex * 2 + axis].compare(
                      Rational::from_double(budget, root.original_box[axis * 2].lower)) >= 0 &&
                      uv[vertex * 2 + axis].compare(Rational::from_double(
                          budget, root.original_box[axis * 2 + 1].upper)) <= 0,
                  "original plane facet outside prepared source envelope");
      parent.original_plane = true;
      ++stats.plane_facets;
      facets.push_back(parent);
      continue;
    }
    if (candidates_only) {
      const auto &facet = construction->facets[facet_id];
      if (facet.key.generator == FacetGenerator::packet) {
        require(facet.leaf < selection->leaves.size(), "packet bound has no captured leaf");
        const auto &leaf = selection->leaves[facet.leaf];
        require(leaf.key.root.value < surface.source().roots().size(), "packet source root absent");
        const auto &prepared = surface.source().roots()[leaf.key.root.value];
        require(prepared.source_slot == carrier.value, "packet leaf names foreign source carrier");
        for (uint32_t vertex = 0; vertex < 3; ++vertex)
          for (uint32_t axis = 0; axis < 2; ++axis)
            require(uv[2 * vertex + axis].compare(
                        import_exact(budget, selection->values.view(leaf.box[2 * axis]))) >= 0 &&
                        uv[2 * vertex + axis].compare(import_exact(
                            budget, selection->values.view(leaf.box[2 * axis + 1]))) <= 0,
                    "packet actual triangle escapes captured leaf");
        if (pieces.size() >= limits.pieces || pieces.size() >= UINT32_MAX)
          throw Error(ErrorCode::resource_exhausted, "packet direct piece capacity");
        parent.original = prepared.original;
        parent.whole_original = prepared.original;
        pieces.push_back({static_cast<uint32_t>(facets.size()),
                          facet.leaf,
                          prepared.original,
                          {},
                          {},
                          TrimPieceGeometry::whole_parent});
        parent.pieces.count = 1;
        ++stats.pieces;
      } else {
        require(facet.key.generator == FacetGenerator::band, "unknown ordinary facet generator");
        const std::array middle{uv[0].add(uv[2]).add(uv[4]).divide(Rational(budget, 3)),
                                uv[1].add(uv[3]).add(uv[5]).divide(Rational(budget, 3))};
        const auto selected = find_original_carrier_root(surface.source(), carrier, middle);
        const auto &support = surface.source().original_roots()[selected.value];
        bool contained = true;
        for (uint32_t vertex = 0; vertex < 3; ++vertex)
          for (uint32_t axis = 0; axis < 2; ++axis)
            contained =
                contained &&
                uv[2 * vertex + axis].compare(
                    Rational::from_double(budget, support.original_box[2 * axis].lower)) >= 0 &&
                uv[2 * vertex + axis].compare(
                    Rational::from_double(budget, support.original_box[2 * axis + 1].upper)) <= 0;
        parent.original = selected;
        if (contained)
          parent.whole_original = selected;
      }
      facets.push_back(parent);
      check_work();
      continue;
    }
    if (!index)
      index.emplace(*selection, budget);
    std::array<Rational, 4> box{uv[0], uv[0], uv[1], uv[1]};
    for (uint32_t vertex = 1; vertex < 3; ++vertex)
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto &coordinate = uv[vertex * 2 + axis];
        if (coordinate.compare(box[axis * 2]) < 0)
          box[axis * 2] = coordinate;
        if (coordinate.compare(box[axis * 2 + 1]) > 0)
          box[axis * 2 + 1] = coordinate;
      }
    const std::array<double, 4> filter_box{box[0].bounds().lower, box[1].bounds().upper,
                                           box[2].bounds().lower, box[3].bounds().upper};
    const auto candidates = index->candidates(filter_box, stats, limits.candidate_visits);
    const Rational zero(budget), one(budget, 1);
    for (const auto leaf_id : candidates) {
      const auto &leaf = selection->leaves[leaf_id];
      std::array<Rational, 4> bounds{import_exact(budget, selection->values.view(leaf.box[0])),
                                     import_exact(budget, selection->values.view(leaf.box[1])),
                                     import_exact(budget, selection->values.view(leaf.box[2])),
                                     import_exact(budget, selection->values.view(leaf.box[3]))};
      std::pmr::vector<ClipPoint> polygon(&budget);
      polygon.reserve(7);
      polygon.push_back({{uv[0], uv[1]}, {one, zero, zero}});
      polygon.push_back({{uv[2], uv[3]}, {zero, one, zero}});
      polygon.push_back({{uv[4], uv[5]}, {zero, zero, one}});
      for (uint32_t plane = 0; plane < 4; ++plane) {
        clip_polygon(polygon, {plane / 2, bounds[plane], plane % 2 == 0}, budget);
        check_work();
      }
      stats.clip_vertices += polygon.size();
      for (size_t fan = 1; fan + 1 < polygon.size(); ++fan) {
        const std::array<const ClipPoint *, 3> vertices{&polygon[0], &polygon[fan],
                                                        &polygon[fan + 1]};
        const auto piece_uv = triangle_uv(*vertices[0], *vertices[1], *vertices[2]);
        const auto area = orientation(piece_uv);
        require(area.sign() >= 0, "virtual clip changed facet orientation");
        if (area.sign() == 0)
          continue;
        if (pieces.size() >= limits.pieces || pieces.size() >= UINT32_MAX ||
            exact_entries > limits.exact_entries || 15 > limits.exact_entries - exact_entries)
          throw Error(ErrorCode::resource_exhausted, "trimmed facet piece/coordinate capacity");
        TrimPieceRecord piece{static_cast<uint32_t>(facets.size()),
                              leaf_id,
                              surface.source().roots()[leaf.key.root.value].original,
                              {exact_entries, 6},
                              {exact_entries + 6, 9}};
        for (const auto &value : piece_uv)
          static_cast<void>(builder.append(value));
        for (const auto *vertex : vertices)
          for (const auto &lambda : vertex->barycentric)
            static_cast<void>(builder.append(lambda));
        exact_entries += 15;
        pieces.push_back(piece);
        ++stats.pieces;
        check_work();
      }
    }
    parent.pieces.count = pieces.size() - parent.pieces.first;
    require(parent.pieces.count != 0, "actual facet has no selected source pieces");
    facets.push_back(parent);
    check_work();
  }
  auto data = std::allocate_shared<TrimFacetData>(
      RunAllocator<TrimFacetData>(resource), std::move(surface), std::move(construction),
      std::move(domain), std::move(allowances), builder.seal(), limits);
  data->facets = std::move(facets);
  data->pieces = std::move(pieces);
  data->stats = stats;
  return data;
}
} // namespace
TrimFacetWork prepare_trim_facet_work(JobContext &context, SurfaceBoundView surface,
                                      FaceConstructionResult construction, FaceDomainLease domain,
                                      FacetAllowances allowances,
                                      std::span<const uint32_t> selected, ExactTableKey key,
                                      TrimFacetLimits limits) {
  return TrimFacetWork(prepare_work_data(context, std::move(surface), std::move(construction),
                                         std::move(domain), std::move(allowances), selected, key,
                                         limits, false));
}
TrimFacetWork prepare_trim_facet_candidates(JobContext &context, SurfaceBoundView surface,
                                            FaceConstructionResult construction,
                                            FaceDomainLease domain, FacetAllowances allowances,
                                            std::span<const uint32_t> selected, ExactTableKey key,
                                            TrimFacetLimits limits) {
  return TrimFacetWork(prepare_work_data(context, std::move(surface), std::move(construction),
                                         std::move(domain), std::move(allowances), selected, key,
                                         limits, true));
}
TrimFacetWork materialize_trim_facet_fallback(JobContext &context, const TrimFacetWork &candidate,
                                              std::span<const uint32_t> selected,
                                              ExactTableKey key) {
  const auto &seed = candidate.data();
  require(key != candidate.values().key(), "fallback exact table aliases candidate work");
  auto &budget = context.budget;
  std::pmr::vector<uint32_t> indices(selected.begin(), selected.end(), &budget), facets(&budget);
  std::sort(indices.begin(), indices.end(), [&](uint32_t a, uint32_t b) {
    budget.charge(1);
    return a < b;
  });
  require(std::adjacent_find(indices.begin(), indices.end()) == indices.end(),
          "duplicate fallback parent");
  for (auto row : indices) {
    budget.charge(1);
    require(row < candidate.facets().size(), "fallback parent outside capture");
    const auto &parent = candidate.facets()[row];
    require(!parent.original_plane && parent.pieces.count == 0 &&
                seed.construction->facets[parent.construction_facet].key.generator ==
                    FacetGenerator::band,
            "fallback subset is not deferred band work");
    facets.push_back(parent.construction_facet);
  }
  auto clipped = prepare_work_data(context, seed.surface, seed.construction, seed.domain,
                                   seed.allowances, facets, key, seed.limits, false);
  auto merged = std::allocate_shared<TrimFacetData>(
      RunAllocator<TrimFacetData>(seed.surface.resource()), seed.surface, seed.construction,
      seed.domain, seed.allowances, clipped->values, seed.limits);
  merged->stats = clipped->stats;
  merged->stats.plane_facets += seed.stats.plane_facets;
  size_t at = 0;
  for (uint32_t row = 0; row < seed.facets.size(); ++row) {
    budget.charge(1);
    auto parent = seed.facets[row];
    const auto first = merged->pieces.size();
    if (at < indices.size() && indices[at] == row) {
      const auto &added = clipped->facets[at++];
      require(added.construction_facet == parent.construction_facet && added.pieces.count,
              "fallback parent map differs or lacks source pieces");
      for (uint64_t i = 0; i < added.pieces.count; ++i) {
        budget.charge(1);
        auto piece = clipped->pieces[added.pieces.first + i];
        piece.parent = row;
        merged->pieces.push_back(piece);
      }
    } else {
      for (uint64_t i = 0; i < parent.pieces.count; ++i) {
        budget.charge(1);
        auto piece = seed.pieces[parent.pieces.first + i];
        require(piece.geometry == TrimPieceGeometry::whole_parent,
                "candidate already contains materialized virtual coordinates");
        piece.parent = row;
        merged->pieces.push_back(piece);
      }
    }
    parent.pieces = {first, merged->pieces.size() - first};
    merged->facets.push_back(parent);
  }
  require(at == indices.size(), "fallback parent merge is incomplete");
  if (merged->pieces.size() > seed.limits.pieces || merged->pieces.size() > UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "merged fallback piece capacity");
  merged->stats.pieces = merged->pieces.size();
  return TrimFacetWork(std::move(merged));
}

namespace {
void check_batch(const TrimFacetWork &work, std::span<const uint32_t> indices, size_t count,
                 bgfd::Budget &budget) {
  require(!indices.empty() && indices.size() <= work.data().limits.batch_records,
          "invalid trimmed numerical batch size");
  std::pmr::vector<uint32_t> ordered(indices.begin(), indices.end(), &budget);
  for (const auto index : ordered)
    require(index < count, "foreign trimmed numerical row");
  std::sort(ordered.begin(), ordered.end(), [&](uint32_t a, uint32_t b) {
    budget.charge(1);
    return a < b;
  });
  require(std::adjacent_find(ordered.begin(), ordered.end()) == ordered.end(),
          "duplicate trimmed numerical row");
}
std::array<Rational, 6> piece_uv(bgfd::Budget &budget, const TrimFacetWork &work,
                                 const TrimPieceRecord &piece) {
  if (piece.geometry == TrimPieceGeometry::whole_parent) {
    require(piece.parent < work.facets().size(), "direct piece parent missing");
    return facet_uv(budget, *work.data().construction,
                    work.facets()[piece.parent].construction_facet);
  }
  require(piece.original_uv.count == 6, "trim UV coordinate shape");
  const auto at = piece.original_uv.first;
  return {exact(budget, work.values(), at),     exact(budget, work.values(), at + 1),
          exact(budget, work.values(), at + 2), exact(budget, work.values(), at + 3),
          exact(budget, work.values(), at + 4), exact(budget, work.values(), at + 5)};
}
std::array<Rational, 9> piece_xyz_exact(bgfd::Budget &budget, const TrimFacetWork &work,
                                        const TrimPieceRecord &piece) {
  if (piece.geometry == TrimPieceGeometry::whole_parent) {
    require(piece.parent < work.facets().size(), "direct piece parent missing");
    const auto xyz =
        facet_xyz(*work.data().construction, work.facets()[piece.parent].construction_facet);
    return {Rational::from_double(budget, xyz[0]), Rational::from_double(budget, xyz[1]),
            Rational::from_double(budget, xyz[2]), Rational::from_double(budget, xyz[3]),
            Rational::from_double(budget, xyz[4]), Rational::from_double(budget, xyz[5]),
            Rational::from_double(budget, xyz[6]), Rational::from_double(budget, xyz[7]),
            Rational::from_double(budget, xyz[8])};
  }
  require(piece.parent < work.facets().size() && piece.barycentric.count == 9,
          "trim exact SAME-L barycentric shape/parent");
  const auto xyz =
      facet_xyz(*work.data().construction, work.facets()[piece.parent].construction_facet);
  std::array<Rational, 9> result{Rational(budget), Rational(budget), Rational(budget),
                                 Rational(budget), Rational(budget), Rational(budget),
                                 Rational(budget), Rational(budget), Rational(budget)};
  for (uint32_t vertex = 0; vertex < 3; ++vertex)
    for (uint32_t corner = 0; corner < 3; ++corner) {
      const auto lambda =
          exact(budget, work.values(), piece.barycentric.first + 3 * vertex + corner);
      for (uint32_t axis = 0; axis < 3; ++axis)
        result[3 * vertex + axis] = result[3 * vertex + axis].add(
            lambda.multiply(Rational::from_double(budget, xyz[3 * corner + axis])));
    }
  return result;
}
std::array<Interval, 9> piece_xyz_intervals(bgfd::Budget &budget, const TrimFacetWork &work,
                                            const TrimPieceRecord &piece) {
  require(piece.parent < work.facets().size(), "trim interval SAME-L parent missing");
  const auto xyz =
      facet_xyz(*work.data().construction, work.facets()[piece.parent].construction_facet);
  std::array<Interval, 9> result{};
  if (piece.geometry == TrimPieceGeometry::whole_parent) {
    for (uint32_t i = 0; i < 9; ++i)
      result[i] = numeric::point(xyz[i]);
    return result;
  }
  require(piece.barycentric.count == 9, "trim interval SAME-L barycentric shape");
  for (uint32_t vertex = 0; vertex < 3; ++vertex)
    for (uint32_t corner = 0; corner < 3; ++corner) {
      const auto lambda = exact_interval(
          exact(budget, work.values(), piece.barycentric.first + 3 * vertex + corner));
      for (uint32_t axis = 0; axis < 3; ++axis)
        result[3 * vertex + axis] =
            numeric::add(result[3 * vertex + axis],
                         numeric::mul(lambda, numeric::point(xyz[3 * corner + axis])));
    }
  for (const auto &coordinate : result)
    if (!numeric::valid(coordinate))
      throw Error(ErrorCode::unresolved,
                  "trim SAME-L interval dot exceeds finite arithmetic profile");
  return result;
}
Rational work_allowance(bgfd::Budget &budget, const TrimFacetWork &work) {
  const auto &quality = work.data().allowances;
  return import_exact(budget, quality.values.view(quality.face_position));
}
OriginalRootSlot whole_source_root(const TrimFacetRecord &parent, const char *message) {
  if (parent.original_plane)
    return parent.original;
  if (parent.whole_original.has_value())
    return parent.whole_original.value();
  throw Error(ErrorCode::invalid_source, message);
}
std::array<Rational, 4> leaf_box(bgfd::Budget &budget, const TrimFacetWork &work,
                                 const TrimPieceRecord &piece) {
  const auto selection = work.data().surface.selection();
  require(piece.leaf < selection->leaves.size(), "foreign trim piece leaf");
  const auto &leaf = selection->leaves[piece.leaf];
  return {import_exact(budget, selection->values.view(leaf.box[0])),
          import_exact(budget, selection->values.view(leaf.box[1])),
          import_exact(budget, selection->values.view(leaf.box[2])),
          import_exact(budget, selection->values.view(leaf.box[3]))};
}
SourceTriangleInput source_input(bgfd::Budget &budget, const SourceCache &cache,
                                 OriginalRootSlot original, std::span<const Rational, 6> uv,
                                 std::span<const Interval, 9> xyz, const Rational &allowance) {
  require(original.value < cache.original_roots().size(), "foreign source triangle root");
  const auto &root = cache.original_roots()[original.value];
  SourceTriangleInput input;
  input.original = original;
  input.allowance_down = allowance.bounds().lower;
  for (uint32_t vertex = 0; vertex < 3; ++vertex)
    for (uint32_t axis = 0; axis < 2; ++axis) {
      const auto lo = Rational::from_double(budget, root.original_box[axis * 2].lower),
                 hi = Rational::from_double(budget, root.original_box[axis * 2 + 1].upper);
      input.normalized_uv[vertex * 2 + axis] = normalized_interval(uv[vertex * 2 + axis], lo, hi);
    }
  for (uint32_t i = 0; i < 9; ++i)
    input.affine_xyz[i] = xyz[i];
  return input;
}
SourceTriangleInput source_input(bgfd::Budget &budget, const SourceCache &cache,
                                 OriginalRootSlot original, std::span<const Rational, 6> uv,
                                 std::span<const Rational, 9> xyz, const Rational &allowance) {
  std::array<Interval, 9> enclosed{};
  for (uint32_t i = 0; i < 9; ++i)
    enclosed[i] = exact_interval(xyz[i]);
  return source_input(budget, cache, original, uv, enclosed, allowance);
}
} // namespace
TrimBaseBatch pack_base_pieces(JobContext &context, const TrimFacetWork &work,
                               std::span<const uint32_t> indices) {
  auto &budget = context.budget;
  check_batch(work, indices, work.pieces().size(), budget);
  const auto resource = work.data().surface.resource();
  TrimBaseBatch batch(work, resource.get());
  batch.indices.assign(indices.begin(), indices.end());
  batch.inputs.reserve(indices.size());
  const auto selection = work.data().surface.selection();
  for (const auto index : indices) {
    const auto &piece = work.pieces()[index];
    const auto uv = piece_uv(budget, work, piece);
    const auto box = leaf_box(budget, work, piece);
    const auto xyz = piece_xyz_intervals(budget, work, piece);
    const auto &leaf = selection->leaves[piece.leaf];
    ActualFacetInput input;
    input.source_to_bilinear_upper = leaf.source_to_bilinear_upper;
    for (uint32_t corner = 0; corner < 4; ++corner) {
      require(leaf.corners[corner].value < selection->points.size(), "foreign bilinear corner");
      const auto &point = selection->points[leaf.corners[corner].value];
      for (uint32_t axis = 0; axis < 3; ++axis)
        input.actual_corners[corner * 3 + axis] = point.actual_xyz[axis];
    }
    for (uint32_t vertex = 0; vertex < 3; ++vertex)
      for (uint32_t axis = 0; axis < 2; ++axis)
        input.uv[vertex * 2 + axis] =
            normalized_interval(uv[vertex * 2 + axis], box[axis * 2], box[axis * 2 + 1]);
    for (uint32_t i = 0; i < 9; ++i)
      input.affine_xyz[i] = xyz[i];
    batch.inputs.push_back(input);
  }
  return batch;
}
TrimSourceBatch pack_source_pieces(JobContext &context, const TrimFacetWork &work,
                                   std::span<const uint32_t> indices) {
  auto &budget = context.budget;
  check_batch(work, indices, work.pieces().size(), budget);
  TrimSourceBatch batch(work, work.data().surface.resource().get());
  batch.indices.assign(indices.begin(), indices.end());
  batch.inputs.reserve(indices.size());
  const auto allowance = work_allowance(budget, work);
  for (const auto index : indices) {
    const auto &piece = work.pieces()[index];
    const auto uv = piece_uv(budget, work, piece);
    const auto xyz = piece_xyz_intervals(budget, work, piece);
    batch.inputs.push_back(
        source_input(budget, work.data().surface.source(), piece.original, uv, xyz, allowance));
  }
  return batch;
}
TrimPlaneBatch pack_plane_facets(JobContext &context, const TrimFacetWork &work,
                                 std::span<const uint32_t> indices) {
  auto &budget = context.budget;
  check_batch(work, indices, work.facets().size(), budget);
  TrimPlaneBatch batch(work, work.data().surface.resource().get());
  batch.indices.assign(indices.begin(), indices.end());
  batch.inputs.reserve(indices.size());
  const auto allowance = work_allowance(budget, work);
  for (const auto index : indices) {
    const auto &parent = work.facets()[index];
    require(parent.original_plane, "nonplane submitted to plane facet branch");
    const auto uv = facet_uv(budget, *work.data().construction, parent.construction_facet);
    const auto actual = facet_xyz(*work.data().construction, parent.construction_facet);
    std::array<Rational, 9> xyz{Rational(budget), Rational(budget), Rational(budget),
                                Rational(budget), Rational(budget), Rational(budget),
                                Rational(budget), Rational(budget), Rational(budget)};
    for (uint32_t i = 0; i < 9; ++i)
      xyz[i] = Rational::from_double(budget, actual[i]);
    batch.inputs.push_back(
        source_input(budget, work.data().surface.source(), parent.original, uv, xyz, allowance));
  }
  return batch;
}
TrimSourceBatch pack_original_facets(JobContext &context, const TrimFacetWork &work,
                                     std::span<const uint32_t> indices) {
  auto &budget = context.budget;
  check_batch(work, indices, work.facets().size(), budget);
  TrimSourceBatch batch(work, work.data().surface.resource().get());
  batch.indices.assign(indices.begin(), indices.end());
  const auto allowance = work_allowance(budget, work);
  for (auto index : indices) {
    const auto &parent = work.facets()[index];
    const auto original = whole_source_root(parent, "whole source support unavailable");
    const auto uv = facet_uv(budget, *work.data().construction, parent.construction_facet);
    const auto actual = facet_xyz(*work.data().construction, parent.construction_facet);
    std::array<Rational, 9> xyz{Rational(budget), Rational(budget), Rational(budget),
                                Rational(budget), Rational(budget), Rational(budget),
                                Rational(budget), Rational(budget), Rational(budget)};
    for (uint32_t i = 0; i < 9; ++i)
      xyz[i] = Rational::from_double(budget, actual[i]);
    batch.inputs.push_back(
        source_input(budget, work.data().surface.source(), original, uv, xyz, allowance));
  }
  return batch;
}
TrimPreciseResult compare_trim_original_facet_precise(JobContext &context,
                                                      const TrimFacetWork &work, uint32_t index) {
  require(index < work.facets().size(), "precise direct parent missing");
  const auto &parent = work.facets()[index];
  const auto original = whole_source_root(parent, "precise whole source support unavailable");
  auto &budget = context.budget;
  const auto uv = facet_uv(budget, *work.data().construction, parent.construction_facet);
  const auto actual = facet_xyz(*work.data().construction, parent.construction_facet);
  std::array<Rational, 9> xyz{Rational(budget), Rational(budget), Rational(budget),
                              Rational(budget), Rational(budget), Rational(budget),
                              Rational(budget), Rational(budget), Rational(budget)};
  for (uint32_t i = 0; i < 9; ++i)
    xyz[i] = Rational::from_double(budget, actual[i]);
  const auto allowance = work_allowance(budget, work);
  return compare_original_triangle_precise(work.data().surface.source(), original, uv, xyz,
                                           allowance, budget);
}
namespace {
Rational norm_square(std::span<const Rational> vector, bgfd::Budget &budget) {
  auto result = Rational(budget);
  for (const auto &coordinate : vector)
    result = result.add(coordinate.multiply(coordinate));
  return result;
}
Rational sqrt_upper(const Rational &square) {
  if (square.sign() == 0)
    return square;
  const auto numerator = square.numerator().bit_length(),
             denominator = square.denominator().bit_length();
  const auto gap = denominator > numerator ? denominator - numerator : 0;
  if (gap > 8192)
    throw Error(ErrorCode::unresolved, "trim norm exponent profile");
  return square.square_root_bounds(160 + static_cast<uint32_t>((gap + 1) / 2)).upper;
}
Rational enclosure_norm(std::span<const ExactEnclosure, 3> values, bgfd::Budget &budget) {
  std::array<Rational, 3> magnitudes{Rational(budget), Rational(budget), Rational(budget)};
  for (uint32_t axis = 0; axis < 3; ++axis) {
    const auto lo =
        values[axis].lower.sign() < 0 ? values[axis].lower.negative() : values[axis].lower;
    const auto hi =
        values[axis].upper.sign() < 0 ? values[axis].upper.negative() : values[axis].upper;
    magnitudes[axis] = lo.compare(hi) > 0 ? lo : hi;
  }
  return sqrt_upper(norm_square(magnitudes, budget));
}
TrimPreciseResult precise_source(const SourceCache &cache, OriginalRootSlot original,
                                 std::span<const Rational, 6> uv, std::span<const Rational, 9> xyz,
                                 const Rational &allowance, bgfd::Budget &budget) {
  admit_numeric_environment();
  const auto &root = cache.original_roots()[original.value];
  for (uint32_t vertex = 0; vertex < 3; ++vertex)
    for (uint32_t axis = 0; axis < 2; ++axis)
      require(uv[vertex * 2 + axis].compare(
                  Rational::from_double(budget, root.original_box[axis * 2].lower)) >= 0 &&
                  uv[vertex * 2 + axis].compare(
                      Rational::from_double(budget, root.original_box[axis * 2 + 1].upper)) <= 0,
              "precise actual triangle outside original root");
  auto upper = Rational(budget);
  bool certified = false;
  if (root.family == GeometryFamily::rational_tensor) {
    const auto net = surface_detail::original_net_exact(cache, original, budget);
    const auto triangle = restrict_to_triangle(net, uv);
    const auto d = triangle.degree;
    auto wmin = triangle.controls[3], maximum_square = Rational(budget);
    for (size_t at = 7; at < triangle.controls.size(); at += 4)
      if (triangle.controls[at].compare(wmin) < 0)
        wmin = triangle.controls[at];
    require(wmin.sign() > 0, "precise original triangle positive weights");
    for (uint32_t i = 0; i <= d + 1; ++i)
      for (uint32_t j = 0; j <= d + 1 - i; ++j) {
        const uint32_t alpha[]{d + 1 - i - j, i, j};
        std::array<Rational, 3> residual{Rational(budget), Rational(budget), Rational(budget)};
        for (uint32_t vertex = 0; vertex < 3; ++vertex) {
          if (alpha[vertex] == 0)
            continue;
          const TriangleCoefficient coefficient{d, i - static_cast<uint32_t>(vertex == 1),
                                                j - static_cast<uint32_t>(vertex == 2)};
          const auto at = numeric::triangle_coefficient_index(coefficient) * 4;
          const auto factor = Rational(budget, alpha[vertex]).divide(Rational(budget, d + 1));
          for (uint32_t axis = 0; axis < 3; ++axis)
            residual[axis] =
                residual[axis].add(factor.multiply(triangle.controls[at + axis].subtract(
                    triangle.controls[at + 3].multiply(xyz[vertex * 3 + axis]))));
        }
        const auto square = norm_square(residual, budget);
        if (square.compare(maximum_square) > 0)
          maximum_square = square;
      }
    upper = sqrt_upper(maximum_square).divide(wmin);
    certified =
        maximum_square.compare(allowance.multiply(allowance).multiply(wmin.multiply(wmin))) <= 0;
  } else if (root.family == GeometryFamily::plane) {
    auto largest_square = Rational(budget);
    for (uint32_t vertex = 0; vertex < 3; ++vertex) {
      std::array<Rational, 3> residual{Rational(budget), Rational(budget), Rational(budget)};
      for (uint32_t axis = 0; axis < 3; ++axis)
        residual[axis] =
            Rational::from_double(budget, root.frame[axis])
                .add(uv[vertex * 2].multiply(Rational::from_double(budget, root.frame[3 + axis])))
                .add(uv[vertex * 2 + 1].multiply(
                    Rational::from_double(budget, root.frame[6 + axis])))
                .subtract(xyz[vertex * 3 + axis]);
      const auto square = norm_square(residual, budget);
      if (square.compare(largest_square) > 0)
        largest_square = square;
    }
    upper = sqrt_upper(largest_square);
    certified = largest_square.compare(allowance.multiply(allowance)) <= 0;
  } else {
    std::array<Rational, 4> box{uv[0], uv[0], uv[1], uv[1]};
    for (uint32_t vertex = 1; vertex < 3; ++vertex)
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto &coordinate = uv[vertex * 2 + axis];
        if (coordinate.compare(box[axis * 2]) < 0)
          box[axis * 2] = coordinate;
        if (coordinate.compare(box[axis * 2 + 1]) > 0)
          box[axis * 2 + 1] = coordinate;
      }
    GeometryContext context(budget, {192, 512});
    const auto &carrier = cache.source().view().carriers[root.source_slot];
    const auto jet = enclose_analytic_surface_jet(context, carrier, box, 192);
    const auto muu = enclosure_norm(jet.duu, budget), muv = enclosure_norm(jet.duv, budget),
               mvv = enclosure_norm(jet.dvv, budget);
    auto departure = Rational(budget);
    for (uint32_t vertex = 0; vertex < 3; ++vertex) {
      const auto point =
          evaluate_analytic_surface(context, carrier, uv[vertex * 2], uv[vertex * 2 + 1]);
      const std::array residual{
          geometry_detail::difference(point.coordinates[0], ExactEnclosure(xyz[vertex * 3])),
          geometry_detail::difference(point.coordinates[1], ExactEnclosure(xyz[vertex * 3 + 1])),
          geometry_detail::difference(point.coordinates[2], ExactEnclosure(xyz[vertex * 3 + 2]))};
      const auto value = enclosure_norm(residual, budget);
      if (value.compare(departure) > 0)
        departure = value;
    }
    const auto hu = box[1].subtract(box[0]), hv = box[3].subtract(box[2]);
    upper = muu.multiply(hu.multiply(hu))
                .add(muv.multiply(hu).multiply(hv).multiply(Rational(budget, 2)))
                .add(mvv.multiply(hv.multiply(hv)))
                .divide(Rational(budget, 8))
                .add(departure);
    certified = upper.compare(allowance) <= 0;
  }
  SourceTriangleBounds bounds;
  bounds.status = NumericStatus::ready;
  bounds.position_upper =
      (certified && upper.compare(allowance) > 0 ? allowance : upper).bounds().upper;
  return {bounds, certified};
}
} // namespace
TrimPreciseResult
compare_original_triangle_precise(const SourceCache &cache, OriginalRootSlot original,
                                  std::span<const Rational, 6> uv, std::span<const Rational, 9> xyz,
                                  const Rational &allowance, bgfd::Budget &budget) {
  require(original.value < cache.original_roots().size() && &allowance.budget() == &budget &&
              allowance.sign() >= 0,
          "invalid exact original triangle allowance/root");
  for (const auto &value : uv)
    require(&value.budget() == &budget, "exact original triangle UV owner mismatch");
  for (const auto &value : xyz)
    require(&value.budget() == &budget, "exact original triangle L owner mismatch");
  require(orientation(uv).sign() > 0, "exact original triangle requires positive roles");
  return precise_source(cache, original, uv, xyz, allowance, budget);
}
TrimPreciseResult compare_trim_piece_precise(JobContext &context, const TrimFacetWork &work,
                                             uint32_t piece_index, TrimBoundMethod method) {
  require(piece_index < work.pieces().size(), "foreign precise trim piece");
  auto &budget = context.budget;
  const auto &piece = work.pieces()[piece_index];
  const auto uv = piece_uv(budget, work, piece);
  const auto xyz = piece_xyz_exact(budget, work, piece);
  const auto allowance = work_allowance(budget, work);
  if (method == TrimBoundMethod::original_source)
    return precise_source(work.data().surface.source(), piece.original, uv, xyz, allowance, budget);
  require(method == TrimBoundMethod::cached_bilinear, "unknown trimmed bound method");
  const std::array indices{piece_index};
  const auto batch = pack_base_pieces(context, work, indices);
  const auto box = leaf_box(budget, work, piece);
  std::array<Rational, 6> normalized{Rational(budget), Rational(budget), Rational(budget),
                                     Rational(budget), Rational(budget), Rational(budget)};
  for (uint32_t vertex = 0; vertex < 3; ++vertex)
    for (uint32_t axis = 0; axis < 2; ++axis)
      normalized[vertex * 2 + axis] = uv[vertex * 2 + axis]
                                          .subtract(box[axis * 2])
                                          .divide(box[axis * 2 + 1].subtract(box[axis * 2]));
  const auto result =
      bound_actual_facet_precise(batch.inputs[0], normalized, xyz, allowance, budget);
  return {{result.bounds.status, result.bounds.total_upper}, result.certified};
}
TrimPreciseResult compare_trim_plane_precise(JobContext &context, const TrimFacetWork &work,
                                             uint32_t facet_index) {
  require(facet_index < work.facets().size() && work.facets()[facet_index].original_plane,
          "foreign precise original plane facet");
  auto &budget = context.budget;
  const auto &parent = work.facets()[facet_index];
  const auto uv = facet_uv(budget, *work.data().construction, parent.construction_facet);
  const auto actual = facet_xyz(*work.data().construction, parent.construction_facet);
  std::array<Rational, 9> xyz{Rational(budget), Rational(budget), Rational(budget),
                              Rational(budget), Rational(budget), Rational(budget),
                              Rational(budget), Rational(budget), Rational(budget)};
  for (uint32_t i = 0; i < 9; ++i)
    xyz[i] = Rational::from_double(budget, actual[i]);
  const auto allowance = work_allowance(budget, work);
  return precise_source(work.data().surface.source(), parent.original, uv, xyz, allowance, budget);
}
namespace {
struct SquaredDistance final {
  Rational lower, upper;
};
SquaredDistance point_distance(std::span<const ExactEnclosure, 3> point,
                               const std::array<double, 3> &actual, bgfd::Budget &budget) {
  auto lower = Rational(budget), upper = Rational(budget);
  for (uint32_t axis = 0; axis < 3; ++axis) {
    const auto q = Rational::from_double(budget, actual[axis]);
    const auto lo = point[axis].lower.subtract(q), hi = point[axis].upper.subtract(q);
    const auto alo = lo.sign() < 0 ? lo.negative() : lo;
    const auto ahi = hi.sign() < 0 ? hi.negative() : hi;
    const auto minimum = lo.sign() <= 0 && hi.sign() >= 0 ? Rational(budget)
                         : alo.compare(ahi) < 0           ? alo
                                                          : ahi;
    const auto maximum = alo.compare(ahi) > 0 ? alo : ahi;
    lower = lower.add(minimum.multiply(minimum));
    upper = upper.add(maximum.multiply(maximum));
  }
  return {lower, upper};
}
bool proved_point_floor(const SourceCache &cache, OriginalRootSlot original,
                        std::span<const Rational, 2> uv, const std::array<double, 3> &actual,
                        const Rational &allowance, bgfd::Budget &budget, uint32_t &precise_tasks,
                        uint32_t limit) {
  SampleInput input;
  input.original = original;
  for (uint32_t axis = 0; axis < 2; ++axis)
    input.uv[axis] = exact_interval(uv[axis]);
  const auto sample = evaluate_sample(cache, input);
  const auto threshold = allowance.multiply(allowance);
  if (sample.status == NumericStatus::ready) {
    const auto coordinate = [&](uint32_t axis) {
      return ExactEnclosure(Rational::from_double(budget, sample.original_xyz[axis].lower),
                            Rational::from_double(budget, sample.original_xyz[axis].upper));
    };
    const std::array point{coordinate(0), coordinate(1), coordinate(2)};
    const auto distance = point_distance(point, actual, budget);
    if (distance.upper.compare(threshold) <= 0)
      return false;
    if (distance.lower.compare(threshold) > 0)
      return true;
  }
  if (++precise_tasks > limit)
    throw Error(ErrorCode::resource_exhausted, "trim point uncertainty task limit");
  const auto &root = cache.original_roots()[original.value];
  if (root.family == GeometryFamily::rational_tensor) {
    const auto value = exact_original_surface_point(cache, original, uv, budget);
    const std::array point{ExactEnclosure(value[0]), ExactEnclosure(value[1]),
                           ExactEnclosure(value[2])};
    return point_distance(point, actual, budget).lower.compare(threshold) > 0;
  }
  if (root.family == GeometryFamily::plane) {
    const auto coordinate = [&](uint32_t axis) {
      return Rational::from_double(budget, root.frame[axis])
          .add(uv[0].multiply(Rational::from_double(budget, root.frame[3 + axis])))
          .add(uv[1].multiply(Rational::from_double(budget, root.frame[6 + axis])));
    };
    const std::array point{ExactEnclosure(coordinate(0)), ExactEnclosure(coordinate(1)),
                           ExactEnclosure(coordinate(2))};
    return point_distance(point, actual, budget).lower.compare(threshold) > 0;
  }
  for (const uint32_t bits : {128U, 192U, 256U, 384U}) {
    GeometryContext geometry(budget, {bits, bits + 256});
    const auto point = evaluate_analytic_surface(
        geometry, cache.source().view().carriers[root.source_slot], uv[0], uv[1]);
    const auto distance = point_distance(point.coordinates, actual, budget);
    if (distance.upper.compare(threshold) <= 0)
      return false;
    if (distance.lower.compare(threshold) > 0)
      return true;
  }
  throw Error(ErrorCode::unresolved,
              "actual point departure threshold unresolved at precision cap");
}
ArithmeticStage next_point_stage(ArithmeticStage current) {
  if (current == ArithmeticStage::rn64)
    return ArithmeticStage::p128;
  if (current == ArithmeticStage::p128)
    return ArithmeticStage::p192;
  if (current == ArithmeticStage::p192)
    return ArithmeticStage::p256;
  if (current == ArithmeticStage::p256)
    return ArithmeticStage::precise;
  throw Error(ErrorCode::unresolved,
              "actual binary64 point profile cannot meet positional allowance");
}
} // namespace
TrimPointFloors trim_point_floor_requests(JobContext &context, const TrimFacetWork &work,
                                          std::span<const uint32_t> failed_facets) {
  auto &budget = context.budget;
  const auto &data = work.data();
  const auto &cache = data.surface.source();
  const auto selection = data.surface.selection();
  TrimPointFloors result(work, data.surface.resource().get());
  const auto allowance = work_allowance(budget, work);
  const auto &mesh = *data.construction;
  const auto carrier = mesh.source.view().carrier_index.find(
      mesh.source.view().faces[mesh.key.face.value].carrier_id);
  struct Less final {
    bgfd::Budget *budget;
    bool operator()(uint64_t a, uint64_t b) const {
      budget->charge(1);
      return a < b;
    }
  };
  std::pmr::set<uint64_t, Less> seen(Less{&budget}, &budget);
  uint32_t precise_tasks = 0;
  const auto probe = [&](TrimFloorKind kind, uint32_t facet, uint32_t authority,
                         OriginalRootSlot original, const std::array<ExactRef, 2> &parameter,
                         const ExactTable &table, const std::array<double, 3> &actual,
                         ArithmeticStage current) {
    require(original.value < cache.original_roots().size() &&
                cache.original_roots()[original.value].source_slot == carrier.value,
            "foreign actual point-floor source root");
    const auto key = (static_cast<uint64_t>(kind) << 32) | authority;
    if (!seen.insert(key).second)
      return;
    const std::array uv{import_exact(budget, table.view(parameter[0])),
                        import_exact(budget, table.view(parameter[1]))};
    if (proved_point_floor(cache, original, uv, actual, allowance, budget, precise_tasks,
                           data.limits.precise_tasks))
      result.requests.push_back(
          {kind, facet, authority, current,
           kind == TrimFloorKind::source_boundary ? current : next_point_stage(current)});
  };
  for (const auto facet_index : failed_facets) {
    require(facet_index < work.facets().size(), "foreign point-floor facet");
    const auto &parent = work.facets()[facet_index];
    const auto &facet = mesh.facets[parent.construction_facet];
    for (const auto site_index : facet.sites) {
      const auto &site = mesh.sites[site_index];
      if (const auto *surface = std::get_if<SurfaceSiteSource>(&site.source)) {
        if (numeric::finite(surface->departure_upper) && surface->departure_upper >= 0. &&
            Rational::from_double(budget, surface->departure_upper).compare(allowance) <= 0)
          continue;
        probe(TrimFloorKind::surface_site, facet_index, site_index, surface->original, site.uv,
              mesh.values, site.actual_xyz, surface->stage);
      } else {
        const auto &boundary = std::get<BoundarySiteSource>(site.source);
        const std::array uv{import_exact(budget, mesh.values.view(site.uv[0])),
                            import_exact(budget, mesh.values.view(site.uv[1]))};
        const auto original = find_original_carrier_root(cache, carrier, uv);
        // Occurrence UV matters: the same physical node can have another chart
        // occurrence. Deduplicate by construction site, retain physical row below.
        const auto key = (static_cast<uint64_t>(TrimFloorKind::source_boundary) << 32) | site_index;
        if (seen.insert(key).second &&
            proved_point_floor(cache, original, uv, site.actual_xyz, allowance, budget,
                               precise_tasks, data.limits.precise_tasks))
          result.requests.push_back({TrimFloorKind::source_boundary, facet_index,
                                     boundary.boundary_node, ArithmeticStage::rn64,
                                     ArithmeticStage::rn64});
      }
    }
    for (uint64_t at = parent.pieces.first; at < parent.pieces.first + parent.pieces.count; ++at) {
      const auto &leaf = selection->leaves[work.pieces()[at].leaf];
      for (const auto corner : leaf.corners) {
        const auto &point = selection->points[corner.value];
        if (numeric::finite(point.departure_upper) && point.departure_upper >= 0. &&
            Rational::from_double(budget, point.departure_upper).compare(allowance) <= 0)
          continue;
        probe(TrimFloorKind::bilinear_corner, facet_index, corner.value, point.key.original,
              point.key.parameter, selection->values, point.actual_xyz, point.key.stage);
      }
    }
  }
  return result;
}
} // namespace bgmesh::adaptive
