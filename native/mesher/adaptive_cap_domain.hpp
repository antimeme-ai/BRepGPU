#ifndef BREPGPU_MESHER_ADAPTIVE_CAP_DOMAIN_HPP
#define BREPGPU_MESHER_ADAPTIVE_CAP_DOMAIN_HPP
#include "adaptive_curves_types.hpp"
#include "source_cache.hpp"
#include <span>
#include <variant>

namespace bgmesh {
struct JobContext;
namespace adaptive {
struct CapDomainKey final {
  uint64_t source_generation = 0, query = 0;
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  uint64_t boundary_generation = 0, face_generation = 0, chart_revision = 0;
  uint64_t domain_revision = 0, assignment_epoch = 0, profile_revision = 0;
  auto operator<=>(const CapDomainKey &) const = default;
};
struct PhysicalCapEndpoint final {
  Slot<EdgeTag> edge;
  Slot<CoedgeTag> sector;
  ExactRef original_t;
  PhysicalNodeKey expected_key;
};
struct SurfaceCapPoint final {
  OriginalRootSlot original_root;
};
struct CapPointPlan final {
  std::array<ExactRef, 2> uv;
  std::variant<PhysicalCapEndpoint, SurfaceCapPoint> authority;
};
struct CapDomainPiece final {
  uint32_t cap = UINT32_MAX;
  OriginalRootSlot original_root;
  std::array<uint32_t, 2> cut_points;
  std::array<ExactRef, 4> cut_uv;
  std::array<ExactRef, 2> finite_v;
  std::array<uint32_t, 3> positive_chart_roles;
  int32_t regular_sheet_sign = 0;
  BoundaryKey cut_key;
};
struct CapDomainRow final {
  uint32_t wedge = UINT32_MAX;
  std::array<uint32_t, 2> angular_to_sector;
  uint32_t pole_boundary_node = UINT32_MAX;
  Range<CapPointPlan> points;
  Range<CapDomainPiece> pieces;
  uint32_t transcendental_bits = 96, norm_fractional_bits = 96;
  // Original incoming/outgoing wedge sectors, independent of angular order.
  std::array<BoundaryKey, 2> meridian_intervals{};
};
struct CapCutControlData;
struct ProducedCapDomainData;
class CapDomainProducer;
class CapCutControlLease final {
  std::shared_ptr<const CapCutControlData> data_;
  explicit CapCutControlLease(std::shared_ptr<const CapCutControlData> data)
      : data_(std::move(data)) {}
  friend class CapDomainProducer;

public:
  CapCutControlLease() = default;
  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(data_); }
  [[nodiscard]] const CapDomainKey &origin() const;
  [[nodiscard]] const SourceCache &source() const;
  [[nodiscard]] const ExactTable &values() const;
  [[nodiscard]] const std::array<ExactRef, 4> &old_cut() const;
  [[nodiscard]] std::span<const PhysicalCapEndpoint, 2> endpoints() const;
  [[nodiscard]] uint32_t angular_level() const;
  [[nodiscard]] uint64_t revision() const;
  [[nodiscard]] uint32_t pending_radial_sides() const;
  [[nodiscard]] const std::optional<std::array<ExactRef, 4>> &fixed_metric_factor() const;
};
class ProducedCapDomainLease final {
  std::shared_ptr<const ProducedCapDomainData> data_;
  explicit ProducedCapDomainLease(std::shared_ptr<const ProducedCapDomainData> data)
      : data_(std::move(data)) {}
  friend class CapDomainProducer;

public:
  ProducedCapDomainLease() = default;
  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(data_); }
  [[nodiscard]] const CapDomainKey &key() const;
  [[nodiscard]] const SourceCache &source() const;
  [[nodiscard]] const SharedCurveSnapshot &curves() const;
  [[nodiscard]] const ExactTable &values() const;
  [[nodiscard]] std::shared_ptr<RunResource> resource() const;
  [[nodiscard]] std::span<const SeamWedge> wedges() const;
  [[nodiscard]] std::span<const CapDomainRow> caps() const;
  [[nodiscard]] std::span<const CapPointPlan> points() const;
  [[nodiscard]] std::span<const CapDomainPiece> pieces() const;
  [[nodiscard]] const CapCutControlLease &control() const;
};
struct CapDomainRequest final {
  SourceCache source;
  SharedCurveSnapshot curves;
  CapDomainKey key;
  ExactTableKey table_key;
  CapCutControlLease continuation;
  uint32_t transcendental_bits = 96, norm_fractional_bits = 96;
};
struct CapDomainPreparation final {
  ProducedCapDomainLease plan;
  CapCutControlLease continuation;
  std::pmr::vector<BoundaryKey> boundary_requests;
  explicit CapDomainPreparation(std::pmr::memory_resource *resource)
      : boundary_requests(resource) {}
};
enum class CapCutDirection : uint32_t { angular, radial, retain };
struct CapContinuationRequest final {
  ProducedCapDomainLease expected;
  CapCutDirection direction;
  ExactTableKey table_key;
  std::optional<ExactTable> metric_values;
  std::optional<std::array<ExactRef, 4>> metric_factor;
};
// No cut Q is constructed here. Nonempty requests prohibit a completed plan.
[[nodiscard]] CapDomainPreparation prepare_cap_domain(const CapDomainRequest &, JobContext &);
[[nodiscard]] CapCutControlLease continue_cap_domain(const CapContinuationRequest &, JobContext &);
} // namespace adaptive
} // namespace bgmesh
#endif
