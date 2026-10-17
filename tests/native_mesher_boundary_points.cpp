#include "../native/mesher/boundary_evaluation.hpp"
#include <bit>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
std::pmr::vector<bgmesh::BoundaryEvaluationResult>
evaluate(bgmesh::NativeScheduler &scheduler, const bgmesh::GeometryStore &geometry,
         const bgmesh::BoundaryEvaluationBatch &batch,
         const std::shared_ptr<bgmesh::RunResource> &run) {
  std::pmr::vector<std::shared_ptr<bgmesh::BoundaryPointJob>> jobs(run.get());
  // Each literal task is its own child here to force interleaving. Production
  // may batch adjacent canonical slots without changing their source identity.
  for (std::size_t i = 0; i < batch.tasks().size(); ++i) {
    auto job = std::allocate_shared<bgmesh::BoundaryPointJob>(
        bgmesh::RunAllocator<bgmesh::BoundaryPointJob>(run), run, geometry, batch,
        bgmesh::Range<bgmesh::BoundaryEvaluationTask>{i, 1},
        bgmesh::NativeTaskKey{{bgmesh::OwnerKind::run, 0, i}, batch.epoch().value, 0});
    scheduler.submit(job);
    jobs.push_back(std::move(job));
  }
  scheduler.drain();
  std::size_t completed = 0;
  while (auto completion = scheduler.take()) {
    completion->rethrow();
    require(completion->state == bgmesh::StepState::done, "point child must finish its range");
    ++completed;
  }
  require(completed == jobs.size(), "every original sample must complete before commit");
  std::pmr::vector<bgmesh::BoundaryEvaluationResult> results(run.get());
  for (const auto &job : jobs)
    results.insert(results.end(), job->results().begin(), job->results().end());
  return results;
}
void actual(const bgmesh::GeometryStore &geometry, uint32_t threads,
            const std::shared_ptr<bgmesh::RunResource> &run) {
  bgfd::Budget owner({uint64_t{1} << 24, uint64_t{1} << 32}, run.get());
  bgmesh::NativeScheduler scheduler(run, {threads, 8, 1u << 20, uint64_t{1} << 32});
  bgmesh::DefinitionBoundaryForest forest(owner, geometry.view().source,
                                          bgmesh::Slot<bgmesh::DefinitionTag>{0}, run);
  const auto initial = forest.pending_evaluations();
  if (!initial)
    throw std::runtime_error("original mapped knot omitted from native execution");
  require(initial->tasks().size() == 1, "original mapped knot enters native execution");
  auto points = evaluate(scheduler, geometry, *initial, run);
  require(points[0].dimension == 3 && points[0].xyz == bgmesh::Vec3{.25, 0, 0},
          "literal original descending curve maps t=.5 to x=.25");
  (void)forest.complete_evaluations(initial->epoch(), points);
  const bgmesh::Slot<bgmesh::EdgeTag> edge{0};
  const auto intervals = forest.intervals(edge);
  const auto epoch = forest.begin_epoch();
  forest.arrive(epoch, bgmesh::Slot<bgmesh::FaceTag>{0}, 0, intervals);
  forest.arrive(epoch, bgmesh::Slot<bgmesh::FaceTag>{1}, 0, {});
  const auto split = forest.prepare_splits(epoch);
  auto subdivided = evaluate(scheduler, geometry, split, run);
  require(subdivided.size() == 2 && subdivided[0].xyz == bgmesh::Vec3{.125, 0, 0} &&
              subdivided[1].xyz == bgmesh::Vec3{.625, 0, 0},
          "native original evaluator uses each retained exact midpoint");
  const auto update = forest.complete_evaluations(epoch, subdivided);
  require(update.changed_faces.size() == 2 && update.new_samples.size() == 2,
          "native point children feed one shared incident-face transaction");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "source fixture argument required");
    auto run = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 28);
    const auto source = bgmesh::load_source(argv[1], run);
    bgfd::Budget compiler({uint64_t{1} << 24, uint64_t{1} << 32}, run.get());
    const auto geometry = bgmesh::compile_geometry(
        source, run, {{bgmesh::OwnerKind::geometry, 0, 0}, 0, 0}, compiler);
    {
      const auto foreign_source = bgmesh::load_source(argv[1], run);
      const auto foreign_geometry = bgmesh::compile_geometry(
          foreign_source, run, {{bgmesh::OwnerKind::geometry, 0, 1}, 0, 0}, compiler);
      bgmesh::DefinitionBoundaryForest forest(compiler, source,
                                              bgmesh::Slot<bgmesh::DefinitionTag>{0}, run);
      const auto batch = forest.pending_evaluations();
      if (!batch)
        throw std::runtime_error("authority fixture lacks initial batch");
      bool rejected = false;
      try {
        const bgmesh::BoundaryPointJob job(
            run, foreign_geometry, *batch, {0, 1},
            {{bgmesh::OwnerKind::run, 0, 0}, batch->epoch().value, 0});
      } catch (const bgmesh::Error &error) {
        rejected = error.code == bgmesh::ErrorCode::internal_error;
      }
      require(rejected, "foreign original geometry cannot interpret unqualified curve slots");
    }
    actual(geometry, 1, run);
    actual(geometry, 4, run);
    std::cout << "PASS\tnative original boundary point children CPU1/CPU4\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  }
}
