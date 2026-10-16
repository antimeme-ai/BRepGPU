#include "topology.hpp"
#include <algorithm>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#if defined(__x86_64__)
#include <immintrin.h>
#endif

namespace bgmesh {
namespace {
constexpr uint32_t none = UINT32_MAX;
[[noreturn]] void invalid(const char *message) { throw bgfd::Failure(BGFD_INVALID_INPUT, message); }
[[noreturn]] void internal(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
[[nodiscard]] uint32_t narrow(std::size_t value) {
  if (value >= none)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "topology slot count exhausted");
  return static_cast<uint32_t>(value);
}
[[nodiscard]] uint64_t edge_key(SiteSlot a, SiteSlot b) {
  if (a == b)
    invalid("collapsed topology edge");
  return (uint64_t{std::min(a.value, b.value)} << 32) | std::max(a.value, b.value);
}
[[nodiscard]] uint64_t hash_word(uint64_t value) noexcept {
  value ^= value >> 30;
  value *= UINT64_C(0xbf58476d1ce4e5b9);
  value ^= value >> 27;
  value *= UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31);
}
template <class T> void reserve_growth(std::pmr::vector<T> &values, std::size_t additional) {
  if (additional > values.max_size() - values.size())
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "topology vector capacity overflow");
  const auto required = values.size() + additional;
  if (required > values.capacity()) {
    const auto old = values.capacity();
    const auto growth = old <= values.max_size() / 2 ? old * 2 : values.max_size();
    values.reserve(std::max(required, growth));
  }
}
struct Enclosure final {
  double lo, hi;
};
[[nodiscard]] Enclosure subtract(Enclosure a, Enclosure b) {
  return {std::nextafter(a.lo - b.hi, -INFINITY), std::nextafter(a.hi - b.lo, INFINITY)};
}
[[nodiscard]] Enclosure multiply(Enclosure a, Enclosure b) {
  const std::array<double, 4> values{a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
  for (const auto value : values)
    if (!std::isfinite(value))
      return {-INFINITY, INFINITY};
  const auto bounds = std::minmax_element(values.begin(), values.end());
  return {std::nextafter(*bounds.first, -INFINITY), std::nextafter(*bounds.second, INFINITY)};
}
[[nodiscard]] bool filter_environment() noexcept {
#if defined(__x86_64__) && FLT_EVAL_METHOD == 0
  return std::fegetround() == FE_TONEAREST && (_mm_getcsr() & uint32_t{0xE040}) == 0;
#else
  return false;
#endif
}
} // namespace

struct MutableFaceTopology::Impl final {
  struct Point final {
    Rational x, y, tx, ty;
    Enclosure ix, iy;
    uint32_t hint = none;
    Point(Rational a, Rational b, Rational ta, Rational tb)
        : x(std::move(a)), y(std::move(b)), tx(std::move(ta)), ty(std::move(tb)) {
      const auto bx = x.bounds(), by = y.bounds();
      ix = {bx.lower, bx.upper};
      iy = {by.lower, by.upper};
    }
  };
  struct Triangle final {
    std::array<SiteSlot, 3> sites{};
    std::array<uint32_t, 3> edges{none, none, none}, neighbors{none, none, none};
    uint64_t generation = 0, lineage = 0, walk_stamp = 0;
    bool live = false, accepted = false;
  };
  struct Incident final {
    uint32_t triangle = none, side = none;
  };
  struct EdgeRow final {
    std::array<SiteSlot, 2> sites;
    std::array<Incident, 2> incidents{};
    Rational length2;
    uint64_t generation = 0;
    uint32_t count = 0;
    bool live = false;
    std::optional<BoundaryKey> constraint;
    EdgeRow(std::array<SiteSlot, 2> endpoints, Rational length)
        : sites(endpoints), length2(std::move(length)) {}
  };
  struct Bucket final {
    uint64_t key = 0;
    uint32_t edge = none;
    uint8_t state = 0;
  };
  struct SiteBucket final {
    uint64_t hash = 0;
    SiteSlot site{none};
  };
  bgfd::Budget &budget;
  std::array<Rational, 4> transform;
  std::pmr::vector<Point> points;
  std::pmr::vector<SiteBucket> site_index;
  std::pmr::vector<Triangle> triangles;
  std::pmr::vector<EdgeRow> edges;
  std::pmr::vector<uint32_t> free_triangles, free_edges;
  std::pmr::vector<TriangleLineage> lineages;
  std::pmr::vector<TopologyEdit> changes;
  std::pmr::vector<Bucket> index;
  uint64_t walk_stamp = 0;
  std::size_t indexed = 0, tombstones = 0;
  bool usable = true, sealed = false;
  TopologyStats counters;

  Impl(bgfd::Budget &owner, const std::array<Rational, 4> &matrix)
      : budget(owner), transform{matrix[0].clone_into(owner), matrix[1].clone_into(owner),
                                 matrix[2].clone_into(owner), matrix[3].clone_into(owner)},
        points(&owner), site_index(&owner), triangles(&owner), edges(&owner),
        free_triangles(&owner), free_edges(&owner), lineages(&owner), changes(&owner),
        index(&owner) {
    if (transform[0].multiply(transform[3]).subtract(transform[1].multiply(transform[2])).sign() <=
        0)
      invalid("topology metric factor must have positive determinant");
    index.resize(16);
    site_index.resize(16);
  }
  void require_usable() const {
    if (!usable)
      internal("mutable topology generation failed");
  }
  void require_sealed() const {
    require_usable();
    if (!sealed)
      invalid("initial topology is not sealed");
  }
  const Point &point(SiteSlot slot) const {
    if (slot.value >= points.size())
      invalid("topology site missing");
    return points[slot.value];
  }
  TriangleHandle handle(uint32_t slot) const { return {slot, triangles[slot].generation}; }
  EdgeHandle edge_handle(uint32_t slot) const { return {slot, edges[slot].generation}; }
  Triangle &checked(TriangleHandle value) {
    require_usable();
    if (value.slot >= triangles.size() || !triangles[value.slot].live ||
        triangles[value.slot].generation != value.generation)
      invalid("stale triangle handle");
    return triangles[value.slot];
  }
  EdgeRow &checked(EdgeHandle value) {
    require_usable();
    if (value.slot >= edges.size() || !edges[value.slot].live ||
        edges[value.slot].generation != value.generation)
      invalid("stale edge handle");
    return edges[value.slot];
  }
  int orientation(std::array<SiteSlot, 3> sites) {
    budget.charge(1);
    ++counters.orientations;
    const auto &a = point(sites[0]), &b = point(sites[1]), &c = point(sites[2]);
    if (filter_environment()) {
      const auto value = subtract(multiply(subtract(b.ix, a.ix), subtract(c.iy, a.iy)),
                                  multiply(subtract(b.iy, a.iy), subtract(c.ix, a.ix)));
      if (value.lo > 0 || value.hi < 0) {
        ++counters.filtered_orientations;
        return value.lo > 0 ? 1 : -1;
      }
    }
    ++counters.exact_orientations;
    return b.x.subtract(a.x)
        .multiply(c.y.subtract(a.y))
        .subtract(b.y.subtract(a.y).multiply(c.x.subtract(a.x)))
        .sign();
  }
  Rational length(std::array<SiteSlot, 2> sites) const {
    const auto &p = point(sites[0]), &q = point(sites[1]);
    const auto x = p.tx.subtract(q.tx), y = p.ty.subtract(q.ty);
    return x.multiply(x).add(y.multiply(y));
  }
  uint32_t lookup(uint64_t key) {
    const auto mask = index.size() - 1;
    auto slot = static_cast<std::size_t>(hash_word(key)) & mask;
    for (std::size_t count = 0; count < index.size(); ++count) {
      budget.charge(1);
      ++counters.edge_probes;
      const auto &entry = index[slot];
      if (entry.state == 0)
        return none;
      if (entry.state == 1 && entry.key == key)
        return entry.edge;
      slot = (slot + 1) & mask;
    }
    internal("topology edge index cycle");
  }
  void place(uint64_t key, uint32_t edge) {
    const auto mask = index.size() - 1;
    auto slot = static_cast<std::size_t>(hash_word(key)) & mask;
    std::size_t first_dead = index.size();
    for (std::size_t count = 0; count < index.size(); ++count) {
      budget.charge(1);
      ++counters.edge_probes;
      if (index[slot].state == 2 && first_dead == index.size())
        first_dead = slot;
      if (index[slot].state == 0) {
        if (first_dead != index.size()) {
          slot = first_dead;
          --tombstones;
        }
        index[slot] = {key, edge, 1};
        ++indexed;
        return;
      }
      if (index[slot].state == 1 && index[slot].key == key)
        internal("duplicate topology edge index");
      slot = (slot + 1) & mask;
    }
    internal("topology edge index full");
  }
  void erase_index(uint64_t key) {
    const auto mask = index.size() - 1;
    auto slot = static_cast<std::size_t>(hash_word(key)) & mask;
    for (std::size_t count = 0; count < index.size(); ++count) {
      budget.charge(1);
      ++counters.edge_probes;
      if (index[slot].state == 0)
        internal("topology edge index missing on erase");
      if (index[slot].state == 1 && index[slot].key == key) {
        index[slot].state = 2;
        --indexed;
        ++tombstones;
        return;
      }
      slot = (slot + 1) & mask;
    }
    internal("topology edge erase cycle");
  }
  void prepare(std::size_t additional_triangles, std::size_t additional_edges) {
    if (additional_triangles > none - triangles.size() || additional_edges > none - edges.size())
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "topology transaction exceeds slot range");
    reserve_growth(triangles, additional_triangles);
    reserve_growth(edges, additional_edges);
    reserve_growth(free_triangles, 2);
    reserve_growth(free_edges, 6);
    reserve_growth(lineages, additional_triangles);
    if (indexed + additional_edges >= index.size() / 2 || tombstones > index.size() / 4) {
      std::size_t capacity = index.size();
      while (indexed + additional_edges >= capacity / 2) {
        if (capacity > std::numeric_limits<std::size_t>::max() / 2)
          throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "topology hash capacity overflow");
        capacity *= 2;
      }
      std::pmr::vector<Bucket> fresh(capacity, Bucket{}, &budget);
      const auto mask = capacity - 1;
      for (uint32_t i = 0; i < edges.size(); ++i)
        if (edges[i].live) {
          const auto key = edge_key(edges[i].sites[0], edges[i].sites[1]);
          auto slot = static_cast<std::size_t>(hash_word(key)) & mask;
          while (fresh[slot].state != 0) {
            budget.charge(1);
            ++counters.edge_probes;
            slot = (slot + 1) & mask;
          }
          budget.charge(1);
          ++counters.edge_probes;
          fresh[slot] = {key, i, 1};
        }
      index.swap(fresh);
      tombstones = 0;
    }
  }
  uint32_t create_edge(SiteSlot a, SiteSlot b) {
    const auto key = edge_key(a, b);
    const auto existing = lookup(key);
    if (existing != none)
      return existing;
    EdgeRow row({a.value < b.value ? a : b, a.value < b.value ? b : a}, length({a, b}));
    uint32_t slot;
    if (free_edges.empty()) {
      slot = narrow(edges.size());
      edges.push_back(std::move(row));
    } else {
      slot = free_edges.back();
      free_edges.pop_back();
      row.generation = edges[slot].generation;
      edges[slot] = std::move(row);
    }
    auto &edge = edges[slot];
    if (++edge.generation == 0)
      internal("edge generation overflow");
    edge.live = true;
    place(key, slot);
    ++counters.live_edges;
    counters.edge_peak = std::max(counters.edge_peak, counters.live_edges);
    return slot;
  }
  uint32_t install(std::array<SiteSlot, 3> sites, uint64_t parent, LineageOperation operation,
                   bool spent) {
    budget.charge(1);
    uint32_t slot;
    if (free_triangles.empty()) {
      slot = narrow(triangles.size());
      triangles.emplace_back();
    } else {
      slot = free_triangles.back();
      free_triangles.pop_back();
    }
    auto &triangle = triangles[slot];
    const auto generation = triangle.generation + 1;
    if (generation == 0)
      internal("triangle generation overflow");
    triangle = Triangle{};
    triangle.generation = generation;
    triangle.sites = sites;
    triangle.live = true;
    triangle.lineage = lineages.size();
    lineages.push_back({sites, parent, operation, spent});
    for (uint32_t side = 0; side < 3; ++side) {
      const auto a = sites[side], b = sites[(side + 1) % 3];
      const auto edge_slot = create_edge(a, b);
      auto &edge = edges[edge_slot];
      if (edge.count == 2)
        invalid("third incident triangle");
      if (edge.count == 1) {
        const auto other = edge.incidents[0];
        const auto &other_sites = triangles[other.triangle].sites;
        if (other_sites[other.side] != b || other_sites[(other.side + 1) % 3] != a)
          invalid("triangle edge directions disagree");
        triangle.neighbors[side] = other.triangle;
        triangles[other.triangle].neighbors[other.side] = slot;
      }
      edge.incidents[edge.count++] = {slot, side};
      triangle.edges[side] = edge_slot;
      points[a.value].hint = slot;
    }
    ++counters.live_triangles;
    counters.triangle_peak = std::max(counters.triangle_peak, counters.live_triangles);
    return slot;
  }
  void remove(uint32_t slot) {
    budget.charge(1);
    auto &triangle = triangles[slot];
    for (uint32_t side = 0; side < 3; ++side) {
      auto &edge = edges[triangle.edges[side]];
      uint32_t at = 0;
      while (at < edge.count && edge.incidents[at].triangle != slot)
        ++at;
      if (at == edge.count)
        internal("missing reciprocal edge incident");
      edge.incidents[at] = edge.incidents[edge.count - 1];
      --edge.count;
      if (edge.count == 1) {
        const auto other = edge.incidents[0];
        triangles[other.triangle].neighbors[other.side] = none;
      } else {
        edge.live = false;
        erase_index(edge_key(edge.sites[0], edge.sites[1]));
        free_edges.push_back(triangle.edges[side]);
        --counters.live_edges;
      }
      if (points[triangle.sites[side].value].hint == slot)
        points[triangle.sites[side].value].hint = none;
    }
    triangle.live = false;
    free_triangles.push_back(slot);
    --counters.live_triangles;
  }
  struct PatchTriangle final {
    std::array<SiteSlot, 3> sites;
    uint64_t parent;
    bool spent;
  };
  void replace(std::span<const uint32_t> old, std::span<const PatchTriangle> children,
               LineageOperation operation, SiteSlot new_site) {
    struct SavedConstraint final {
      uint64_t key;
      BoundaryKey value;
    };
    std::array<SavedConstraint, 6> saved{};
    std::size_t saved_count = 0;
    for (const auto &child : children)
      if (orientation(child.sites) <= 0)
        invalid("nonpositive replacement triangle");
    for (const auto slot : old)
      for (const auto edge_slot : triangles[slot].edges)
        if (const auto constraint = edges[edge_slot].constraint) {
          if (saved_count >= saved.size())
            internal("constraint patch exceeds bound");
          const auto &row = edges[edge_slot];
          saved[saved_count++] = {edge_key(row.sites[0], row.sites[1]), *constraint};
        }
    prepare(children.size(), children.size() * 3);
    reserve_growth(changes, 1);
    TopologyEdit change;
    change.new_site = new_site;
    for (const auto slot : old)
      change.removed[change.removed_count++] = handle(slot);
    try {
      for (const auto slot : old)
        remove(slot);
      for (const auto &child : children) {
        const auto inserted = install(child.sites, child.parent, operation, child.spent);
        change.inserted[change.inserted_count++] = handle(inserted);
      }
      for (std::size_t i = 0; i < saved_count; ++i) {
        const auto edge_slot = lookup(saved[i].key);
        if (edge_slot == none)
          internal("protected edge removed by local transaction");
        edges[edge_slot].constraint = saved[i].value;
      }
      changes.push_back(change);
    } catch (...) {
      usable = false;
      throw;
    }
  }
  uint64_t point_hash(const std::array<Rational, 2> &value) {
    uint64_t hash = 0;
    for (const auto &coordinate : value) {
      hash = hash_word(hash ^ static_cast<uint64_t>(coordinate.sign() + 1));
      for (const auto limb : coordinate.numerator().magnitude()) {
        budget.charge(1);
        hash = hash_word(hash ^ limb);
      }
      hash = hash_word(hash ^ UINT64_C(0x82a2b175229d6a5b));
      for (const auto limb : coordinate.denominator().magnitude()) {
        budget.charge(1);
        hash = hash_word(hash ^ limb);
      }
    }
    return hash;
  }
  bool contains(const std::array<Rational, 2> &value) {
    const auto hash = point_hash(value), mask = site_index.size() - 1;
    auto bucket = static_cast<std::size_t>(hash) & mask;
    for (std::size_t probes = 0; probes < site_index.size(); ++probes) {
      budget.charge(1);
      const auto &entry = site_index[bucket];
      if (entry.site.value == none)
        return false;
      const auto &existing = points[entry.site.value];
      if (entry.hash == hash && existing.x.compare(value[0]) == 0 &&
          existing.y.compare(value[1]) == 0)
        return true;
      bucket = (bucket + 1) & mask;
    }
    internal("site hash lookup cycle");
  }
  SiteSlot append(const std::array<Rational, 2> &value) {
    require_usable();
    for (const auto &coordinate : value) {
      if (&coordinate.budget() != &budget)
        invalid("topology site uses another owner");
      const auto words = coordinate.denominator().magnitude();
      uint32_t nonzero = 0;
      for (const auto word : words) {
        budget.charge(1);
        if (word != 0) {
          if ((word & (word - 1)) != 0)
            invalid("topology site must be dyadic");
          ++nonzero;
        }
      }
      if (nonzero != 1)
        invalid("topology site must be dyadic");
    }
    if (points.size() + 1 >= site_index.size() / 2) {
      if (site_index.size() > site_index.max_size() / 2)
        throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "site hash capacity exhausted");
      std::pmr::vector<SiteBucket> fresh(site_index.size() * 2, SiteBucket{}, &budget);
      const auto mask = fresh.size() - 1;
      for (const auto &entry : site_index)
        if (entry.site.value != none) {
          auto at = static_cast<std::size_t>(entry.hash) & mask;
          while (fresh[at].site.value != none) {
            budget.charge(1);
            at = (at + 1) & mask;
          }
          budget.charge(1);
          fresh[at] = entry;
        }
      site_index.swap(fresh);
    }
    const auto hash = point_hash(value);
    const auto mask = site_index.size() - 1;
    auto bucket = static_cast<std::size_t>(hash) & mask;
    while (site_index[bucket].site.value != none) {
      budget.charge(1);
      const auto &entry = site_index[bucket];
      const auto &existing = points[entry.site.value];
      if (entry.hash == hash && existing.x.compare(value[0]) == 0 &&
          existing.y.compare(value[1]) == 0)
        invalid("duplicate exact topology site");
      bucket = (bucket + 1) & mask;
    }
    budget.charge(1);
    Point point(value[0], value[1],
                transform[0].multiply(value[0]).add(transform[1].multiply(value[1])),
                transform[2].multiply(value[0]).add(transform[3].multiply(value[1])));
    const SiteSlot slot{narrow(points.size())};
    points.push_back(std::move(point));
    site_index[bucket] = {hash, slot};
    ++counters.sites;
    return slot;
  }
  uint32_t longest(uint32_t slot) {
    const auto &row = triangles[slot];
    auto best = row.edges[0];
    for (std::size_t side = 1; side < 3; ++side) {
      const auto candidate = row.edges[side];
      const auto &a = edges[candidate], &b = edges[best];
      const int comparison = a.length2.compare(b.length2);
      if (comparison > 0 ||
          (comparison == 0 && edge_key(a.sites[0], a.sites[1]) < edge_key(b.sites[0], b.sites[1])))
        best = candidate;
    }
    return best;
  }
  SiteSlot split_edge(uint32_t edge_slot, std::optional<SiteSlot> selected, bool heuristic) {
    const auto edge = edges[edge_slot];
    if (edge.count != 2 || edge.constraint)
      invalid("local split needs an unprotected internal edge");
    const auto first = edge.incidents[0], second = edge.incidents[1];
    const auto left = triangles[first.triangle], right = triangles[second.triangle];
    const auto a = left.sites[first.side], b = left.sites[(first.side + 1) % 3];
    const auto c = left.sites[(first.side + 2) % 3], d = right.sites[(second.side + 2) % 3];
    SiteSlot p;
    if (selected)
      p = *selected;
    else {
      const Rational two(budget, 2);
      p = append({point(a).x.add(point(b).x).divide(two), point(a).y.add(point(b).y).divide(two)});
    }
    const bool left_spent = heuristic || lineages[left.lineage].heuristic_spent;
    const bool right_spent = heuristic || lineages[right.lineage].heuristic_spent;
    const std::array<uint32_t, 2> old{first.triangle, second.triangle};
    const std::array<PatchTriangle, 4> children{{{{a, p, c}, left.lineage, left_spent},
                                                 {{p, b, c}, left.lineage, left_spent},
                                                 {{b, p, d}, right.lineage, right_spent},
                                                 {{p, a, d}, right.lineage, right_spent}}};
    try {
      replace(old, children, heuristic ? LineageOperation::heuristic : LineageOperation::bisection,
              p);
    } catch (...) {
      usable = false;
      throw;
    }
    ++counters.edge_splits;
    return p;
  }
};

MutableFaceTopology::MutableFaceTopology(bgfd::Budget &budget, std::array<double, 4> transform)
    : MutableFaceTopology(budget,
                          std::array<Rational, 4>{Rational::from_double(budget, transform[0]),
                                                  Rational::from_double(budget, transform[1]),
                                                  Rational::from_double(budget, transform[2]),
                                                  Rational::from_double(budget, transform[3])}) {}
MutableFaceTopology::MutableFaceTopology(bgfd::Budget &budget,
                                         const std::array<Rational, 4> &transform)
    : budget_(&budget), impl_(nullptr) {
  std::pmr::polymorphic_allocator<Impl> allocator(&budget);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, budget, transform);
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
MutableFaceTopology::~MutableFaceTopology() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(budget_).deallocate(impl_, 1);
}
bool MutableFaceTopology::is_usable() const noexcept { return impl_->usable; }
SiteSlot MutableFaceTopology::append_site(std::array<double, 2> value) {
  if (impl_->sealed)
    invalid("sites are published only by edits after initial sealing");
  return impl_->append(
      {Rational::from_double(*budget_, value[0]), Rational::from_double(*budget_, value[1])});
}
SiteSlot MutableFaceTopology::append_site(const std::array<Rational, 2> &value) {
  if (impl_->sealed)
    invalid("sites are published only by edits after initial sealing");
  return impl_->append(value);
}
TriangleHandle MutableFaceTopology::add_initial_triangle(std::array<SiteSlot, 3> sites) {
  impl_->require_usable();
  if (impl_->sealed)
    invalid("initial topology already sealed");
  if (impl_->orientation(sites) <= 0)
    invalid("initial triangle must be positive");
  impl_->prepare(1, 3);
  try {
    return impl_->handle(impl_->install(sites, UINT64_MAX, LineageOperation::initial, false));
  } catch (...) {
    impl_->usable = false;
    throw;
  }
}
void MutableFaceTopology::seal_initial() {
  impl_->require_usable();
  if (impl_->sealed || impl_->counters.live_triangles == 0)
    invalid("cannot seal initial topology");
  for (const auto &point : impl_->points) {
    budget_->charge(1);
    if (point.hint == none)
      invalid("unused initial topology site");
  }
  impl_->sealed = true;
}
bool MutableFaceTopology::is_live(TriangleHandle handle) const {
  impl_->require_usable();
  return handle.slot < impl_->triangles.size() && impl_->triangles[handle.slot].live &&
         impl_->triangles[handle.slot].generation == handle.generation;
}
TriangleView MutableFaceTopology::triangle(TriangleHandle handle) const {
  const auto &row = impl_->checked(handle);
  TriangleView result{
      row.sites, {}, {}, row.lineage, impl_->lineages[row.lineage].heuristic_spent, row.accepted};
  for (uint32_t side = 0; side < 3; ++side) {
    result.edges[side] = impl_->edge_handle(row.edges[side]);
    if (row.neighbors[side] != none)
      result.neighbors[side] = impl_->handle(row.neighbors[side]);
  }
  return result;
}
EdgeView MutableFaceTopology::edge(EdgeHandle handle) const {
  const auto &row = impl_->checked(handle);
  EdgeView result{row.sites, {}, row.count, row.constraint};
  for (uint32_t i = 0; i < row.count; ++i)
    result.incidents[i] = impl_->handle(row.incidents[i].triangle);
  return result;
}
std::optional<EdgeHandle> MutableFaceTopology::find_edge(SiteSlot a, SiteSlot b) const {
  impl_->require_usable();
  (void)impl_->point(a);
  (void)impl_->point(b);
  const auto slot = impl_->lookup(edge_key(a, b));
  return slot == none ? std::nullopt : std::optional(impl_->edge_handle(slot));
}
std::pmr::vector<TriangleHandle> MutableFaceTopology::triangles() const {
  impl_->require_usable();
  std::pmr::vector<TriangleHandle> result(budget_);
  result.reserve(static_cast<std::size_t>(impl_->counters.live_triangles));
  for (uint32_t i = 0; i < impl_->triangles.size(); ++i) {
    budget_->charge(1);
    if (impl_->triangles[i].live)
      result.push_back(impl_->handle(i));
  }
  return result;
}
const Rational &MutableFaceTopology::coordinate(SiteSlot slot, CoordinateAxis axis) const {
  impl_->require_usable();
  const auto &point = impl_->point(slot);
  return axis == CoordinateAxis::x ? point.x : point.y;
}
const Rational &MutableFaceTopology::metric_coordinate(SiteSlot slot, CoordinateAxis axis) const {
  impl_->require_usable();
  const auto &point = impl_->point(slot);
  return axis == CoordinateAxis::x ? point.tx : point.ty;
}
int MutableFaceTopology::compare_axis(SiteSlot a, SiteSlot b, CoordinateAxis axis) const {
  return coordinate(a, axis).compare(coordinate(b, axis));
}
int MutableFaceTopology::orient(std::array<SiteSlot, 3> sites) const {
  impl_->require_usable();
  return impl_->orientation(sites);
}
const TriangleLineage &MutableFaceTopology::lineage(uint64_t index) const {
  impl_->require_usable();
  if (index >= impl_->lineages.size())
    invalid("triangle lineage missing");
  return impl_->lineages[static_cast<std::size_t>(index)];
}
TopologyStats MutableFaceTopology::stats() const {
  impl_->require_usable();
  return impl_->counters;
}
std::pmr::vector<TopologyEdit> MutableFaceTopology::take_changes() {
  impl_->require_usable();
  std::pmr::vector<TopologyEdit> result(budget_);
  result.swap(impl_->changes);
  return result;
}
void MutableFaceTopology::protect_edge(EdgeHandle handle, BoundaryKey key) {
  impl_->checked(handle).constraint = key;
}
void MutableFaceTopology::mark_accepted(TriangleHandle handle) {
  impl_->checked(handle).accepted = true;
}
std::optional<SiteSlot> MutableFaceTopology::insert_heuristic(TriangleHandle handle,
                                                              std::array<double, 2> value) {
  if(!std::isfinite(value[0])||!std::isfinite(value[1])){
    impl_->require_sealed();
    const auto&target=impl_->checked(handle);
    impl_->lineages[target.lineage].heuristic_spent=true;
    return std::nullopt;
  }
  return insert_heuristic(handle,
                          std::array<Rational, 2>{Rational::from_double(*budget_, value[0]),
                                                  Rational::from_double(*budget_, value[1])});
}
std::optional<SiteSlot>
MutableFaceTopology::insert_heuristic(TriangleHandle handle, const std::array<Rational, 2> &value) {
  impl_->require_sealed();
  auto &target = impl_->checked(handle);
  auto &parent = impl_->lineages[target.lineage];
  if (parent.heuristic_spent)
    return std::nullopt;
  parent.heuristic_spent = true;
  if (impl_->contains(value))
    return std::nullopt;
  std::array<int, 3> turns{};
  uint32_t zero_count = 0, zero_side = 0;
  for (uint32_t side = 0; side < 3; ++side) {
    const auto &a = impl_->point(target.sites[side]);
    const auto &b = impl_->point(target.sites[(side + 1) % 3]);
    turns[side] = b.x.subtract(a.x)
                      .multiply(value[1].subtract(a.y))
                      .subtract(b.y.subtract(a.y).multiply(value[0].subtract(a.x)))
                      .sign();
    if (turns[side] < 0)
      return std::nullopt;
    if (turns[side] == 0) {
      ++zero_count;
      zero_side = side;
    }
  }
  if (zero_count > 1)
    return std::nullopt;
  if (zero_count == 1) {
    const auto &edge = impl_->edges[target.edges[zero_side]];
    if (edge.count != 2 || edge.constraint)
      return std::nullopt;
    for (uint32_t i = 0; i < edge.count; ++i) {
      const auto other = edge.incidents[i].triangle;
      if (other != handle.slot && impl_->lineages[impl_->triangles[other].lineage].heuristic_spent)
        return std::nullopt;
    }
    const auto other = edge.incidents[0].triangle == handle.slot ? edge.incidents[1].triangle
                                                                 : edge.incidents[0].triangle;
    impl_->lineages[impl_->triangles[other].lineage].heuristic_spent = true;
  }
  const auto site = impl_->append(value);
  try {
    if (zero_count == 1) {
      (void)impl_->split_edge(target.edges[zero_side], site, true);
    } else {
      const auto sites = target.sites;
      const auto lineage = target.lineage;
      const std::array<uint32_t, 1> old{handle.slot};
      const std::array<Impl::PatchTriangle, 3> children{
          {{{sites[0], sites[1], site}, lineage, true},
           {{sites[1], sites[2], site}, lineage, true},
           {{sites[2], sites[0], site}, lineage, true}}};
      impl_->replace(old, children, LineageOperation::heuristic, site);
      ++impl_->counters.interior_splits;
    }
  } catch (...) {
    impl_->usable = false;
    throw;
  }
  return site;
}
SiteSlot MutableFaceTopology::bisect_internal_edge(EdgeHandle handle) {
  impl_->require_sealed();
  (void)impl_->checked(handle);
  return impl_->split_edge(handle.slot, std::nullopt, false);
}
RefineResult MutableFaceTopology::refine_longest(TriangleHandle target) {
  impl_->require_sealed();
  (void)impl_->checked(target);
  uint64_t splits = 0;
  while (is_live(target)) {
    if (++impl_->walk_stamp == 0)
      internal("topology walk stamp exhausted");
    uint32_t current = target.slot;
    uint64_t steps = 0;
    for (;;) {
      budget_->charge(1);
      ++impl_->counters.walk_steps;
      if (++steps > impl_->counters.live_triangles ||
          impl_->triangles[current].walk_stamp == impl_->walk_stamp)
        internal("nonmonotone longest-edge propagation cycle");
      impl_->triangles[current].walk_stamp = impl_->walk_stamp;
      const auto longest = impl_->longest(current);
      const auto &edge = impl_->edges[longest];
      if (edge.count == 1 || edge.constraint)
        return {RefineKind::boundary_request, impl_->edge_handle(longest), splits};
      if (edge.count != 2)
        internal("invalid longest edge incidence");
      const auto next = edge.incidents[0].triangle == current ? edge.incidents[1].triangle
                                                              : edge.incidents[0].triangle;
      const auto next_longest = impl_->longest(next);
      const int comparison = edge.length2.compare(impl_->edges[next_longest].length2);
      if (comparison == 0) {
        (void)impl_->split_edge(longest, std::nullopt, false);
        ++splits;
        break;
      }
      if (comparison > 0)
        internal("longest edge length decreased during propagation");
      current = next;
    }
  }
  return {RefineKind::target_bisected, std::nullopt, splits};
}
} // namespace bgmesh
