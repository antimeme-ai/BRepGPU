#include "../native/mesher/exact.hpp"
#include "../native/mesher/reservations.hpp"
#include <atomic>
#include <barrier>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void prepaid() {
  auto run = std::make_shared<bgmesh::RunResource>(65536);
  {
    auto lease = bgmesh::reserve_resource(run, 8192);
    const auto reserved = run->live();
    require(reserved >= 8192 && reserved < 16384, "full cap and bounded metadata reserved");
    {
      std::pmr::vector<std::byte> values(lease.get());
      values.resize(6000);
      require(run->live() == reserved && lease->live() == values.capacity(),
              "prepaid bytes must not be charged twice");
      bool refused = false;
      try {
        values.reserve(9000);
      } catch (const bgmesh::Error &error) {
        refused = error.code == bgmesh::ErrorCode::resource_exhausted;
      }
      require(refused && values.size() == 6000 && run->live() == reserved,
              "failed prepaid growth preserves allocation and accounting");
    }
    require(lease->live() == 0 && run->live() == reserved,
            "idle reservation remains charged until released");
    {
      bgfd::Budget owner({4096, 100000}, lease.get());
      std::pmr::vector<std::byte> values(&owner);
      values.resize(3000);
      bool refused = false;
      try {
        values.reserve(5000);
      } catch (const bgfd::Failure &error) {
        refused = error.status == BGFD_RESOURCE_EXHAUSTED;
      }
      require(refused && run->live() == reserved, "child Budget has its own hard cap");
    }
  }
  require(run->live() == 0, "reservation metadata and cap released");
  bool refused = false;
  try {
    (void)bgmesh::reserve_resource(run, 65536);
  } catch (const bgmesh::Error &error) {
    refused = error.code == bgmesh::ErrorCode::resource_exhausted;
  }
  require(refused && run->live() == 0, "failed reservation rolls back metadata");
  std::weak_ptr<bgmesh::RunResource> weak = run;
  auto lease = bgmesh::reserve_resource(run, 4096);
  run.reset();
  require(!weak.expired(), "reservation retains parent");
  lease.reset();
  require(weak.expired(), "last reservation releases parent");
}
void simultaneous() {
  constexpr std::ptrdiff_t count = 16;
  auto run = std::make_shared<bgmesh::RunResource>(100000);
  std::barrier attempted(count);
  std::atomic<uint32_t> admitted{0};
  std::vector<std::jthread> threads;
  for (std::ptrdiff_t i = 0; i < count; ++i)
    threads.emplace_back([&] {
      std::shared_ptr<bgmesh::ResourceReservation> lease;
      try {
        lease = bgmesh::reserve_resource(run, 16384);
        admitted.fetch_add(1);
      } catch (const bgmesh::Error &error) {
        if (error.code != bgmesh::ErrorCode::resource_exhausted)
          std::terminate();
      }
      attempted.arrive_and_wait();
    });
  threads.clear();
  require(admitted.load() > 0 && admitted.load() * uint64_t{16384} <= 100000 &&
              run->peak() <= 100000 && run->live() == 0,
          "concurrent admission cannot oversubscribe or leak run storage");
}
} // namespace
int main() {
  try {
    prepaid();
    simultaneous();
    std::cout << "PASS\tprepaid run reservations\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  }
}
