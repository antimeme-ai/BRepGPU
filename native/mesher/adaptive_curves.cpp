#include "adaptive_curves.hpp"
#include "adaptive_cache.hpp"
#include "adaptive_curve_geometry.hpp"
#include "adaptive_curve_numeric.hpp"
#include "adaptive_device_owner.hpp"
#include "adaptive_traces.hpp"
#include "cuda/adaptive_curve_api.h"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <dlfcn.h>
#include <limits>
#include <numeric>
#include <optional>

namespace bgmesh::adaptive {
namespace {
void require(bool ok, ErrorCode code, const char *what) {
  if (!ok)
    throw Error(code, what);
}
template <class T> T &needed(std::optional<T> &value) {
  if (!value.has_value())
    throw Error(ErrorCode::internal_error, "missing active curve-session state");
  return value.value();
}
template <class T> const T &needed(const std::optional<T> &value) {
  if (!value.has_value())
    throw Error(ErrorCode::internal_error, "missing active curve-session state");
  return value.value();
}
uint32_t slot(std::size_t n) {
  require(n < invalid_slot, ErrorCode::resource_exhausted, "curve cache index overflow");
  return static_cast<uint32_t>(n);
}
uint64_t hash_word(uint64_t h, uint64_t w) {
  w ^= w >> 30;
  w *= 0xbf58476d1ce4e5b9ULL;
  w ^= w >> 27;
  w *= 0x94d049bb133111ebULL;
  return (h ^ (w ^ (w >> 31))) * 1099511628211ULL;
}
uint64_t scalar_hash(uint64_t h, const Rational &q) {
  h = hash_word(h, static_cast<uint64_t>(q.sign() + 1));
  for (auto w : q.numerator().magnitude())
    h = hash_word(h, w);
  h = hash_word(h, UINT64_MAX);
  for (auto w : q.denominator().magnitude())
    h = hash_word(h, w);
  return h;
}
bool same_scalar(const Rational &a, const Rational &b) {
  return a.sign() == b.sign() &&
         std::ranges::equal(a.numerator().magnitude(), b.numerator().magnitude()) &&
         std::ranges::equal(a.denominator().magnitude(), b.denominator().magnitude());
}
struct CachedCurvePoint {
  Slot<CurveTag> source;
  Rational t;
  CurveSampleInput input;
  std::array<CurveSampleResult, 5> values{};
  uint32_t valid = 0;
  CachedCurvePoint(Slot<CurveTag> curve, Rational parameter, CurveSampleInput task)
      : source(curve), t(std::move(parameter)), input(task) {}
};
struct CachedCurveNode {
  CurveRootSlot root;
  Rational begin, end;
  ScalarRange controls;
  uint32_t children[2]{invalid_slot, invalid_slot};
  uint32_t depth = 0, first_bound = invalid_slot, parent = invalid_slot, side = 0;
  bool ready = true;
  CachedCurveNode(CurveRootSlot original, Rational a, Rational b, ScalarRange net, uint32_t level)
      : root(original), begin(std::move(a)), end(std::move(b)), controls(net), depth(level) {}
};
struct CurveBoundTier {
  uint32_t points[2], signature, next;
  CurveNodeBounds bounds;
};
// Packed rows with one owned open-address index; exact identity is limb equality,
// never a hash alone or an ephemeral forest sample slot.
struct CurveNumericalCache {
  bgfd::Budget &budget;
  const CurveSourceCache &source;
  const CurveLimits &limits;
  CurveStats &stats;
  uint32_t initial_nodes = 0;
  std::pmr::vector<CachedCurvePoint> points;
  std::pmr::vector<CachedCurveNode> nodes;
  std::pmr::vector<uint32_t> point_index, node_index, point_stage, device_stage;
  std::pmr::vector<Interval> controls;
  std::pmr::vector<CurveBoundTier> tiers;
  std::pmr::vector<ScalarRange> installs;
  CurveNumericalCache(bgfd::Budget &owner, const CurveSourceCache &roots, const CurveLimits &caps,
                      CurveStats &counts)
      : budget(owner), source(roots), limits(caps), stats(counts), points(&owner), nodes(&owner),
        point_index(&owner), node_index(&owner), point_stage(&owner), device_stage(&owner),
        controls(&owner), tiers(&owner), installs(&owner) {
    require(source.controls().size() <= limits.control_scalars, ErrorCode::resource_exhausted,
            "curve original control capacity");
    controls.assign(source.controls().begin(), source.controls().end());
  }
  uint64_t point_hash(Slot<CurveTag> curve, const Rational &t,
                      const CurveSampleInput &input) const {
    auto h = scalar_hash(curve.value, t);
    h = hash_word(h, input.fixed_actual);
    if (input.fixed_actual)
      for (double x : input.actual_xyz)
        h = hash_word(h, std::bit_cast<uint64_t>(x));
    return h;
  }
  uint64_t point_hash(const CachedCurvePoint &p) const {
    return point_hash(p.source, p.t, p.input);
  }
  uint64_t node_hash(CurveRootSlot root, const Rational &a, const Rational &b) const {
    return scalar_hash(scalar_hash(root.value, a), b);
  }
  uint64_t node_hash(const CachedCurveNode &n) const { return node_hash(n.root, n.begin, n.end); }
  void grow_points() {
    if (!point_index.empty() && (points.size() + 1) * 2 <= point_index.size())
      return;
    const auto size = std::max<std::size_t>(16, point_index.size() * 2);
    std::pmr::vector<uint32_t> fresh(size, invalid_slot, &budget);
    for (uint32_t i = 0; i < points.size(); ++i) {
      auto at = point_hash(points[i]) & (size - 1);
      while (fresh[at] != invalid_slot) {
        budget.charge(1);
        at = (at + 1) & (size - 1);
      }
      fresh[at] = i;
    }
    point_index.swap(fresh);
  }
  void grow_nodes() {
    if (!node_index.empty() && uint64_t{initial_nodes + 1} * 2 <= node_index.size())
      return;
    const auto size = std::max<std::size_t>(16, node_index.size() * 2);
    std::pmr::vector<uint32_t> fresh(size, invalid_slot, &budget);
    for (uint32_t i = 0; i < nodes.size(); ++i) {
      if (nodes[i].parent != invalid_slot)
        continue;
      auto at = node_hash(nodes[i]) & (size - 1);
      while (fresh[at] != invalid_slot) {
        budget.charge(1);
        at = (at + 1) & (size - 1);
      }
      fresh[at] = i;
    }
    node_index.swap(fresh);
  }
  uint32_t find_point(Slot<CurveTag> curve, const Rational &t) const {
    if (point_index.empty())
      return invalid_slot;
    CurveSampleInput input{};
    auto at = point_hash(curve, t, input) & (point_index.size() - 1);
    while (point_index[at] != invalid_slot) {
      budget.charge(1);
      const auto id = point_index[at];
      const auto &p = points[id];
      if (p.source == curve && !p.input.fixed_actual && same_scalar(p.t, t))
        return id;
      at = (at + 1) & (point_index.size() - 1);
    }
    return invalid_slot;
  }
  uint32_t point(Slot<CurveTag> curve, const Rational &t, std::optional<Vec3> fixed = {}) {
    CurveSampleInput input{};
    input.original = find_original_curve_root(source, curve, t);
    const auto b = t.bounds();
    input.t = {b.lower, b.upper};
    input.fixed_actual = fixed ? 1u : 0u;
    if (fixed)
      for (uint32_t a = 0; a < 3; ++a)
        input.actual_xyz[a] = (*fixed)[a];
    grow_points();
    auto at = point_hash(curve, t, input) & (point_index.size() - 1);
    while (point_index[at] != invalid_slot) {
      budget.charge(1);
      const auto id = point_index[at];
      const auto &p = points[id];
      if (p.source == curve && same_scalar(p.t, t) && p.input.fixed_actual == input.fixed_actual &&
          (!fixed || std::equal(std::begin(p.input.actual_xyz), std::end(p.input.actual_xyz),
                                std::begin(input.actual_xyz), [](double a, double b) {
                                  return std::bit_cast<uint64_t>(a) == std::bit_cast<uint64_t>(b);
                                })))
        return id;
      at = (at + 1) & (point_index.size() - 1);
    }
    require(points.size() < limits.points, ErrorCode::resource_exhausted, "curve point capacity");
    const auto id = slot(points.size());
    points.emplace_back(curve, t.clone_into(budget), input);
    point_stage.push_back(0);
    device_stage.push_back(invalid_slot);
    point_index[at] = id;
    return id;
  }
  std::pair<std::size_t, uint32_t> node_position(CurveRootSlot root, const Rational &a,
                                                 const Rational &b) {
    grow_nodes();
    auto at = node_hash(root, a, b) & (node_index.size() - 1);
    while (node_index[at] != invalid_slot) {
      budget.charge(1);
      const auto id = node_index[at];
      const auto &n = nodes[id];
      if (n.root.value == root.value && same_scalar(n.begin, a) && same_scalar(n.end, b))
        return {at, id};
      at = (at + 1) & (node_index.size() - 1);
    }
    return {at, invalid_slot};
  }
  uint32_t initial_node(Slot<CurveTag> curve, const Rational &a, const Rational &b) {
    const std::array<Rational, 2> domain{a, b};
    const auto root = find_prepared_curve_root(source, curve, domain);
    const auto [at, existing] = node_position(root, a, b);
    if (existing != invalid_slot) {
      ++stats.nodes_reused;
      return existing;
    }
    require(nodes.size() < limits.nodes, ErrorCode::resource_exhausted, "curve node capacity");
    auto net = prepare_curve_interval_controls(source, root, domain, budget);
    require(net.size() <= limits.control_scalars - controls.size(), ErrorCode::resource_exhausted,
            "curve initial restriction control capacity");
    const ScalarRange range{slot(controls.size()), slot(net.size())};
    controls.insert(controls.end(), net.begin(), net.end());
    const auto id = slot(nodes.size());
    nodes.emplace_back(root, a.clone_into(budget), b.clone_into(budget), range, 0);
    node_index[at] = id;
    ++initial_nodes;
    ++stats.nodes_created;
    if (range.count)
      installs.push_back(range);
    return id;
  }
  std::array<uint32_t, 2> children(uint32_t parent) {
    if (nodes[parent].children[0] != invalid_slot) {
      stats.nodes_reused += 2;
      return {nodes[parent].children[0], nodes[parent].children[1]};
    }
    require(nodes[parent].depth < limits.max_depth, ErrorCode::unresolved,
            "curve path depth exhausted");
    require(nodes.size() + 2 <= limits.nodes, ErrorCode::resource_exhausted,
            "curve child node capacity");
    const auto count = nodes[parent].controls.count;
    require(uint64_t{count} * 2 <= limits.control_scalars - controls.size(),
            ErrorCode::resource_exhausted, "curve paired control capacity");
    const auto a = nodes[parent].begin.clone_into(budget), z = nodes[parent].end.clone_into(budget);
    auto middle = a.add(z).divide(Rational(budget, 2));
    const auto root = nodes[parent].root;
    const auto depth = nodes[parent].depth + 1;
    const auto first = slot(nodes.size());
    const auto offset = slot(controls.size());
    controls.resize(controls.size() + 2 * count);
    nodes.emplace_back(root, a, middle, ScalarRange{offset, count}, depth);
    nodes.emplace_back(root, middle, z, ScalarRange{offset + count, count}, depth);
    for (uint32_t i = 0; i < 2; ++i) {
      nodes[first + i].parent = parent;
      nodes[first + i].side = i;
      nodes[first + i].ready = !count;
    }
    nodes[parent].children[0] = first;
    nodes[parent].children[1] = first + 1;
    stats.nodes_created += 2;
    return {first, first + 1};
  }
};
using CurveOwner = detail::DeviceOwner<CurveCudaAPI>;
std::atomic<uint64_t> curve_sessions{1}, forest_tables{1};
} // namespace

struct AdaptiveBoundarySession::Impl {
  enum class Phase {
    idle,
    evaluate,
    evaluation_fold,
    prepare,
    nodes,
    node_fold,
    trace_source,
    trace_source_fold,
    trace_carrier_fold,
    trace_bound_fold,
    commit,
    restrictions,
    done,
    failed,
    closed
  };
  enum class Operation { none, samples, nodes, restrictions, carriers, traces };
  struct StopForward {
    std::stop_source *source;
    void operator()() const noexcept { source->request_stop(); }
  };
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  Slot<DefinitionTag> definition;
  std::shared_ptr<const CurveSourceCache> roots;
  CurveLimits limits;
  std::stop_source cancellation;
  std::stop_token stop;
  std::optional<std::stop_callback<StopForward>> external_stop;
  bgfd::Budget budget;
  CurveStats counts;
  CurveNumericalCache cache;
  uint64_t serial = curve_sessions.fetch_add(1), query = 0, batch_serial = 0, ticket = 0;
  uint32_t ordinal = 0, jobs = 0, install_index = 0;
  Phase phase = Phase::idle;
  Operation operation = Operation::none;
  bool source_guards_done = false;
  std::optional<NativeScheduler> workers;
  CurveOwner *device = nullptr;
  std::optional<CurveBoundaryQuery> request;
  std::pmr::vector<FaceCurveAllowance> overrides;
  std::optional<SourceCache> carriers;
  std::optional<DefinitionBoundaryForest> forest;
  std::optional<BoundaryEvaluationBatch> evaluation;
  DefinitionBoundarySnapshot snapshot;
  PhysicalCurveBounds physical;
  SharedCurveSnapshot output;
  std::pmr::vector<uint32_t> interval_nodes, sample_ids, evaluation_ids, promotion_points;
  std::pmr::vector<BoundaryPointRevision> promotion_rows;
  std::pmr::vector<BoundaryKey> live_keys, requests;
  std::pmr::vector<uint32_t> live_nodes;
  std::pmr::vector<CurveDeviceSample> sample_tasks;
  std::pmr::vector<CurveSampleResult> sample_answers;
  std::pmr::vector<CurveDeviceNode> node_tasks;
  std::pmr::vector<CurveDeviceNodeAnswer> node_answers;
  std::pmr::vector<uint32_t> node_certified;
  std::pmr::vector<CurveDeviceRestriction> restriction_tasks;
  std::pmr::vector<AffineTraceBoundInput> trace_tasks;
  std::pmr::vector<Interval> trace_controls;
  std::pmr::vector<CurveNodeBounds> trace_answers;
  struct CarrierTask {
    OriginalRootSlot original;
    ExactTable table;
    std::array<ExactRef, 2> uv;
  };
  std::pmr::vector<CarrierTask> carrier_tasks;
  std::pmr::vector<SampleResult> carrier_answers;
  std::pmr::vector<TraceSourceWork> trace_sources;
  std::pmr::vector<TraceSiteWork> trace_site_work;
  std::pmr::vector<TraceIntervalWork> trace_interval_work;
  std::pmr::vector<UseTraceResult> trace_results;
  std::pmr::vector<Range<uint32_t>> trace_ranges;
  std::pmr::vector<Slot<FaceTag>> active_faces;
  std::pmr::vector<uint32_t> active_face_indices, restriction_parents;
  Impl(std::shared_ptr<RunResource> owner, SourceStore original, Slot<DefinitionTag> part,
       CurveSourceCache compiled, CurveLimits cap, const std::stop_token &token)
      : resource(std::move(owner)), source(std::move(original)), definition(part),
        roots(std::allocate_shared<CurveSourceCache>(RunAllocator<CurveSourceCache>(resource),
                                                     std::move(compiled))),
        limits(cap), stop(cancellation.get_token()),
        budget({cap.host_bytes, cap.total_work}, resource.get(), stop),
        cache(budget, *roots, limits, counts), overrides(&budget), interval_nodes(&budget),
        sample_ids(&budget), evaluation_ids(&budget), promotion_points(&budget),
        promotion_rows(&budget), live_keys(&budget), requests(&budget), live_nodes(&budget),
        sample_tasks(&budget), sample_answers(&budget), node_tasks(&budget), node_answers(&budget),
        node_certified(&budget), restriction_tasks(&budget), trace_tasks(&budget),
        trace_controls(&budget), trace_answers(&budget), carrier_tasks(&budget),
        carrier_answers(&budget), trace_sources(&budget), trace_site_work(&budget),
        trace_interval_work(&budget), trace_results(&budget), trace_ranges(&budget),
        active_faces(&budget), active_face_indices(&budget), restriction_parents(&budget) {
    require(resource && source.view().resource == resource &&
                &roots->source().view() == &source.view(),
            ErrorCode::invalid_source, "curve session source/resource mismatch");
    require(part.value < source.view().definitions.size() && cap.cpu_threads &&
                cap.cpu_threads <= 1024 && cap.nodes && cap.points && cap.frontier &&
                cap.requests && cap.max_depth && cap.max_depth <= 64 && cap.control_scalars &&
                cap.child_bytes && cap.child_work && cap.total_work && cap.host_bytes,
            ErrorCode::invalid_source, "invalid curve session limits/definition");
    external_stop.emplace(token, StopForward{&cancellation});
    admit_numeric_environment();
    if (cap.cpu_threads > 1)
      workers.emplace(resource, SchedulerLimits{cap.cpu_threads, cap.cpu_threads * 4,
                                                cap.child_bytes, cap.child_work});
  }
  ~Impl() {
    if (workers) {
      workers->stop();
      workers->drain();
    }
  }
  void stopped() {
    if (stop.stop_requested())
      throw Error(ErrorCode::cancelled, "shared curve query cancelled");
    budget.charge(0);
  }
  ExactTableKey table_key(uint32_t count = 1) {
    require(count <= UINT32_MAX - ordinal, ErrorCode::resource_exhausted,
            "curve task table ordinals exhausted");
    const auto first = ordinal;
    ordinal += count;
    return {{OwnerKind::run, UINT32_MAX - 4, serial}, query, first};
  }
  FaceCurveAllowance face_allowance(Slot<FaceTag> face) const {
    const auto it = std::lower_bound(overrides.begin(), overrides.end(), face,
                                     [](const auto &row, auto key) { return row.face < key; });
    if (it != overrides.end() && it->face == face)
      return *it;
    return {face, needed(request).default_positional, needed(request).default_node_source};
  }
  Rational edge_allowance(Slot<EdgeTag> edge) {
    auto result =
        import_exact(budget, needed(request).values.view(needed(request).default_positional));
    for (auto index : source.view().edge_faces.at(edge.value)) {
      const Slot<FaceTag> face{index};
      if (source.view().faces[index].definition_id !=
          source.view().definitions[definition.value].id)
        continue;
      auto tau = import_exact(budget, needed(request).values.view(face_allowance(face).positional));
      if (tau.compare(result) < 0)
        result = std::move(tau);
    }
    return result;
  }
  Rational edge_node_allowance(Slot<EdgeTag> edge) {
    auto result =
        import_exact(budget, needed(request).values.view(needed(request).default_node_source));
    for (auto f : source.view().edge_faces.at(edge.value)) {
      if (source.view().faces[f].definition_id != source.view().definitions[definition.value].id)
        continue;
      auto cap = import_exact(
          budget, needed(request).values.view(face_allowance(Slot<FaceTag>{f}).node_source));
      if (cap.compare(result) < 0)
        result = std::move(cap);
    }
    return result;
  }
  bool image_allowed(Slot<EdgeTag> edge) const {
    for (auto f : source.view().edge_faces.at(edge.value)) {
      if (source.view().faces[f].definition_id != source.view().definitions[definition.value].id)
        continue;
      const auto carrier = source.view().carrier_index.find(source.view().faces[f].carrier_id);
      if (source.view().carriers[carrier.value].kind != 1)
        return false;
    }
    return true;
  }
  const CurveSampleResult &point(uint32_t id) const {
    return cache.points[id].values[cache.point_stage[id]];
  }
  uint32_t boundary_point(Slot<CurveTag> curve, BoundarySampleSlot node) {
    const auto sample = needed(forest).sample(node);
    try {
      return cache.point(curve, needed(forest).parameter(node),
                         sample.key.generator == NodeGenerator::original_edge
                             ? std::nullopt
                             : std::optional<Vec3>{sample.xyz});
    } catch (const Error &error) {
      if (error.code != ErrorCode::invalid_source)
        throw;
      throw Error(ErrorCode::unsupported,
                  "original boundary endpoint has no supplied curve source-cache root");
    }
  }
  void rows(Operation op, uint32_t first, uint32_t end, JobContext &context) {
    for (uint32_t i = first; i < end; ++i) {
      if (stop.stop_requested())
        throw Error(ErrorCode::cancelled, "curve batch cancelled");
      context.budget.charge(op == Operation::nodes ? 8192u : 4096u);
      if (op == Operation::samples)
        sample_answers[i] = evaluate_curve_sample(*roots, sample_tasks[i].input);
      else if (op == Operation::nodes) {
        const auto &task = node_tasks[i];
        auto &answer = node_answers[i];
        answer.bounds = task.cached ? task.cached_bounds
                                    : bound_curve_node(*roots, task.input,
                                                       std::span<const Interval>(cache.controls)
                                                           .subspan(task.input.controls.first,
                                                                    task.input.controls.count));
        answer.selected = CurveCorrespondence::affine_parameter;
        auto bound = answer.bounds.parameter_upper;
        if (task.allow_image && answer.bounds.image_available &&
            answer.bounds.image_upper <= bound) {
          bound = answer.bounds.image_upper;
          answer.selected = answer.bounds.correspondence;
        }
        answer.action = answer.bounds.status != NumericStatus::ready ? 2u
                        : bound <= task.tolerance_down               ? 0u
                                                                     : 1u;
      } else if (op == Operation::restrictions) {
        const auto &task = restriction_tasks[i];
        const auto &root = roots->roots()[task.root.value];
        std::array<Interval, 132> scratch{};
        restrict_curve_midpoint_pair(
            root,
            std::span<const Interval>(cache.controls).subspan(task.parent.first, task.parent.count),
            std::span<Interval>(cache.controls).subspan(task.left_destination, task.parent.count),
            std::span<Interval>(cache.controls).subspan(task.right_destination, task.parent.count),
            scratch);
      } else if (op == Operation::carriers) {
        const auto &task = carrier_tasks[i];
        std::array<Rational, 2> uv{import_exact(context.budget, task.table.view(task.uv[0])),
                                   import_exact(context.budget, task.table.view(task.uv[1]))};
        SampleInput input{};
        input.original = task.original;
        for (uint32_t a = 0; a < 2; ++a) {
          const auto bounds = uv[a].bounds();
          input.uv[a] = {bounds.lower, bounds.upper};
        }
        auto value = evaluate_sample(needed(carriers), input);
        if (value.status != NumericStatus::ready)
          value = evaluate_sample_precise(needed(carriers), task.original, uv,
                                          ArithmeticStage::p128, context.budget);
        require(value.status == NumericStatus::ready, ErrorCode::unresolved,
                "chosen trace carrier sample unresolved");
        carrier_answers[i] = value;
      } else if (op == Operation::traces)
        trace_answers[i] = numeric::bound_affine_trace_rn64(
            trace_tasks[i], trace_tasks[i].input.controls.count
                                ? trace_controls.data() + trace_tasks[i].input.controls.first
                                : nullptr);
    }
  }
  struct KernelJob final : NativeTask {
    Impl *owner;
    Operation operation;
    Range<uint32_t> range;
    KernelJob(NativeTaskKey key, Impl *p, Operation op, Range<uint32_t> slice)
        : NativeTask(key), owner(p), operation(op), range(slice) {}
    StepState step(JobContext &context) override {
      owner->rows(operation, slot(range.first), slot(range.first + range.count), context);
      return StepState::done;
    }
  };
  void submit(Operation op, uint32_t count, JobContext &context) {
    require(operation == Operation::none && !jobs && !ticket, ErrorCode::internal_error,
            "overlapping curve batch");
    operation = op;
    require(count <= limits.frontier, ErrorCode::resource_exhausted,
            "curve horizon batch capacity");
    budget.charge(uint64_t{count} * (op == Operation::nodes ? 8192u : 4096u));
    if (!count)
      return;
    if (device && op != Operation::carriers) {
      DeviceError error{};
      int32_t status = 0;
      if (op == Operation::samples)
        status = device->api->samples(device->session, sample_tasks.data(), count, &ticket, &error);
      else if (op == Operation::nodes)
        status = device->api->nodes(device->session, node_tasks.data(), count, &ticket, &error);
      else if (op == Operation::restrictions)
        status = device->api->paired_children(device->session, restriction_tasks.data(), count,
                                              &ticket, &error);
      else
        status = device->api->trace_bounds(device->session, trace_tasks.data(), count,
                                           trace_controls.data(), slot(trace_controls.size()),
                                           &ticket, &error);
      detail::device_check(status, error);
      return;
    }
    if (!workers) {
      rows(op, 0, count, context);
      return;
    }
    ++batch_serial;
    const auto number = std::min(count, limits.cpu_threads * 4);
    // Degree-squared costs partition high-degree curves without one worker per
    // edge; stable prefix sums choose slices, completion order never folds rows.
    std::pmr::vector<uint64_t> costs(count + 1, 0, &budget);
    for (uint32_t i = 0; i < count; ++i) {
      uint64_t degree = 1;
      if (op == Operation::samples)
        degree = roots->original_roots()[sample_tasks[i].input.original.value].degree + 1;
      else if (op == Operation::nodes)
        degree = roots->roots()[node_tasks[i].input.root.value].degree + 1;
      else if (op == Operation::restrictions)
        degree = roots->roots()[restriction_tasks[i].root.value].degree + 1;
      else if (op == Operation::traces)
        degree = trace_tasks[i].rational_curve.degree + 1;
      costs[i + 1] = costs[i] + degree * degree;
    }
    uint32_t first = 0;
    for (uint32_t i = 0; i < number; ++i) {
      uint32_t end = count;
      if (i + 1 < number) {
        const auto target = costs.back() * (i + 1) / number;
        end = static_cast<uint32_t>(
            std::lower_bound(costs.begin() + first + 1, costs.end(), target) - costs.begin());
        end = std::clamp(end, first + 1, count - (number - i - 1));
      }
      NativeTaskKey key{{OwnerKind::run, UINT32_MAX - 6, uint64_t{i} + 1}, batch_serial, serial};
      workers->submit(std::allocate_shared<KernelJob>(RunAllocator<KernelJob>(resource), key, this,
                                                      op, Range<uint32_t>{first, end - first}));
      first = end;
    }
    jobs = number;
  }
  bool ready(void *answers, uint32_t count) {
    if (device && ticket) {
      DeviceError error{};
      uint32_t done = 0;
      detail::device_check(
          device->api->poll(device->session, ticket, &done, answers, count, &error), error);
      if (!done)
        return false;
      ticket = 0;
    }
    if (workers && jobs) {
      while (auto completion = workers->take()) {
        --jobs;
        completion->rethrow();
      }
      if (jobs)
        return false;
    }
    operation = Operation::none;
    return true;
  }
  void queue_points(std::span<const uint32_t> ids, JobContext &context) {
    sample_ids.assign(ids.begin(), ids.end());
    std::sort(sample_ids.begin(), sample_ids.end());
    sample_ids.erase(std::unique(sample_ids.begin(), sample_ids.end()), sample_ids.end());
    sample_tasks.clear();
    for (const auto id : sample_ids) {
      auto &p = cache.points[id];
      const auto stage = cache.point_stage[id];
      const bool valid = (p.valid & (1u << stage)) != 0;
      if (valid && (!device || cache.device_stage[id] == stage)) {
        ++counts.point_hits;
        continue;
      }
      auto input = p.input;
      input.stage = static_cast<ArithmeticStage>(stage);
      sample_tasks.push_back(
          {id, input, valid ? p.values[stage] : CurveSampleResult{}, valid ? 1u : 0u});
    }
    sample_answers.resize(sample_tasks.size());
    submit(Operation::samples, slot(sample_tasks.size()), context);
  }
  void fold_points(JobContext &context) {
    for (uint32_t i = 0; i < sample_tasks.size(); ++i) {
      auto &task = sample_tasks[i];
      auto &cached = cache.points[task.destination];
      auto value = sample_answers[i];
      auto &stage = cache.point_stage[task.destination];
      if (!task.supplied) {
        ++counts.point_evaluations;
        if (device)
          ++counts.device_points;
      }
      while (value.status != NumericStatus::ready) {
        require(stage < 4, ErrorCode::unresolved, "curve point precision exhausted");
        require(++counts.exact_fallbacks <= limits.total_work, ErrorCode::resource_exhausted,
                "curve fallback work cap");
        ++stage;
        value = evaluate_curve_sample_precise(
            *roots, cached.input.original, cached.t.clone_into(context.budget),
            static_cast<ArithmeticStage>(stage), context.budget,
            cached.input.fixed_actual
                ? std::optional<Vec3>{{cached.input.actual_xyz[0], cached.input.actual_xyz[1],
                                       cached.input.actual_xyz[2]}}
                : std::nullopt);
      }
      cached.values[stage] = value;
      cached.valid |= 1u << stage;
      cache.device_stage[task.destination] =
          device && stage == static_cast<uint32_t>(task.input.stage) ? stage : invalid_slot;
    }
  }
  bool install_ready() {
    if (!device)
      return true;
    if (ticket) {
      if (!ready(nullptr, 0))
        return false;
      ++install_index;
    }
    if (install_index < cache.installs.size()) {
      const auto range = cache.installs[install_index];
      DeviceError error{};
      detail::device_check(device->api->install_controls(device->session, range.first,
                                                         cache.controls.data() + range.first,
                                                         range.count, &ticket, &error),
                           error);
      return false;
    }
    return true;
  }
  void prepare_horizon(JobContext &context) {
    snapshot = needed(forest).freeze_definition_boundary();
    live_keys.clear();
    live_nodes.clear();
    std::pmr::vector<uint32_t> points(&budget);
    for (const auto &edge_row : snapshot->edges) {
      const auto &edge = source.view().edges[edge_row.edge.value];
      const auto chain = needed(forest).chain(edge_row.edge);
      const auto keys = needed(forest).intervals(edge_row.edge);
      if (keys.empty()) {
        if (edge.curve_id) {
          const auto curve = source.view().curve_index.find(edge.curve_id);
          for (auto node : chain)
            points.push_back(boundary_point(curve, node));
        }
        continue;
      }
      require(edge.curve_id.value != 0, ErrorCode::unsupported,
              "nondegenerate edge has no original curve");
      const auto curve = source.view().curve_index.find(edge.curve_id);
      for (uint32_t j = 0; j < keys.size(); ++j) {
        const auto key = keys[j];
        require(live_keys.size() < std::min(limits.frontier, limits.nodes),
                ErrorCode::resource_exhausted, "curve live frontier capacity");
        if (key.interval >= interval_nodes.size())
          interval_nodes.resize(key.interval + 1, invalid_slot);
        if (interval_nodes[key.interval] == invalid_slot) {
          const auto &a = needed(forest).parameter(chain[j]),
                     &z = needed(forest).parameter(chain[j + 1]);
          interval_nodes[key.interval] =
              a.compare(z) < 0 ? cache.initial_node(curve, a, z) : cache.initial_node(curve, z, a);
        }
        const auto id = interval_nodes[key.interval];
        live_keys.push_back(key);
        live_nodes.push_back(id);
        points.push_back(boundary_point(curve, chain[j]));
        points.push_back(boundary_point(curve, chain[j + 1]));
      }
    }
    evaluation_ids = points;
    queue_points(evaluation_ids, context);
    phase = Phase::nodes;
  }
  void check_source_guards(JobContext &context) {
    if (source_guards_done)
      return;
    uint32_t ambiguities = 0;
    for (auto index : source.view().definition_edges.at(definition.value)) {
      const Slot<EdgeTag> edge_slot{index};
      const auto &edge = source.view().edges[index];
      const auto chain = needed(forest).chain(edge_slot);
      require(chain.size() >= 2, ErrorCode::internal_error, "original endpoint chain missing");
      const auto cap = edge_node_allowance(edge_slot).clone_into(context.budget);
      const auto cap_squared = cap.multiply(cap);
      auto exact_guard = [&](std::span<const Rational, 3> a, const Vec3 &target) {
        Rational square(context.budget);
        for (uint32_t axis = 0; axis < 3; ++axis) {
          const auto d = a[axis].subtract(Rational::from_double(context.budget, target[axis]));
          square = square.add(d.multiply(d));
        }
        require(square.compare(cap_squared) <= 0, ErrorCode::unresolved,
                "original endpoint source-node cap exceeded");
      };
      for (uint32_t endpoint = 0; endpoint < 2; ++endpoint) {
        const auto node = endpoint == 0 ? chain.front() : chain.back();
        const auto canonical = needed(forest).sample(node);
        const auto original_slot = source.view().vertex_index.find(edge.vertex_ids[endpoint]);
        const auto &v = source.view().vertices[original_slot.value].xyz;
        const std::array<Rational, 3> exact_vertex{Rational::from_double(context.budget, v[0]),
                                                   Rational::from_double(context.budget, v[1]),
                                                   Rational::from_double(context.budget, v[2])};
        exact_guard(exact_vertex, canonical.xyz);
        if (!edge.curve_id)
          continue;
        const auto curve = source.view().curve_index.find(edge.curve_id);
        const auto id = boundary_point(curve, node);
        const auto &sample = point(id);
        for (const auto &target : std::array<Vec3, 2>{v, canonical.xyz}) {
          Interval square{};
          for (uint32_t axis = 0; axis < 3; ++axis)
            square =
                numeric::add(square, numeric::square(numeric::sub(sample.original_xyz[axis],
                                                                  numeric::point(target[axis]))));
          if (numeric::valid(square) && std::isfinite(square.upper) &&
              Rational::from_double(context.budget, square.upper).compare(cap_squared) <= 0)
            continue;
          if (numeric::valid(square) && std::isfinite(square.lower) &&
              Rational::from_double(context.budget, std::max(0., square.lower))
                      .compare(cap_squared) > 0)
            throw Error(ErrorCode::unresolved, "original endpoint source-node cap exceeded");
          require(++ambiguities <= limits.ambiguities, ErrorCode::resource_exhausted,
                  "source-node ambiguity horizon capacity");
          ++counts.exact_fallbacks;
          auto t = needed(forest).parameter(node).clone_into(context.budget);
          if (const auto exact = exact_curve_value(*roots, curve, t, context.budget)) {
            exact_guard(*exact, target);
            continue;
          }
          bool accepted = false;
          for (uint32_t stage = 1; stage <= 4 && !accepted; ++stage) {
            const auto precise = evaluate_curve_sample_precise(
                *roots, cache.points[id].input.original, t, static_cast<ArithmeticStage>(stage),
                context.budget, target);
            accepted =
                precise.status == NumericStatus::ready && std::isfinite(precise.departure_upper) &&
                Rational::from_double(context.budget, precise.departure_upper).compare(cap) <= 0;
          }
          require(accepted, ErrorCode::unresolved,
                  "original endpoint source-node guard unresolved at precision cap");
        }
      }
    }
    source_guards_done = true;
  }
  uint32_t bound_signature(uint32_t a, uint32_t b) const {
    return cache.point_stage[a] | (cache.point_stage[b] << 3);
  }
  const CurveNodeBounds *cached_bound(uint32_t node, std::array<uint32_t, 2> endpoints) const {
    const auto a = endpoints[0], b = endpoints[1];
    for (auto at = cache.nodes[node].first_bound; at != invalid_slot; at = cache.tiers[at].next) {
      const auto &t = cache.tiers[at];
      if (t.points[0] == a && t.points[1] == b && t.signature == bound_signature(a, b))
        return &t.bounds;
    }
    return nullptr;
  }
  void prepare_nodes(JobContext &context) {
    node_tasks.clear();
    for (uint32_t i = 0; i < live_keys.size(); ++i) {
      const auto view = needed(forest).interval(live_keys[i]);
      const auto curve =
          source.view().curve_index.find(source.view().edges[view.edge.value].curve_id);
      auto a = boundary_point(curve, view.samples[0]), z = boundary_point(curve, view.samples[1]);
      if (cache.points[a].t.compare(cache.points[z].t) > 0)
        std::swap(a, z);
      const auto id = live_nodes[i];
      const auto &n = cache.nodes[id];
      CurveDeviceNode task{};
      task.slot = i;
      task.input.root = n.root;
      task.input.controls = n.controls;
      const auto lo = n.begin.bounds(), hi = n.end.bounds();
      task.input.t[0] = {lo.lower, lo.upper};
      task.input.t[1] = {hi.lower, hi.upper};
      task.points[0] = a;
      task.points[1] = z;
      for (uint32_t end = 0; end < 2; ++end) {
        const auto &p = point(task.points[end]);
        task.input.endpoint_departures[end] = p.departure_upper;
        std::copy(std::begin(p.actual_xyz), std::end(p.actual_xyz),
                  task.input.actual_endpoints + end * 3);
      }
      task.allow_image = image_allowed(view.edge) ? 1u : 0u;
      task.tolerance_down = edge_allowance(view.edge).bounds().lower;
      if (const auto *cached = cached_bound(id, {a, z})) {
        task.cached = 1;
        task.cached_bounds = *cached;
      }
      node_tasks.push_back(task);
    }
    node_answers.resize(node_tasks.size());
    node_certified.assign(node_tasks.size(), 0);
    submit(Operation::nodes, slot(node_tasks.size()), context);
    phase = Phase::node_fold;
  }
  void fold_nodes(JobContext &context) {
    requests.clear();
    promotion_points.clear();
    promotion_rows.clear();
    uint32_t ambiguities = 0;
    for (uint32_t i = 0; i < node_tasks.size(); ++i) {
      const auto &task = node_tasks[i];
      auto &answer = node_answers[i];
      ++counts.chord_tests;
      if (device && !task.cached)
        ++counts.device_bounds;
      if (!task.cached && answer.bounds.status == NumericStatus::ready) {
        auto &node = cache.nodes[live_nodes[i]];
        cache.tiers.push_back({{task.points[0], task.points[1]},
                               bound_signature(task.points[0], task.points[1]),
                               node.first_bound,
                               answer.bounds});
        node.first_bound = slot(cache.tiers.size() - 1);
      }
      const auto view = needed(forest).interval(live_keys[i]);
      auto tau = edge_allowance(view.edge);
      bool accepted = false;
      if (answer.bounds.status == NumericStatus::ready) {
        auto upper = answer.bounds.parameter_upper;
        if (task.allow_image && answer.bounds.image_available &&
            answer.bounds.image_upper <= upper) {
          upper = answer.bounds.image_upper;
          answer.selected = answer.bounds.correspondence;
        } else
          answer.selected = CurveCorrespondence::affine_parameter;
        accepted =
            std::isfinite(upper) &&
            Rational::from_double(context.budget, upper).compare(tau.clone_into(context.budget)) <=
                0;
      }
      // This canonical operation-count band chooses whether to spend exact
      // work; it proves nothing. READY failures far from the threshold still
      // split directly. Only the exact same-task comparison can accept equality.
      auto selected_upper = answer.bounds.parameter_upper;
      if (task.allow_image && answer.bounds.image_available)
        selected_upper = std::min(selected_upper, answer.bounds.image_upper);
      const auto tolerance_upper = tau.bounds().upper;
      const auto degree = roots->roots()[task.input.root.value].degree + 1;
      const auto rounding_band =
          numeric::add(
              numeric::point(answer.bounds.numerical_upper),
              numeric::scale(numeric::point(std::max(selected_upper, tolerance_upper)),
                             64. * static_cast<double>(degree) * static_cast<double>(degree) *
                                 std::numeric_limits<double>::epsilon()))
              .upper;
      const bool near_threshold =
          std::isfinite(selected_upper) && std::isfinite(tolerance_upper) &&
          selected_upper <=
              numeric::add(numeric::point(tolerance_upper), numeric::point(rounding_band)).upper;
      if (!accepted && (answer.bounds.status != NumericStatus::ready || near_threshold ||
                        answer.bounds.numerical_upper > task.tolerance_down)) {
        require(++ambiguities <= limits.ambiguities, ErrorCode::resource_exhausted,
                "curve precise ambiguity horizon capacity");
        require(++counts.exact_fallbacks <= limits.total_work, ErrorCode::resource_exhausted,
                "curve exact work cap");
        const auto &node = cache.nodes[live_nodes[i]];
        const std::array<Rational, 2> domain{node.begin.clone_into(context.budget),
                                             node.end.clone_into(context.budget)};
        const auto precise =
            bound_curve_node_precise(*roots, node.root, domain, task.input.actual_endpoints,
                                     tau.clone_into(context.budget), context.budget);
        answer.bounds = precise.bounds;
        node_certified[i] =
            (precise.parameter_certified ? 1u : 0u) | (precise.image_certified ? 2u : 0u);
        if (task.allow_image && precise.image_certified) {
          accepted = true;
          answer.selected = CurveCorrespondence::monotone_line_image;
        } else if (precise.parameter_certified) {
          accepted = true;
          answer.selected = CurveCorrespondence::affine_parameter;
        }
      }
      require(answer.bounds.status == NumericStatus::ready, ErrorCode::unresolved,
              "curve bound unresolved after precise fallback");
      if (!accepted && answer.bounds.numerical_upper > task.tolerance_down) {
        for (auto id : task.points)
          promotion_points.push_back(id);
      } else if (!accepted)
        requests.push_back(live_keys[i]);
    }
    std::sort(promotion_points.begin(), promotion_points.end());
    promotion_points.erase(std::unique(promotion_points.begin(), promotion_points.end()),
                           promotion_points.end());
    require(promotion_points.size() <= limits.ambiguities, ErrorCode::resource_exhausted,
            "curve promotion horizon capacity");
    for (auto id : promotion_points) {
      auto &cached = cache.points[id];
      auto &stage = cache.point_stage[id];
      require(stage < 4, ErrorCode::unresolved,
              "actual curve point numerical floor exceeds requested accuracy");
      ++stage;
      if (!(cached.valid & (1u << stage))) {
        ++counts.exact_fallbacks;
        auto value = evaluate_curve_sample_precise(
            *roots, cached.input.original, cached.t.clone_into(context.budget),
            static_cast<ArithmeticStage>(stage), context.budget,
            cached.input.fixed_actual
                ? std::optional<Vec3>{{cached.input.actual_xyz[0], cached.input.actual_xyz[1],
                                       cached.input.actual_xyz[2]}}
                : std::nullopt);
        require(value.status == NumericStatus::ready, ErrorCode::unresolved,
                "promoted physical curve point unresolved");
        cached.values[stage] = value;
        cached.valid |= 1u << stage;
      }
    }
    if (!promotion_points.empty()) {
      for (auto index : source.view().definition_edges.at(definition.value)) {
        const Slot<EdgeTag> edge{index};
        const auto &original = source.view().edges[index];
        if (!original.curve_id)
          continue;
        const auto curve = source.view().curve_index.find(original.curve_id);
        const auto chain = needed(forest).chain(edge);
        for (auto node : chain) {
          const auto old = needed(forest).sample(node);
          if (old.key.generator != NodeGenerator::original_edge)
            continue;
          const auto id = cache.point(curve, needed(forest).parameter(node));
          if (!std::binary_search(promotion_points.begin(), promotion_points.end(), id))
            continue;
          const auto &next = point(id);
          promotion_rows.push_back({node,
                                    old.key,
                                    old.constructed_revision,
                                    {next.actual_xyz[0], next.actual_xyz[1], next.actual_xyz[2]}});
        }
      }
    }
  }
  void build_physical(JobContext &context) {
    ExactBuilder values(resource, table_key());
    std::pmr::vector<PhysicalCurveInterval> rows(resource.get());
    rows.reserve(live_keys.size());
    uint32_t at = 0;
    for (const auto &edge_row : snapshot->edges) {
      const auto &edge = source.view().edges[edge_row.edge.value];
      const auto keys = needed(forest).intervals(edge_row.edge);
      const auto chain = needed(forest).chain(edge_row.edge);
      for (uint32_t j = 0; j < keys.size(); ++j, ++at) {
        require(keys[j] == live_keys[at], ErrorCode::internal_error, "curve horizon order changed");
        auto a = chain[j], z = chain[j + 1];
        auto ia = slot(edge_row.nodes.first + j), iz = ia + 1;
        if (needed(forest).parameter(a).compare(needed(forest).parameter(z)) > 0) {
          std::swap(a, z);
          std::swap(ia, iz);
        }
        PhysicalCurveInterval row{};
        row.boundary_key = keys[j];
        row.boundary_nodes = {ia, iz};
        row.original_t = {values.append(needed(forest).parameter(a)),
                          values.append(needed(forest).parameter(z))};
        const auto &bound = node_answers[at];
        const auto exact_allowance = edge_allowance(edge_row.edge);
        row.parameter_chord_upper = (node_certified[at] & 1u)
                                        ? values.append(exact_allowance)
                                        : values.append(Rational::from_double(
                                              context.budget, bound.bounds.parameter_upper));
        row.selected_correspondence = bound.selected;
        if (bound.bounds.image_available) {
          row.image_chord_upper =
              (node_certified[at] & 2u)
                  ? values.append(exact_allowance)
                  : values.append(Rational::from_double(context.budget, bound.bounds.image_upper));
          const auto &descriptor = roots->roots()[cache.nodes[live_nodes[at]].root.value];
          const auto curve = source.view().curve_index.find(edge.curve_id);
          const auto first = exact_curve_value(
              *roots, curve, needed(forest).parameter(a).clone_into(context.budget),
              context.budget);
          const auto last = exact_curve_value(
              *roots, curve, needed(forest).parameter(z).clone_into(context.budget),
              context.budget);
          require(first && last, ErrorCode::unsupported,
                  "line image has no exact source scalar profile");
          GeometricLineRecord map{};
          for (uint32_t axis = 0; axis < 3; ++axis)
            map.covector[axis] =
                values.append(Rational(context.budget, axis == descriptor.line_axis ? 1 : 0));
          map.curve_scalar = {values.append(needed(first)[descriptor.line_axis]),
                              values.append(needed(last)[descriptor.line_axis])};
          map.trace_scalar = {
              values.append(Rational::from_double(
                  context.budget, needed(forest).sample(a).xyz[descriptor.line_axis])),
              values.append(Rational::from_double(
                  context.budget, needed(forest).sample(z).xyz[descriptor.line_axis]))};
          map.original_t = row.original_t;
          map.traversal = edge.parameter_range[1] > edge.parameter_range[0] ? 1 : -1;
          row.image_map = map;
        }
        rows.push_back(row);
      }
    }
    auto pool = std::allocate_shared<PhysicalCurveBoundsData>(
        RunAllocator<PhysicalCurveBoundsData>(resource), resource, snapshot, values.seal());
    pool->intervals = std::move(rows);
    physical = std::move(pool);
  }
  struct KnotSelection {
    uint32_t axis, position;
  };
  double expanded_knot(const Carrier &carrier, KnotSelection selection) const {
    const auto axis = selection.axis, position = selection.position;
    const auto knots = source.view().view(axis == 0 ? carrier.knot_u : carrier.knot_v);
    const auto multiplicities = source.view().view(axis == 0 ? carrier.mult_u : carrier.mult_v);
    uint64_t end = 0;
    for (uint32_t i = 0; i < knots.size(); ++i) {
      end += multiplicities[i];
      if (position < end)
        return knots[i];
    }
    throw Error(ErrorCode::invalid_source, "carrier natural knot domain is incomplete");
  }
  std::pmr::vector<CarrierRequest> trace_carrier_requests() {
    struct GuideBox {
      bool ready = false;
      Interval axes[2]{};
    };
    std::pmr::vector<GuideBox> guides(source.view().curves.size(), GuideBox{}, &budget);
    // One original-root walk establishes all guide envelopes. No carrier/use
    // performs a global root scan, and adapter rational UV hints do not cut out
    // source spans needed by later trace work.
    for (const auto &root : roots->original_roots()) {
      if (root.dimension != 2)
        continue;
      budget.charge(uint64_t{root.degree + 1} * 16);
      Interval box[2]{};
      if (root.family == CurveFamily::rational) {
        for (uint32_t i = 0; i <= root.degree; ++i) {
          const auto base = root.controls.first + i * 3;
          for (uint32_t a = 0; a < 2; ++a) {
            const auto value =
                numeric::div(roots->controls()[base + a], roots->controls()[base + 2]);
            require(numeric::valid(value), ErrorCode::unresolved, "pcurve source hull unresolved");
            if (!i)
              box[a] = value;
            else {
              box[a].lower = std::min(box[a].lower, value.lower);
              box[a].upper = std::max(box[a].upper, value.upper);
            }
          }
        }
      } else
        for (uint32_t a = 0; a < 2; ++a) {
          if (root.family == CurveFamily::line)
            box[a] =
                numeric::add(numeric::point(root.frame[a]),
                             numeric::scale({root.original_t[0].lower, root.original_t[1].upper},
                                            root.frame[3 + a]));
          else {
            const auto radius =
                numeric::add(
                    numeric::point(numeric::magnitude(numeric::mul(
                        numeric::point(root.radii[0]), numeric::point(root.frame[3 + a])))),
                    numeric::point(numeric::magnitude(numeric::mul(
                        numeric::point(root.radii[1]), numeric::point(root.frame[6 + a])))))
                    .upper;
            box[a] = numeric::add(numeric::point(root.frame[a]), {-radius, radius});
          }
        }
      auto &guide = guides[root.source_slot];
      for (uint32_t a = 0; a < 2; ++a) {
        require(numeric::valid(box[a]), ErrorCode::unresolved, "nonfinite original pcurve hull");
        if (!guide.ready)
          guide.axes[a] = box[a];
        else {
          guide.axes[a].lower = std::min(guide.axes[a].lower, box[a].lower);
          guide.axes[a].upper = std::max(guide.axes[a].upper, box[a].upper);
        }
      }
      guide.ready = true;
    }
    std::pmr::vector<CarrierRequest> selected(&budget);
    for (auto index : source.view().definition_faces.at(definition.value)) {
      const auto &face = source.view().faces[index];
      const auto slot = source.view().carrier_index.find(face.carrier_id);
      const auto &carrier = source.view().carriers[slot.value];
      auto domain = carrier.uv_bounds;
      if (carrier.kind == 6)
        for (uint32_t axis = 0; axis < 2; ++axis) {
          domain[2 * axis] = expanded_knot(carrier, {axis, carrier.degrees[axis]});
          domain[2 * axis + 1] = expanded_knot(carrier, {axis, carrier.control_shape[axis]});
        }
      else
        for (uint64_t li = 0; li < face.loops.count; ++li) {
          const auto &loop =
              source.view()
                  .loops[source.view()
                             .loop_index.find(source.view().loop_ids[face.loops.first + li])
                             .value];
          for (uint64_t ui = 0; ui < loop.coedges.count; ++ui) {
            const auto &use =
                source.view().coedges[source.view()
                                          .coedge_index
                                          .find(source.view().coedge_ids[loop.coedges.first + ui])
                                          .value];
            const auto curve = source.view().curve_index.find(use.curve_id);
            const auto &guide = guides[curve.value];
            require(guide.ready, ErrorCode::unsupported,
                    "incident guide has no prepared curve source envelope");
            for (uint32_t axis = 0; axis < 2; ++axis) {
              auto box = guide.axes[axis];
              if (use.periodic_shift[axis]) {
                require(trace_axis_is_periodic(carrier, axis), ErrorCode::unsupported,
                        "undeclared periodic trace axis");
                box = numeric::add(
                    box, numeric::scale(numeric::half_pi(),
                                        4. * static_cast<double>(use.periodic_shift[axis])));
              }
              domain[2 * axis] = std::min(domain[2 * axis], box.lower);
              domain[2 * axis + 1] = std::max(domain[2 * axis + 1], box.upper);
            }
          }
        }
      selected.push_back({slot, domain});
    }
    std::sort(selected.begin(), selected.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a.source < b.source;
    });
    uint32_t used = 0;
    for (const auto &row : selected) {
      if (used && selected[used - 1].source == row.source) {
        for (uint32_t axis = 0; axis < 2; ++axis) {
          selected[used - 1].domain[2 * axis] =
              std::min(selected[used - 1].domain[2 * axis], row.domain[2 * axis]);
          selected[used - 1].domain[2 * axis + 1] =
              std::max(selected[used - 1].domain[2 * axis + 1], row.domain[2 * axis + 1]);
        }
      } else
        selected[used++] = row;
    }
    selected.resize(used);
    return selected;
  }
  void prepare_traces(JobContext &context) {
    if (!carriers) {
      auto selected = trace_carrier_requests();
      if (!selected.empty())
        carriers = prepare_trace_source_cache(
            source, selected, resource, {},
            {limits.nodes, limits.control_scalars, limits.total_work}, context.budget);
    }
    trace_sources.clear();
    trace_site_work.clear();
    trace_interval_work.clear();
    trace_ranges.clear();
    active_faces.clear();
    active_face_indices.clear();
    std::pmr::vector<uint32_t> ids(&budget);
    evaluation_ids.clear();
    uint32_t face_index = 0;
    for (auto index : source.view().definition_faces.at(definition.value)) {
      const auto current_index = face_index++;
      if (trace_results[current_index] &&
          trace_identity(trace_results[current_index]).boundary_generation ==
              needed(forest).face_generation(Slot<FaceTag>{index}))
        continue;
      Slot<FaceTag> face{index};
      const auto quality = face_allowance(face);
      const auto generation = needed(forest).face_generation(face);
      require(generation < UINT64_MAX, ErrorCode::resource_exhausted,
              "trace face generation exhausted");
      TraceFaceRequest input{resource,
                             source,
                             roots,
                             needed(carriers),
                             snapshot,
                             physical,
                             face,
                             {query, generation + 1, generation},
                             needed(request).values,
                             quality.positional,
                             quality.node_source,
                             table_key(5)};
      auto work = prepare_use_trace(input, context);
      const auto first = slot(evaluation_ids.size());
      for (const auto &task : trace_curve_samples(work)) {
        auto t = import_exact(budget, trace_values(work).view(task.parameter));
        const auto id = cache.point(task.curve, t);
        evaluation_ids.push_back(id);
        ids.push_back(id);
      }
      trace_ranges.push_back({first, evaluation_ids.size() - first});
      trace_sources.push_back(std::move(work));
      active_faces.push_back(face);
      active_face_indices.push_back(current_index);
    }
    queue_points(ids, context);
    phase = Phase::trace_source_fold;
  }
  void prepare_carriers(JobContext &context) {
    carrier_tasks.clear();
    trace_site_work.clear();
    std::pmr::vector<Range<uint32_t>> ranges(&budget);
    for (uint32_t f = 0; f < trace_sources.size(); ++f) {
      std::pmr::vector<CurveSampleResult> answers(&budget);
      for (uint64_t i = 0; i < trace_ranges[f].count; ++i)
        answers.push_back(point(evaluation_ids[trace_ranges[f].first + i]));
      auto work = prepare_trace_sites(trace_sources[f], trace_completion_key(trace_sources[f]),
                                      answers, context);
      const auto first = slot(carrier_tasks.size());
      for (const auto &task : trace_carrier_samples(work)) {
        std::array<Rational, 2> uv{
            import_exact(context.budget, trace_values(work).view(task.uv[0])),
            import_exact(context.budget, trace_values(work).view(task.uv[1]))};
        OriginalRootSlot root;
        try {
          root = find_original_carrier_root(needed(carriers), task.carrier, uv);
        } catch (const Error &error) {
          if (error.code != ErrorCode::invalid_source)
            throw;
          throw Error(ErrorCode::unsupported,
                      "chosen UV lies outside the finite prepared trace carrier profile");
        }
        carrier_tasks.push_back({root, trace_values(work), task.uv});
      }
      ranges.push_back({first, carrier_tasks.size() - first});
      trace_site_work.push_back(std::move(work));
    }
    trace_ranges = std::move(ranges);
    carrier_answers.resize(carrier_tasks.size());
    submit(Operation::carriers, slot(carrier_tasks.size()), context);
    phase = Phase::trace_carrier_fold;
  }
  void prepare_trace_bounds(JobContext &context) {
    trace_interval_work.clear();
    trace_tasks.clear();
    trace_controls.clear();
    std::pmr::vector<Range<uint32_t>> ranges(&budget);
    for (uint32_t f = 0; f < trace_site_work.size(); ++f) {
      const auto range = trace_ranges[f];
      auto sites = complete_trace_sites(
          trace_site_work[f], trace_completion_key(trace_site_work[f]),
          std::span<const SampleResult>(carrier_answers).subspan(range.first, range.count),
          context);
      auto work = prepare_trace_intervals(sites, context);
      const auto first = slot(trace_tasks.size());
      const auto offset = slot(trace_controls.size());
      const auto net = trace_bound_controls(work);
      require(net.size() <= limits.control_scalars - trace_controls.size(),
              ErrorCode::resource_exhausted, "trace net horizon capacity");
      trace_controls.insert(trace_controls.end(), net.begin(), net.end());
      for (const auto &task : trace_bound_tasks(work)) {
        auto input = task.bound;
        require(input.input.controls.first <= UINT32_MAX - offset, ErrorCode::resource_exhausted,
                "trace control offset overflow");
        input.input.controls.first += offset;
        trace_tasks.push_back(input);
      }
      ranges.push_back({first, trace_tasks.size() - first});
      trace_interval_work.push_back(std::move(work));
    }
    trace_ranges = std::move(ranges);
    trace_answers.resize(trace_tasks.size());
    counts.trace_tests += trace_tasks.size();
    if (device)
      counts.device_bounds += trace_tasks.size();
    submit(Operation::traces, slot(trace_tasks.size()), context);
    phase = Phase::trace_bound_fold;
  }
  void fold_traces(JobContext &context) {
    for (uint32_t f = 0; f < trace_interval_work.size(); ++f) {
      const auto range = trace_ranges[f];
      auto result = complete_trace_intervals(
          trace_interval_work[f], trace_completion_key(trace_interval_work[f]),
          std::span<const CurveNodeBounds>(trace_answers).subspan(range.first, range.count),
          context);
      const auto fallbacks = trace_exact_fallbacks(result);
      require(fallbacks <= UINT64_MAX - counts.exact_fallbacks, ErrorCode::resource_exhausted,
              "curve fallback counter overflow");
      counts.exact_fallbacks += fallbacks;
      const auto wanted = trace_requests(result);
      require(wanted.size() <= limits.requests - requests.size(), ErrorCode::resource_exhausted,
              "trace request horizon capacity");
      requests.insert(requests.end(), wanted.begin(), wanted.end());
      trace_results[active_face_indices[f]] = std::move(result);
    }
  }
  void freeze(JobContext &context) {
    ExactBuilder values(resource, table_key());
    auto copy = [&](ExactRef ref, const ExactTable &from) {
      return values.append(import_exact(context.budget, from.view(ref)));
    };
    auto line = [&](GeometricLineRecord &map, const ExactTable &from) {
      for (auto &ref : map.covector)
        ref = copy(ref, from);
      for (auto &ref : map.curve_scalar)
        ref = copy(ref, from);
      for (auto &ref : map.trace_scalar)
        ref = copy(ref, from);
      for (auto &ref : map.original_t)
        ref = copy(ref, from);
    };
    std::pmr::vector<PhysicalCurveInterval> physical_rows(resource.get());
    for (auto row : physical->intervals) {
      for (auto &ref : row.original_t)
        ref = copy(ref, physical->values);
      row.parameter_chord_upper = copy(row.parameter_chord_upper, physical->values);
      if (row.image_chord_upper)
        row.image_chord_upper = copy(*row.image_chord_upper, physical->values);
      if (row.image_map)
        line(*row.image_map, physical->values);
      physical_rows.push_back(row);
    }
    std::pmr::vector<UseCurveTrace> use_rows(resource.get());
    std::pmr::vector<UseCurveSite> site_rows(resource.get());
    std::pmr::vector<UseCurveInterval> interval_rows(resource.get());
    for (const auto &result : trace_results) {
      require(result != nullptr, ErrorCode::internal_error, "missing original face trace result");
      const auto &from = trace_values(result);
      std::pmr::vector<uint32_t> remapped_nodes(trace_sites(result).size(), invalid_slot, &budget);
      for (const auto &use : trace_uses(result)) {
        const auto &original = source.view().coedges[use.use.value];
        const auto edge = source.view().edge_index.find(original.edge_id);
        const auto at = std::lower_bound(snapshot->edges.begin(), snapshot->edges.end(), edge,
                                         [](const auto &row, auto key) { return row.edge < key; });
        require(at != snapshot->edges.end() && at->edge == edge &&
                    at->nodes.count == use.sites.count,
                ErrorCode::internal_error, "retained face trace no longer matches physical chain");
        for (uint64_t j = 0; j < use.sites.count; ++j)
          remapped_nodes[use.sites.first + j] =
              slot(at->nodes.first + (original.sense > 0 ? j : use.sites.count - 1 - j));
      }
      uint32_t remap_index = 0;
      const auto site_offset = slot(site_rows.size()), interval_offset = slot(interval_rows.size());
      for (auto row : trace_sites(result)) {
        row.boundary_node = remapped_nodes[remap_index++];
        require(row.boundary_node != invalid_slot, ErrorCode::internal_error,
                "unmapped retained use site");
        row.original_t = copy(row.original_t, from);
        row.mapped_s = copy(row.mapped_s, from);
        for (auto &ref : row.chosen_original_uv)
          ref = copy(ref, from);
        for (auto &ref : row.chart_xy)
          ref = copy(ref, from);
        row.node_source_upper = copy(row.node_source_upper, from);
        site_rows.push_back(row);
      }
      for (auto row : trace_intervals(result)) {
        for (auto &id : row.sites)
          id += site_offset;
        row.carrier_trace_to_actual_chord_upper =
            copy(row.carrier_trace_to_actual_chord_upper, from);
        row.source_to_selected_trace_upper = copy(row.source_to_selected_trace_upper, from);
        for (auto &ref : row.uv_chord_component_upper)
          ref = copy(ref, from);
        if (row.authoritative_domain_upper)
          row.authoritative_domain_upper = copy(*row.authoritative_domain_upper, from);
        if (row.image_map)
          line(*row.image_map, from);
        if (row.cylinder_identity) {
          auto &map = *row.cylinder_identity;
          for (auto &ref : map.original_t)
            ref = copy(ref, from);
          for (auto &ref : map.chosen_uv)
            ref = copy(ref, from);
          map.constant_v = copy(map.constant_v, from);
        }
        interval_rows.push_back(row);
      }
      for (auto row : trace_uses(result)) {
        row.sites.first += site_offset;
        row.intervals.first += interval_offset;
        use_rows.push_back(row);
      }
    }
    auto result = std::allocate_shared<SharedCurveSnapshotData>(
        RunAllocator<SharedCurveSnapshotData>(resource), resource, source, definition, query,
        values.seal(), snapshot);
    result->physical_intervals = std::move(physical_rows);
    result->uses = std::move(use_rows);
    result->sites = std::move(site_rows);
    result->intervals = std::move(interval_rows);
    output = std::move(result);
    phase = Phase::done;
  }
  void prepare_evaluations(JobContext &context) {
    evaluation = needed(forest).pending_evaluations();
    evaluation_ids.clear();
    if (!evaluation) {
      phase = Phase::prepare;
      return;
    }
    require(needed(evaluation).tasks().size() <= limits.frontier, ErrorCode::resource_exhausted,
            "curve evaluation horizon capacity");
    for (const auto &task : needed(evaluation).tasks()) {
      auto t = import_exact(budget, needed(evaluation).parameters().view(task.parameter));
      evaluation_ids.push_back(cache.point(task.curve, t));
    }
    queue_points(evaluation_ids, context);
    phase = Phase::evaluation_fold;
  }
  void finish_evaluations() {
    std::pmr::vector<BoundaryEvaluationResult> rows(&budget);
    rows.reserve(evaluation_ids.size());
    for (uint32_t i = 0; i < evaluation_ids.size(); ++i) {
      const auto &p = point(evaluation_ids[i]);
      rows.push_back(
          {needed(evaluation).tasks()[i], 3, {p.actual_xyz[0], p.actual_xyz[1], p.actual_xyz[2]}});
    }
    auto update = needed(forest).complete_evaluations(needed(evaluation).epoch(), rows);
    counts.installed_splits += update.removed_intervals.size();
    counts.invalidated_faces += update.changed_faces.size();
    restriction_tasks.clear();
    restriction_parents.clear();
    std::pmr::vector<uint8_t> seen_parents(cache.nodes.size(), uint8_t{0}, &budget);
    for (uint32_t i = 0; i < update.removed_intervals.size(); ++i) {
      const auto old = update.removed_intervals[i];
      require(old.interval < interval_nodes.size() && interval_nodes[old.interval] != invalid_slot,
              ErrorCode::internal_error, "split interval missing numerical owner");
      const auto parent = interval_nodes[old.interval];
      const auto children = cache.children(parent);
      const auto interval = needed(forest).interval(old);
      const bool forward = needed(forest)
                               .parameter(interval.samples[0])
                               .compare(needed(forest).parameter(interval.samples[1])) < 0;
      for (uint32_t child = 0; child < 2; ++child) {
        const auto key = update.inserted_intervals[2 * i + child];
        if (key.interval >= interval_nodes.size())
          interval_nodes.resize(key.interval + 1, invalid_slot);
        interval_nodes[key.interval] = children[forward ? child : 1 - child];
      }
      if (!cache.nodes[children[0]].ready && !seen_parents[parent]) {
        seen_parents[parent] = 1;
        const auto &node = cache.nodes[parent];
        restriction_parents.push_back(parent);
        restriction_tasks.push_back({node.root, node.controls,
                                     cache.nodes[children[0]].controls.first,
                                     cache.nodes[children[1]].controls.first});
      }
    }
    evaluation.reset();
    phase = restriction_tasks.empty() ? Phase::prepare : Phase::restrictions;
  }
  void commit() {
    std::sort(requests.begin(), requests.end());
    requests.erase(std::unique(requests.begin(), requests.end()), requests.end());
    require(requests.size() <= limits.requests, ErrorCode::resource_exhausted, "curve request cap");
    std::pmr::vector<uint32_t> parents(&budget);
    for (auto key : requests) {
      const auto id = interval_nodes[key.interval];
      require(cache.nodes[id].depth < limits.max_depth, ErrorCode::unresolved,
              "shared curve split depth exhausted");
      parents.push_back(id);
    }
    std::sort(parents.begin(), parents.end());
    parents.erase(std::unique(parents.begin(), parents.end()), parents.end());
    uint64_t missing_nodes = 0, missing_controls = 0;
    struct NewPoint {
      Slot<CurveTag> curve;
      Rational parameter;
    };
    std::pmr::vector<NewPoint> candidates(&budget);
    for (auto id : parents) {
      const auto &node = cache.nodes[id];
      if (node.children[0] == invalid_slot) {
        missing_nodes += 2;
        missing_controls += uint64_t{node.controls.count} * 2;
      }
      auto t = node.begin.add(node.end).divide(Rational(budget, 2));
      const Slot<CurveTag> curve{roots->roots()[node.root.value].source_slot};
      if (cache.find_point(curve, t) == invalid_slot)
        candidates.push_back({curve, std::move(t)});
    }
    std::sort(candidates.begin(), candidates.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a.curve != b.curve ? a.curve < b.curve : a.parameter.compare(b.parameter) < 0;
    });
    candidates.erase(std::unique(candidates.begin(), candidates.end(),
                                 [](const auto &a, const auto &b) {
                                   return a.curve == b.curve &&
                                          same_scalar(a.parameter, b.parameter);
                                 }),
                     candidates.end());
    require(missing_nodes <= limits.nodes - cache.nodes.size() &&
                missing_controls <= limits.control_scalars - cache.controls.size(),
            ErrorCode::resource_exhausted, "shared curve whole-horizon child capacity");
    require(candidates.size() <= limits.points - cache.points.size(), ErrorCode::resource_exhausted,
            "shared curve whole-horizon sample capacity");
    // All numerical rows/child slabs are reserved before the physical owner
    // starts its transaction. Shared source parents emit one restriction pair.
    for (const auto &point : candidates)
      (void)cache.point(point.curve, point.parameter);
    for (auto id : parents)
      (void)cache.children(id);
    // Physical failures and occurrence failures are folded before the sole
    // forest transaction. Owner requests also represent wires without fake faces.
    const auto epoch = needed(forest).begin_epoch();
    ++counts.epochs;
    counts.requested_splits += requests.size();
    needed(forest).request_owner_splits(epoch, requests);
    needed(forest).stage_owner_promotions(epoch, promotion_rows);
    for (auto f : source.view().definition_faces.at(definition.value))
      needed(forest).arrive(epoch, Slot<FaceTag>{f},
                            needed(forest).face_generation(Slot<FaceTag>{f}), {});
    require(needed(forest).horizon_complete(epoch), ErrorCode::internal_error,
            "incomplete shared curve horizon");
    evaluation = needed(forest).prepare_splits(epoch);
    phase = Phase::evaluate;
  }
  StepState advance(JobContext &context) {
    stopped();
    switch (phase) {
    case Phase::idle:
    case Phase::closed:
    case Phase::failed:
      throw Error(ErrorCode::invalid_source, "shared curve query is not active");
    case Phase::evaluate:
      prepare_evaluations(context);
      break;
    case Phase::evaluation_fold:
      if (!ready(sample_answers.data(), slot(sample_answers.size())))
        break;
      fold_points(context);
      finish_evaluations();
      break;
    case Phase::restrictions:
      if (operation == Operation::none) {
        counts.control_restrictions += restriction_tasks.size();
        if (device)
          counts.device_restrictions += restriction_tasks.size();
        submit(Operation::restrictions, slot(restriction_tasks.size()), context);
        break;
      }
      if (!ready(nullptr, 0))
        break;
      for (auto parent : restriction_parents)
        for (auto child : cache.nodes[parent].children)
          cache.nodes[child].ready = true;
      phase = Phase::prepare;
      break;
    case Phase::prepare:
      prepare_horizon(context);
      break;
    case Phase::nodes:
      if (operation == Operation::samples) {
        if (!ready(sample_answers.data(), slot(sample_answers.size())))
          break;
        fold_points(context);
      }
      if (device) {
        bool missing = false;
        for (auto id : evaluation_ids)
          if (cache.device_stage[id] != cache.point_stage[id])
            missing = true;
        if (missing) {
          queue_points(evaluation_ids, context);
          break;
        }
      }
      check_source_guards(context);
      if (!install_ready())
        break;
      prepare_nodes(context);
      break;
    case Phase::node_fold:
      if (!ready(node_answers.data(), slot(node_answers.size())))
        break;
      fold_nodes(context);
      if (!requests.empty() || !promotion_points.empty()) {
        phase = Phase::commit;
        break;
      }
      build_physical(context);
      phase = Phase::trace_source;
      break;
    case Phase::trace_source:
      prepare_traces(context);
      break;
    case Phase::trace_source_fold:
      if (!ready(sample_answers.data(), slot(sample_answers.size())))
        break;
      fold_points(context);
      prepare_carriers(context);
      break;
    case Phase::trace_carrier_fold:
      if (!ready(nullptr, 0))
        break;
      prepare_trace_bounds(context);
      break;
    case Phase::trace_bound_fold:
      if (!ready(trace_answers.data(), slot(trace_answers.size())))
        break;
      fold_traces(context);
      if (requests.empty())
        freeze(context);
      else {
        promotion_rows.clear();
        phase = Phase::commit;
      }
      break;
    case Phase::commit:
      commit();
      break;
    case Phase::done:
      return StepState::done;
    }
    return phase == Phase::done ? StepState::done : StepState::ready;
  }
};
AdaptiveBoundarySession::AdaptiveBoundarySession(std::shared_ptr<RunResource> resource,
                                                 SourceStore source, Slot<DefinitionTag> definition,
                                                 CurveSourceCache roots, CurveLimits limits,
                                                 const std::stop_token &token)
    : resource_(std::move(resource)), impl_(nullptr) {
  require(resource_ && source.view().resource == resource_ &&
              &roots.source().view() == &source.view() &&
              definition.value < source.view().definitions.size(),
          ErrorCode::invalid_source, "shared curve source/resource/definition mismatch");
  auto *memory = resource_->allocate(sizeof(Impl), alignof(Impl));
  try {
    impl_ = new (memory)
        Impl(resource_, std::move(source), definition, std::move(roots), limits, token);
  } catch (...) {
    resource_->deallocate(memory, sizeof(Impl), alignof(Impl));
    throw;
  }
}
AdaptiveBoundarySession::~AdaptiveBoundarySession() {
  if (!impl_)
    return;
  try {
    close();
  } catch (...) {
    if (impl_->device) {
      detail::retain_poison(impl_->device);
      impl_->device = nullptr;
    }
  }
  impl_->~Impl();
  resource_->deallocate(impl_, sizeof(Impl), alignof(Impl));
}
void AdaptiveBoundarySession::open_device(const CacheDevice &request) {
  auto &p = *impl_;
  p.stopped();
  require(p.phase == Impl::Phase::idle && !p.query && !p.device, ErrorCode::invalid_source,
          "curve device must open before queries");
  require(!p.roots->roots().empty(), ErrorCode::unsupported,
          "empty curve source bank has no device operation");
  auto *memory = p.resource->allocate(sizeof(CurveOwner), alignof(CurveOwner));
  p.device = new (memory) CurveOwner(p.resource);
  auto &owner = *p.device;
  owner.module = dlopen(request.module.c_str(), RTLD_NOW | RTLD_LOCAL);
  require(owner.module, ErrorCode::device_failure, "cannot load shared curve CUDA module");
  using Factory = const CurveCudaAPI *(*)();
  auto factory = reinterpret_cast<Factory>(dlsym(owner.module, "brepgpu_adaptive_curve_cuda_v1"));
  require(factory, ErrorCode::device_failure, "curve CUDA factory missing");
  owner.api = factory();
  require(owner.api && owner.api->abi == curve_cuda_abi && owner.api->size == sizeof(CurveCudaAPI),
          ErrorCode::device_failure, "curve CUDA ABI mismatch");
  DeviceConfig config{};
  config.abi = adaptive_cuda_abi;
  config.size = sizeof(config);
  config.device = request.device;
  config.points = p.limits.points;
  config.nodes = p.limits.nodes;
  config.controls = p.limits.control_scalars;
  config.frontier = p.limits.frontier;
  config.host_bytes = p.limits.host_bytes;
  config.device_bytes = p.limits.device_bytes;
  config.work = p.limits.total_work;
  config.reservation_context = &owner;
  config.reserve = detail::reserve_external<CurveCudaAPI>;
  config.release = [](auto *context, auto *token) noexcept {
    detail::release_external(*static_cast<CurveOwner *>(context),
                             static_cast<detail::ExternalLease *>(token));
  };
  DeviceError error{};
  detail::device_check(
      owner.api->create(&config, p.roots->roots().data(), slot(p.roots->roots().size()),
                        p.roots->original_roots().data(), slot(p.roots->original_roots().size()),
                        p.cache.controls.data(), slot(p.cache.controls.size()), &owner.session,
                        &error),
      error);
}
void AdaptiveBoundarySession::begin(const CurveBoundaryQuery &input) {
  auto &p = *impl_;
  p.stopped();
  require(p.phase == Impl::Phase::idle && !p.output, ErrorCode::invalid_source,
          "shared curve query already active");
  require(input.generation > p.query && input.values.resource() == p.resource,
          ErrorCode::invalid_source, "stale curve query or foreign exact resource");
  auto positive = [&](ExactRef ref) {
    require(import_exact(p.budget, input.values.view(ref)).sign() > 0, ErrorCode::invalid_source,
            "nonpositive curve allowance");
  };
  positive(input.default_positional);
  positive(input.default_node_source);
  p.overrides.assign(input.overrides.begin(), input.overrides.end());
  std::sort(p.overrides.begin(), p.overrides.end(),
            [](const auto &a, const auto &b) { return a.face < b.face; });
  for (uint32_t i = 0; i < p.overrides.size(); ++i) {
    const auto &entry = p.overrides[i];
    require(entry.face.value < p.source.view().faces.size() &&
                p.source.view().faces[entry.face.value].definition_id ==
                    p.source.view().definitions[p.definition.value].id &&
                (!i || p.overrides[i - 1].face != entry.face),
            ErrorCode::invalid_source, "foreign or repeated curve face allowance");
    positive(entry.positional);
    positive(entry.node_source);
  }
  p.source_guards_done = false;
  p.query = input.generation;
  p.request = CurveBoundaryQuery{input.generation, input.values, input.default_positional,
                                 input.default_node_source, p.overrides};
  p.ordinal = 0;
  ++p.counts.queries;
  std::fill(p.cache.point_stage.begin(), p.cache.point_stage.end(), 0);
  p.interval_nodes.clear();
  p.snapshot.reset();
  p.physical.reset();
  p.requests.clear();
  p.promotion_points.clear();
  p.promotion_rows.clear();
  p.trace_results.clear();
  p.trace_results.resize(p.source.view().definition_faces.at(p.definition.value).size());
  p.trace_sources.clear();
  p.trace_site_work.clear();
  p.trace_interval_work.clear();
  p.forest.emplace(p.budget, p.source, p.definition, p.resource,
                   OwnerKey{OwnerKind::run, UINT32_MAX - 5, forest_tables.fetch_add(1)});
  p.phase = Impl::Phase::evaluate;
}
StepState AdaptiveBoundarySession::step(JobContext &context) {
  try {
    return impl_->advance(context);
  } catch (...) {
    impl_->phase = Impl::Phase::failed;
    throw;
  }
}
SharedCurveSnapshot AdaptiveBoundarySession::result() const {
  require(impl_->phase == Impl::Phase::done && impl_->output, ErrorCode::invalid_source,
          "shared curve result not ready");
  return impl_->output;
}
CurveStats AdaptiveBoundarySession::stats() const noexcept {
  auto result = impl_->counts;
  result.host_reserved_bytes = impl_->budget.live();
  result.host_peak_upper_bytes = impl_->budget.peak();
  if (impl_->device && impl_->device->api && impl_->device->session) {
    DeviceStats actual{};
    DeviceError error{};
    if (!impl_->device->api->stats(impl_->device->session, &actual, &error)) {
      result.uploaded_bytes = actual.uploaded_bytes;
      result.downloaded_bytes = actual.downloaded_bytes;
      result.host_reserved_bytes += actual.host_reserved_bytes;
      result.host_peak_upper_bytes += actual.host_peak_bytes;
      result.device_reserved_bytes = actual.device_reserved_bytes;
      result.device_peak_bytes = actual.device_peak_bytes;
    }
  }
  return result;
}
void AdaptiveBoundarySession::release_result() {
  auto &p = *impl_;
  require(p.phase == Impl::Phase::done, ErrorCode::invalid_source,
          "shared curve result not complete");
  p.output.reset();
  p.forest.reset();
  p.snapshot.reset();
  p.physical.reset();
  p.request.reset();
  p.trace_sources.clear();
  p.trace_site_work.clear();
  p.trace_interval_work.clear();
  p.trace_results.clear();
  p.phase = Impl::Phase::idle;
}
void AdaptiveBoundarySession::cancel() noexcept {
  impl_->cancellation.request_stop();
  if (impl_->workers)
    impl_->workers->stop();
}
void AdaptiveBoundarySession::close() {
  auto &p = *impl_;
  if (p.workers) {
    p.workers->stop();
    p.workers->drain();
    p.workers.reset();
    p.jobs = 0;
  }
  if (p.device) {
    auto *owner = p.device;
    if (owner->session && owner->api) {
      DeviceError error{};
      detail::device_check(owner->api->destroy(owner->session, &error), error);
      owner->session = nullptr;
    }
    require(!owner->leases, ErrorCode::device_failure,
            "shared curve external reservations remain after close");
    if (owner->module)
      dlclose(owner->module);
    owner->~CurveOwner();
    p.resource->deallocate(owner, sizeof(CurveOwner), alignof(CurveOwner));
    p.device = nullptr;
  }
  p.phase = Impl::Phase::closed;
}
} // namespace bgmesh::adaptive
