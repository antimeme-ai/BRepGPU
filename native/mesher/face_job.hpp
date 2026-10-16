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
struct FaceHorizonCell final {
  CellResultKey cell;
  std::array<PhysicalNode, 3> nodes;
  std::array<DeclaredContactEdge, 3> edges;
};
struct FaceHorizonData final {
  std::shared_ptr<RunResource> resource;
  FaceFrontierIdentity face;
  std::pmr::vector<TriangleHandle> removed;
  std::pmr::vector<FaceHorizonCell> inserted;
  FaceHorizonData(std::shared_ptr<RunResource> owner, FaceFrontierIdentity identity)
      : resource(std::move(owner)), face(identity), removed(resource.get()),
        inserted(resource.get()) {}
};
using FaceHorizon = std::shared_ptr<const FaceHorizonData>;
struct FaceOptimizerEdge final {
  EdgeHandle edge;
  uint64_t key;
};
struct FaceOptimizerReplyData final {
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<FaceOptimizerEdge> edges;
  std::optional<std::array<CellResultKey, 2>> committed;
  explicit FaceOptimizerReplyData(std::shared_ptr<RunResource> owner)
      : resource(std::move(owner)), edges(resource.get()) {}
};
using FaceOptimizerReply = std::shared_ptr<const FaceOptimizerReplyData>;
enum class FaceOwnerEventKind {
  unstarted,
  cells,
  candidate,
  boundary_horizon,
  settled,
  optimizer_queue,
  optimizer_proposal,
  optimizer_committed,
  finished
};
struct FaceOwnerEvent final {
  FaceOwnerEventKind kind = FaceOwnerEventKind::unstarted;
  FaceCellWork cells;
  FaceCandidateWork candidate;
  std::optional<BoundaryKey> boundary;
  FaceHorizon horizon{};
  FaceOptimizationWork optimization{};
  FaceOptimizerReply optimizer{};
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
  void begin_optimizer();
  void prepare_flip(EdgeHandle);
  void reject_flip();
  void finish_flip(const std::array<CellBoundResult, 2> &, ContactApproval);
  [[nodiscard]] FaceResult result() const;
  [[nodiscard]] const FaceEngine &engine() const;
  // Read between steps; the coordinator folds this once per completion.
  [[nodiscard]] FaceOwnerWork take_owner_work();
};
} // namespace bgmesh
#endif
