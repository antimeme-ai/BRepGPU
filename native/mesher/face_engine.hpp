#ifndef BREPGPU_MESHER_FACE_ENGINE_HPP
#define BREPGPU_MESHER_FACE_ENGINE_HPP
#include "cell_bounds.hpp"
#include "contact_epoch.hpp"
#include "face_frontier.hpp"
#include "face_result.hpp"
#include "face_seed.hpp"
#include "metrics.hpp"

namespace bgmesh {
struct FaceMetricView final {
  const std::array<Rational, 4> &fixed_factor;
  MetricScales scales;
};
struct FaceCellInput final {
  BoundTaskHeader header;
  Slot<CarrierTag> carrier;
  CellMapKind map_kind;
  std::array<ExactRef, 6> original_uv;
  std::array<ExactRef, 9> actual_xyz;
  ExactRef allowance;
  // Working-chart coordinates are explicit. For plane cells original_uv is a
  // recovery/display locator, never a parametric residual domain.
  std::array<ExactRef, 6> chart_xy{};
};
struct FaceCellWorkData final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  ExactTable values;
  std::pmr::vector<FaceCellInput> cells;
  FaceBoundarySnapshot chart_source;
  FaceCellWorkData(std::shared_ptr<RunResource> owner, GeometryStore store, ExactTable table,
                   FaceBoundarySnapshot snapshot = {})
      : resource(std::move(owner)), geometry(std::move(store)), values(std::move(table)),
        cells(resource.get()), chart_source(std::move(snapshot)) {}
};
using FaceCellWork = std::shared_ptr<const FaceCellWorkData>;
// The refinement transaction stays within this face owner. Child work retains
// only these values, never its Budget or a mutable topology reference.
enum class FaceCandidateKind { heuristic, longest_edge_midpoint };
struct FaceCandidateData final {
  FaceBoundarySnapshot boundary;
  CellResultKey target;
  uint64_t ordinal;
  FaceCandidateKind kind;
  ExactTable values;
  std::array<ExactRef, 2> chart, original_uv;
  // Plane work reserves the response table before child dispatch. original_uv
  // is then only a valid source-domain Newton locator hint; chart is XY truth.
  std::optional<ExactTableKey> result_table_key{};
};
using FaceCandidateWork = std::shared_ptr<const FaceCandidateData>;
enum class FaceCandidateDecision { admitted, protected_feature };
struct CandidateLocator final {
  ExactTable values;
  std::array<ExactRef, 2> uv;
  std::array<ExactRef, 4> enclosure;
};
struct FaceCandidateResult final {
  CellResultKey target;
  uint64_t ordinal;
  FaceCandidateDecision decision;
  Vec3 xyz;
  std::optional<BoundaryKey> feature;
  std::optional<CandidateLocator> locator{};
};
enum class FaceAdvanceKind { pending, unresolved, candidate, boundary_request, settled };
struct FaceAdvance final {
  FaceAdvanceKind kind;
  std::optional<CellResultKey> cell;
  FaceCandidateWork candidate;
  std::optional<BoundaryKey> boundary;
};
struct FaceOptimizationData final {
  FaceBoundarySnapshot boundary;
  FaceCellWork cells;
  uint64_t ordinal;
  EdgeHandle edge;
  std::array<CellResultKey, 2> old_cells;
  std::array<std::array<SiteSlot, 3>, 2> sites;
  std::array<std::array<PhysicalNode, 3>, 2> nodes;
  std::array<std::array<DeclaredContactEdge, 3>, 2> edges;
  std::array<double, 2> old_potential, new_potential;
};
using FaceOptimizationWork = std::shared_ptr<const FaceOptimizationData>;
struct FaceOptimizationCommit final {
  std::array<CellResultKey, 2> cells;
  std::pmr::vector<EdgeHandle> affected_edges;
  explicit FaceOptimizationCommit(std::pmr::memory_resource *resource) : affected_edges(resource) {}
};
struct FaceEngineStats final {
  uint64_t cell_packets = 0, candidates = 0, accepted_candidates = 0;
  uint64_t rejected_results = 0, optimizer_proposals = 0, optimizer_flips = 0;
  uint64_t domain_failures = 0;
  TopologyStats topology;
  FaceFrontierStats frontier;
};
// One persistent Budget/GeometryContext supplied by the root NativeTask owner.
// No executor, waits, Python evaluator or alternate source model is introduced.
class FaceEngine final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  FaceEngine(bgfd::Budget &, GeometryContext &, FaceBoundarySnapshot, FaceMetricView,
             const Rational &allowance, uint64_t seed);
  ~FaceEngine();
  FaceEngine(const FaceEngine &) = delete;
  FaceEngine &operator=(const FaceEngine &) = delete;
  [[nodiscard]] std::optional<FaceCellWork> take_cell_work(uint32_t maximum);
  [[nodiscard]] bool complete(const CellBoundResult &);
  [[nodiscard]] FaceAdvance advance();
  [[nodiscard]] bool complete_candidate(const FaceCandidateResult &);
  [[nodiscard]] CellResultKey retry(CellResultKey);
  [[nodiscard]] bool mark_contact_failure(CellResultKey);
  [[nodiscard]] std::pmr::vector<TopologyEdit> take_changes();
  [[nodiscard]] std::pmr::vector<CellResultKey> take_settled();
  [[nodiscard]] const MutableFaceTopology &topology() const;
  [[nodiscard]] const PhysicalNode &node(SiteSlot) const;
  [[nodiscard]] std::span<const int32_t> traversal_adapters() const;
  [[nodiscard]] std::array<DeclaredContactEdge, 3> declared_edges(TriangleHandle) const;
  [[nodiscard]] FaceEngineStats stats() const;
  // Caller starts only after the complete definition contact horizon is clear.
  [[nodiscard]] std::pmr::vector<EdgeHandle> begin_optimizer();
  [[nodiscard]] uint64_t optimizer_edge_key(EdgeHandle) const;
  [[nodiscard]] std::optional<FaceOptimizationWork> prepare_flip(EdgeHandle);
  void reject_flip(uint64_t ordinal);
  [[nodiscard]] FaceOptimizationCommit
  complete_flip(uint64_t ordinal, const std::array<CellBoundResult, 2> &, const ContactApproval &);
  // Successful final horizon only; repeated export returns the same lease.
  // Afterwards topology/work mutation is prohibited. Read-only stats remain valid.
  [[nodiscard]] FaceResult freeze_result();
};
} // namespace bgmesh
#endif
