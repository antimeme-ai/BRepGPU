#ifndef BREPGPU_MESHER_ADAPTIVE_TRACES_HPP
#define BREPGPU_MESHER_ADAPTIVE_TRACES_HPP
#include "adaptive_curve_geometry.hpp"
#include "adaptive_curves_types.hpp"
#include "scheduler.hpp"
#include <array>
#include <memory>
#include <span>

namespace bgmesh::adaptive {
// Original spatial t is literal edge.parameter_range/vertex order. Neither
// use.sense nor edge.curve_same_sense is a second parameter remapping.
[[nodiscard]] Rational map_trace_parameter(const SourceStore &, Slot<CoedgeTag>, const Rational &);
[[nodiscard]] bool trace_axis_is_periodic(const Carrier &, uint32_t axis) noexcept;
// Whole original 2D pcurve IMAGE, including ordered exact chosen endpoints.
// This does not identify each mapped-s subinterval with a chosen subsegment.
[[nodiscard]] std::optional<OriginalLineImage>
match_authoritative_line_image(const SourceStore &, const CurveSourceCache &, Slot<CoedgeTag>,
                               const ExactTable &, std::span<const UseCurveSite>, bgfd::Budget &);

struct TraceIdentity final {
  uint64_t query = 0, face_generation = 0, boundary_generation = 0;
  auto operator<=>(const TraceIdentity &) const = default;
};
struct TraceFaceRequest final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  std::shared_ptr<const CurveSourceCache> curves;
  SourceCache carriers;
  DefinitionBoundarySnapshot boundary;
  PhysicalCurveBounds physical;
  Slot<FaceTag> face;
  TraceIdentity identity;
  ExactTable allowances;
  ExactRef positional, node_source;
  // Reserve a distinct consecutive range of five ordinals for this face
  // operation; no competing constructor chooses an ordinal from the same pool.
  // Owner is original face/generation, or session-unique run owner with
  // definition=UINT32_MAX-4 and generation=query. Actual source/definition
  // membership is independently checked through retained boundary/source.
  ExactTableKey table_key;
};
struct TraceCurveSampleTask final {
  Slot<CurveTag> curve;
  ExactRef parameter;
};
struct TraceCarrierSampleTask final {
  Slot<CarrierTag> carrier;
  std::array<ExactRef, 2> uv;
};
struct TraceCompletionKey final {
  TraceIdentity identity;
  ExactTableKey work_key;
  auto operator<=>(const TraceCompletionKey &) const = default;
};
struct TraceSourceWorkData;
struct TraceSiteWorkData;
struct TraceSitesData;
struct TraceIntervalWorkData;
struct UseTraceResultData;
using TraceSourceWork = std::shared_ptr<const TraceSourceWorkData>;
using TraceSiteWork = std::shared_ptr<const TraceSiteWorkData>;
using TraceSites = std::shared_ptr<const TraceSitesData>;
using TraceIntervalWork = std::shared_ptr<const TraceIntervalWorkData>;
using UseTraceResult = std::shared_ptr<const UseTraceResultData>;

// Whole original face/junction inventory. Runtime executes these immutable
// numerical tasks in stable batch order and returns their exact ordered result
// pool. These functions never call a scheduler or mutate a physical owner.
[[nodiscard]] TraceSourceWork prepare_use_trace(const TraceFaceRequest &, JobContext &);
[[nodiscard]] const ExactTable &trace_values(const TraceSourceWork &);
[[nodiscard]] std::span<const TraceCurveSampleTask> trace_curve_samples(const TraceSourceWork &);
[[nodiscard]] TraceCompletionKey trace_completion_key(const TraceSourceWork &);
[[nodiscard]] TraceSiteWork prepare_trace_sites(const TraceSourceWork &, TraceCompletionKey,
                                                std::span<const CurveSampleResult>, JobContext &);
[[nodiscard]] const ExactTable &trace_values(const TraceSiteWork &);
[[nodiscard]] std::span<const TraceCarrierSampleTask> trace_carrier_samples(const TraceSiteWork &);
[[nodiscard]] TraceCompletionKey trace_completion_key(const TraceSiteWork &);
[[nodiscard]] TraceSites complete_trace_sites(const TraceSiteWork &, TraceCompletionKey,
                                              std::span<const SampleResult>, JobContext &);
[[nodiscard]] TraceIntervalWork prepare_trace_intervals(const TraceSites &, JobContext &);
enum class TraceBoundPurpose : uint32_t { carrier_trace, authoritative_uv };
struct TraceBoundTask final {
  uint32_t interval = 0;
  TraceBoundPurpose purpose = TraceBoundPurpose::carrier_trace;
  AffineTraceBoundInput bound;
};
[[nodiscard]] const ExactTable &trace_values(const TraceIntervalWork &);
[[nodiscard]] TraceCompletionKey trace_completion_key(const TraceIntervalWork &);
[[nodiscard]] std::span<const TraceBoundTask> trace_bound_tasks(const TraceIntervalWork &);
[[nodiscard]] std::span<const Interval> trace_bound_controls(const TraceIntervalWork &);
[[nodiscard]] UseTraceResult complete_trace_intervals(const TraceIntervalWork &, TraceCompletionKey,
                                                      std::span<const CurveNodeBounds>,
                                                      JobContext &);
[[nodiscard]] const ExactTable &trace_values(const UseTraceResult &);
[[nodiscard]] TraceIdentity trace_identity(const UseTraceResult &);
[[nodiscard]] std::span<const UseCurveTrace> trace_uses(const UseTraceResult &);
[[nodiscard]] std::span<const UseCurveSite> trace_sites(const UseTraceResult &);
[[nodiscard]] std::span<const UseCurveInterval> trace_intervals(const UseTraceResult &);
[[nodiscard]] std::span<const BoundaryKey> trace_requests(const UseTraceResult &);
[[nodiscard]] std::span<const SeamWedge> trace_wedges(const UseTraceResult &);
[[nodiscard]] uint64_t trace_exact_fallbacks(const UseTraceResult &);
} // namespace bgmesh::adaptive
#endif
