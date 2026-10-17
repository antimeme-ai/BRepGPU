#include "adaptive_regions.hpp"
#include "adaptive_curve_geometry.hpp"
#include "adaptive_region_cdt.hpp"
#include "adaptive_traces.hpp"
#include "exact_metric.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <numeric>
namespace bgmesh::adaptive {
namespace {
constexpr uint32_t none = UINT32_MAX;
void require(bool yes, const char *message) {
  if (!yes)
    throw Error(ErrorCode::invalid_source, message);
}
uint32_t word(size_t value) {
  if (value >= none)
    throw Error(ErrorCode::resource_exhausted, "trimmed construction index overflow");
  return static_cast<uint32_t>(value);
}
using UV = std::array<Rational, 2>;
bool equal_uv(const UV &a, const UV &b) {
  return a[0].compare(b[0]) == 0 && a[1].compare(b[1]) == 0;
}
UV read_uv(bgfd::Budget &b, const ExactTable &values, const std::array<ExactRef, 2> &refs) {
  return {import_exact(b, values.view(refs[0])), import_exact(b, values.view(refs[1]))};
}
Rational cross(const UV &a, const UV &b, const UV &c) {
  return b[0]
      .subtract(a[0])
      .multiply(c[1].subtract(a[1]))
      .subtract(b[1].subtract(a[1]).multiply(c[0].subtract(a[0])));
}
std::span<const UseCurveTrace> face_uses(const SharedCurveSnapshot &curves, Slot<FaceTag> face) {
  const auto first = std::lower_bound(curves->uses.begin(), curves->uses.end(), face,
                                      [](const auto &row, auto key) { return row.face < key; });
  const auto end = std::upper_bound(first, curves->uses.end(), face,
                                    [](auto key, const auto &row) { return key < row.face; });
  return {first, end};
}
void validate_domain_request(const DomainRequest &request) {
  require(request.curves && request.curves->boundary, "missing shared source boundary");
  const auto &curves = *request.curves;
  require(request.table_key != curves.values.key() &&
              request.table_key != curves.boundary->parameters.key(),
          "face domain exact table aliases a retained source pool");
  require(curves.query_generation == request.key.query &&
              curves.definition == request.key.definition,
          "stale trimmed face source query");
  const auto &s = curves.source.view();
  require(request.key.face.value < s.faces.size() &&
              request.key.definition.value < s.definitions.size() &&
              s.faces[request.key.face.value].definition_id ==
                  s.definitions[request.key.definition.value].id,
          "foreign trimmed face/definition");
}
void require_domain_key(const FaceDomainLease &domain, const FaceConstructionKey &key) {
  require(domain && domain->key.source_generation == key.source_generation &&
              domain->key.query == key.query &&
              domain->key.boundary_generation == key.boundary_generation &&
              domain->key.chart_revision == key.chart_revision &&
              domain->key.profile_revision == key.profile_revision &&
              domain->key.assignment_epoch == key.assignment_epoch &&
              domain->key.face == key.face && domain->key.definition == key.definition,
          "stale source face domain lease");
}
std::pmr::vector<BoundaryKey> coarse_requests(const SharedCurveSnapshot &curves,
                                              std::span<const UseCurveTrace> uses,
                                              bgfd::Budget &budget) {
  std::pmr::vector<BoundaryKey> wanted(&budget);
  for (const auto &use : uses)
    for (uint64_t j = 0; j < use.intervals.count; ++j) {
      budget.charge(1);
      const auto &interval = curves->intervals[use.intervals.first + j];
      if (import_exact(budget, curves->values.view(interval.source_to_selected_trace_upper))
              .sign() > 0)
        wanted.push_back(interval.boundary_key);
    }
  std::sort(wanted.begin(), wanted.end());
  wanted.erase(std::unique(wanted.begin(), wanted.end()), wanted.end());
  return wanted;
}
WholeUseLineImage image_record(const DomainRequest &request, const UseCurveTrace &trace,
                               ExactBuilder &values, bgfd::Budget &budget) {
  const auto &snapshot = *request.curves;
  const auto &source = snapshot.source.view();
  const auto &use = source.coedges[trace.use.value];
  if (!snapshot.curve_source || &snapshot.curve_source->source().view() != &source)
    throw Error(ErrorCode::unsupported, "authoritative face identity needs original curve cache");
  if (use.periodic_shift[0] || use.periodic_shift[1])
    throw Error(ErrorCode::unsupported, "authoritative symbolic lift identity is unproved");
  const auto chosen =
      std::span<const UseCurveSite>(snapshot.sites).subspan(trace.sites.first, trace.sites.count);
  auto image = match_authoritative_line_image(snapshot.source, *snapshot.curve_source, trace.use,
                                              snapshot.values, chosen, budget);
  if (!image)
    throw Error(ErrorCode::unsupported,
                "original authoritative whole-use exact line image unproved");
  const auto curve = source.curve_index.find(use.curve_id);
  const int source_direction =
      import_exact(budget, snapshot.values.view(chosen.back().mapped_s))
          .compare(import_exact(budget, snapshot.values.view(chosen.front().mapped_s)));
  WholeUseLineImage row;
  row.use = trace.use;
  row.raw_curve = curve;
  row.chosen_sites = trace.sites;
  row.raw_traversal = source_direction;
  for (uint32_t j = 0; j < 4; ++j)
    row.endpoints[j] = values.append(image->endpoints[j]);
  for (uint32_t j = 0; j < 2; ++j) {
    row.covector[j] = values.append(image->covector[j]);
    row.scalar_endpoints[j] = values.append(image->scalars[j]);
  }
  return row;
}
struct ImportedDomain final {
  std::pmr::vector<UV> points;
  std::pmr::vector<DomainPointRef> refs;
  std::pmr::vector<DomainSegment> segments;
  DomainIndexStats stats;
  ImportedDomain(const FaceDomainLease &domain, bgfd::Budget &budget)
      : points(&budget), refs(&budget), segments(&budget) {
    for (const auto &site : domain->sites)
      points.push_back(read_uv(budget, domain->values, site.uv));
    for (const auto &point : points)
      refs.push_back({&point[0], &point[1]});
    for (uint32_t l = 0; l < domain->loops.size(); ++l) {
      const auto &loop = domain->loops[l];
      for (uint64_t j = 0; j < loop.edges.count; ++j) {
        const auto &edge = domain->edges[loop.edges.first + j];
        segments.push_back({SiteSlot{edge.sites[0]},
                            SiteSlot{edge.sites[1]},
                            {l, j},
                            domain_segment_box(refs[edge.sites[0]], refs[edge.sites[1]])});
      }
    }
  }
};
} // namespace
DomainPreparation prepare_face_domain(const DomainRequest &request, JobContext &context) {
  validate_domain_request(request);
  auto &budget = context.budget;
  const auto resource = request.curves->resource;
  DomainPreparation result(resource.get());
  const auto traces = face_uses(request.curves, request.key.face);
  require(!traces.empty(), "trimmed face has no source use inventory");
  const auto &source = request.curves->source.view();
  const auto &face = source.faces[request.key.face.value];
  for (const auto &trace : traces)
    require(trace.face == request.key.face &&
                trace.boundary_generation == request.key.boundary_generation,
            "stale face boundary generation");
  const auto site_first = traces.front().sites.first;
  const auto site_end = traces.back().sites.first + traces.back().sites.count;
  require(site_end <= request.curves->sites.size(), "face site range invalid");
  std::pmr::vector<uint32_t> mapped(site_end - site_first, none, &budget);
  std::pmr::vector<std::pair<uint32_t, uint32_t>> use_order(&budget);
  for (uint32_t i = 0; i < traces.size(); ++i)
    use_order.push_back({traces[i].use.value, i});
  std::sort(use_order.begin(), use_order.end());
  for (size_t i = 1; i < use_order.size(); ++i)
    require(use_order[i - 1].first != use_order[i].first, "repeated source use");
  ExactBuilder values(resource, request.table_key);
  std::pmr::vector<UV> points(&budget);
  std::pmr::vector<DomainBoundarySite> sites(resource.get());
  std::pmr::vector<SourceOccurrenceBinding> bindings(resource.get());
  std::pmr::vector<DomainBoundaryEdge> edges(resource.get());
  std::pmr::vector<DomainBoundaryLoop> loops(resource.get());
  std::pmr::vector<FaceDomainUse> policies(resource.get());
  std::pmr::vector<uint8_t> consumed(traces.size(), uint8_t{0}, &budget);
  bool coarse = false;
  for (uint64_t li = 0; li < face.loops.count; ++li) {
    budget.charge(1);
    const auto loop_slot = source.loop_index.find(source.loop_ids[face.loops.first + li]);
    const auto &original = source.loops[loop_slot.value];
    std::pmr::vector<uint32_t> joints(original.coedges.count, none, &budget);
    const auto start = edges.size();
    for (uint64_t ui = 0; ui < original.coedges.count; ++ui) {
      const auto use_slot =
          source.coedge_index.find(source.coedge_ids[original.coedges.first + ui]);
      auto hit = std::lower_bound(use_order.begin(), use_order.end(), use_slot.value,
                                  [](const auto &row, uint32_t id) { return row.first < id; });
      require(hit != use_order.end() && hit->first == use_slot.value, "missing original coedge");
      const auto trace_index = hit->second;
      require(!consumed[trace_index], "coedge appears in multiple loops");
      consumed[trace_index] = 1;
      const auto &trace = traces[trace_index];
      require(trace.sites.count >= 2 && trace.intervals.count + 1 == trace.sites.count &&
                  trace.sites.first >= site_first &&
                  trace.sites.first + trace.sites.count <= site_end &&
                  trace.intervals.first <= request.curves->intervals.size() &&
                  trace.intervals.count <= request.curves->intervals.size() - trace.intervals.first,
              "invalid original use site/interval range");
      FaceDomainUse policy;
      policy.use = use_slot;
      if (!source.coedges[use_slot.value].reconstructed) {
        policy.policy = DomainPolicy::authoritative_exact_image;
        policy.image = image_record(request, trace, values, budget);
      }
      policies.push_back(policy);
      for (uint64_t j = 0; j < trace.sites.count; ++j) {
        budget.charge(1);
        const auto global = word(trace.sites.first + j);
        const auto &raw = request.curves->sites[global];
        require(raw.loop == loop_slot && raw.boundary_node < request.curves->boundary->nodes.size(),
                "foreign use loop/node");
        auto uv = read_uv(budget, request.curves->values, raw.chart_xy);
        const auto original_uv = read_uv(budget, request.curves->values, raw.chosen_original_uv);
        require(equal_uv(uv, original_uv), "ordinary UV constructor received alternate chart");
        uint32_t site = none;
        if (j == 0 || j + 1 == trace.sites.count) {
          const auto joint = j == 0 ? ui : (ui + 1) % original.coedges.count;
          require(raw.junction_occurrence == joint, "original loop joint occurrence differs");
          site = joints[joint];
          if (site != none) {
            require(equal_uv(points[site], uv), "settled adjacent chart joint differs");
            const auto &a = request.curves->boundary->nodes[sites[site].boundary_node];
            const auto &b = request.curves->boundary->nodes[raw.boundary_node];
            require(a.key == b.key, "loop joint physical identities differ");
            for (uint32_t axis = 0; axis < 3; ++axis)
              require(std::bit_cast<uint64_t>(a.xyz[axis]) == std::bit_cast<uint64_t>(b.xyz[axis]),
                      "loop joint actual XYZ differs");
          } else {
            site = word(points.size());
            joints[joint] = site;
          }
        } else
          require(raw.junction_occurrence == none, "interior use point claims joint identity");
        if (site == none)
          site = word(points.size());
        if (site == points.size()) {
          sites.push_back(
              {global, raw.boundary_node, {values.append(uv[0]), values.append(uv[1])}});
          points.push_back(std::move(uv));
        }
        require(mapped[global - site_first] == none, "repeated shared use-site row");
        mapped[global - site_first] = site;
        bindings.push_back({global, site});
        if (j) {
          const auto &interval = request.curves->intervals[trace.intervals.first + j - 1];
          require(interval.sites == std::array<uint32_t, 2>{global - 1, global},
                  "source interval site order differs");
          edges.push_back({{mapped[global - 1 - site_first], site}, interval.boundary_key});
        }
      }
    }
    Rational area(budget);
    for (size_t j = start; j < edges.size(); ++j) {
      const auto &e = edges[j];
      area = area.add(points[e.sites[0]][0]
                          .multiply(points[e.sites[1]][1])
                          .subtract(points[e.sites[0]][1].multiply(points[e.sites[1]][0])));
    }
    if (edges.size() - start < 3 || area.sign() == 0)
      coarse = true;
    const bool positive = original.outer;
    const int adapter = area.sign() == 0 ? 1 : (positive ? 1 : -1) * area.sign();
    if (adapter < 0) {
      std::reverse(edges.begin() + static_cast<std::ptrdiff_t>(start), edges.end());
      for (size_t j = start; j < edges.size(); ++j)
        std::swap(edges[j].sites[0], edges[j].sites[1]);
    }
    loops.push_back({loop_slot, positive, adapter, {start, edges.size() - start}});
  }
  require(std::ranges::all_of(consumed, [](uint8_t value) { return value != 0; }),
          "extra face coedge inventory");
  std::pmr::vector<DomainPointRef> refs(&budget);
  for (const auto &point : points)
    refs.push_back({&point[0], &point[1]});
  std::pmr::vector<DomainSegment> segments(&budget);
  for (uint32_t l = 0; l < loops.size(); ++l) {
    const auto &loop = loops[l];
    for (uint64_t j = 0; j < loop.edges.count; ++j) {
      const auto &e = edges[loop.edges.first + j];
      if (equal_uv(points[e.sites[0]], points[e.sites[1]]))
        coarse = true;
      segments.push_back({SiteSlot{e.sites[0]},
                          SiteSlot{e.sites[1]},
                          {l, j},
                          domain_segment_box(refs[e.sites[0]], refs[e.sites[1]])});
    }
  }
  DomainIndexStats stats;
  DomainSegmentIndex index(budget, refs, stats, segments);
  if (index.crossing())
    coarse = true;
  if (!coarse)
    for (const auto &loop : loops) {
      auto own =
          std::span<const DomainSegment>(segments).subspan(loop.edges.first, loop.edges.count);
      if (index.winding_except(own.front().a, own) != (loop.positive ? 0 : 1))
        coarse = true;
    }
  if (coarse) {
    auto wanted = coarse_requests(request.curves, traces, budget);
    if (wanted.empty())
      throw Error(ErrorCode::unresolved,
                  "selected face graph lacks faithful source refinement evidence");
    result.requests.assign(wanted.begin(), wanted.end());
    return result;
  }
  auto domain = std::allocate_shared<FaceDomainData>(RunAllocator<FaceDomainData>(resource),
                                                     resource, request.curves->source,
                                                     request.curves, request.key, values.seal());
  domain->uses = std::move(policies);
  domain->sites = std::move(sites);
  domain->source_bindings = std::move(bindings);
  domain->edges = std::move(edges);
  domain->loops = std::move(loops);
  result.domain = std::move(domain);
  return result;
}
struct FaceDomainClassifier::Impl final {
  FaceDomainLease domain;
  bgfd::Budget budget;
  ImportedDomain imported;
  DomainSegmentIndex index;
  Impl(const std::shared_ptr<RunResource> &resource, FaceDomainLease policy,
       DomainClassifierLimits limits, std::stop_token stop)
      : domain(std::move(policy)),
        budget({limits.bytes, limits.work}, resource.get(), std::move(stop)),
        imported(domain, budget), index(budget, imported.refs, imported.stats, imported.segments) {}
};
FaceDomainClassifier::FaceDomainClassifier(std::shared_ptr<RunResource> resource,
                                           FaceDomainLease domain, DomainClassifierLimits limits,
                                           const std::stop_token &stop)
    : resource_(std::move(resource)), impl_(nullptr) {
  require(resource_ && domain && domain->resource == resource_ && limits.bytes && limits.work,
          "invalid domain classifier owner/limits");
  void *memory = resource_->allocate(sizeof(Impl), alignof(Impl));
  try {
    impl_ = new (memory) Impl(resource_, std::move(domain), limits, stop);
  } catch (...) {
    resource_->deallocate(memory, sizeof(Impl), alignof(Impl));
    throw;
  }
}
FaceDomainClassifier::~FaceDomainClassifier() {
  if (impl_) {
    impl_->~Impl();
    resource_->deallocate(impl_, sizeof(Impl), alignof(Impl));
  }
}
DomainClassifierStats FaceDomainClassifier::stats() const noexcept {
  return {impl_->budget.work(), impl_->budget.peak()};
}
CandidateClassification FaceDomainClassifier::classify(const CandidateBoxes &candidates,
                                                       uint64_t work_allowance) {
  auto work_scope = impl_->budget.scoped_work(work_allowance);
  const auto &domain = impl_->domain;
  require(candidates && &domain->source.view() == &candidates->source.source().view(),
          "foreign candidate source domain");
  const auto &face = candidates->key.face;
  require(domain->key.definition == face.definition && domain->key.face == face.face &&
              domain->key.source_generation == face.source_generation &&
              domain->key.assignment_epoch == face.assignment_epoch &&
              domain->key.boundary_generation == face.boundary_generation &&
              domain->key.chart_revision == face.chart_revision &&
              domain->key.query == candidates->key.selection.query &&
              domain->key.profile_revision == candidates->key.selection.profile_revision,
          "stale candidate face domain");
  auto &budget = impl_->budget;
  budget.charge(0);
  CandidateClassification result(candidates);
  result.domain = domain;
  for (const auto &candidate : candidates->boxes) {
    budget.charge(1);
    std::array<Rational, 4> box{import_exact(budget, candidates->values.view(candidate.box[0])),
                                import_exact(budget, candidates->values.view(candidate.box[1])),
                                import_exact(budget, candidates->values.view(candidate.box[2])),
                                import_exact(budget, candidates->values.view(candidate.box[3]))};
    if (impl_->index.touches_rectangle(box))
      result.labels.push_back(SurfaceClass::band);
    else {
      UV middle{box[0].add(box[1]).divide(Rational(budget, 2)),
                box[2].add(box[3]).divide(Rational(budget, 2))};
      const auto winding = impl_->index.winding({&middle[0], &middle[1]});
      require(winding == 0 || winding == 1, "chosen face winding multiplicity differs");
      result.labels.push_back(winding ? SurfaceClass::interior : SurfaceClass::outside);
    }
  }
  return result;
}
std::pmr::vector<SurfaceClass>
FaceDomainClassifier::classify_points(const ExactTable &values,
                                      std::span<const std::array<ExactRef, 2>> points,
                                      uint64_t work_allowance) {
  auto work_scope = impl_->budget.scoped_work(work_allowance);
  require(values.resource() == resource_, "point classification has foreign owner");
  std::pmr::vector<SurfaceClass> result(resource_.get());
  auto &budget = impl_->budget;
  for (const auto &refs : points) {
    budget.charge(1);
    const auto point = read_uv(budget, values, refs);
    const DomainPointRef query{&point[0], &point[1]};
    if (impl_->index.boundary_contact(query))
      result.push_back(SurfaceClass::band);
    else {
      const auto winding = impl_->index.winding(query);
      require(winding == 0 || winding == 1, "point classification has material multiplicity");
      result.push_back(winding ? SurfaceClass::interior : SurfaceClass::outside);
    }
  }
  return result;
}
namespace {
uint32_t path_step(const LeafKey &key, std::span<const uint64_t> words, uint32_t depth) {
  require(depth < key.depth && key.path.first + depth / 32 < words.size() &&
              depth / 32 < key.path.count,
          "leaf path word is absent");
  return static_cast<uint32_t>((words[key.path.first + depth / 32] >> (2 * (depth % 32))) & 3);
}
bool path_less(const LeafKey &a, std::span<const uint64_t> aw, const LeafKey &b,
               std::span<const uint64_t> bw) {
  if (a.root.value != b.root.value)
    return a.root.value < b.root.value;
  const auto length = std::min(a.depth, b.depth);
  for (uint32_t i = 0; i < length; ++i) {
    const auto x = path_step(a, aw, i), y = path_step(b, bw, i);
    if (x != y)
      return x < y;
  }
  return a.depth < b.depth;
}
bool path_prefix(const LeafKey &a, std::span<const uint64_t> aw, const LeafKey &b,
                 std::span<const uint64_t> bw) {
  if (a.root.value != b.root.value || a.depth > b.depth)
    return false;
  for (uint32_t i = 0; i < a.depth; ++i)
    if (path_step(a, aw, i) != path_step(b, bw, i))
      return false;
  return true;
}
std::optional<SurfaceClass> footprint_role(const PacketFootprint &footprint, const LeafKey &key,
                                           std::span<const uint64_t> words, bgfd::Budget &budget) {
  const auto &selection = *footprint->packets->selection;
  auto hit = std::upper_bound(footprint->rows.begin(), footprint->rows.end(), key,
                              [&](const auto &a, const auto &b) {
                                budget.charge(1);
                                return path_less(a, words, b.prefix, selection.path_words);
                              });
  if (hit == footprint->rows.begin())
    return std::nullopt;
  --hit;
  if (!path_prefix(hit->prefix, selection.path_words, key, words))
    return std::nullopt;
  return hit->role;
}
} // namespace
PacketFootprint freeze_packet_footprint(const PacketMesh &mesh, const SurfaceBoundView &view,
                                        bgfd::Budget &budget) {
  require(mesh && mesh->selection, "missing selected packet footprint");
  const auto &selection = *mesh->selection;
  require(view && view.selection() == mesh->selection,
          "packet footprint uses foreign face selection");
  auto result = std::allocate_shared<PacketFootprintData>(
      RunAllocator<PacketFootprintData>(mesh->resource), mesh, view.key().face);
  std::pmr::vector<uint8_t> enabled(selection.leaves.size(), uint8_t{0}, &budget);
  for (const auto &packet : mesh->packets) {
    require(packet.leaf < enabled.size(), "footprint packet leaf missing");
    enabled[packet.leaf] = 1;
  }
  for (uint32_t i = 0; i < selection.leaves.size(); ++i) {
    budget.charge(1);
    result->rows.push_back(
        {selection.leaves[i].key, enabled[i] ? SurfaceClass::interior : SurfaceClass::band});
  }
  std::sort(result->rows.begin(), result->rows.end(), [&](const auto &a, const auto &b) {
    budget.charge(1);
    return path_less(a.prefix, selection.path_words, b.prefix, selection.path_words);
  });
  return result;
}
CandidateClassification apply_footprint(const PacketFootprint &footprint,
                                        CandidateClassification raw, bgfd::Budget &budget) {
  require(footprint && raw.candidates, "missing footprint classification lease");
  const auto &selection = *footprint->packets->selection;
  const auto &candidate = *raw.candidates;
  require(footprint->face == candidate.key.face &&
              &selection.source.view() == &candidate.source.view() &&
              selection.key.session == candidate.key.selection.session &&
              selection.key.query == candidate.key.selection.query &&
              selection.key.profile_revision == candidate.key.selection.profile_revision,
          "stale source packet footprint");
  require(raw.labels.size() == candidate.boxes.size(), "partial footprint classification horizon");
  for (uint32_t i = 0; i < raw.labels.size(); ++i) {
    budget.charge(1);
    const auto &key = candidate.boxes[i].key;
    const auto role = footprint_role(footprint, key, candidate.path_words, budget);
    if (!role) {
      require(raw.labels[i] == SurfaceClass::outside,
              "candidate path is not a frozen footprint descendant");
      continue;
    }
    if (raw.labels[i] == SurfaceClass::outside) {
      require(*role != SurfaceClass::interior,
              "frozen packet descendant left chosen source domain");
      continue;
    }
    if (*role == SurfaceClass::interior)
      require(raw.labels[i] == SurfaceClass::interior,
              "frozen packet descendant touches or leaves chosen domain");
    raw.labels[i] = *role;
  }
  return raw;
}
std::pmr::vector<uint32_t> packet_leaf_slots(const PacketFootprint &footprint,
                                             const SurfaceBoundView &view, bgfd::Budget &budget) {
  require(static_cast<bool>(view), "packet leaf routing has no surface view");
  const auto selection = view.selection();
  const auto classes = view.classes();
  require(classes.size() == selection->leaves.size(), "selected leaf class inventory differs");
  if (footprint) {
    const auto &original = *footprint->packets->selection;
    require(footprint->face == view.key().face &&
                &original.source.view() == &selection->source.view() &&
                original.key.session == selection->key.session &&
                original.key.query == selection->key.query &&
                original.key.profile_revision == selection->key.profile_revision,
            "packet leaf routing uses stale footprint");
  }
  std::pmr::vector<uint32_t> result(&budget);
  for (uint32_t i = 0; i < classes.size(); ++i) {
    budget.charge(1);
    require(classes[i] != SurfaceClass::outside,
            "selected numerical cover contains outside material");
    if (!footprint) {
      if (classes[i] == SurfaceClass::interior)
        result.push_back(i);
      continue;
    }
    const auto role =
        footprint_role(footprint, selection->leaves[i].key, selection->path_words, budget);
    if (!role)
      throw Error(ErrorCode::invalid_source, "selected leaf is not a frozen footprint descendant");
    if (*role == SurfaceClass::interior) {
      require(classes[i] == SurfaceClass::interior,
              "packet descendant touches chosen source graph");
      result.push_back(i);
    }
  }
  return result;
}
namespace {
struct BuildLoop final {
  std::pmr::vector<uint32_t> sites;
  std::pmr::vector<ProtectedRegionEdge> edges;
  std::optional<Slot<LoopTag>> source;
  int32_t adapter = 1;
  bool positive = false;
  uint32_t component = none;
  explicit BuildLoop(std::pmr::memory_resource *r) : sites(r), edges(r) {}
};
struct SideGeometry final {
  uint32_t axis;
  Rational constant, low, high;
  int direction;
};
bool side_less(const SideGeometry &a, const SideGeometry &b) {
  if (a.axis != b.axis)
    return a.axis < b.axis;
  for (const auto pair : {std::pair{&a.constant, &b.constant}, std::pair{&a.low, &b.low},
                          std::pair{&a.high, &b.high}}) {
    const int order = pair.first->compare(*pair.second);
    if (order)
      return order < 0;
  }
  return a.direction < b.direction;
}
bool same_side(const SideGeometry &a, const SideGeometry &b) {
  return !side_less(a, b) && !side_less(b, a);
}
bool uv_less(const UV &a, const UV &b) {
  const int x = a[0].compare(b[0]);
  return x ? x < 0 : a[1].compare(b[1]) < 0;
}
bool same_actual(const ConstructionSite &a, const ConstructionSite &b) {
  if (a.constructed_revision != b.constructed_revision || a.source.index() != b.source.index())
    return false;
  if (const auto *boundary = std::get_if<BoundarySiteSource>(&a.source)) {
    if (boundary->boundary_node != std::get<BoundarySiteSource>(b.source).boundary_node)
      return false;
  } else {
    const auto &left = std::get<SurfaceSiteSource>(a.source),
               &right = std::get<SurfaceSiteSource>(b.source);
    if (left.original.value != right.original.value || left.stage != right.stage ||
        left.anchor != right.anchor || left.birth_horizon != right.birth_horizon)
      return false;
  }
  for (uint32_t i = 0; i < 3; ++i)
    if (std::bit_cast<uint64_t>(a.actual_xyz[i]) != std::bit_cast<uint64_t>(b.actual_xyz[i]))
      return false;
  return true;
}
std::array<Rational, 4> face_metric(const RegionInput &input, bgfd::Budget &budget) {
  if (input.previous) {
    std::array<Rational, 4> metric{Rational(budget), Rational(budget), Rational(budget),
                                   Rational(budget)};
    for (uint32_t i = 0; i < 4; ++i)
      metric[i] =
          import_exact(budget, input.previous->values.view(input.previous->fixed_metric_factor[i]));
    return metric;
  }
  const auto selection = input.surface.selection();
  require(!selection->leaves.empty(), "face metric has no covered source leaf");
  std::pmr::vector<uint8_t> packet(selection->leaves.size(), uint8_t{0}, &budget);
  for (const auto &row : input.packets->packets) {
    require(row.leaf < packet.size(), "packet leaf outside source selection");
    packet[row.leaf] = 1;
  }
  uint32_t chosen = 0;
  for (uint32_t i = 0; i < packet.size(); ++i)
    if (!packet[i]) {
      chosen = i;
      break;
    }
  const auto &leaf = selection->leaves[chosen];
  std::array<Rational, 4> box{import_exact(budget, selection->values.view(leaf.box[0])),
                              import_exact(budget, selection->values.view(leaf.box[1])),
                              import_exact(budget, selection->values.view(leaf.box[2])),
                              import_exact(budget, selection->values.view(leaf.box[3]))};
  auto du = box[1].subtract(box[0]), dv = box[3].subtract(box[2]);
  require(du.sign() > 0 && dv.sign() > 0, "face metric leaf width is not positive");
  Rational g00(budget), g01(budget), g11(budget);
  for (uint32_t axis = 0; axis < 3; ++axis) {
    std::array<Rational, 4> q{Rational(budget), Rational(budget), Rational(budget),
                              Rational(budget)};
    for (uint32_t j = 0; j < 4; ++j) {
      require(leaf.corners[j].value < selection->points.size(), "face metric corner missing");
      q[j] =
          Rational::from_double(budget, selection->points[leaf.corners[j].value].actual_xyz[axis]);
    }
    const auto u =
        q[1].subtract(q[0]).add(q[3].subtract(q[2])).divide(du.multiply(Rational(budget, 2)));
    const auto v =
        q[2].subtract(q[0]).add(q[3].subtract(q[1])).divide(dv.multiply(Rational(budget, 2)));
    g00 = g00.add(u.multiply(u));
    g01 = g01.add(u.multiply(v));
    g11 = g11.add(v.multiply(v));
  }
  const auto determinant = g00.multiply(g11).subtract(g01.multiply(g01));
  if (g00.sign() <= 0 || determinant.sign() <= 0)
    throw Error(ErrorCode::unresolved, "cached face Gram metric is not positive definite");
  auto t00 = rounded_sqrt53(g00), t01 = rounded_dyadic53(g01.divide(t00)),
       t11 = rounded_sqrt53(determinant.divide(g00));
  require(t00.sign() > 0 && t11.sign() > 0, "face metric factor is singular");
  return {std::move(t00), std::move(t01), Rational(budget), std::move(t11)};
}
class RegionBuilder final {
  const RegionInput &input;
  bgfd::Budget &budget;
  std::shared_ptr<RunResource> resource;
  ExactBuilder exact;
  std::pmr::vector<UV> points;
  std::pmr::vector<ConstructionSite> sites;
  std::pmr::vector<SourceOccurrenceBinding> bindings;
  std::pmr::vector<BuildLoop> loops;
  std::pmr::vector<RegionFacet> facets;
  std::pmr::vector<FacetKey> changed_keys;
  std::pmr::vector<RegionComponent> components;
  std::pmr::vector<uint32_t> grid;
  std::pmr::vector<uint32_t> anchors;
  std::pmr::vector<SideGeometry> sides;
  std::array<Rational, 4> metric;
  uint32_t append(UV uv, ConstructionSite site) {
    const auto slot = word(points.size());
    for (uint32_t axis = 0; axis < 2; ++axis)
      site.uv[axis] = exact.append(uv[axis]);
    for (double value : site.actual_xyz)
      require(std::isfinite(value), "construction actual source point is nonfinite");
    points.push_back(std::move(uv));
    sites.push_back(std::move(site));
    return slot;
  }
  uint32_t grid_site(uint32_t slot) {
    const auto selection = input.surface.selection();
    require(slot < selection->points.size(), "packet point outside selection");
    if (grid[slot] != none)
      return grid[slot];
    const auto &point = selection->points[slot];
    auto uv = read_uv(budget, selection->values, point.chart_xy);
    require(equal_uv(uv, read_uv(budget, selection->values, point.original_uv)),
            "ordinary face packet has alternate chart coordinates");
    ConstructionSite site{
        {},
        point.actual_xyz,
        point.key.constructed_generation,
        SurfaceSiteSource{point.key.original, point.key.stage, false, 0, point.departure_upper}};
    return grid[slot] = append(std::move(uv), std::move(site));
  }
  void source_sites() {
    const auto &boundary = *input.domain->curves->boundary;
    for (const auto &row : input.domain->sites) {
      budget.charge(1);
      require(row.boundary_node < boundary.nodes.size() &&
                  row.boundary_node < boundary.constructed_revisions.size(),
              "source boundary node revision missing");
      append(read_uv(budget, input.domain->values, row.uv),
             ConstructionSite{{},
                              boundary.nodes[row.boundary_node].xyz,
                              boundary.constructed_revisions[row.boundary_node],
                              BoundarySiteSource{row.boundary_node}});
    }
    bindings.assign(input.domain->source_bindings.begin(), input.domain->source_bindings.end());
    for (const auto &row : input.domain->loops) {
      loops.emplace_back(&budget);
      auto &loop = loops.back();
      loop.source = row.source;
      loop.adapter = row.traversal_adapter;
      loop.positive = row.positive;
      for (uint64_t j = 0; j < row.edges.count; ++j) {
        const auto &edge = input.domain->edges[row.edges.first + j];
        require(edge.sites[0] < sites.size() && edge.sites[1] < sites.size(),
                "domain boundary site is missing");
        loop.sites.push_back(edge.sites[0]);
        loop.edges.push_back({edge.sites, edge.provenance});
      }
    }
  }
  void packet_facets() {
    const auto selection = input.surface.selection();
    std::pmr::vector<uint32_t> old_packets(&budget);
    std::pmr::vector<uint32_t> old_first_by_leaf(&budget);
    uint64_t next_owner = 0;
    if (input.previous) {
      const auto &old = *input.previous;
      old_first_by_leaf.assign(old.surface.selection()->leaves.size(), none);
      for (uint32_t i = 0; i < old.facets.size(); ++i) {
        budget.charge(1);
        const auto &facet = old.facets[i];
        if (facet.key.generator == FacetGenerator::packet) {
          require(facet.leaf < old_first_by_leaf.size(), "prior packet source leaf missing");
          if (old_first_by_leaf[facet.leaf] == none)
            old_first_by_leaf[facet.leaf] = i;
        }
      }
      for (uint32_t i = 0; i < old.packets->packets.size(); ++i)
        old_packets.push_back(i);
      std::sort(old_packets.begin(), old_packets.end(), [&](uint32_t a, uint32_t b) {
        budget.charge(1);
        const auto &left = old.surface.selection()->leaves[old.packets->packets[a].leaf].key;
        const auto &right = old.surface.selection()->leaves[old.packets->packets[b].leaf].key;
        return path_less(left, old.surface.selection()->path_words, right,
                         old.surface.selection()->path_words);
      });
      for (const auto &facet : old.facets)
        if (facet.key.generator == FacetGenerator::packet) {
          if (facet.key.owner == UINT64_MAX)
            throw Error(ErrorCode::resource_exhausted, "packet owner identity exhausted");
          next_owner = std::max(next_owner, facet.key.owner + 1);
        }
    }
    for (const auto &packet : input.packets->packets) {
      require(packet.leaf < selection->leaves.size(), "packet leaf outside source selection");
      const auto start = facets.size();
      for (uint64_t i = 0; i < packet.triangles.count; ++i) {
        budget.charge(1);
        const auto &triangle = input.packets->triangles[packet.triangles.first + i];
        require(triangle.leaf == packet.leaf, "packet triangle owns another leaf");
        std::array<uint32_t, 3> mapped{};
        for (uint32_t j = 0; j < 3; ++j)
          mapped[j] = grid_site(triangle.points[j].value);
        require(cross(points[mapped[0]], points[mapped[1]], points[mapped[2]]).sign() > 0,
                "packet triangle is not positive");
        facets.push_back({{FacetGenerator::packet, 0, 0, word(i)}, mapped, none, packet.leaf});
      }
      uint64_t owner = next_owner, revision = input.key.construction_revision;
      bool unchanged = false;
      if (input.previous) {
        const auto &old = *input.previous;
        const auto old_selection = old.surface.selection();
        const auto &leaf = selection->leaves[packet.leaf].key;
        const auto hit = std::lower_bound(
            old_packets.begin(), old_packets.end(), leaf, [&](uint32_t index, const auto &key) {
              budget.charge(1);
              return path_less(old_selection->leaves[old.packets->packets[index].leaf].key,
                               old_selection->path_words, key, selection->path_words);
            });
        if (hit != old_packets.end()) {
          const auto old_leaf = old.packets->packets[*hit].leaf;
          const auto &old_key = old_selection->leaves[old_leaf].key;
          if (path_prefix(old_key, old_selection->path_words, leaf, selection->path_words) &&
              old_key.depth == leaf.depth) {
            require(old_first_by_leaf[old_leaf] != none, "prior packet has no actual triangles");
            const auto old_first = old.facets.begin() + old_first_by_leaf[old_leaf];
            owner = old_first->key.owner;
            revision = old_first->key.revision;
            auto old_end = old_first;
            while (old_end != old.facets.end() &&
                   old_end->key.generator == FacetGenerator::packet && old_end->key.owner == owner)
              ++old_end;
            unchanged = static_cast<uint64_t>(old_end - old_first) == packet.triangles.count;
            for (uint64_t i = 0; i < packet.triangles.count && unchanged; ++i) {
              const auto &current = facets[start + i],
                         &prior = old_first[static_cast<std::ptrdiff_t>(i)];
              bool matched = false;
              for (uint32_t shift = 0; shift < 3 && !matched; ++shift) {
                bool same = true;
                for (uint32_t j = 0; j < 3; ++j) {
                  budget.charge(1);
                  const auto previous_index = prior.sites[(j + shift) % 3],
                             current_index = current.sites[j];
                  if (!same_actual(old.sites[previous_index], sites[current_index]) ||
                      !equal_uv(read_uv(budget, old.values, old.sites[previous_index].uv),
                                points[current_index]))
                    same = false;
                }
                matched = same;
              }
              if (!matched)
                unchanged = false;
            }
            if (!unchanged) {
              if (revision == UINT64_MAX)
                throw Error(ErrorCode::resource_exhausted, "packet revision exhausted");
              ++revision;
            }
          }
        }
      }
      if (owner == next_owner) {
        if (next_owner == UINT64_MAX)
          throw Error(ErrorCode::resource_exhausted, "packet owner identity exhausted");
        ++next_owner;
      }
      for (size_t i = start; i < facets.size(); ++i) {
        facets[i].key.owner = owner;
        facets[i].key.revision = revision;
        if (!unchanged)
          changed_keys.push_back(facets[i].key);
      }
    }
  }
  struct RimWord final {
    uint32_t a, b, side = none;
  };
  SideGeometry geometry(uint32_t a, uint32_t b) {
    const int x = points[a][0].compare(points[b][0]), y = points[a][1].compare(points[b][1]);
    require((x == 0) != (y == 0), "packet rim is not a nonzero axis-aligned segment");
    const uint32_t axis = x ? 0 : 1;
    const auto &p = points[a][axis], &q = points[b][axis];
    return {axis, points[a][1 - axis], p.compare(q) < 0 ? p : q, p.compare(q) < 0 ? q : p,
            p.compare(q) < 0 ? 1 : -1};
  }
  void rim_loops() {
    std::pmr::vector<RimWord> rim(&budget);
    std::pmr::vector<uint32_t> order(&budget);
    for (const auto &edge : input.packets->rims) {
      budget.charge(1);
      rim.push_back({grid_site(edge.points[1].value), grid_site(edge.points[0].value), none});
    }
    order.resize(rim.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
      budget.charge(1);
      return side_less(geometry(rim[a].a, rim[a].b), geometry(rim[b].a, rim[b].b));
    });
    for (size_t at = 0; at < order.size();) {
      auto side = geometry(rim[order[at]].a, rim[order[at]].b);
      size_t end = at + 1;
      while (end < order.size()) {
        auto next = geometry(rim[order[end]].a, rim[order[end]].b);
        if (next.axis != side.axis || next.direction != side.direction ||
            next.constant.compare(side.constant) != 0 || next.low.compare(side.high) != 0)
          break;
        side.high = next.high;
        ++end;
      }
      const auto side_index = word(sides.size());
      sides.push_back(std::move(side));
      for (size_t j = at; j < end; ++j)
        rim[order[j]].side = side_index;
      at = end;
    }
    // Initial geometry order is stable under atomic side densification.
    if (input.previous) {
      std::pmr::vector<SideGeometry> old(&budget);
      struct OldAtomic final {
        SideGeometry shape;
        uint64_t id;
      };
      std::pmr::vector<OldAtomic> words(&budget);
      for (const auto &edge : input.previous->edges)
        if (const auto *key = std::get_if<RimIntervalKey>(&edge.provenance)) {
          const auto a =
              read_uv(budget, input.previous->values, input.previous->sites[edge.sites[0]].uv);
          const auto b =
              read_uv(budget, input.previous->values, input.previous->sites[edge.sites[1]].uv);
          const uint32_t axis = a[0].compare(b[0]) ? 0 : 1;
          const int direction = a[axis].compare(b[axis]) < 0 ? 1 : -1;
          words.push_back({{axis, a[1 - axis], direction > 0 ? a[axis] : b[axis],
                            direction > 0 ? b[axis] : a[axis], direction},
                           key->footprint_side});
        }
      std::sort(words.begin(), words.end(), [&](const auto &a, const auto &b) {
        budget.charge(1);
        return a.id != b.id ? a.id < b.id : side_less(a.shape, b.shape);
      });
      for (size_t at = 0; at < words.size();) {
        auto shape = words[at].shape;
        auto end = at + 1;
        while (end < words.size() && words[end].id == words[at].id) {
          require(shape.high.compare(words[end].shape.low) == 0, "prior footprint side has a gap");
          shape.high = words[end].shape.high;
          ++end;
        }
        require(words[at].id == old.size(), "prior footprint side inventory is not canonical");
        old.push_back(std::move(shape));
        at = end;
      }
      require(old.size() == sides.size(), "packet footprint changed within boundary generation");
      for (size_t i = 0; i < old.size(); ++i)
        require(same_side(old[i], sides[i]), "packet footprint geometry changed");
    }
    std::pmr::vector<std::pair<uint32_t, uint32_t>> outgoing(&budget), incoming(&budget);
    for (uint32_t i = 0; i < rim.size(); ++i) {
      outgoing.push_back({rim[i].a, i});
      incoming.push_back({rim[i].b, i});
    }
    std::sort(outgoing.begin(), outgoing.end());
    std::sort(incoming.begin(), incoming.end());
    for (size_t i = 0; i < outgoing.size(); ++i) {
      require(outgoing[i].first == incoming[i].first, "packet rim has open incidence");
      if (i)
        require(outgoing[i - 1].first != outgoing[i].first,
                "packet rim pinch survived packet construction");
    }
    std::pmr::vector<uint8_t> seen(rim.size(), uint8_t{0}, &budget);
    std::pmr::vector<uint32_t> atomic(sides.size(), 0, &budget);
    // Ordinal follows the geometric low-to-high atomic order, independent of
    // cycle start.
    std::pmr::vector<uint32_t> ordinal(rim.size(), 0, &budget);
    for (uint32_t index : order)
      ordinal[index] = atomic[rim[index].side]++;
    for (const auto &[start, edge_index] : outgoing) {
      (void)start;
      if (seen[edge_index])
        continue;
      loops.emplace_back(&budget);
      auto &loop = loops.back();
      uint32_t edge = edge_index;
      Rational area(budget);
      do {
        require(!seen[edge], "packet rim cycle revisits another cycle");
        seen[edge] = 1;
        const auto &word_row = rim[edge];
        loop.sites.push_back(word_row.a);
        loop.edges.push_back(
            {{word_row.a, word_row.b},
             RimIntervalKey{word_row.side, input.key.construction_revision, ordinal[edge]}});
        area = area.add(points[word_row.a][0]
                            .multiply(points[word_row.b][1])
                            .subtract(points[word_row.a][1].multiply(points[word_row.b][0])));
        const auto next =
            std::lower_bound(outgoing.begin(), outgoing.end(), std::pair{word_row.b, uint32_t{0}});
        require(next != outgoing.end() && next->first == word_row.b,
                "packet rim successor missing");
        edge = next->second;
      } while (edge != edge_index);
      require(area.sign() != 0, "packet rim cycle has zero area");
      loop.positive = area.sign() > 0;
    }
  }
  void anchor_sites() {
    if (!input.anchors) {
      require(input.anchor_births.empty() && input.anchor_components.empty(),
              "anchor metadata without sample lease");
      return;
    }
    require(input.anchors->view.key() == input.surface.key(),
            "anchor samples use stale surface view");
    require(input.anchor_births.size() == input.anchors->points.size(),
            "anchor birth inventory differs");
    if (input.previous)
      require(input.anchor_components.size() == input.anchors->points.size(),
              "rebuild anchor component inventory missing");
    for (uint32_t i = 0; i < input.anchors->points.size(); ++i) {
      budget.charge(1);
      const auto &row = input.anchors->points[i];
      auto uv = read_uv(budget, input.anchors->values, row.chart_xy);
      require(equal_uv(uv, read_uv(budget, input.anchors->values, row.original_uv)),
              "band anchor uses alternate chart");
      anchors.push_back(
          append(std::move(uv),
                 ConstructionSite{{},
                                  row.actual_xyz,
                                  row.key.constructed_generation,
                                  SurfaceSiteSource{row.key.original, row.key.stage, true,
                                                    input.anchor_births[i], row.departure_upper}}));
    }
  }
  struct BandSites final {
    std::span<const uint32_t> loops, anchors;
  };
  void triangulate(BandSites inventory, uint64_t component, uint64_t revision, bool discover) {
    const auto loop_ids = inventory.loops, anchor_ids = inventory.anchors;
    std::pmr::vector<uint32_t> global(&budget);
    std::pmr::vector<UV> local(&budget);
    std::pmr::vector<std::pmr::vector<uint32_t>> cycle_sites(&budget);
    auto add = [&](uint32_t index) {
      budget.charge(1);
      const auto slot = word(local.size());
      global.push_back(index);
      local.push_back(points[index]);
      return slot;
    };
    for (uint32_t id : loop_ids) {
      cycle_sites.emplace_back();
      for (uint32_t index : loops[id].sites)
        cycle_sites.back().push_back(add(index));
    }
    for (uint32_t index : anchor_ids) {
      (void)add(index);
    }
    std::pmr::vector<RegionCycleView> cycle_views(&budget);
    for (const auto &cycle : cycle_sites)
      cycle_views.push_back({cycle});
    auto result =
        construct_region_cdt(budget, local, cycle_views, metric, 0x243f6a8885a308d3ULL, discover);
    if (discover) {
      for (uint32_t j = 0; j < loop_ids.size(); ++j)
        loops[loop_ids[j]].component = result.loop_components[j];
      for (uint32_t j = 0; j < result.triangles.size(); ++j) {
        const auto &triangle = result.triangles[j];
        facets.push_back({{FacetGenerator::band, result.components[j], revision, 0},
                          {global[triangle[0]], global[triangle[1]], global[triangle[2]]},
                          result.components[j]});
      }
    } else {
      require(result.component_count == 1,
              "local band rebuild changed connected component inventory");
      for (uint32_t id : loop_ids)
        loops[id].component = static_cast<uint32_t>(component);
      uint32_t ordinal = 0;
      for (const auto &triangle : result.triangles) {
        facets.push_back({{FacetGenerator::band, component, revision, ordinal++},
                          {global[triangle[0]], global[triangle[1]], global[triangle[2]]},
                          static_cast<uint32_t>(component)});
        changed_keys.push_back(facets.back().key);
      }
    }
  }
  std::pair<uint32_t, uint64_t> loop_token(const BuildLoop &loop) const {
    if (loop.source)
      return {0, loop.source->value};
    uint64_t minimum = UINT64_MAX;
    for (const auto &edge : loop.edges)
      minimum = std::min(minimum, std::get<RimIntervalKey>(edge.provenance).footprint_side);
    return {1, minimum};
  }
  std::pair<uint32_t, uint64_t> old_token(const RegionLoop &loop) const {
    if (loop.source_loop)
      return {0, loop.source_loop->value};
    uint64_t minimum = UINT64_MAX;
    for (uint64_t j = 0; j < loop.edges.count; ++j)
      minimum = std::min(
          minimum, std::get<RimIntervalKey>(input.previous->edges[loop.edges.first + j].provenance)
                       .footprint_side);
    return {1, minimum};
  }
  void band_facets() {
    if (!input.previous) {
      std::pmr::vector<uint32_t> ids(loops.size(), &budget);
      std::iota(ids.begin(), ids.end(), 0);
      if (!ids.empty())
        triangulate({ids, anchors}, 0, input.key.construction_revision, true);
      else
        require(anchors.empty(), "anchor has no band component");
      return;
    }
    const auto &old = *input.previous;
    struct OldLoop final {
      std::pair<uint32_t, uint64_t> token;
      uint32_t component, index;
    };
    std::pmr::vector<OldLoop> tokens(&budget);
    for (uint32_t component = 0; component < old.components.size(); ++component) {
      const auto &row = old.components[component];
      for (uint64_t j = 0; j < row.loops.count; ++j)
        tokens.push_back(
            {old_token(old.loops[row.loops.first + j]), component, word(row.loops.first + j)});
    }
    std::sort(tokens.begin(), tokens.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a.token < b.token;
    });
    std::pmr::vector<uint32_t> prior_loop(loops.size(), none, &budget);
    std::pmr::vector<std::pmr::vector<uint32_t>> component_loops(&budget),
        component_anchors(&budget);
    component_loops.resize(old.components.size());
    component_anchors.resize(old.components.size());
    for (uint32_t i = 0; i < loops.size(); ++i) {
      auto &loop = loops[i];
      const auto token = loop_token(loop);
      const auto hit =
          std::lower_bound(tokens.begin(), tokens.end(), token, [&](const auto &a, const auto &b) {
            budget.charge(1);
            return a.token < b;
          });
      require(hit != tokens.end() && hit->token == token,
              "local band rebuild changed geometric cycle inventory");
      loop.component = hit->component;
      prior_loop[i] = hit->index;
      component_loops[loop.component].push_back(i);
    }
    for (uint32_t i = 0; i < anchors.size(); ++i) {
      require(input.anchor_components[i] < component_anchors.size(),
              "band anchor component is stale");
      component_anchors[input.anchor_components[i]].push_back(anchors[i]);
    }
    std::pmr::vector<uint32_t> sorted(points.size(), &budget);
    std::iota(sorted.begin(), sorted.end(), 0);
    std::sort(sorted.begin(), sorted.end(), [&](uint32_t a, uint32_t b) {
      budget.charge(1);
      return uv_less(points[a], points[b]);
    });
    std::pmr::vector<uint32_t> remap(old.sites.size(), none, &budget);
    for (uint32_t i = 0; i < old.sites.size(); ++i) {
      auto point = read_uv(budget, old.values, old.sites[i].uv);
      const auto hit =
          std::lower_bound(sorted.begin(), sorted.end(), point, [&](uint32_t a, const UV &b) {
            budget.charge(1);
            return uv_less(points[a], b);
          });
      if (hit != sorted.end() && equal_uv(points[*hit], point))
        remap[i] = *hit;
    }
    std::pmr::vector<uint32_t> old_anchor_component(points.size(), none, &budget);
    for (const auto &facet : old.facets)
      if (facet.key.generator == FacetGenerator::band)
        for (uint32_t index : facet.sites) {
          budget.charge(1);
          if (remap[index] != none &&
              std::holds_alternative<SurfaceSiteSource>(old.sites[index].source) &&
              std::get<SurfaceSiteSource>(old.sites[index].source).anchor)
            old_anchor_component[remap[index]] = facet.component;
        }
    for (uint32_t component = 0; component < old.components.size(); ++component) {
      const auto &prior = old.components[component];
      require(prior.id == component, "prior band component identity is not canonical");
      const auto &ids = component_loops[component], &owned_anchors = component_anchors[component];
      bool changed = std::find(input.changed_components.begin(), input.changed_components.end(),
                               component) != input.changed_components.end();
      if (ids.size() != prior.loops.count)
        throw Error(ErrorCode::invalid_source, "frozen band cycle inventory changed");
      for (uint32_t id : ids) {
        const auto &previous_loop = old.loops[prior_loop[id]];
        if (previous_loop.sites.count != loops[id].sites.size())
          changed = true;
        else
          for (uint64_t j = 0; j < previous_loop.sites.count; ++j) {
            budget.charge(1);
            const auto previous_site = old.loop_sites[previous_loop.sites.first + j];
            if (remap[previous_site] != loops[id].sites[j])
              changed = true;
          }
      }
      for (uint64_t j = 0; j < prior.facets.count; ++j) {
        const auto &facet = old.facets[prior.facets.first + j];
        for (uint32_t index : facet.sites)
          if (remap[index] == none || !same_actual(old.sites[index], sites[remap[index]]))
            changed = true;
      }
      for (uint32_t index : owned_anchors) {
        budget.charge(1);
        if (old_anchor_component[index] != component)
          changed = true;
      }
      if (changed) {
        if (prior.revision == UINT64_MAX)
          throw Error(ErrorCode::resource_exhausted, "band component revision exhausted");
        triangulate({ids, owned_anchors}, component, prior.revision + 1, false);
      } else
        for (uint64_t j = 0; j < prior.facets.count; ++j) {
          auto facet = old.facets[prior.facets.first + j];
          for (auto &index : facet.sites)
            index = remap[index];
          facets.push_back(facet);
        }
    }
    for (uint64_t component : input.anchor_components)
      require(component < old.components.size(), "band anchor component is stale");
  }

public:
  RegionBuilder(const RegionInput &request, JobContext &job)
      : input(request), budget(job.budget), resource(request.domain->resource),
        exact(resource, request.table_key), points(&budget), sites(resource.get()),
        bindings(resource.get()), loops(&budget), facets(resource.get()), changed_keys(&budget),
        components(resource.get()), grid(request.surface.selection()->points.size(), none, &budget),
        anchors(&budget), sides(&budget), metric(face_metric(request, budget)) {}
  FaceConstructionResult run() {
    source_sites();
    packet_facets();
    rim_loops();
    anchor_sites();
    band_facets();
    // Group actual facets and source cycles by stable generator/component keys.
    // Initial global component output has not yet assigned per-component
    // ordinals.
    if (!input.previous) {
      std::pmr::vector<uint32_t> ordinals(&budget);
      ordinals.resize(loops.size(), 0);
      for (auto &facet : facets)
        if (facet.key.generator == FacetGenerator::band) {
          require(facet.component < ordinals.size(), "band ordinal component missing");
          facet.key.ordinal = ordinals[facet.component]++;
        }
    }
    std::sort(facets.begin(), facets.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a.key < b.key;
    });
    std::pmr::vector<uint8_t> used(sites.size(), uint8_t{0}, &budget);
    for (const auto &facet : facets)
      for (uint32_t site : facet.sites)
        used[site] = 1;
    std::pmr::vector<uint32_t> site_remap(sites.size(), none, &budget);
    std::pmr::vector<ConstructionSite> retained_sites(resource.get());
    for (uint32_t i = 0; i < sites.size(); ++i) {
      budget.charge(1);
      if (!used[i]) {
        const auto *sample = std::get_if<SurfaceSiteSource>(&sites[i].source);
        require(!input.previous && sample && sample->anchor,
                "construction retained an unused numerical/source site");
        continue;
      }
      site_remap[i] = word(retained_sites.size());
      retained_sites.push_back(std::move(sites[i]));
    }
    for (auto &binding : bindings)
      binding.construction_site = site_remap[binding.construction_site];
    std::pmr::vector<uint32_t> loop_order(loops.size(), &budget);
    std::iota(loop_order.begin(), loop_order.end(), 0);
    std::sort(loop_order.begin(), loop_order.end(), [&](uint32_t a, uint32_t b) {
      budget.charge(1);
      return loops[a].component != loops[b].component ? loops[a].component < loops[b].component
                                                      : loop_token(loops[a]) < loop_token(loops[b]);
    });
    std::pmr::vector<uint32_t> loop_sites(resource.get());
    std::pmr::vector<ProtectedRegionEdge> edges(resource.get());
    std::pmr::vector<RegionLoop> output_loops(resource.get());
    for (uint32_t id : loop_order) {
      const auto &loop = loops[id];
      const auto site_first = loop_sites.size(), edge_first = edges.size();
      for (uint32_t site : loop.sites)
        loop_sites.push_back(site_remap[site]);
      for (auto edge : loop.edges) {
        for (auto &site : edge.sites)
          site = site_remap[site];
        edges.push_back(std::move(edge));
      }
      output_loops.push_back({loop.positive,
                              {site_first, loop.sites.size()},
                              {edge_first, loop.edges.size()},
                              loop.source,
                              loop.adapter});
    }
    for (size_t at = 0; at < loop_order.size();) {
      const auto component = loops[loop_order[at]].component;
      size_t end = at + 1;
      while (end < loop_order.size() && loops[loop_order[end]].component == component)
        ++end;
      const auto first = std::lower_bound(
          facets.begin(), facets.end(), FacetKey{FacetGenerator::band, component, 0, 0},
          [](const auto &a, const auto &key) { return a.key < key; });
      auto last = first;
      while (last != facets.end() && last->key.generator == FacetGenerator::band &&
             last->key.owner == component)
        ++last;
      require(first != last, "band component has no actual facets");
      components.push_back(
          {component,
           first->key.revision,
           {at, end - at},
           {static_cast<uint64_t>(first - facets.begin()), static_cast<uint64_t>(last - first)}});
      at = end;
    }
    std::array<ExactRef, 4> factor{};
    for (uint32_t i = 0; i < 4; ++i)
      factor[i] = exact.append(metric[i]);
    auto output = std::allocate_shared<FaceConstructionResultData>(
        RunAllocator<FaceConstructionResultData>(resource), resource, input.domain->source,
        input.surface.source(), input.domain->curves, input.domain, input.surface, input.packets,
        input.key, exact.seal());
    output->fixed_metric_factor = factor;
    output->sites = std::move(retained_sites);
    output->source_bindings = std::move(bindings);
    output->loop_sites = std::move(loop_sites);
    for (auto &facet : facets)
      for (auto &site : facet.sites)
        site = site_remap[site];
    output->edges = std::move(edges);
    output->loops = std::move(output_loops);
    output->components = std::move(components);
    output->facets = std::move(facets);
    output->anchor_components.assign(anchors.size(), UINT64_MAX);
    std::pmr::vector<uint32_t> anchor_of_site(output->sites.size(), none, &budget);
    for (uint32_t i = 0; i < anchors.size(); ++i)
      if (site_remap[anchors[i]] != none)
        anchor_of_site[site_remap[anchors[i]]] = i;
    for (const auto &facet : output->facets)
      if (facet.key.generator == FacetGenerator::band)
        for (uint32_t index : facet.sites) {
          budget.charge(1);
          if (anchor_of_site[index] != none)
            output->anchor_components[anchor_of_site[index]] = facet.component;
        }
    std::sort(changed_keys.begin(), changed_keys.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a < b;
    });
    size_t cursor = 0;
    for (uint32_t i = 0; i < output->facets.size(); ++i) {
      budget.charge(1);
      const auto &key = output->facets[i].key;
      while (cursor < changed_keys.size() && changed_keys[cursor] < key)
        ++cursor;
      if (!input.previous || (cursor < changed_keys.size() && changed_keys[cursor] == key))
        output->changed_facets.push_back(i);
    }
    return output;
  }
};
void validate_region_input(const RegionInput &input) {
  require_domain_key(input.domain, input.key);
  require(input.surface && input.packets, "missing surface/packet construction input");
  require(input.domain->resource == input.surface.resource() &&
              input.packets->selection == input.surface.selection(),
          "foreign surface packet selection");
  require(input.surface.classified_domain() == input.domain,
          "surface cover was not classified against this chosen domain");
  require(input.table_key != input.domain->values.key() &&
              input.table_key != input.surface.selection()->values.key() &&
              input.table_key != input.domain->curves->values.key() &&
              (!input.previous || input.table_key != input.previous->values.key()) &&
              (!input.anchors || input.table_key != input.anchors->values.key()),
          "construction exact table aliases retained input values");
  const auto &source = input.domain->source.view();
  const auto carrier = source.carrier_index.find(source.faces[input.key.face.value].carrier_id);
  require(&input.surface.source().source().view() == &source,
          "surface view belongs to another source store");
  for (const auto &leaf : input.surface.selection()->leaves)
    require(leaf.key.root.value < input.surface.source().roots().size() &&
                input.surface.source().roots()[leaf.key.root.value].source_slot == carrier.value,
            "surface selection contains another face carrier");
  const auto &key = input.surface.key();
  require(key.face.definition == input.key.definition && key.face.face == input.key.face &&
              key.face.source_generation == input.key.source_generation &&
              key.face.boundary_generation == input.key.boundary_generation &&
              key.face.chart_revision == input.key.chart_revision &&
              key.face.assignment_epoch == input.key.assignment_epoch &&
              key.selection.query == input.key.query &&
              key.selection.profile_revision == input.key.profile_revision,
          "stale face surface construction view");
  if (input.previous) {
    require_domain_key(input.previous->domain, input.key);
    require(input.previous->key.construction_revision < input.key.construction_revision,
            "construction rebuild revision did not advance");
  }
}
} // namespace
FaceConstructionResult build_face_regions(const RegionInput &input, JobContext &context) {
  validate_region_input(input);
  return RegionBuilder(input, context).run();
}
FaceConstructionResult rebuild_face_regions(const RegionInput &input, JobContext &context) {
  require(static_cast<bool>(input.previous), "local region rebuild has no previous result");
  return build_face_regions(input, context);
}
namespace {
Rational dyadic_round(const Rational &value, uint32_t bits) {
  auto &budget = value.budget();
  const auto &numerator = value.numerator();
  auto magnitude = numerator.sign() < 0 ? numerator.negative() : bgfd::Integer(numerator);
  auto division =
      bgfd::Integer::divmod_nonnegative(magnitude.shift_left(bits), value.denominator());
  const int compare = division.second.shift_left(1).compare(value.denominator());
  const bool odd = !division.first.magnitude().empty() && (division.first.magnitude()[0] & 1);
  if (compare > 0 || (compare == 0 && odd))
    division.first = division.first.add(Rational(budget, 1).numerator());
  if (value.sign() < 0)
    division.first = division.first.negative();
  return Rational::from_ratio(std::move(division.first),
                              Rational(budget, 1).numerator().shift_left(bits));
}
bool strict_triangle(const std::array<UV, 3> &triangle, const UV &point) {
  for (uint32_t i = 0; i < 3; ++i)
    if (cross(triangle[i], triangle[(i + 1) % 3], point).sign() <= 0)
      return false;
  return true;
}
UV centroid(const std::array<UV, 3> &triangle, bgfd::Budget &budget) {
  return {triangle[0][0].add(triangle[1][0]).add(triangle[2][0]).divide(Rational(budget, 3)),
          triangle[0][1].add(triangle[1][1]).add(triangle[2][1]).divide(Rational(budget, 3))};
}
std::array<UV, 3> facet_uv(const FaceConstructionResult &construction, const RegionFacet &facet,
                           bgfd::Budget &budget) {
  return {read_uv(budget, construction->values, construction->sites[facet.sites[0]].uv),
          read_uv(budget, construction->values, construction->sites[facet.sites[1]].uv),
          read_uv(budget, construction->values, construction->sites[facet.sites[2]].uv)};
}
uint64_t unsigned_word(const bgfd::Integer &value) {
  require(value.sign() >= 0 && value.bit_length() <= 64, "packet target integer exceeds 64 bits");
  const auto words = value.magnitude();
  return (words.empty() ? 0 : words[0]) |
         (words.size() > 1 ? static_cast<uint64_t>(words[1]) << 32 : 0);
}
PacketGridTarget grid_target(const FaceConstructionResult &construction, const RegionFacet &facet,
                             bgfd::Budget &budget) {
  const auto selection = construction->surface.selection();
  require(facet.leaf < selection->leaves.size(), "packet facet source leaf is stale");
  const auto leaf_index = facet.leaf;
  const auto &leaf = selection->leaves[leaf_index];
  require(leaf.key.root.value < selection->source.roots().size(),
          "packet prepared root is missing");
  const auto &root = selection->source.roots()[leaf.key.root.value];
  const auto triangle = facet_uv(construction, facet, budget);
  const auto middle = centroid(triangle, budget);
  std::array<Rational, 2> origin{Rational::from_double(budget, root.prepared_box[0].lower),
                                 Rational::from_double(budget, root.prepared_box[2].lower)};
  std::array<Rational, 2> width{
      Rational::from_double(budget, root.prepared_box[1].upper).subtract(origin[0]),
      Rational::from_double(budget, root.prepared_box[3].upper).subtract(origin[1])};
  for (uint32_t axis = 0; axis < 4; ++axis)
    require(root.prepared_box[axis].lower == root.prepared_box[axis].upper,
            "prepared root endpoint is not exact binary64");
  require(width[0].sign() > 0 && width[1].sign() > 0, "packet prepared root has nonpositive width");
  const UV normalized{middle[0].subtract(origin[0]).divide(width[0]),
                      middle[1].subtract(origin[1]).divide(width[1])};
  for (uint32_t bits = 1; bits <= 64; ++bits) {
    UV gridpoint{dyadic_round(normalized[0], bits), dyadic_round(normalized[1], bits)};
    const UV point{origin[0].add(width[0].multiply(gridpoint[0])),
                   origin[1].add(width[1].multiply(gridpoint[1]))};
    if (!strict_triangle(triangle, point))
      continue;
    PacketGridTarget target;
    target.old_facet = facet.key;
    target.surface = construction->surface.key();
    target.root = leaf.key.root;
    target.leaf = leaf_index;
    for (uint32_t axis = 0; axis < 2; ++axis) {
      target.numerator[axis] = unsigned_word(gridpoint[axis].numerator());
      const auto power = gridpoint[axis].denominator().bit_length() - 1;
      require(power <= 64, "packet target dyadic depth exceeds cap");
      target.depth[axis] = static_cast<uint32_t>(power);
    }
    return target;
  }
  throw Error(ErrorCode::unresolved,
              "failed packet facet has no capped strict-interior grid target");
}
} // namespace
ConstructionRefinement plan_construction_refinement(const RefinementInput &input,
                                                    JobContext &context) {
  require(input.construction && input.bounds, "missing construction bound horizon");
  const auto &construction = input.construction;
  const auto &key = input.bounds->work.key();
  require(key.construction == construction->key && key.surface == construction->surface.key(),
          "stale construction facet bound horizon");
  auto &budget = context.budget;
  const auto resource = construction->resource;
  require(input.bounds->resource == resource && input.anchor_bits > 0 &&
              input.anchor_bits <= UINT32_MAX,
          "invalid construction refinement owner/cap");
  ExactBuilder exact(resource, input.table_key);
  std::pmr::vector<LeafKey> splits(resource.get());
  std::pmr::vector<PacketGridTarget> targets(resource.get());
  std::pmr::vector<BandAnchorLocator> anchors(resource.get());
  std::pmr::vector<uint64_t> changed(resource.get());
  std::pmr::vector<uint32_t> failed(&budget), leaf_splits(&budget);
  const auto selection = construction->surface.selection();
  const auto prepared = input.bounds->work.facets();
  require(input.bounds->facets.size() == prepared.size(),
          "partial construction facet completion horizon");
  std::pmr::vector<std::pair<uint32_t, uint32_t>> prepared_by_facet(&budget);
  for (uint32_t i = 0; i < prepared.size(); ++i)
    prepared_by_facet.push_back({prepared[i].construction_facet, i});
  std::sort(prepared_by_facet.begin(), prepared_by_facet.end(), [&](const auto &a, const auto &b) {
    budget.charge(1);
    return a < b;
  });
  std::pmr::vector<uint32_t> completed(&budget);
  for (const auto &answer : input.bounds->facets) {
    budget.charge(1);
    require(answer.facet < construction->facets.size(),
            "facet result references absent construction triangle");
    completed.push_back(answer.facet);
    if (!answer.accepted)
      failed.push_back(answer.facet);
  }
  const auto index_less = [&](const auto &a, const auto &b) {
    budget.charge(1);
    return a < b;
  };
  std::sort(completed.begin(), completed.end(), index_less);
  for (uint32_t i = 0; i < completed.size(); ++i) {
    require(!i || completed[i] != completed[i - 1], "duplicate construction facet completion");
    require(completed[i] == prepared_by_facet[i].first,
            "construction completion does not match captured numerical request");
  }
  std::sort(failed.begin(), failed.end(), index_less);
  for (uint32_t failed_facet : failed) {
    const auto &facet = construction->facets[failed_facet];
    if (facet.key.generator == FacetGenerator::packet) {
      auto target = grid_target(construction, facet, budget);
      target.facet = failed_facet;
      targets.push_back(std::move(target));
      continue;
    }
    require(facet.component < construction->components.size(),
            "failed band facet has stale component");
    const auto triangle = facet_uv(construction, facet, budget);
    const auto middle = centroid(triangle, budget);
    const auto area = cross(triangle[0], triangle[1], triangle[2]);
    require(area.sign() > 0, "failed band parent facet is not positive");
    std::optional<UV> chosen;
    for (uint64_t bits = 1; bits <= input.anchor_bits; ++bits) {
      const auto precision = static_cast<uint32_t>(bits);
      UV candidate{dyadic_round(middle[0], precision), dyadic_round(middle[1], precision)};
      bool central = true;
      for (uint32_t j = 0; j < 3; ++j)
        if (cross(triangle[j], triangle[(j + 1) % 3], candidate)
                .multiply(Rational(budget, 6))
                .compare(area) < 0)
          central = false;
      if (central) {
        chosen = std::move(candidate);
        break;
      }
    }
    if (!chosen)
      throw Error(ErrorCode::unresolved,
                  "failed band facet has no central anchor under exact-word cap");
    const auto &anchor_point = chosen.value();
    uint32_t chosen_leaf = none;
    const auto contains = [&](uint32_t index) {
      budget.charge(1);
      require(index < selection->leaves.size(), "numerical source overlap names missing leaf");
      const auto &leaf = selection->leaves[index];
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto low = import_exact(budget, selection->values.view(leaf.box[2 * axis])),
                   high = import_exact(budget, selection->values.view(leaf.box[2 * axis + 1]));
        if (anchor_point[axis].compare(low) < 0 || anchor_point[axis].compare(high) > 0)
          return false;
      }
      return true;
    };
    const auto hit = std::lower_bound(prepared_by_facet.begin(), prepared_by_facet.end(),
                                      std::pair{failed_facet, uint32_t{0}}, index_less);
    require(hit != prepared_by_facet.end() && hit->first == failed_facet,
            "failed facet absent from retained numerical work");
    const auto &record = prepared[hit->second];
    if (record.pieces.count) {
      require(record.pieces.first + record.pieces.count <= input.bounds->work.pieces().size(),
              "failed facet piece inventory exceeds numerical lease");
      // The positive pieces already partition the captured parent triangle.
      // Its central point belongs to at least one of their closed leaf boxes.
      for (uint64_t i = 0; i < record.pieces.count; ++i) {
        const auto leaf = input.bounds->work.pieces()[record.pieces.first + i].leaf;
        if (contains(leaf)) {
          chosen_leaf = leaf;
          break;
        }
      }
    } else {
      require(record.original_plane, "nonplane failed facet has no positive numerical overlap");
      // The admitted original-plane fast path deliberately omits clip pieces.
      for (uint32_t i = 0; i < selection->leaves.size(); ++i)
        if (contains(i)) {
          chosen_leaf = i;
          break;
        }
    }
    require(chosen_leaf < selection->leaves.size(),
            "failed band central anchor lacks a covered source leaf");
    anchors.push_back({facet.key,
                       facet.component,
                       chosen_leaf,
                       {exact.append((*chosen)[0]), exact.append((*chosen)[1])},
                       input.horizon});
    changed.push_back(facet.component);
    leaf_splits.push_back(chosen_leaf);
  }
  std::sort(leaf_splits.begin(), leaf_splits.end());
  leaf_splits.erase(std::unique(leaf_splits.begin(), leaf_splits.end()), leaf_splits.end());
  for (uint32_t leaf : leaf_splits)
    splits.push_back(selection->leaves[leaf].key);
  std::sort(changed.begin(), changed.end());
  changed.erase(std::unique(changed.begin(), changed.end()), changed.end());
  auto output = std::allocate_shared<ConstructionRefinementData>(
      RunAllocator<ConstructionRefinementData>(resource), resource, construction,
      construction->surface, exact.seal());
  output->splits = std::move(splits);
  output->packet_targets = std::move(targets);
  output->band_anchors = std::move(anchors);
  output->changed_components = std::move(changed);
  return output;
}
} // namespace bgmesh::adaptive
