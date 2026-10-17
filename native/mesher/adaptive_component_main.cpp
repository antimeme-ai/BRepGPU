#include "adaptive_assembly.hpp"
#include "adaptive_caller_io.hpp"
#include "adaptive_component.hpp"
#include "adaptive_component_io.hpp"
#include "adaptive_trimmed_io.hpp"
#include "output_directory.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <unordered_map>

namespace {
using namespace bgmesh;
using namespace bgmesh::adaptive;
using Clock = std::chrono::steady_clock;
using Box = std::array<Rational, 6>;
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
uint64_t integer(std::string_view text) {
  uint64_t out = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), out);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    invalid("invalid unsigned integer option");
  return out;
}
double number(std::string_view text) {
  double out = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), out);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || !std::isfinite(out))
    invalid("invalid finite numeric option");
  return out;
}
struct Options final {
  std::filesystem::path source, output, module;
  uint64_t definition = 0, memory_mib = 1024;
  uint64_t construction_work = TrimFaceLimits{}.construction_work;
  uint32_t threads = 0, max_nodes = 1u << 18, max_depth = 64;
  std::optional<uint32_t> face_owners;
  int32_t device = 0;
  double node_source = 0;
  std::vector<double> positional;
  bool p100 = false, help = false;
};
Options options(int argc, char **argv) {
  Options out;
  for (int i = 1; i < argc; ++i) {
    const std::string_view key(argv[i]);
    if (key == "--help") {
      out.help = true;
      continue;
    }
    if (i + 1 == argc)
      invalid("missing option value");
    const std::string_view value(argv[++i]);
    if (key == "--source")
      out.source = value;
    else if (key == "--output")
      out.output = value;
    else if (key == "--cuda-module")
      out.module = value;
    else if (key == "--definition")
      out.definition = integer(value);
    else if (key == "--memory-mib")
      out.memory_mib = integer(value);
    else if (key == "--construction-work")
      out.construction_work = integer(value);
    else if (key == "--positional-mm")
      out.positional.push_back(number(value));
    else if (key == "--node-source-mm")
      out.node_source = number(value);
    else if (key == "--backend") {
      if (value != "cpu" && value != "p100")
        invalid("backend must be cpu or p100");
      out.p100 = value == "p100";
    } else if (key == "--threads" || key == "--face-owners" || key == "--max-nodes" ||
               key == "--max-depth" || key == "--device") {
      const auto n = integer(value);
      if (n > INT32_MAX)
        invalid("option exceeds native index range");
      if (key == "--threads")
        out.threads = static_cast<uint32_t>(n);
      else if (key == "--face-owners")
        out.face_owners = static_cast<uint32_t>(n);
      else if (key == "--max-nodes")
        out.max_nodes = static_cast<uint32_t>(n);
      else if (key == "--max-depth")
        out.max_depth = static_cast<uint32_t>(n);
      else
        out.device = static_cast<int32_t>(n);
    } else
      invalid("unknown option");
  }
  if (out.help)
    return out;
  if (out.source.empty() || out.output.empty() || !out.definition || out.positional.empty() ||
      !(out.node_source > 0) || !out.threads || out.threads > 256)
    invalid("required: --source --definition --output --positional-mm positive "
            "--node-source-mm positive --threads 1..256");
  for (const auto epsilon : out.positional)
    if (!(epsilon > 0))
      invalid("positional allowance must be positive");
  if (out.face_owners && (!out.face_owners.value() || out.face_owners.value() > 256))
    invalid("face owners must be 1..256");
  if (!out.memory_mib || out.memory_mib > (UINT64_MAX >> 20) || !out.construction_work ||
      !out.max_nodes || out.max_nodes > UINT32_MAX / 264 || !out.max_depth || out.max_depth > 64)
    invalid("invalid memory, work, node capacity or depth64 limit");
  if (out.p100 != !out.module.empty())
    invalid("p100 requires explicit --cuda-module; CPU does not load device modules");
  out.output = std::filesystem::absolute(out.output).lexically_normal();
  return out;
}
void run(const Options &o, std::string_view &stage) {
  const auto full = Clock::now();
  const uint64_t bytes = o.memory_mib << 20;
  const auto resource = std::make_shared<RunResource>(bytes);
  JobContext context({bytes, uint64_t{1} << 44}, resource.get(), {});
  auto &budget = context.budget;
  OutputDirectory output(o.output);
  std::ofstream timing(output.path() / "timings.tsv"), queries(output.path() / "queries.tsv"),
      faces(output.path() / "faces.tsv"), metadata(output.path() / "component.tsv");
  if (!timing || !queries || !faces || !metadata)
    throw Error(ErrorCode::io_failure, "cannot open component metadata");
  timing << std::setprecision(17) << "scope\tseconds\n";
  const auto record = [&](std::string_view name, Clock::time_point start) {
    timing << name << '\t' << elapsed(start) << '\n';
  };
  stage = "source.decode";
  auto tick = Clock::now();
  const auto source = load_source(o.source, resource);
  const auto &original = source.view();
  const auto definition = original.definition_index.find(DefinitionId{o.definition});
  if (original.definition_faces.at(definition.value).empty())
    invalid("selected component has no original faces");
  record(stage, tick);
  // Reserved caller-only namespace, distinct from runtime/session exact pools.
  constexpr OwnerKey caller_owner{OwnerKind::run, UINT32_MAX - 16, 1};
  stage = "source.envelopes_and_initial_assembly";
  tick = Clock::now();
  const auto prepared =
      prepare_adaptive_assembly(budget, source, resource,
                                {Rational::from_double(budget, o.positional.front()),
                                 Rational::from_double(budget, o.node_source)},
                                {{caller_owner, 1, 0}, {caller_owner, 1, 1}});
  record(stage, tick);
  stage = "source.compile_banks";
  tick = Clock::now();
  std::pmr::vector<CarrierRequest> carriers(resource.get());
  std::pmr::vector<uint8_t> seen(original.carriers.size(), 0, resource.get());
  for (auto slot : original.definition_faces.at(definition.value)) {
    const auto &row = prepared->faces.at(slot);
    const auto &profile = prepared->carriers.at(row.carrier_envelope);
    if (row.source.value != slot ||
        profile.source != original.carrier_index.find(original.faces[slot].carrier_id))
      throw Error(ErrorCode::internal_error, "prepared original face profile mismatch");
    if (!seen[profile.source.value]) {
      seen[profile.source.value] = 1;
      carriers.push_back({profile.source, profile.cache_domain});
    }
  }
  std::sort(carriers.begin(), carriers.end(),
            [](const auto &a, const auto &b) { return a.source.value < b.source.value; });
  SourceCacheLimits source_limits;
  source_limits.roots = o.max_nodes;
  source_limits.control_scalars = o.max_nodes * 264;
  const auto cache =
      prepare_trim_source_cache(source, carriers, resource, {}, source_limits, budget);
  const auto curve_requests = caller::definition_curve_requests(source, definition, resource);
  CurveCacheLimits curve_limits;
  curve_limits.control_scalars = o.max_nodes * 264;
  const auto curves =
      prepare_curve_source_cache(source, curve_requests, resource, {}, curve_limits, budget);
  record(stage, tick);
  ComponentLimits limits;
  limits.face_owners = o.face_owners.value_or(o.threads);
  auto &surface = limits.face.surface;
  surface.nodes = surface.frontier = o.max_nodes;
  surface.points = o.max_nodes * 4;
  surface.control_scalars = o.max_nodes * 264;
  surface.max_axis_depth = o.max_depth;
  surface.cpu_threads = 1;
  surface.host_bytes = surface.device_bytes = bytes;
  surface.child_bytes = std::min(bytes, uint64_t{1} << 26);
  limits.face.construction_work = o.construction_work;
  auto &physical = limits.curves;
  physical.nodes = physical.frontier = physical.requests = o.max_nodes;
  physical.points = o.max_nodes * 4;
  physical.control_scalars = o.max_nodes * 264;
  physical.max_depth = o.max_depth;
  physical.cpu_threads = o.threads;
  physical.host_bytes = physical.device_bytes = bytes;
  physical.child_bytes = surface.child_bytes;
  stage = "service.construct";
  tick = Clock::now();
  NativeScheduler scheduler(resource, {o.threads, std::max(o.threads, limits.face_owners),
                                       surface.child_bytes, o.construction_work});
  std::optional<CacheDevice> device;
  if (o.p100)
    device = CacheDevice{o.device, o.module};
  ComponentMesher mesher(resource, cache, limits, device);
  record(stage, tick);
  metadata << "key\tvalue\nscope\tcomplete_definition\ndefinition\t" << o.definition
           << "\nsource_digest\t";
  caller::digest(metadata, original.source_digest);
  metadata << "\nsource_units_mm\t" << std::setprecision(17) << original.units_mm << "\nbackend\t"
           << (o.p100 ? "p100" : "cpu") << "\nthreads\t" << o.threads << "\nface_owner_limit\t"
           << limits.face_owners << "\nnumeric_profile\t" << cache.profile().revision << '\n';
  faces << "query\tdefinition\tface\tmesh\tboundary\tboundary_epoch\tface_boundary_generation\n";
  queries
      << std::setprecision(17)
      << "query\trequested_positional_mm\trequested_node_mm\tlocal_positional_exact\tlocal_node_"
         "exact"
         "\tfaces\tfacets\towners\tassignments\tface_steps\tboundary_rounds\trequested_"
         "intervals\tretained_faces\tcontact_epochs\tcontact_facets\tcontact_candidate_"
         "pairs\tcontact_forbidden_pairs"
         "\tcurve_device_points_query\tcurve_device_bounds_query\tcurve_uploaded_bytes_"
         "query\tcurve_downloaded_bytes_query"
         "\tsurface_points_cumulative\tsurface_source_tests_cumulative\tsurface_exact_cumulative"
         "\tsurface_device_points_cumulative\tsurface_device_bounds_cumulative\tsurface_device_"
         "restrictions_cumulative"
         "\tsurface_uploaded_bytes_cumulative\tsurface_downloaded_bytes_cumulative"
         "\tsurface_host_reserved_bytes\tsurface_host_peak_sum_upper_bytes\tsurface_device_"
         "reserved_bytes\tsurface_device_peak_sum_upper_bytes"
         "\trun_peak_including_reservations_bytes\n";
  std::pmr::vector<Box> boxes(&budget);
  std::pmr::vector<DefinitionEnvelopeView> views(&budget);
  for (std::size_t q = 0; q < o.positional.size(); ++q) {
    const auto generation = static_cast<uint64_t>(q) + 1;
    const auto prefix = "query" + std::to_string(q) + ".";
    stage = "query.assembly_transport";
    tick = Clock::now();
    auto assembly = prepared->assembly;
    if (q) {
      if (boxes.empty()) {
        boxes.reserve(prepared->definitions.size());
        for (const auto &row : prepared->definitions) {
          const auto value = [&](std::size_t axis) {
            return import_exact(budget, prepared->envelope_values.view(row.xyz[axis]));
          };
          boxes.push_back({value(0), value(1), value(2), value(3), value(4), value(5)});
        }
        for (std::size_t i = 0; i < boxes.size(); ++i)
          views.push_back({source, prepared->definitions[i].source, boxes[i]});
      }
      assembly = prepare_assembly(budget, source, resource, views,
                                  {Rational::from_double(budget, o.positional[q]),
                                   Rational::from_double(budget, o.node_source)},
                                  {caller_owner, generation, 1});
    }
    const auto &allowances = assembly.view();
    const auto found = std::find_if(allowances.definitions.begin(), allowances.definitions.end(),
                                    [&](const auto &row) { return row.definition == definition; });
    if (found == allowances.definitions.end())
      throw Error(ErrorCode::internal_error, "selected definition has no exact assembly allowance");
    record(prefix + "assembly_transport", tick);
    stage = "query.complete_component";
    tick = Clock::now();
    const auto result = mesher.mesh(
        {generation, definition, curves, allowances.values, found->positional, found->node_source},
        scheduler, context);
    record(prefix + "complete_component", tick);
    const auto &stats = result->stats;
    const auto totals = caller::surface_totals(result->owners);
    timing << prefix << "owner_setup\t" << stats.owner_setup_seconds << '\n'
           << prefix << "boundary_setup\t" << stats.boundary_setup_seconds << '\n'
           << prefix << "boundary\t" << stats.boundary_seconds << '\n'
           << prefix << "faces\t" << stats.face_seconds << '\n'
           << prefix << "contacts\t" << stats.contact_seconds << '\n'
           << prefix << "boundary_close\t" << stats.boundary_close_seconds << '\n';
    stage = "output.query";
    tick = Clock::now();
    caller::write_component(output.path() / ("query-" + std::to_string(q)), result, faces, q,
                            "query-" + std::to_string(q), budget);
    record(prefix + "output", tick);
    queries << q << '\t' << o.positional[q] << '\t' << o.node_source << '\t';
    caller::write_exact(queries, allowances.values.view(found->positional));
    queries << '\t';
    caller::write_exact(queries, allowances.values.view(found->node_source));
    for (uint64_t count : {uint64_t(result->faces.size()),
                           stats.facets,
                           uint64_t(result->owners.size()),
                           stats.assignments,
                           stats.face_steps,
                           stats.boundary_rounds,
                           stats.requested_intervals,
                           stats.retained_faces,
                           stats.contacts.epochs,
                           stats.contacts.queried_facets,
                           stats.contacts.candidate_pairs,
                           stats.contacts.retained_forbidden_pairs,
                           stats.curves.device_points,
                           stats.curves.device_bounds,
                           stats.curves.uploaded_bytes,
                           stats.curves.downloaded_bytes,
                           totals.point_evaluations,
                           totals.source_triangle_tests,
                           totals.exact_fallbacks,
                           totals.device_points,
                           totals.device_bounds,
                           totals.device_restrictions,
                           totals.uploaded_bytes,
                           totals.downloaded_bytes,
                           totals.host_reserved_bytes,
                           totals.host_peak_bytes,
                           totals.device_reserved_bytes,
                           totals.device_peak_bytes,
                           resource->peak()})
      queries << '\t' << count;
    queries << '\n';
  }
  stage = "service.close";
  tick = Clock::now();
  mesher.close();
  scheduler.stop();
  scheduler.drain();
  record(stage, tick);
  stage = "output.original_source";
  tick = Clock::now();
  caller::write_source(output.path() / "source.bgsource", source);
  record(stage, tick);
  timing << "prepublication_horizon\t" << elapsed(full) << '\n';
  caller::close_stream(queries);
  caller::close_stream(faces);
  caller::close_stream(metadata);
  caller::close_stream(timing);
  stage = "output.publish";
  output.publish();
  std::cout << std::setprecision(17) << "adaptive_component\tcomplete\tdefinition\t" << o.definition
            << "\tqueries\t" << o.positional.size() << "\tseconds\t" << elapsed(full) << '\n';
}
} // namespace
int main(int argc, char **argv) {
  std::string_view stage = "options";
  try {
    const auto o = options(argc, argv);
    if (o.help) {
      std::cout
          << "brepgpu-adaptive-component --source FILE --definition ID --output NEW_DIRECTORY "
             "--positional-mm VALUE [--positional-mm VALUE ...] --node-source-mm VALUE "
             "--threads N [--face-owners N] [--memory-mib N --max-nodes N --max-depth N "
             "--construction-work N] [--backend p100 --cuda-module LIBRARY --device N]\n";
      return 0;
    }
    run(o, stage);
    return 0;
  } catch (const Error &error) {
    std::cerr << "error\t" << stage << '\t' << static_cast<unsigned>(error.code) << '\t'
              << error.what() << '\n';
  } catch (const bgfd::Failure &error) {
    std::cerr << "error\t" << stage << "\texact\t" << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error\t" << stage << "\tunexpected\t" << error.what() << '\n';
  }
  return 1;
}
