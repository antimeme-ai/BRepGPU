#ifndef BREPGPU_MESHER_POLE_SEED_HPP
#define BREPGPU_MESHER_POLE_SEED_HPP
#include "geometry_poles.hpp"
#include "topology.hpp"
namespace bgmesh {
struct FaceBoundarySegment;
// These finite descriptors are frozen by the SAME geometry preparation owner.
// All ExactRefs address FaceBoundarySnapshot.values.  Source occurrence loops
// remain complete; these rows name only the constructed ordinary remainder.
struct FaceConstructionSegment final {
  uint32_t site;
  BoundaryKey constraint;
};
struct FaceConstructionLoop final {
  Slot<LoopTag> source_loop;
  bool outer;
  Range<FaceConstructionSegment> segments;
};
struct FrozenPolePiece final {
  uint32_t cap;
  std::array<uint32_t, 2> cut_sites;
  BoundaryKey cut;
  std::array<ExactRef, 4> cut_uv;
  // Permutes source-map roles (pole,left cut,right cut) into POSITIVE chart
  // connectivity. Publisher alone applies original source face orientation.
  std::array<uint32_t, 3> positive_chart_roles;
  ExactRef position_upper, band_upper;
  ExactRef angular_upper, radial_upper, corner_upper;
};
struct FrozenPoleCap final {
  AnalyticPoleKind kind;
  Slot<CoedgeTag> quotient_use;
  std::array<Slot<CoedgeTag>, 2> meridian_uses;
  std::array<Range<FaceBoundarySegment>, 2> consumed_meridians;
  // Literal original pcurve traversal is separate from collapsed spatial t.
  std::array<ExactRef, 2> pcurve_parameter, original_pole_v;
  uint32_t pole_site;
  Range<FrozenPolePiece> pieces;
  std::array<BoundaryKey, 2> radial_features;
  uint64_t cut_revision;
  int32_t regular_sheet_sign = 1;
  // Canonical profile upper bounds independent of radial/extra-angular edits.
  // Failure to fit is unresolved, not a lower-bound infeasibility claim.
  std::optional<ExactRef> fixed_pole_upper{}, fixed_band_upper{};
  GeometryBoundPrecision precision{};
};
// Coordinator-owned finite input, preserved across a face-cut-only restart.
// Geometry chooses the initial power-of-two partition from its pi/2 admission;
// angular_level is an additional uniform dyadic refinement count, not density.
struct PoleCutHistory final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  Slot<FaceTag> face;
  std::array<uint64_t, 3> generations;
  ExactTable values;
  std::array<ExactRef, 4> cut_uv;
  AnalyticPoleKind kind;
  // Both endpoints must contract before publishing a replacement cut.  A
  // canonical horizon may split just one meridian in each intermediate epoch.
  uint32_t radial_pending_mask;
};
struct PoleCutControl final {
  uint32_t angular_level = 0;
  uint64_t revision = 0;
  std::shared_ptr<const PoleCutHistory> history{};
};
} // namespace bgmesh
#endif
