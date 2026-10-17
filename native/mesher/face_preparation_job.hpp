#ifndef BREPGPU_MESHER_FACE_PREPARATION_JOB_HPP
#define BREPGPU_MESHER_FACE_PREPARATION_JOB_HPP
#include "face_job.hpp"
#include "face_preparation.hpp"
#include "run_preparation.hpp"

namespace bgmesh {
struct FacePreparationInput final {
  PreparedRun run;
  SourceBoundarySnapshot boundary;
  uint64_t generation, seed;
  bool allow_plane_profile = true;
  PoleCutControl pole_control{};
};
struct FacePreparationLimits final {
  uint64_t bytes, work;
  BoundBatchLimits batch;
  GeometryBoundPrecision precision{};
  ChartProfileLimits profile{};
};
enum class FacePreparationEventKind { unstarted, curves, boundary_horizon, ready };
struct FacePreparationEvent final {
  FacePreparationEventKind kind = FacePreparationEventKind::unstarted;
  std::optional<BoundBatch> curves;
  std::optional<BoundaryKey> boundary;
  PoleCutControl pole_control{};
};
// One retained owner through all chord-bound yields. It releases its mutable
// arithmetic when replaced by the face-refinement owner at the same task key.
class FacePreparationJob final : public NativeTask {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  FacePreparationJob(const std::shared_ptr<RunResource> &, FacePreparationInput,
                     FacePreparationLimits, std::stop_token);
  ~FacePreparationJob() override;
  StepState step(JobContext &) override;
  [[nodiscard]] const FacePreparationEvent &event() const;
  void finish_curves(BoundCompleted);
  [[nodiscard]] const FaceOwnerInput &result() const;
  [[nodiscard]] FaceOwnerWork take_owner_work();
};
// A candidate imports only immutable work into its scheduler-private scratch.
class CandidatePointJob final : public NativeTask {
  FaceCandidateWork input_;
  std::optional<FaceCandidateResult> output_;
  bool started_ = false;

public:
  explicit CandidatePointJob(FaceCandidateWork);
  StepState step(JobContext &) override;
  [[nodiscard]] const FaceCandidateResult &result() const;
};
} // namespace bgmesh
#endif
