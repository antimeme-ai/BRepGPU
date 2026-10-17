#include "../native/mesher/cuda/adaptive_curve_api.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
using namespace bgmesh::adaptive;
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
void checked(int32_t status, const DeviceError &error) {
  if (status)
    throw std::runtime_error(error.message);
}
struct Fixture final {
  const CurveCudaAPI *api;
  const AdaptiveCudaAPI *surface;
  void *module;
  DeviceSession *session = nullptr;
  uint64_t leases = 0, bytes = 0;
  DeviceConfig config{};
  DeviceError error{};
  static void *reserve(void *context, uint64_t amount) noexcept {
    auto &self = *static_cast<Fixture *>(context);
    try {
      auto *token = new uint64_t(amount);
      ++self.leases;
      self.bytes += amount;
      return token;
    } catch (...) {
      return nullptr;
    }
  }
  static void release(void *context, void *opaque) noexcept {
    auto &self = *static_cast<Fixture *>(context);
    auto *token = static_cast<uint64_t *>(opaque);
    self.bytes -= *token;
    --self.leases;
    delete token;
  }
  explicit Fixture(const char *path, int32_t device) {
    module = ::dlopen(path, RTLD_NOW | RTLD_LOCAL);
    require(module != nullptr, "curve module load failed");
    const auto factory = reinterpret_cast<const CurveCudaAPI *(*)()>(
        ::dlsym(module, "brepgpu_adaptive_curve_cuda_v1"));
    require(factory != nullptr, "curve operation export missing");
    api = factory();
    require(api && api->abi == curve_cuda_abi && api->size == sizeof(CurveCudaAPI),
            "curve operation ABI mismatch");
    const auto old_factory =
        reinterpret_cast<const AdaptiveCudaAPI *(*)()>(::dlsym(module, "brepgpu_adaptive_cuda_v1"));
    require(old_factory != nullptr, "surface operation export missing");
    surface = old_factory();
    config.abi = curve_cuda_abi;
    config.size = sizeof(config);
    config.device = device;
    config.points = 32;
    config.controls = 8192;
    config.nodes = config.frontier = 1024;
    config.host_bytes = config.device_bytes = uint64_t{1} << 27;
    config.work = uint64_t{1} << 32;
    config.reservation_context = this;
    config.reserve = reserve;
    config.release = release;
  }
  Fixture(const Fixture &) = delete;
  Fixture &operator=(const Fixture &) = delete;
  ~Fixture() {
    if (session && api->destroy(session, &error))
      std::terminate();
    if (module)
      ::dlclose(module);
  }
  void poll(uint64_t ticket, void *out, uint32_t count) {
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    for (;;) {
      uint32_t ready = 0;
      checked(api->poll(session, ticket, &ready, out, count, &error), error);
      if (ready)
        return;
      require(std::chrono::steady_clock::now() < until, "curve operation deadline");
      std::this_thread::yield();
    }
  }
  void close() {
    checked(api->destroy(session, &error), error);
    session = nullptr;
    require(!leases && !bytes, "curve drain leaked callback reservations");
  }
};
void exercise(const char *path, int32_t device) {
  Fixture f(path, device);
  std::array<CurveRootDescriptor, 2> roots{};
  for (uint32_t i = 0; i < roots.size(); ++i) {
    roots[i].original = {i};
    roots[i].source_slot = i;
    roots[i].source_id = 10 + i;
    roots[i].original_t[1] = roots[i].prepared_t[1] = {1., 1.};
    roots[i].line_profile = 1;
  }
  roots[0].frame[3] = 1.;
  roots[1].family = CurveFamily::rational;
  roots[1].degree = 32;
  roots[1].controls = {0, 132};
  std::array<Interval, 132> controls{};
  for (uint32_t i = 0; i < 33; ++i) {
    controls[i * 4] = {double(i), double(i)};
    controls[i * 4 + 3] = {1., 1.};
  }
  auto create = [&](const DeviceConfig &config, const auto &descriptors) {
    return f.api->create(&config, descriptors.data(), 2, descriptors.data(), 2, controls.data(),
                         controls.size(), &f.session, &f.error);
  };
  auto tiny = f.config;
  tiny.host_bytes = 1;
  require(create(tiny, roots) != 0, "tiny curve owner accepted");
  if (f.session)
    f.close();
  require(!f.leases, "rejected curve create retained leases");
  auto malformed = roots;
  malformed[1].dimension = 4;
  require(create(f.config, malformed) != 0 && !f.session, "bad curve shape admitted");
  checked(create(f.config, roots), f.error);

  constexpr uint32_t scan_count = 513;
  std::vector<CurveDeviceNode> nodes(scan_count);
  for (uint32_t i = 0; i < nodes.size(); ++i) {
    auto &task = nodes[i];
    task.slot = i;
    task.input.root = {0};
    task.input.t[1] = {1., 1.};
    task.cached = 1;
    task.tolerance_down = 1.;
    task.cached_bounds.status = NumericStatus::ready;
    task.cached_bounds.parameter_upper = i % 3 ? .25 : 2.;
  }
  uint64_t ticket = 0;
  checked(f.api->nodes(f.session, nodes.data(), scan_count, &ticket, &f.error), f.error);
  std::vector<CurveDeviceNodeAnswer> answers(nodes.size());
  uint32_t ready = 0;
  require(f.api->poll(f.session, ticket, &ready, answers.data(), 1, &f.error) != 0,
          "curve poll accepted too-small record capacity");
  f.poll(ticket, answers.data(), scan_count);
  uint32_t prefix = 0;
  for (uint32_t i = 0; i < nodes.size(); ++i) {
    require(answers[i].split_prefix == prefix, "wrong curve exclusive split prefix");
    const bool split = i % 3 == 0;
    require(answers[i].action == (split ? 1U : 0U), "wrong curve frontier mark");
    prefix += split;
  }
  require(f.api->poll(f.session, ticket, &ready, answers.data(), scan_count, &f.error) != 0,
          "consumed curve ticket accepted");
  nodes[1].slot = 0;
  require(f.api->nodes(f.session, nodes.data(), 2, &ticket, &f.error) != 0,
          "duplicate curve destinations accepted");
  nodes[1].slot = 1;
  nodes[0].tolerance_down = -1.;
  require(f.api->nodes(f.session, nodes.data(), 1, &ticket, &f.error) != 0,
          "negative curve tolerance accepted");

  std::array<CurveDeviceSample, 3> samples{};
  for (uint32_t i = 0; i < 3; ++i) {
    samples[i].destination = i;
    samples[i].input.original = {1};
    samples[i].input.t = {double(i) * .5, double(i) * .5};
  }
  samples[0].input.fixed_actual = samples[2].input.fixed_actual = 1;
  samples[0].input.actual_xyz[0] = -.125;
  samples[2].input.actual_xyz[0] = 32.25;
  checked(f.api->samples(f.session, samples.data(), samples.size(), &ticket, &f.error), f.error);
  samples[0].input.t = {1., 1.}; // Borrowed input may change after return.
  std::array<CurveSampleResult, 3> values{};
  f.poll(ticket, values.data(), values.size());
  for (uint32_t i = 0; i < values.size(); ++i) {
    const auto expected = double(i) * 16.;
    require(values[i].status == NumericStatus::ready &&
                values[i].original_xyz[0].lower <= expected &&
                values[i].original_xyz[0].upper >= expected,
            "degree32 original sample enclosure failed");
    require(std::abs(values[i].actual_xyz[0] - expected) <= values[i].departure_upper,
            "fixed actual point borrowed a different Q departure");
  }
  require(values[0].actual_xyz[0] == -.125 && values[2].actual_xyz[0] == 32.25,
          "fixed source endpoint was replaced");
  CurveDeviceRestriction pair{{1}, {0, 132}, 132, 264};
  auto bad_pair = pair;
  bad_pair.left_destination = 0;
  require(f.api->paired_children(f.session, &bad_pair, 1, &ticket, &f.error) != 0,
          "paired child overwrites parent");
  bad_pair = pair;
  bad_pair.right_destination = bad_pair.left_destination;
  require(f.api->paired_children(f.session, &bad_pair, 1, &ticket, &f.error) != 0,
          "paired children overlap");
  checked(f.api->paired_children(f.session, &pair, 1, &ticket, &f.error), f.error);
  f.poll(ticket, nullptr, 0);
  std::array<CurveDeviceNode, 2> children{};
  for (uint32_t i = 0; i < 2; ++i) {
    auto &task = children[i];
    task.slot = i;
    task.input.root = {1};
    task.input.controls = {132 * (i + 1), 132};
    task.input.t[0] = {double(i) * .5, double(i) * .5};
    task.input.t[1] = {double(i + 1) * .5, double(i + 1) * .5};
    task.points[0] = i;
    task.points[1] = i + 1;
    task.tolerance_down = 1.;
  }
  checked(f.api->nodes(f.session, children.data(), 2, &ticket, &f.error), f.error);
  std::array<CurveDeviceNodeAnswer, 2> child_bounds{};
  f.poll(ticket, child_bounds.data(), child_bounds.size());
  require(child_bounds[0].bounds.status == NumericStatus::ready &&
              child_bounds[1].bounds.status == NumericStatus::ready &&
              child_bounds[0].bounds.parameter_upper >= .125 &&
              child_bounds[1].bounds.parameter_upper >= .25 &&
              child_bounds[0].bounds.parameter_upper < 1. &&
              child_bounds[1].bounds.parameter_upper < 1.,
          "paired nets/degree33 endpoint residual are wrong");
  checked(
      f.api->install_controls(f.session, 396, controls.data(), controls.size(), &ticket, &f.error),
      f.error);
  f.poll(ticket, nullptr, 0);
  require(f.api->install_controls(f.session, 396, controls.data(), controls.size(), &ticket,
                                  &f.error) != 0,
          "initial control install overwrites retained net");

  std::array<AffineTraceBoundInput, 2> traces{};
  traces[0].kind = AffineTraceBoundKind::rational;
  traces[0].rational_curve = roots[1];
  traces[0].input.controls = {0, 132};
  traces[0].input.t[1] = {1., 1.};
  traces[0].affine_endpoints[3] = {32.125, 32.25};
  traces[1].kind = AffineTraceBoundKind::direct_derivative;
  traces[1].input.t[1] = {1., 1.};
  traces[1].second_derivative_upper = 8.;
  checked(f.api->trace_bounds(f.session, traces.data(), traces.size(), controls.data(),
                              controls.size(), &ticket, &f.error),
          f.error);
  std::array<CurveNodeBounds, 2> trace_bounds{};
  f.poll(ticket, trace_bounds.data(), trace_bounds.size());
  require(trace_bounds[0].status == NumericStatus::ready &&
              trace_bounds[0].parameter_upper >= .25 && trace_bounds[0].parameter_upper < 1. &&
              trace_bounds[1].status == NumericStatus::ready &&
              trace_bounds[1].parameter_upper >= 1. && trace_bounds[1].parameter_upper < 1.01,
          "SAME virtual L or direct derivative trace bound is wrong");
  DeviceNode surface_task{};
  require(f.surface->nodes(f.session, &surface_task, 1, &ticket, &f.error) != 0,
          "surface operation consumed curve descriptor memory");
  DeviceStats stats{};
  checked(f.api->stats(f.session, &stats, &f.error), f.error);
  require(stats.uploaded_bytes && stats.downloaded_bytes &&
              stats.host_reserved_bytes + stats.device_reserved_bytes == f.bytes,
          "curve reservations/transfers omit actual owned storage");
  // Destruction must drain an outstanding stream operation and return all
  // leases.
  checked(f.api->nodes(f.session, children.data(), 2, &ticket, &f.error), f.error);
  f.close();
  RootDescriptor plane{};
  plane.original = {0};
  plane.original_box[1] = plane.original_box[3] = {1., 1.};
  std::copy(std::begin(plane.original_box), std::end(plane.original_box), plane.prepared_box);
  checked(f.surface->create(&f.config, &plane, 1, &plane, 1, nullptr, 0, &f.session, &f.error),
          f.error);
  require(f.api->samples(f.session, samples.data(), 1, &ticket, &f.error) != 0,
          "curve operation consumed surface descriptor memory");
  f.close();
  std::cout << "curve_device\tprefixes\t513\tdegree\t32\tleases\t0\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw std::runtime_error("expected module and device");
    const std::string device(argv[2]);
    require(device == "0" || device == "1", "expected P100 index 0 or 1");
    exercise(argv[1], device == "0" ? 0 : 1);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
