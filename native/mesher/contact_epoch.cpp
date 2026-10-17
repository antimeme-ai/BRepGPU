#include "contact_epoch.hpp"
#include <algorithm>
#include <bit>
#include <memory>

namespace bgmesh {
namespace {
constexpr uint32_t none = UINT32_MAX;
[[noreturn]] void invalid(const char *s) { throw Error(ErrorCode::invalid_source, s); }
[[noreturn]] void internal(const char *s) { throw Error(ErrorCode::internal_error, s); }
template <class T> void grow(std::pmr::vector<T> &v, std::size_t n) {
  if (n > v.max_size() - v.size())
    throw Error(ErrorCode::resource_exhausted, "contact epoch capacity exhausted");
  const auto need = v.size() + n;
  if (need > v.capacity())
    v.reserve(std::max(need, v.capacity() <= v.max_size() / 2 ? v.capacity() * 2 : v.max_size()));
}
bool same_xyz(Vec3 a, Vec3 b) {
  for (std::size_t i = 0; i < 3; ++i)
    if (std::bit_cast<uint64_t>(a[i]) != std::bit_cast<uint64_t>(b[i]))
      return false;
  return true;
}
void add_stats(ContactPredicateStats &a, const ContactPredicateStats &b) {
  a.filtered_separation += b.filtered_separation;
  a.filtered_shared_edge += b.filtered_shared_edge;
  a.filtered_shared_vertex += b.filtered_shared_vertex;
  a.filtered_projected += b.filtered_projected;
  a.exact_pairs += b.exact_pairs;
}
} // namespace
struct FrozenContactFacet final {
  FacetHandle handle;
  PreparedContactFacet prepared;
};
struct FrozenContactPair final {
  std::array<uint32_t, 2> facets;
  AllowedSimplex allowed;
};
struct ContactPairData final {
  std::shared_ptr<RunResource> resource;
  ContactEpoch epoch;
  std::pmr::vector<FrozenContactFacet> facets;
  std::pmr::vector<FrozenContactPair> pairs;
  ContactPairData(std::shared_ptr<RunResource> r, ContactEpoch e)
      : resource(std::move(r)), epoch(e), facets(resource.get()), pairs(resource.get()) {}
};
struct ContactPairResultData final {
  std::shared_ptr<const ContactPairData> source;
  uint32_t first;
  std::pmr::vector<uint8_t> permitted;
  ContactPredicateStats stats;
  ContactPairResultData(std::shared_ptr<const ContactPairData> data, uint32_t begin)
      : source(std::move(data)), first(begin), permitted(source->resource.get()) {}
};
ContactPairBatch::ContactPairBatch(std::shared_ptr<const ContactPairData> data,
                                   ContactPairRange range)
    : data_(std::move(data)), first_(range.first), count_(range.count) {}
ContactEpoch ContactPairBatch::epoch() const noexcept { return data_->epoch; }
ContactPairCompleted classify_contact_batch(bgfd::Budget &budget, const ContactPairBatch &batch) {
  auto result = std::allocate_shared<ContactPairResultData>(
      RunAllocator<ContactPairResultData>(batch.data_->resource), batch.data_, batch.first_);
  result->permitted.reserve(batch.count_);
  for (uint32_t i = 0; i < batch.count_; ++i) {
    budget.charge(1);
    const auto &pair = batch.data_->pairs[batch.first_ + i];
    const auto contact =
        classify_contact(budget, batch.data_->facets[pair.facets[0]].prepared,
                         batch.data_->facets[pair.facets[1]].prepared, pair.allowed, result->stats);
    result->permitted.push_back(contact.permitted ? 1u : 0u);
  }
  return ContactPairCompleted(result);
}
struct ContactOwnerIdentity final {};
static std::shared_ptr<const ContactOwnerIdentity>
new_contact_identity(const std::shared_ptr<RunResource> &resource) {
  if (!resource)
    invalid("null contact run resource");
  return std::allocate_shared<ContactOwnerIdentity>(RunAllocator<ContactOwnerIdentity>(resource));
}
struct ApprovalStamp final {
  uint64_t scene_revision, proposal_ordinal;
};
struct ContactApprovalData final {
  std::shared_ptr<RunResource> resource;
  std::shared_ptr<const ContactOwnerIdentity> instance;
  ContactEpoch epoch;
  uint64_t revision, proposal_ordinal;
  std::array<FacetHandle, 2> old;
  std::array<CellResultKey, 2> cells;
  std::array<PreparedContactFacet, 2> proposed;
  std::array<std::array<DeclaredContactEdge, 3>, 2> edges;
  std::array<std::array<PhysicalNodeKey, 3>, 2> node_keys;
  ContactApprovalData(std::shared_ptr<RunResource> r,
                      std::shared_ptr<const ContactOwnerIdentity> id, ContactEpoch e,
                      ApprovalStamp stamp, std::array<FacetHandle, 2> retired,
                      std::array<CellResultKey, 2> keys,
                      std::array<PreparedContactFacet, 2> geometry,
                      std::array<std::array<DeclaredContactEdge, 3>, 2> declarations,
                      std::array<std::array<PhysicalNodeKey, 3>, 2> physical_keys)
      : resource(std::move(r)), instance(std::move(id)), epoch(e), revision(stamp.scene_revision),
        proposal_ordinal(stamp.proposal_ordinal), old(retired), cells(keys),
        proposed(std::move(geometry)), edges(declarations), node_keys(physical_keys) {}
};
bool contact_approval_matches(const ContactApproval &approval, const ContactApprovalClaim &claim) {
  if (!approval.data_ || approval.data_->proposal_ordinal != claim.proposal_ordinal ||
      approval.data_->cells != claim.old_cells || approval.data_->edges != claim.edges)
    return false;
  for (std::size_t i = 0; i < 2; ++i)
    for (std::size_t c = 0; c < 3; ++c)
      if (approval.data_->node_keys[i][c] != claim.nodes[i][c].key ||
          !same_xyz(approval.data_->proposed[i].facet().xyz[c], claim.nodes[i][c].xyz))
        return false;
  return true;
}
struct ContactEpochOwner::Impl final {
  enum class Phase { idle, open, classifying, failed };
  struct FaceRow final {
    OwnerKey owner;
    Slot<FaceTag> source;
    FaceFrontierIdentity expected;
    std::optional<FaceFrontierIdentity> previous;
    ContactFaceDelta delta;
    uint32_t head = none;
  };
  struct FacetRow final {
    FacetHandle handle{};
    CellResultKey cell{};
    std::array<DeclaredContactEdge, 3> edges{};
    std::optional<PreparedContactFacet> prepared;
    uint32_t face = none, previous = none, next = none, pair_head = none;
    uint32_t frozen_index = none;
    uint64_t frozen_epoch = 0, new_epoch = 0;
    bool seen = false, live = false;
  };
  struct NodeRow final {
    Vec3 xyz{};
    PhysicalNodeKey key{};
    uint64_t refs = 0;
    bool seen = false;
  };
  struct ForbiddenPair final {
    std::array<uint32_t, 2> facets;
    std::array<uint32_t, 2> previous{none, none}, next{none, none};
    uint32_t position = none;
  };
  bgfd::Budget &budget;
  std::shared_ptr<RunResource> resource;
  std::shared_ptr<const ContactOwnerIdentity> identity;
  uint64_t revision = 0;
  SourceStore source;
  Slot<DefinitionTag> definition;
  ContactEpochLimits limits;
  FacetBVH bvh;
  std::pmr::vector<FaceRow> faces;
  std::pmr::vector<FacetRow> facets;
  std::pmr::vector<NodeRow> nodes;
  std::pmr::vector<ForbiddenPair> forbidden;
  std::pmr::vector<uint32_t> live_pairs, free_pairs;
  std::shared_ptr<const ContactPairData> frozen;
  std::pmr::vector<uint8_t> answers;
  uint32_t dispatched = 0, completed = 0;
  uint64_t arrivals = 0;
  ContactEpoch epoch{0};
  Phase phase = Phase::idle;
  ContactEpochStats counters;
  Impl(bgfd::Budget &b, std::shared_ptr<RunResource> r, SourceStore s, Slot<DefinitionTag> d,
       ContactEpochLimits l)
      : budget(b), resource(std::move(r)), identity(new_contact_identity(resource)),
        source(std::move(s)), definition(d), limits(l), bvh(b), faces(&b), facets(&b), nodes(&b),
        forbidden(&b), live_pairs(&b), free_pairs(&b), answers(&b) {
    if (!resource || d.value >= source.view().definitions.size() || limits.pair_capacity == 0)
      invalid("invalid contact definition/resource/limits");
    for (const auto slot : source.view().definition_faces.at(d.value)) {
      budget.charge(1);
      const OwnerKey owner{OwnerKind::face, d.value, source.view().faces[slot].id.value};
      faces.push_back({owner, {slot}, {owner, 0, 0}, {}, {}, none});
    }
    std::sort(faces.begin(), faces.end(), [&](const auto &a, const auto &c) {
      budget.charge(1);
      return a.owner < c.owner;
    });
  }
  void check() const {
    if (phase == Phase::failed)
      internal("contact owner failed");
  }
  void current(ContactEpoch e) const {
    check();
    if (e != epoch)
      invalid("stale contact epoch");
  }
  uint32_t face_index(OwnerKey owner) const {
    const auto it =
        std::lower_bound(faces.begin(), faces.end(), owner, [&](const auto &a, auto key) {
          budget.charge(1);
          return a.owner < key;
        });
    if (it == faces.end() || it->owner != owner)
      invalid("foreign face in contact horizon");
    return static_cast<uint32_t>(it - faces.begin());
  }
  template <class T> void ensure(std::pmr::vector<T> &v, uint32_t slot) {
    if (slot == none)
      invalid("invalid contact slot");
    if (slot >= v.size()) {
      budget.charge(static_cast<uint64_t>(slot) + 1 - v.size());
      grow(v, static_cast<std::size_t>(slot) + 1 - v.size());
      v.resize(static_cast<std::size_t>(slot) + 1);
    }
  }
  FacetRow &get(FacetHandle handle) {
    if (handle.slot.value >= facets.size())
      invalid("unknown contact facet");
    auto &row = facets[handle.slot.value];
    if (!row.live || row.handle != handle)
      invalid("stale contact facet generation");
    return row;
  }
  static const PreparedContactFacet &preparation(const FacetRow &row) {
    if (!row.prepared)
      internal("live contact facet lacks preparation");
    return *row.prepared;
  }
  std::size_t side(const ForbiddenPair &pair, uint32_t facet) const {
    if (pair.facets[0] == facet)
      return 0;
    if (pair.facets[1] == facet)
      return 1;
    internal("contact pair adjacency mismatch");
  }
  void erase_pair(uint32_t at) {
    auto &pair = forbidden[at];
    for (std::size_t i = 0; i < 2; ++i) {
      const auto vertex = pair.facets[i], before = pair.previous[i], after = pair.next[i];
      if (before == none)
        facets[vertex].pair_head = after;
      else
        forbidden[before].next[side(forbidden[before], vertex)] = after;
      if (after != none)
        forbidden[after].previous[side(forbidden[after], vertex)] = before;
    }
    const auto position = pair.position, last = live_pairs.back();
    live_pairs[position] = last;
    forbidden[last].position = position;
    live_pairs.pop_back();
    free_pairs.push_back(at);
  }
  void add_pair(std::array<uint32_t, 2> pair_facets) {
    grow(live_pairs, 1);
    grow(free_pairs, 1);
    uint32_t at;
    if (free_pairs.empty()) {
      if (forbidden.size() >= none)
        throw Error(ErrorCode::resource_exhausted, "contact pair slot exhausted");
      grow(forbidden, 1);
      at = static_cast<uint32_t>(forbidden.size());
      forbidden.push_back({pair_facets});
    } else {
      at = free_pairs.back();
      free_pairs.pop_back();
      forbidden[at] = ForbiddenPair{pair_facets};
    }
    auto &pair = forbidden[at];
    pair.position = static_cast<uint32_t>(live_pairs.size());
    live_pairs.push_back(at);
    for (std::size_t i = 0; i < 2; ++i) {
      auto &head = facets[pair_facets[i]].pair_head;
      pair.next[i] = head;
      if (head != none)
        forbidden[head].previous[side(forbidden[head], pair_facets[i])] = at;
      head = at;
    }
  }
  void erase_facet(FacetHandle handle) {
    budget.charge(1);
    auto &row = get(handle);
    grow(free_pairs, live_pairs.size());
    while (row.pair_head != none) {
      budget.charge(1);
      erase_pair(row.pair_head);
    }
    bvh.erase(handle);
    const auto geometry = preparation(row).facet();
    for (const auto node : geometry.nodes) {
      budget.charge(1);
      if (nodes[node.value].refs == 0)
        internal("contact node count underflow");
      --nodes[node.value].refs;
    }
    if (row.previous == none)
      faces[row.face].head = row.next;
    else
      facets[row.previous].next = row.next;
    if (row.next != none)
      facets[row.next].previous = row.previous;
    row.live = false;
    row.prepared.reset();
    row.previous = none;
    row.next = none;
  }
  bool same(const FacetRow &row, const ContactFacetInput &input) const {
    if (row.edges != input.edges ||
        preparation(row).facet().definition != input.geometry.definition ||
        preparation(row).facet().nodes != input.geometry.nodes)
      return false;
    for (std::size_t i = 0; i < 3; ++i)
      if (nodes[input.geometry.nodes[i].value].key != input.node_keys[i] ||
          !same_xyz(preparation(row).facet().xyz[i], input.geometry.xyz[i]))
        return false;
    return true;
  }
  void declarations(const ContactFacet &geometry, FaceFrontierIdentity face,
                    const std::array<DeclaredContactEdge, 3> &edges) const {
    if (geometry.definition != definition.value)
      invalid("contact facet definition mismatch");
    for (const auto &edge : edges) {
      if (edge.kind == ContactEdgeKind::internal_face || edge.kind == ContactEdgeKind::face_cut) {
        if (edge.owner != face.owner.id || edge.generation != face.face_generation)
          invalid("face contact declaration owner/generation mismatch");
      } else if (edge.kind == ContactEdgeKind::source_edge) {
        if (edge.generation != 0)
          invalid("persistent source contact edge generation must be zero");
        const auto slot = source.view().edge_index.find(EdgeId{edge.owner});
        const auto admitted = source.view().definition_edges.at(definition.value);
        if (!std::binary_search(admitted.begin(), admitted.end(), slot.value, [&](auto a, auto c) {
              budget.charge(1);
              return source.view().edges[a].id < source.view().edges[c].id;
            }))
          invalid("contact source edge outside definition");
      } else
        invalid("unknown contact edge declaration kind");
    }
  }
  bool insert_facet(const ContactFacetInput &input, uint32_t fi,
                    const PreparedContactFacet *known = nullptr) {
    budget.charge(1);
    if (input.cell.face != faces[fi].expected)
      invalid("contact cell belongs to stale face generation");
    declarations(input.geometry, input.cell.face, input.edges);
    ensure(facets, input.facet.slot.value);
    auto &row = facets[input.facet.slot.value];
    if (row.live) {
      if (row.handle != input.facet || row.face != fi || row.cell.face != input.cell.face ||
          !same(row, input) || row.cell.triangle != input.cell.triangle ||
          input.cell.request_revision < row.cell.request_revision)
        invalid("live facet revision changed without retirement");
      row.cell = input.cell;
      return false;
    }
    if (row.seen && input.facet.generation <= row.handle.generation)
      invalid("retired contact facet handle reused");
    const auto prepared = known ? *known : prepare_contact_facet(budget, input.geometry);
    for (std::size_t i = 0; i < 3; ++i) {
      const auto slot = input.geometry.nodes[i].value;
      ensure(nodes, slot);
      auto &node = nodes[slot];
      if (input.node_keys[i].definition != definition.value)
        invalid("contact physical key belongs to foreign definition");
      if (node.seen &&
          (node.key != input.node_keys[i] || !same_xyz(node.xyz, input.geometry.xyz[i])))
        invalid("shared physical node has different key/XYZ words");
      if (node.refs == UINT64_MAX)
        throw Error(ErrorCode::resource_exhausted, "contact node incidence exhausted");
      node.seen = true;
      node.xyz = input.geometry.xyz[i];
      node.key = input.node_keys[i];
      ++node.refs;
    }
    row.handle = input.facet;
    row.cell = input.cell;
    row.edges = input.edges;
    row.prepared = prepared;
    row.face = fi;
    row.previous = none;
    row.next = faces[fi].head;
    if (row.next != none)
      facets[row.next].previous = input.facet.slot.value;
    faces[fi].head = input.facet.slot.value;
    row.pair_head = none;
    row.seen = true;
    row.live = true;
    row.new_epoch = epoch.value;
    row.frozen_epoch = 0;
    return true;
  }
  AllowedSimplex permission(const FacetRow &a, const FacetRow &b) const {
    const auto &an = preparation(a).facet().nodes, &bn = preparation(b).facet().nodes;
    std::array<NodeSlot, 3> common{};
    std::size_t count = 0;
    for (const auto first : an)
      for (const auto second : bn)
        if (first == second)
          common[count++] = first;
    if (count == 1)
      return {SimplexDimension::vertex, {common[0], common[0]}};
    if (count == 2) {
      auto edge_index = [&](const auto &points) {
        for (std::size_t i = 0; i < 3; ++i) {
          const auto x = points[i], y = points[(i + 1) % 3];
          if ((x == common[0] && y == common[1]) || (x == common[1] && y == common[0]))
            return i;
        }
        internal("two shared nodes lack triangle edge");
      };
      if (a.edges[edge_index(an)] == b.edges[edge_index(bn)])
        return {SimplexDimension::edge, {common[0], common[1]}};
    }
    return {};
  }
  uint32_t freeze(ContactPairData &data, uint32_t slot) {
    auto &row = facets[slot];
    if (row.frozen_epoch == epoch.value)
      return row.frozen_index;
    if (data.facets.size() >= none)
      throw Error(ErrorCode::resource_exhausted, "frozen contact facets exhausted");
    row.frozen_index = static_cast<uint32_t>(data.facets.size());
    row.frozen_epoch = epoch.value;
    data.facets.push_back({row.handle, preparation(row)});
    return row.frozen_index;
  }
};
ContactEpochOwner::ContactEpochOwner(bgfd::Budget &budget, std::shared_ptr<RunResource> resource,
                                     SourceStore source, Slot<DefinitionTag> definition,
                                     ContactEpochLimits limits)
    : budget_(&budget), impl_(nullptr) {
  std::pmr::polymorphic_allocator<Impl> allocator(&budget);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, budget, std::move(resource), std::move(source), definition, limits);
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
ContactEpochOwner::~ContactEpochOwner() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(budget_).deallocate(impl_, 1);
}
ContactEpoch ContactEpochOwner::begin(std::span<const FaceFrontierIdentity> input) {
  impl_->check();
  if (impl_->phase != Impl::Phase::idle || input.size() != impl_->faces.size())
    invalid("contact horizon incomplete or already active");
  if (impl_->epoch.value == UINT64_MAX)
    throw Error(ErrorCode::resource_exhausted, "contact epoch exhausted");
  std::pmr::vector<FaceFrontierIdentity> sorted(input.begin(), input.end(), budget_);
  std::sort(sorted.begin(), sorted.end(), [&](const auto &a, const auto &b) {
    budget_->charge(1);
    return a.owner < b.owner;
  });
  for (std::size_t i = 0; i < sorted.size(); ++i) {
    budget_->charge(1);
    auto &row = impl_->faces[i];
    if (sorted[i].owner != row.owner)
      invalid("contact horizon dropped/repeated source face");
    if (row.previous && (sorted[i].face_generation < row.previous->face_generation ||
                         sorted[i].boundary_generation < row.previous->boundary_generation ||
                         (sorted[i].boundary_generation != row.previous->boundary_generation &&
                          sorted[i].face_generation <= row.previous->face_generation)))
      invalid("contact face generation did not advance coherently");
  }
  for (std::size_t i = 0; i < sorted.size(); ++i) {
    impl_->faces[i].expected = sorted[i];
    impl_->faces[i].delta.reset();
  }
  ++impl_->epoch.value;
  ++impl_->counters.epochs;
  impl_->arrivals = 0;
  impl_->phase = Impl::Phase::open;
  return impl_->epoch;
}
void ContactEpochOwner::arrive(ContactEpoch epoch, ContactFaceDelta delta) {
  impl_->current(epoch);
  if (impl_->phase != Impl::Phase::open || !delta)
    invalid("contact arrival outside open horizon");
  auto &row = impl_->faces[impl_->face_index(delta->face.owner)];
  if (row.delta || row.expected != delta->face)
    invalid("stale/duplicate face contact arrival");
  row.delta = std::move(delta);
  ++impl_->arrivals;
}
bool ContactEpochOwner::horizon_complete(ContactEpoch epoch) const {
  impl_->current(epoch);
  return impl_->phase == Impl::Phase::open && impl_->arrivals == impl_->faces.size();
}
void ContactEpochOwner::prepare(ContactEpoch epoch) {
  impl_->current(epoch);
  if (!horizon_complete(epoch))
    invalid("contact preparation before complete face horizon");
  try {
    if (impl_->revision == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "contact scene revision exhausted");
    ++impl_->revision;
    std::pmr::vector<uint32_t> changed(budget_);
    for (uint32_t fi = 0; fi < impl_->faces.size(); ++fi) {
      auto &face = impl_->faces[fi];
      const auto &delta = *face.delta;
      std::pmr::vector<FacetHandle> removed(delta.removed.begin(), delta.removed.end(), budget_);
      std::sort(removed.begin(), removed.end(), [&](auto a, auto b) {
        budget_->charge(1);
        return a < b;
      });
      for (const auto handle : removed) {
        if (impl_->get(handle).face != fi)
          invalid("contact face retired foreign facet");
        impl_->erase_facet(handle);
      }
      std::pmr::vector<ContactFacetInput> inserted(delta.inserted.begin(), delta.inserted.end(),
                                                   budget_);
      std::sort(inserted.begin(), inserted.end(), [&](const auto &a, const auto &b) {
        budget_->charge(1);
        return a.facet < b.facet;
      });
      for (std::size_t i = 1; i < inserted.size(); ++i) {
        budget_->charge(1);
        if (inserted[i - 1].facet == inserted[i].facet)
          invalid("duplicate facet in contact delta");
      }
      for (const auto &input : inserted)
        if (impl_->insert_facet(input, fi))
          changed.push_back(input.facet.slot.value);
      if (!face.previous || *face.previous != face.expected)
        for (auto slot = face.head; slot != none; slot = impl_->facets[slot].next) {
          budget_->charge(1);
          if (impl_->facets[slot].cell.face != face.expected)
            invalid("rebuilt face retained stale actual facet");
        }
    }
    std::sort(changed.begin(), changed.end(), [&](auto a, auto b) {
      budget_->charge(1);
      return impl_->facets[a].handle < impl_->facets[b].handle;
    });
    if (impl_->bvh.stats().live_facets == 0) {
      std::pmr::vector<FacetBox> boxes(budget_);
      boxes.reserve(changed.size());
      for (const auto slot : changed) {
        budget_->charge(1);
        const auto &row = impl_->facets[slot];
        boxes.push_back(facet_box(row.handle, impl_->preparation(row)));
      }
      impl_->bvh.build(boxes);
    } else
      for (const auto slot : changed) {
        budget_->charge(1);
        const auto &row = impl_->facets[slot];
        impl_->bvh.insert(facet_box(row.handle, impl_->preparation(row)));
      }
    auto frozen = std::allocate_shared<ContactPairData>(
        RunAllocator<ContactPairData>(impl_->resource), impl_->resource, epoch);
    for (const auto slot : changed) {
      budget_->charge(1);
      ++impl_->counters.queried_facets;
      const auto &row = impl_->facets[slot];
      const auto candidates = impl_->bvh.query(facet_box(row.handle, impl_->preparation(row)));
      for (const auto other : candidates) {
        budget_->charge(1);
        if (other == row.handle)
          continue;
        const auto &neighbor = impl_->get(other);
        if (neighbor.new_epoch == epoch.value && other < row.handle)
          continue;
        if (frozen->pairs.size() >= impl_->limits.pair_capacity)
          throw Error(ErrorCode::resource_exhausted, "contact candidate pair capacity exhausted");
        const auto a = row.handle < other ? slot : other.slot.value,
                   b = row.handle < other ? other.slot.value : slot;
        frozen->pairs.push_back({{impl_->freeze(*frozen, a), impl_->freeze(*frozen, b)},
                                 impl_->permission(impl_->facets[a], impl_->facets[b])});
      }
    }
    std::sort(frozen->pairs.begin(), frozen->pairs.end(), [&](const auto &a, const auto &b) {
      budget_->charge(1);
      const auto ak = std::array{frozen->facets[a.facets[0]].handle,
                                 frozen->facets[a.facets[1]].handle},
                 bk = std::array{frozen->facets[b.facets[0]].handle,
                                 frozen->facets[b.facets[1]].handle};
      return ak < bk;
    });
    impl_->answers.assign(frozen->pairs.size(), 0);
    impl_->dispatched = 0;
    impl_->completed = 0;
    impl_->counters.candidate_pairs += frozen->pairs.size();
    impl_->frozen = std::move(frozen);
    impl_->phase = Impl::Phase::classifying;
  } catch (...) {
    impl_->phase = Impl::Phase::failed;
    throw;
  }
}
std::optional<ContactPairBatch> ContactEpochOwner::take_batch(ContactEpoch epoch,
                                                              uint32_t maximum) {
  impl_->current(epoch);
  if (impl_->phase != Impl::Phase::classifying || maximum == 0)
    invalid("invalid contact batch phase/capacity");
  if (impl_->dispatched == impl_->frozen->pairs.size())
    return std::nullopt;
  const auto count =
      std::min(maximum, static_cast<uint32_t>(impl_->frozen->pairs.size()) - impl_->dispatched);
  ContactPairBatch result(impl_->frozen, {impl_->dispatched, count});
  impl_->dispatched += count;
  return result;
}
bool ContactEpochOwner::complete(const ContactPairCompleted &completion) {
  impl_->check();
  if (impl_->phase != Impl::Phase::classifying || !completion.data_ ||
      completion.data_->source != impl_->frozen)
    return false;
  const auto &result = *completion.data_;
  if (result.first > impl_->dispatched ||
      result.permitted.size() > impl_->dispatched - result.first)
    invalid("contact completion outside dispatched range");
  for (std::size_t i = 0; i < result.permitted.size(); ++i) {
    budget_->charge(1);
    if (impl_->answers[result.first + i] != 0 || result.permitted[i] > 1)
      invalid("duplicate/invalid contact result");
  }
  for (std::size_t i = 0; i < result.permitted.size(); ++i)
    impl_->answers[result.first + i] = result.permitted[i] + 1;
  impl_->completed += static_cast<uint32_t>(result.permitted.size());
  add_stats(impl_->counters.predicates, result.stats);
  return true;
}
std::pmr::vector<CellResultKey> ContactEpochOwner::finish(ContactEpoch epoch) {
  impl_->current(epoch);
  if (impl_->phase != Impl::Phase::classifying || impl_->completed != impl_->frozen->pairs.size())
    invalid("contact finish before whole result horizon");
  try {
    for (std::size_t i = 0; i < impl_->answers.size(); ++i)
      if (impl_->answers[i] == 1) {
        budget_->charge(1);
        const auto &pair = impl_->frozen->pairs[i];
        impl_->add_pair({impl_->frozen->facets[pair.facets[0]].handle.slot.value,
                         impl_->frozen->facets[pair.facets[1]].handle.slot.value});
      }
    std::pmr::vector<CellResultKey> marks(budget_);
    if (impl_->live_pairs.size() > marks.max_size() / 2)
      throw Error(ErrorCode::resource_exhausted, "contact mark capacity exhausted");
    marks.reserve(impl_->live_pairs.size() * 2);
    for (const auto index : impl_->live_pairs) {
      budget_->charge(1);
      const auto &pair = impl_->forbidden[index];
      marks.push_back(impl_->facets[pair.facets[0]].cell);
      marks.push_back(impl_->facets[pair.facets[1]].cell);
    }
    std::sort(marks.begin(), marks.end(), [&](const auto &a, const auto &b) {
      budget_->charge(1);
      return a < b;
    });
    marks.erase(std::unique(marks.begin(), marks.end()), marks.end());
    for (auto &face : impl_->faces) {
      budget_->charge(1);
      face.previous = face.expected;
      face.delta.reset();
    }
    impl_->frozen.reset();
    impl_->answers.clear();
    impl_->phase = Impl::Phase::idle;
    return marks;
  } catch (...) {
    impl_->phase = Impl::Phase::failed;
    throw;
  }
}
std::optional<ContactApproval>
ContactEpochOwner::approve_replacement(uint64_t proposal_ordinal, std::array<FacetHandle, 2> old,
                                       const std::array<ContactProposedFacet, 2> &proposed) {
  impl_->check();
  if (impl_->phase != Impl::Phase::idle || !impl_->live_pairs.empty() || impl_->epoch.value == 0)
    invalid("optimizer contacts require settled complete definition");
  if (old[0] == old[1])
    invalid("optimizer replacement repeats old facet");
  const auto &first = impl_->get(old[0]), &second = impl_->get(old[1]);
  if (first.cell.face != second.cell.face)
    invalid("optimizer replacement spans face generations");
  const auto common = impl_->permission(first, second);
  if (common.dimension != SimplexDimension::edge)
    invalid("optimizer old pair lacks declared shared edge");
  auto check_nodes = [&](const ContactProposedFacet &facet) {
    impl_->declarations(facet.geometry, first.cell.face, facet.edges);
    for (std::size_t i = 0; i < 3; ++i) {
      budget_->charge(1);
      const auto node = facet.geometry.nodes[i];
      const auto &a = Impl::preparation(first).facet().nodes,
                 &b = Impl::preparation(second).facet().nodes;
      if (std::find(a.begin(), a.end(), node) == a.end() &&
          std::find(b.begin(), b.end(), node) == b.end())
        invalid("fixed-site proposal introduced a physical node");
      if (node.value >= impl_->nodes.size() || !impl_->nodes[node.value].seen ||
          impl_->nodes[node.value].key != facet.node_keys[i] ||
          !same_xyz(impl_->nodes[node.value].xyz, facet.geometry.xyz[i]))
        invalid("fixed-site proposal moved a canonical physical node");
    }
  };
  check_nodes(proposed[0]);
  check_nodes(proposed[1]);
  std::optional<std::array<PreparedContactFacet, 2>> prepared;
  try {
    prepared.emplace(std::array{prepare_contact_facet(*budget_, proposed[0].geometry),
                                prepare_contact_facet(*budget_, proposed[1].geometry)});
  } catch (const bgfd::Failure &failure) {
    if (failure.status == BGFD_INVALID_INPUT)
      return std::nullopt;
    throw;
  }
  if (!prepared)
    internal("prepared optimizer facets missing");
  std::array<Impl::FacetRow, 2> rows;
  for (std::size_t i = 0; i < 2; ++i) {
    rows[i].prepared = (*prepared)[i];
    rows[i].edges = proposed[i].edges;
  }
  const auto allowed = impl_->permission(rows[0], rows[1]);
  if (!classify_contact(*budget_, (*prepared)[0], (*prepared)[1], allowed,
                        impl_->counters.predicates)
           .permitted)
    return std::nullopt;
  for (std::size_t i = 0; i < 2; ++i) {
    const auto candidates = impl_->bvh.query(facet_box(old[i], (*prepared)[i]));
    for (const auto other : candidates) {
      budget_->charge(1);
      if (other == old[0] || other == old[1])
        continue;
      const auto &live = impl_->get(other);
      if (!classify_contact(*budget_, (*prepared)[i], Impl::preparation(live),
                            impl_->permission(rows[i], live), impl_->counters.predicates)
               .permitted)
        return std::nullopt;
    }
  }
  const auto data = std::allocate_shared<ContactApprovalData>(
      RunAllocator<ContactApprovalData>(impl_->resource), impl_->resource, impl_->identity,
      impl_->epoch, ApprovalStamp{impl_->revision, proposal_ordinal}, old,
      std::array{first.cell, second.cell}, std::move(*prepared),
      std::array{proposed[0].edges, proposed[1].edges},
      std::array{proposed[0].node_keys, proposed[1].node_keys});
  return ContactApproval(data);
}
bool ContactEpochOwner::approval_current(const ContactApproval &approval) const {
  impl_->check();
  if (!approval.data_ || impl_->phase != Impl::Phase::idle ||
      approval.data_->instance != impl_->identity || approval.data_->epoch != impl_->epoch ||
      approval.data_->revision != impl_->revision)
    return false;
  for (std::size_t i = 0; i < 2; ++i) {
    const auto handle = approval.data_->old[i];
    if (handle.slot.value >= impl_->facets.size())
      return false;
    const auto &row = impl_->facets[handle.slot.value];
    if (!row.live || row.handle != handle || row.cell != approval.data_->cells[i])
      return false;
  }
  return true;
}
void ContactEpochOwner::commit_replacement(const ContactApproval &approval,
                                           const std::array<ContactFacetInput, 2> &replacement) {
  if (!approval_current(approval))
    invalid("stale optimizer contact approval");
  const auto &data = *approval.data_;
  for (std::size_t i = 0; i < 2; ++i) {
    const auto &actual = replacement[i];
    const auto &expected = data.proposed[i].facet();
    if (actual.cell.face != data.cells[0].face || actual.edges != data.edges[i] ||
        actual.node_keys != data.node_keys[i] ||
        actual.geometry.definition != expected.definition ||
        actual.geometry.nodes != expected.nodes)
      invalid("optimizer commit differs from approved facet identity");
    for (std::size_t k = 0; k < 3; ++k)
      if (!same_xyz(actual.geometry.xyz[k], expected.xyz[k]))
        invalid("optimizer commit differs from approved actual XYZ");
    if (actual.facet == data.old[0] || actual.facet == data.old[1] ||
        (i == 1 && actual.facet == replacement[0].facet))
      invalid("optimizer failed to advance facet revisions");
  }
  if (impl_->revision == UINT64_MAX)
    throw Error(ErrorCode::resource_exhausted, "contact scene revision exhausted");
  try {
    const auto fi = impl_->face_index(data.cells[0].face.owner);
    for (const auto old : data.old)
      impl_->erase_facet(old);
    for (std::size_t i = 0; i < 2; ++i) {
      if (!impl_->insert_facet(replacement[i], fi, &data.proposed[i]))
        internal("optimizer failed to insert approved facet");
      impl_->bvh.insert(facet_box(replacement[i].facet, data.proposed[i]));
    }
    ++impl_->revision;
  } catch (...) {
    impl_->phase = Impl::Phase::failed;
    throw;
  }
}
ContactEpochStats ContactEpochOwner::stats() const {
  impl_->check();
  auto result = impl_->counters;
  result.bvh = impl_->bvh.stats();
  result.retained_forbidden_pairs = impl_->live_pairs.size();
  return result;
}
} // namespace bgmesh
