#include "boundary.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace bgmesh {
namespace {
constexpr uint64_t absent = UINT64_MAX;
[[noreturn]] void invalid(const char *message) { throw bgfd::Failure(BGFD_INVALID_INPUT, message); }
[[noreturn]] void internal(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
template <class T> void reserve_more(std::pmr::vector<T> &v, std::size_t count) {
  if (count > v.max_size() - v.size())
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "boundary capacity exhausted");
  const auto need = v.size() + count;
  if (need > v.capacity())
    v.reserve(std::max(need, v.capacity() <= v.max_size() / 2 ? v.capacity() * 2 : v.max_size()));
}
uint32_t checked_slot(std::size_t value) {
  if (value >= UINT32_MAX)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "boundary sample slot exhausted");
  return static_cast<uint32_t>(value);
}
} // namespace
BoundaryEvaluationData::BoundaryEvaluationData(std::shared_ptr<RunResource> owner,
                                               BoundaryEpoch identity, SourceStore original,
                                               Slot<DefinitionTag> definition_slot,
                                               ExactTable table,
                                               std::pmr::vector<BoundaryEvaluationTask> rows)
    : resource(std::move(owner)), epoch(identity), source(std::move(original)),
      definition(definition_slot), parameters(std::move(table)), tasks(std::move(rows)) {}

struct DefinitionBoundaryForest::Impl final {
  struct Sample final {
    Rational t;
    PhysicalNodeKey key;
    Vec3 xyz{};
    bool ready = false;
    uint64_t constructed_revision = 0;
  };
  struct Interval final {
    uint32_t edge;
    std::array<BoundarySampleSlot, 2> samples;
    std::array<uint64_t, 2> children{absent, absent};
    bool leaf = true;
  };
  struct EdgeState final {
    Slot<EdgeTag> source;
    std::optional<Slot<CurveTag>> curve;
    std::array<BoundarySampleSlot, 2> endpoints;
    std::pmr::vector<uint64_t> roots;
    uint64_t next_ordinal = 1;
    EdgeState(Slot<EdgeTag> id, std::optional<Slot<CurveTag>> geometry,
              std::pmr::memory_resource *resource)
        : source(id), curve(geometry), roots(resource) {}
  };
  struct FaceState final {
    Slot<FaceTag> source;
    uint64_t generation = 0, last_arrival = absent;
  };
  struct Proposal final {
    BoundaryKey key;
    Rational t;
    BoundarySampleSlot sample;
    uint64_t ordinal;
  };
  enum class Phase { initial, idle, open, prepared, failed };
  bgfd::Budget &budget;
  SourceStore source;
  Slot<DefinitionTag> definition;
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<EdgeState> edges;
  std::pmr::vector<FaceState> faces;
  std::pmr::vector<Sample> samples;
  std::pmr::vector<Interval> leaves;
  std::pmr::vector<BoundaryKey> requests;
  std::pmr::vector<Proposal> proposals;
  std::pmr::vector<BoundaryPointRevision> promotions;
  std::pmr::vector<SourceVertexAlias> vertex_aliases;
  std::shared_ptr<const SourceVertexAliasData> vertex_alias_owner;
  std::optional<BoundaryEvaluationBatch> pending;
  Phase phase = Phase::initial;
  BoundaryEpoch epoch{};
  uint32_t table_ordinal = 0;
  std::optional<OwnerKey> table_owner;
  std::size_t arrivals = 0;
  BoundaryStats counters;
  Impl(bgfd::Budget &b, SourceStore original, Slot<DefinitionTag> d, std::shared_ptr<RunResource> r,
       std::optional<OwnerKey> owner)
      : budget(b), source(std::move(original)), definition(d), resource(std::move(r)), edges(&b),
        faces(&b), samples(&b), leaves(&b), requests(&b), proposals(&b), promotions(&b),
        vertex_aliases(&b) {
    table_owner = owner;
    if (!resource || definition.value >= data().definitions.size())
      invalid("invalid boundary definition");
    std::pmr::vector<uint32_t> edge_order(data().definition_edges.at(d.value).begin(),
                                          data().definition_edges.at(d.value).end(), &budget);
    std::sort(edge_order.begin(), edge_order.end(), [&](auto a, auto c) {
      budget.charge(1);
      return a < c;
    });
    build_vertex_aliases(edge_order);
    if (!vertex_aliases.empty()) {
      budget.charge(vertex_aliases.size());
      vertex_alias_owner = std::allocate_shared<SourceVertexAliasData>(
          RunAllocator<SourceVertexAliasData>(resource), resource,
          std::span<const SourceVertexAlias>(vertex_aliases));
    }
    for (const auto slot : data().definition_faces.at(d.value)) {
      budget.charge(1);
      faces.push_back({Slot<FaceTag>{slot}, 0, absent});
    }
    std::sort(faces.begin(), faces.end(), [&](const auto &a, const auto &c) {
      budget.charge(1);
      return a.source < c.source;
    });
    std::pmr::vector<uint64_t> evaluations(&budget);
    for (const auto slot : edge_order) {
      budget.charge(1);
      const auto &edge = data().edges[slot];
      std::optional<Slot<CurveTag>> curve;
      if (edge.curve_id)
        curve = data().curve_index.find(edge.curve_id);
      const auto local = checked_slot(edges.size());
      edges.emplace_back(Slot<EdgeTag>{slot}, curve, &budget);
      auto &state = edges.back();
      std::pmr::vector<double> points(&budget);
      points.push_back(edge.parameter_range[0]);
      points.push_back(edge.parameter_range[1]);
      const auto lo = std::min(points[0], points[1]), hi = std::max(points[0], points[1]);
      if (!edge.degenerate && lo < hi) {
        if (curve)
          for (const auto t : data().view(data().curves[curve->value].knots)) {
            budget.charge(1);
            if (t > lo && t < hi)
              points.push_back(t);
          }
        for (const auto use_slot : data().edge_coedges.at(slot)) {
          budget.charge(1);
          const auto &use = data().coedges[use_slot];
          const auto &loop = data().loops[data().loop_index.find(use.loop_id).value];
          const auto &face = data().faces[data().face_index.find(loop.face_id).value];
          if (face.definition_id != data().definitions[definition.value].id)
            continue;
          // These are ORIGINAL source-t guide breakpoints only. This does not
          // implement the map or promote rational pcurve preimages to sites.
          if (use.parameter_map_kind != 1)
            for (uint64_t i = 0; i < use.parameter_map.count; ++i) {
              budget.charge(1);
              const auto t =
                  data().parameter_maps[static_cast<std::size_t>(use.parameter_map.first + i)][0];
              if (t > lo && t < hi)
                points.push_back(t);
            }
        }
        std::sort(points.begin(), points.end(), [&](double a, double c) {
          budget.charge(1);
          return a < c;
        });
        points.erase(std::unique(points.begin(), points.end()), points.end());
        if (edge.parameter_range[0] > edge.parameter_range[1])
          std::reverse(points.begin(), points.end());
      }
      // A constant/declared degenerate edge keeps both original endpoint
      // occurrences but creates no positive interval or fabricated midpoint.
      std::pmr::vector<BoundarySampleSlot> chain(&budget);
      for (std::size_t i = 0; i < points.size(); ++i) {
        budget.charge(1);
        const auto sample = BoundarySampleSlot{checked_slot(samples.size())};
        if (i == 0 || i + 1 == points.size()) {
          const auto vertex = edge.vertex_ids[i == 0 ? 0 : 1];
          const auto original_slot = data().vertex_index.find(vertex);
          const auto representative = canonical_vertex(original_slot);
          const auto &original_vertex = data().vertices[representative.value];
          samples.push_back({Rational::from_double(budget, points[i]),
                             {definition.value,
                              representative == original_slot ? alias_generator(original_slot)
                                                              : NodeGenerator::declared_quotient,
                              original_vertex.id.value, 0, 0},
                             original_vertex.xyz,
                             true});
        } else {
          if (!curve)
            invalid("interior boundary sample has no original curve");
          samples.push_back({Rational::from_double(budget, points[i]),
                             {definition.value, NodeGenerator::original_edge, edge.id.value, 0,
                              state.next_ordinal++},
                             {},
                             false});
          evaluations.push_back(sample.value);
        }
        chain.push_back(sample);
      }
      state.endpoints = {chain.front(), chain.back()};
      if (!edge.degenerate && lo < hi)
        for (std::size_t i = 1; i < chain.size(); ++i) {
          budget.charge(1);
          state.roots.push_back(leaves.size());
          leaves.push_back({local, {chain[i - 1], chain[i]}});
        }
    }
    counters.samples = samples.size();
    counters.live_intervals = leaves.size();
    if (evaluations.empty())
      phase = Phase::idle;
    else
      pending = make_batch(evaluations);
  }
  const SourceData &data() const { return source.view(); }
  void build_vertex_aliases(std::span<const uint32_t> edge_order) {
    const auto inventory = data().definition_vertices.at(definition.value);
    std::pmr::vector<uint32_t> vertices(inventory.begin(), inventory.end(), &budget);
    std::sort(vertices.begin(), vertices.end(), [&](auto a, auto b) {
      budget.charge(1);
      return a < b;
    });
    std::pmr::vector<uint32_t> parents(&budget), representatives(&budget);
    std::pmr::vector<uint8_t> ranks(vertices.size(), uint8_t{0}, &budget);
    parents.reserve(vertices.size());
    representatives.reserve(vertices.size());
    for (std::size_t i = 0; i < vertices.size(); ++i) {
      budget.charge(1);
      parents.push_back(checked_slot(i));
      representatives.push_back(vertices[i]);
    }
    auto root = [&](uint32_t index) {
      while (parents[index] != index) {
        budget.charge(1);
        parents[index] = parents[parents[index]];
        index = parents[index];
      }
      return index;
    };
    auto local = [&](VertexId vertex) {
      const auto slot = data().vertex_index.find(vertex).value;
      const auto it = std::lower_bound(vertices.begin(), vertices.end(), slot, [&](auto a, auto b) {
        budget.charge(1);
        return a < b;
      });
      if (it == vertices.end() || *it != slot)
        invalid("declared edge vertex outside definition");
      return checked_slot(static_cast<std::size_t>(it - vertices.begin()));
    };
    for (const auto slot : edge_order) {
      budget.charge(1);
      const auto &edge = data().edges[slot];
      if (!edge.degenerate)
        continue;
      auto a = root(local(edge.vertex_ids[0])), b = root(local(edge.vertex_ids[1]));
      if (a == b)
        continue;
      if (ranks[a] < ranks[b])
        std::swap(a, b);
      parents[b] = a;
      if (ranks[a] == ranks[b])
        ++ranks[a];
      if (data().vertices[representatives[b]].id < data().vertices[representatives[a]].id)
        representatives[a] = representatives[b];
    }
    // Include the representative itself so lookup distinguishes a true
    // multi-vertex component from an unchanged same-ID degenerate edge.
    std::pmr::vector<uint32_t> sizes(vertices.size(), uint32_t{0}, &budget);
    for (std::size_t i = 0; i < vertices.size(); ++i) {
      budget.charge(1);
      ++sizes[root(checked_slot(i))];
    }
    for (std::size_t i = 0; i < vertices.size(); ++i) {
      budget.charge(1);
      const auto component = root(checked_slot(i));
      if (sizes[component] > 1)
        vertex_aliases.push_back({{vertices[i]}, {representatives[component]}});
    }
  }
  auto vertex_alias(Slot<VertexTag> vertex) const {
    return std::lower_bound(vertex_aliases.begin(), vertex_aliases.end(), vertex,
                            [&](const auto &row, auto key) {
                              budget.charge(1);
                              return row.original < key;
                            });
  }
  Slot<VertexTag> canonical_vertex(Slot<VertexTag> vertex) const {
    const auto it = vertex_alias(vertex);
    return it != vertex_aliases.end() && it->original == vertex ? it->representative : vertex;
  }
  NodeGenerator alias_generator(Slot<VertexTag> vertex) const {
    const auto it = vertex_alias(vertex);
    return it != vertex_aliases.end() && it->original == vertex ? NodeGenerator::declared_quotient
                                                                : NodeGenerator::original_vertex;
  }
  void check() const {
    if (phase == Phase::failed)
      internal("boundary generation failed");
  }
  void current(BoundaryEpoch identity) const {
    check();
    if (identity != epoch)
      invalid("stale boundary epoch");
  }
  std::size_t edge_index(Slot<EdgeTag> slot) const {
    const auto it =
        std::lower_bound(edges.begin(), edges.end(), slot, [&](const auto &row, auto key) {
          budget.charge(1);
          return row.source < key;
        });
    if (it == edges.end() || it->source != slot)
      invalid("edge outside boundary definition");
    return static_cast<std::size_t>(it - edges.begin());
  }
  std::size_t face_index(Slot<FaceTag> slot) const {
    const auto it =
        std::lower_bound(faces.begin(), faces.end(), slot, [&](const auto &row, auto key) {
          budget.charge(1);
          return row.source < key;
        });
    if (it == faces.end() || it->source != slot)
      invalid("face outside boundary definition");
    return static_cast<std::size_t>(it - faces.begin());
  }
  const Interval &get_interval(BoundaryKey key) const {
    if (key.kind != BoundaryOwnerKind::source_edge)
      invalid("boundary interval is not an original edge owner");
    if (key.interval >= leaves.size())
      invalid("unknown boundary interval");
    const auto &row = leaves[static_cast<std::size_t>(key.interval)];
    if (data().edges[edges[row.edge].source.value].id.value != key.owner)
      invalid("boundary interval owner mismatch");
    return row;
  }
  BoundaryKey key(uint64_t index) const {
    return {data().edges[edges[leaves[static_cast<std::size_t>(index)].edge].source.value].id.value,
            index};
  }
  ExactTableKey next_table() {
    if (table_ordinal == UINT32_MAX)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "boundary table ordinal exhausted");
    return {table_owner.value_or(OwnerKey{OwnerKind::boundary, definition.value,
                                          data().definitions[definition.value].id.value}),
            epoch.value, table_ordinal++};
  }
  BoundaryEvaluationBatch make_batch(std::span<const uint64_t> existing) {
    ExactBuilder builder(resource, next_table());
    std::pmr::vector<BoundaryEvaluationTask> tasks(resource.get());
    tasks.reserve(existing.empty() ? proposals.size() : existing.size());
    if (!existing.empty())
      for (const auto sample : existing) {
        budget.charge(1);
        const auto &node = samples[static_cast<std::size_t>(sample)];
        const auto original = data().edge_index.find(EdgeId{node.key.source});
        const auto curve = data().curve_index.find(data().edges[original.value].curve_id);
        tasks.push_back({epoch, BoundarySampleSlot{checked_slot(static_cast<std::size_t>(sample))},
                         curve, builder.append(node.t)});
      }
    else
      for (const auto &proposal : proposals) {
        budget.charge(1);
        const auto &row = get_interval(proposal.key);
        const auto curve = edges[row.edge].curve;
        if (!curve)
          invalid("split interval has no original curve");
        tasks.push_back({epoch, proposal.sample, *curve, builder.append(proposal.t)});
      }
    return BoundaryEvaluationBatch(std::allocate_shared<BoundaryEvaluationData>(
        RunAllocator<BoundaryEvaluationData>(resource), resource, epoch, source, definition,
        builder.seal(), std::move(tasks)));
  }
  std::pmr::vector<uint64_t> ordered_leaves(Slot<EdgeTag> source_edge) const {
    const auto &state = edges[edge_index(source_edge)];
    std::pmr::vector<uint64_t> result(&budget), stack(&budget);
    for (auto it = state.roots.rbegin(); it != state.roots.rend(); ++it)
      stack.push_back(*it);
    uint64_t visited = 0;
    while (!stack.empty()) {
      budget.charge(1);
      if (++visited > leaves.size())
        internal("boundary interval traversal cycle");
      const auto index = stack.back();
      stack.pop_back();
      const auto &row = leaves[static_cast<std::size_t>(index)];
      if (row.leaf)
        result.push_back(index);
      else {
        stack.push_back(row.children[1]);
        stack.push_back(row.children[0]);
      }
    }
    return result;
  }
};

DefinitionBoundaryForest::DefinitionBoundaryForest(bgfd::Budget &budget, SourceStore source,
                                                   Slot<DefinitionTag> definition,
                                                   std::shared_ptr<RunResource> resource,
                                                   std::optional<OwnerKey> table_owner)
    : budget_(&budget), impl_(nullptr) {
  std::pmr::polymorphic_allocator<Impl> a(&budget);
  impl_ = a.allocate(1);
  try {
    std::construct_at(impl_, budget, std::move(source), definition, std::move(resource),
                      table_owner);
  } catch (...) {
    a.deallocate(impl_, 1);
    throw;
  }
}
DefinitionBoundaryForest::~DefinitionBoundaryForest() {
  std::destroy_at(impl_);
  std::pmr::polymorphic_allocator<Impl>(budget_).deallocate(impl_, 1);
}
std::optional<BoundaryEvaluationBatch> DefinitionBoundaryForest::pending_evaluations() const {
  impl_->check();
  return impl_->pending;
}
BoundaryEpoch DefinitionBoundaryForest::begin_epoch() {
  impl_->check();
  if (impl_->phase != Impl::Phase::idle)
    invalid("boundary owner not ready for epoch");
  if (impl_->epoch.value == UINT64_MAX)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "boundary epoch exhausted");
  ++impl_->epoch.value;
  ++impl_->counters.epochs;
  impl_->arrivals = 0;
  impl_->requests.clear();
  impl_->promotions.clear();
  impl_->phase = Impl::Phase::open;
  return impl_->epoch;
}
void DefinitionBoundaryForest::arrive(BoundaryEpoch epoch, Slot<FaceTag> face, uint64_t generation,
                                      std::span<const BoundaryKey> requests) {
  impl_->current(epoch);
  if (impl_->phase != Impl::Phase::open)
    invalid("boundary horizon is not open");
  auto &state = impl_->faces[impl_->face_index(face)];
  if (generation != state.generation || state.last_arrival == epoch.value)
    invalid("stale/duplicate boundary face result");
  for (const auto request : requests) {
    impl_->budget.charge(1);
    const auto &row = impl_->get_interval(request);
    if (!row.leaf)
      invalid("boundary request names a retired interval");
    const auto incident = impl_->data().edge_faces.at(impl_->edges[row.edge].source.value);
    if (!std::binary_search(incident.begin(), incident.end(), face.value, [&](auto a, auto b) {
          impl_->budget.charge(1);
          return a < b;
        }))
      invalid("boundary request not incident on requesting face");
  }
  reserve_more(impl_->requests, requests.size());
  for (const auto request : requests) {
    impl_->budget.charge(1);
    impl_->requests.push_back(request);
  }
  state.last_arrival = epoch.value;
  ++impl_->arrivals;
  impl_->counters.requests += requests.size();
}
void DefinitionBoundaryForest::request_owner_splits(BoundaryEpoch epoch,
                                                    std::span<const BoundaryKey> requests) {
  impl_->current(epoch);
  if (impl_->phase != Impl::Phase::open)
    invalid("boundary owner request horizon is not open");
  for (const auto request : requests) {
    impl_->budget.charge(1);
    if (!impl_->get_interval(request).leaf)
      invalid("owner request names a retired source interval");
  }
  reserve_more(impl_->requests, requests.size());
  impl_->requests.insert(impl_->requests.end(), requests.begin(), requests.end());
  impl_->counters.requests += requests.size();
}
void DefinitionBoundaryForest::stage_owner_promotions(
    BoundaryEpoch epoch, std::span<const BoundaryPointRevision> revisions) {
  impl_->current(epoch);
  if (impl_->phase != Impl::Phase::open)
    invalid("boundary promotion horizon is not open");
  // Validate a complete submitted set before changing even the staged set.
  std::pmr::vector<BoundaryPointRevision> combined(impl_->promotions, &impl_->budget);
  reserve_more(combined, revisions.size());
  for (const auto &revision : revisions) {
    impl_->budget.charge(1);
    if (revision.sample.value >= impl_->samples.size())
      invalid("unknown promoted source sample");
    const auto &node = impl_->samples[revision.sample.value];
    if (!node.ready || node.key != revision.expected_key ||
        node.constructed_revision != revision.expected_constructed_revision)
      invalid("stale or foreign source point revision");
    if (node.key.generator != NodeGenerator::original_edge)
      invalid("original source vertex or quotient cannot promote");
    if (node.constructed_revision == UINT64_MAX)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "source point revision exhausted");
    for (const auto x : revision.xyz)
      if (!std::isfinite(x))
        invalid("nonfinite promoted source point");
    combined.push_back(revision);
  }
  std::sort(combined.begin(), combined.end(), [&](const auto &a, const auto &b) {
    impl_->budget.charge(1);
    return a.sample < b.sample;
  });
  for (std::size_t i = 1; i < combined.size(); ++i) {
    impl_->budget.charge(1);
    if (combined[i - 1].sample == combined[i].sample)
      invalid("duplicate promoted source sample");
  }
  impl_->promotions = std::move(combined);
}
bool DefinitionBoundaryForest::horizon_complete(BoundaryEpoch epoch) const {
  impl_->current(epoch);
  return impl_->phase == Impl::Phase::open && impl_->arrivals == impl_->faces.size();
}
BoundaryEvaluationBatch DefinitionBoundaryForest::prepare_splits(BoundaryEpoch epoch) {
  impl_->current(epoch);
  if (!horizon_complete(epoch))
    invalid("incomplete boundary request horizon");
  try {
    std::sort(impl_->requests.begin(), impl_->requests.end(), [&](auto a, auto b) {
      impl_->budget.charge(1);
      return a < b;
    });
    impl_->budget.charge(impl_->requests.size());
    impl_->requests.erase(std::unique(impl_->requests.begin(), impl_->requests.end()),
                          impl_->requests.end());
    impl_->proposals.clear();
    reserve_more(impl_->proposals, impl_->requests.size());
    uint32_t previous_edge = UINT32_MAX;
    uint64_t edge_count = 0;
    for (const auto request : impl_->requests) {
      impl_->budget.charge(1);
      const auto &row = impl_->get_interval(request);
      auto &edge = impl_->edges[row.edge];
      const auto &begin = impl_->samples[row.samples[0].value].t;
      const auto &end = impl_->samples[row.samples[1].value].t;
      if (previous_edge != row.edge) {
        previous_edge = row.edge;
        edge_count = 0;
      }
      const auto ordinal = edge.next_ordinal + edge_count;
      if (ordinal < edge.next_ordinal || ordinal == UINT64_MAX)
        throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "boundary ordinal exhausted");
      const auto sample =
          BoundarySampleSlot{checked_slot(impl_->samples.size() + impl_->proposals.size())};
      impl_->proposals.push_back(
          {request, begin.add(end).divide(Rational(impl_->budget, 2)), sample, ordinal});
      ++edge_count;
    }
    impl_->pending = impl_->make_batch({});
    impl_->phase = Impl::Phase::prepared;
    return *impl_->pending;
  } catch (...) {
    impl_->phase = Impl::Phase::failed;
    throw;
  }
}
BoundaryUpdate
DefinitionBoundaryForest::complete_evaluations(BoundaryEpoch epoch,
                                               std::span<const BoundaryEvaluationResult> results) {
  impl_->current(epoch);
  if (!impl_->pending ||
      (impl_->phase != Impl::Phase::initial && impl_->phase != Impl::Phase::prepared))
    invalid("no pending boundary evaluations");
  const auto tasks = impl_->pending->tasks();
  if (results.size() != tasks.size())
    invalid("incomplete boundary evaluation result set");
  for (std::size_t i = 0; i < results.size(); ++i) {
    impl_->budget.charge(1);
    if (results[i].task != tasks[i] || results[i].task.epoch != epoch || results[i].dimension != 3)
      invalid("boundary evaluation task/result mismatch");
    for (const auto x : results[i].xyz)
      if (!std::isfinite(x))
        invalid("nonfinite canonical edge sample");
  }
  BoundaryUpdate update(&impl_->budget);
  if (impl_->phase == Impl::Phase::initial) {
    update.new_samples.reserve(results.size());
    impl_->budget.charge(results.size());
    for (const auto &result : results) {
      auto &node = impl_->samples[result.task.sample.value];
      node.xyz = result.xyz;
      node.ready = true;
      update.new_samples.push_back(result.task.sample);
    }
    impl_->pending.reset();
    impl_->phase = Impl::Phase::idle;
    return update;
  }
  // Build and reserve the entire edit before publishing a new sample/leaf.
  std::pmr::vector<Impl::Sample> new_samples(&impl_->budget);
  std::pmr::vector<Slot<FaceTag>> affected(&impl_->budget);
  new_samples.reserve(results.size());
  for (std::size_t i = 0; i < impl_->proposals.size(); ++i) {
    impl_->budget.charge(1);
    const auto &proposal = impl_->proposals[i];
    const auto &row = impl_->get_interval(proposal.key);
    const auto &edge = impl_->edges[row.edge];
    new_samples.push_back({proposal.t,
                           {impl_->definition.value, NodeGenerator::original_edge,
                            impl_->data().edges[edge.source.value].id.value, 0, proposal.ordinal},
                           results[i].xyz,
                           true});
    for (const auto face : impl_->data().edge_faces.at(edge.source.value)) {
      impl_->budget.charge(1);
      if (impl_->data().faces[face].definition_id ==
          impl_->data().definitions[impl_->definition.value].id)
        affected.push_back(Slot<FaceTag>{face});
    }
  }
  update.revised_samples.reserve(impl_->promotions.size());
  for (const auto &revision : impl_->promotions) {
    impl_->budget.charge(1);
    const auto &node = impl_->samples[revision.sample.value];
    if (node.key != revision.expected_key ||
        node.constructed_revision != revision.expected_constructed_revision)
      invalid("staged source revision is no longer current");
    const auto edge = impl_->data().edge_index.find(EdgeId{node.key.source});
    for (const auto face : impl_->data().edge_faces.at(edge.value)) {
      impl_->budget.charge(1);
      if (impl_->data().faces[face].definition_id ==
          impl_->data().definitions[impl_->definition.value].id)
        affected.push_back(Slot<FaceTag>{face});
    }
  }
  std::sort(affected.begin(), affected.end(), [&](auto a, auto b) {
    impl_->budget.charge(1);
    return a < b;
  });
  impl_->budget.charge(affected.size());
  affected.erase(std::unique(affected.begin(), affected.end()), affected.end());
  update.changed_faces.reserve(affected.size());
  update.new_samples.reserve(results.size());
  update.removed_intervals.reserve(results.size());
  update.inserted_intervals.reserve(results.size() * 2);
  for (const auto face : affected) {
    const auto generation = impl_->faces[impl_->face_index(face)].generation;
    if (generation == UINT64_MAX)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "boundary face generation exhausted");
    update.changed_faces.push_back({face, generation, generation + 1});
  }
  reserve_more(impl_->samples, new_samples.size());
  reserve_more(impl_->leaves, new_samples.size() * 2);
  try {
    for (std::size_t i = 0; i < impl_->proposals.size(); ++i) {
      impl_->budget.charge(1);
      const auto &proposal = impl_->proposals[i];
      const auto old = impl_->leaves[static_cast<std::size_t>(proposal.key.interval)];
      const auto first = impl_->leaves.size(), second = first + 1;
      impl_->samples.push_back(std::move(new_samples[i]));
      impl_->leaves.push_back({old.edge, {old.samples[0], proposal.sample}});
      impl_->leaves.push_back({old.edge, {proposal.sample, old.samples[1]}});
      auto &parent = impl_->leaves[static_cast<std::size_t>(proposal.key.interval)];
      parent.leaf = false;
      parent.children = {first, second};
      ++impl_->edges[old.edge].next_ordinal;
      update.new_samples.push_back(proposal.sample);
      update.removed_intervals.push_back(proposal.key);
      update.inserted_intervals.push_back(impl_->key(first));
      update.inserted_intervals.push_back(impl_->key(second));
    }
    for (const auto &revision : impl_->promotions) {
      impl_->budget.charge(1);
      auto &node = impl_->samples[revision.sample.value];
      node.xyz = revision.xyz;
      ++node.constructed_revision;
      update.revised_samples.push_back(revision.sample);
    }
    for (const auto &change : update.changed_faces) {
      impl_->budget.charge(1);
      impl_->faces[impl_->face_index(change.face)].generation = change.generation;
    }
    impl_->counters.splits += results.size();
    impl_->counters.samples = impl_->samples.size();
    impl_->counters.live_intervals += results.size();
    impl_->counters.invalidated_faces += affected.size();
    impl_->proposals.clear();
    impl_->promotions.clear();
    impl_->pending.reset();
    impl_->phase = Impl::Phase::idle;
    return update;
  } catch (...) {
    impl_->phase = Impl::Phase::failed;
    throw;
  }
}
uint64_t DefinitionBoundaryForest::face_generation(Slot<FaceTag> face) const {
  impl_->check();
  return impl_->faces[impl_->face_index(face)].generation;
}
std::pmr::vector<BoundaryKey> DefinitionBoundaryForest::intervals(Slot<EdgeTag> edge) const {
  impl_->check();
  std::pmr::vector<BoundaryKey> result(&impl_->budget);
  for (const auto index : impl_->ordered_leaves(edge))
    result.push_back(impl_->key(index));
  return result;
}
std::pmr::vector<BoundarySampleSlot> DefinitionBoundaryForest::chain(Slot<EdgeTag> edge) const {
  impl_->check();
  const auto ordered = impl_->ordered_leaves(edge);
  std::pmr::vector<BoundarySampleSlot> result(&impl_->budget);
  if (ordered.empty()) {
    const auto &state = impl_->edges[impl_->edge_index(edge)];
    return {state.endpoints.begin(), state.endpoints.end(), &impl_->budget};
  }
  result.reserve(ordered.size() + 1);
  result.push_back(impl_->leaves[static_cast<std::size_t>(ordered.front())].samples[0]);
  for (const auto index : ordered) {
    impl_->budget.charge(1);
    result.push_back(impl_->leaves[static_cast<std::size_t>(index)].samples[1]);
  }
  return result;
}
BoundaryIntervalView DefinitionBoundaryForest::interval(BoundaryKey key) const {
  impl_->check();
  const auto &row = impl_->get_interval(key);
  return {impl_->edges[row.edge].source, row.samples, row.leaf};
}
BoundarySampleView DefinitionBoundaryForest::sample(BoundarySampleSlot slot) const {
  impl_->check();
  if (slot.value >= impl_->samples.size())
    invalid("unknown boundary sample");
  const auto &row = impl_->samples[slot.value];
  if (!row.ready)
    invalid("boundary sample evaluation pending");
  return {row.key, row.xyz, row.constructed_revision};
}
const Rational &DefinitionBoundaryForest::parameter(BoundarySampleSlot slot) const {
  impl_->check();
  if (slot.value >= impl_->samples.size())
    invalid("unknown boundary sample");
  return impl_->samples[slot.value].t;
}
std::pmr::vector<uint8_t>
DefinitionBoundaryForest::sample_path(Slot<EdgeTag> edge, BoundarySampleSlot sample_slot) const {
  impl_->check();
  if (sample_slot.value >= impl_->samples.size() || !impl_->samples[sample_slot.value].ready)
    invalid("unknown or unsettled source sample path");
  const auto &state = impl_->edges[impl_->edge_index(edge)];
  std::pmr::vector<uint8_t> path(budget_);
  if (sample_slot == state.endpoints[0] || sample_slot == state.endpoints[1])
    return path;
  if (state.roots.empty())
    invalid("sample does not belong to degenerate edge");
  const auto &t = impl_->samples[sample_slot.value].t;
  const auto direction = impl_->samples[state.endpoints[1].value].t.compare(
      impl_->samples[state.endpoints[0].value].t);
  std::size_t first = 0, last = state.roots.size();
  while (first < last) {
    budget_->charge(1);
    const auto middle = first + (last - first) / 2;
    const auto &root = impl_->leaves[static_cast<std::size_t>(state.roots[middle])];
    if (t.compare(impl_->samples[root.samples[1].value].t) * direction > 0)
      first = middle + 1;
    else
      last = middle;
  }
  if (first == state.roots.size())
    invalid("sample outside source edge roots");
  auto current = state.roots[first];
  const auto &root = impl_->leaves[static_cast<std::size_t>(current)];
  for (const auto endpoint : root.samples)
    if (sample_slot == endpoint)
      return path;
  if (t.compare(impl_->samples[root.samples[0].value].t) * direction <= 0 ||
      t.compare(impl_->samples[root.samples[1].value].t) * direction >= 0)
    invalid("sample identity does not belong to original source root");
  uint64_t steps = 0;
  while (true) {
    budget_->charge(1);
    if (++steps > impl_->leaves.size())
      internal("source path interval cycle");
    const auto &interval = impl_->leaves[static_cast<std::size_t>(current)];
    if (interval.leaf)
      invalid("sample is not an installed source midpoint");
    const auto &left = impl_->leaves[static_cast<std::size_t>(interval.children[0])];
    const auto midpoint = left.samples[1];
    if (sample_slot == midpoint)
      return path;
    const auto order = t.compare(impl_->samples[midpoint.value].t) * direction;
    if (order == 0)
      invalid("source midpoint has a different sample identity");
    const uint8_t child = order < 0 ? 0u : 1u;
    reserve_more(path, 1);
    path.push_back(child);
    current = interval.children[child];
  }
}
SourceBoundarySnapshot DefinitionBoundaryForest::freeze_face_boundary(Slot<FaceTag> face) const {
  impl_->check();
  if (impl_->phase != Impl::Phase::idle)
    invalid("original boundary is not settled for export");
  const auto generation = impl_->faces[impl_->face_index(face)].generation;
  const auto &source = impl_->data();
  const auto &original = source.faces[face.value];
  std::pmr::vector<Slot<EdgeTag>> incident(&impl_->budget);
  for (uint64_t li = 0; li < original.loops.count; ++li) {
    impl_->budget.charge(1);
    const auto loop_id = source.loop_ids[static_cast<std::size_t>(original.loops.first + li)];
    const auto &loop = source.loops[source.loop_index.find(loop_id).value];
    for (uint64_t ui = 0; ui < loop.coedges.count; ++ui) {
      impl_->budget.charge(1);
      const auto use_id = source.coedge_ids[static_cast<std::size_t>(loop.coedges.first + ui)];
      const auto &use = source.coedges[source.coedge_index.find(use_id).value];
      incident.push_back(source.edge_index.find(use.edge_id));
    }
  }
  std::sort(incident.begin(), incident.end(), [&](auto a, auto b) {
    impl_->budget.charge(1);
    return source.edges[a.value].id < source.edges[b.value].id;
  });
  incident.erase(std::unique(incident.begin(), incident.end()), incident.end());
  auto rows = freeze_rows(incident);
  return std::allocate_shared<SourceBoundarySnapshotData>(
      RunAllocator<SourceBoundarySnapshotData>(impl_->resource), std::move(*rows), face,
      generation);
}
DefinitionBoundarySnapshot DefinitionBoundaryForest::freeze_definition_boundary() const {
  impl_->check();
  std::pmr::vector<Slot<EdgeTag>> inventory(budget_);
  inventory.reserve(impl_->edges.size());
  for (const auto &edge : impl_->edges) {
    budget_->charge(1);
    inventory.push_back(edge.source);
  }
  std::sort(inventory.begin(), inventory.end(), [&](auto a, auto b) {
    budget_->charge(1);
    return impl_->data().edges[a.value].id < impl_->data().edges[b.value].id;
  });
  return freeze_rows(inventory);
}
std::shared_ptr<DefinitionBoundarySnapshotData>
DefinitionBoundaryForest::freeze_rows(std::span<const Slot<EdgeTag>> incident) const {
  impl_->check();
  if (impl_->phase != Impl::Phase::idle)
    invalid("original boundary is not settled for export");
  ExactBuilder builder(impl_->resource, impl_->next_table());
  // Collect transport rows in the owner arena while the parameter table is
  // still open. The published object receives only sealed values and copies.
  std::pmr::vector<SourceBoundaryEdge> edge_rows(&impl_->budget);
  std::pmr::vector<PhysicalNode> node_rows(&impl_->budget);
  std::pmr::vector<uint64_t> revisions(&impl_->budget);
  std::pmr::vector<BoundaryKey> interval_rows(&impl_->budget);
  std::pmr::vector<Range<uint8_t>> paths(budget_);
  std::pmr::vector<uint8_t> bits(budget_);
  for (const auto edge : incident) {
    impl_->budget.charge(1);
    const auto samples = chain(edge);
    const auto keys = intervals(edge);
    const Range<PhysicalNode> nodes{node_rows.size(), samples.size()};
    const Range<BoundaryKey> constraints{interval_rows.size(), keys.size()};
    for (const auto slot : samples) {
      impl_->budget.charge(1);
      const auto value = sample(slot);
      node_rows.push_back({value.key, value.xyz, builder.append(parameter(slot))});
      revisions.push_back(value.constructed_revision);
      const auto path = sample_path(edge, slot);
      paths.push_back({bits.size(), path.size()});
      bits.insert(bits.end(), path.begin(), path.end());
    }
    interval_rows.insert(interval_rows.end(), keys.begin(), keys.end());
    edge_rows.push_back({edge, nodes, constraints});
  }
  auto result = std::allocate_shared<DefinitionBoundarySnapshotData>(
      RunAllocator<DefinitionBoundarySnapshotData>(impl_->resource), impl_->resource, impl_->source,
      impl_->definition, impl_->epoch, builder.seal());
  result->edges.assign(edge_rows.begin(), edge_rows.end());
  result->nodes.assign(node_rows.begin(), node_rows.end());
  result->constructed_revisions.assign(revisions.begin(), revisions.end());
  result->intervals.assign(interval_rows.begin(), interval_rows.end());
  result->sample_paths.assign(paths.begin(), paths.end());
  result->path_bits.assign(bits.begin(), bits.end());
  result->vertex_alias_owner = impl_->vertex_alias_owner;
  if (result->vertex_alias_owner)
    result->vertex_aliases = result->vertex_alias_owner->rows;
  return result;
}
BoundaryStats DefinitionBoundaryForest::stats() const {
  impl_->check();
  return impl_->counters;
}
} // namespace bgmesh
