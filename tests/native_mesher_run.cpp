#include "../native/mesher/meshing_run.hpp"
#include "../native/mesher/mesh_publish.hpp"
#include "../native/mesher/mesh_io.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
PreparedRun prepare(const std::filesystem::path &path, const std::shared_ptr<RunResource> &resource,
                     NativeScheduler &scheduler, double allowance) {
  auto task = std::allocate_shared<RunPreparationJob>(
      RunAllocator<RunPreparationJob>(resource), resource,
      RunPreparationInput{path, allowance, .000001});
  scheduler.submit(task);
  scheduler.drain();
  auto completion = scheduler.take();
  if (!completion)
    throw std::runtime_error("source preparation completion missing");
  completion->rethrow();
  require(completion->state == StepState::done, "source preparation did not finish");
  return task->result();
}
std::vector<char> bytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void run(const std::filesystem::path &fixtures, const std::filesystem::path &output,
          bool curved, uint32_t threads, const char *module = nullptr) {
  const auto resource = std::make_shared<RunResource>(1ULL << 31);
  NativeScheduler scheduler(resource, {threads, 16, 1ULL << 27, 1ULL << 40});
  const double allowance = curved ? .05 : .00001;
  const auto source = prepare(fixtures / (curved ? "parabolic-sheet.bgsource" : "warped-plane.bgsource"),
                               resource, scheduler, allowance);
  MeshingRunLimits limits;
  limits.coordinator_bytes = 1ULL << 27;
  limits.numerical_services = threads;
  limits.point_batch = 1;
  limits.contact_batch = 1;
  limits.preparation.bytes = 1ULL << 25;
  limits.face.bytes = 1ULL << 25;
  if (module) {
    limits.cuda = load_bound_cuda(resource, module);
    limits.devices.push_back({0, 1ULL << 26, 1ULL << 26});
  }
  const auto result = mesh_prepared_run(source, scheduler, limits);
  require(result->faces.size() == 1 && result->boundaries.size() == 1,
          "whole coordinator lost the complete original inventory");
  require(result->stats.boundary_rounds >= 1 && result->stats.contact_rounds >= 1,
          "whole coordinator skipped an owner horizon");
  const auto &face = *result->faces[0];
  require(face.boundary->uses.size() == 4 &&
              face.boundary->trace_maps.size() == face.boundary->segments.size() &&
              face.boundary->segments.size() >= 4,
          "whole source occurrence/trace semantics missing");
  if (!curved) {
    require(face.boundary->chart_kind == WorkingChartKind::physical_plane_convex_q &&
                face.triangles.size() == 2 && result->stats.boundary_rounds == 1,
            "nonlinear flat carrier must stay two triangles through the complete operation");
  } else {
    require(face.triangles.size() > 2 && result->stats.boundary_rounds > 1 &&
                result->stats.boundary_points > 0,
            "curved source must exercise shared source splits and actual refinement");
  }
  bgfd::Budget budget({1ULL << 27, 1ULL << 40}, resource.get());
  if (curved) {
    // Independent literal POWER surface; never call the production evaluator.
    for (const auto &triangle : face.triangles) {
      for (uint32_t a = 0; a <= 8; ++a)
        for (uint32_t b = 0; b <= 8 - a; ++b) {
          const std::array<long double, 3> weights{
              static_cast<long double>(a) / 8, static_cast<long double>(b) / 8,
              static_cast<long double>(8 - a - b) / 8};
          long double u = 0, v = 0;
          std::array<long double, 3> actual{};
          for (std::size_t i = 0; i < 3; ++i) {
            const auto &site = face.sites[triangle.sites[i].value];
            u += weights[i] * import_exact(budget, face.values.view(site.original_uv[0])).to_double();
            v += weights[i] * import_exact(budget, face.values.view(site.original_uv[1])).to_double();
            for (std::size_t axis = 0; axis < 3; ++axis)
              actual[axis] += weights[i] * site.node.xyz[axis];
          }
          const std::array<long double, 3> expected{1.5L * u - .5L * u * u, v, u * u};
          long double squared = 0;
          for (std::size_t axis = 0; axis < 3; ++axis)
            squared += (expected[axis] - actual[axis]) * (expected[axis] - actual[axis]);
          require(squared <= static_cast<long double>(allowance) * allowance,
                  "whole result exceeds independent original POWER surface distance");
        }
    }
  }
  const auto mesh = publish_native_mesh(budget, source, result->boundaries, result->faces,
                                        {allowance, .000001, 1});
  const std::string name = std::string(curved ? "curved" : "plane") +
                           (module ? "-p100" : "-cpu") + std::to_string(threads) + ".bgnmesh";
  const auto path = output / name;
  write_native_mesh(path, mesh);
  const auto restored = load_native_mesh(path, resource);
  require(restored.view().facets.size() == face.triangles.size(),
          "complete semantic publication/readback changed actual facets");
  if (!module && threads == 4)
    require(bytes(path) == bytes(output / (std::string(curved ? "curved" : "plane") + "-cpu1.bgnmesh")),
            "CPU worker count changed complete semantic mesh bytes");
  if (module)
    require(bytes(path) == bytes(output / (std::string(curved ? "curved" : "plane") + "-cpu1.bgnmesh")),
            "CPU/P100 placement changed complete semantic mesh bytes");
  if (module && curved)
    require(result->stats.numerical_device_tasks > 0,
            "P100 complete curved run performed no device residual tasks");
  std::cout << name << '\t' << face.triangles.size() << '\t' << result->stats.seconds << '\t'
            << result->stats.boundary_rounds << '\t' << result->stats.optimizer_flips << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 3 || argc == 4, "expected source fixture/output directory and optional CUDA module");
    const std::filesystem::path output(argv[2]);
    std::filesystem::create_directories(output);
    for (const bool curved : {false, true}) {
      run(argv[1], output, curved, 1);
      run(argv[1], output, curved, 4);
      if (argc == 4)
        run(argv[1], output, curved, 4, argv[3]);
    }
    std::cout << "PASS\tcomplete native source-to-mesh owner coordination\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
