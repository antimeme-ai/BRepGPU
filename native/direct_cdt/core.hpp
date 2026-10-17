#ifndef BREPGPU_DIRECT_CDT_CORE_HPP
#define BREPGPU_DIRECT_CDT_CORE_HPP
#include "predicates.hpp"
#include <array>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <vector>

namespace bgcdt {
struct CoreStats final {
  uint64_t merge_steps = 0, crossed_edges = 0, cavity_occurrences = 0;
  uint64_t cavity_insertions = 0, secondary_deletions = 0, max_cavity = 0;
  uint64_t quadedge_peak = 0, triangle_peak = 0, constraints = 0;
  uint64_t location_steps = 0, hash_probes = 0, secondary_calls = 0;
  uint64_t initial_triangles = 0, void_triangles = 0;
  uint64_t cap_clockwise_seeds = 0, cap_vertical_seeds = 0;
  uint64_t order_ns = 0, initial_ns = 0, conversion_ns = 0, recovery_ns = 0, selection_ns = 0;
};
using TriangleWord = std::array<Slot, 3>;
struct RegionInventory final {
  uint64_t boundary_sites, positive_cycles, negative_cycles;
};
[[nodiscard]] std::pmr::vector<TriangleWord>
solve_directed_regions(Predicates &, bgfd::Budget &, std::span<const uint32_t> slots,
                       std::span<const bgfd_loop> loops, RegionInventory, uint64_t seed,
                       CoreStats &);
[[nodiscard]] std::pmr::vector<TriangleWord> solve(Predicates &predicates, bgfd::Budget &budget,
                                                   std::span<const uint32_t> slots,
                                                   std::span<const bgfd_loop> loops, uint64_t seed,
                                                   CoreStats &stats);
} // namespace bgcdt
#endif
