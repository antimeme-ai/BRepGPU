#ifndef BREPGPU_MESHER_GEOMETRY_CHARTS_HPP
#define BREPGPU_MESHER_GEOMETRY_CHARTS_HPP
#include "geometry_jets.hpp"

namespace bgmesh {
enum class WorkingChartKind { original_uv, analytic_unwrapped_uv, physical_plane_convex_q };
struct ChartProfileLimits final {
  uint32_t subdivision_depth = 8;
  uint64_t cells = 65536;
};
struct FaceEnvelopeRequest final {
  Slot<FaceTag> face;
  uint64_t generation;
  GeometryBoundPrecision precision{};
};
struct FaceChartRequest final {
  Slot<FaceTag> face;
  uint64_t generation;
  Rational positional_allowance, node_source_allowance;
  GeometryBoundPrecision precision{};
  ChartProfileLimits profile_limits{};
  bool allow_plane_profile = true;
};
struct UseEnvelope final {
  Slot<CoedgeTag> use;
  std::array<Rational, 2> mapped_parameter_range;
  std::array<Rational, 4> uv;
};
// Owner-local exact envelopes. They bound evaluation/locator preparation, not
// trim embedding or material ownership. A retained Store qualifies typed slots.
struct FaceEnvelope final {
  GeometryStore geometry;
  Slot<FaceTag> face;
  Slot<CarrierTag> carrier;
  uint64_t generation;
  std::array<Rational, 4> uv;
  std::pmr::vector<UseEnvelope> uses;
  FaceEnvelope(const GeometryStore &, Slot<FaceTag>, Slot<CarrierTag>, uint64_t,
               std::array<Rational, 4>);
  FaceEnvelope(const FaceEnvelope &);
  FaceEnvelope(FaceEnvelope &&) noexcept = default;
  FaceEnvelope &operator=(const FaceEnvelope &) = delete;
  FaceEnvelope &operator=(FaceEnvelope &&) = delete;
};
struct PlaneChart final {
  std::array<Rational, 4> natural_rectangle;
  std::array<Rational, 3> origin;
  // Columns X,Y (interleaved peraxis) and rows of exact dual projection.
  std::array<Rational, 6> basis, projection;
  // Ordered corner XY images (00,10,11,01), and fixed left-preconditioner
  // for the source projected Jacobian used in strong-monotonicity admission.
  std::array<Rational, 8> quadrilateral;
  std::array<Rational, 4> preconditioner;
  Rational height_upper;
  // Construction cache: lower bound for sym(M*J_projection) on natural R.
  Rational inverse_monotonicity_lower;
};
struct WorkingChart final {
  WorkingChartKind kind;
  FaceEnvelope envelope;
  std::optional<PlaneChart> plane;
};
struct DefinitionEnvelopeRequest final {
  Slot<DefinitionTag> definition;
  uint64_t generation;
  GeometryBoundPrecision precision{};
};
struct DefinitionEnvelope final {
  GeometryStore geometry;
  Slot<DefinitionTag> definition;
  uint64_t generation;
  std::array<Rational, 6> xyz;
  std::pmr::vector<FaceEnvelope> faces;
  DefinitionEnvelope(const GeometryStore &, Slot<DefinitionTag>, uint64_t, std::array<Rational, 6>);
  DefinitionEnvelope(const DefinitionEnvelope &);
  DefinitionEnvelope(DefinitionEnvelope &&) noexcept = default;
  DefinitionEnvelope &operator=(const DefinitionEnvelope &) = delete;
  DefinitionEnvelope &operator=(DefinitionEnvelope &&) = delete;
};

[[nodiscard]] FaceEnvelope prepare_face_envelope(GeometryContext &, const GeometryStore &,
                                                 const FaceEnvelopeRequest &);
[[nodiscard]] DefinitionEnvelope prepare_definition_envelope(GeometryContext &,
                                                             const GeometryStore &,
                                                             const DefinitionEnvelopeRequest &);
[[nodiscard]] WorkingChart choose_chart(GeometryContext &, const GeometryStore &,
                                        const FaceChartRequest &);
} // namespace bgmesh
#endif
