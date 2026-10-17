#include "../native/direct_cdt/core_detail.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
struct BranchFlags final {
  bool clockwise = false, vertical = false;
};
void fixture(const char *name, const std::vector<std::array<int32_t, 2>> &points,
             BranchFlags &branches) {
  std::vector<uint32_t> limbs{1};
  std::vector<bgfd_node> nodes;
  std::vector<bgcdt::Slot> boundary;
  const auto integer = [&](int32_t value) {
    const int32_t sign = (value > 0) - (value < 0);
    if (sign == 0)
      return bgfd_integer{0, 0, 0};
    const auto offset = static_cast<uint64_t>(limbs.size());
    limbs.push_back(static_cast<uint32_t>(value < 0 ? -int64_t{value} : value));
    return bgfd_integer{offset, 1, sign};
  };
  for (std::size_t i = 0; i < points.size(); ++i) {
    nodes.push_back({static_cast<uint64_t>(i), integer(points[i][0]), integer(points[i][1])});
    boundary.push_back(static_cast<bgcdt::Slot>(i));
  }
  const auto apex = static_cast<bgcdt::Slot>(nodes.size());
  nodes.push_back({apex, {0, 0, 0}, {0, 0, 0}});
  bgfd_input input{};
  input.nodes = nodes.data();
  input.node_count = nodes.size();
  input.limbs = limbs.data();
  input.limb_count = limbs.size();
  input.x_weight = input.y_weight = {0, 1, 1};
  for (uint64_t seed = 0; seed < 16; ++seed) {
    bgfd::Budget budget({UINT64_C(1) << 24, UINT64_C(1) << 24});
    bgcdt::Predicates predicates(budget, input);
    bgcdt::CoreStats stats;
    const auto triangles =
        bgcdt::detail::signed_cap(predicates, budget, boundary, apex, seed, stats);
    require(triangles.size() == points.size() - 2, "cap source triangle count differs");
    require(stats.secondary_calls == 1, "actual signed cap was not invoked");
    branches.clockwise = branches.clockwise || stats.cap_clockwise_seeds != 0;
    branches.vertical = branches.vertical || stats.cap_vertical_seeds != 0;
    for (const auto &triangle : triangles) {
      for (auto slot : triangle)
        require(slot < points.size(), "cap invented boundary occurrence");
      std::cout << name << '\t' << seed << "\ttriangle\t" << triangle[0] << '\t' << triangle[1]
                << '\t' << triangle[2] << '\n';
    }
    std::cout << name << '\t' << seed << "\tstats\t" << stats.cap_clockwise_seeds << '\t'
              << stats.cap_vertical_seeds << '\t' << stats.secondary_deletions << '\n';
  }
}
} // namespace
int main() {
  std::cout << "case\tseed\tkind\ta\tb\tc\n";
  BranchFlags branches;
  fixture("cap-clockwise", {{50, 50}, {0, 1}, {-50, 50}, {-50, -50}, {50, -50}}, branches);
  fixture("cap-vertical", {{50, 50}, {10, 50}, {-10, 50}, {-50, 50}, {-50, -50}, {50, -50}},
          branches);
  require(branches.clockwise, "no clockwise XY cap seed exercised");
  require(branches.vertical, "no vertical XY cap seed exercised");
}
