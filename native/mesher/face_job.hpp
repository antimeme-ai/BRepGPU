#ifndef BREPGPU_MESHER_FACE_JOB_HPP
#define BREPGPU_MESHER_FACE_JOB_HPP
#include "face_engine.hpp"
#include "scheduler.hpp"

namespace bgmesh {
struct FaceOwnerInput final {
  FaceBoundarySnapshot boundary;
  ExactTable metric_values;
  std::array<ExactRef, 4> fixed_factor;
  // Physical diagonal squared, chart widths u/v, local positional allowance.
  std::array<ExactRef, 4> scales;
  uint64_t seed;
};
struct FaceOwnerLimits final {
  uint64_t bytes, work;
  uint32_t cell_batch;
};
enum class FaceOwnerEventKind { unstarted, cells, candidate, boundary_horizon, settled, finished };
struct FaceOwnerEvent final {
  FaceOwnerEventKind kind = FaceOwnerEventKind::unstarted;
  FaceCellWork cells;
  FaceCandidateWork candidate;
  std::optional<BoundaryKey> boundary;
};
struct FaceOwnerWork final {
  uint64_t exact_work = 0, peak_bytes = 0;
};
// Coordinator transfers inputs only between completed scheduler steps. One
// retained engine/arena owns the entire face generation, including transactions
// staged across child yields. Child packets never borrow its arithmetic owner.
// The required token is the submitting scheduler's token, so cancellation also
// interrupts arithmetic retained across steps.
class FaceOwnerJob final : public NativeTask {
  struct Impl;
  std::shared_ptr<RunResource> resource_;
  Impl *impl_;

public:
  FaceOwnerJob(const std::shared_ptr<RunResource> &, FaceOwnerInput, FaceOwnerLimits,
               std::stop_token);
  ~FaceOwnerJob() override;
  StepState step(JobContext &) override;
  [[nodiscard]] const FaceOwnerEvent &event() const;
  void finish_cells(std::span<const CellBoundResult>);
  void finish_candidate(FaceCandidateResult);
  void mark_contacts(std::span<const CellResultKey>);
  void finish_generation();
  [[nodiscard]] const FaceEngine &engine() const;
  // Read between steps; the coordinator folds this once per completion.
  [[nodiscard]] FaceOwnerWork take_owner_work();
};
} // namespace bgmesh
#endif
