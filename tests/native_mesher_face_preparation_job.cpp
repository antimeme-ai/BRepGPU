#include "../native/mesher/boundary_evaluation.hpp"
#include "../native/mesher/face_preparation_job.hpp"
#include "../native/mesher/numerical_service.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
StepState execute(NativeScheduler &scheduler, const std::shared_ptr<NativeTask> &task) {
  scheduler.submit(task);
  for (;;) {
    scheduler.drain();
    auto completion = scheduler.take();
    if (!completion || completion->task != task)
      throw std::runtime_error("lost native task completion");
    completion->rethrow();
    if (completion->state != StepState::ready)
      return completion->state;
    scheduler.submit(task);
  }
}
void check(const std::filesystem::path &path, uint32_t threads, bool tight) {
  const auto resource = std::make_shared<RunResource>(1ULL << 30);
  NativeScheduler scheduler(resource, {threads, 16, 1ULL << 27, 1ULL << 38});
  auto run_job = std::allocate_shared<RunPreparationJob>(
      RunAllocator<RunPreparationJob>(resource), resource,
      RunPreparationInput{path, tight ? .005 : 1., .000001});
  require(execute(scheduler, run_job) == StepState::done, "whole run preparation failed");
  const auto run = run_job->result();
  bgfd::Budget owner({1ULL << 26, 1ULL << 36}, resource.get());
  DefinitionBoundaryForest forest(owner, run->geometry.view().source, {0}, resource);
  if (const auto batch = forest.pending_evaluations()) {
    auto child = std::allocate_shared<BoundaryPointJob>(
        RunAllocator<BoundaryPointJob>(resource), resource, run->geometry, *batch,
        Range<BoundaryEvaluationTask>{0, batch->tasks().size()},
        NativeTaskKey{{OwnerKind::boundary, 0, 0}, batch->epoch().value, 0});
    require(execute(scheduler, child) == StepState::done, "shared curve points failed");
    static_cast<void>(forest.complete_evaluations(batch->epoch(), child->results()));
  }
  auto prep = std::allocate_shared<FacePreparationJob>(
      RunAllocator<FacePreparationJob>(resource), resource,
      FacePreparationInput{run, forest.freeze_face_boundary({0}), 17, 23, false},
      FacePreparationLimits{1ULL << 26, 1ULL << 36, {2, 2}}, scheduler.stop_token());
  bool early = false;
  try {
    static_cast<void>(prep->result());
  } catch (const Error &) {
    early = true;
  }
  require(early, "preparation published incomplete seed/metric");
  constexpr NumericalServiceLimits limits{
      {8, 8, 1ULL << 25, 1ULL << 34, 1ULL << 38}, {128, {8, 8}}, 8, {}};
  auto service = std::allocate_shared<NumericalServiceJob>(
      RunAllocator<NumericalServiceJob>(resource), resource,
      NativeTaskKey{{OwnerKind::run, 0, 19}, 0, 0}, run->bank, limits, scheduler.stop_token());
  uint64_t requests = 0;
  for (;;) {
    const auto state = execute(scheduler, prep);
    if (state != StepState::wait_child)
      break;
    require(prep->event().kind == FacePreparationEventKind::curves && prep->event().curves,
            "preparation yielded untyped curve work");
    const auto curves = prep->event().curves;
    if (!curves)
      throw std::runtime_error("missing preparation curve request");
    service->begin({{prep->key.owner, prep->key.generation, ++requests}, *curves});
    require(execute(scheduler, service) == StepState::done, "curve service failed");
    prep->finish_curves(std::get<BoundCompleted>(service->result().output));
    service->release_result();
  }
  require(requests > 0, "authored parabolic boundary never reached real numerical service");
  if (tight) {
    require(prep->event().kind == FacePreparationEventKind::boundary_horizon &&
                prep->event().boundary.has_value(),
            "failed actual chord must request shared edge before producing a face");
  } else {
    require(prep->event().kind == FacePreparationEventKind::ready,
            "loose original geometry did not produce complete face input");
    const auto input = prep->result();
    require(input.boundary->face_generation == 17 && input.metric_values.key().ordinal == 0 &&
                input.boundary->sites.size() == 4,
            "source preparation lost source-owned inventory/metric generation");
    const auto work = prep->take_owner_work();
    require(work.exact_work > 0 && prep->take_owner_work().exact_work == 0,
            "persistent preparation cost was omitted or counted twice");
    auto face = std::allocate_shared<FaceOwnerJob>(
        RunAllocator<FaceOwnerJob>(resource), resource, input,
        FaceOwnerLimits{1ULL << 26, 1ULL << 36, 8}, scheduler.stop_token());
    prep.reset(); // refinement must not retain the mutable preparation arena
    while (execute(scheduler, face) == StepState::wait_child) {
      require(face->event().kind == FaceOwnerEventKind::cells,
              "loose authored face unexpectedly requested refinement");
      service->begin({{face->key.owner, face->key.generation, ++requests}, face->event().cells});
      require(execute(scheduler, service) == StepState::done, "actual-cell service failed");
      face->finish_cells(
          std::get<std::shared_ptr<const CellBatchResult>>(service->result().output)->cells);
      service->release_result();
    }
    require(face->event().kind == FaceOwnerEventKind::settled &&
                face->engine().topology().triangles().size() == 2,
            "prepared seed did not reach actual bounded direct-CDT horizon");
    ExactBuilder values(resource, {input.boundary->values.key().owner, 17, 10000});
    const auto half = values.append(Rational(owner, 1).divide(Rational(owner, 2)));
    const std::array<ExactRef, 2> uv{half, half};
    auto work_packet = std::allocate_shared<FaceCandidateData>(
        RunAllocator<FaceCandidateData>(resource),
        FaceCandidateData{input.boundary,
                          {{face->key.owner, 17, input.boundary->boundary_generation}, {0, 0}, 0},
                          10000,
                          FaceCandidateKind::heuristic,
                          values.seal(),
                          uv,
                          uv});
    auto candidate = std::allocate_shared<CandidatePointJob>(
        RunAllocator<CandidatePointJob>(resource), work_packet);
    require(execute(scheduler, candidate) == StepState::done, "candidate child did not complete");
    const auto &point = candidate->result();
    require(point.xyz == Vec3{.625, .5, .25} &&
                point.decision == FaceCandidateDecision::protected_feature && point.feature &&
                point.feature->owner == 400,
            "native candidate disagrees with literal POWER S=(3u/2-u²/2,v,u²)");
  }
  service->close();
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected authored chart fixture directory");
    for (const uint32_t threads : {1U, 4U}) {
      check(std::filesystem::path(argv[1]) / "parabolic-sheet.bgsource", threads, false);
      check(std::filesystem::path(argv[1]) / "parabolic-sheet.bgsource", threads, true);
    }
    std::cout << "PASS\tnative source/preparation/numerical/CDT/candidate chain CPU1/4\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
