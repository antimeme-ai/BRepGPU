#include "../native/mesher/assembly.hpp"
#include "../native/mesher/face_engine.hpp"
#include "../native/mesher/face_preparation.hpp"
#include "../native/mesher/geometry_candidate.hpp"
#include "../native/mesher/geometry_domain.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void check(const std::filesystem::path &path, bool curved, bool tight) {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
  const auto source = load_source(path, resource);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 36}, resource.get());
  const auto geometry =
      compile_geometry(source, resource, {{OwnerKind::geometry, 0, 135}, 0, 0}, b);
  GeometryContext context(b);
  DefinitionBoundaryForest forest(b, source, {0}, resource);
  if (const auto pending = forest.pending_evaluations()) {
    std::pmr::vector<BoundaryEvaluationResult> results(&b);
    for (const auto &task : pending->tasks()) {
      const auto t = import_exact(b, pending->parameters().view(task.parameter));
      const auto p = canonical_original_curve(context, geometry, task.curve, t);
      results.push_back({task, p.dimension, p.xyz});
    }
    static_cast<void>(forest.complete_evaluations(pending->epoch(), results));
  }
  const auto original = forest.freeze_face_boundary({0});
  const auto tau = tight ? Rational(b, 1).divide(Rational(b, 200)) : Rational(b, 1);
  FaceBoundaryPreparation prep(
      b, context, geometry, original,
      {.face = {0},
       .generation = 17,
       .positional_allowance = tau,
       .node_source_allowance = Rational(b, 1).divide(Rational(b, 1000000)),
       .allow_plane_profile = false});
  require((prep.state() == FacePreparationState::curve_work) == curved,
          "only nontrivial original curve chords need numerical runtime work");
  if (curved) {
    bool rejected = false;
    try {
      static_cast<void>(prep.snapshot());
    } catch (const Error &e) {
      rejected = e.code == ErrorCode::invalid_source;
    }
    require(rejected, "snapshot cannot bypass outstanding actual chord bounds");
    const auto roots = prep.curve_roots();
    const auto bank =
        prepare_bound_roots(b, resource, geometry, roots, {{OwnerKind::geometry, 0, 135}, 0, 1});
    BoundRuntime runtime(resource, bank,
                         {8, 8, uint64_t{1} << 24, uint64_t{1} << 34, uint64_t{1} << 36});
    while (const auto work = prep.take_curve_work(bank, {1, 1}))
      prep.complete(runtime.resolve(runtime.filter_cpu(*work)));
    runtime.close();
  }
  if (tight) {
    require(prep.state() == FacePreparationState::boundary_request &&
                prep.boundary_request().has_value(),
            "actual failed chord requests canonical shared edge before any face seed");
  } else {
    require(prep.state() == FacePreparationState::ready,
            "complete source geometry and keyed chord fold settle seed");
    const auto seed = prep.snapshot();
    require(seed->protection != nullptr, "settled seed retains its source feature index");
    require(seed->face_generation == 17 &&
                seed->boundary_generation == original->boundary_generation &&
                seed->source_generation == geometry.view().table.key().generation,
            "source, face and boundary generations retain independent meanings");
    require(seed->sites.size() == 4 && seed->segments.size() == 4 && seed->uses.size() == 4 &&
                seed->loops.size() == 1 && seed->trace_maps.size() == 4,
            "complete original sites, intervals, uses, loops and finite trace maps retained");
    require(prep.snapshot().get() == seed.get(),
            "immutable settled seed is published once and copied on repeat");
    const auto definition =
        prepare_definition_envelope(context, geometry, {.definition = {0}, .generation = 17});
    const auto metric = prepare_face_metric(context, seed, {source, {0}, definition.xyz}, tau);
    const std::array<Rational, 4> factor{import_exact(b, metric.values.view(metric.factor[0])),
                                         import_exact(b, metric.values.view(metric.factor[1])),
                                         import_exact(b, metric.values.view(metric.factor[2])),
                                         import_exact(b, metric.values.view(metric.factor[3]))};
    const std::array<Rational, 4> scales{import_exact(b, metric.values.view(metric.scales[0])),
                                         import_exact(b, metric.values.view(metric.scales[1])),
                                         import_exact(b, metric.values.view(metric.scales[2])),
                                         import_exact(b, metric.values.view(metric.scales[3]))};
    require(
        scales[0].compare(Rational(b, curved ? 3 : 2)) == 0 &&
            scales[1].compare(Rational(b, 1)) == 0 && scales[2].compare(Rational(b, 1)) == 0 &&
            scales[3].compare(tau) == 0,
        "sealed metric uses complete hand-derived original bbox and actual chart widths/allowance");
    require(metric.values.key().ordinal == 0 &&
                metric.values.key().owner == seed->values.key().owner,
            "fixed metric occupies reserved ordinal0, independent of later prep and face packets");
    require(import_exact(b, seed->values.view(seed->prepared_original_rectangle[1]))
                    .compare(Rational(b, 1)) == 0,
            "semantic chart rectangle retains original natural bounds, not adapter hint");
    FaceEngine engine(b, context, seed, {factor, {scales[0], scales[1], scales[2], scales[3]}}, tau,
                      23);
    require(engine.topology().triangles().size() == 2,
            "actual prepared seed is consumed by complete-source admission and direct CDT");
    const auto cells = engine.take_cell_work(8);
    require(cells && (*cells)->values.key().owner == seed->values.key().owner &&
                (*cells)->values.key().generation == seed->face_generation &&
                (*cells)->values.key().ordinal > seed->values.key().ordinal,
            "actual consumer continues prep table namespace above final seed, without aliased "
            "ExactRefs");
  }
}
void authoritative(const std::filesystem::path &path,bool curved,bool mixed) {
 const auto resource=std::make_shared<RunResource>(uint64_t{1}<<28);
 const auto source=load_source(path,resource);bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<36},resource.get());
 const auto geometry=compile_geometry(source,resource,{{OwnerKind::geometry,0,145},0,0},b);GeometryContext ctx(b);
 DefinitionBoundaryForest forest(b,source,{0},resource);
 require(!forest.pending_evaluations(),"authored whole source has only original endpoints");
 FaceBoundaryPreparation prep(b,ctx,geometry,forest.freeze_face_boundary({0}),{.face={0},.generation=21,
  .positional_allowance=Rational(b,1),.node_source_allowance=Rational(b,1).divide(Rational(b,1000000))});
 const auto roots=prep.curve_roots();
 const auto bank=prepare_bound_roots(b,resource,geometry,roots,{{OwnerKind::geometry,0,145},0,1});
 BoundRuntime runtime(resource,bank,{8,8,uint64_t{1}<<24,uint64_t{1}<<34,uint64_t{1}<<36});
 while(const auto work=prep.take_curve_work(bank,{8,8}))prep.complete(runtime.resolve(runtime.filter_cpu(*work)));
 require(prep.state()==FacePreparationState::ready,"whole authoritative producer settles actual source/chord fold");
 const auto seed=prep.snapshot();
 require(seed->chart_kind==WorkingChartKind::original_uv&&seed->domain_map,
  "authoritative source retains actual map after optional plane fallback, never relabeled reconstructed");
 require(seed->domain_map->strips.size()==(curved?1U:0U)&&seed->domain_map->fans.empty(),
  "same producer retains actual curved strip or named identity authority");
 require(seed->domain_map->policy==(mixed?DomainReferencePolicy::hybrid_pullback:DomainReferencePolicy::authoritative_original),
  "same producer preserves mixed per-use authority");
 require(seed->protection!=nullptr,"authoritative seed prepares actual physical feature protection");
 const auto z=Rational(b,0),o=Rational(b,1);
 FaceEngine engine(b,ctx,seed,{{o,z,z,o},{Rational(b,2),o,o,o}},o,23);
 const auto work=engine.take_cell_work(8);
 require(work&&(*work)->cells.size()==2,"actual authoritative seed enters same direct CDT/cell engine");
 for(const auto &cell:(*work)->cells) {
  const auto allowance=import_exact(b,(*work)->values.view(cell.allowance));
  require(allowance.compare(curved?o.subtract(o.divide(Rational(b,32))):o)==0,
   "engine subtracts actual source-domain displacement exactly once before numeric bound");
 }
 runtime.close();
}
void tiny_hole(const std::filesystem::path &path) {
 const auto resource=std::make_shared<RunResource>(uint64_t{1}<<28);
 const auto source=load_source(path,resource);bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<38},resource.get());
 const auto geometry=compile_geometry(source,resource,{{OwnerKind::geometry,0,146},0,0},b);GeometryContext ctx(b);
 DefinitionBoundaryForest forest(b,source,{0},resource);
 const auto evaluate=[&](const BoundaryEvaluationBatch &batch) {
  std::pmr::vector<BoundaryEvaluationResult> results(&b);
  for(const auto &task:batch.tasks()) {
   const auto t=import_exact(b,batch.parameters().view(task.parameter));
   const auto p=canonical_original_curve(ctx,geometry,task.curve,t);results.push_back({task,p.dimension,p.xyz});
  }
  static_cast<void>(forest.complete_evaluations(batch.epoch(),results));
 };
 if(const auto initial=forest.pending_evaluations())evaluate(*initial);
 bool settled=false;
 for(uint32_t epoch=0;epoch<8;++epoch) {
  FaceBoundaryPreparation prep(b,ctx,geometry,forest.freeze_face_boundary({0}),{.face={0},.generation=30+epoch,
   .positional_allowance=Rational(b,1),.node_source_allowance=Rational(b,1).divide(Rational(b,1000000)),.allow_plane_profile=false});
  const auto roots=prep.curve_roots();
  const auto bank=prepare_bound_roots(b,resource,geometry,roots,{{OwnerKind::geometry,0,146},0,epoch+1});
  BoundRuntime runtime(resource,bank,{16,16,uint64_t{1}<<24,uint64_t{1}<<34,uint64_t{1}<<36});
  while(const auto work=prep.take_curve_work(bank,{16,16}))prep.complete(runtime.resolve(runtime.filter_cpu(*work)));
  runtime.close();
  if(epoch==0)require(prep.state()==FacePreparationState::boundary_request,
   "tiny nonconstant closed original cubic must refine topology even when chord error is below epsilon");
  if(prep.state()==FacePreparationState::ready) {
   const auto seed=prep.snapshot();require(seed->loops.size()==2&&seed->loops[1].segments.count>=3,
    "source-driven master splits preserve complete small hole, never delete or tolerance-weld it");
   const auto z=Rational(b,0),o=Rational(b,1);
   FaceEngine engine(b,ctx,seed,{{o,z,z,o},{Rational(b,2),o,o,o}},o,23);
   require(engine.topology().triangles().size()>=7,"same direct CDT consumes actual independently sized closed hole");
   settled=true;break;
  }
  const auto requested=prep.boundary_request();require(requested&&requested->owner==404,
    "coarse source loop requests its own original spatial interval");
  const auto next=forest.begin_epoch();const std::array keys{*requested};
  forest.arrive(next,{0},forest.face_generation({0}),keys);evaluate(forest.prepare_splits(next));
 }
 require(settled,"bounded source-driven small hole reaches usable ordinary topology");
}
void candidates(const std::filesystem::path &path, bool plane = false) {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
  const auto source = load_source(path, resource);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 36}, resource.get());
  const auto geometry =
      compile_geometry(source, resource, {{OwnerKind::geometry, 0, 136}, 0, 0}, b);
  GeometryContext context(b);
  DefinitionBoundaryForest forest(b, source, {0}, resource);
  if (const auto pending = forest.pending_evaluations()) {
    std::pmr::vector<BoundaryEvaluationResult> results(&b);
    for (const auto &task : pending->tasks()) {
      const auto t = import_exact(b, pending->parameters().view(task.parameter));
      const auto p = canonical_original_curve(context, geometry, task.curve, t);
      results.push_back({task, p.dimension, p.xyz});
    }
    static_cast<void>(forest.complete_evaluations(pending->epoch(), results));
  }
  FaceBoundaryPreparation prep(
      b, context, geometry, forest.freeze_face_boundary({0}),
      {.face = {0},
       .generation = 19,
       .positional_allowance = Rational(b, 1),
       .node_source_allowance = Rational(b, 1).divide(Rational(b, 1000000)),
       .allow_plane_profile = plane});
  const auto seed = prep.snapshot();
  for(const auto &site:seed->sites) require(site.original_uv_enclosure.has_value()==plane,
    "actual plane inverse enclosure survives producer transport; UV stays direct parameter");
  const OwnerKey owner{OwnerKind::face, 0, source.view().faces[0].id.value};
  const auto candidate = [&](int64_t y, uint64_t ordinal) {
    ExactBuilder values(
        resource, {owner, 19, static_cast<uint32_t>(seed->values.key().ordinal + 2 * ordinal)});
    const std::array<ExactRef, 2> uv{values.append(Rational(b, 1).divide(Rational(b, 2))),
                                     values.append(Rational(b, y).divide(Rational(b, 4)))};
    auto work = std::allocate_shared<FaceCandidateData>(
        RunAllocator<FaceCandidateData>(resource),
        FaceCandidateData{
            seed,
            {{owner, 19, seed->boundary_generation}, {0, 0}, 0},
            ordinal,
            FaceCandidateKind::heuristic,
            values.seal(),
            uv,
            uv,
            plane ? std::optional<ExactTableKey>{{owner, 19,
                                                  static_cast<uint32_t>(seed->values.key().ordinal +
                                                                        2 * ordinal + 1)}}
                  : std::nullopt});
    return evaluate_geometry_candidate(context, work);
  };
  const auto center = candidate(2, 1);
  require(center.decision == FaceCandidateDecision::admitted && center.xyz == Vec3{.5, .5, 0.} &&
              !center.feature,
          "strict exact diametral test admits square center on four circle boundaries");
  if (plane) {
    require(center.locator.has_value(), "plane candidate returns actual original inverse locator");
    const auto &l = *center.locator;
    const auto lo = import_exact(b, l.values.view(l.enclosure[0])),
               hi = import_exact(b, l.values.view(l.enclosure[1]));
    const auto f = [&](const Rational &u) {
      return u.multiply(Rational(b, 3)).subtract(u.multiply(u)).divide(Rational(b, 2));
    };
    require(f(lo).compare(Rational(b, 1).divide(Rational(b, 2))) <= 0 &&
                f(hi).compare(Rational(b, 1).divide(Rational(b, 2))) >= 0,
            "returned plane locator enclosure brackets independent POWER inverse");
  }
  const auto near_edge = candidate(1, 2);
  require(near_edge.decision == FaceCandidateDecision::protected_feature &&
              near_edge.xyz == Vec3{.5, .25, 0.} && near_edge.feature &&
              near_edge.feature->owner == 400,
          "exact dot((p-a),(p-b))=-3/16 requests original bottom edge");
  auto unprotected = std::allocate_shared<FaceBoundarySnapshotData>(
      RunAllocator<FaceBoundarySnapshotData>(resource), resource, geometry, seed->definition,
      seed->face,
      std::array<uint64_t, 3>{seed->source_generation, seed->face_generation,
                              seed->boundary_generation},
      seed->chart_kind, seed->values);
  ExactBuilder missing_values(resource, {owner, 19, seed->values.key().ordinal + 20});
  const std::array<ExactRef, 2> missing_uv{
      missing_values.append(Rational(b, 1).divide(Rational(b, 2))),
      missing_values.append(Rational(b, 1).divide(Rational(b, 2)))};
  const auto unprotected_work = std::allocate_shared<FaceCandidateData>(
      RunAllocator<FaceCandidateData>(resource),
      FaceCandidateData{unprotected,
                        {{owner, 19, seed->boundary_generation}, {0, 0}, 0},
                        20,
                        FaceCandidateKind::heuristic,
                        missing_values.seal(),
                        missing_uv,
                        missing_uv});
  bool refused = false;
  try {
    static_cast<void>(evaluate_geometry_candidate(context, unprotected_work));
  } catch (const Error &error) {
    refused = error.code == ErrorCode::unsupported;
  }
  require(refused, "missing source feature protection never implies permission");
  // The index uses actual spatial coordinates rather than a UV-circle proxy.
  require(encroached_boundary(b, *seed->protection, {.5, .5, 10.}) == std::nullopt,
          "off-plane candidate outside physical feature balls is not a UV false positive");
}

void plane_fallback(const std::filesystem::path &path) {
 const auto resource=std::make_shared<RunResource>(uint64_t{1}<<28);
 const auto source=load_source(path,resource);
 bgfd::Budget b({uint64_t{1}<<27,uint64_t{1}<<36},resource.get());
 const auto geometry=compile_geometry(source,resource,{{OwnerKind::geometry,0,141},0,0},b);
 GeometryContext context(b);DefinitionBoundaryForest forest(b,source,{0},resource);
 require(!forest.pending_evaluations(),"authored source has no mandatory interior map breaks");
 FaceBoundaryPreparation prep(b,context,geometry,forest.freeze_face_boundary({0}),{.face={0},.generation=20,
  .positional_allowance=Rational(b,1),.node_source_allowance=Rational(b,1).divide(Rational(b,1000000))});
 const auto roots=prep.curve_roots();
 const auto bank=prepare_bound_roots(b,resource,geometry,roots,{{OwnerKind::geometry,0,141},0,1});
 BoundRuntime runtime(resource,bank,{8,8,uint64_t{1}<<24,uint64_t{1}<<34,uint64_t{1}<<36});
 while(const auto work=prep.take_curve_work(bank,{8,8}))prep.complete(runtime.resolve(runtime.filter_cpu(*work)));
 runtime.close();
 require(prep.state()==FacePreparationState::ready,"optional plane failure retries originalUV before publication");
 const auto seed=prep.snapshot();
 require(seed->chart_kind==WorkingChartKind::original_uv&&!seed->plane,
  "source-node tolerance does not force a false inverse outside Q");
 for(const auto &site:seed->sites)require(!site.original_uv_enclosure,"fallback UV remains original parameter");
}

} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected authored fixture directory");
    const std::filesystem::path root(argv[1]);
    check(root / "warped-plane.bgsource", false, false);
    check(root / "parabolic-sheet.bgsource", true, false);
    check(root / "parabolic-sheet.bgsource", true, true);
    authoritative(root/"authoritative-joint.bgsource",false,false);
    authoritative(root/"authoritative-curved-trim.bgsource",true,false);
    authoritative(root/"mixed-curved-trim.bgsource",true,true);
    tiny_hole(root/"tiny-closed-cubic-hole.bgsource");
    candidates(root / "affine-plane.bgsource");
    candidates(root / "warped-plane.bgsource", true);
    plane_fallback(root/"plane-exterior-node.bgsource");
    std::cout << "PASS\tactual original face preparation\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
