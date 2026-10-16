#ifndef BREPGPU_DIRECT_CDT_CORE_DETAIL_HPP
#define BREPGPU_DIRECT_CDT_CORE_DETAIL_HPP
#include "core.hpp"
namespace bgcdt::detail {
// Internal owning subroutine, shared by production recovery and native fixtures.
// Input is the cyclic boundary of a lifted-convex deletion cap, excluding apex.
// Output indexes boundary occurrences, preserving distinct occurrence identity.
[[nodiscard]] std::pmr::vector<TriangleWord>
signed_cap(Predicates &predicates, bgfd::Budget &budget, std::span<const Slot> boundary,
           Slot deleted_apex, uint64_t seed, CoreStats &stats);
} // namespace bgcdt::detail
#endif
