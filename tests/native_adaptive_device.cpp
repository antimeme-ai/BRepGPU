#include "../native/mesher/adaptive_geometry.hpp"
#include "../native/mesher/cuda/adaptive_api.h"
#include "../native/mesher/cuda/adaptive_assignment_api.h"
#include "../native/mesher/reservations.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
using namespace bgmesh;
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
  struct Lease final {
    std::shared_ptr<ResourceReservation> reservation;
  };
  std::shared_ptr<RunResource> resource;
  const SourceCache &cache;
  void *module = nullptr;
  const AdaptiveCudaAPI *api = nullptr;
  DeviceSession *session = nullptr;
  uint64_t leases = 0;
  DeviceConfig config{};
  static void *reserve(void *context, uint64_t bytes) noexcept {
    auto &self = *static_cast<Fixture *>(context);
    try {
      auto reservation = reserve_resource(self.resource, bytes);
      RunAllocator<Lease> allocator(self.resource);
      auto *lease = allocator.allocate(1);
      std::construct_at(lease, Lease{std::move(reservation)});
      ++self.leases;
      return lease;
    } catch (...) {
      return nullptr;
    }
  }
  // Callback ABI fixes context before its opaque reservation token.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  static void release(void *context, void *token) noexcept {
    auto &self = *static_cast<Fixture *>(context);
    auto *lease = static_cast<Lease *>(token);
    std::destroy_at(lease);
    RunAllocator<Lease>(self.resource).deallocate(lease, 1);
    --self.leases;
  }
  Fixture(const SourceCache &source, const char *path, int32_t device)
      : resource(source.view().resource), cache(source) {
    module = ::dlopen(path, RTLD_NOW | RTLD_LOCAL);
    require(module != nullptr, "device module load failed");
    auto factory =
        reinterpret_cast<const AdaptiveCudaAPI *(*)()>(::dlsym(module, "brepgpu_adaptive_cuda_v1"));
    require(factory != nullptr, "device factory missing");
    api = factory();
    require(api && api->abi == adaptive_cuda_abi && api->size == sizeof(AdaptiveCudaAPI),
            "device ABI mismatch");
    config.abi = adaptive_cuda_abi;
    config.size = sizeof(config);
    config.device = device;
    config.points = 16;
    config.controls = 4096;
    config.nodes = 1024;
    config.frontier = 1024;
    config.host_bytes = config.device_bytes = uint64_t{1} << 26;
    config.work = uint64_t{1} << 30;
    config.reservation_context = this;
    config.reserve = reserve;
    config.release = release;
  }
  Fixture(const Fixture &) = delete;
  Fixture &operator=(const Fixture &) = delete;
  ~Fixture() {
    if (session) {
      DeviceError error{};
      if (api->destroy(session, &error)) {
        std::cerr << "unexpected device drain failure: " << error.message << '\n';
        std::terminate();
      }
    }
    if (module)
      ::dlclose(module);
  }
  int32_t create(const DeviceConfig &configuration, std::span<const RootDescriptor> prepared,
                 std::span<const RootDescriptor> original, DeviceSession **output) {
    DeviceError error{};
    return api->create(&configuration, prepared.data(), static_cast<uint32_t>(prepared.size()),
                       original.data(), static_cast<uint32_t>(original.size()),
                       cache.controls().data(), static_cast<uint32_t>(cache.controls().size()),
                       output, &error);
  }
  void poll(uint64_t ticket, void *output, uint32_t count) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    for (;;) {
      uint32_t ready = 0;
      DeviceError error{};
      checked(api->poll(session, ticket, &ready, output, count, &error), error);
      if (ready)
        return;
      require(std::chrono::steady_clock::now() < deadline, "device protocol deadline");
      std::this_thread::yield();
    }
  }
  void rejected_create(const DeviceConfig &configuration, std::span<const RootDescriptor> prepared,
                       std::span<const RootDescriptor> original) {
    DeviceSession *owner = nullptr;
    require(create(configuration, prepared, original, &owner) != 0, "malformed create accepted");
    if (owner) {
      DeviceNode task{};
      uint64_t ticket = 0;
      DeviceError error{};
      require(api->nodes(owner, &task, 1, &ticket, &error) != 0,
              "failed-create owner permits work");
      checked(api->destroy(owner, &error), error);
    }
    require(leases == 0, "failed create leaked callback reservations");
  }
  void close() {
    DeviceError error{};
    checked(api->destroy(session, &error), error);
    session = nullptr;
    require(leases == 0, "healthy device drain leaked callback reservations");
    require(::dlclose(module) == 0, "module close failed");
    module = nullptr;
  }
};
void exercise(const SourceCache &cache, const char *module, int32_t device) {
  Fixture fixture(cache, module, device);
  const auto before = fixture.resource->live();
  auto tiny = fixture.config;
  tiny.host_bytes = 1;
  fixture.rejected_create(tiny, cache.roots(), cache.original_roots());
  auto prepared = std::vector<RootDescriptor>(cache.roots().begin(), cache.roots().end());
  auto original =
      std::vector<RootDescriptor>(cache.original_roots().begin(), cache.original_roots().end());
  const auto rational = std::find_if(prepared.begin(), prepared.end(), [](const auto &root) {
    return root.family == GeometryFamily::rational_tensor;
  });
  const auto plane = std::find_if(prepared.begin(), prepared.end(), [](const auto &root) {
    return root.family == GeometryFamily::plane;
  });
  const auto cylinder = std::find_if(prepared.begin(), prepared.end(), [](const auto &root) {
    return root.family == GeometryFamily::cylinder;
  });
  require(rational != prepared.end() && plane != prepared.end() && cylinder != prepared.end(),
          "expected original fixture families");
  const auto rational_slot = static_cast<uint32_t>(rational - prepared.begin());
  const auto plane_slot = static_cast<uint32_t>(plane - prepared.begin());
  const auto cylinder_slot = static_cast<uint32_t>(cylinder - prepared.begin());
  --prepared[rational_slot].controls.count;
  fixture.rejected_create(fixture.config, prepared, original);
  prepared.assign(cache.roots().begin(), cache.roots().end());
  --original.front().controls.count;
  fixture.rejected_create(fixture.config, prepared, original);
  original.assign(cache.original_roots().begin(), cache.original_roots().end());
  // Deliberately malformed wire enum attacks create admission before any kernel.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  prepared[0].family = static_cast<GeometryFamily>(UINT32_MAX);
  fixture.rejected_create(fixture.config, prepared, original);
  require(fixture.resource->live() == before, "rejected create retained native reservation");
  require(fixture.create(fixture.config, cache.roots(), cache.original_roots(), &fixture.session) ==
              0,
          "valid device create failed");
  const auto get_assignment = reinterpret_cast<const AssignmentCudaAPI *(*)()>(
      ::dlsym(fixture.module, "brepgpu_adaptive_assignment_cuda_v1"));
  require(get_assignment != nullptr, "device assignment retirement extension missing");
  const auto *assignment = get_assignment();
  require(assignment && assignment->abi == adaptive_assignment_cuda_abi &&
              assignment->size == sizeof(AssignmentCudaAPI) && assignment->retire,
          "device assignment ABI mismatch");

  std::vector<DeviceNode> nodes(513);
  for (uint32_t i = 0; i < nodes.size(); ++i) {
    auto &node = nodes[i];
    node.slot = i;
    node.input.root = {cylinder_slot};
    std::copy(std::begin(cache.roots()[cylinder_slot].prepared_box),
              std::end(cache.roots()[cylinder_slot].prepared_box), node.input.box);
    node.cached = 1;
    node.cached_bounds.status = NumericStatus::ready;
    node.cached_bounds.regularity_proved = 1;
    node.cached_bounds.packet_upper = i % 3 ? .5 : 2.;
    node.cached_bounds.score_u = 1.;
    node.cached_bounds.score_v = 2.;
    node.tolerance_down = 1.;
    node.fair_axis = 2;
  }
  uint64_t ticket = 0;
  DeviceError error{};
  auto duplicate_nodes = std::array{nodes[0], nodes[0]};
  require(fixture.api->nodes(fixture.session, duplicate_nodes.data(), 2, &ticket, &error) != 0,
          "duplicate node slots accepted");
  checked(fixture.api->nodes(fixture.session, nodes.data(), static_cast<uint32_t>(nodes.size()),
                             &ticket, &error),
          error);
  std::vector<DeviceNodeAnswer> answers(nodes.size());
  answers[0].action = 777;
  uint32_t ready = 99;
  require(fixture.api->poll(fixture.session, ticket + 1, &ready, answers.data(),
                            static_cast<uint32_t>(answers.size()), &error) != 0,
          "foreign ticket accepted");
  require(fixture.api->poll(fixture.session, ticket, &ready, answers.data(), 512, &error) != 0 &&
              answers[0].action == 777,
          "short poll consumed or wrote result");
  uint64_t rejected_ticket = 0;
  require(fixture.api->nodes(fixture.session, nodes.data(), 1, &rejected_ticket, &error) != 0,
          "second outstanding operation accepted");
  fixture.poll(ticket, answers.data(), static_cast<uint32_t>(answers.size()));
  uint32_t prefix = 0;
  for (uint32_t i = 0; i < answers.size(); ++i) {
    require(answers[i].split_prefix == prefix, "exclusive split scan differs from integer model");
    require(answers[i].action == (i % 3 ? 0U : 1U), "cached mark changed");
    if (!(i % 3))
      ++prefix;
  }
  require(fixture.api->poll(fixture.session, ticket, &ready, answers.data(),
                            static_cast<uint32_t>(answers.size()), &error) != 0,
          "consumed ticket accepted");
  std::array<DeviceSample, 2> samples{};
  for (auto &sample : samples) {
    sample.destination = 0;
    sample.input.original = cache.roots()[plane_slot].original;
    sample.input.uv[0] = cache.roots()[plane_slot].prepared_box[0];
    sample.input.uv[1] = cache.roots()[plane_slot].prepared_box[2];
  }
  require(fixture.api->samples(fixture.session, samples.data(), 2, &ticket, &error) != 0,
          "duplicate sample destinations accepted");
  const auto &root = cache.roots()[rational_slot];
  const auto destination = static_cast<uint32_t>(cache.controls().size());
  DeviceRestriction restriction{{rational_slot}, root.controls, destination, SplitAxis::u, 0};
  const std::array duplicates{restriction, restriction};
  require(fixture.api->restrict_nodes(fixture.session, duplicates.data(), 2, &ticket, &error) != 0,
          "overlapping restriction destinations accepted");
  auto overlap = restriction;
  overlap.destination = root.controls.first;
  require(fixture.api->restrict_nodes(fixture.session, &overlap, 1, &ticket, &error) != 0,
          "restriction overwrites batch parent read");
  checked(fixture.api->restrict_nodes(fixture.session, &restriction, 1, &ticket, &error), error);
  require(assignment->retire(fixture.session, 0, 1, &error) != 0,
          "retirement consumed outstanding device restriction");
  fixture.poll(ticket, nullptr, 0);
  checked(fixture.api->samples(fixture.session, samples.data(), 1, &ticket, &error), error);
  SampleResult point{};
  fixture.poll(ticket, &point, 1);
  require(point.status == NumericStatus::ready, "authored plane point unresolved");
  const auto expected_point = point;
  // Original authored fixture: S(u,v)=(2+2u+v,-1+3v,1/2), here u=v=0.
  require(point.actual_xyz[0] == 2. && point.actual_xyz[1] == -1. && point.actual_xyz[2] == .5,
          "original affine plane value differs from authored formula");
  require(assignment->retire(fixture.session, 1, 2, &error) != 0 &&
              assignment->retire(fixture.session, 0, 0, &error) != 0,
          "stale/nonmonotone assignment retirement accepted");
  checked(assignment->retire(fixture.session, 0, 1, &error), error);
  auto stale_child = restriction;
  stale_child.parent = {destination, root.controls.count};
  stale_child.destination = destination + root.controls.count;
  require(fixture.api->restrict_nodes(fixture.session, &stale_child, 1, &ticket, &error) != 0,
          "retired child control range remained valid");
  auto stale_point = nodes.front();
  stale_point.cached = 0;
  std::fill(std::begin(stale_point.points), std::end(stale_point.points), 0);
  require(fixture.api->nodes(fixture.session, &stale_point, 1, &ticket, &error) != 0,
          "retired point range remained valid");
  checked(fixture.api->restrict_nodes(fixture.session, &restriction, 1, &ticket, &error), error);
  fixture.poll(ticket, nullptr, 0);
  checked(fixture.api->samples(fixture.session, samples.data(), 1, &ticket, &error), error);
  fixture.poll(ticket, &point, 1);
  require(point.status == NumericStatus::ready &&
              std::ranges::equal(point.actual_xyz, expected_point.actual_xyz),
          "source bank changed after assignment retirement");
  checked(assignment->retire(fixture.session, 1, 2, &error), error);
  // Destroy also drains an actual outstanding point operation.
  checked(fixture.api->samples(fixture.session, samples.data(), 1, &ticket, &error), error);
  fixture.close();
  require(fixture.resource->live() == before, "healthy drain retained native reservation");
  std::cout << "adaptive device protocol PASS: 513 prefixes, finite refusals, drain device "
            << device << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 4, "expected SOURCE MODULE DEVICE");
    int32_t device = -1;
    const std::string_view text(argv[3]);
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), device);
    require(parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && device >= 0,
            "invalid device index");
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
    const auto source = load_source(argv[1], resource);
    bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 32}, resource.get());
    const std::array requests{
        CarrierRequest{source.view().carrier_index.find(CarrierId{9}), {0., 1., 0., 1.}},
        CarrierRequest{source.view().carrier_index.find(CarrierId{10}), {0., 1., 0., 1.}},
        CarrierRequest{source.view().carrier_index.find(CarrierId{11}), {0., 1., 0., 1.}}};
    const auto cache = prepare_source_cache(source, requests, resource, {}, {}, budget);
    exercise(cache, argv[2], device);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
  } catch (const bgfd::Failure &error) {
    std::cerr << error.what() << '\n';
  }
  return 1;
}
