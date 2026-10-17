#ifndef BREPGPU_MESHER_ADAPTIVE_SURFACE_VIEW_HPP
#define BREPGPU_MESHER_ADAPTIVE_SURFACE_VIEW_HPP
#include "adaptive_types.hpp"

namespace bgmesh::adaptive {
struct FaceDomainData;
struct SurfaceFaceIdentity final {
  Slot<DefinitionTag> definition;
  Slot<FaceTag> face;
  uint64_t source_generation = 0, boundary_generation = 0;
  uint64_t face_generation = 0, chart_revision = 0;
  uint64_t assignment_epoch = 0, domain_revision = 0;
  auto operator<=>(const SurfaceFaceIdentity &) const = default;
};
struct SurfaceViewKey final {
  SelectionKey selection;
  SurfaceFaceIdentity face;
  auto operator<=>(const SurfaceViewKey &) const = default;
};
enum class SurfaceClass : uint32_t { outside, interior, band, mapped_cap };
struct CandidateBox final {
  LeafKey key; // path range addresses this candidate lease's path_words
  std::array<ExactRef, 4> box;
};
struct CandidateBoxesData final {
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  ExactTable values;
  SurfaceViewKey key;
  uint64_t request = 0;
  std::pmr::vector<CandidateBox> boxes;
  std::pmr::vector<uint64_t> path_words;
  CandidateBoxesData(std::shared_ptr<RunResource> owner, SourceCache roots, ExactTable exact,
                     SurfaceViewKey identity, uint64_t ordinal)
      : resource(std::move(owner)), source(std::move(roots)), values(std::move(exact)),
        key(identity), request(ordinal), boxes(resource.get()), path_words(resource.get()) {
    if (!resource || source.source().view().resource != resource || values.resource() != resource)
      throw Error(ErrorCode::invalid_source, "surface candidate resource mismatch");
  }
};
using CandidateBoxes = std::shared_ptr<const CandidateBoxesData>;
// Raw material coverage from the SAME domain classifier. The public label word
// may later receive an ordinary packet/band footprint, while this immutable
// capability preserves mapped and mixed cap coverage.
struct CapCandidateCoverage final {
  SurfaceClass material = SurfaceClass::outside;
  Range<uint32_t> pieces;
};
struct CapClassificationData final {
  CandidateBoxes candidates;
  std::shared_ptr<const FaceDomainData> domain;
  std::pmr::vector<CapCandidateCoverage> rows;
  std::pmr::vector<uint32_t> pieces;
  explicit CapClassificationData(CandidateBoxes input)
      : candidates(std::move(input)),
        rows(candidates ? candidates->resource.get() : std::pmr::null_memory_resource()),
        pieces(candidates ? candidates->resource.get() : std::pmr::null_memory_resource()) {
    if (!candidates)
      throw Error(ErrorCode::invalid_source, "empty cap classifier horizon");
  }
};
class FaceDomainClassifier;
class CapClassificationLease final {
  std::shared_ptr<const CapClassificationData> data_;
  explicit CapClassificationLease(std::shared_ptr<const CapClassificationData> data)
      : data_(std::move(data)) {}
  friend class FaceDomainClassifier;

public:
  CapClassificationLease() = default;
  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(data_); }
  [[nodiscard]] const CapClassificationData &data() const {
    if (!data_)
      throw Error(ErrorCode::invalid_source, "empty cap classification");
    return *data_;
  }
};
// Candidate index and exact box/path address the retained classifier operation;
// no device pointer or current mutable node slot crosses the host view boundary.
struct SurfaceCellCoverage final {
  CapClassificationLease classification;
  uint32_t candidate = UINT32_MAX;
};
struct CandidateClassification final {
  CandidateBoxes candidates;
  std::pmr::vector<SurfaceClass> labels;
  std::shared_ptr<const FaceDomainData> domain;
  CapClassificationLease cap_classification;
  explicit CandidateClassification(CandidateBoxes input)
      : candidates(std::move(input)),
        labels(candidates ? candidates->resource.get() : std::pmr::null_memory_resource()) {
    if (!candidates)
      throw Error(ErrorCode::invalid_source, "empty surface classification horizon");
  }
};
struct SurfaceBoundViewData final {
  std::shared_ptr<const PatchSelection> selection;
  SurfaceViewKey key;
  std::shared_ptr<const FaceDomainData> classified_domain;
  std::pmr::vector<SurfaceClass> classes;
  std::pmr::vector<SurfaceCellCoverage> cap_coverage, mapped_cells;
  std::pmr::vector<RootSlot> prepared_roots; // complete query closure, including outside branches
  explicit SurfaceBoundViewData(std::pmr::memory_resource *owner)
      : classes(owner), cap_coverage(owner), mapped_cells(owner), prepared_roots(owner) {}
};
class SurfaceCacheSession;
// Immutable host capability, not a device allocation lease. Only the producer
// mints it after validating its selected branch/classification partition.
class SurfaceBoundView final {
  std::shared_ptr<const SurfaceBoundViewData> data_;
  explicit SurfaceBoundView(std::shared_ptr<const SurfaceBoundViewData> data)
      : data_(std::move(data)) {}
  friend class SurfaceCacheSession;

public:
  SurfaceBoundView() = default;
  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(data_); }
  [[nodiscard]] std::shared_ptr<const PatchSelection> selection() const {
    if (!data_ || !data_->selection)
      throw Error(ErrorCode::invalid_source, "empty surface bound view");
    return data_->selection;
  }
  [[nodiscard]] SurfaceViewKey key() const {
    (void)selection();
    return data_->key;
  }
  [[nodiscard]] const SourceCache &source() const { return selection()->source; }
  [[nodiscard]] std::shared_ptr<RunResource> resource() const { return selection()->resource; }
  [[nodiscard]] std::shared_ptr<const FaceDomainData> classified_domain() const {
    (void)selection();
    return data_->classified_domain;
  }
  [[nodiscard]] std::span<const RootSlot> prepared_roots() const {
    (void)selection();
    return data_->prepared_roots;
  }
  [[nodiscard]] std::span<const SurfaceCellCoverage> cap_coverage() const {
    (void)selection();
    return data_->cap_coverage;
  }
  [[nodiscard]] std::span<const SurfaceCellCoverage> mapped_cells() const {
    (void)selection();
    return data_->mapped_cells;
  }
  [[nodiscard]] std::span<const SurfaceClass> classes() const {
    (void)selection();
    return data_->classes;
  }
};
} // namespace bgmesh::adaptive
#endif
