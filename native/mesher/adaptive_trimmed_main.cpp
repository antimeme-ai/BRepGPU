#include "adaptive_cache.hpp"
#include "adaptive_caller_io.hpp"
#include "adaptive_curves.hpp"
#include "adaptive_trimmed_io.hpp"
#include "curve_source_cache.hpp"
#include "output_directory.hpp"
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>

namespace {
using namespace bgmesh;
using namespace bgmesh::adaptive;
using Clock = std::chrono::steady_clock;
double seconds(Clock::time_point begin) {
  return std::chrono::duration<double>(Clock::now() - begin).count();
}
[[noreturn]] void invalid(std::string_view message) {
  throw Error(ErrorCode::invalid_source, std::string(message));
}
uint64_t integer(std::string_view text) {
  uint64_t value = 0;
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    invalid("invalid unsigned integer option");
  return value;
}
double number(std::string_view text) {
  double value = 0;
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
      !std::isfinite(value))
    invalid("invalid finite numeric option");
  return value;
}
struct Options final {
  std::filesystem::path source, output, module;
  uint64_t definition = 0, face = 0, memory_mib = 1024;
  double node_source = 0;
  uint32_t threads = 0, max_nodes = 1u << 18, max_depth = 64;
  int32_t device = 0;
  bool p100 = false, help = false;
  std::vector<double> positional;
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
    else if (key == "--face")
      out.face = integer(value);
    else if (key == "--memory-mib")
      out.memory_mib = integer(value);
    else if (key == "--positional-mm")
      out.positional.push_back(number(value));
    else if (key == "--node-source-mm")
      out.node_source = number(value);
    else if (key == "--backend") {
      if (value != "cpu" && value != "p100")
        invalid("backend must be cpu or p100");
      out.p100 = value == "p100";
    } else if (key == "--threads" || key == "--max-nodes" ||
               key == "--max-depth" || key == "--device") {
      const auto n = integer(value);
      if (n > INT32_MAX)
        invalid("option exceeds native index range");
      if (key == "--threads")
        out.threads = static_cast<uint32_t>(n);
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
  if (out.source.empty() || out.output.empty() || !out.definition ||
      !out.face || !(out.node_source > 0) || out.positional.empty() ||
      out.threads == 0 || out.threads > 256)
    invalid("required: --source --definition --face --output --positional-mm "
            "positive "
            "--node-source-mm positive "
            "--threads 1..256");
  for (double value : out.positional)
    if (value <= 0)
      invalid("positional allowance must be positive");
  if (!out.memory_mib || out.memory_mib > (UINT64_MAX >> 20) ||
      !out.max_nodes || out.max_nodes > UINT32_MAX / 264 || !out.max_depth ||
      out.max_depth > 64)
    invalid("invalid memory, node capacity or depth64 limit");
  if (out.p100 != !out.module.empty())
    invalid("p100 requires explicit --cuda-module; CPU does not load device "
            "modules");
  out.output = std::filesystem::absolute(out.output).lexically_normal();
  return out;
}
void close(std::ofstream &out) {
  out.close();
  if (!out)
    throw Error(ErrorCode::io_failure, "output write/close failed");
}
std::array<double, 4> original_envelope(const SourceData &source,
                                        const Carrier &carrier) {
  if (carrier.kind != 6)
    return carrier.uv_bounds; // explicit finite analytic admission profile
  std::array<double, 4> result{};
  for (uint32_t axis = 0; axis < 2; ++axis) {
    const auto knots = source.view(axis == 0 ? carrier.knot_u : carrier.knot_v);
    const auto mult = source.view(axis == 0 ? carrier.mult_u : carrier.mult_v);
    uint64_t expanded = 0;
    bool first = false, last = false;
    for (std::size_t i = 0; i < knots.size(); ++i) {
      const auto end = expanded + mult[i];
      if (expanded <= carrier.degrees[axis] && carrier.degrees[axis] < end) {
        result[2 * axis] = knots[i];
        first = true;
      }
      if (expanded <= carrier.control_shape[axis] &&
          carrier.control_shape[axis] < end) {
        result[2 * axis + 1] = knots[i];
        last = true;
      }
      expanded = end;
    }
    if (!first || !last || !(result[2 * axis] < result[2 * axis + 1]))
      invalid("original rational natural domain is not positive");
  }
  return result;
}
void run(const Options &o, std::string_view &stage) {
  const auto full = Clock::now();
  const uint64_t bytes = o.memory_mib << 20;
  const auto resource = std::make_shared<RunResource>(bytes);
  bgfd::Budget budget({bytes, uint64_t{1} << 44}, resource.get());
  OutputDirectory output(o.output);
  std::ofstream timings(output.path() / "timings.tsv"),
      queries(output.path() / "queries.tsv");
  if (!timings || !queries)
    throw Error(ErrorCode::io_failure, "cannot open trimmed-face metadata");
  timings << std::setprecision(17) << "scope\tseconds\n";
  const auto timing = [&](std::string_view scope, Clock::time_point begin) {
    timings << scope << '\t' << seconds(begin) << '\n';
  };
  stage = "source.decode";
  auto tick = Clock::now();
  const auto source = load_source(o.source, resource);
  timing(stage, tick);
  const auto &s = source.view();
  const auto definition = s.definition_index.find(DefinitionId{o.definition});
  const auto face = s.face_index.find(FaceId{o.face});
  if (s.faces.at(face.value).definition_id !=
      s.definitions.at(definition.value).id)
    invalid("selected original face does not belong to definition");
  const auto carrier = s.carrier_index.find(s.faces.at(face.value).carrier_id);
  stage = "source.compile_original_curves";
  tick = Clock::now();
  const auto curve_requests =
      caller::definition_curve_requests(source, definition, resource);
  CurveCacheLimits curve_cache_limits;
  curve_cache_limits.control_scalars = o.max_nodes * 264;
  const auto curve_cache = prepare_curve_source_cache(
      source, curve_requests, resource, {}, curve_cache_limits, budget);
  timing(stage, tick);
  stage = "source.compile_regular_surface";
  tick = Clock::now();
  SourceCacheLimits source_limits;
  source_limits.roots = o.max_nodes;
  source_limits.control_scalars = o.max_nodes * 264;
  const std::array surface_requests{CarrierRequest{
      carrier, original_envelope(s, s.carriers.at(carrier.value))}};
  const auto surface_cache = prepare_trim_source_cache(
      source, surface_requests, resource, {}, source_limits, budget);
  timing(stage, tick);
  CurveLimits curve_limits;
  curve_limits.nodes = curve_limits.frontier = curve_limits.requests =
      o.max_nodes;
  curve_limits.points = o.max_nodes * 4;
  curve_limits.control_scalars = o.max_nodes * 264;
  curve_limits.max_depth = o.max_depth;
  curve_limits.cpu_threads = o.threads;
  curve_limits.host_bytes = curve_limits.device_bytes = bytes;
  curve_limits.child_bytes = std::min(bytes, uint64_t{1} << 26);
  TrimFaceLimits face_limits;
  auto &limits = face_limits.surface;
  limits.nodes = limits.frontier = o.max_nodes;
  limits.points = o.max_nodes * 4;
  limits.control_scalars = o.max_nodes * 264;
  limits.max_axis_depth = o.max_depth;
  limits.cpu_threads = o.threads;
  limits.host_bytes = limits.device_bytes = bytes;
  limits.child_bytes = std::min(bytes, uint64_t{1} << 26);
  JobContext context({limits.child_bytes, limits.total_work}, resource.get(),
                     {});
  stage = "session.create";
  tick = Clock::now();
  AdaptiveBoundarySession boundary(resource, source, definition, curve_cache,
                                   curve_limits);
  FaceTrimSession session(resource, surface_cache, face_limits);
  timing(stage, tick);
  stage = "device.open";
  tick = Clock::now();
  if (o.p100) {
    const CacheDevice device{o.device, o.module};
    boundary.open_device(device);
    session.open_device(device);
  }
  timing(stage, tick);
  const auto settle_boundary = [&] {
    while (boundary.step(context) != StepState::done)
      std::this_thread::yield();
  };
  queries
      << std::setprecision(17)
      << "query\tpositional_mm\tnode_source_mm\tsource_digest\tnumeric_"
         "profile\tbackend"
         "\tthreads\tsource_units_mm\tdefinition\tface\tprepared_"
         "roots\toriginal_roots"
         "\tsites\tfacets\tband_components\tboundary_nodes\tboundary_intervals"
         "\tclassifications_cumulative\tpacket_rounds_cumulative\tregion_"
         "builds_cumulative"
         "\tband_anchors_cumulative\tpacket_targets_cumulative\tfacet_horizons_"
         "cumulative"
         "\tbound_pieces_cumulative\tdirect_source_tests_cumulative"
         "\tsurface_point_evaluations_cumulative\tsurface_point_hits_cumulative"
         "\tsurface_restrictions_cumulative\tsurface_nodes_created_cumulative"
         "\tsurface_nodes_reused_cumulative\tsurface_exact_fallbacks_cumulative"
         "\tsurface_device_points_cumulative\tsurface_device_bounds_cumulative"
         "\tsurface_device_restrictions_cumulative\tuploaded_bytes_cumulative"
         "\tdownloaded_bytes_cumulative\tsurface_host_reserved_bytes"
         "\tsurface_host_peak_bytes\tsurface_device_reserved_bytes\tsurface_"
         "device_peak_bytes"
         "\trun_peak_including_device_reservations_bytes"
         "\tclassification_seconds_cumulative\tpacket_seconds_"
         "cumulative\tregion_seconds_cumulative"
         "\tbound_seconds_cumulative\tanchor_seconds_cumulative\n";
  for (std::size_t query = 0; query < o.positional.size(); ++query) {
    stage = "query.prepare";
    tick = Clock::now();
    ExactBuilder builder(resource,
                         {{OwnerKind::run, definition.value, 1}, query + 1, 0});
    const auto positional =
        builder.append(Rational::from_double(budget, o.positional[query]));
    const auto node_source =
        builder.append(Rational::from_double(budget, o.node_source));
    const auto values = builder.seal();
    timing(stage, tick);
    stage = "query.construct_shared_boundary";
    tick = Clock::now();
    boundary.begin({query + 1, values, positional, node_source, {}});
    settle_boundary();
    timing(stage, tick);
    stage = "query.construct_complete_face";
    tick = Clock::now();
    session.begin({query + 1, definition, face, boundary.result(), values,
                   positional, node_source});
    for (;;) {
      const auto progress = session.step(context);
      if (progress == TrimFaceProgress::complete)
        break;
      if (progress == TrimFaceProgress::needs_boundary) {
        boundary.refine(boundary.result(), session.boundary_requests());
        settle_boundary();
        session.replace_boundary(boundary.result());
      } else {
        std::this_thread::yield();
      }
    }
    timing(stage, tick);
    {
      const auto result = session.result();
      stage = "output.shared_boundary";
      tick = Clock::now();
      caller::write_curves(output.path() /
                               ("query-" + std::to_string(query) + ".bgacrv"),
                           *result->boundary, budget);
      timing(stage, tick);
      stage = "output.trimmed_face";
      tick = Clock::now();
      caller::write_trimmed_face(
          output.path() / ("query-" + std::to_string(query) + ".bgatrm"),
          result, budget);
      timing(stage, tick);
      const auto stats = session.stats();
      const auto curves = boundary.stats();
      const auto &cache = stats.surface;
      const auto &mesh = *result->construction;
      queries << query << '\t' << o.positional[query] << '\t' << o.node_source
              << '\t';
      caller::digest(queries, s.source_digest);
      queries << '\t' << surface_cache.profile().revision << '\t'
              << (o.p100 ? "p100" : "cpu") << '\t' << o.threads << '\t'
              << s.units_mm << '\t' << o.definition << '\t' << o.face;
      for (uint64_t value :
           {uint64_t(surface_cache.roots().size()),
            uint64_t(surface_cache.original_roots().size()),
            uint64_t(mesh.sites.size()),
            uint64_t(mesh.facets.size()),
            uint64_t(mesh.components.size()),
            uint64_t(result->boundary->boundary->nodes.size()),
            uint64_t(result->boundary->physical_intervals.size()),
            stats.classifications,
            stats.packet_rounds,
            stats.region_builds,
            stats.band_anchors,
            stats.packet_targets,
            stats.facet_horizons,
            stats.bound_pieces,
            stats.direct_source_tests,
            cache.point_evaluations,
            cache.point_hits,
            cache.control_restrictions,
            cache.nodes_created,
            cache.nodes_reused,
            cache.exact_fallbacks,
            cache.device_points,
            cache.device_bounds,
            cache.device_restrictions,
            cache.uploaded_bytes + curves.uploaded_bytes,
            cache.downloaded_bytes + curves.downloaded_bytes,
            cache.host_reserved_bytes,
            cache.host_peak_bytes,
            cache.device_reserved_bytes,
            cache.device_peak_bytes,
            resource->peak()})
        queries << '\t' << value;
      for (double value :
           {stats.classification_seconds, stats.packet_seconds,
            stats.region_seconds, stats.bound_seconds, stats.anchor_seconds})
        queries << '\t' << value;
      queries << '\n';
    }
    session.release_result();
    boundary.release_result();
  }
  stage = "session.close";
  tick = Clock::now();
  session.close();
  boundary.close();
  timing(stage, tick);
  timing("through_session_close", full);
  close(queries);
  close(timings);
  stage = "output.publish";
  output.publish();
  std::cout << "adaptive_trimmed_face\tcomplete\tqueries\t"
            << o.positional.size() << "\tthrough_publication_seconds\t"
            << std::setprecision(17) << seconds(full) << '\n';
}
} // namespace
int main(int argc, char **argv) {
  std::string_view stage = "options";
  try {
    const auto o = options(argc, argv);
    if (o.help) {
      std::cout
          << "brepgpu-adaptive-trimmed --source FILE --definition ID --face ID "
             "--output "
             "NEW_DIRECTORY --positional-mm VALUE [--positional-mm VALUE ...] "
             "--node-source-mm VALUE --threads N "
             "[--backend p100 --cuda-module LIBRARY --device N]\n";
      return 0;
    }
    run(o, stage);
    return 0;
  } catch (const Error &error) {
    std::cerr << "error\t" << stage << '\t' << static_cast<unsigned>(error.code)
              << '\t' << error.what() << '\n';
  } catch (const bgfd::Failure &error) {
    std::cerr << "error\t" << stage << "\texact\t" << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error\t" << stage << "\tunexpected\t" << error.what() << '\n';
  }
  return 1;
}
