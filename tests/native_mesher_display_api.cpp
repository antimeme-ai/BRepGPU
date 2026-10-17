#include "../native/om10/types.hpp"
#include <algorithm>
#include <dlfcn.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void appearance(const bgom10::Api *old_api, const bgom10::Api *api, int32_t device) {
  using namespace bgom10;
  Carrier carrier{};
  carrier.id = 1;
  carrier.tag = 1;
  carrier.x[0] = carrier.y[1] = carrier.z[2] = 1;
  carrier.uv[0] = carrier.uv[2] = -1;
  carrier.uv[1] = carrier.uv[3] = 1;
  Definition definition{1, 0, 3, 0, 1};
  Instance instances[2]{};
  for (uint32_t i = 0; i < 2; ++i) {
    instances[i].id = i + 1;
    instances[i].reserved = i + 1;
    instances[i].matrix[0] = instances[i].matrix[5] = instances[i].matrix[10] = 1;
    instances[i].matrix[3] = i == 0 ? -1.5 : 1.5;
  }
  Vertex vertices[3]{};
  const double xy[3][2] = {{-1., -1.}, {1., -1.}, {-1., 1.}};
  for (uint32_t i = 0; i < 3; ++i) {
    vertices[i].xyz[0] = vertices[i].uv[0] = xy[i][0];
    vertices[i].xyz[1] = vertices[i].uv[1] = xy[i][1];
    vertices[i].flags = STORED_XYZ | COFACTOR_NORMAL;
    vertices[i].physical = i + 1;
  }
  Triangle triangle{{0, 1, 2}, UINT32_MAX, 1};
  Material materials[3]{{{.8, 0., 0.}, .5}, {{0., 0., .8}, .5}, {{0., .8, 0.}, .5}};
  VertexRef refs[6]{{0, 0}, {1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 1}};
  PrimitiveRef primitives[2]{{0, 0, 0, 0}, {0, 1, 3, 0}};
  SceneView scene{&definition, 1, instances, 2, vertices, 3, &triangle,  1, &carrier, 1,
                  materials,   3, nullptr,   0, refs,     6, primitives, 2};
  Error error{};
  Times times{};
  void *token = nullptr;
  require(!old_api->create(&scene, 1, device, &token, &error, &times) && !token,
          "legacy profile accepted occurrence appearance fields");
  Request request{};
  request.width = 64;
  request.height = 32;
  request.supersample = 1;
  request.basis[0] = request.basis[4] = request.basis[8] = 1;
  request.pixel_center[0] = 32;
  request.pixel_center[1] = 16;
  request.scale = 8;
  request.background[0] = request.background[1] = request.background[2] = .01;
  const size_t count = request.width * request.height;
  std::vector<uint8_t> rgba(count * 4);
  std::vector<double> depth(count);
  std::vector<uint64_t> primitive(count), instance(count);
  FrameOutput frame{rgba.data(), depth.data(), primitive.data(), instance.data(), nullptr, nullptr};
  for (bool explicit_face : {false, true}) {
    triangle.material = explicit_face ? 2 : UINT32_MAX;
    require(api->create(&scene, 2, device, &token, &error, &times) && token,
            "source appearance scene admission");
    require(api->evaluate(token, nullptr, &error, &times), "source appearance evaluate");
    require(api->render(token, &request, &frame, &error, &times), "source appearance render");
    bool red = false, blue = false, green = false;
    for (size_t i = 0; i < count; ++i) {
      red = red || (rgba[i * 4] > rgba[i * 4 + 1] && rgba[i * 4] > rgba[i * 4 + 2]);
      blue = blue || (rgba[i * 4 + 2] > rgba[i * 4] && rgba[i * 4 + 2] > rgba[i * 4 + 1]);
      green = green || (rgba[i * 4 + 1] > rgba[i * 4] && rgba[i * 4 + 1] > rgba[i * 4 + 2]);
    }
    require(explicit_face ? (green && !red && !blue) : (red && blue && !green),
            "actual source occurrence/explicit-face appearance precedence");
    require(api->close(token, &error, &times), "source appearance close");
    token = nullptr;
  }
  triangle.material = UINT32_MAX;
  const auto reject_scene = [&] {
    const bool admitted = api->create(&scene, 1, device, &token, &error, &times);
    if (token) {
      require(api->close(token, &error, &times), "invalid-scene probe close");
      token = nullptr;
    }
    require(!admitted, "direct factory accepted record rejected by finite file contract");
  };
  vertices[0].xyz[0] = std::numeric_limits<double>::quiet_NaN();
  reject_scene();
  vertices[0].xyz[0] = -1.;
  vertices[0].uv[0] = 2.;
  reject_scene();
  vertices[0].uv[0] = -1.;
  triangle.vertex[2] = 3;
  reject_scene();
  triangle.vertex[2] = 2;
  refs[0].source = 3;
  reject_scene();
  refs[0].source = 0;
  primitives[0].vertex_base = 1;
  reject_scene();
  primitives[0].vertex_base = 0;
  instances[1].id = 1;
  reject_scene();
  instances[1].id = 2;
  carrier.y[1] = 0;
  reject_scene();
  carrier.y[1] = 1;
  materials[0].roughness = 0;
  reject_scene();
  materials[0].roughness = .5;
  require(api->create(&scene, 1, device, &token, &error, &times) && token,
          "request validation probe admission");
  require(api->evaluate(token, nullptr, &error, &times), "request validation probe evaluation");
  const auto reject_request = [&](const Request &bad) {
    std::fill(rgba.begin(), rgba.end(), uint8_t{73});
    require(!api->render(token, &bad, &frame, &error, &times),
            "direct render accepted invalid finite Request");
    require(std::all_of(rgba.begin(), rgba.end(), [](uint8_t x) { return x == 73; }),
            "invalid Request mutated frame output");
  };
  auto bad = request;
  bad.width = 0;
  reject_request(bad);
  bad = request;
  bad.scale = 0;
  reject_request(bad);
  bad = request;
  bad.flags = 8;
  reject_request(bad);
  bad = request;
  bad.basis[0] = 0;
  reject_request(bad);
  bad = request;
  bad.background[0] = 2;
  reject_request(bad);
  bad = request;
  bad.center[0] = std::numeric_limits<double>::quiet_NaN();
  reject_request(bad);
  require(api->render(token, &request, &frame, &error, &times),
          "invalid Request poisoned otherwise healthy session");
  require(api->close(token, &error, &times), "request validation probe close");
  token = nullptr;
  for (uint32_t bad : {0U, 4U}) {
    instances[0].reserved = bad;
    require(!api->create(&scene, 1, device, &token, &error, &times) && !token,
            "direct source API accepted absent/out-of-range inherited appearance");
  }
  instances[0].reserved = 1;
  vertices[0].flags = vertices[1].flags = vertices[2].flags = 0;
  triangle.material = 0;
  require(!old_api->create(&scene, 1, device, &token, &error, &times) && !token,
          "legacy direct API accepted source occurrence override with old vertexflags");
}
void check(const bgom10::Api *old_api, const bgom10::Api *api, int32_t device) {
  using namespace bgom10;
  require(old_api && api && old_api->version == 1 && api->version == 2,
          "explicit display ABI versions");
  Carrier carrier{};
  carrier.id = 1;
  carrier.tag = 1;
  carrier.x[0] = carrier.y[1] = carrier.z[2] = 1;
  Vertex vertex{};
  vertex.xyz[0] = 2;
  vertex.xyz[1] = 3;
  vertex.xyz[2] = 4;
  vertex.flags = STORED_XYZ | COFACTOR_NORMAL;
  Definition definition{1, 0, 1, 0, 0};
  SceneView scene{};
  scene.definitions = &definition;
  scene.ndefinitions = 1;
  scene.carriers = &carrier;
  scene.ncarriers = 1;
  scene.vertices = &vertex;
  scene.nvertices = 1;
  Error error{};
  Times times{};
  void *token = nullptr;
  require(!old_api->create(&scene, 1, device, &token, &error, &times) && token == nullptr,
          "legacy factory silently consumed new normal flags");
  require(api->create(&scene, 1, device, &token, &error, &times) && token,
          "new profile session create");
  Jet jet{};
  require(api->evaluate(token, &jet, &error, &times), "new profile source evaluate");
  require(jet.used[0] == 2 && jet.used[1] == 3 && jet.used[2] == 4 && jet.normal[2] == 1 &&
              jet.flags == COFACTOR_INSTANCE_NORMAL,
          "stored point/normal semantics lost at ABI");
  require(api->close(token, &error, &times), "new profile session close");
  token = nullptr;
  vertex.flags = 0;
  require(!api->create(&scene, 1, device, &token, &error, &times) && token == nullptr,
          "new profile accepted vertex without stored XYZ/cofactor declaration");
  vertex.flags = STORED_XYZ;
  require(old_api->create(&scene, 1, device, &token, &error, &times) && token,
          "shared validator rejected a valid legacy stored-position scene");
  require(old_api->evaluate(token, &jet, &error, &times) && jet.used[0] == 2 &&
              jet.used[1] == 3 && jet.used[2] == 4 && jet.normal[2] == 1,
          "shared validator changed legacy source evaluation");
  require(old_api->close(token, &error, &times), "legacy validation probe close");
  token = nullptr;
}
} // namespace
int main(int argc, char **argv) {
  void *plugin = nullptr;
  try {
    check(bgom10::cpu_api(), bgom10::cpu_api_v2(), 0);
    appearance(bgom10::cpu_api(), bgom10::cpu_api_v2(), 0);
    if (argc > 1) {
      plugin = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
      require(plugin, "cannot load display plugin");
      const auto old_factory =
          reinterpret_cast<const bgom10::Api *(*)()>(dlsym(plugin, "brepgpu_om10_cuda_v1"));
      const auto factory =
          reinterpret_cast<const bgom10::Api *(*)()>(dlsym(plugin, "brepgpu_om10_cuda_v2"));
      require(old_factory && factory, "both explicit display factories required");
      check(old_factory(), factory(), argc > 2 ? std::stoi(argv[2]) : 0);
      appearance(old_factory(), factory(), argc > 2 ? std::stoi(argv[2]) : 0);
      dlclose(plugin);
      plugin = nullptr;
    }
    std::cout << "PASS\texplicit stored-source display ABI\n";
    return 0;
  } catch (const std::exception &error) {
    if (plugin)
      dlclose(plugin);
    std::cerr << error.what() << '\n';
    return 1;
  }
}
