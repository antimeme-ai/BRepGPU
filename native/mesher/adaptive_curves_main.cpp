#include "adaptive_cache.hpp"
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
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    invalid("invalid unsigned integer option");
  return value;
}
double number(std::string_view text) {
  double value = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || !std::isfinite(value))
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
    } else if (key == "--threads" || key == "--max-nodes" || key == "--max-depth" ||
               key == "--device") {
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
  if (out.source.empty() || out.output.empty() || !out.definition || !(out.node_source > 0) ||
      out.positional.empty() || out.threads == 0 || out.threads > 256)
    invalid("required: --source --definition --output --positional-mm positive "
            "--node-source-mm positive "
            "--threads 1..256");
  for (double value : out.positional)
    if (value <= 0)
      invalid("positional allowance must be positive");
  if (!out.memory_mib || out.memory_mib > (UINT64_MAX >> 20) || !out.max_nodes ||
      out.max_nodes > UINT32_MAX / 264 || !out.max_depth || out.max_depth > 64)
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
class Wire final {
  std::ofstream out_;
  bgfd::Budget &budget_;

public:
  Wire(const std::filesystem::path &path, bgfd::Budget &budget)
      : out_(path, std::ios::binary), budget_(budget) {
    if (!out_)
      throw Error(ErrorCode::io_failure, "cannot open shared-boundary artifact");
  }
  void bytes(std::span<const std::byte> bytes) {
    budget_.charge(bytes.size());
    out_.write(reinterpret_cast<const char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!out_)
      throw Error(ErrorCode::io_failure, "shared-boundary artifact write failed");
  }
  template <unsigned count> void word(uint64_t value) {
    static_assert(count == 4 || count == 8);
    std::array<std::byte, 8> bytes{};
    for (unsigned i = 0; i < count; ++i)
      bytes[i] = static_cast<std::byte>((value >> (8 * i)) & 255);
    this->bytes(std::span(bytes).first(count));
  }
  void rational(ExactView value) {
    if (value.numerator.size() > UINT32_MAX || value.denominator.size() > UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted,
                  "shared-boundary rational exceeds wire count range");
    word<4>(static_cast<uint32_t>(value.sign));
    word<4>(value.numerator.size());
    word<4>(value.denominator.size());
    for (auto w : value.numerator)
      word<4>(w);
    for (auto w : value.denominator)
      word<4>(w);
  }
  void finish() { close(out_); }
};
void write_curves(const std::filesystem::path &path, const SharedCurveSnapshotData &mesh,
                  bgfd::Budget &budget) {
  const auto &s = mesh.source.view();
  const auto &b = *mesh.boundary;
  if (mesh.physical_intervals.size() != b.intervals.size() ||
      b.nodes.size() != b.constructed_revisions.size() || b.nodes.size() != b.sample_paths.size())
    throw Error(ErrorCode::internal_error, "incomplete shared-boundary row inventory");
  Wire out(path, budget);
  constexpr std::array magic{std::byte{'B'}, std::byte{'G'}, std::byte{'A'}, std::byte{'C'},
                             std::byte{'R'}, std::byte{'V'}, std::byte{'0'}, std::byte{'1'}};
  out.bytes(magic);
  out.word<4>(1);
  out.word<4>(1);
  for (uint64_t value : {s.definitions[mesh.definition.value].id.value, uint64_t(b.nodes.size()),
                         uint64_t(b.edges.size()), uint64_t(b.vertex_aliases.size()),
                         uint64_t(mesh.physical_intervals.size()), uint64_t(mesh.uses.size()),
                         uint64_t(mesh.sites.size()), uint64_t(mesh.intervals.size())})
    out.word<8>(value);
  out.bytes(s.source_digest);
  const auto rational = [&](ExactRef ref) { out.rational(mesh.values.view(ref)); };
  const auto optional_exact = [&](const std::optional<ExactRef> &ref) {
    out.word<4>(ref.has_value());
    if (ref)
      rational(*ref);
  };
  const auto key = [&](BoundaryKey value) {
    out.word<8>(value.owner);
    out.word<8>(value.interval);
    out.word<4>(static_cast<uint32_t>(value.kind));
  };
  const auto line = [&](const std::optional<GeometricLineRecord> &value) {
    out.word<4>(value.has_value());
    if (!value)
      return;
    for (auto ref : value->covector)
      rational(ref);
    for (auto ref : value->curve_scalar)
      rational(ref);
    for (auto ref : value->trace_scalar)
      rational(ref);
    for (auto ref : value->original_t)
      rational(ref);
    out.word<4>(static_cast<uint32_t>(value->traversal));
  };
  const auto range = [](uint64_t first, uint64_t count, uint64_t size) {
    if (first > size || count > size - first)
      throw Error(ErrorCode::internal_error, "shared-boundary output range outside retained rows");
  };
  for (std::size_t i = 0; i < b.nodes.size(); ++i) {
    const auto &node = b.nodes[i];
    if (!node.original_parameter)
      throw Error(ErrorCode::internal_error, "boundary node lacks original parameter");
    out.word<4>(static_cast<uint32_t>(node.key.generator));
    for (uint64_t value :
         {node.key.source, node.key.generation, node.key.ordinal, b.constructed_revisions[i]})
      out.word<8>(value);
    for (double value : node.xyz) {
      if (!std::isfinite(value))
        throw Error(ErrorCode::internal_error, "nonfinite boundary XYZ");
      out.word<8>(std::bit_cast<uint64_t>(value));
    }
    out.rational(b.parameters.view(*node.original_parameter));
    const auto path_bits = b.sample_paths[i];
    range(path_bits.first, path_bits.count, b.path_bits.size());
    if (path_bits.count > UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "boundary path exceeds wire index");
    out.word<4>(path_bits.count);
    for (uint64_t j = 0; j < path_bits.count; ++j) {
      const auto direction = b.path_bits[path_bits.first + j];
      if (direction > 1)
        throw Error(ErrorCode::internal_error, "invalid source sample path");
      const std::array byte{static_cast<std::byte>(direction)};
      out.bytes(byte);
    }
  }
  for (const auto &row : b.edges) {
    const auto &edge = s.edges.at(row.edge.value);
    range(row.nodes.first, row.nodes.count, b.nodes.size());
    range(row.intervals.first, row.intervals.count, b.intervals.size());
    for (uint64_t value :
         {edge.id.value, edge.curve_id.value, edge.vertex_ids[0].value, edge.vertex_ids[1].value,
          row.nodes.first, row.nodes.count, row.intervals.first, row.intervals.count})
      out.word<8>(value);
    for (double value : edge.parameter_range)
      out.word<8>(std::bit_cast<uint64_t>(value));
    for (uint32_t value : {uint32_t(edge.curve_same_sense), uint32_t(edge.same_parameter),
                           uint32_t(edge.degenerate), uint32_t{0}})
      out.word<4>(value);
  }
  for (const auto &alias : b.vertex_aliases) {
    out.word<8>(s.vertices.at(alias.original.value).id.value);
    out.word<8>(s.vertices.at(alias.representative.value).id.value);
  }
  for (std::size_t i = 0; i < mesh.physical_intervals.size(); ++i) {
    const auto &row = mesh.physical_intervals[i];
    if (row.boundary_key != b.intervals[i])
      throw Error(ErrorCode::internal_error, "physical interval ordering differs from boundary");
    key(row.boundary_key);
    for (auto node : row.boundary_nodes) {
      if (node >= b.nodes.size())
        throw Error(ErrorCode::internal_error, "physical interval references absent node");
      out.word<4>(node);
    }
    for (auto ref : row.original_t)
      rational(ref);
    rational(row.parameter_chord_upper);
    optional_exact(row.image_chord_upper);
    out.word<4>(static_cast<uint32_t>(row.selected_correspondence));
    line(row.image_map);
  }
  for (const auto &row : mesh.uses) {
    const auto &use = s.coedges.at(row.use.value);
    const auto &face = s.faces.at(row.face.value);
    range(row.sites.first, row.sites.count, mesh.sites.size());
    range(row.intervals.first, row.intervals.count, mesh.intervals.size());
    for (uint64_t value :
         {use.id.value, face.id.value, use.edge_id.value, face.carrier_id.value, use.loop_id.value})
      out.word<8>(value);
    out.word<4>(static_cast<uint32_t>(use.sense));
    out.word<4>(use.reconstructed);
    out.word<4>(use.parameter_map_kind);
    for (int32_t value : use.periodic_shift)
      out.word<4>(static_cast<uint32_t>(value));
    out.word<4>(static_cast<uint32_t>(row.authority));
    for (uint64_t value : {row.sites.first, row.sites.count, row.intervals.first,
                           row.intervals.count, row.face_generation, row.boundary_generation})
      out.word<8>(value);
  }
  for (const auto &row : mesh.sites) {
    if (row.boundary_node >= b.nodes.size())
      throw Error(ErrorCode::internal_error, "use site references absent boundary node");
    out.word<4>(row.boundary_node);
    out.word<8>(s.loops.at(row.loop.value).id.value);
    out.word<4>(row.junction_occurrence);
    for (int32_t value : row.original_periodic_shift)
      out.word<4>(static_cast<uint32_t>(value));
    for (int32_t value : row.selected_branch)
      out.word<4>(static_cast<uint32_t>(value));
    rational(row.original_t);
    rational(row.mapped_s);
    for (auto ref : row.chosen_original_uv)
      rational(ref);
    for (auto ref : row.chart_xy)
      rational(ref);
    rational(row.node_source_upper);
  }
  for (const auto &row : mesh.intervals) {
    key(row.boundary_key);
    for (auto site : row.sites) {
      if (site >= mesh.sites.size())
        throw Error(ErrorCode::internal_error, "use interval references absent site");
      out.word<4>(site);
    }
    out.word<4>(static_cast<uint32_t>(row.authority));
    out.word<4>(static_cast<uint32_t>(row.correspondence));
    rational(row.carrier_trace_to_actual_chord_upper);
    rational(row.source_to_selected_trace_upper);
    for (auto ref : row.uv_chord_component_upper)
      rational(ref);
    optional_exact(row.authoritative_domain_upper);
    line(row.image_map);
    out.word<4>(row.cylinder_identity.has_value());
    if (row.cylinder_identity) {
      const auto &identity = *row.cylinder_identity;
      out.word<8>(s.curves.at(identity.spatial.value).id.value);
      out.word<8>(s.carriers.at(identity.carrier.value).id.value);
      out.word<4>(static_cast<uint32_t>(identity.orientation));
      for (auto ref : identity.original_t)
        rational(ref);
      for (auto ref : identity.chosen_uv)
        rational(ref);
      rational(identity.constant_v);
    }
  }
  out.finish();
}
void digest(std::ostream &out, const SourceDigest &source) {
  constexpr std::string_view digits = "0123456789abcdef";
  for (auto byte : source) {
    const auto value = std::to_integer<unsigned>(byte);
    out << digits[value >> 4] << digits[value & 15];
  }
}
CurveRequest endpoint_curve_request(const SourceData &source, Slot<CurveTag> slot, double t) {
  const auto &curve = source.curves.at(slot.value);
  auto domain = curve.parameter_range;
  if (curve.kind == 4)
    domain = {0., 1.};
  if (curve.kind == 5) {
    const auto knots = source.view(curve.knots);
    const auto mult = source.view(curve.multiplicities);
    const auto controls = source.view(curve.controls).size() / (curve.dimension + 1);
    uint64_t expanded = 0;
    bool first = false, last = false;
    for (std::size_t i = 0; i < knots.size(); ++i) {
      const auto end = expanded + mult[i];
      if (expanded <= curve.degree && curve.degree < end) {
        domain[0] = knots[i];
        first = true;
      }
      if (expanded <= controls && controls < end) {
        domain[1] = knots[i];
        last = true;
      }
      expanded = end;
    }
    if (first && last)
      for (std::size_t i = 1; i < knots.size(); ++i)
        if (knots[i - 1] >= domain[0] && knots[i] <= domain[1] && knots[i - 1] < knots[i] &&
            knots[i - 1] <= t && t <= knots[i])
          return {slot, {knots[i - 1], knots[i]}};
    throw Error(ErrorCode::unsupported, "zero-range edge has no original positive spline span");
  }
  if (!(domain[0] < domain[1] && domain[0] <= t && t <= domain[1]))
    throw Error(ErrorCode::unsupported, "zero-range edge has no original positive curve span");
  return {slot, domain};
}
void run(const Options &o, std::string_view &stage) {
  const auto full = Clock::now();
  const uint64_t bytes = o.memory_mib << 20;
  const auto resource = std::make_shared<RunResource>(bytes);
  bgfd::Budget budget({bytes, uint64_t{1} << 44}, resource.get());
  OutputDirectory output(o.output);
  std::ofstream timings(output.path() / "timings.tsv"), queries(output.path() / "queries.tsv");
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
  std::pmr::vector<CurveRequest> requests(resource.get());
  for (auto edge_slot : s.definition_edges.at(definition.value)) {
    const auto &edge = s.edges.at(edge_slot);
    // A declared quotient has no positive physical interval, but its original
    // endpoint Ce facts still need a real original evaluator span.
    auto physical_domain = edge.parameter_range;
    if (physical_domain[1] < physical_domain[0])
      std::swap(physical_domain[0], physical_domain[1]);
    if (edge.curve_id) {
      const auto curve = s.curve_index.find(edge.curve_id);
      requests.push_back(physical_domain[0] < physical_domain[1]
                             ? CurveRequest{curve, physical_domain}
                             : endpoint_curve_request(s, curve, physical_domain[0]));
    }
    for (auto use_slot : s.edge_coedges.at(edge_slot)) {
      const auto &use = s.coedges.at(use_slot);
      const auto &loop = s.loops.at(s.loop_index.find(use.loop_id).value);
      const auto &face = s.faces.at(s.face_index.find(loop.face_id).value);
      if (face.definition_id != s.definitions[definition.value].id)
        continue;
      if (use.parameter_map_kind != 1 && use.parameter_map_kind != 2)
        throw Error(ErrorCode::unsupported, "unimplemented original use parameter map");
      // Keep every occurrence of THIS definition, not one guide per edge. A
      // kind2 map may demand values beyond the saved guide range.
      auto domain = physical_domain;
      if (use.parameter_map_kind == 2) {
        const auto map =
            std::span(s.parameter_maps).subspan(use.parameter_map.first, use.parameter_map.count);
        if (map.empty())
          invalid("empty original piecewise parameter map");
        domain = {map.front()[1], map.front()[1]};
        for (const auto &pair : map) {
          domain[0] = std::min(domain[0], pair[1]);
          domain[1] = std::max(domain[1], pair[1]);
        }
      }
      const auto guide = s.curve_index.find(use.curve_id);
      requests.push_back(domain[0] < domain[1] ? CurveRequest{guide, domain}
                                               : endpoint_curve_request(s, guide, domain[0]));
    }
  }
  timing(stage, tick);
  stage = "source.compile";
  tick = Clock::now();
  CurveCacheLimits cache_limits;
  cache_limits.control_scalars = o.max_nodes * 264;
  const auto cache =
      prepare_curve_source_cache(source, requests, resource, {}, cache_limits, budget);
  timing(stage, tick);
  CurveLimits limits;
  limits.nodes = limits.frontier = limits.requests = o.max_nodes;
  limits.points = o.max_nodes * 4;
  limits.control_scalars = o.max_nodes * 264;
  limits.max_depth = o.max_depth;
  limits.cpu_threads = o.threads;
  limits.host_bytes = limits.device_bytes = bytes;
  limits.child_bytes = std::min(bytes, uint64_t{1} << 26);
  JobContext context({limits.child_bytes, limits.total_work}, resource.get(), {});
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
    ExactBuilder builder(resource, {{OwnerKind::run, definition.value, 1}, query + 1, 0});
    const auto positional = builder.append(Rational::from_double(budget, o.positional[query]));
    const auto node_source = builder.append(Rational::from_double(budget, o.node_source));
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
        throw Error(ErrorCode::internal_error, "shared-boundary session returned unexpected state");
      std::this_thread::yield();
    }
    timing(stage, tick);
    {
      const auto result = session.result();
      stage = "output.shared_boundary";
      tick = Clock::now();
      write_curves(output.path() / ("query-" + std::to_string(query) + ".bgacrv"), *result, budget);
      timing(stage, tick);
      const auto counts = session.stats();
      queries << query << '\t' << o.positional[query] << '\t' << o.node_source << '\t';
      digest(queries, s.source_digest);
      queries << "\trn64_anchor32_p128_v1\t" << (o.p100 ? "p100" : "cpu") << '\t' << o.threads
              << '\t' << s.units_mm << '\t' << cache.roots().size() << '\t'
              << cache.original_roots().size() << '\t' << result->boundary->edges.size() << '\t'
              << result->uses.size() << '\t' << result->boundary->nodes.size() << '\t'
              << result->physical_intervals.size() << '\t' << result->sites.size() << '\t'
              << result->intervals.size();
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
  std::cout << "adaptive_shared_boundary\tcomplete\tqueries\t" << o.positional.size()
            << "\tthrough_publication_seconds\t" << std::setprecision(17) << seconds(full) << '\n';
}
} // namespace
int main(int argc, char **argv) {
  std::string_view stage = "options";
  try {
    const auto o = options(argc, argv);
    if (o.help) {
      std::cout << "brepgpu-adaptive-curves --source FILE --definition ID --output "
                   "NEW_DIRECTORY "
                   "--positional-mm VALUE [--positional-mm VALUE ...] "
                   "--node-source-mm VALUE "
                   "--threads N [--backend p100 --cuda-module LIBRARY --device N]\n";
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
