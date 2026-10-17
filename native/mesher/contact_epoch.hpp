#ifndef BREPGPU_MESHER_CONTACT_EPOCH_HPP
#define BREPGPU_MESHER_CONTACT_EPOCH_HPP
#include "contact.hpp"
#include "face_frontier.hpp"
#include "source.hpp"

namespace bgmesh {
enum class ContactEdgeKind { internal_face, source_edge, face_cut };
struct DeclaredContactEdge final {
  ContactEdgeKind kind;
  uint64_t owner, generation, ordinal;
  auto operator<=>(const DeclaredContactEdge &) const = default;
};
struct ContactFacetInput final {
  FacetHandle facet;
  CellResultKey cell;
  ContactFacet geometry;
  // Entry i declares the edge nodes[i] -> nodes[(i+1)%3].
  std::array<DeclaredContactEdge, 3> edges;
  std::array<PhysicalNodeKey, 3> node_keys{};
};
struct ContactFaceDeltaData final {
  std::shared_ptr<RunResource> resource;
  FaceFrontierIdentity face;
  std::pmr::vector<FacetHandle> removed;
  std::pmr::vector<ContactFacetInput> inserted;
  ContactFaceDeltaData(std::shared_ptr<RunResource> owner, FaceFrontierIdentity identity)
      : resource(std::move(owner)), face(identity), removed(resource.get()),
        inserted(resource.get()) {}
};
using ContactFaceDelta = std::shared_ptr<const ContactFaceDeltaData>;
struct ContactEpoch final {
  uint64_t value;
  auto operator<=>(const ContactEpoch &) const = default;
};
struct ContactPairData;
struct ContactPairResultData;
struct ContactPairRange final {
  uint32_t first, count;
};
class ContactEpochOwner;
class ContactPairCompleted;
class ContactPairBatch final {
  std::shared_ptr<const ContactPairData> data_;
  uint32_t first_, count_;
  ContactPairBatch(std::shared_ptr<const ContactPairData>, ContactPairRange);
  friend class ContactEpochOwner;
  friend class ContactPairCompleted;
  friend ContactPairCompleted classify_contact_batch(bgfd::Budget &, const ContactPairBatch &);

public:
  [[nodiscard]] ContactEpoch epoch() const noexcept;
  [[nodiscard]] uint32_t size() const noexcept { return count_; }
};
class ContactPairCompleted final {
  std::shared_ptr<const ContactPairResultData> data_;
  explicit ContactPairCompleted(std::shared_ptr<const ContactPairResultData> data)
      : data_(std::move(data)) {}
  friend class ContactEpochOwner;
  friend ContactPairCompleted classify_contact_batch(bgfd::Budget &, const ContactPairBatch &);
};
[[nodiscard]] ContactPairCompleted classify_contact_batch(bgfd::Budget &, const ContactPairBatch &);
struct ContactProposedFacet final {
  ContactFacet geometry;
  std::array<DeclaredContactEdge, 3> edges;
  std::array<PhysicalNodeKey, 3> node_keys{};
};
struct ContactApprovalClaim final {
  uint64_t proposal_ordinal;
  std::array<CellResultKey, 2> old_cells;
  std::array<std::array<PhysicalNode, 3>, 2> nodes;
  std::array<std::array<DeclaredContactEdge, 3>, 2> edges;
};
struct ContactApprovalData;
class ContactApproval final {
  std::shared_ptr<const ContactApprovalData> data_;
  explicit ContactApproval(std::shared_ptr<const ContactApprovalData> data)
      : data_(std::move(data)) {}
  friend class ContactEpochOwner;
  friend bool contact_approval_matches(const ContactApproval &, const ContactApprovalClaim &);
};
[[nodiscard]] bool contact_approval_matches(const ContactApproval &, const ContactApprovalClaim &);
struct ContactEpochLimits final {
  uint32_t pair_capacity;
};
struct ContactEpochStats final {
  uint64_t epochs = 0, queried_facets = 0, candidate_pairs = 0, retained_forbidden_pairs = 0;
  BVHStats bvh;
  ContactPredicateStats predicates;
};
// One definition coordinator. prepare() mutates only after the FULL horizon;
// optimizer commit requires its current reserved approval. Read-only children
// own packed immutable preparations. finish() folds ALL answers in pair order.
class ContactEpochOwner final {
  struct Impl;
  bgfd::Budget *budget_;
  Impl *impl_;

public:
  ContactEpochOwner(bgfd::Budget &, std::shared_ptr<RunResource>, SourceStore, Slot<DefinitionTag>,
                    ContactEpochLimits);
  ~ContactEpochOwner();
  ContactEpochOwner(const ContactEpochOwner &) = delete;
  ContactEpochOwner &operator=(const ContactEpochOwner &) = delete;
  [[nodiscard]] ContactEpoch begin(std::span<const FaceFrontierIdentity>);
  void arrive(ContactEpoch, ContactFaceDelta);
  [[nodiscard]] bool horizon_complete(ContactEpoch) const;
  void prepare(ContactEpoch);
  [[nodiscard]] std::optional<ContactPairBatch> take_batch(ContactEpoch, uint32_t maximum);
  [[nodiscard]] bool complete(const ContactPairCompleted &);
  [[nodiscard]] std::pmr::vector<CellResultKey> finish(ContactEpoch);
  [[nodiscard]] ContactEpochStats stats() const;
  [[nodiscard]] std::optional<ContactApproval>
  approve_replacement(uint64_t proposal_ordinal, std::array<FacetHandle, 2>,
                      const std::array<ContactProposedFacet, 2> &);
  [[nodiscard]] bool approval_current(const ContactApproval &) const;
  // The coordinator reserves the sole global optimizer transaction between
  // approval_current, the face reply, and this scene replacement.
  void commit_replacement(const ContactApproval &, const std::array<ContactFacetInput, 2> &);
};
} // namespace bgmesh
#endif
