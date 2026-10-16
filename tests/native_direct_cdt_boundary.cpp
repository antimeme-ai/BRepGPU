#include "../native/direct_cdt/direct_cdt.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace {
void require(bool condition) {
  if (!condition)
    std::abort();
}
bgfd_error error() {
  bgfd_error result{};
  result.abi = BGFD_ABI;
  result.size = sizeof(result);
  return result;
}
// These original integer cases also run through the independent Python CDT
// reader. Here they exercise native allocation/index/recovery lifetimes under
// the instrumented host, without loading an ASan DSO into Python.
void domain_fixture(const std::vector<std::array<int32_t, 2>> &points,
                    const std::vector<uint32_t> &lengths) {
  std::vector<uint32_t> limbs{1}, slots;
  std::vector<bgfd_node> nodes;
  std::vector<bgfd_loop> loops;
  const auto integer = [&](int32_t coordinate) {
    const int sign = (coordinate > 0) - (coordinate < 0);
    if (sign == 0)
      return bgfd_integer{0, 0, 0};
    const auto magnitude =
        static_cast<uint32_t>(coordinate < 0 ? -int64_t{coordinate} : coordinate);
    const auto offset = static_cast<uint64_t>(limbs.size());
    limbs.push_back(magnitude);
    return bgfd_integer{offset, 1, sign};
  };
  for (std::size_t i = 0; i < points.size(); ++i) {
    nodes.push_back({static_cast<uint64_t>(i), integer(points[i][0]), integer(points[i][1])});
    slots.push_back(static_cast<uint32_t>(i));
  }
  uint64_t offset = 0;
  for (uint32_t length : lengths) {
    loops.push_back({offset, length});
    offset += length;
  }
  require(offset == points.size());
  const bgfd_integer one{0, 1, 1};
  bgcdt_input input{};
  input.abi = BGCDT_ABI;
  input.size = sizeof(input);
  input.domain = {BGFD_ABI,
                  sizeof(bgfd_input),
                  0,
                  0,
                  1U << 24,
                  1U << 24,
                  nodes.data(),
                  nodes.size(),
                  loops.data(),
                  loops.size(),
                  slots.data(),
                  slots.size(),
                  limbs.data(),
                  limbs.size(),
                  one,
                  one};
  auto failure = error();
  bgcdt_context *context = nullptr;
  require(bgcdt_create(&input, &context, &failure) == BGFD_OK);
  require(bgcdt_construct(context, &failure) == BGFD_OK);
  uint64_t required = 0;
  require(bgcdt_copy_triangles(context, nullptr, 0, &required, &failure) == BGFD_OK);
  require(required == 3 * (points.size() + 2 * (lengths.size() - 1) - 2));
  std::vector<uint64_t> output(static_cast<std::size_t>(required));
  require(bgcdt_copy_triangles(context, output.data(), required, &required, &failure) == BGFD_OK);
  for (uint64_t id : output)
    require(id < points.size());
  bgcdt_destroy(context);
}
} // namespace

int main() {
  require(bgcdt_abi_size(0) == BGCDT_ABI);
  require(bgcdt_abi_size(1) == sizeof(bgcdt_input));
  require(bgcdt_abi_size(2) == sizeof(bgfd_error));
  require(bgcdt_abi_size(3) == sizeof(bgcdt_stats));
  bgcdt_destroy(nullptr);
  auto failure = error();
  bgcdt_context *context = nullptr;
  require(bgcdt_create(nullptr, &context, &failure) == BGFD_INVALID_INPUT);
  require(context == nullptr);

  const std::array<uint32_t, 1> limbs{1};
  const bgfd_integer zero{0, 0, 0}, one{0, 1, 1};
  const std::array<bgfd_node, 3> nodes{{{0, zero, zero}, {UINT64_MAX, one, zero}, {7, zero, one}}};
  const std::array<bgfd_loop, 1> loops{{{0, 3}}};
  const std::array<uint32_t, 3> slots{0, 1, 2};
  bgcdt_input input{};
  input.abi = BGCDT_ABI;
  input.size = sizeof(input);
  input.seed = UINT64_MAX;
  input.domain = {BGFD_ABI,     sizeof(bgfd_input), 0,
                  79,           1U << 20,           1U << 20,
                  nodes.data(), nodes.size(),       loops.data(),
                  loops.size(), slots.data(),       slots.size(),
                  limbs.data(), limbs.size(),       one,
                  one};
  require(bgcdt_create(&input, &context, &failure) == BGFD_OK);
  require(context != nullptr);
  bgcdt_stats prepared{};
  prepared.abi = BGCDT_ABI;
  prepared.size = sizeof(prepared);
  require(bgcdt_get_stats(context, &prepared, &failure) == BGFD_OK);
  uint64_t required = 0;
  require(bgcdt_copy_triangles(context, nullptr, 0, &required, &failure) == BGFD_INVALID_INPUT);
  require(bgcdt_construct(context, &failure) == BGFD_OK);
  require(bgcdt_construct(context, &failure) == BGFD_OK);
  require(bgcdt_copy_triangles(context, nullptr, 0, &required, &failure) == BGFD_OK);
  require(required == 3);
  std::array<uint64_t, 3> triangle{};
  require(bgcdt_copy_triangles(context, triangle.data(), triangle.size(), &required, &failure) ==
          BGFD_OK);
  bool positive = false;
  for (std::size_t i = 0; i < triangle.size(); ++i)
    positive = positive || (triangle[i] == 0 && triangle[(i + 1) % 3] == UINT64_MAX &&
                            triangle[(i + 2) % 3] == 7);
  require(positive);
  bgcdt_stats stats{};
  stats.abi = BGCDT_ABI;
  stats.size = sizeof(stats);
  require(bgcdt_get_stats(context, &stats, &failure) == BGFD_OK);
  require(stats.token == 79 && stats.seed == UINT64_MAX && stats.triangles == 1);
  require(stats.work >= stats.validation_work && stats.work > 0);
  require(stats.memory_peak <= input.domain.memory_limit);
  bgcdt_destroy(context);
  context = nullptr;

  input.domain.work_limit = prepared.work;
  require(bgcdt_create(&input, &context, &failure) == BGFD_OK);
  require(bgcdt_construct(context, &failure) == BGFD_RESOURCE_EXHAUSTED);
  require(bgcdt_copy_triangles(context, nullptr, 0, &required, &failure) == BGFD_INVALID_INPUT);
  require(bgcdt_construct(context, &failure) == BGFD_INVALID_INPUT);
  require(bgcdt_get_stats(context, &stats, &failure) == BGFD_OK);
  require(stats.work <= input.domain.work_limit && stats.triangles == 0);
  bgcdt_destroy(context);
  context = nullptr;

  input.domain.work_limit = 0;
  require(bgcdt_create(&input, &context, &failure) == BGFD_RESOURCE_EXHAUSTED);
  require(context == nullptr);
  input.domain.work_limit = 1U << 20;
  input.domain.memory_limit = 0;
  require(bgcdt_create(&input, &context, &failure) == BGFD_RESOURCE_EXHAUSTED);
  require(context == nullptr);
  input.domain.memory_limit = 1U << 20;
  input.reserved = 1;
  require(bgcdt_create(&input, &context, &failure) == BGFD_INVALID_INPUT);
  require(context == nullptr);
  input.reserved = 0;
  input.size = 0;
  require(bgcdt_create(&input, &context, &failure) == BGFD_INVALID_INPUT);
  input.size = sizeof(input);
  failure.reserved = 1;
  require(bgcdt_create(&input, &context, &failure) == BGFD_INVALID_INPUT);
  require(context == nullptr);
  failure = error();
  input.domain.x_weight.offset = UINT64_MAX;
  require(bgcdt_create(&input, &context, &failure) == BGFD_INVALID_INPUT);
  require(context == nullptr);
  domain_fixture({{0, 0},
                  {12, 0},
                  {12, 5},
                  {10, 5},
                  {10, 1},
                  {9, 1},
                  {9, 5},
                  {7, 5},
                  {7, 1},
                  {6, 1},
                  {6, 5},
                  {4, 5},
                  {4, 1},
                  {3, 1},
                  {3, 5},
                  {0, 5}},
                 {16});
  domain_fixture({{0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 8}, {8, 8}, {8, 2}, {0, 2}}, {8});
  domain_fixture({{0, 0},
                  {20, 0},
                  {20, 12},
                  {0, 12},
                  {2, 2},
                  {2, 5},
                  {5, 5},
                  {5, 2},
                  {8, 6},
                  {8, 10},
                  {11, 10},
                  {11, 6},
                  {14, 3},
                  {14, 5},
                  {16, 3}},
                 {4, 4, 4, 3});
  return 0;
}
