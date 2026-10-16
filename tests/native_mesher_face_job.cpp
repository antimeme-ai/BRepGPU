#include "../native/mesher/face_job.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <class F> void rejects(F action, const char *message) {
  bool rejected = false;
  try {
    action();
  } catch (const Error &) {
    rejected = true;
  }
  require(rejected, message);
}
StepCompletion completed(NativeScheduler &scheduler) {
  scheduler.drain();
  auto result = scheduler.take();
  if (!result)
    throw std::runtime_error("native owner completion missing");
  return std::move(*result);
}
FaceOwnerInput input(bgfd::Budget &b, const std::shared_ptr<RunResource> &run,
                     const GeometryStore &geometry) {
  const auto &source = geometry.view().source.view();
  const auto face_id = source.faces[0].id.value;
  ExactBuilder builder(run, {{OwnerKind::face, 0, face_id}, 4, 0});
  std::array<std::array<ExactRef, 2>, 4> refs{};
  const std::array<Vec2, 4> square{{{0, 0}, {1, 0}, {1, 1}, {0, 1}}};
  for (std::size_t i = 0; i < square.size(); ++i)
    for (std::size_t a = 0; a < 2; ++a)
      refs[i][a] = builder.append(Rational::from_double(b, square[i][a]));
  const auto zero = builder.append(Rational(b)), one = builder.append(Rational(b, 1));
  auto boundary = std::allocate_shared<FaceBoundarySnapshotData>(
      RunAllocator<FaceBoundarySnapshotData>(run), run, geometry, Slot<DefinitionTag>{0},
      Slot<FaceTag>{0}, std::array<uint64_t, 3>{2, 4, 7}, WorkingChartKind::original_uv,
      builder.seal());
  for (uint32_t i = 0; i < 4; ++i) {
    boundary->sites.push_back(
        {refs[i],
         refs[i],
         {{0, NodeGenerator::original_vertex, source.vertices[i].id.value, 0, 0},
          source.vertices[i].xyz,
          {}}});
    boundary->segments.push_back({i, {source.edges[i].id.value, i}, {i}, {zero, one}, {0, 0}});
  }
  boundary->loops.push_back({{0}, true, {0, 4}});
  ExactBuilder metric(run, {{OwnerKind::geometry, 0, face_id}, 4, 1});
  const auto z = metric.append(Rational(b)), o = metric.append(Rational(b, 1));
  const auto tau = metric.append(Rational(b, 1).divide(Rational(b, 64)));
  return {std::move(boundary), metric.seal(), {o, z, z, o}, {o, o, o, tau}, 17};
}
void run(const std::shared_ptr<RunResource> &resource, const GeometryStore &geometry,
         uint32_t threads) {
  bgfd::Budget owner({1ULL << 25, 1ULL << 35}, resource.get());
  NativeScheduler scheduler(resource, {threads, 8, 1ULL << 20, 1ULL << 32});
  auto job = std::allocate_shared<FaceOwnerJob>(
      RunAllocator<FaceOwnerJob>(resource), resource, input(owner, resource, geometry),
      FaceOwnerLimits{1ULL << 25, 1ULL << 35, 8}, scheduler.stop_token());
  scheduler.submit(job);
  auto completion = completed(scheduler);
  completion.rethrow();
  require(completion.state == StepState::wait_child &&
              job->event().kind == FaceOwnerEventKind::cells,
          "face must yield immutable cell request without waiting on a worker");
  const auto packet = job->event().cells;
  require(packet && packet->cells.size() == 2, "initial complete cell packet missing");
  const auto stats = job->take_owner_work();
  require(stats.exact_work > 0 && stats.peak_bytes > 0 && job->take_owner_work().exact_work == 0,
          "persistent face arithmetic must be charged once independently of scheduler scratch");
  std::pmr::vector<CellBoundResult> answers(resource.get());
  for (const auto &cell : packet->cells)
    answers.push_back({cell.header, BoundDecision::accept});
  rejects([&] { job->finish_cells(std::span<const CellBoundResult>(answers).first(1)); },
          "partial child batch resumed face");
  auto stale = answers;
  --stale[0].header.face_generation;
  rejects([&] { job->finish_cells(stale); }, "stale face generation resumed parent");
  auto duplicate = answers;
  duplicate[1] = duplicate[0];
  rejects([&] { job->finish_cells(duplicate); }, "duplicated child replaced missing cell");
  std::reverse(answers.begin(), answers.end());
  job->finish_cells(answers);
  rejects([&] { job->finish_cells(answers); }, "child batch delivered twice");
  scheduler.submit(job);
  completion = completed(scheduler);
  completion.rethrow();
  require(completion.state == StepState::wait_barrier &&
              job->event().kind == FaceOwnerEventKind::settled &&
              job->engine().topology().triangles().size() == 2,
          "complete numerical answers must reach the definition horizon");
  job->finish_generation();
  scheduler.submit(job);
  completion = completed(scheduler);
  completion.rethrow();
  require(completion.state == StepState::done && job->event().kind == FaceOwnerEventKind::finished,
          "definition coordinator must explicitly finish settled generation");
  std::stop_source stop;
  stop.request_stop();
  auto cancelled = std::allocate_shared<FaceOwnerJob>(
      RunAllocator<FaceOwnerJob>(resource), resource, input(owner, resource, geometry),
      FaceOwnerLimits{1ULL << 25, 1ULL << 35, 8}, stop.get_token());
  scheduler.submit(cancelled);
  completion = completed(scheduler);
  require(completion.state == StepState::failed, "cancelled owner started geometry");
  bool observed = false;
  try {
    completion.rethrow();
  } catch (const bgfd::Cancelled &) {
    observed = true;
  }
  require(observed, "persistent owner did not retain cancellation token");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected original geometry fixture");
    const auto resource = std::make_shared<bgmesh::RunResource>(1ULL << 29);
    const auto source = bgmesh::load_source(argv[1], resource);
    bgfd::Budget budget({1ULL << 26, 1ULL << 35}, resource.get());
    const auto geometry = bgmesh::compile_geometry(
        source, resource, {{bgmesh::OwnerKind::geometry, 0, 0}, 2, 0}, budget);
    run(resource, geometry, 1);
    run(resource, geometry, 4);
    std::cout << "PASS\tnative face owner continuations CPU1/CPU4\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
