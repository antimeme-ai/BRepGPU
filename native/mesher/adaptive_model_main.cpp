#include "adaptive_assembly.hpp"
#include "adaptive_caller_io.hpp"
#include "adaptive_component.hpp"
#include "adaptive_component_io.hpp"
#include "adaptive_scene.hpp"
#include "adaptive_trimmed_io.hpp"
#include "native_display.hpp"
#include "output_directory.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
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
  std::filesystem::path source, output, module, display_module;
  uint64_t memory_mib = 1024;
  std::vector<uint64_t> definitions;
  uint32_t width = 1920, height = 1800;
  double explosion = 2.;
  uint64_t construction_work = TrimFaceLimits{}.construction_work;
  uint32_t threads = 0, max_nodes = 1u << 18, max_depth = 64;
  std::optional<uint32_t> face_owners;
  int32_t device = 0;
  double node_source = 0;
  std::vector<double> positional;
  bool p100 = false, mesh_only = false, help = false;
};
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
      invalid("missing option value");
    const std::string_view value(argv[++i]);
    if (key == "--source")
      out.source = value;
    else if (key == "--output")
      out.output = value;
    else if (key == "--cuda-module")
      out.module = value;
    else if (key == "--definition")
      out.definitions.push_back(integer(value));
    else if (key == "--display-module")
      out.display_module = value;
    else if (key == "--explosion")
      out.explosion = number(value);
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
               key == "--max-depth" || key == "--device" || key == "--width" || key == "--height") {
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
      else if (key == "--width")
        out.width = static_cast<uint32_t>(n);
      else if (key == "--height")
        out.height = static_cast<uint32_t>(n);
      else
        out.device = static_cast<int32_t>(n);
    } else
      invalid("unknown option");
  }
  if (out.help)
    return out;
  if (out.source.empty() || out.output.empty() || out.positional.empty() ||
      !(out.node_source > 0) || !out.threads || out.threads > 256)
    invalid("required: --source --output --positional-mm positive "
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
  if ((!out.p100 && !out.display_module.empty()) ||
      (out.p100 && !out.mesh_only && out.display_module.empty()))
    invalid("p100 display requires --display-module; CPU does not load device modules");
  if (!out.width || !out.height || out.width > 8192 || out.height > 8192 || out.explosion < 0. ||
      out.explosion > 100.)
    invalid("invalid image dimensions/explosion strength");
  if (std::any_of(out.definitions.begin(), out.definitions.end(), [](auto id) { return !id; }))
    invalid("zero selected definition ID");
  out.output = std::filesystem::absolute(out.output).lexically_normal();
  return out;
}
void escaped(std::ostream &out, std::string_view value) {
  for (const auto c : value) {
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
void labels(const std::filesystem::path &path, const AdaptiveScene &scene, bgfd::Budget &budget) {
  std::ofstream out(path);
  if (!out)
    throw Error(ErrorCode::io_failure, "cannot open assembly labels");
  out << "label\toccurrence\tdefinition\tproduct_id\tproduct_name\tusage_ids\tusage_names\n";
  const auto &assembly = scene->assembly.view();
  const auto &source = scene->source.view();
  for (uint32_t i = 0; i < scene->original_instances.size(); ++i) {
    budget.charge(1);
    const auto original = scene->original_instances[i];
    const auto &instance = assembly.instances.at(original);
    const auto &product = source.products[source.product_index.find(instance.product).value];
    out << i + 1 << '\t' << uint64_t(original) + 1 << '\t'
        << source.definitions[instance.definition.value].id.value << '\t' << product.id.value
        << '\t';
    escaped(out, source.text(product.name));
    out << '\t';
    for (uint64_t j = 0; j < instance.path.count; ++j) {
      if (j)
        out << '/';
      out << assembly.paths.at(instance.path.first + j).value;
    }
    out << '\t';
    for (uint64_t j = 0; j < instance.path.count; ++j) {
      if (j)
        out << '/';
      const auto id = assembly.paths.at(instance.path.first + j);
      escaped(out, source.text(source.usages[source.usage_index.find(id).value].name));
    }
    out << '\n';
  }
  caller::close_stream(out);
}
void run(const Options &o, std::string_view &stage) {
  const auto full = Clock::now();
  const uint64_t bytes = o.memory_mib << 20;
  const auto resource = std::make_shared<RunResource>(bytes);
  JobContext context({bytes, uint64_t{1} << 44}, resource.get(), {});
  auto &budget = context.budget;
  OutputDirectory output(o.output);
  std::ofstream timing(output.path() / "timings.tsv"), queries(output.path() / "queries.tsv"),
      faces(output.path() / "faces.tsv"), metadata(output.path() / "model.tsv");
  if (!timing || !queries || !faces || !metadata)
    throw Error(ErrorCode::io_failure, "cannot open model metadata");
  timing << std::setprecision(17) << "scope\tseconds\n";
  const auto record = [&](std::string_view name, Clock::time_point start) {
    const auto seconds = elapsed(start);
    timing << name << '\t' << seconds << '\n';
    timing.flush();
    if (!timing)
      throw Error(ErrorCode::io_failure, "model timing write failed");
    return seconds;
  };
  stage = "source.decode";
  auto tick = Clock::now();
  const auto source = load_source(o.source, resource);
  const auto &original = source.view();
  const auto decode_seconds = record(stage, tick);
  constexpr OwnerKey caller_owner{OwnerKind::run, UINT32_MAX - 17, 1};
  stage = "source.envelopes_and_initial_assembly";
  tick = Clock::now();
  const auto prepared =
      prepare_adaptive_assembly(budget, source, resource,
                                {Rational::from_double(budget, o.positional.front()),
                                 Rational::from_double(budget, o.node_source)},
                                {{caller_owner, 1, 0}, {caller_owner, 1, 1}});
  const auto envelope_seconds = record(stage, tick);
  std::pmr::vector<uint8_t> placed(original.definitions.size(), 0, resource.get());
  for (const auto &instance : prepared->assembly.view().instances) {
    budget.charge(1);
    placed.at(instance.definition.value) = 1;
  }
  std::pmr::vector<Slot<DefinitionTag>> definitions(resource.get());
  if (o.definitions.empty()) {
    for (uint32_t i = 0; i < placed.size(); ++i)
      if (placed[i])
        definitions.push_back({i});
  } else {
    for (const auto id : o.definitions) {
      const auto slot = original.definition_index.find(DefinitionId{id});
      if (!placed.at(slot.value))
        invalid("selected definition has no original placement");
      definitions.push_back(slot);
    }
  }
  std::sort(definitions.begin(), definitions.end(), [&](auto a, auto b) {
    return original.definitions[a.value].id < original.definitions[b.value].id;
  });
  if (definitions.empty() ||
      std::adjacent_find(definitions.begin(), definitions.end()) != definitions.end())
    invalid("model requires nonempty unique placed definitions");
  for (const auto def : definitions)
    if (original.definition_faces.at(def.value).empty())
      invalid("placed definition has no faces");
  stage = "source.compile_banks";
  tick = Clock::now();
  std::pmr::vector<CarrierRequest> carriers(resource.get());
  std::pmr::vector<uint8_t> seen(original.carriers.size(), 0, resource.get());
  for (const auto def : definitions) {
    for (const auto f : original.definition_faces.at(def.value)) {
      budget.charge(1);
      const auto &row = prepared->faces.at(f);
      const auto &profile = prepared->carriers.at(row.carrier_envelope);
      if (row.source.value != f ||
          profile.source != original.carrier_index.find(original.faces[f].carrier_id))
        throw Error(ErrorCode::internal_error, "model original face profile mismatch");
      if (!seen[profile.source.value]) {
        seen[profile.source.value] = 1;
        carriers.push_back({profile.source, profile.cache_domain});
      }
    }
  }
  std::sort(carriers.begin(), carriers.end(),
            [](const auto &a, const auto &b) { return a.source.value < b.source.value; });
  SourceCacheLimits source_limits;
  source_limits.roots = o.max_nodes;
  source_limits.control_scalars = o.max_nodes * 264;
  // Singular banks remain refused until the produced cap-domain runtime is connected.
  const auto cache =
      prepare_quotient_trim_source_cache(source, carriers, resource, {}, source_limits, budget);
  CurveCacheLimits curve_limits;
  curve_limits.control_scalars = o.max_nodes * 264;
  std::pmr::vector<CurveSourceCache> curve_banks(resource.get());
  curve_banks.reserve(definitions.size());
  for (const auto def : definitions) {
    const auto requests = caller::definition_curve_requests(source, def, resource);
    curve_banks.push_back(
        prepare_curve_source_cache(source, requests, resource, {}, curve_limits, budget));
  }
  const auto source_seconds = decode_seconds + envelope_seconds + record(stage, tick);
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
  std::shared_ptr<NativeDisplayModule> display_module;
  if (!o.mesh_only)
    display_module = std::make_shared<NativeDisplayModule>(o.p100 ? std::optional(o.display_module)
                                                                  : std::nullopt);
  record(stage, tick);
  metadata << "key\tvalue\nscope\t"
           << (o.definitions.empty() ? "complete_assembly" : "selected_components")
           << "\nsource_digest\t";
  caller::digest(metadata, original.source_digest);
  metadata << std::setprecision(17) << "\nsource_units_mm\t" << original.units_mm << "\nbackend\t"
           << (o.p100 ? "p100" : "cpu") << "\nthreads\t" << o.threads << "\nface_owner_limit\t"
           << limits.face_owners << "\nnumeric_profile\t" << cache.profile().revision
           << "\nselected_definitions\t" << definitions.size() << "\nsource_definitions\t"
           << original.definitions.size() << "\nsource_faces\t" << original.faces.size()
           << "\nsource_placements\t" << prepared->assembly.view().instances.size()
           << "\ncurve_service\tcold_per_component\n";
  faces << "query\tdefinition\tface\tmesh\tboundary\tboundary_epoch\tface_boundary_generation\n";
  queries << std::setprecision(17)
          << "query\tdefinition\trequested_positional_mm\trequested_node_mm\tlocal_positional_"
             "exact\tlocal_node_exact"
             "\tfaces\tfacets\tassignments\tboundary_rounds\tretained_faces\tcontact_candidate_"
             "pairs\tcontact_forbidden_pairs"
             "\tcurve_device_points_query\tcurve_device_bounds_query\tcurve_uploaded_bytes_"
             "query\tcurve_downloaded_bytes_query"
             "\tsurface_device_points_cumulative\tsurface_device_bounds_cumulative\tsurface_"
             "uploaded_bytes_cumulative"
             "\tsurface_downloaded_bytes_cumulative\tsurface_cap_facet_tests_cumulative"
             "\tsurface_device_cap_facets_cumulative\tsurface_domain_replacements_cumulative"
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
          const auto value = [&](std::size_t a) {
            return import_exact(budget, prepared->envelope_values.view(row.xyz[a]));
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
    record(prefix + "assembly_transport", tick);
    const auto relative = std::filesystem::path("query-" + std::to_string(q));
    const auto directory = output.path() / relative;
    if (!std::filesystem::create_directory(directory))
      throw Error(ErrorCode::io_failure, "cannot create model query directory");
    std::pmr::vector<ComponentResult> components(resource.get());
    components.reserve(definitions.size());
    double mesh_seconds = 0.;
    for (uint32_t d = 0; d < definitions.size(); ++d) {
      const auto def = definitions[d];
      const auto id = original.definitions[def.value].id.value;
      const auto component_prefix = prefix + "definition" + std::to_string(id) + ".";
      const auto &allowances = assembly.view();
      const auto found = std::find_if(allowances.definitions.begin(), allowances.definitions.end(),
                                      [&](const auto &row) { return row.definition == def; });
      if (found == allowances.definitions.end())
        throw Error(ErrorCode::internal_error, "model definition allowance missing");
      stage = "query.complete_component";
      tick = Clock::now();
      const auto result = mesher.mesh({generation, def, curve_banks[d], allowances.values,
                                       found->positional, found->node_source},
                                      scheduler, context);
      const auto component_seconds = elapsed(tick);
      mesh_seconds += component_seconds;
      record(component_prefix + "complete_component", tick);
      const auto &stats = result->stats;
      const auto totals = caller::surface_totals(result->owners);
      timing << component_prefix << "owner_setup\t" << stats.owner_setup_seconds << '\n'
             << component_prefix << "boundary_setup\t" << stats.boundary_setup_seconds << '\n'
             << component_prefix << "boundary\t" << stats.boundary_seconds << '\n'
             << component_prefix << "faces\t" << stats.face_seconds << '\n'
             << component_prefix << "contacts\t" << stats.contact_seconds << '\n'
             << component_prefix << "boundary_close\t" << stats.boundary_close_seconds << '\n';
      queries << q << '\t' << id << '\t' << o.positional[q] << '\t' << o.node_source << '\t';
      caller::write_exact(queries, allowances.values.view(found->positional));
      queries << '\t';
      caller::write_exact(queries, allowances.values.view(found->node_source));
      for (const auto count :
           {uint64_t(result->faces.size()), stats.facets, stats.assignments, stats.boundary_rounds,
            stats.retained_faces, stats.contacts.candidate_pairs,
            stats.contacts.retained_forbidden_pairs, stats.curves.device_points,
            stats.curves.device_bounds, stats.curves.uploaded_bytes, stats.curves.downloaded_bytes,
            totals.device_points, totals.device_bounds, totals.uploaded_bytes,
            totals.downloaded_bytes, totals.cap_facet_tests, totals.device_cap_facets,
            totals.domain_replacements, resource->peak()})
        queries << '\t' << count;
      queries << '\n';
      stage = "output.component";
      tick = Clock::now();
      const auto component_relative = relative / ("definition-" + std::to_string(id));
      caller::write_component(output.path() / component_relative, result, faces, q,
                              component_relative, budget);
      record(component_prefix + "output", tick);
      components.push_back(result);
      std::cout << "component\tcomplete\tquery\t" << q << "\tdefinition\t" << id << "\tfaces\t"
                << result->faces.size() << "\tfacets\t" << stats.facets << "\tseconds\t"
                << component_seconds << '\n'
                << std::flush;
    }
    timing << prefix << "complete_meshing\t" << mesh_seconds << '\n';
    // Even --mesh-only prepares the source-qualified scene inventory/BOM; it
    // omits only display consumers and images, never semantic admission.
    stage = "query.scene";
    tick = Clock::now();
    const auto scene = make_adaptive_scene(
        budget, resource,
        {assembly,
         components,
         o.definitions.empty() ? SceneScope::complete_assembly : SceneScope::selected_components,
         {}});
    record(prefix + "scene", tick);
    stage = "output.labels";
    tick = Clock::now();
    labels(directory / "labels.tsv", scene, budget);
    record(prefix + "labels", tick);
    if (!o.mesh_only) {
      stage = "query.presentation";
      tick = Clock::now();
      const auto presentation =
          make_presentation(budget, scene->view(), o.width, o.height, o.explosion);
      NativeDisplay display(resource, display_module, scene, presentation);
      record(prefix + "presentation", tick);
      stage = "output.scene_views";
      tick = Clock::now();
      write_scene(directory / "scene.bgscene", display.view(), budget);
      write_views(directory / "views.bgview", presentation.views, budget);
      record(prefix + "scene_views", tick);
      stage = "display.create";
      tick = Clock::now();
      bgom10::Times startup{};
      display.open(o.threads, o.device, startup);
      timing << prefix << "display.create\t" << startup.create / 1000. << '\n'
             << prefix << "display.upload\t" << startup.upload / 1000. << '\n';
      stage = "display.evaluate";
      display.evaluate(startup);
      timing << prefix << "display.evaluate\t" << startup.evaluate / 1000. << '\n';
      const auto display_startup = elapsed(tick);
      for (std::size_t frame = 0; frame < 2; ++frame) {
        stage = frame ? "display.exploded" : "display.assembled";
        tick = Clock::now();
        bgom10::Times times{};
        display.render(presentation.views[frame], times);
        const auto render_seconds = elapsed(tick);
        const auto name = prefix + (frame ? "exploded." : "assembled.");
        timing << name << "project\t" << times.project / 1000. << '\n'
               << name << "setup\t" << times.setup / 1000. << '\n'
               << name << "bins\t" << times.bins / 1000. << '\n'
               << name << "shading\t" << times.display / 1000. << '\n'
               << name << "download\t" << times.download / 1000. << '\n'
               << name << "render_total\t" << render_seconds << '\n';
        stage = "output.annotation";
        tick = Clock::now();
        std::ostringstream caption;
        caption << std::fixed << std::setprecision(3) << "SOURCE " << source_seconds << " S MESH "
                << mesh_seconds << " S DISPLAY " << display_startup + render_seconds << " S EPS "
                << std::scientific << std::setprecision(2) << o.positional[q] << " DELTA "
                << o.node_source;
        const auto heading =
            std::string(o.p100 ? "BREPGPU P100 " : "BREPGPU CPU ") +
            (o.p100 ? std::to_string(o.device) : std::to_string(o.threads) + " THREADS") +
            (frame ? " EXPLODED" : " ASSEMBLED");
        annotate_frame(budget, display.pixels(), presentation.views[frame], presentation,
                       frame != 0, {heading, caption.str()});
        record(name + "annotation", tick);
        stage = "output.png";
        tick = Clock::now();
        write_png(directory / (frame ? "exploded.png" : "assembled.png"), o.width, o.height,
                  display.pixels(), budget);
        record(name + "png", tick);
      }
      stage = "display.close";
      tick = Clock::now();
      bgom10::Times closing{};
      display.close(closing);
      record(prefix + "display.close", tick);
    }
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
  std::cout << std::setprecision(17) << "adaptive_model\tcomplete\tdefinitions\t"
            << definitions.size() << "\tqueries\t" << o.positional.size() << "\tseconds\t"
            << elapsed(full) << '\n';
}
} // namespace
int main(int argc, char **argv) {
  std::string_view stage = "options";
  try {
    const auto o = options(argc, argv);
    if (o.help) {
      std::cout
          << "brepgpu-adaptive-model --source FILE --output NEW_DIRECTORY --positional-mm VALUE "
             "[--positional-mm VALUE ...] --node-source-mm VALUE --threads N [--definition ID ...] "
             "[--face-owners N --memory-mib N --max-nodes N --max-depth N --construction-work N] "
             "[--backend p100 --cuda-module LIBRARY --display-module LIBRARY --device N] "
             "[--width N --height N --explosion VALUE --mesh-only]\n";
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
