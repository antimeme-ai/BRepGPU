#include "../native/mesher/adaptive_assembly.hpp"
#include "../native/mesher/adaptive_caller_io.hpp"
#include "../native/mesher/adaptive_component.hpp"
#include "../native/mesher/adaptive_trimmed_io.hpp"
#include <fstream>
#include <iostream>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool okay, const char *message) {
  if (!okay)
    throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc >= 4 && argc <= 7,
            "usage: component source output threads [module device] [positional]");
    const double positional = argc == 5 || argc == 7 ? std::stod(argv[argc - 1]) : .01;
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 33);
    JobContext context({uint64_t{1} << 29, uint64_t{1} << 44}, resource.get(), {});
    const auto source = load_source(argv[1], resource);
    const auto &original = source.view();
    require(!original.definitions.empty(), "source definition required");
    const auto prepared = prepare_adaptive_assembly(
        context.budget, source, resource,
        {Rational::from_double(context.budget, positional),
         Rational::from_double(context.budget, 1.e-8)},
        {{{OwnerKind::run, 900, 1}, 1, 0}, {{OwnerKind::run, 900, 1}, 1, 1}});
    std::vector<CarrierRequest> requests;
    for (const auto &carrier : prepared->carriers)
      requests.push_back({carrier.source, carrier.cache_domain});
    const auto cache =
        prepare_trim_source_cache(source, requests, resource, {}, {}, context.budget);
    const auto curve_requests = caller::definition_curve_requests(source, {0}, resource);
    const auto curves =
        prepare_curve_source_cache(source, curve_requests, resource, {}, {}, context.budget);
    const auto threads = static_cast<uint32_t>(std::stoul(argv[3]));
    ComponentLimits limits;
    limits.face_owners = 2;
    limits.curves.cpu_threads = threads;
    NativeScheduler scheduler(resource, {threads, 4, uint64_t{1} << 27, uint64_t{1} << 42});
    std::optional<CacheDevice> device;
    if (argc == 6 || argc == 7)
      device = CacheDevice{static_cast<int32_t>(std::stoi(argv[5])), argv[4]};
    ComponentMesher mesher(resource, cache, limits, device);
    const auto &allowances = prepared->assembly.view();
    const auto found = std::find_if(allowances.definitions.begin(), allowances.definitions.end(),
                                    [](const auto &row) { return row.definition.value == 0; });
    require(found != allowances.definitions.end(), "definition local allowances required");
    const auto output =
        mesher.mesh({1, {0}, curves, allowances.values, found->positional, found->node_source},
                    scheduler, context);
    require(output && output->faces.size() == original.definition_faces.at(0).size(),
            "complete original face inventory required");
    require(output->stats.assignments >= output->faces.size(), "actual assignments required");
    mesher.close();
    if (threads == 4 && !device) {
      // The same real component must drain a failure INSIDE a face task, with
      // other owners live, and refuse subsequent use of that failed service.
      auto failing_limits = limits;
      failing_limits.face.facets.work = 1;
      NativeScheduler failing_scheduler(resource,
                                        {threads, 4, uint64_t{1} << 27, uint64_t{1} << 42});
      ComponentMesher failing(resource, cache, failing_limits);
      bool exhausted = false;
      try {
        (void)failing.mesh(output->query, failing_scheduler, context);
      } catch (const bgfd::Failure &error) {
        exhausted = error.status == BGFD_RESOURCE_EXHAUSTED;
      } catch (const Error &error) {
        exhausted = error.code == ErrorCode::resource_exhausted;
      }
      require(exhausted && failing_scheduler.idle(), "failed face tasks did not drain");
      bool terminal = false;
      try {
        (void)failing.mesh(output->query, failing_scheduler, context);
      } catch (const Error &error) {
        terminal = error.code == ErrorCode::cancelled || error.code == ErrorCode::invalid_source;
      }
      require(terminal, "failed component owner was reused");
      failing.close();
    }
    const std::filesystem::path directory(argv[2]);
    caller::write_curves(directory / "final.bgacrv", *output->boundary, context.budget);
    std::ofstream index(directory / "faces.tsv");
    require(index.good(), "open component index");
    for (uint32_t i = 0; i < output->faces.size(); ++i) {
      const auto &face = output->faces[i];
      const auto slot = original.definition_faces.at(0)[i];
      require(face->construction->key.face.value == slot, "canonical original face inventory");
      const auto stem = std::to_string(original.faces[slot].id.value);
      caller::write_curves(directory / (stem + ".bgacrv"), *face->boundary, context.budget);
      caller::write_trimmed_face(directory / (stem + ".bgatrm"), face, context.budget);
      index << original.faces[slot].id.value << '\t' << stem << ".bgacrv\t" << stem << ".bgatrm\n";
    }
    index.close();
    require(index.good(), "write component index");
    std::cout << "PASS\tfaces\t" << output->faces.size() << "\tassignments\t"
              << output->stats.assignments << "\tboundary_rounds\t" << output->stats.boundary_rounds
              << "\trequested_intervals\t" << output->stats.requested_intervals
              << "\tretained_faces\t" << output->stats.retained_faces << "\tfacets\t"
              << output->stats.facets << "\tcontact_epochs\t" << output->stats.contacts.epochs
              << "\tcontact_facets\t" << output->stats.contacts.queried_facets
              << "\tforbidden_pairs\t" << output->stats.contacts.retained_forbidden_pairs << '\n';
    return 0;
  } catch (const bgfd::Failure &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
  }
  return 1;
}
