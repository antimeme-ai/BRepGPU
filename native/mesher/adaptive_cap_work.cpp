#include "adaptive_cap_work.hpp"
#include "adaptive_numeric.hpp"
#include "adaptive_surface_cache.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <numeric>

namespace bgmesh::adaptive {
struct CapFacetData final {
  SurfaceBoundView surface;
  FaceConstructionResult construction;
  FaceDomainLease domain;
  FacetAllowances allowances;
  ExactTable values;
  ExactRef allowance;
  TrimFacetWorkKey key;
  TrimFacetLimits limits;
  CapPreparationStats stats;
  std::pmr::vector<CapFacetRecord> facets;
  std::pmr::vector<PreparedCapSource> sources;
  CapFacetData(SurfaceBoundView view, FaceConstructionResult mesh, FaceDomainLease policy,
               FacetAllowances quality, ExactTable exact, ExactRef tau, TrimFacetLimits caps)
      : surface(std::move(view)), construction(std::move(mesh)), domain(std::move(policy)),
        allowances(std::move(quality)), values(std::move(exact)), allowance(tau),
        key{surface.key(), construction->key, values.key()}, limits(caps),
        facets(surface.resource().get()), sources(surface.resource().get()) {}
};
struct CapWorkAccess final {
  static const CapFacetData &get(const CapFacetWork &work) { return *work.data_; }
};
const TrimFacetWorkKey &CapFacetWork::key() const { return data_->key; }
const ExactTable &CapFacetWork::values() const { return data_->values; }
std::span<const CapFacetRecord> CapFacetWork::facets() const { return data_->facets; }
CapPreparationStats CapFacetWork::stats() const { return data_->stats; }

namespace {
void require(bool okay, const char *why) {
  if (!okay)
    throw Error(ErrorCode::invalid_source, why);
}
Rational read(bgfd::Budget &budget, const ExactTable &values, ExactRef ref) {
  return import_exact(budget, values.view(ref));
}
bool same_xyz(std::span<const double, 3> a, std::span<const double, 3> b) {
  for (uint32_t i = 0; i < 3; ++i)
    if (std::bit_cast<uint64_t>(a[i]) != std::bit_cast<uint64_t>(b[i]))
      return false;
  return true;
}
bool same_generation(const FaceConstructionKey &a, const FaceConstructionKey &b) {
  return a.source_generation == b.source_generation && a.query == b.query &&
         a.definition == b.definition && a.face == b.face &&
         a.boundary_generation == b.boundary_generation && a.face_generation == b.face_generation &&
         a.chart_revision == b.chart_revision && a.domain_revision == b.domain_revision &&
         a.assignment_epoch == b.assignment_epoch && a.profile_revision == b.profile_revision;
}
bool same_cap_key(const CapDomainKey &a, const FaceConstructionKey &b) {
  return a.source_generation == b.source_generation && a.query == b.query &&
         a.definition == b.definition && a.face == b.face &&
         a.boundary_generation == b.boundary_generation && a.face_generation == b.face_generation &&
         a.chart_revision == b.chart_revision && a.domain_revision == b.domain_revision &&
         a.assignment_epoch == b.assignment_epoch && a.profile_revision == b.profile_revision;
}
template <class T> bool contains(Range<T> range, uint32_t index, size_t size) {
  return range.first <= size && range.count <= size - range.first && index >= range.first &&
         index - range.first < range.count;
}
void same_parameter(bgfd::Budget &budget, const ExactTable &a, ExactRef x, const ExactTable &b,
                    ExactRef y, const char *why) {
  require(read(budget, a, x).compare(read(budget, b, y)) == 0, why);
}
void physical_site(const FaceConstructionResultData &mesh, uint32_t site, uint32_t node) {
  const auto &boundary = *mesh.curves->boundary;
  require(site < mesh.sites.size() && node < boundary.nodes.size() &&
              node < boundary.constructed_revisions.size(),
          "cap physical site outside captured boundary");
  const auto &value = mesh.sites[site];
  const auto *authority = std::get_if<BoundarySiteSource>(&value.source);
  require(authority && authority->boundary_node == node &&
              value.constructed_revision == boundary.constructed_revisions[node] &&
              same_xyz(value.actual_xyz, boundary.nodes[node].xyz),
          "cap site differs from current physical authority");
}
void physical_cut(const FaceConstructionResultData &mesh, uint32_t site,
                  const PhysicalCapEndpoint &plan, const ProducedCapDomainLease &caps,
                  bgfd::Budget &budget) {
  require(site < mesh.sites.size(), "cap cut names foreign construction site");
  const auto *authority = std::get_if<BoundarySiteSource>(&mesh.sites[site].source);
  require(authority, "physical cap cut has private surface authority");
  physical_site(mesh, site, authority->boundary_node);
  const auto &boundary = *mesh.curves->boundary;
  const auto &node = boundary.nodes[authority->boundary_node];
  const auto &source = mesh.source.view();
  require(plan.edge.value < source.edges.size() && plan.sector.value < source.coedges.size() &&
              source.coedges[plan.sector.value].edge_id == source.edges[plan.edge.value].id &&
              node.key == plan.expected_key && node.original_parameter,
          "cap cut original edge/sector/key mismatch");
  const auto found = std::lower_bound(boundary.edges.begin(), boundary.edges.end(), plan.edge,
                                      [&](const SourceBoundaryEdge &edge, Slot<EdgeTag> key) {
                                        budget.charge(1);
                                       require(edge.edge.value < source.edges.size(),
                                               "cap master inventory has foreign original edge");
                                       return source.edges[edge.edge.value].id <
                                              source.edges[key.value].id;
                                      });
  require(found != boundary.edges.end() && found->edge == plan.edge &&
              contains(found->nodes, authority->boundary_node, boundary.nodes.size()),
          "cap cut no longer belongs to its current master chain");
  same_parameter(budget, caps.values(), plan.original_t, boundary.parameters,
                 *node.original_parameter, "cap cut changed original spatial parameter");
}
void surface_cut(const FaceConstructionResultData &mesh, uint32_t site, uint32_t point,
                 const SurfaceCapPoint &plan, const ProducedCapDomainLease &caps,
                 bgfd::Budget &budget) {
  require(mesh.cap_points && site < mesh.sites.size() && point < mesh.cap_point_bindings.size() &&
              mesh.cap_point_bindings[point].sample_row < mesh.cap_points->points.size(),
          "cap interior cut lacks captured PointPool result");
  const auto &value = mesh.sites[site];
  const auto *authority = std::get_if<SurfaceSiteSource>(&value.source);
  const auto &sample = mesh.cap_points->points[mesh.cap_point_bindings[point].sample_row];
  require(authority && authority->original.value == plan.original_root.value &&
              sample.key.original.value == plan.original_root.value &&
              authority->stage == sample.key.stage &&
              value.constructed_revision == sample.key.constructed_generation &&
              same_xyz(value.actual_xyz, sample.actual_xyz),
          "cap interior cut differs from captured PointPool authority");
  for (uint32_t axis = 0; axis < 2; ++axis)
    same_parameter(budget, caps.values(), caps.points()[point].uv[axis], mesh.cap_points->values,
                   sample.original_uv[axis],
                   "cap PointPool locator differs from produced cut plan");
}
} // namespace

CapFacetWork prepare_cap_facet_work(JobContext &context, SurfaceBoundView surface,
                                    FaceConstructionResult construction, FaceDomainLease domain,
                                    FacetAllowances allowances,
                                    std::span<const uint32_t> selected_facets,
                                    ExactTableKey result_key, TrimFacetLimits limits) {
  admit_numeric_environment();
  require(surface && construction && domain && domain->caps,
          "missing cap source/domain/view/construction lease");
  auto &budget = context.budget;
  require(limits.work && limits.pieces && limits.exact_entries && limits.batch_records &&
              limits.candidate_visits && limits.precise_tasks,
          "invalid cap work limits");
  if (selected_facets.size() > limits.pieces || selected_facets.size() > UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "cap work selected facet limit");
  const auto scope = budget.scoped_work(limits.work);
  uint64_t visits = 0;
  const auto visit = [&] {
    budget.charge(1);
    if (++visits > limits.candidate_visits)
      throw Error(ErrorCode::resource_exhausted, "cap work admission visits exhausted");
  };
  const auto resource = surface.resource();
  const auto selection = surface.selection();
  const auto view_key = surface.key();
  const auto &identity = construction->key;
  const auto &caps = domain->caps;
  require(resource && construction->resource == resource && domain->resource == resource &&
              allowances.values.resource() == resource && caps.resource() == resource &&
              &construction->source.view() == &surface.source().source().view() &&
              &domain->source.view() == &construction->source.view() &&
              &construction->surface_source.view() == &surface.source().view() &&
              &caps.source().view() == &surface.source().view() &&
              surface.source().purpose() == SourceCachePurpose::quotient_trim &&
              caps.curves() == construction->curves && domain->curves == construction->curves,
          "cap work source/bank/resource mismatch");
  require(construction->surface && construction->surface.key() == view_key &&
              construction->surface.selection() == selection &&
              surface.classified_domain() == domain && construction->domain == domain &&
              same_generation(identity, domain->key) && same_cap_key(caps.key(), identity) &&
              identity.source_generation == surface.source().generation() &&
              identity.source_generation == view_key.face.source_generation &&
              identity.query == view_key.selection.query &&
              identity.definition == view_key.face.definition &&
              identity.face == view_key.face.face &&
              identity.boundary_generation == view_key.face.boundary_generation &&
              identity.face_generation == view_key.face.face_generation &&
              identity.chart_revision == view_key.face.chart_revision &&
              identity.domain_revision == view_key.face.domain_revision &&
              identity.assignment_epoch == view_key.face.assignment_epoch &&
              identity.profile_revision == view_key.selection.profile_revision &&
              identity.profile_revision == surface.source().profile().revision,
          "stale cap work source/domain/view generation");
  require(result_key.generation == identity.query && result_key != construction->values.key() &&
              result_key != selection->values.key() && result_key != domain->values.key() &&
              result_key != caps.values().key() && result_key != allowances.values.key() &&
              result_key != construction->curves->values.key() &&
              result_key != construction->curves->boundary->parameters.key() &&
              (!construction->cap_points || result_key != construction->cap_points->values.key()),
          "cap work exact result table alias/generation");
  const auto tau = read(budget, allowances.values, allowances.face_position);
  require(tau.sign() > 0, "nonpositive cap positional allowance");
  const auto &source = construction->source.view();
  require(identity.face.value < source.faces.size() &&
              identity.definition.value < source.definitions.size() &&
              source.faces[identity.face.value].definition_id ==
                  source.definitions[identity.definition.value].id,
          "cap work foreign face/definition");
  const auto carrier = source.carrier_index.find(source.faces[identity.face.value].carrier_id);
  std::pmr::vector<uint32_t> requested(&budget);
  requested.reserve(selected_facets.size());
  for (const auto index : selected_facets) {
    visit();
    require(index < construction->facets.size(), "cap work foreign selected facet");
    requested.push_back(index);
  }
  std::sort(requested.begin(), requested.end(), [&](uint32_t a, uint32_t b) {
    visit();
    return construction->facets[a].key < construction->facets[b].key;
  });
  for (size_t i = 1; i < requested.size(); ++i)
    require(construction->facets[requested[i - 1]].key != construction->facets[requested[i]].key,
            "cap work duplicate selected facet");

  require(construction->cap_point_bindings.size() == caps.points().size(),
          "cap work missing produced-point binding inventory");
  if (construction->cap_points) {
    const auto &sample = *construction->cap_points;
    require(sample.resource == resource && sample.values.resource() == resource && sample.view &&
                sample.view.key() == view_key && sample.view.selection() == selection,
            "cap work stale cut PointPool capture");
  }

  ExactBuilder exact(resource, result_key);
  const auto allowance = exact.append(tau);
  uint64_t entries = 1;
  std::pmr::vector<CapFacetRecord> records(resource.get());
  std::pmr::vector<PreparedCapSource> prepared(resource.get());
  std::pmr::map<std::pair<uint32_t, AnalyticCapKind>, uint32_t> prepared_index(&budget);
  for (const auto index : requested) {
    visit();
    const auto &facet = construction->facets[index];
    require(facet.key.generator == FacetGenerator::mapped_cap && facet.mapped,
            "ordinary facet entered mapped cap work");
    const auto &mapped = *facet.mapped;
    require(mapped.cap < caps.caps().size() && mapped.piece < caps.pieces().size(),
            "cap work foreign cap/piece");
    const auto &cap = caps.caps()[mapped.cap];
    const auto &piece = caps.pieces()[mapped.piece];
    require(piece.cap == mapped.cap && contains(cap.pieces, mapped.piece, caps.pieces().size()) &&
                cap.wedge < caps.wedges().size() &&
                piece.original_root.value < surface.source().original_roots().size() &&
                cap.transcendental_bits >= 32 && cap.transcendental_bits <= 4096 &&
                cap.norm_fractional_bits <= 4096,
            "cap work malformed produced cap row");
    const auto &wedge = caps.wedges()[cap.wedge];
    const auto &root = surface.source().original_roots()[piece.original_root.value];
    require(root.source_slot == carrier.value && mapped.normal.kind == wedge.kind &&
                mapped.normal.regular_sheet_sign == piece.regular_sheet_sign,
            "cap work source/normal sector mismatch");
    std::array<uint32_t, 3> source_sites{};
    for (uint32_t role = 0; role < 3; ++role) {
      const auto position = mapped.source_role_to_facet[role];
      require(position < 3 && facet.sites[position] < construction->sites.size(),
              "cap work foreign source-role site");
      source_sites[role] = facet.sites[position];
      require(piece.positive_chart_roles[role] < 3 &&
                  mapped.source_role_to_facet[piece.positive_chart_roles[role]] == role,
              "cap work source-role permutation differs from positive chart roles");
    }
    require(source_sites[0] != source_sites[1] && source_sites[0] != source_sites[2] &&
                source_sites[1] != source_sites[2],
            "cap work repeated actual site");
    for (uint32_t side = 0; side < 2; ++side)
      same_parameter(budget, construction->values, mapped.normal.angular_interval[side],
                     caps.values(), piece.cut_uv[2 * side], "cap normal angular sector changed");
    physical_site(*construction, source_sites[0], cap.pole_boundary_node);
    for (uint32_t side = 0; side < 2; ++side) {
      visit();
      const auto point = piece.cut_points[side];
      require(contains(cap.points, point, caps.points().size()) &&
                  point < construction->cap_point_bindings.size() &&
                  construction->cap_point_bindings[point].construction_site == source_sites[side + 1],
              "cap work cut point/site binding mismatch");
      const auto &plan = caps.points()[point];
      for (uint32_t axis = 0; axis < 2; ++axis) {
        same_parameter(budget, construction->values,
                       construction->sites[source_sites[side + 1]].uv[axis], caps.values(),
                       plan.uv[axis], "cap construction cut UV changed");
        same_parameter(budget, caps.values(), plan.uv[axis], caps.values(),
                       piece.cut_uv[2 * side + axis], "cap piece differs from cut point plan");
      }
      if (const auto *physical = std::get_if<PhysicalCapEndpoint>(&plan.authority)) {
        require(construction->cap_point_bindings[point].sample_row == UINT32_MAX,
                "physical cap cut has a private sampler row");
        require(physical->sector == wedge.meridian_uses[0] ||
                    physical->sector == wedge.meridian_uses[1],
                "cap cut physical sector differs from source wedge");
        physical_cut(*construction, source_sites[side + 1], *physical, caps, budget);
      } else
        surface_cut(*construction, source_sites[side + 1], point,
                    std::get<SurfaceCapPoint>(plan.authority), caps, budget);
    }
    const auto source_key = std::pair{piece.original_root.value, wedge.kind};
    auto found = prepared_index.find(source_key);
    if (found == prepared_index.end()) {
      if (prepared.size() >= limits.pieces || prepared.size() >= UINT32_MAX)
        throw Error(ErrorCode::resource_exhausted, "cap work source descriptor limit");
      const auto id = static_cast<uint32_t>(prepared.size());
      prepared.push_back(
          prepare_cap_source(surface.source(), piece.original_root, wedge.kind, budget));
      found = prepared_index.emplace(source_key, id).first;
    }
    if (entries > limits.exact_entries || limits.exact_entries - entries < 6)
      throw Error(ErrorCode::resource_exhausted, "cap work exact entry limit");
    CapFacetRecord record{index,
                          mapped.cap,
                          mapped.piece,
                          found->second,
                          {},
                          {},
                          {},
                          piece.positive_chart_roles,
                          piece.regular_sheet_sign,
                          false,
                          {cap.transcendental_bits, cap.norm_fractional_bits}};
    for (uint32_t i = 0; i < 4; ++i)
      record.cut_uv[i] = exact.append(read(budget, caps.values(), piece.cut_uv[i]));
    for (uint32_t i = 0; i < 2; ++i)
      record.finite_v[i] = exact.append(read(budget, caps.values(), piece.finite_v[i]));
    entries += 6;
    for (uint32_t role = 0; role < 3; ++role)
      std::copy_n(construction->sites[source_sites[role]].actual_xyz.begin(), 3,
                  record.actual_xyz.begin() + 3 * role);
    record.physical_nonzero = actual_facet_nonzero(record.actual_xyz, budget);
    records.push_back(record);
  }
  auto data = std::allocate_shared<CapFacetData>(
      RunAllocator<CapFacetData>(resource), std::move(surface), std::move(construction),
      std::move(domain), std::move(allowances), exact.seal(), allowance, limits);
  data->stats = {records.size(), prepared.size(), entries};
  data->facets = std::move(records);
  data->sources = std::move(prepared);
  return CapFacetWork(std::move(data));
}

CapFacetBatch pack_cap_facets(JobContext &context, const CapFacetWork &work,
                              std::span<const uint32_t> indices) {
  admit_numeric_environment();
  const auto &data = CapWorkAccess::get(work);
  auto &budget = context.budget;
  const auto scope = budget.scoped_work(data.limits.work);
  if (indices.size() > data.limits.batch_records)
    throw Error(ErrorCode::resource_exhausted, "cap pack batch limit");
  CapFacetBatch result(work, data.surface.resource().get());
  const auto tau = read(budget, data.values, data.allowance).bounds().lower;
  for (const auto index : indices) {
    budget.charge(1);
    require(index < data.facets.size(), "cap pack foreign work facet");
    const auto &record = data.facets[index];
    CapFacetTask task{data.sources[record.prepared_source].numeric(), {}};
    task.input.original = task.source.original;
    for (uint32_t i = 0; i < 4; ++i) {
      const auto value = read(budget, data.values, record.cut_uv[i]).bounds();
      task.input.cut_uv[i] = {value.lower, value.upper};
    }
    for (uint32_t i = 0; i < 2; ++i) {
      const auto value = read(budget, data.values, record.finite_v[i]).bounds();
      task.input.finite_v[i] = {value.lower, value.upper};
    }
    std::copy(record.actual_xyz.begin(), record.actual_xyz.end(), task.input.actual_xyz);
    task.input.allowance_down = std::max(0., tau);
    result.indices.push_back(index);
    result.inputs.push_back(task);
  }
  return result;
}
PreciseCapResult compare_cap_work_facet_precise(JobContext &context, const CapFacetWork &work,
                                                uint32_t index) {
  const auto &data = CapWorkAccess::get(work);
  auto &budget = context.budget;
  const auto scope = budget.scoped_work(data.limits.work);
  require(index < data.facets.size() && data.facets[index].physical_nonzero,
          "precise cap work requires a nonzero captured facet");
  const auto &row = data.facets[index];
  const std::array cut{
      read(budget, data.values, row.cut_uv[0]), read(budget, data.values, row.cut_uv[1]),
      read(budget, data.values, row.cut_uv[2]), read(budget, data.values, row.cut_uv[3])};
  const std::array finite{read(budget, data.values, row.finite_v[0]),
                          read(budget, data.values, row.finite_v[1])};
  auto result =
      compare_cap_facet_precise(data.sources[row.prepared_source], cut, finite, row.actual_xyz,
                                read(budget, data.values, data.allowance), row.precision, budget);
  require(result.bounds.facet_roles == row.positive_chart_roles &&
              result.bounds.regular_sheet_sign == row.regular_sheet_sign,
          "precise cap work changed captured map roles");
  return result;
}
} // namespace bgmesh::adaptive
