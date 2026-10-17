#include "../native/mesher/adaptive_assembly.hpp"
#include "../native/mesher/adaptive_caller_io.hpp"
#include "../native/mesher/adaptive_component.hpp"
#include "../native/mesher/adaptive_scene.hpp"
#include "../native/mesher/adaptive_trimmed_io.hpp"
#include "../native/mesher/presentation.hpp"
#include <bit>
#include <dlfcn.h>
#include <fstream>
#include <iostream>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool okay, const char *message) {
  if (!okay)
    throw std::runtime_error(message);
}
template <class T> void write_records(const std::filesystem::path &path, std::span<const T> rows) {
  std::ofstream stream(path, std::ios::binary);
  stream.write(reinterpret_cast<const char *>(rows.data()),
               static_cast<std::streamsize>(rows.size_bytes()));
  stream.close();
  require(stream.good(), "write native display test records");
}
struct Consumer final {
  const bgom10::Api *api = bgom10::cpu_api_v2();
  void *module = nullptr, *token = nullptr;
  bgom10::Error error{};
  bgom10::Times times{};
  ~Consumer() {
    if (token)
      api->close(token, &error, &times);
    if (module)
      dlclose(module);
  }
  void check(bool okay) const {
    if (!okay)
      throw std::runtime_error(std::string(error.stage) + ": " + error.message);
  }
};
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc >= 3 && argc <= 6,
            "usage: adaptive scene original-source output-directory [module device] [--render]");
    const bool render = std::string_view(argv[argc - 1]) == "--render";
    const auto args = argc - static_cast<int>(render);
    require(args == 3 || args == 5, "invalid display consumer arguments");
    const auto resource = std::make_shared<RunResource>(uint64_t{1} << 33);
    JobContext context({uint64_t{1} << 29, uint64_t{1} << 44}, resource.get(), {});
    const auto source = load_source(argv[1], resource);
    const auto assembly = prepare_adaptive_assembly(
        context.budget, source, resource,
        {Rational::from_double(context.budget, .01), Rational::from_double(context.budget, 1.e-8)},
        {{{OwnerKind::run, 901, 1}, 1, 0}, {{OwnerKind::run, 901, 1}, 1, 1}});
    std::vector<CarrierRequest> carriers;
    for (const auto &carrier : assembly->carriers)
      carriers.push_back({carrier.source, carrier.cache_domain});
    const auto cache =
        prepare_trim_source_cache(source, carriers, resource, {}, {}, context.budget);
    ComponentLimits limits;
    limits.face_owners = 1;
    NativeScheduler scheduler(resource, {1, 4, uint64_t{1} << 27, uint64_t{1} << 42});
    ComponentMesher mesher(resource, cache, limits);
    std::vector<ComponentResult> components;
    for (const auto &definition : assembly->assembly.view().definitions) {
      if (std::none_of(assembly->assembly.view().instances.begin(),
                       assembly->assembly.view().instances.end(), [&](const auto &instance) {
                         return instance.definition == definition.definition;
                       }))
        continue;
      const auto requests =
          caller::definition_curve_requests(source, definition.definition, resource);
      const auto curves =
          prepare_curve_source_cache(source, requests, resource, {}, {}, context.budget);
      components.push_back(
          mesher.mesh({1, definition.definition, curves, assembly->assembly.view().values,
                       definition.positional, definition.node_source},
                      scheduler, context));
    }
    mesher.close();
    const auto scene =
        make_adaptive_scene(context.budget, resource,
                            {assembly->assembly, components, SceneScope::complete_assembly, {}});
    require(scene && !scene->vertices.empty() && !scene->triangles.empty(),
            "complete native scene required");
    require(scene->vertices.size() == scene->vertex_sources.size(), "source provenance inventory");
    for (std::size_t i = 0; i < scene->vertices.size(); ++i) {
      const auto &provenance = scene->vertex_sources[i];
      const auto &mesh =
          *scene->components.at(provenance.component)->faces.at(provenance.face)->construction;
      const auto &actual = mesh.sites.at(provenance.site);
      require(provenance.original_uv == actual.uv, "exact original UV provenance changed");
      for (uint32_t axis = 0; axis < 3; ++axis)
        require(std::bit_cast<uint64_t>(scene->vertices[i].xyz[axis]) ==
                    std::bit_cast<uint64_t>(actual.actual_xyz[axis]),
                "stored actual XYZ words changed in display adapter");
      for (uint32_t axis = 0; axis < 2; ++axis)
        require(
            std::bit_cast<uint64_t>(scene->vertices[i].uv[axis]) ==
                std::bit_cast<uint64_t>(
                    import_exact(context.budget, mesh.values.view(actual.uv[axis])).to_double()),
            "original exact UV normal locator changed");
    }
    const std::filesystem::path output(argv[2]);
    std::filesystem::create_directories(output);
    const auto presentation = make_presentation(context.budget, scene->view(), 320, 320, 1.);
    auto view = scene->view();
    std::vector<bgom10::Instance> placed(scene->instances.begin(), scene->instances.end());
    for (std::size_t i = 0; i < placed.size(); ++i)
      std::copy(presentation.explosion[i].begin(), presentation.explosion[i].end(),
                placed[i].explosion);
    view.instances = placed.data();
    write_scene(output / "actual.bgscene", view, context.budget);
    write_views(output / "actual.bgview", presentation.views, context.budget);
    Consumer consumer;
    int32_t device = 0;
    if (args == 5) {
      consumer.module = dlopen(argv[3], RTLD_NOW | RTLD_LOCAL);
      require(consumer.module, "open actual display CUDA module");
      const auto factory = reinterpret_cast<const bgom10::Api *(*)()>(
          dlsym(consumer.module, "brepgpu_om10_cuda_v2"));
      require(factory, "source-profile CUDA display API missing");
      consumer.api = factory();
      device = std::stoi(argv[4]);
    }
    require(consumer.api && consumer.api->version == 2 &&
                consumer.api->size >= sizeof(bgom10::Api) && consumer.api->create &&
                consumer.api->evaluate && consumer.api->render && consumer.api->close,
            "source-profile display API required");
    consumer.check(
        consumer.api->create(&view, 2, device, &consumer.token, &consumer.error, &consumer.times));
    std::vector<bgom10::Jet> jets(scene->vertices.size());
    consumer.check(
        consumer.api->evaluate(consumer.token, jets.data(), &consumer.error, &consumer.times));
    write_records(output / "actual.jets", std::span<const bgom10::Jet>(jets));
    for (uint32_t frame = 0; frame < 2; ++frame) {
      auto request = presentation.views[frame];
      if (!render)
        request.flags = bgom10::PROJECT_ONLY;
      std::vector<uint8_t> rgba(uint64_t{request.width} * request.height * 4);
      std::vector<double> depth(uint64_t{request.width} * request.height);
      std::vector<uint64_t> primitives(depth.size()), instances(depth.size());
      std::vector<bgom10::Projected> projected(view.nplaced_vertices);
      bgom10::FrameOutput result{rgba.data(),      depth.data(),     primitives.data(),
                                 instances.data(), projected.data(), nullptr};
      consumer.check(consumer.api->render(consumer.token, &request, &result, &consumer.error,
                                          &consumer.times));
      write_records(output / ("frame-" + std::to_string(frame) + ".projected"),
                    std::span<const bgom10::Projected>(projected));
      if (render)
        write_png(output / ("frame-" + std::to_string(frame) + ".png"), request.width,
                  request.height, rgba, context.budget);
    }
    consumer.check(consumer.api->close(consumer.token, &consumer.error, &consumer.times));
    consumer.token = nullptr;
    for (const auto &component : components) {
      for (const auto &face : component->faces) {
        const auto id = source.view().faces[face->construction->key.face.value].id.value;
        const auto stem = std::to_string(id);
        caller::write_curves(output / (stem + ".bgacrv"), *face->boundary, context.budget);
        caller::write_trimmed_face(output / (stem + ".bgatrm"), face, context.budget);
      }
    }
    std::cout << "PASS\tdefinitions\t" << scene->definitions.size() << "\tinstances\t"
              << scene->instances.size() << "\ttriangles\t" << scene->triangles.size() << '\n';
    return 0;
  } catch (const bgfd::Failure &error) {
    std::cerr << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
  }
  return 1;
}
