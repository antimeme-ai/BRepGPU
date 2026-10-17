#ifndef BREPGPU_MESHER_FACE_PREPARATION_HPP
#define BREPGPU_MESHER_FACE_PREPARATION_HPP
#include "bound_runtime.hpp"
#include "boundary.hpp"
#include "face_seed.hpp"
namespace bgmesh {
struct DefinitionEnvelopeView;
struct FrozenFaceMetric final {
  ExactTable values;
  std::array<ExactRef,4> factor,scales;
};
[[nodiscard]] FrozenFaceMetric prepare_face_metric(GeometryContext &,FaceBoundarySnapshot,
                                                    const DefinitionEnvelopeView &,const Rational &allowance,
                                                    GeometryBoundPrecision = {});
enum class FacePreparationState { curve_work, boundary_request, ready };
// One retained owner arena/context supplied by the root preparation task. Source
// and boundary inputs are immutable; no Rational from another owner is borrowed.
class FaceBoundaryPreparation final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  FaceBoundaryPreparation(bgfd::Budget &, GeometryContext &, GeometryStore, SourceBoundarySnapshot,
                          const FaceChartRequest &,PoleCutControl = {});
  ~FaceBoundaryPreparation();
  FaceBoundaryPreparation(const FaceBoundaryPreparation &) = delete;
  FaceBoundaryPreparation &operator=(const FaceBoundaryPreparation &) = delete;
  [[nodiscard]] FacePreparationState state() const;
  [[nodiscard]] std::optional<BoundaryKey> boundary_request() const;
  [[nodiscard]] std::pmr::vector<BoundRootSelection> curve_roots() const;
  [[nodiscard]] std::optional<BoundBatch> take_curve_work(const BoundRootBank &, BoundBatchLimits);
  void complete(const BoundCompleted &);
  [[nodiscard]] FaceBoundarySnapshot snapshot();
  [[nodiscard]] PoleCutControl next_pole_control() const;
};
} // namespace bgmesh
#endif
