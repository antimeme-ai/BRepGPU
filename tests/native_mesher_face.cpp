#include "../native/mesher/face_engine.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
FaceBoundarySnapshot seed(bgfd::Budget &budget, const std::shared_ptr<RunResource> &resource,
                          const GeometryStore &geometry) {
  ExactBuilder builder(resource, {{OwnerKind::face, 0, 900}, 4, 0});
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
  }
  result->loops.push_back({{0}, true, {0, 4}});
  return result;
}
template<class T> T required(std::optional<T> value,const char *message) {
  if(!value) throw std::runtime_error(message);
  return *value;
}
template<std::size_t N,std::size_t...I>
std::array<Rational,N> read_refs(bgfd::Budget &budget,const ExactTable &values,
                                 const std::array<ExactRef,N> &refs,std::index_sequence<I...>) {
  return {import_exact(budget,values.view(refs[I]))...};
}
CellBoundResult evaluate(bgfd::Budget &budget,const std::shared_ptr<RunResource> &resource,
                          BoundRootBank bank,const FaceCellWork &work,uint32_t ordinal) {
  const auto &input=work->cells.front();
  const auto uv=read_refs(budget,work->values,input.original_uv,std::make_index_sequence<6>{});
  const auto xyz=read_refs(budget,work->values,input.actual_xyz,std::make_index_sequence<9>{});
  const auto tau=import_exact(budget,work->values.view(input.allowance));
  const ActualTriangleView triangle{input.header,input.carrier,input.map_kind,uv,xyz,tau};
  const auto prepared=prepare_actual_cell(budget,resource,bank,triangle,
      {{OwnerKind::face,0,900},4,ordinal},{64,{16,1}});
  BoundRuntime runtime(resource,bank,{16,1,1ULL<<24,1ULL<<32,1ULL<<35});
  CellBoundFold fold(resource,prepared);
  for(uint32_t first=0;first<prepared.pieces().size();first+=16)
    fold.add(runtime.resolve(runtime.filter_cpu(prepared.batch(
      {first,std::min(16u,static_cast<uint32_t>(prepared.pieces().size())-first)}))));
  return fold.result();
}
void run(const std::shared_ptr<RunResource> &resource, const GeometryStore &geometry) {
  bgfd::Budget budget({1ULL << 28, 1ULL << 35}, resource.get());
  GeometryContext context(budget);
  const auto boundary = seed(budget, resource, geometry);
  const Rational one(budget, 1), tau = one.divide(Rational(budget, 64));
  const std::array factor{one, Rational(budget), Rational(budget), one};
  FaceEngine engine(budget, context, boundary, {factor, {one, one, one, tau}}, tau, 17);
  const auto packet=required(engine.take_cell_work(1),"bounded initial cell work missing");
  require(packet->cells.size()==1,"cell packet exceeded capacity");
  const auto &input=packet->cells[0];
  const std::array<BoundRootSelection,1> roots{SurfaceRootSlot{0}};
  const auto bank=prepare_bound_roots(budget,resource,geometry,roots,
      {{OwnerKind::geometry,0,1},2,0});
  const auto first_answer=evaluate(budget,resource,bank,packet,100);
  require(first_answer.decision==BoundDecision::bound_fail,
          "authored tight weighted patch did not fail actual full-cell bound");
  require(input.header.source_generation == 2 && input.header.face_generation == 4 &&
              input.header.boundary_generation == 7 && input.header.request_revision == 0 &&
              input.header.bound_revision == 1,
          "cell policy/request/owner counters collided");
  auto stale = input.header;
  --stale.face_generation;
  require(!engine.complete({stale, BoundDecision::accept}), "stale rebuilt-face result accepted");
  require(engine.complete(first_answer),"current actual bound rejected");
  require(!engine.complete({input.header, BoundDecision::accept}), "duplicate answer accepted");
  // Work and semantic priority are separate: dispatch the remaining initial
  // cell, then return exact parent answers in the reverse order.
  const auto second=required(engine.take_cell_work(1),"second initial cell missing");
  require(second->cells.size()==1 && !engine.take_cell_work(1),
          "pending delta stream duplicates work");
  require(engine.complete(evaluate(budget,resource,bank,second,101)),
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
    const auto &node=engine.node(required(edits.front().new_site,"local committed site missing"));
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
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
