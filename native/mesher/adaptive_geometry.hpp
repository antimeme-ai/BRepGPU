#ifndef BREPGPU_MESHER_ADAPTIVE_GEOMETRY_HPP
#define BREPGPU_MESHER_ADAPTIVE_GEOMETRY_HPP
#include "exact.hpp"
#include "source_cache.hpp"
namespace bgmesh::adaptive {
struct PreciseBoundResult {
  NodeBounds bounds;
  bool certified;
};
struct PreciseFacetResult {
  ActualFacetBounds bounds;
  bool certified;
};
// Source-only selected-root fallback during compilation. The temporary PMR
// values belong to the supplied owner; SourceCache copies intervals before seal.
[[nodiscard]] std::pmr::vector<Interval>
enclose_source_root_precise(const SourceStore &, const RootDescriptor &, bgfd::Budget &);
void admit_numeric_environment();
[[nodiscard]] SampleResult evaluate_sample_precise(const SourceCache &, OriginalRootSlot,
                                                   std::span<const Rational, 2>, ArithmeticStage,
                                                   bgfd::Budget &);
[[nodiscard]] PreciseBoundResult bound_node_precise(const SourceCache &, RootSlot,
                                                    std::span<const Rational, 4>,
                                                    std::span<const double, 12>, const Rational &,
                                                    bgfd::Budget &);
[[nodiscard]] PreciseFacetResult bound_actual_facet_precise(const ActualFacetInput &,
                                                            std::span<const Rational, 6>,
                                                            std::span<const Rational, 9>,
                                                            const Rational &, bgfd::Budget &);
[[nodiscard]] SampleResult evaluate_sample(const SourceCache &, const SampleInput &);
[[nodiscard]] std::array<Rational,3> exact_original_surface_point(const SourceCache&,OriginalRootSlot,
    std::span<const Rational,2>,bgfd::Budget&);
[[nodiscard]] NodeBounds bound_node(const SourceCache &, const NodePredicate &,
                                    std::span<const Interval> node_controls = {});
[[nodiscard]] ActualFacetBounds bound_actual_facet(const ActualFacetInput &);
} // namespace bgmesh::adaptive
#endif
