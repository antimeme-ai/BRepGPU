#include "face_frontier.hpp"
#include <algorithm>
#include <memory>

namespace bgmesh {
namespace {
constexpr uint32_t absent = UINT32_MAX;
[[noreturn]] void invalid(const char *message) { throw bgfd::Failure(BGFD_INVALID_INPUT, message); }
[[noreturn]] void internal(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
template <class T> void grow(std::pmr::vector<T> &v, std::size_t count) {
  if (count > v.max_size() - v.size())
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "face frontier capacity exhausted");
  const auto n = v.size() + count;
  if (n > v.capacity())
    v.reserve(std::max(n, v.capacity() <= v.max_size() / 2 ? v.capacity() * 2 : v.max_size()));
}
} // namespace
struct FaceCellFrontier::Impl final {
  struct Row final {
    uint64_t generation = 0, lineage = 0;
    uint32_t revision = 0, heap = absent;
    CellNumericalState numerical = CellNumericalState::pending;
    bool active = false, pending_new = false, contact = false;
  };
  struct Entry final {
    TriangleHandle triangle;
    Rational diameter;
    uint64_t lineage;
  };
  bgfd::Budget &budget;
  MutableFaceTopology &mesh;
  FaceFrontierIdentity identity;
  std::pmr::vector<Row> rows;
  std::pmr::vector<Entry> heap;
  std::pmr::vector<CellResultKey> settled, pending;
  std::size_t pending_cursor = 0;
  FaceFrontierStats counters;
  bool usable = true, initialized = false;
  Impl(bgfd::Budget &b, MutableFaceTopology &m, FaceFrontierIdentity key)
      : budget(b), mesh(m), identity(key), rows(&b), heap(&b), settled(&b), pending(&b) {
    if (identity.owner.kind != OwnerKind::face)
      invalid("face frontier requires face owner");
  }
  void check() const {
    if (!usable)
      internal("face frontier generation failed");
  }
  void ready() const {
    check();
    if (!initialized)
      invalid("face frontier not initialized");
  }
  void ensure(uint32_t slot) {
    if (slot == absent)
      invalid("invalid frontier triangle slot");
    if (slot >= rows.size()) {
      const auto added = static_cast<std::size_t>(slot) + 1 - rows.size();
      budget.charge(added);
      grow(rows, added);
      rows.resize(static_cast<std::size_t>(slot) + 1);
    }
  }
  CellNumericalState state(const Row &row) const {
    return row.numerical == CellNumericalState::accepted && row.contact
               ? CellNumericalState::needs_refine
               : row.numerical;
  }
  bool higher(const Entry &a, const Entry &b) {
    budget.charge(1);
    ++counters.comparisons;
    const int comparison = a.diameter.compare(b.diameter);
    return comparison > 0 ||
           (comparison == 0 &&
            (a.lineage < b.lineage || (a.lineage == b.lineage && a.triangle < b.triangle)));
  }
  void exchange(uint32_t a, uint32_t b) {
    std::swap(heap[a], heap[b]);
    rows[heap[a].triangle.slot].heap = a;
    rows[heap[b].triangle.slot].heap = b;
  }
  void up(uint32_t at) {
    while (at != 0) {
      const auto parent = (at - 1) / 2;
      if (!higher(heap[at], heap[parent]))
        break;
      exchange(at, parent);
      at = parent;
    }
  }
  void down(uint32_t at) {
    while (static_cast<uint64_t>(at) * 2 + 1 < heap.size()) {
      auto next = at * 2 + 1;
      if (next + 1 < heap.size() && higher(heap[next + 1], heap[next]))
        ++next;
      if (!higher(heap[next], heap[at]))
        break;
      exchange(at, next);
      at = next;
    }
  }
  void insert(TriangleHandle handle) {
    auto &row = rows[handle.slot];
    if (row.heap != absent)
      internal("cell already queued");
    if (heap.size() >= absent)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "frontier heap slot exhausted");
    grow(heap, 1);
    const auto at = static_cast<uint32_t>(heap.size());
    heap.push_back({handle, mesh.diameter_squared(handle).clone_into(budget), row.lineage});
    row.heap = at;
    up(at);
  }
  void erase(uint32_t at) {
    if (at >= heap.size())
      internal("frontier heap index mismatch");
    const auto removed = heap[at].triangle;
    const auto last = static_cast<uint32_t>(heap.size() - 1);
    if (at != last)
      exchange(at, last);
    heap.pop_back();
    rows[removed.slot].heap = absent;
    if (at < heap.size()) {
      if (at != 0 && higher(heap[at], heap[(at - 1) / 2]))
        up(at);
      else
        down(at);
    }
  }
  CellResultKey make_key(TriangleHandle handle) const {
    return {identity, handle, rows[handle.slot].revision};
  }
  bool current(const CellResultKey &key) const {
    if (key.face != identity || key.triangle.slot >= rows.size())
      return false;
    const auto &row = rows[key.triangle.slot];
    return row.active && !row.pending_new && row.generation == key.triangle.generation &&
           row.revision == key.request_revision && mesh.is_live(key.triangle);
  }
  Row &checked(TriangleHandle handle) {
    if (handle.slot >= rows.size())
      invalid("untracked frontier cell");
    auto &row = rows[handle.slot];
    if (!row.active || row.pending_new || row.generation != handle.generation ||
        !mesh.is_live(handle))
      invalid("stale frontier cell");
    return row;
  }
  void install(TriangleHandle handle) {
    auto &row = rows[handle.slot];
    const auto triangle = mesh.triangle(handle);
    row.generation = handle.generation;
    row.lineage = triangle.lineage;
    row.revision = 0;
    row.heap = absent;
    row.numerical = CellNumericalState::pending;
    row.active = true;
    row.pending_new = false;
    row.contact = false;
    insert(handle);
    grow(pending, 1);
    pending.push_back(make_key(handle));
    ++counters.tracked_cells;
  }
};
FaceCellFrontier::FaceCellFrontier(bgfd::Budget &budget, MutableFaceTopology &mesh,
                                   FaceFrontierIdentity identity)
    : budget_(&budget), impl_(nullptr) {
  std::pmr::polymorphic_allocator<Impl> a(&budget);
  impl_ = a.allocate(1);
  try {
    std::construct_at(impl_, budget, mesh, identity);
  } catch (...) {
    a.deallocate(impl_, 1);
    throw;
  }
}
FaceCellFrontier::~FaceCellFrontier() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(budget_).deallocate(impl_, 1);
}
void FaceCellFrontier::initialize() {
  impl_->check();
  if (impl_->initialized)
    invalid("face frontier initialized twice");
  try {
    for (const auto handle : impl_->mesh.triangles()) {
      impl_->budget.charge(1);
      impl_->ensure(handle.slot);
      impl_->install(handle);
    }
    impl_->initialized = true;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
CellResultKey FaceCellFrontier::key(TriangleHandle handle) const {
  impl_->ready();
  (void)impl_->checked(handle);
  return impl_->make_key(handle);
}
bool FaceCellFrontier::complete(CellResultKey key, CellNumericalState state) {
  impl_->ready();
  if (state == CellNumericalState::pending ||
      (state != CellNumericalState::accepted && state != CellNumericalState::needs_refine &&
       state != CellNumericalState::unresolved))
    invalid("invalid bound completion state");
  if (!impl_->current(key) ||
      impl_->rows[key.triangle.slot].numerical != CellNumericalState::pending) {
    ++impl_->counters.stale_results;
    return false;
  }
  impl_->rows[key.triangle.slot].numerical = state;
  return true;
}
CellResultKey FaceCellFrontier::retry(TriangleHandle handle) {
  impl_->ready();
  auto &row = impl_->checked(handle);
  if (row.revision == UINT32_MAX)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "cell bound revision exhausted");
  try {
    ++row.revision;
    row.numerical = CellNumericalState::pending;
    if (row.heap == absent)
      impl_->insert(handle);
    grow(impl_->pending, 1);
    const auto key = impl_->make_key(handle);
    impl_->pending.push_back(key);
    return key;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
bool FaceCellFrontier::mark_contact_failure(CellResultKey key) {
  impl_->ready();
  if (!impl_->current(key)) {
    ++impl_->counters.stale_results;
    return false;
  }
  auto &row = impl_->rows[key.triangle.slot];
  if (row.numerical != CellNumericalState::accepted)
    invalid("contact epoch used an unaccepted cell");
  try {
    row.contact = true;
    if (row.heap == absent)
      impl_->insert(key.triangle);
    return true;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
std::optional<CellFront> FaceCellFrontier::next() {
  impl_->ready();
  try {
    while (!impl_->heap.empty()) {
      const auto handle = impl_->heap.front().triangle;
      const auto &row = impl_->checked(handle);
      const auto key = impl_->make_key(handle);
      const auto state = impl_->state(row);
      if (state != CellNumericalState::accepted)
        return CellFront{key, state, row.contact};
      grow(impl_->settled, 1);
      impl_->settled.push_back(key);
      impl_->erase(0);
    }
    return std::nullopt;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
std::pmr::vector<CellResultKey> FaceCellFrontier::take_settled() {
  impl_->ready();
  std::pmr::vector<CellResultKey> result(&impl_->budget);
  for (const auto &key : impl_->settled) {
    impl_->budget.charge(1);
    if (impl_->current(key) &&
        impl_->state(impl_->rows[key.triangle.slot]) == CellNumericalState::accepted)
      result.push_back(key);
  }
  impl_->settled.clear();
  return result;
}
std::pmr::vector<CellResultKey> FaceCellFrontier::take_pending(uint32_t maximum) {
  impl_->ready();
  if (maximum == 0)
    invalid("zero pending-work capacity");
  try {
    std::pmr::vector<CellResultKey> result(&impl_->budget);
    while (result.size() < maximum && impl_->pending_cursor < impl_->pending.size()) {
      impl_->budget.charge(1);
      const auto key = impl_->pending[impl_->pending_cursor];
      if (impl_->current(key) &&
          impl_->rows[key.triangle.slot].numerical == CellNumericalState::pending)
        result.push_back(key);
      ++impl_->pending_cursor;
    }
    if (impl_->pending_cursor == impl_->pending.size()) {
      impl_->pending.clear();
      impl_->pending_cursor = 0;
    }
    return result;
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
void FaceCellFrontier::apply_changes(std::span<const TopologyEdit> edits) {
  impl_->ready();
  try {
    std::pmr::vector<TriangleHandle> candidates(&impl_->budget);
    for (const auto &edit : edits) {
      impl_->budget.charge(1);
      if (edit.removed_count > edit.removed.size() || edit.inserted_count > edit.inserted.size())
        internal("invalid topology edit counts");
      for (uint32_t i = 0; i < edit.removed_count; ++i) {
        const auto handle = edit.removed[i];
        if (handle.slot >= impl_->rows.size())
          internal("removed cell was not tracked");
        auto &row = impl_->rows[handle.slot];
        if ((!row.active && !row.pending_new) || row.generation != handle.generation)
          internal("removed frontier generation mismatch");
        if (row.active) {
          if (row.heap != absent)
            impl_->erase(row.heap);
          --impl_->counters.tracked_cells;
        }
        row.active = false;
        row.pending_new = false;
      }
      for (uint32_t i = 0; i < edit.inserted_count; ++i) {
        const auto handle = edit.inserted[i];
        impl_->ensure(handle.slot);
        auto &row = impl_->rows[handle.slot];
        if (row.active || row.pending_new)
          internal("inserted frontier slot was not removed");
        row.generation = handle.generation;
        row.pending_new = true;
        candidates.push_back(handle);
      }
      ++impl_->counters.edit_transactions;
    }
    // Intermediate insertions can already be retired by a later transaction in
    // this SAME delta batch. Fold by slot/generation, then inspect survivors only.
    for (const auto handle : candidates) {
      impl_->budget.charge(1);
      const auto &row = impl_->rows[handle.slot];
      if (row.pending_new && row.generation == handle.generation)
        impl_->install(handle);
    }
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
FaceFrontierStats FaceCellFrontier::stats() const {
  impl_->ready();
  auto stats = impl_->counters;
  stats.heap_cells = impl_->heap.size();
  return stats;
}
} // namespace bgmesh
