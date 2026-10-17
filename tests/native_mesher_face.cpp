#include "../native/mesher/face_engine.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
enum class SeedFault { none, dropped_use, reordered_use, reversed_spatial_t };
FaceBoundarySnapshot seed(bgfd::Budget &budget, const std::shared_ptr<RunResource> &resource,
                          const GeometryStore &geometry, SeedFault fault = SeedFault::none) {
  ExactBuilder builder(resource, {{OwnerKind::face, 0, 900}, 4, 5});
  const std::array<Vec2, 4> points{{{0, 0}, {1, 0}, {1, 1}, {0, 1}}};
  std::array<std::array<ExactRef, 2>, 4> coordinates{};
  for (std::size_t i = 0; i < points.size(); ++i)
    for (std::size_t axis = 0; axis < 2; ++axis)
      coordinates[i][axis] = builder.append(Rational::from_double(budget, points[i][axis]));
  const auto zero = builder.append(Rational(budget)), one = builder.append(Rational(budget, 1));
  auto result = std::allocate_shared<FaceBoundarySnapshotData>(
      RunAllocator<FaceBoundarySnapshotData>(resource), resource, geometry, Slot<DefinitionTag>{0},
      Slot<FaceTag>{0}, std::array<uint64_t, 3>{2, 4, 7}, WorkingChartKind::original_uv,
      builder.seal());
  const auto &source = geometry.view().source.view();
  for (uint32_t i = 0; i < 4; ++i) {
    result->sites.push_back(
        {coordinates[i],
         coordinates[i],
         {{0, NodeGenerator::original_vertex, source.vertices[i].id.value, 0, 0},
          source.vertices[i].xyz,
          {}}});
    result->segments.push_back({i, {source.edges[i].id.value, i}, {i}, {zero, one}, {0, 0}});
    result->uses.push_back({{i}, FaceUseKind::ordinary_chain, {i, 1}, {zero, one}});
  }
  result->loops.push_back({{0}, true, {0, 4}, {0, 4}});
  if (fault == SeedFault::dropped_use) {
    result->uses.pop_back();
    --result->loops[0].uses.count;
  }
  if (fault == SeedFault::reordered_use)
    std::swap(result->uses[0], result->uses[1]);
  if (fault == SeedFault::reversed_spatial_t)
    result->segments[0].source_parameter = {one, zero};
  return result;
}
template <class T> T required(std::optional<T> value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
  return *value;
}
template <std::size_t N, std::size_t... I>
std::array<Rational, N> read_refs(bgfd::Budget &budget, const ExactTable &values,
                                  const std::array<ExactRef, N> &refs, std::index_sequence<I...>) {
  return {import_exact(budget, values.view(refs[I]))...};
}
struct EvaluationChoice final {
  uint32_t table_ordinal;
  std::size_t cell = 0;
};
CellBoundResult evaluate(bgfd::Budget &budget, const std::shared_ptr<RunResource> &resource,
                         const BoundRootBank &bank, const FaceCellWork &work,
                         EvaluationChoice choice) {
  const auto &input = work->cells.at(choice.cell);
  const auto uv = read_refs(budget, work->values, input.original_uv, std::make_index_sequence<6>{});
  const auto xyz = read_refs(budget, work->values, input.actual_xyz, std::make_index_sequence<9>{});
  const auto tau = import_exact(budget, work->values.view(input.allowance));
  const ActualTriangleView triangle{input.header, input.carrier, input.map_kind, uv, xyz, tau};
  const auto prepared =
      prepare_actual_cell(budget, resource, bank, triangle,
                          {{OwnerKind::face, 0, 900}, 4, choice.table_ordinal}, {64, {16, 1}});
  BoundRuntime runtime(resource, bank, {16, 1, 1ULL << 24, 1ULL << 32, 1ULL << 35});
  CellBoundFold fold(resource, prepared);
  for (uint32_t first = 0; first < prepared.pieces().size(); first += 16)
    fold.add(runtime.resolve(runtime.filter_cpu(prepared.batch(
        {first, std::min(16u, static_cast<uint32_t>(prepared.pieces().size()) - first)}))));
  return fold.result();
}
void optimizer(const std::shared_ptr<RunResource> &resource, const GeometryStore &geometry) {
  bgfd::Budget budget({1ULL << 28, 1ULL << 35}, resource.get());
  GeometryContext context(budget);
  const Rational one(budget, 1), zero(budget), tau(budget, 8);
  const std::array<BoundRootSelection, 1> roots{SurfaceRootSlot{0}};
  const auto bank =
      prepare_bound_roots(budget, resource, geometry, roots, {{OwnerKind::geometry, 0, 2}, 2, 0});
  uint32_t improved = 0;
  std::optional<FaceResult> retained;
  for (const int64_t shear : {-1, 1}) {
    const std::array factor{one, Rational(budget, shear), zero, one};
    FaceEngine engine(budget, context, seed(budget, resource, geometry),
                      {factor, {one, one, one, tau}}, tau, 17);
    while (const auto work = engine.take_cell_work(1)) {
      const auto result = evaluate(budget, resource, bank, *work, {200});
      require(result.decision == BoundDecision::accept && engine.complete(result),
              "coarse optimizer parent did not pass actual numerical bound");
    }
    require(engine.advance().kind == FaceAdvanceKind::settled, "optimizer parent not settled");
    const FaceFrontierIdentity identity{{OwnerKind::face, 0, 900}, 4, 7};
    ContactEpochOwner contacts(budget, resource, geometry.view().source, {0}, {32});
    const std::array identities{identity};
    const auto epoch = contacts.begin(identities);
    auto delta = std::allocate_shared<ContactFaceDeltaData>(
        RunAllocator<ContactFaceDeltaData>(resource), resource, identity);
    for (const auto handle : engine.topology().triangles()) {
      ContactFacetInput input{{{handle.slot}, handle.generation},
                              {identity, handle, 0},
                              {0, {}, {}},
                              engine.declared_edges(handle)};
      const auto sites = engine.topology().triangle(handle).sites;
      for (std::size_t c = 0; c < 3; ++c) {
        input.geometry.nodes[c] = {sites[c].value};
        input.geometry.xyz[c] = engine.node(sites[c]).xyz;
        input.node_keys[c] = engine.node(sites[c]).key;
      }
      delta->inserted.push_back(input);
    }
    contacts.arrive(epoch, delta);
    contacts.prepare(epoch);
    while (const auto work = contacts.take_batch(epoch, 8))
      require(contacts.complete(classify_contact_batch(budget, *work)),
              "initial optimizer contact child stale");
    require(contacts.finish(epoch).empty(), "initial optimizer facets have forbidden contacts");
    const auto edges = engine.begin_optimizer();
    require(edges.size() == 1, "square optimizer inventory is not local single edge");
    const auto key = engine.optimizer_edge_key(edges.front());
    const auto work = engine.prepare_flip(edges.front());
    if (!work)
      continue;
    ++improved;
    require((*work)->cells->cells.size() == 2 && key != 0,
            "optimizer proposal lacks two actual cell packets");
    const auto &headers = (*work)->cells->cells;
    require((headers[0].header.key.cell >> 63) == 1 &&
                headers[0].header.key.cell != headers[1].header.key.cell,
            "optimizer temporary keys collide with actual triangle slots");
    require(!engine.complete({headers[0].header, BoundDecision::accept}),
            "ordinary completion accepted optimizer temporary key");
    engine.reject_flip((*work)->ordinal);
    require(engine.optimizer_edge_key(edges.front()) == key,
            "cancelled optimizer changed connectivity");
    const auto again =
        required(engine.prepare_flip(edges.front()), "rejected flip could not be reproposed");
    const std::array answers{evaluate(budget, resource, bank, again->cells, {201, 0}),
                             evaluate(budget, resource, bank, again->cells, {202, 1})};
    require(answers[0].decision == BoundDecision::accept &&
                answers[1].decision == BoundDecision::accept,
            "optimizer proposed actual cells did not pass");
    std::array<ContactProposedFacet, 2> proposed{};
    for (std::size_t i = 0; i < 2; ++i) {
      proposed[i].geometry.definition = 0;
      proposed[i].edges = again->edges[i];
      for (std::size_t c = 0; c < 3; ++c) {
        proposed[i].geometry.nodes[c] = {again->sites[i][c].value};
        proposed[i].geometry.xyz[c] = again->nodes[i][c].xyz;
        proposed[i].node_keys[c] = again->nodes[i][c].key;
      }
    }
    const std::array old{
        FacetHandle{{again->old_cells[0].triangle.slot}, again->old_cells[0].triangle.generation},
        FacetHandle{{again->old_cells[1].triangle.slot}, again->old_cells[1].triangle.generation}};
    const auto approval = required(contacts.approve_replacement(again->ordinal, old, proposed),
                                   "optimizer proposal contact rejected");
    require(contacts.approval_current(approval), "optimizer approval stale before sole commit");
    auto stale_answers = answers;
    --stale_answers[0].header.face_generation;
    bool rejected = false;
    try {
      (void)engine.complete_flip(again->ordinal, stale_answers, approval);
    } catch (const Error &error) {
      rejected = error.code == ErrorCode::invalid_source;
    }
    require(rejected && engine.topology().is_live(edges.front()),
            "stale optimizer proof mutated connectivity");
    const auto changed = engine.complete_flip(again->ordinal, answers, approval);
    std::array<ContactFacetInput, 2> committed{};
    for (std::size_t i = 0; i < 2; ++i)
      committed[i] = {{{changed.cells[i].triangle.slot}, changed.cells[i].triangle.generation},
                      changed.cells[i],
                      proposed[i].geometry,
                      proposed[i].edges,
                      proposed[i].node_keys};
    contacts.commit_replacement(approval, committed);
    require(!contacts.approval_current(approval),
            "accepted face flip did not retire contact scene approval");
    require(changed.affected_edges.size() == 1 && !engine.topology().is_live(edges.front()),
            "optimizer missed local replacement edge/stale old handle");
    require(engine.advance().kind == FaceAdvanceKind::settled && !engine.take_cell_work(8),
            "accepted flip unnecessarily queued duplicate numerical work");
    require(!engine.prepare_flip(changed.affected_edges.front()),
            "strict once-rounded potential allowed immediate inverse flip");
    const auto frozen = engine.freeze_result();
    require(frozen->sites.size() == 4 && frozen->triangles.size() == 2 &&
                frozen->boundary == again->boundary &&
                frozen->values.key().ordinal > again->cells->values.key().ordinal,
            "final face export dropped source/coordinates/live topology");
    require(engine.freeze_result() == frozen, "repeat freeze did not reuse immutable lease");
    require(frozen->source_sites.size() == frozen->boundary->sites.size(),
            "final result omitted source occurrence to active topology mapping");
    for (std::size_t i = 0; i < frozen->source_sites.size(); ++i)
      require(frozen->source_sites[i] && frozen->source_sites[i]->value == i,
              "ordinary source site mapping changed active topology identity");
    for (const auto &triangle : frozen->triangles) {
      const auto &ancestry = frozen->lineages.at(static_cast<std::size_t>(triangle.lineage));
      require(ancestry.operation == LineageOperation::flip && ancestry.parent < triangle.lineage &&
                  ancestry.secondary_parent < triangle.lineage &&
                  ancestry.parent != ancestry.secondary_parent,
              "flipped child lost either retired-parent ancestry");
    }
    bool mutation_rejected = false;
    try {
      (void)engine.take_cell_work(1);
    } catch (const Error &error) {
      mutation_rejected = error.code == ErrorCode::invalid_source;
    }
    require(mutation_rejected, "frozen result allowed later mutable work");
    retained = frozen;
  }
  require(improved != 0,
          "two opposite initial metrics never exercised strict optimizer improvement");
  if (!retained)
    throw std::runtime_error("final result lease absent");
  const auto &frozen = **retained;
  require(import_exact(budget, frozen.values.view(frozen.sites.front().chart[0])).sign() == 0,
          "sealed result depended on destroyed face Budget/topology");
  std::cout << "native fixed-site optimizer: actual bounds/contact/strict potential PASS\n";
}
void run(const std::shared_ptr<RunResource> &resource, const GeometryStore &geometry) {
  bgfd::Budget budget({1ULL << 28, 1ULL << 35}, resource.get());
  GeometryContext context(budget);
  const auto boundary = seed(budget, resource, geometry);
  const Rational one(budget, 1), tau = one.divide(Rational(budget, 64));
  const std::array factor{one, Rational(budget), Rational(budget), one};
  for (const auto fault :
       {SeedFault::dropped_use, SeedFault::reordered_use, SeedFault::reversed_spatial_t}) {
    bool rejected = false;
    try {
      FaceEngine invalid_seed(budget, context, seed(budget, resource, geometry, fault),
                              {factor, {one, one, one, tau}}, tau, 17);
    } catch (const Error &e) {
      rejected = e.code == ErrorCode::invalid_source;
    }
    require(rejected, "selected polygon hid dropped/reordered/original-t source use");
  }
  FaceEngine engine(budget, context, boundary, {factor, {one, one, one, tau}}, tau, 17);
  const auto packet = required(engine.take_cell_work(1), "bounded initial cell work missing");
  require(packet->cells.size() == 1 && packet->values.key().ordinal == 6,
          "cell capacity or final-seed table ordinal continuation failed");
  const auto &input = packet->cells[0];
  const std::array<BoundRootSelection, 1> roots{SurfaceRootSlot{0}};
  const auto bank =
      prepare_bound_roots(budget, resource, geometry, roots, {{OwnerKind::geometry, 0, 1}, 2, 0});
  const auto first_answer = evaluate(budget, resource, bank, packet, {100});
  require(first_answer.decision == BoundDecision::bound_fail,
          "authored tight weighted patch did not fail actual full-cell bound");
  require(input.header.source_generation == 2 && input.header.face_generation == 4 &&
              input.header.boundary_generation == 7 && input.header.request_revision == 0 &&
              input.header.bound_revision == 1,
          "cell policy/request/owner counters collided");
  auto stale = input.header;
  --stale.face_generation;
  require(!engine.complete({stale, BoundDecision::accept}), "stale rebuilt-face result accepted");
  require(engine.complete(first_answer), "current actual bound rejected");
  require(!engine.complete({input.header, BoundDecision::accept}), "duplicate answer accepted");
  // Work and semantic priority are separate: dispatch the remaining initial
  // cell, then return exact parent answers in the reverse order.
  const auto second = required(engine.take_cell_work(1), "second initial cell missing");
  require(second->cells.size() == 1 && !engine.take_cell_work(1),
          "pending delta stream duplicates work");
  require(engine.complete(evaluate(budget, resource, bank, second, {101})),
          "second actual exact bound rejected");
  const auto next = engine.advance();
  require(next.kind == FaceAdvanceKind::candidate,
          "authored weighted patch did not exercise staged candidate refinement");
  if (next.kind == FaceAdvanceKind::candidate) {
    require(static_cast<bool>(next.candidate), "candidate lacks immutable coordinates");
    const auto &work = *next.candidate;
    const auto u = import_exact(budget, work.values.view(work.original_uv[0]));
    const auto v = import_exact(budget, work.values.view(work.original_uv[1]));
    const auto point = canonical_original_surface(context, geometry, {0}, u, v);
    // Authored square fixture: this interior candidate has no encroached
    // nonincident feature. Product admission is a geometry-owned child job.
    const FaceCandidateResult result{
        work.target, work.ordinal, FaceCandidateDecision::admitted, point.xyz, {}};
    require(engine.complete_candidate(result), "current candidate rejected");
    require(!engine.complete_candidate(result), "candidate completion committed twice");
    const auto edits = engine.take_changes();
    require(!edits.empty() && edits.front().new_site, "local committed edit missing");
    const auto &node =
        engine.node(required(edits.front().new_site, "local committed site missing"));
    require(node.xyz == point.xyz && node.key.generator == NodeGenerator::face_site &&
                node.key.generation == 4,
            "committed node lost common evaluator/owner identity");
    require(engine.take_cell_work(8).has_value(), "inserted triangles not queued locally");
  }
  std::cout << "native face engine: ordinary immutable work/refinement PASS\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected original geometry fixture");
    auto resource = std::make_shared<bgmesh::RunResource>(1ULL << 30);
    const auto source = bgmesh::load_source(argv[1], resource);
    bgfd::Budget budget({1ULL << 28, 1ULL << 35}, resource.get());
    run(resource, bgmesh::compile_geometry(source, resource,
                                           {{bgmesh::OwnerKind::geometry, 0, 0}, 2, 0}, budget));
    optimizer(resource, bgmesh::compile_geometry(
                            source, resource, {{bgmesh::OwnerKind::geometry, 0, 0}, 2, 0}, budget));
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
