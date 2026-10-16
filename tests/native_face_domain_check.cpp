#include "exact.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "face-domain check failed: %s\n", message);
    std::exit(1);
  }
}
void arithmetic() {
  bgfd::Budget budget({.bytes = 1U << 20, .work = 1U << 24});
  const std::array<uint32_t, 4> words = {0xffffffffU, 0xffffffffU, 1U, 1U};
  const auto a = bgfd::Integer::decode(budget, {0, 2, 1}, words.data(), words.size());
  const auto one = bgfd::Integer::decode(budget, {2, 1, 1}, words.data(), words.size());
  const std::array<uint32_t, 3> power = {0, 0, 1};
  const auto t = bgfd::Integer::decode(budget, {0, 3, 1}, power.data(), power.size());
  require(a.add(one).compare(t) == 0, "carry across two full limbs");
  require(t.subtract(one).compare(a) == 0, "borrow across two zero limbs");
  require(a.add(a.negative()).sign() == 0, "signed cancellation canonical zero");
  const auto b = a.subtract(one), c = b.subtract(one);
  require(a.multiply(c).subtract(b.multiply(b)).compare(one.negative()) == 0,
          "wide product cancellation is minus one");
  require(a.negative().multiply(c).sign() < 0, "negative multiplication");
  bool rejected = false;
  try {
    static_cast<void>(bgfd::Integer::decode(budget, {0, 3, 1}, words.data(), 2));
  } catch (const bgfd::Failure &error) {
    rejected = error.status == BGFD_INVALID_INPUT;
  }
  require(rejected, "descriptor bound rejection");
}
void hole_reuse() {
  const std::array<uint32_t, 4> limbs = {1, 2, 4, 7};
  const bgfd_integer zero{0, 0, 0}, one{0, 1, 1}, two{1, 1, 1}, four{2, 1, 1}, seven{3, 1, 1};
  const std::array<bgfd_node, 7> nodes = {bgfd_node{0, zero, zero},   bgfd_node{1, seven, zero},
                                          bgfd_node{2, seven, seven}, bgfd_node{3, zero, seven},
                                          bgfd_node{4, two, two},     bgfd_node{5, two, four},
                                          bgfd_node{6, four, two}};
  const std::array<uint32_t, 7> slots = {0, 1, 2, 3, 4, 5, 6};
  const std::array<bgfd_loop, 2> loops = {bgfd_loop{0, 4}, bgfd_loop{4, 3}};
  bgfd_input input{};
  input.abi = BGFD_ABI;
  input.size = sizeof(input);
  input.memory_limit = 1U << 20;
  input.work_limit = 1U << 24;
  input.nodes = nodes.data();
  input.node_count = nodes.size();
  input.loops = loops.data();
  input.loop_count = loops.size();
  input.slots = slots.data();
  input.slot_count = slots.size();
  input.limbs = limbs.data();
  input.limb_count = limbs.size();
  input.x_weight = one;
  input.y_weight = one;
  bgfd_context *context = nullptr;
  bgfd_error error{};
  error.abi = BGFD_ABI;
  error.size = sizeof(error);
  require(bgfd_create(&input, &context, &error) == BGFD_OK, "create holed domain");
  for (int pass = 0; pass < 3; ++pass) {
    require(bgfd_triangulate(context, &error) == BGFD_OK, "repeat hole tree mutation");
    uint64_t count = 0;
    require(bgfd_copy_triangles(context, nullptr, 0, &count, &error) == BGFD_OK && count == 21,
            "hole triangle count");
    std::array<uint64_t, 21> result{};
    require(bgfd_copy_triangles(context, result.data(), result.size(), &count, &error) == BGFD_OK,
            "hole output copy");
    std::array<bool, 7> seen{};
    for (uint64_t id : result) {
      require(id < seen.size(), "hole original IDs");
      seen[id] = true;
    }
    for (bool used : seen)
      require(used, "hole original sample missing");
    bgfd_crossing crossing{};
    crossing.abi = BGFD_ABI;
    crossing.size = sizeof(crossing);
    require(bgfd_first_crossing(context, &crossing, &error) == BGFD_OK && crossing.found == 0,
            "source tree rebuilt after bridge");
  }
  require(bgfd_triangulate(context, &error) == BGFD_OK, "hole final solve");
  uint64_t count = 0;
  std::array<uint64_t, 1> short_result{};
  require(bgfd_copy_triangles(context, short_result.data(), 1, &count, &error) ==
              BGFD_INVALID_INPUT,
          "short output rejected");
  require(bgfd_copy_triangles(context, nullptr, 0, &count, &error) == BGFD_INVALID_INPUT,
          "failed operation clears stale result");
  bgfd_destroy(context);
  input.slot_count = 6;
  require(bgfd_create(&input, &context, &error) == BGFD_INVALID_INPUT && context == nullptr,
          "hole word overrun rejected");
}
void api() {
  const std::array<uint32_t, 1> limbs = {1};
  const bgfd_integer zero{0, 0, 0}, one{0, 1, 1};
  const std::array<bgfd_node, 4> nodes = {bgfd_node{0, zero, zero}, bgfd_node{1, one, zero},
                                          bgfd_node{2, one, one}, bgfd_node{3, zero, one}};
  const std::array<uint32_t, 4> slots = {0, 1, 2, 3};
  const std::array<bgfd_loop, 1> loops = {bgfd_loop{0, 4}};
  bgfd_input input{};
  input.abi = BGFD_ABI;
  input.size = sizeof(input);
  input.token = std::numeric_limits<uint64_t>::max();
  input.memory_limit = 1U << 20;
  input.work_limit = 1U << 24;
  input.nodes = nodes.data();
  input.node_count = nodes.size();
  input.loops = loops.data();
  input.loop_count = loops.size();
  input.slots = slots.data();
  input.slot_count = slots.size();
  input.limbs = limbs.data();
  input.limb_count = limbs.size();
  input.x_weight = one;
  input.y_weight = one;
  bgfd_context *context = nullptr;
  bgfd_error error{};
  error.abi = BGFD_ABI;
  error.size = sizeof(error);
  require(bgfd_create(&input, &context, &error) == BGFD_OK, "create exact source quad");
  for (int pass = 0; pass < 3; ++pass) {
    require(bgfd_triangulate(context, &error) == BGFD_OK, "repeat exact source quad");
    uint64_t count = 0;
    require(bgfd_copy_triangles(context, nullptr, 0, &count, &error) == BGFD_OK && count == 6,
            "output size");
    std::array<uint64_t, 6> result{};
    require(bgfd_copy_triangles(context, result.data(), result.size(), &count, &error) == BGFD_OK,
            "output copy");
    require(result == std::array<uint64_t, 6>{3, 0, 1, 1, 2, 3}, "exact cursor triples");
    bgfd_stats stats{};
    stats.abi = BGFD_ABI;
    stats.size = sizeof(stats);
    require(bgfd_get_stats(context, &stats, &error) == BGFD_OK && stats.token == input.token &&
                stats.ear_blocker_queries == 0,
            "convex stats/token");
  }
  bgfd_crossing crossing{};
  crossing.abi = BGFD_ABI;
  crossing.size = sizeof(crossing);
  require(bgfd_first_crossing(context, &crossing, &error) == BGFD_OK && crossing.found == 0,
          "no crossing");
  uint64_t count = 0;
  require(bgfd_copy_triangles(context, nullptr, 0, &count, &error) == BGFD_INVALID_INPUT,
          "previous output cleared");
  bgfd_destroy(context);
  bgfd_destroy(nullptr);
  input.nodes = nullptr;
  require(bgfd_create(&input, &context, &error) == BGFD_INVALID_INPUT && context == nullptr,
          "null input array");
  input.nodes = nodes.data();
  input.memory_limit = 1;
  require(bgfd_create(&input, &context, &error) == BGFD_RESOURCE_EXHAUSTED && context == nullptr,
          "memory exhaustion is not answer");
  input.memory_limit = 1U << 20;
  input.work_limit = 1;
  require(bgfd_create(&input, &context, &error) == BGFD_RESOURCE_EXHAUSTED && context == nullptr,
          "work exhaustion is not answer");
  input.work_limit = 1U << 24;
  input.size = 0;
  require(bgfd_create(&input, &context, &error) == BGFD_INVALID_INPUT, "ABI size rejection");
  input.size = sizeof(input);
  error.size = 0;
  require(bgfd_create(&input, &context, &error) == BGFD_INVALID_INPUT && context == nullptr,
          "error output ABI size rejected");
}
} // namespace
int main() {
  arithmetic();
  api();
  hole_reuse();
  std::puts("face-domain exact arithmetic and ownership checks PASS");
}
