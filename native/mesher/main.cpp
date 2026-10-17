#include "mesh_io.hpp"
#include "output_directory.hpp"
#include "mesh_publish.hpp"
#include "meshing_run.hpp"
#include "presentation.hpp"
#include "native_display.hpp"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <linux/fs.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
using namespace bgmesh;
using Clock = std::chrono::steady_clock;
using Time = Clock::time_point;
double seconds(Time begin) { return std::chrono::duration<double>(Clock::now() - begin).count(); }
[[noreturn]] void invalid(const std::string &s) { throw Error(ErrorCode::invalid_source, s); }
struct Options final {
  std::filesystem::path source, output, bound_module, display_module;
  double positional = 0, node_source = 0, explosion = 2;
  uint32_t threads = 0, width = 1920, height = 1800;
  uint64_t memory_mib = 65536, owner_mib = 64, seed = 0;
  int32_t device = 0;
  bool p100 = false, mesh_only = false, help = false;
};
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
Options options(int argc, char **argv) {
  Options out;
  for (int i = 1; i < argc; ++i) {
    const std::string_view key(argv[i]);
    if (key == "--help") {
      out.help = true;
      continue;
    }
    if (key == "--mesh-only") {
      out.mesh_only = true;
      continue;
    }
    if (i + 1 == argc)
      invalid("missing option value: " + std::string(key));
    const std::string_view value(argv[++i]);
    if (key == "--source")
      out.source = value;
    else if (key == "--output")
      out.output = value;
    else if (key == "--bound-module")
      out.bound_module = value;
    else if (key == "--display-module")
      out.display_module = value;
    else if (key == "--positional-mm")
      out.positional = number(value);
    else if (key == "--node-source-mm")
      out.node_source = number(value);
    else if (key == "--explosion")
      out.explosion = number(value);
    else if (key == "--memory-mib")
      out.memory_mib = integer(value);
    else if (key == "--owner-mib")
      out.owner_mib = integer(value);
    else if (key == "--seed")
      out.seed = integer(value);
    else if (key == "--backend") {
      if (value != "cpu" && value != "p100")
        invalid("backend must be cpu or p100");
      out.p100 = value == "p100";
    } else if (key == "--threads" || key == "--width" || key == "--height" || key == "--device") {
      const auto n = integer(value);
      if (n > static_cast<uint64_t>(INT32_MAX))
        invalid("numeric option exceeds native slot range");
      if (key == "--threads")
        out.threads = static_cast<uint32_t>(n);
      else if (key == "--width")
        out.width = static_cast<uint32_t>(n);
      else if (key == "--height")
        out.height = static_cast<uint32_t>(n);
      else
        out.device = static_cast<int32_t>(n);
    } else
      invalid("unknown option: " + std::string(key));
  }
  if (out.help)
    return out;
  if (out.source.empty() || out.output.empty() || out.positional <= 0 || out.node_source <= 0 ||
      out.threads == 0 || out.threads > 256)
    invalid("required: --source --output --positional-mm positive --node-source-mm positive "
            "--threads 1..256");
  if (!out.width || !out.height || out.width > 8192 || out.height > 8192 || out.explosion < 0 ||
      out.explosion > 100)
    invalid("invalid image dimensions/explosion strength");
  if (!out.memory_mib || !out.owner_mib || out.memory_mib > (UINT64_MAX >> 20) ||
      out.owner_mib > (UINT64_MAX >> 20))
    invalid("invalid memory/owner MiB limits");
  if (out.p100 && (out.bound_module.empty() || (!out.mesh_only && out.display_module.empty())))
    invalid("p100 requires --bound-module and, for display, --display-module; no fallback");
  if (!out.p100 && (!out.bound_module.empty() || !out.display_module.empty()))
    invalid("CPU backend does not load device modules");
  out.output = std::filesystem::absolute(out.output).lexically_normal();
  return out;
}
struct Stage final {
  std::string_view name;
  double value;
};
class Ledger final {
  std::pmr::vector<Stage> rows_;

public:
  explicit Ledger(std::pmr::memory_resource *r) : rows_(r) {}
  void add(std::string_view name, double value) { rows_.push_back({name, value}); }
  const std::pmr::vector<Stage> &rows() const { return rows_; }
};
void checked_close(std::ofstream &out) {
  out.close();
  if (!out)
    throw Error(ErrorCode::io_failure, "metadata write/close failed");
}
void text(std::ostream &out, std::string_view value) {
  // Escaped TSV preserves original names, including their tabs/newlines.
  for (char c : value) {
    if (c == '\t')
      out << "\\t";
    else if (c == '\n')
      out << "\\n";
    else if (c == '\r')
      out << "\\r";
    else if (c == '\\')
      out << "\\\\";
    else
      out.put(c);
  }
}
void labels(const std::filesystem::path &path, const NativeMesh &mesh, bgfd::Budget &budget) {
  std::ofstream out(path);
  if (!out)
    throw Error(ErrorCode::io_failure, "cannot open labels TSV");
  out << "occurrence\tproduct_id\tproduct_name\tusage_ids\tusage_names\n";
  const auto &d = mesh.view();
  const auto &s = d.source.view();
  for (std::size_t i = 0; i < d.instances.size(); ++i) {
    budget.charge(1);
    const auto &instance = d.instances[i];
    const auto &product = s.products[s.product_index.find(instance.product).value];
    out << i + 1 << '\t' << product.id.value << '\t';
    text(out, s.text(product.name));
    out << '\t';
    for (uint64_t j = 0; j < instance.path.count; ++j) {
      if (j)
        out << '/';
      out << d.usage_paths[static_cast<std::size_t>(instance.path.first + j)].value;
    }
    out << '\t';
    for (uint64_t j = 0; j < instance.path.count; ++j) {
      if (j)
        out << '/';
      const auto id = d.usage_paths[static_cast<std::size_t>(instance.path.first + j)];
      text(out, s.text(s.usages[s.usage_index.find(id).value].name));
    }
    out << '\n';
  }
  checked_close(out);
}
void render_stages(Ledger &ledger, const bgom10::Times &t, bool exploded) {
  const std::array<std::string_view, 6> a{"assembled.project",  "assembled.setup",
                                          "assembled.bins",     "assembled.shading",
                                          "assembled.download", "assembled.render_total"};
  const std::array<std::string_view, 6> e{"exploded.project",  "exploded.setup",
                                          "exploded.bins",     "exploded.shading",
                                          "exploded.download", "exploded.render_total"};
  const auto &names = exploded ? e : a;
  const std::array values{t.project, t.setup, t.bins, t.display, t.download, 0.};
  for (std::size_t i = 0; i + 1 < names.size(); ++i)
    ledger.add(names[i], values[i] / 1000.);
}
struct Report final {
  double publication = 0, closed = 0, through_publication = 0, teardown = 0;
};
Report run(const Options &o, Time full, std::string_view &stage) {
  Report report;
  Time teardown;
  {
    const auto resource = std::make_shared<RunResource>(o.memory_mib << 20);
    bgfd::Budget budget({o.memory_mib << 20, uint64_t{1} << 48}, resource.get());
    Ledger ledger(&budget);
    stage = "backend_load";
    auto tick = Clock::now();
    std::shared_ptr<NativeDisplayModule> display_module;
    if (!o.mesh_only)
      display_module = std::make_shared<NativeDisplayModule>(
          o.p100 ? std::optional(o.display_module) : std::nullopt);
    std::optional<BoundCudaModule> bound;
    if (o.p100)
      bound = load_bound_cuda(resource, o.bound_module);
    ledger.add("backend.load", seconds(tick));
    stage = "temporary_output";
    OutputDirectory output(o.output);
    PreparedRun prepared;
    MeshedRun meshed;
    double source_elapsed = 0;
    uint64_t source_work = 0;
    {
      stage = "source_prepare";
      tick = Clock::now();
      NativeScheduler scheduler(resource, {o.threads, std::max(uint32_t{16}, o.threads * 4),
                                           uint64_t{1} << 30, uint64_t{1} << 44});
      auto preparation = std::allocate_shared<RunPreparationJob>(
          RunAllocator<RunPreparationJob>(resource), resource,
          RunPreparationInput{o.source, o.positional, o.node_source});
      scheduler.submit(preparation);
      scheduler.drain();
      const auto completion = scheduler.take();
      if (!completion)
        throw Error(ErrorCode::internal_error, "source preparation has no completion");
      completion->rethrow();
      if (completion->state != StepState::done)
        throw Error(ErrorCode::internal_error, "source preparation yielded unexpectedly");
      prepared = preparation->result();
      source_elapsed = seconds(tick);
      source_work = completion->exact_work;
      ledger.add("source_prepare.total", source_elapsed);
      const auto &t = prepared->times;
      ledger.add("source.decode", t.source_seconds);
      ledger.add("source.geometry", t.geometry_seconds);
      ledger.add("source.envelopes", t.envelopes_seconds);
      ledger.add("source.assembly", t.assembly_seconds);
      ledger.add("source.bound_bank", t.bank_seconds);
      MeshingRunLimits limits;
      limits.face.bytes = o.owner_mib << 20;
      limits.preparation.bytes = o.owner_mib << 20;
      limits.face.work = limits.preparation.work = uint64_t{1} << 44;
      limits.numerical_services = std::min(o.threads, uint32_t{8});
      limits.seed = o.seed;
      if (bound) {
        limits.cuda = *bound;
        limits.devices.push_back({o.device, uint64_t{1} << 28, uint64_t{1} << 28});
      }
      stage = "meshing";
      tick = Clock::now();
      meshed = mesh_prepared_run(prepared, scheduler, limits);
      ledger.add("meshing.total", seconds(tick));
      stage = "scheduler_close";
      tick = Clock::now();
    }
    ledger.add("scheduler.close", seconds(tick));
    stage = "publish_values";
    tick = Clock::now();
    const auto mesh = publish_native_mesh(budget, prepared, meshed->boundaries, meshed->faces,
                                          prepared->requested);
    ledger.add("output.values", seconds(tick));
    stage = "semantic_write";
    tick = Clock::now();
    write_native_mesh(output.path() / "mesh.bgnmesh", mesh);
    ledger.add("output.semantic_write", seconds(tick));
    stage = "labels";
    tick = Clock::now();
    labels(output.path() / "labels.tsv", mesh, budget);
    ledger.add("output.labels", seconds(tick));
    if (!o.mesh_only) {
      stage = "source_scene";
      tick = Clock::now();
      const auto scene = make_scene(budget, mesh, prepared->geometry);
      ledger.add("output.scene_build", seconds(tick));
      stage = "presentation";
      tick = Clock::now();
      const auto presentation = make_presentation(budget, scene, o.width, o.height, o.explosion);
      ledger.add("output.presentation", seconds(tick));
      NativeDisplay display(resource, display_module, scene, presentation);
      stage = "display_files";
      tick = Clock::now();
      write_scene(output.path() / "scene.bgscene", display.view(), budget);
      write_views(output.path() / "views.bgview", presentation.views, budget);
      ledger.add("output.scene_views_write", seconds(tick));
      stage = "display_create";
      bgom10::Times startup{};
      tick = Clock::now();
      display.open(o.threads, o.device, startup);
      ledger.add("display.create", startup.create / 1000.);
      ledger.add("display.upload", startup.upload / 1000.);
      stage = "display_evaluate";
      display.evaluate(startup);
      ledger.add("display.evaluate", startup.evaluate / 1000.);
      const double display_startup = seconds(tick);
      for (std::size_t frame = 0; frame < 2; ++frame) {
        stage = frame ? "exploded_render" : "assembled_render";
        tick = Clock::now();
        bgom10::Times timing{};
        display.render(presentation.views[frame], timing);
        const auto elapsed = seconds(tick);
        render_stages(ledger, timing, frame != 0);
        ledger.add(frame ? "exploded.render_total" : "assembled.render_total", elapsed);
        std::ostringstream caption;
        caption << std::fixed << std::setprecision(3) << "SOURCE " << source_elapsed << " S MESH "
                << meshed->stats.seconds << " S DISPLAY " << display_startup + elapsed << " S EPS "
                << std::scientific << std::setprecision(2) << o.positional << " DELTA "
                << o.node_source;
        const auto heading =
            std::string(o.p100 ? "BREPGPU P100 " : "BREPGPU CPU ") +
            (o.p100 ? std::to_string(o.device) : std::to_string(o.threads) + " THREADS") +
            (frame ? " EXPLODED" : " ASSEMBLED");
        annotate_frame(budget, display.pixels(), presentation.views[frame], presentation,
                       frame != 0, {heading, caption.str()});
        stage = frame ? "exploded_png" : "assembled_png";
        tick = Clock::now();
        write_png(output.path() / (frame ? "exploded.png" : "assembled.png"), o.width, o.height,
                  display.pixels(), budget);
        ledger.add(frame ? "exploded.png" : "assembled.png", seconds(tick));
      }
      stage = "display_close";
      bgom10::Times closing{};
      display.close(closing);
      ledger.add("display.close", closing.close / 1000.);
    }
    stage = "timings_write";
    ledger.add("whole.prepublication", seconds(full));
    std::ofstream metadata(output.path() / "timings.tsv");
    if (!metadata)
      throw Error(ErrorCode::io_failure, "cannot open timing TSV");
    metadata << "kind\tname\tvalue\tunit\n" << std::setprecision(17);
    for (const auto &row : ledger.rows())
      metadata << "stage\t" << row.name << '\t' << row.value << "\tseconds\n";
    const auto count = [&](std::string_view name, uint64_t value) {
      metadata << "count\t" << name << '\t' << value << "\tcount\n";
    };
    count("nodes", mesh.view().nodes.size());
    count("facets", mesh.view().facets.size());
    count("instances", mesh.view().instances.size());
    count("threads", o.threads);
    count("memory_mib", o.memory_mib);
    count("owner_mib", o.owner_mib);
    count("seed", o.seed);
    count("numerical_cpu_tasks", meshed->stats.numerical_cpu_tasks);
    count("numerical_device_tasks", meshed->stats.numerical_device_tasks);
    count("numerical_exact_tasks", meshed->stats.numerical_exact_tasks);
    const auto work = [&](std::string_view name, uint64_t value) {
      metadata << "work\t" << name << '\t' << value << "\toperations\n";
    };
    work("source_prepare", source_work);
    work("scheduler", meshed->stats.scheduler_work);
    work("owner", meshed->stats.owner_work);
    work("numerical", meshed->stats.numerical_work);
    count("boundary_rounds", meshed->stats.boundary_rounds);
    count("contact_rounds", meshed->stats.contact_rounds);
    count("contact_candidates", meshed->stats.contact_candidates);
    count("boundary_points", meshed->stats.boundary_points);
    count("optimizer_proposals", meshed->stats.optimizer_proposals);
    count("optimizer_flips", meshed->stats.optimizer_flips);
    metadata << "memory\trun_peak_including_reservations\t" << resource->peak() << "\tbytes\n";
    metadata << "quality\tpositional_requested\t" << o.positional
             << "\tmm\nquality\tnode_source_requested\t" << o.node_source << "\tmm\n";
    metadata << "config\tsource_original_digest\t";
    static constexpr char hex[] = "0123456789abcdef";
    for (const auto word : prepared->geometry.view().source.view().source_digest) {
      const auto value = std::to_integer<unsigned int>(word);
      metadata << hex[value >> 4] << hex[value & 15U];
    }
    metadata << "\tsha256\n";
    metadata << "config\tbackend\t" << (o.p100 ? "p100" : "cpu") << "\ttext\nconfig\tscope\t"
             << (o.mesh_only ? "mesh_only" : "mesh_and_display") << "\ttext\n";
    metadata << "config\texplosion_strength\t" << o.explosion << "\tratio\n";
    checked_close(metadata);
    report.closed = seconds(full);
    stage = "publication";
    tick = Clock::now();
    output.publish();
    report.publication = seconds(tick);
    report.through_publication = seconds(full);
    teardown = Clock::now();
  }
  report.teardown = seconds(teardown);
  return report;
}
std::string_view code(ErrorCode value) {
  switch (value) {
  case ErrorCode::invalid_source:
    return "invalid_source";
  case ErrorCode::unsupported:
    return "unsupported";
  case ErrorCode::resource_exhausted:
    return "resource_exhausted";
  case ErrorCode::unresolved:
    return "unresolved";
  case ErrorCode::cancelled:
    return "cancelled";
  case ErrorCode::device_failure:
    return "device_failure";
  case ErrorCode::io_failure:
    return "io_failure";
  case ErrorCode::internal_error:
    return "internal_error";
  }
  return "unknown";
}
} // namespace
int main(int argc, char **argv) {
  const auto full = Clock::now();
  std::string_view stage = "options";
  try {
    const auto request = options(argc, argv);
    if (request.help) {
      std::cout << "brepgpu-mesh --source BGSRC --output NEW_DIRECTORY --positional-mm X "
                   "--node-source-mm X --threads N [--backend cpu|p100 --bound-module SO "
                   "--display-module SO --device N] [--mesh-only --memory-mib 65536 --owner-mib 64 "
                   "--width 1920 --height 1800 --explosion 2 --seed 0]\n";
      return 0;
    }
    const auto result = run(request, full, stage);
    std::cout << std::setprecision(17) << "PUBLISHED\t" << request.output.string()
              << "\nrequired_output_through_close_seconds\t" << result.closed
              << "\npublication_seconds\t" << result.publication
              << "\nthrough_publication_seconds\t" << result.through_publication
              << "\nresource_teardown_seconds\t" << result.teardown << '\n';
    return 0;
  } catch (const Error &e) {
    std::cerr << "ERROR\t" << stage << '\t' << code(e.code) << '\t' << e.what() << '\n';
  } catch (const bgfd::Cancelled &) {
    std::cerr << "ERROR\t" << stage << "\tcancelled\tnative work cancelled\n";
  } catch (const bgfd::Failure &e) {
    std::cerr << "ERROR\t" << stage << "\tresource_or_exact_failure\t" << e.what() << '\n';
  } catch (const std::exception &e) {
    std::cerr << "ERROR\t" << stage << "\tnative_failure\t" << e.what() << '\n';
  }
  return 1;
}
