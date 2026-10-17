#include "adaptive_cache.hpp"
#include "adaptive_caller_io.hpp"
#include "adaptive_curves.hpp"
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
  uint64_t definition = 0, memory_mib = 1024;
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
      !(out.node_source > 0) || out.positional.empty() || out.threads == 0 ||
      out.threads > 256)
    invalid("required: --source --definition --output --positional-mm positive "
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
void run(const Options &o, std::string_view &stage) {
  const auto full = Clock::now();
  const uint64_t bytes = o.memory_mib << 20;
  const auto resource = std::make_shared<RunResource>(bytes);
  bgfd::Budget budget({bytes, uint64_t{1} << 44}, resource.get());
  OutputDirectory output(o.output);
  std::ofstream timings(output.path() / "timings.tsv"),
      queries(output.path() / "queries.tsv");
  if (!timings || !queries)
    throw Error(ErrorCode::io_failure, "cannot open shared-boundary metadata");
  timings << std::setprecision(17) << "scope\tseconds\n";
  auto timing = [&](std::string_view scope, Clock::time_point tick) {
    timings << scope << '\t' << seconds(tick) << '\n';
  };
  stage = "source.decode";
  auto tick = Clock::now();
  const auto source = load_source(o.source, resource);
  timing(stage, tick);
  const auto &s = source.view();
  const auto definition = s.definition_index.find(DefinitionId{o.definition});
  stage = "source.select_definition_curves";
  tick = Clock::now();
  const auto requests =
      caller::definition_curve_requests(source, definition, resource);
  timing(stage, tick);
  stage = "source.compile";
  tick = Clock::now();
  CurveCacheLimits cache_limits;
  cache_limits.control_scalars = o.max_nodes * 264;
  const auto cache = prepare_curve_source_cache(source, requests, resource, {},
                                                cache_limits, budget);
  timing(stage, tick);
  CurveLimits limits;
  limits.nodes = limits.frontier = limits.requests = o.max_nodes;
  limits.points = o.max_nodes * 4;
  limits.control_scalars = o.max_nodes * 264;
  limits.max_depth = o.max_depth;
  limits.cpu_threads = o.threads;
  limits.host_bytes = limits.device_bytes = bytes;
  limits.child_bytes = std::min(bytes, uint64_t{1} << 26);
  JobContext context({limits.child_bytes, limits.total_work}, resource.get(),
                     {});
  stage = "session.create";
  tick = Clock::now();
  AdaptiveBoundarySession session(resource, source, definition, cache, limits);
  timing(stage, tick);
  stage = "device.open";
  tick = Clock::now();
  if (o.p100)
    session.open_device({o.device, o.module});
  timing(stage, tick);
  queries << std::setprecision(17)
          << "query\tpositional_mm\tnode_source_mm\tsource_digest\tnumeric_"
             "profile\tbackend\tthreads"
             "\tsource_units_mm\tprepared_roots\toriginal_"
             "roots\tedges\tuses\tnode_rows\tphysical_intervals"
             "\tsites\tuse_intervals\tqueries\tepochs\tnodes_created\tnodes_"
             "reused\tpoint_evaluations"
             "\tpoint_hits\tcontrol_restrictions\tchord_tests\ttrace_"
             "tests\texact_fallbacks"
             "\trequested_splits\tinstalled_splits\tinvalidated_faces\tdevice_"
             "points\tdevice_bounds"
             "\tdevice_restrictions\tuploaded_bytes\tdownloaded_bytes\tsession_"
             "host_reserved_bytes"
             "\tsession_host_peak_upper_bytes\tdevice_reserved_bytes\tdevice_"
             "peak_bytes"
             "\trun_peak_including_device_reservations_bytes\n";
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
    session.begin({query + 1, values, positional, node_source, {}});
    while (true) {
      const auto state = session.step(context);
      if (state == StepState::done)
        break;
      if (state != StepState::ready)
        throw Error(ErrorCode::internal_error,
                    "shared-boundary session returned unexpected state");
      std::this_thread::yield();
    }
    timing(stage, tick);
    {
      const auto result = session.result();
      stage = "output.shared_boundary";
      tick = Clock::now();
      caller::write_curves(output.path() /
                               ("query-" + std::to_string(query) + ".bgacrv"),
                           *result, budget);
      timing(stage, tick);
      const auto counts = session.stats();
      queries << query << '\t' << o.positional[query] << '\t' << o.node_source
              << '\t';
      caller::digest(queries, s.source_digest);
      queries << "\trn64_anchor32_p128_v1\t" << (o.p100 ? "p100" : "cpu")
              << '\t' << o.threads << '\t' << s.units_mm << '\t'
              << cache.roots().size() << '\t' << cache.original_roots().size()
              << '\t' << result->boundary->edges.size() << '\t'
              << result->uses.size() << '\t' << result->boundary->nodes.size()
              << '\t' << result->physical_intervals.size() << '\t'
              << result->sites.size() << '\t' << result->intervals.size();
      for (uint64_t value : {counts.queries,
                             counts.epochs,
                             counts.nodes_created,
                             counts.nodes_reused,
                             counts.point_evaluations,
                             counts.point_hits,
                             counts.control_restrictions,
                             counts.chord_tests,
                             counts.trace_tests,
                             counts.exact_fallbacks,
                             counts.requested_splits,
                             counts.installed_splits,
                             counts.invalidated_faces,
                             counts.device_points,
                             counts.device_bounds,
                             counts.device_restrictions,
                             counts.uploaded_bytes,
                             counts.downloaded_bytes,
                             counts.host_reserved_bytes,
                             counts.host_peak_upper_bytes,
                             counts.device_reserved_bytes,
                             counts.device_peak_bytes})
        queries << '\t' << value;
      queries << '\t' << resource->peak() << '\n';
    }
    session.release_result();
  }
  stage = "session.close";
  tick = Clock::now();
  session.close();
  timing(stage, tick);
  timing("through_session_close", full);
  close(queries);
  close(timings);
  stage = "output.publish";
  output.publish();
  std::cout << "adaptive_shared_boundary\tcomplete\tqueries\t"
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
          << "brepgpu-adaptive-curves --source FILE --definition ID --output "
             "NEW_DIRECTORY "
             "--positional-mm VALUE [--positional-mm VALUE ...] "
             "--node-source-mm VALUE "
             "--threads N [--backend p100 --cuda-module LIBRARY --device N]\n";
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
