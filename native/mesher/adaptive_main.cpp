#include "adaptive_cache.hpp"
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
  uint64_t carrier = 0, memory_mib = 1024;
  uint32_t threads = 0, max_nodes = 1u << 18, max_depth = 64;
  int32_t device = 0;
  bool p100 = false, help = false;
  std::vector<double> positional;
  std::array<std::optional<double>, 4> rectangle;
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
    else if (key == "--carrier")
      out.carrier = integer(value);
    else if (key == "--memory-mib")
      out.memory_mib = integer(value);
    else if (key == "--positional-mm")
      out.positional.push_back(number(value));
    else if (key == "--u0" || key == "--u1" || key == "--v0" || key == "--v1") {
      const std::size_t slot = key == "--u0"   ? 0
                               : key == "--u1" ? 1
                               : key == "--v0" ? 2
                                               : 3;
      if (out.rectangle[slot])
        invalid("duplicate rectangle option");
      out.rectangle[slot] = number(value);
    } else if (key == "--backend") {
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
  if (out.source.empty() || out.output.empty() || !out.carrier ||
      out.positional.empty() || out.threads == 0 || out.threads > 256)
    invalid("required: --source --carrier --output --positional-mm positive "
            "--threads 1..256");
  for (double value : out.positional)
    if (value <= 0)
      invalid("positional allowance must be positive");
  if (!out.memory_mib || out.memory_mib > (UINT64_MAX >> 20) ||
      !out.max_nodes || out.max_nodes > UINT32_MAX / 256 || !out.max_depth ||
      out.max_depth > 64)
    invalid("invalid memory, node capacity or depth64 limit");
  const auto rectangle_count =
      std::count_if(out.rectangle.begin(), out.rectangle.end(),
                    [](const auto &v) { return v.has_value(); });
  if (rectangle_count != 0 && rectangle_count != 4)
    invalid("rectangle requires all of --u0 --u1 --v0 --v1");
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
      throw Error(ErrorCode::io_failure, "cannot open patch artifact");
  }
  void bytes(std::span<const std::byte> bytes) {
    budget_.charge(bytes.size());
    out_.write(reinterpret_cast<const char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!out_)
      throw Error(ErrorCode::io_failure, "patch artifact write failed");
  }
  void word(uint64_t value, unsigned count) {
    std::array<std::byte, 8> bytes{};
    for (unsigned i = 0; i < count; ++i)
      bytes[i] = static_cast<std::byte>((value >> (8 * i)) & 255);
    this->bytes(std::span(bytes).first(count));
  }
  void rational(ExactView value) {
    if (value.numerator.size() > UINT32_MAX ||
        value.denominator.size() > UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted,
                  "patch rational exceeds wire count range");
    word(static_cast<uint32_t>(value.sign), 4);
    word(value.numerator.size(), 4);
    word(value.denominator.size(), 4);
    for (auto w : value.numerator)
      word(w, 4);
    for (auto w : value.denominator)
      word(w, 4);
  }
  void finish() { close(out_); }
};
void write_patch(const std::filesystem::path &path, const PacketMeshData &mesh,
                 uint64_t carrier, bgfd::Budget &budget) {
  const auto &selection = *mesh.selection;
  if (selection.points.size() > UINT32_MAX || mesh.triangles.empty() ||
      !mesh.demoted_leaf_slots.empty())
    throw Error(ErrorCode::internal_error,
                "cannot publish incomplete patch result");
  Wire out(path, budget);
  constexpr std::array magic{std::byte{'B'}, std::byte{'G'}, std::byte{'A'},
                             std::byte{'P'}, std::byte{'C'}, std::byte{'H'},
                             std::byte{'0'}, std::byte{'1'}};
  out.bytes(magic);
  out.word(1, 4);
  out.word(selection.source.profile().revision, 4);
  out.word(carrier, 8);
  out.word(selection.points.size(), 8);
  out.word(mesh.triangles.size(), 8);
  out.bytes(selection.source.source().view().source_digest);
  for (const auto &point : selection.points) {
    for (double value : point.actual_xyz) {
      if (!std::isfinite(value))
        throw Error(ErrorCode::internal_error, "nonfinite patch point");
      out.word(std::bit_cast<uint64_t>(value), 8);
    }
    for (auto value : point.original_uv)
      out.rational(selection.values.view(value));
  }
  for (const auto &triangle : mesh.triangles)
    for (auto point : triangle.points) {
      if (point.value >= selection.points.size())
        throw Error(ErrorCode::internal_error,
                    "patch point index outside result");
      out.word(point.value, 4);
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
std::array<double, 2> natural_axis(const SourceData &source, Range<double> knot_range,
                                  Range<uint32_t> multiplicity_range, uint32_t degree,
                                  uint32_t control_count) {
  const auto knots = source.view(knot_range);
  const auto multiplicities = source.view(multiplicity_range);
  if (knots.size() != multiplicities.size() || control_count <= degree)
    invalid("invalid original rational knot axis");
  std::array<std::optional<double>, 2> endpoints;
  uint64_t expanded = 0;
  for (std::size_t i = 0; i < knots.size(); ++i) {
    const auto next = expanded + multiplicities[i];
    if (expanded <= degree && degree < next)
      endpoints[0] = knots[i];
    if (expanded <= control_count && control_count < next)
      endpoints[1] = knots[i];
    expanded = next;
  }
  if (!endpoints[0] || !endpoints[1] || *endpoints[0] >= *endpoints[1])
    invalid("empty original rational knot domain");
  return {*endpoints[0], *endpoints[1]};
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
    throw Error(ErrorCode::io_failure, "cannot open patch metadata");
  timings << std::setprecision(17) << "scope\tseconds\n";
  auto timing = [&](std::string_view scope, Clock::time_point tick) {
    timings << scope << '\t' << seconds(tick) << '\n';
  };
  stage = "source.decode";
  auto tick = Clock::now();
  const auto source = load_source(o.source, resource);
  timing(stage, tick);
  const auto slot = source.view().carrier_index.find(CarrierId{o.carrier});
  const auto &carrier = source.view().carriers[slot.value];
  auto rectangle = carrier.uv_bounds;
  if (!o.rectangle[0] && carrier.kind == 6) {
    const auto u = natural_axis(source.view(), carrier.knot_u, carrier.mult_u,
                                carrier.degrees[0], carrier.control_shape[0]);
    const auto v = natural_axis(source.view(), carrier.knot_v, carrier.mult_v,
                                carrier.degrees[1], carrier.control_shape[1]);
    rectangle = {u[0], u[1], v[0], v[1]};
  }
  if (o.rectangle[0])
    for (std::size_t i = 0; i < 4; ++i)
      rectangle[i] = *o.rectangle[i];
  if (!(rectangle[0] < rectangle[1] && rectangle[2] < rectangle[3]))
    invalid("requested source rectangle must have positive area");
  stage = "source.compile";
  tick = Clock::now();
  const CarrierRequest request{slot, rectangle};
  const auto cache = prepare_source_cache(source, std::span(&request, 1),
                                          resource, {}, {}, budget);
  timing(stage, tick);
  if (cache.profile().revision != 1)
    throw Error(ErrorCode::unsupported,
                "caller requires rn64_anchor32_p128_v1");
  CacheLimits limits;
  limits.nodes = limits.frontier = o.max_nodes;
  limits.points = o.max_nodes * 4;
  limits.control_scalars = o.max_nodes * 256;
  limits.max_axis_depth = o.max_depth;
  limits.cpu_threads = o.threads;
  limits.host_bytes = limits.device_bytes = bytes;
  limits.child_bytes = std::min(bytes, uint64_t{1} << 26);
  JobContext context({limits.child_bytes, limits.total_work}, resource.get(),
                     {});
  stage = "session.create";
  tick = Clock::now();
  PatchSession session(resource, cache, limits);
  timing(stage, tick);
  stage = "device.open";
  tick = Clock::now();
  if (o.p100)
    session.open_device({o.device, o.module});
  timing(stage, tick);
  queries << std::setprecision(17)
          << "query\tpositional_mm\tsource_digest\tnumeric_"
             "profile\tbackend\tthreads"
             "\tsource_units_mm\tu0\tu1\tv0\tv1\tprepared_roots\toriginal_roots\tpoints\tfacets"
             "\tnodes_created\tnodes_reused\tpoint_evaluations\tpoint_"
             "hits\tcontrol_restrictions"
             "\texact_fallbacks\tdevice_points\tdevice_bounds\tdevice_"
             "restrictions\tdevice_epochs"
             "\tuploaded_bytes\tdownloaded_bytes\thost_reserved_bytes\thost_"
             "peak_bytes"
             "\tdevice_reserved_bytes\tdevice_peak_bytes\tpacket_"
             "rounds\tactual_facet_tests"
             "\tactual_facet_failures\n";
  for (std::size_t query = 0; query < o.positional.size(); ++query) {
    stage = "query.prepare";
    tick = Clock::now();
    ExactBuilder builder(resource, {{OwnerKind::run, 0, 1}, query + 1, 0});
    const auto allowance = builder.append(
        Rational::from_double(budget, o.positional[query])
            .divide(Rational::from_double(budget, source.view().units_mm)));
    const auto values = builder.seal();
    std::pmr::vector<RootAllowance> allowances(resource.get());
    for (uint32_t i = 0; i < cache.roots().size(); ++i)
      allowances.push_back({RootSlot{i}, allowance});
    timing(stage, tick);
    stage = "query.construct_bound_packets";
    tick = Clock::now();
    session.begin({query + 1, cache, values, allowances});
    while (true) {
      const auto state = session.step(context);
      if (state == StepState::done)
        break;
      if (state != StepState::ready)
        throw Error(ErrorCode::internal_error,
                    "adaptive session returned unexpected state");
      std::this_thread::yield();
    }
    timing(stage, tick);
    {
      const auto mesh = session.result();
      stage = "output.patch";
      tick = Clock::now();
      write_patch(output.path() /
                      ("query-" + std::to_string(query) + ".bgapch"),
                  *mesh, o.carrier, budget);
      timing(stage, tick);
      const auto s = session.stats();
      queries << query << '\t' << o.positional[query] << '\t';
      digest(queries, source.view().source_digest);
      queries << "\trn64_anchor32_p128_v1\t" << (o.p100 ? "p100" : "cpu")
              << '\t' << o.threads << '\t' << source.view().units_mm;
      for (double value : rectangle)
        queries << '\t' << value;
      queries << '\t' << cache.roots().size() << '\t'
              << cache.original_roots().size() << '\t'
              << mesh->selection->points.size() << '\t'
              << mesh->triangles.size();
      for (auto value :
           {s.nodes_created, s.nodes_reused, s.point_evaluations, s.point_hits,
            s.control_restrictions, s.exact_fallbacks, s.device_points,
            s.device_bounds, s.device_restrictions, s.device_epochs,
            s.uploaded_bytes, s.downloaded_bytes, s.host_reserved_bytes,
            s.host_peak_bytes, s.device_reserved_bytes, s.device_peak_bytes,
            s.packet_rounds, s.actual_facet_tests, s.actual_facet_failures})
        queries << '\t' << value;
      queries << '\n';
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
  std::cout << "adaptive_patch\tcomplete\tqueries\t" << o.positional.size()
            << "\tthrough_publication_seconds\t" << std::setprecision(17)
            << seconds(full) << '\n';
}
} // namespace

int main(int argc, char **argv) {
  std::string_view stage = "options";
  try {
    const auto o = options(argc, argv);
    if (o.help) {
      std::cout
          << "brepgpu-adaptive-patch --source FILE --carrier ID --output "
             "NEW_DIRECTORY "
             "--positional-mm VALUE [--positional-mm VALUE ...] --threads N "
             "[--u0 U0 --u1 U1 --v0 V0 --v1 V1] "
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
