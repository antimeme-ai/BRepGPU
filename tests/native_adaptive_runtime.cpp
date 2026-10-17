#include "adaptive_cache.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
uint64_t digest(const PacketMesh &mesh) {
  uint64_t h = 1469598103934665603ULL;
  auto mix = [&](uint64_t word) { h = (h ^ word) * 1099511628211ULL; };
  for (const auto &point : mesh->selection->points) {
    for (double x : point.actual_xyz)
      mix(std::bit_cast<uint64_t>(x));
    for (auto ref : point.chart_xy) {
      const auto v = mesh->selection->values.view(ref);
      mix(static_cast<uint64_t>(v.sign));
      for (auto x : v.numerator)
        mix(x);
      mix(UINT64_MAX);
      for (auto x : v.denominator)
        mix(x);
    }
  }
  for (const auto &t : mesh->triangles)
    for (auto p : t.points)
      mix(p.value);
  return h;
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2 || argc == 4, "usage: runtime source [cuda-module device]");
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 30);
    bgfd::Budget budget({uint64_t{1} << 28, uint64_t{1} << 40}, resource.get());
    auto source = load_source(argv[1], resource);
    const CarrierRequest request{source.view().carrier_index.find(CarrierId{9}), {0, 1, 0, 1}};
    auto cache = prepare_source_cache(source, std::span(&request, 1), resource, {}, {}, budget);
    CacheLimits limits;
    limits.nodes = 65536;
    limits.points = 262144;
    limits.control_scalars = 4194304;
    limits.frontier = 65536;
    limits.host_bytes = uint64_t{1} << 29;
    limits.device_bytes = uint64_t{1} << 29;
    limits.cpu_threads = 4;
    {
      auto trace_only=prepare_trace_source_cache(source,std::span(&request,1),resource,{},{},budget);
      bool refused=false;
      try {PatchSession invalid(resource,trace_only,limits);}
      catch(const Error &error) {refused=error.code==ErrorCode::invalid_source;}
      require(refused,"regular patch admitted trace-only source geometry");
    }
    PatchSession session(resource, cache, limits);
    if (argc == 4)
      session.open_device({static_cast<int32_t>(std::stoi(argv[3])), argv[2]});
    std::array<uint64_t, 4> hashes{};
    const std::array<double, 4> tolerances{.02, .08, .02, .02};
    for (uint32_t query = 0; query < tolerances.size(); ++query) {
      ExactBuilder values(resource, {{OwnerKind::run, 0, 991}, query + 1, 1});
      const auto tau = values.append(Rational::from_double(budget, tolerances[query]));
      auto table = values.seal();
      std::vector<RootAllowance> allowances;
      for (uint32_t i = 0; i < cache.roots().size(); ++i)
        allowances.push_back({{i}, tau});
      session.begin({query + 1, cache, table, allowances});
      JobContext context({uint64_t{1} << 26, uint64_t{1} << 40}, resource.get(), {});
      uint32_t steps = 0;
      while (session.step(context) != StepState::done) {
        require(++steps < 1000000, "runtime did not finish within finite steps");
      }
      const auto result = session.result();
      require(!result->triangles.empty(), "runtime omitted patch triangles");
      require(result->demoted_leaf_slots.empty(), "runtime published missing demoted area");
      require(!result->selection->leaves.empty(), "runtime omitted selected source leaves");
      for (const auto &triangle : result->triangles)
        for (auto p : triangle.points)
          require(p.value < result->selection->points.size(), "runtime triangle point index");
      hashes[query] = digest(result);
      session.release_result();
    }
    require(hashes[0] == hashes[2] && hashes[2] == hashes[3], "runtime warm history changed mesh");
    const auto stats = session.stats();
    require(stats.queries == 4 && stats.point_hits > 0 && stats.nodes_reused > 0,
            "runtime did not reuse immutable cache values");
    require(stats.actual_facet_tests > 0, "runtime omitted actual emitted facet checks");
    if (argc == 4)
      require(stats.device_points > 0 && stats.device_bounds > 0 && stats.device_epochs > 0,
              "runtime omitted actual adaptive GPU work");
    session.close();
    // Direct failure/lifetime checks on the same source operation; no substitute
    // geometry, driver-fault injection or independent certification campaign.
    ExactBuilder faults(resource, {{OwnerKind::run, 0, 992}, 1, 1});
    const auto tau_fault = faults.append(Rational::from_double(budget, .02));
    const auto fault_values = faults.seal();
    std::vector<RootAllowance> fault_allowances;
    for (uint32_t i = 0; i < cache.roots().size(); ++i)
      fault_allowances.push_back({{i}, tau_fault});
    {
      PatchSession invalid(resource, cache, limits);
      auto foreign = prepare_source_cache(source, std::span(&request, 1), resource, {}, {}, budget);
      bool refused = false;
      try {
        invalid.begin({1, foreign, fault_values, fault_allowances});
      } catch (const Error &e) {
        refused = e.code == ErrorCode::invalid_source;
      }
      require(refused, "runtime admitted a foreign cache with identical source IDs");
      invalid.close();
    }
    {
      auto capped = limits;
      capped.nodes = 1;
      capped.cpu_threads = 1;
      PatchSession small(resource, cache, capped);
      small.begin({1, cache, fault_values, fault_allowances});
      JobContext context({uint64_t{1} << 26, uint64_t{1} << 40}, resource.get(), {});
      bool refused = false;
      try {
        for (uint32_t i = 0; i < 10000; ++i)
          if (small.step(context) == StepState::done)
            break;
      } catch (const Error &e) {
        refused = e.code == ErrorCode::resource_exhausted;
      }
      require(refused, "runtime node cap did not refuse required refinement");
      small.close();
    }
    {
      std::stop_source cancellation;
      PatchSession stopped(resource, cache, limits, cancellation.get_token());
      stopped.begin({1, cache, fault_values, fault_allowances});
      JobContext context({uint64_t{1} << 26, uint64_t{1} << 40}, resource.get(), {});
      (void)stopped.step(context);
      cancellation.request_stop();
      bool refused = false;
      try {
        (void)stopped.step(context);
      } catch (const Error &e) {
        refused = e.code == ErrorCode::cancelled;
      }
      require(refused, "runtime did not propagate pending cancellation");
      stopped.close();
    }
    std::cout << "runtime\tGREEN\t" << hashes[0] << '\t' << stats.epochs << '\t'
              << stats.point_evaluations << '\t' << stats.device_points << '\n';
    return 0;
  } catch (const bgfd::Failure &error) {
    std::cerr << "exact failure: " << error.what() << '\n';
    return 1;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
