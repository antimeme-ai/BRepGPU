#ifndef BREPGPU_MESHER_ADAPTIVE_CACHE_DETAIL_HPP
#define BREPGPU_MESHER_ADAPTIVE_CACHE_DETAIL_HPP
#include "adaptive_cache.hpp"
#include "adaptive_device_owner.hpp"
#include "adaptive_geometry.hpp"
#include "adaptive_numeric.hpp"
#include "cuda/adaptive_api.h"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <dlfcn.h>
#include <limits>
#include <optional>
#include <tuple>
#include <unordered_map>

namespace bgmesh::adaptive::detail::surface {
inline void check(bool ok, ErrorCode code, const char *message) {
  if (!ok)
    throw Error(code, message);
}
inline uint32_t narrow(std::size_t n) {
  check(n < invalid_slot, ErrorCode::resource_exhausted, "adaptive slot overflow");
  return static_cast<uint32_t>(n);
}
struct Dyadic {
  uint64_t numerator = 0;
  uint32_t exponent = 0;
  bool operator==(const Dyadic &) const = default;
};
inline Dyadic midpoint(Dyadic a, Dyadic b) {
  const auto depth = std::max(a.exponent, b.exponent);
  check(depth < 64, ErrorCode::unresolved, "adaptive dyadic64 depth exhausted");
  const auto x = a.numerator << (depth - a.exponent), y = b.numerator << (depth - b.exponent);
  check(x < y && x <= UINT64_MAX - y, ErrorCode::internal_error, "invalid dyadic cell interval");
  Dyadic r{x + y, depth + 1};
  while (r.exponent && !(r.numerator & 1u)) {
    r.numerator >>= 1;
    --r.exponent;
  }
  return r;
}
inline Interval parameter(Interval a, Interval b, Dyadic t) {
  if (t.numerator == 0)
    return a;
  if (t.exponent == 0 && t.numerator == 1)
    return b;
  const double n = static_cast<double>(t.numerator);
  Interval value{n, n};
  if (t.numerator > (uint64_t{1} << 53))
    value = {numeric::down(n), numeric::up(n)};
  const double scale = std::ldexp(1., -static_cast<int>(t.exponent));
  auto result = numeric::add(a, numeric::mul(numeric::sub(b, a), numeric::scale(value, scale)));
  // Exact root-relative t lies in [0,1]; intersect with that known domain.
  result.lower = std::max(result.lower, a.lower);
  result.upper = std::min(result.upper, b.upper);
  return result;
}
inline Rational exact_parameter(bgfd::Budget &b, double a, double z, Dyadic t) {
  auto left = Rational::from_double(b, a);
  if (!t.numerator)
    return left;
  if (!t.exponent)
    return Rational::from_double(b, z);
  const std::array<uint32_t, 2> words{static_cast<uint32_t>(t.numerator),
                                      static_cast<uint32_t>(t.numerator >> 32)};
  const std::array<uint32_t, 1> one{1};
  auto fraction = Rational::from_ratio(
      bgfd::Integer::from_words(b, 1, std::span<const uint32_t>(words).first(words[1] ? 2u : 1u)),
      bgfd::Integer::from_words(b, 1, one).shift_left(t.exponent));
  return left.add(Rational::from_double(b, z).subtract(left).multiply(fraction));
}
struct AxisKey {
  uint32_t endpoint = 0, span = 0;
  uint64_t first = 0, last = 0, numerator = 0;
  uint32_t exponent = 0;
  auto operator<=>(const AxisKey &) const = default;
};
struct SampleKey {
  uint32_t source;
  AxisKey u, v;
  auto operator<=>(const SampleKey &) const = default;
};
inline uint64_t mix(uint64_t h, uint64_t v) {
  v ^= v >> 30;
  v *= 0xbf58476d1ce4e5b9ULL;
  v ^= v >> 27;
  v *= 0x94d049bb133111ebULL;
  return (h ^ (v ^ (v >> 31))) * 1099511628211ULL;
}
struct SampleHash {
  std::size_t operator()(const SampleKey &k) const noexcept {
    uint64_t h = mix(0, k.source);
    for (const auto *a : {&k.u, &k.v}) {
      h = mix(h, a->endpoint);
      h = mix(h, a->span);
      h = mix(h, a->first);
      h = mix(h, a->last);
      h = mix(h, a->numerator);
      h = mix(h, a->exponent);
    }
    return static_cast<std::size_t>(h);
  }
};
inline AxisKey axis_key(const RootDescriptor &r, uint32_t axis, Dyadic t) {
  const auto a = r.prepared_box[2 * axis].lower, z = r.prepared_box[2 * axis + 1].lower;
  if (!t.numerator || !t.exponent) {
    const auto x = !t.numerator ? a : z;
    return {1, 0, std::bit_cast<uint64_t>(x == 0. ? 0. : x), 0, 0, 0};
  }
  return {0,
          axis == 0 ? r.span_u : r.span_v,
          std::bit_cast<uint64_t>(a),
          std::bit_cast<uint64_t>(z),
          t.numerator,
          t.exponent};
}
struct ScalarKey {
  uint32_t source, axis;
  AxisKey parameter;
  bool operator==(const ScalarKey &) const = default;
};
struct ScalarHash {
  std::size_t operator()(const ScalarKey &key) const noexcept {
    SampleKey proxy{key.source, key.parameter, {}};
    return SampleHash{}(proxy) ^ static_cast<std::size_t>(key.axis);
  }
};
struct AbsolutePoint {
  ExactTable values;
  std::array<ExactRef, 2> uv;
};
struct PointData {
  SampleKey key;
  uint32_t prepared;
  Dyadic u, v;
  SampleInput input;
  std::array<SampleResult, 5> tiers{};
  uint32_t valid = 0;
  std::optional<AbsolutePoint> absolute;
};
struct NodeMeta {
  RootSlot root;
  uint32_t parent = invalid_slot, depth_u = 0, depth_v = 0;
  uint32_t step = 0, children[4]{invalid_slot, invalid_slot, invalid_slot, invalid_slot};
  std::array<Dyadic, 4> coordinate;
  std::array<uint32_t, 4> points{};
  ScalarRange controls;
  uint32_t first_bound = invalid_slot;
  uint64_t born_query = 0;
  bool controls_ready = true;
};
struct BoundTier {
  uint32_t signature, next;
  NodeBounds value;
};
using DeviceOwner = bgmesh::adaptive::detail::DeviceOwner<AdaptiveCudaAPI>;
using ExternalLease = bgmesh::adaptive::detail::ExternalLease;
using bgmesh::adaptive::detail::device_check;
using bgmesh::adaptive::detail::release_external;
using bgmesh::adaptive::detail::retain_poison;
inline std::atomic<uint64_t> session_ids{1};
struct SurfaceCacheCore {
  enum class Phase {
    idle,
    prepare,
    samples,
    sample_fold,
    restrict_nodes,
    bounds,
    bound_fold,
    freeze,
    facets,
    facet_fold,
    done,
    closed
  };
  enum class Operation { none, samples, restrictions, nodes, facets };
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  CacheLimits limits;
  std::stop_source cancellation;
  std::stop_token stop;
  struct ForwardStop {
    std::stop_source *target;
    void operator()() const noexcept { target->request_stop(); }
  };
  std::optional<std::stop_callback<ForwardStop>> external_stop;
  std::atomic<bool> cancelled{false};
  bgfd::Budget budget;
  uint64_t serial = session_ids.fetch_add(1), query = 0, ticket = 0;
  uint32_t table_ordinal = 0;
  Phase phase = Phase::idle;
  bool selection_only = false;
  uint64_t cover_revision = 0;
  Operation operation = Operation::none;
  CacheStats counts;
  std::pmr::vector<NodeMeta> nodes;
  std::pmr::vector<Interval> controls;
  std::pmr::vector<PointData> points;
  std::pmr::unordered_map<SampleKey, uint32_t, SampleHash> point_index;
  std::pmr::vector<uint32_t> point_stages, resident_stages, roots, frontier, selected, next,
      frozen_point_ids;
  std::pmr::vector<NodeBounds> active_bounds;
  std::pmr::vector<BoundTier> bound_tiers;
  std::optional<ExactTable> query_values;
  std::pmr::vector<ExactRef> allowances;
  std::pmr::vector<double> tolerance_down;
  std::pmr::vector<DeviceSample> sample_tasks;
  std::pmr::vector<SampleResult> sample_answers;
  std::pmr::vector<DeviceRestriction> restriction_tasks;
  std::pmr::vector<DeviceNode> node_tasks;
  std::pmr::vector<DeviceNodeAnswer> node_answers;
  std::pmr::vector<ActualFacetInput> facet_tasks;
  std::pmr::vector<ActualFacetBounds> facet_answers;
  std::shared_ptr<const PatchSelection> frozen;
  PacketMesh mesh;
  DeviceOwner *device = nullptr;
  std::optional<NativeScheduler> workers;
  uint32_t jobs = 0;
  uint64_t batch_serial = 0;

  SurfaceCacheCore(std::shared_ptr<RunResource> r, SourceCache cache, CacheLimits cap,
                   const std::stop_token &token)
      : resource(std::move(r)), source(std::move(cache)), limits(cap),
        stop(cancellation.get_token()),
        budget({cap.host_bytes, cap.total_work}, resource.get(), stop), nodes(&budget),
        controls(&budget), points(&budget), point_index(&budget), point_stages(&budget),
        resident_stages(&budget), roots(&budget), frontier(&budget), selected(&budget),
        next(&budget), frozen_point_ids(&budget), active_bounds(&budget), bound_tiers(&budget),
        allowances(&budget), tolerance_down(&budget), sample_tasks(&budget),
        sample_answers(&budget), restriction_tasks(&budget), node_tasks(&budget),
        node_answers(&budget), facet_tasks(&budget), facet_answers(&budget) {
    external_stop.emplace(token, ForwardStop{&cancellation});
    check(resource && source.source().view().resource == resource, ErrorCode::internal_error,
          "adaptive source resource mismatch");
    check(source.purpose() == SourceCachePurpose::regular_patch, ErrorCode::invalid_source,
          "trace-only source cache cannot form a regular patch");
    check(cap.nodes && cap.points && cap.frontier && cap.control_scalars && cap.ambiguities &&
              cap.max_axis_depth && cap.max_axis_depth <= 64 && cap.cpu_threads &&
              cap.cpu_threads <= 1024 && cap.child_bytes && cap.child_work && cap.total_work &&
              cap.host_bytes,
          ErrorCode::invalid_source, "invalid adaptive cache limits");
    admit_numeric_environment();
    const auto original = source.controls();
    check(original.size() <= cap.control_scalars, ErrorCode::resource_exhausted,
          "adaptive root control cap");
    controls.assign(original.begin(), original.end());
    if (cap.cpu_threads > 1)
      workers.emplace(resource,
                      SchedulerLimits{cap.cpu_threads, cap.cpu_threads * 4,
                                      std::min<uint64_t>(cap.child_bytes, 65536), cap.child_work});
  }
  ~SurfaceCacheCore() {
    if (workers) {
      workers->stop();
      workers->drain();
      workers.reset();
    }
  }
  void stopped() {
    if (cancelled.load(std::memory_order_relaxed) || stop.stop_requested())
      throw Error(ErrorCode::cancelled, "adaptive patch cancelled");
    budget.charge(0);
  }
  Rational allowance(uint32_t root) {
    if (!query_values)
      throw Error(ErrorCode::internal_error, "adaptive query allowance has no values");
    return import_exact(budget, query_values->view(allowances[root]));
  }
  uint32_t signature(const NodeMeta &n) const {
    uint32_t s = 0;
    for (uint32_t i = 0; i < 4; ++i)
      s |= point_stages[n.points[i]] << (3 * i);
    return s;
  }
  const SampleResult &sample(uint32_t p) const { return points[p].tiers[point_stages[p]]; }
  std::array<Rational, 2> exact_uv(const PointData &p) {
    if (p.absolute)
      return {import_exact(budget, p.absolute->values.view(p.absolute->uv[0])),
              import_exact(budget, p.absolute->values.view(p.absolute->uv[1]))};
    const auto &r = source.roots()[p.prepared];
    return {exact_parameter(budget, r.prepared_box[0].lower, r.prepared_box[1].lower, p.u),
            exact_parameter(budget, r.prepared_box[2].lower, r.prepared_box[3].lower, p.v)};
  }
  std::array<Rational, 4> exact_box(const NodeMeta &n) {
    const auto &r = source.roots()[n.root.value];
    return {
        exact_parameter(budget, r.prepared_box[0].lower, r.prepared_box[1].lower, n.coordinate[0]),
        exact_parameter(budget, r.prepared_box[0].lower, r.prepared_box[1].lower, n.coordinate[1]),
        exact_parameter(budget, r.prepared_box[2].lower, r.prepared_box[3].lower, n.coordinate[2]),
        exact_parameter(budget, r.prepared_box[2].lower, r.prepared_box[3].lower, n.coordinate[3])};
  }
  bool less_node(uint32_t a, uint32_t b) {
    if (nodes[a].root.value != nodes[b].root.value)
      return nodes[a].root.value < nodes[b].root.value;
    std::array<uint32_t, 128> x{}, y{};
    uint32_t nx = 0, ny = 0;
    for (auto i = a; nodes[i].parent != invalid_slot; i = nodes[i].parent)
      x[nx++] = nodes[i].step;
    for (auto i = b; nodes[i].parent != invalid_slot; i = nodes[i].parent)
      y[ny++] = nodes[i].step;
    for (uint32_t i = 0; i < std::min(nx, ny); ++i)
      if (x[nx - 1 - i] != y[ny - 1 - i])
        return x[nx - 1 - i] < y[ny - 1 - i];
    return nx < ny;
  }
  uint32_t point(uint32_t root, Dyadic u, Dyadic v) {
    const auto &r = source.roots()[root];
    const SampleKey key{r.source_slot, axis_key(r, 0, u), axis_key(r, 1, v)};
    budget.charge(1);
    if (const auto found = point_index.find(key); found != point_index.end()) {
      ++counts.point_hits;
      return found->second;
    }
    check(points.size() < limits.points, ErrorCode::resource_exhausted,
          "adaptive point capacity exhausted");
    SampleInput input;
    const uint32_t uc = !u.numerator ? 0u : !u.exponent ? 2u : 1u;
    const uint32_t vc = !v.numerator ? 0u : !v.exponent ? 2u : 1u;
    input.original = r.sample_owners[uc + 3 * vc];
    check(input.original.value < source.original_roots().size(), ErrorCode::internal_error,
          "invalid canonical point owner");
    input.uv[0] = parameter(r.prepared_box[0], r.prepared_box[1], u);
    input.uv[1] = parameter(r.prepared_box[2], r.prepared_box[3], v);
    const auto id = narrow(points.size());
    points.push_back({key, root, u, v, input, {}, {}, {}});
    point_stages.push_back(0);
    resident_stages.push_back(invalid_slot);
    point_index.emplace(key, id);
    return id;
  }
  uint32_t make_node(NodeMeta value) {
    check(nodes.size() < limits.nodes, ErrorCode::resource_exhausted,
          "adaptive node capacity exhausted");
    if (selection_only)
      value.points.fill(invalid_slot);
    else
      value.points = {point(value.root.value, value.coordinate[0], value.coordinate[2]),
                      point(value.root.value, value.coordinate[1], value.coordinate[2]),
                      point(value.root.value, value.coordinate[0], value.coordinate[3]),
                      point(value.root.value, value.coordinate[1], value.coordinate[3])};
    value.born_query = query;
    nodes.push_back(value);
    active_bounds.emplace_back();
    ++counts.nodes_created;
    return narrow(nodes.size() - 1);
  }
  NodePredicate predicate(uint32_t slot) {
    const auto &n = nodes[slot];
    const auto &r = source.roots()[n.root.value];
    NodePredicate p;
    p.root = n.root;
    p.controls = n.controls;
    for (uint32_t i = 0; i < 4; ++i)
      p.box[i] =
          parameter(r.prepared_box[i < 2 ? 0 : 2], r.prepared_box[i < 2 ? 1 : 3], n.coordinate[i]);
    for (uint32_t i = 0; i < 4; ++i) {
      const auto &s = sample(n.points[i]);
      p.corner_departures[i] = s.departure_upper;
      std::copy(std::begin(s.actual_xyz), std::end(s.actual_xyz), p.actual_corners + 3 * i);
    }
    return p;
  }
  uint32_t fair_axis(const NodeMeta &n) const {
    const auto inactive = source.roots()[n.root.value].affine_separable_axes;
    if ((inactive & 1u) && !(inactive & 2u))
      return 1;
    if ((inactive & 2u) && !(inactive & 1u))
      return 0;
    if ((n.depth_u + n.depth_v + 1) % 4)
      return 2;
    return n.depth_u <= n.depth_v ? 0u : 1u;
  }
  void rows(Operation op, uint32_t first, uint32_t end, JobContext &context) {
    for (uint32_t i = first; i < end; ++i) {
      if (cancelled.load(std::memory_order_relaxed) || stop.stop_requested())
        throw Error(ErrorCode::cancelled, "adaptive CPU batch cancelled");
      context.budget.charge(op == Operation::nodes     ? 16384
                            : op == Operation::samples ? 4096
                                                       : 4096);
      if (op == Operation::samples)
        sample_answers[i] = evaluate_sample(source, sample_tasks[i].input);
      else if (op == Operation::restrictions) {
        const auto &task = restriction_tasks[i];
        const auto &r = source.roots()[task.root.value];
        const auto count = (r.degree_u + 1) * (r.degree_v + 1) * 4;
        for (uint32_t k = 0; k < count; ++k)
          controls[task.destination + k] = numeric::restricted_coefficient_rn64(
              r, controls.data() + task.parent.first, task.axis, {task.child, k});
      } else if (op == Operation::nodes) {
        const auto &task = node_tasks[i];
        auto &answer = node_answers[i];
        answer.bounds = task.cached
                            ? task.cached_bounds
                            : bound_node(source, task.input,
                                         std::span<const Interval>(controls).subspan(
                                             task.input.controls.first, task.input.controls.count));
        answer.action =
            answer.bounds.status != NumericStatus::ready ? 2u
            : answer.bounds.regularity_proved && answer.bounds.packet_upper <= task.tolerance_down
                ? 0u
            : source.roots()[task.input.root.value].affine_separable_axes == 3u ? 2u
                                                                                : 1u;
        answer.axis = task.fair_axis < 2 ? static_cast<SplitAxis>(task.fair_axis)
                      : answer.bounds.score_v > answer.bounds.score_u ? SplitAxis::v
                                                                      : SplitAxis::u;
      } else if (op == Operation::facets)
        facet_answers[i] = bound_actual_facet(facet_tasks[i]);
    }
  }
  struct KernelJob final : NativeTask {
    SurfaceCacheCore *owner;
    Operation operation;
    Range<uint32_t> slice;
    KernelJob(NativeTaskKey key, SurfaceCacheCore *p, Operation op, Range<uint32_t> range)
        : NativeTask(key), owner(p), operation(op), slice(range) {}
    StepState step(JobContext &context) override {
      owner->rows(operation, narrow(slice.first), narrow(slice.first + slice.count), context);
      return StepState::done;
    }
  };
  void submit(Operation op, uint32_t count) {
    operation = op;
    if (!count)
      return;
    budget.charge(uint64_t{count} * (op == Operation::nodes ? 16384u : 4096u));
    if (device) {
      DeviceError error{};
      int32_t status = 0;
      if (op == Operation::samples)
        status = device->api->samples(device->session, sample_tasks.data(), count, &ticket, &error);
      else if (op == Operation::restrictions)
        status = device->api->restrict_nodes(device->session, restriction_tasks.data(), count,
                                             &ticket, &error);
      else if (op == Operation::nodes)
        status = device->api->nodes(device->session, node_tasks.data(), count, &ticket, &error);
      else
        status = device->api->facets(device->session, facet_tasks.data(), count, &ticket, &error);
      device_check(status, error);
      return;
    }
    if (!workers) {
      JobContext context({limits.child_bytes, limits.child_work}, resource.get(), stop);
      rows(op, 0, count, context);
      return;
    }
    ++batch_serial;
    const auto number = std::min(count, limits.cpu_threads * 4);
    for (uint32_t i = 0; i < number; ++i) {
      const uint32_t a = static_cast<uint32_t>((uint64_t{count} * i) / number),
                     z = static_cast<uint32_t>((uint64_t{count} * (i + 1)) / number);
      NativeTaskKey key{{OwnerKind::run, UINT32_MAX - 1, uint64_t{i} + 1}, batch_serial, serial};
      workers->submit(std::allocate_shared<KernelJob>(RunAllocator<KernelJob>(resource), key, this,
                                                      op, Range<uint32_t>{a, z - a}));
    }
    jobs = number;
  }
  bool ready(void *output, uint32_t count) {
    if (device && ticket) {
      DeviceError error{};
      uint32_t done = 0;
      device_check(device->api->poll(device->session, ticket, &done, output, count, &error), error);
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
  void prepare_points() {
    sample_tasks.clear();
    sample_answers.clear();
    if (selection_only) {
      std::pmr::vector<SampleKey> missing(&budget);
      for (auto slot : frontier) {
        const auto &node = nodes[slot];
        if (node.points[0] != invalid_slot)
          continue;
        const auto &root = source.roots()[node.root.value];
        for (uint32_t i = 0; i < 4; ++i) {
          SampleKey key{root.source_slot, axis_key(root, 0, node.coordinate[i & 1u]),
                        axis_key(root, 1, node.coordinate[2 + (i >> 1)])};
          if (!point_index.contains(key))
            missing.push_back(key);
        }
      }
      std::sort(missing.begin(), missing.end());
      missing.erase(std::unique(missing.begin(), missing.end()), missing.end());
      check(missing.size() <= limits.points - points.size(), ErrorCode::resource_exhausted,
            "classified surface point horizon exceeds capacity");
      for (auto slot : frontier) {
        auto &node = nodes[slot];
        if (node.points[0] == invalid_slot)
          node.points = {point(node.root.value, node.coordinate[0], node.coordinate[2]),
                         point(node.root.value, node.coordinate[1], node.coordinate[2]),
                         point(node.root.value, node.coordinate[0], node.coordinate[3]),
                         point(node.root.value, node.coordinate[1], node.coordinate[3])};
      }
    }
    std::pmr::vector<uint8_t> seen(points.size(), uint8_t{0}, &budget);
    for (auto slot : frontier)
      for (auto id : nodes[slot].points) {
        if (seen[id])
          continue;
        seen[id] = 1;
        auto &p = points[id];
        const auto stage = point_stages[id];
        const bool hit = (p.valid & (1u << stage)) != 0;
        if (hit)
          ++counts.point_hits;
        // Device caches keep the base tier in ordinary queries; supplied results
        // also reinstall a demanded cached/wider tier without reevaluating it.
        DeviceSample task{};
        task.destination = id;
        task.input = p.input;
        task.input.stage = static_cast<ArithmeticStage>(stage);
        if (hit) {
          if (!device || resident_stages[id] == stage)
            continue;
          task.supplied = 1;
          task.supplied_result = p.tiers[stage];
        }
        sample_tasks.push_back(task);
      }
    check(sample_tasks.size() <= limits.frontier * uint64_t{4}, ErrorCode::resource_exhausted,
          "adaptive sample frontier capacity");
    sample_answers.resize(sample_tasks.size());
    submit(Operation::samples, narrow(sample_tasks.size()));
    phase = Phase::sample_fold;
  }
  void fold_samples() {
    for (uint32_t i = 0; i < sample_tasks.size(); ++i) {
      const auto id = sample_tasks[i].destination;
      auto &p = points[id];
      auto result = sample_answers[i];
      auto stage = point_stages[id];
      if (!sample_tasks[i].supplied) {
        ++counts.point_evaluations;
        if (device)
          ++counts.device_points;
      }
      if (result.status != NumericStatus::ready) {
        auto uv = exact_uv(p);
        for (uint32_t tier = std::max(stage, 1u); tier <= 4; ++tier) {
          if (p.valid & (1u << tier))
            result = p.tiers[tier];
          else {
            ++counts.exact_fallbacks;
            result = evaluate_sample_precise(source, p.input.original, uv,
                                             static_cast<ArithmeticStage>(tier), budget);
            p.tiers[tier] = result;
            p.valid |= 1u << tier;
          }
          stage = tier;
          if (result.status == NumericStatus::ready)
            break;
        }
        check(result.status == NumericStatus::ready, ErrorCode::unresolved,
              "adaptive point precision exhausted");
        point_stages[id] = stage;
      }
      p.tiers[stage] = result;
      p.valid |= 1u << stage;
      if (device && sample_answers[i].status == NumericStatus::ready)
        resident_stages[id] = stage;
      check(std::isfinite(result.departure_upper) && result.departure_upper >= 0,
            ErrorCode::unresolved, "adaptive point departure unavailable");
    }
    // Host-resolved device misses must be installed before node kernels read
    // their point pool. These imports never count as device point evaluation.
    if (device) {
      bool imported = false;
      for (uint32_t i = 0; i < sample_tasks.size(); ++i)
        if (sample_answers[i].status != NumericStatus::ready) {
          const auto id = sample_tasks[i].destination;
          sample_tasks[i].supplied = 1;
          sample_tasks[i].supplied_result = sample(id);
          imported = true;
        }
      if (imported) {
        for (auto &task : sample_tasks) {
          task.supplied = 1;
          task.supplied_result = sample(task.destination);
        }
        submit(Operation::samples, narrow(sample_tasks.size()));
        phase = Phase::bounds;
        return;
      }
    }
    phase = Phase::bounds;
  }
  void prepare_nodes() {
    node_tasks.clear();
    node_answers.clear();
    for (auto slot : frontier) {
      const auto &n = nodes[slot];
      DeviceNode task{};
      task.slot = slot;
      task.input = predicate(slot);
      for (uint32_t i = 0; i < 4; ++i)
        task.points[i] = n.points[i];
      task.tolerance_down = tolerance_down[n.root.value];
      task.fair_axis = fair_axis(n);
      const auto sig = signature(n);
      for (auto tier = n.first_bound; tier != invalid_slot; tier = bound_tiers[tier].next)
        if (bound_tiers[tier].signature == sig) {
          task.cached = 1;
          task.cached_bounds = bound_tiers[tier].value;
          break;
        }
      if (n.born_query != query)
        ++counts.nodes_reused;
      node_tasks.push_back(task);
    }
    node_answers.resize(node_tasks.size());
    if (device) {
      ++counts.device_epochs;
      for (const auto &task : node_tasks)
        if (!task.cached)
          ++counts.device_bounds;
    }
    submit(Operation::nodes, narrow(node_tasks.size()));
    phase = Phase::bound_fold;
  }
  void split(uint32_t slot, SplitAxis axis) {
    const auto parent = nodes[slot];
    const auto a = axis == SplitAxis::u ? 0u : 1u;
    const auto depth = a ? parent.depth_v : parent.depth_u;
    check(depth < limits.max_axis_depth, ErrorCode::unresolved,
          "adaptive requested depth exhausted");
    const auto mid = midpoint(parent.coordinate[2 * a], parent.coordinate[2 * a + 1]);
    if (parent.children[2 * a] != invalid_slot) {
      next.push_back(parent.children[2 * a]);
      next.push_back(parent.children[2 * a + 1]);
      return;
    }
    const auto &root = source.roots()[parent.root.value];
    const uint32_t count = root.family == GeometryFamily::rational_tensor
                               ? (root.degree_u + 1) * (root.degree_v + 1) * 4
                               : 0;
    check(nodes.size() + 2 <= limits.nodes &&
              controls.size() + uint64_t{count} * 2 <= limits.control_scalars,
          ErrorCode::resource_exhausted, "adaptive next epoch capacity exhausted");
    // Both children/ranges fit before logical mutation. Allocation failure still
    // fails the query atomically; no partial successful result is exposed.
    // vector growth is geometric; never reserve exact size once per split.
    for (uint32_t side = 0; side < 2; ++side) {
      NodeMeta child;
      child.root = parent.root;
      child.parent = slot;
      child.step = 2 * a + side;
      child.depth_u = parent.depth_u + (a == 0);
      child.depth_v = parent.depth_v + (a == 1);
      child.coordinate = parent.coordinate;
      child.coordinate[2 * a + (side == 0 ? 1u : 0u)] = mid;
      child.controls = {narrow(controls.size()), count};
      child.controls_ready = !selection_only || !count;
      controls.resize(controls.size() + count);
      const auto id = make_node(child);
      nodes[slot].children[2 * a + side] = id;
      next.push_back(id);
      if (count && !selection_only) {
        restriction_tasks.push_back(
            {parent.root, parent.controls, child.controls.first, axis, side});
        ++counts.control_restrictions;
      }
    }
  }
  void preflight_splits(std::span<const std::pair<uint32_t, SplitAxis>> requests) {
    uint64_t add_nodes = 0, add_controls = 0;
    std::pmr::vector<SampleKey> missing_points(&budget);
    for (const auto &[slot, requested_axis] : requests) {
      const auto &n = nodes[slot];
      const auto axis = requested_axis == SplitAxis::u ? 0u : 1u;
      check(source.roots()[n.root.value].affine_separable_axes != 3u, ErrorCode::unresolved,
            "affine patch cannot improve by geometric subdivision");
      check((axis ? n.depth_v : n.depth_u) < limits.max_axis_depth, ErrorCode::unresolved,
            "adaptive requested depth exhausted");
      if (n.children[2 * axis] != invalid_slot)
        continue;
      add_nodes += 2;
      const auto &r = source.roots()[n.root.value];
      if (r.family == GeometryFamily::rational_tensor)
        add_controls += uint64_t{(r.degree_u + 1) * (r.degree_v + 1) * 4} * 2;
      auto coordinates = n.coordinate;
      const auto mid = midpoint(coordinates[2 * axis], coordinates[2 * axis + 1]);
      if (selection_only)
        continue;
      for (uint32_t side = 0; side < 2; ++side) {
        coordinates = n.coordinate;
        coordinates[2 * axis + (side == 0 ? 1u : 0u)] = mid;
        for (uint32_t corner = 0; corner < 4; ++corner) {
          const SampleKey key{r.source_slot, axis_key(r, 0, coordinates[corner & 1u]),
                              axis_key(r, 1, coordinates[2 + (corner >> 1)])};
          if (!point_index.contains(key))
            missing_points.push_back(key);
        }
      }
    }
    std::sort(missing_points.begin(), missing_points.end());
    missing_points.erase(std::unique(missing_points.begin(), missing_points.end()),
                         missing_points.end());
    check(nodes.size() + add_nodes <= limits.nodes &&
              controls.size() + add_controls <= limits.control_scalars &&
              points.size() + missing_points.size() <= limits.points &&
              requests.size() * 2 <= limits.frontier,
          ErrorCode::resource_exhausted, "adaptive complete next epoch exceeds capacity");
    nodes.reserve(nodes.size() + add_nodes);
    active_bounds.reserve(nodes.size() + add_nodes);
    controls.reserve(controls.size() + add_controls);
    points.reserve(points.size() + missing_points.size());
    point_stages.reserve(points.size() + missing_points.size());
    resident_stages.reserve(points.size() + missing_points.size());
    point_index.reserve(points.size() + missing_points.size());
    next.reserve(static_cast<std::size_t>(requests.size() * 2));
  }
  void fold_nodes() {
    next.clear();
    restriction_tasks.clear();
    ++counts.epochs;
    std::pmr::vector<DeviceNodeAnswer> decisions(node_answers.begin(), node_answers.end(), &budget);
    std::pmr::vector<uint32_t> promotions(&budget);
    uint32_t device_prefix = 0;
    bool changed_decision = false;
    for (uint32_t i = 0; i < node_tasks.size(); ++i) {
      const auto slot = node_tasks[i].slot;
      const auto n = nodes[slot];
      auto &result = decisions[i];
      if (device) {
        check(result.split_prefix == device_prefix, ErrorCode::device_failure,
              "adaptive device prefix mismatch");
        if (result.action == 1)
          ++device_prefix;
      }
      check(result.bounds.status != NumericStatus::invalid, ErrorCode::internal_error,
            "invalid owned node predicate");
      const auto tau = allowance(n.root.value);
      const bool floor = result.bounds.numerical_upper > 0 &&
                         result.bounds.numerical_upper >= tolerance_down[n.root.value] * .25;
      if (result.action == 2 || result.bounds.status != NumericStatus::ready || floor) {
        auto box = exact_box(n);
        const auto p = predicate(slot);
        ++counts.exact_fallbacks;
        const auto precise = bound_node_precise(
            source, n.root, box, std::span<const double, 12>(p.actual_corners), tau, budget);
        result.bounds = precise.bounds;
        result.action = precise.certified && result.bounds.regularity_proved ? 0u : 1u;
      } else if (result.action != 0 && result.bounds.regularity_proved &&
                 Rational::from_double(budget, result.bounds.packet_upper).compare(tau) <= 0)
        result.action = 0;
      check(result.bounds.status == NumericStatus::ready, ErrorCode::unresolved,
            "adaptive source bound unresolved");
      changed_decision = changed_decision || result.action != node_answers[i].action;
      const auto fair = fair_axis(n);
      result.axis = fair < 2                                        ? static_cast<SplitAxis>(fair)
                    : result.bounds.score_v > result.bounds.score_u ? SplitAxis::v
                                                                    : SplitAxis::u;
      if (!node_tasks[i].cached) {
        const auto index = narrow(bound_tiers.size());
        // BASE stays BASE. Query-requested precise bounds cannot replace it.
        bound_tiers.push_back({signature(n), n.first_bound, node_answers[i].bounds});
        nodes[slot].first_bound = index;
      }
      active_bounds[slot] = result.bounds;
      if (result.action != 0 && floor)
        for (auto id : n.points)
          if (point_stages[id] < 4)
            promotions.push_back(id);
    }
    if (!promotions.empty()) {
      std::sort(promotions.begin(), promotions.end());
      promotions.erase(std::unique(promotions.begin(), promotions.end()), promotions.end());
      check(promotions.size() <= limits.ambiguities, ErrorCode::resource_exhausted,
            "adaptive point promotion queue exhausted");
      // Resolve the entire sorted authority horizon before changing any node.
      for (auto id : promotions) {
        auto &p = points[id];
        auto stage = point_stages[id] + 1;
        auto uv = exact_uv(p);
        for (; stage <= 4; ++stage) {
          if (!(p.valid & (1u << stage))) {
            ++counts.exact_fallbacks;
            p.tiers[stage] = evaluate_sample_precise(source, p.input.original, uv,
                                                     static_cast<ArithmeticStage>(stage), budget);
            p.valid |= 1u << stage;
          }
          if (p.tiers[stage].status == NumericStatus::ready)
            break;
        }
        check(stage <= 4, ErrorCode::unresolved, "adaptive point promotion exhausted");
        point_stages[id] = stage;
      }
      std::erase_if(selected, [&](uint32_t slot) {
        const bool affected =
            std::any_of(nodes[slot].points.begin(), nodes[slot].points.end(), [&](uint32_t id) {
              return std::binary_search(promotions.begin(), promotions.end(), id);
            });
        if (affected)
          frontier.push_back(slot);
        return affected;
      });
      std::sort(frontier.begin(), frontier.end(), [&](auto x, auto y) { return less_node(x, y); });
      frontier.erase(std::unique(frontier.begin(), frontier.end()), frontier.end());
      phase = Phase::prepare;
      return;
    }
    std::pmr::vector<std::pair<uint32_t, SplitAxis>> requests(&budget);
    for (uint32_t i = 0; i < decisions.size(); ++i)
      if (decisions[i].action != 0 || !decisions[i].bounds.regularity_proved)
        requests.emplace_back(node_tasks[i].slot, decisions[i].axis);
    preflight_splits(requests);
    for (uint32_t i = 0; i < decisions.size(); ++i) {
      const auto slot = node_tasks[i].slot;
      if (decisions[i].action == 0 && decisions[i].bounds.regularity_proved)
        selected.push_back(slot);
      else {
        // The ordinary device scan determines canonical child positions. Only
        // the compact host ambiguity horizon may change those decisions.
        if (device && !changed_decision)
          check(next.size() == uint64_t{decisions[i].split_prefix} * 2, ErrorCode::device_failure,
                "adaptive device child placement mismatch");
        split(slot, decisions[i].axis);
      }
    }
    frontier.assign(next.begin(), next.end());
    std::sort(frontier.begin(), frontier.end(), [&](auto x, auto y) { return less_node(x, y); });
    if (!restriction_tasks.empty()) {
      if (device)
        counts.device_restrictions += restriction_tasks.size();
      submit(Operation::restrictions, narrow(restriction_tasks.size()));
      phase = Phase::restrict_nodes;
    } else
      phase = frontier.empty() ? Phase::freeze : Phase::prepare;
  }
  std::optional<Interval> normalized_point(uint32_t id, const NodeMeta &node, uint32_t axis) const {
    const auto &point = points[id];
    const auto &r = source.roots()[node.root.value];
    const auto &creator = source.roots()[point.prepared];
    const auto t = axis == 0 ? point.u : point.v, lo = node.coordinate[2 * axis],
               hi = node.coordinate[2 * axis + 1];
    if (r.prepared_box[2 * axis].lower == creator.prepared_box[2 * axis].lower &&
        r.prepared_box[2 * axis + 1].lower == creator.prepared_box[2 * axis + 1].lower) {
      if (t == lo)
        return numeric::point(0.);
      if (t == hi)
        return numeric::point(1.);
      const auto depth = axis == 0 ? node.depth_u : node.depth_v;
      check(t.exponent > depth && t.exponent >= lo.exponent, ErrorCode::internal_error,
            "invalid hanging dyadic coordinate");
      const auto shift = t.exponent - lo.exponent;
      check(!lo.numerator || shift < 64, ErrorCode::internal_error, "dyadic normalization shift");
      const auto left = lo.numerator ? lo.numerator << shift : 0;
      check(t.numerator > left, ErrorCode::internal_error, "hanging point outside leaf");
      return parameter(numeric::point(0.), numeric::point(1.),
                       {t.numerator - left, t.exponent - depth});
    }
    const auto &key = axis == 0 ? point.key.u : point.key.v;
    if (key.endpoint) {
      const auto x = std::bit_cast<double>(key.first);
      if (!lo.numerator && x == r.prepared_box[2 * axis].lower)
        return numeric::point(0.);
      if (hi.exponent == 0 && hi.numerator == 1 && x == r.prepared_box[2 * axis + 1].lower)
        return numeric::point(1.);
    }
    return std::nullopt;
  }
  bool physical_facet(const PacketTriangle &triangle) {
    std::array<double, 9> xyz{};
    for (uint32_t i = 0; i < 3; ++i)
      std::copy(frozen->points[triangle.points[i].value].actual_xyz.begin(),
                frozen->points[triangle.points[i].value].actual_xyz.end(), xyz.begin() + 3 * i);
    return actual_facet_nonzero(xyz, budget);
  }
  void freeze_selection() {
    check(cover_revision < UINT64_MAX, ErrorCode::resource_exhausted,
          "adaptive cover revision exhausted");
    std::sort(selected.begin(), selected.end(), [&](auto a, auto b) { return less_node(a, b); });
    selected.erase(std::unique(selected.begin(), selected.end()), selected.end());
    std::pmr::vector<uint32_t> ids(&budget);
    for (auto slot : selected)
      for (auto p : nodes[slot].points)
        ids.push_back(p);
    std::sort(ids.begin(), ids.end(),
              [&](auto a, auto b) { return points[a].key < points[b].key; });
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    check(table_ordinal < UINT32_MAX, ErrorCode::resource_exhausted,
          "adaptive exact table ordinal exhausted");
    frozen_point_ids.assign(ids.begin(), ids.end());
    ExactBuilder values(resource,
                        {{OwnerKind::run, UINT32_MAX - 2, serial}, query, ++table_ordinal});
    std::pmr::unordered_map<ScalarKey, ExactRef, ScalarHash> scalar_refs(&budget);
    auto coordinate_ref = [&](uint32_t root, Dyadic coordinate, uint32_t axis) {
      const auto &r = source.roots()[root];
      const ScalarKey key{r.source_slot, axis, axis_key(r, axis, coordinate)};
      if (const auto found = scalar_refs.find(key); found != scalar_refs.end())
        return found->second;
      const auto value = exact_parameter(budget, r.prepared_box[2 * axis].lower,
                                         r.prepared_box[2 * axis + 1].lower, coordinate);
      const auto ref = values.append(value);
      scalar_refs.emplace(key, ref);
      return ref;
    };
    std::pmr::vector<PointRecord> point_rows(&budget);
    std::pmr::vector<SelectedLeaf> leaf_rows(&budget);
    std::pmr::vector<uint64_t> paths(&budget);
    std::pmr::vector<uint32_t> remap(points.size(), invalid_slot, &budget);
    for (auto id : ids) {
      const auto &p = points[id];
      const auto x = coordinate_ref(p.prepared, p.u, 0), y = coordinate_ref(p.prepared, p.v, 1);
      PointRecord record{
          {p.input.original,
           {x, y},
           static_cast<ArithmeticStage>(point_stages[id]),
           point_stages[id] + 1},
          {x, y},
          {x, y},
          {sample(id).actual_xyz[0], sample(id).actual_xyz[1], sample(id).actual_xyz[2]},
          sample(id).departure_upper};
      remap[id] = narrow(point_rows.size());
      point_rows.push_back(record);
    }
    for (auto slot : selected) {
      const auto &n = nodes[slot];
      SelectedLeaf leaf;
      leaf.key.root = n.root;
      leaf.key.depth = n.depth_u + n.depth_v;
      leaf.key.path = {paths.size(), (leaf.key.depth + 31) / 32};
      paths.resize(paths.size() + leaf.key.path.count);
      std::array<uint32_t, 128> steps{};
      uint32_t depth = 0;
      for (auto at = slot; nodes[at].parent != invalid_slot; at = nodes[at].parent)
        steps[depth++] = nodes[at].step;
      for (uint32_t i = 0; i < depth; ++i)
        paths[leaf.key.path.first + i / 32] |= uint64_t{steps[depth - 1 - i]} << (2 * (i % 32));
      for (uint32_t i = 0; i < 4; ++i) {
        leaf.box[i] = coordinate_ref(n.root.value, n.coordinate[i], i / 2);
        leaf.corners[i] = {remap[n.points[i]]};
      }
      leaf.source_to_bilinear_upper = active_bounds[slot].source_to_bilinear_upper;
      leaf.twist_upper = active_bounds[slot].twist_upper;
      leaf_rows.push_back(leaf);
    }
    auto result = std::allocate_shared<PatchSelection>(
        RunAllocator<PatchSelection>(resource), resource, source, values.seal(),
        SelectionKey{serial, query, source.generation(), source.profile().revision,
                     ++cover_revision});
    result->points.assign(point_rows.begin(), point_rows.end());
    result->leaves.assign(leaf_rows.begin(), leaf_rows.end());
    result->path_words.assign(paths.begin(), paths.end());
    frozen = std::move(result);
    if (selection_only) {
      phase = Phase::done;
      return;
    }
    std::pmr::vector<uint32_t> enabled(&budget);
    for (uint32_t i = 0; i < selected.size(); ++i)
      enabled.push_back(i);
    mesh = build_packet_mesh(budget, resource, frozen, enabled);
    ++counts.packet_rounds;
    check(mesh->demoted_leaf_slots.empty(), ErrorCode::unsupported,
          "untrimmed patch has unmeshed demoted region");
    check(!mesh->triangles.empty(), ErrorCode::unresolved, "adaptive patch has no actual facets");
    facet_tasks.clear();
    facet_answers.clear();
    for (const auto &triangle : mesh->triangles) {
      const auto &leaf = frozen->leaves[triangle.leaf];
      ActualFacetInput task;
      task.source_to_bilinear_upper = leaf.source_to_bilinear_upper;
      for (uint32_t i = 0; i < 4; ++i)
        std::copy(frozen->points[leaf.corners[i].value].actual_xyz.begin(),
                  frozen->points[leaf.corners[i].value].actual_xyz.end(),
                  task.actual_corners + 3 * i);
      const auto &node = nodes[selected[triangle.leaf]];
      for (uint32_t i = 0; i < 3; ++i) {
        const auto point_slot = triangle.points[i].value;
        const auto &p = frozen->points[point_slot];
        for (uint32_t axis = 0; axis < 2; ++axis) {
          if (const auto normalized = normalized_point(frozen_point_ids[point_slot], node, axis))
            task.uv[2 * i + axis] = *normalized;
          else {
            const auto lo = import_exact(budget, frozen->values.view(leaf.box[2 * axis]));
            const auto hi = import_exact(budget, frozen->values.view(leaf.box[2 * axis + 1]));
            const auto position = import_exact(budget, frozen->values.view(p.chart_xy[axis]));
            const auto normalized_bounds = position.subtract(lo).divide(hi.subtract(lo)).bounds();
            task.uv[2 * i + axis] = {normalized_bounds.lower, normalized_bounds.upper};
          }
        }
        for (uint32_t axis = 0; axis < 3; ++axis)
          task.affine_xyz[3 * i + axis] = numeric::point(p.actual_xyz[axis]);
      }
      check(physical_facet(triangle), ErrorCode::unresolved,
            "adaptive actual facet physically collapsed");
      facet_tasks.push_back(task);
    }
    facet_answers.resize(facet_tasks.size());
    counts.actual_facet_tests += facet_tasks.size();
    submit(Operation::facets, narrow(facet_tasks.size()));
    phase = Phase::facet_fold;
  }
  void fold_facets() {
    std::pmr::vector<uint32_t> failed(&budget);
    for (uint32_t i = 0; i < facet_answers.size(); ++i) {
      const auto &triangle = mesh->triangles[i];
      const auto root = frozen->leaves[triangle.leaf].key.root.value;
      const auto tau = allowance(root);
      bool accepted = facet_answers[i].status == NumericStatus::ready &&
                      std::isfinite(facet_answers[i].total_upper) &&
                      Rational::from_double(budget, facet_answers[i].total_upper).compare(tau) <= 0;
      if (!accepted) {
        std::array<Rational, 6> uv{Rational(budget), Rational(budget), Rational(budget),
                                   Rational(budget), Rational(budget), Rational(budget)};
        std::array<Rational, 9> xyz{Rational(budget), Rational(budget), Rational(budget),
                                    Rational(budget), Rational(budget), Rational(budget),
                                    Rational(budget), Rational(budget), Rational(budget)};
        const auto &leaf = frozen->leaves[triangle.leaf];
        for (uint32_t p = 0; p < 3; ++p) {
          const auto &point = frozen->points[triangle.points[p].value];
          for (uint32_t a = 0; a < 2; ++a) {
            const auto lo = import_exact(budget, frozen->values.view(leaf.box[2 * a])),
                       hi = import_exact(budget, frozen->values.view(leaf.box[2 * a + 1]));
            uv[2 * p + a] = import_exact(budget, frozen->values.view(point.chart_xy[a]))
                                .subtract(lo)
                                .divide(hi.subtract(lo));
          }
          for (uint32_t a = 0; a < 3; ++a)
            xyz[3 * p + a] = Rational::from_double(budget, point.actual_xyz[a]);
        }
        ++counts.exact_fallbacks;
        accepted = bound_actual_facet_precise(facet_tasks[i], uv, xyz, tau, budget).certified;
      }
      if (!accepted) {
        failed.push_back(selected[triangle.leaf]);
        ++counts.actual_facet_failures;
      }
    }
    if (failed.empty()) {
      phase = Phase::done;
      return;
    }
    std::sort(failed.begin(), failed.end());
    failed.erase(std::unique(failed.begin(), failed.end()), failed.end());
    std::pmr::vector<std::pair<uint32_t, SplitAxis>> requests(&budget);
    for (auto p : failed) {
      const auto &n = nodes[p];
      const auto &b = active_bounds[p];
      const auto fair = fair_axis(n);
      const auto axis = fair < 2                ? static_cast<SplitAxis>(fair)
                        : b.score_v > b.score_u ? SplitAxis::v
                                                : SplitAxis::u;
      requests.emplace_back(p, axis);
    }
    // Check the complete feedback horizon before modifying the selected cover.
    preflight_splits(requests);
    std::erase_if(selected,
                  [&](auto p) { return std::binary_search(failed.begin(), failed.end(), p); });
    next.clear();
    restriction_tasks.clear();
    for (const auto &[slot, axis] : requests)
      split(slot, axis);
    frontier.assign(next.begin(), next.end());
    mesh.reset();
    frozen.reset();
    if (!restriction_tasks.empty()) {
      if (device)
        counts.device_restrictions += restriction_tasks.size();
      submit(Operation::restrictions, narrow(restriction_tasks.size()));
      phase = Phase::restrict_nodes;
    } else
      phase = Phase::prepare;
  }
};

inline void open_core(SurfaceCacheCore &p, const CacheDevice &request) {
  p.stopped();
  check(p.phase == SurfaceCacheCore::Phase::idle && !p.query && !p.device,
        ErrorCode::invalid_source, "adaptive device must open before queries");
  auto *memory = p.resource->allocate(sizeof(DeviceOwner), alignof(DeviceOwner));
  p.device = new (memory) DeviceOwner(p.resource);
  auto &owner = *p.device;
  owner.module = dlopen(request.module.c_str(), RTLD_NOW | RTLD_LOCAL);
  check(owner.module != nullptr, ErrorCode::device_failure, "cannot load adaptive CUDA module");
  using Factory = const AdaptiveCudaAPI *(*)();
  auto factory = reinterpret_cast<Factory>(dlsym(owner.module, "brepgpu_adaptive_cuda_v1"));
  check(factory != nullptr, ErrorCode::device_failure, "adaptive CUDA factory missing");
  owner.api = factory();
  check(owner.api && owner.api->abi == adaptive_cuda_abi &&
            owner.api->size == sizeof(AdaptiveCudaAPI),
        ErrorCode::device_failure, "adaptive CUDA ABI mismatch");
  const auto op_capacity = std::max(uint64_t{p.limits.frontier} * 4, uint64_t{p.limits.points} * 2);
  check(op_capacity < invalid_slot, ErrorCode::resource_exhausted,
        "adaptive operation capacity overflow");
  DeviceConfig config{};
  config.abi = adaptive_cuda_abi;
  config.size = sizeof(DeviceConfig);
  config.device = request.device;
  config.points = p.limits.points;
  config.controls = p.limits.control_scalars;
  config.nodes = p.limits.nodes;
  config.frontier = static_cast<uint32_t>(op_capacity);
  config.device_bytes = p.limits.device_bytes;
  config.host_bytes = p.limits.host_bytes;
  config.work = p.limits.total_work;
  config.reservation_context = &owner;
  config.reserve = detail::reserve_external<AdaptiveCudaAPI>;
  // The C ABI has two opaque pointers; keep the internal release operation typed.
  config.release = [](auto *context, auto *token) noexcept {
    release_external(*static_cast<DeviceOwner *>(context), static_cast<ExternalLease *>(token));
  };
  DeviceError error{};
  device_check(owner.api->create(&config, p.source.roots().data(), narrow(p.source.roots().size()),
                                 p.source.original_roots().data(),
                                 narrow(p.source.original_roots().size()), p.controls.data(),
                                 narrow(p.controls.size()), &owner.session, &error),
               error);
}

// Preparation owns all potentially throwing allowance decoding and initial
// metadata allocation. Device assignment barriers run after this preparation.
struct BeginPreparation final {
  uint64_t generation;
  bool replacement;
  ExactTable values;
  std::pmr::vector<ExactRef> allowances;
  std::pmr::vector<double> tolerance_down;
  std::pmr::vector<RootSlot> roots;
  BeginPreparation(SurfaceCacheCore &p, const PatchQuery &request, bool same_query)
      : generation(request.generation), replacement(same_query), values(request.values),
        allowances(p.source.roots().size(), {}, &p.budget),
        tolerance_down(p.source.roots().size(), 0., &p.budget), roots(&p.budget) {}
};
inline BeginPreparation prepare_begin_core(SurfaceCacheCore &p, const PatchQuery &request,
                                           bool subset, bool replacement = false) {
  p.stopped();
  check(p.phase == SurfaceCacheCore::Phase::idle && !p.mesh && !p.jobs && !p.ticket,
        ErrorCode::invalid_source, "adaptive query already active");
  check(&request.source.view() == &p.source.view(), ErrorCode::invalid_source,
        "foreign adaptive source cache");
  check((request.generation > p.query || (replacement && request.generation == p.query)) &&
            request.values.resource() == p.resource,
        ErrorCode::invalid_source, "stale adaptive query or foreign table resource");
  check((subset || request.allowances.size() == p.source.roots().size()) &&
            !request.allowances.empty(),
        ErrorCode::invalid_source, "adaptive query requires complete root allowances");
  check(request.allowances.size() <= p.limits.nodes &&
            request.allowances.size() <= p.limits.frontier,
        ErrorCode::resource_exhausted, "adaptive initial root capacity");
  BeginPreparation staged(p, request, replacement);
  std::pmr::vector<uint8_t> seen(p.source.roots().size(), uint8_t{0}, &p.budget);
  uint64_t missing = 0;
  for (const auto &entry : request.allowances) {
    check(entry.root.value < p.source.roots().size() && !seen[entry.root.value],
          ErrorCode::invalid_source, "duplicate/foreign adaptive root");
    seen[entry.root.value] = 1;
    auto tau = import_exact(p.budget, request.values.view(entry.positional));
    check(tau.sign() > 0, ErrorCode::invalid_source, "nonpositive adaptive positional allowance");
    staged.allowances[entry.root.value] = entry.positional;
    staged.tolerance_down[entry.root.value] = tau.bounds().lower;
    staged.roots.push_back(entry.root);
    if (p.roots.empty() || p.roots[entry.root.value] == invalid_slot)
      ++missing;
  }
  check(missing <= p.limits.nodes - p.nodes.size(), ErrorCode::resource_exhausted,
        "adaptive initial root capacity");
  p.nodes.reserve(p.nodes.size() + missing);
  p.active_bounds.reserve(p.active_bounds.size() + missing);
  p.roots.reserve(p.source.roots().size());
  p.frontier.reserve(staged.roots.size());
  return staged;
}
inline void commit_begin_core(SurfaceCacheCore &p, BeginPreparation staged) {
  p.query_values = std::move(staged.values);
  p.allowances = std::move(staged.allowances);
  p.tolerance_down = std::move(staged.tolerance_down);
  p.query = staged.generation;
  if (!staged.replacement) {
    p.table_ordinal = 0;
    ++p.counts.queries;
    std::fill(p.point_stages.begin(), p.point_stages.end(), 0);
  }
  p.frontier.clear();
  p.selected.clear();
  p.next.clear();
  p.restriction_tasks.clear();
  if (p.roots.empty())
    p.roots.resize(p.source.roots().size(), invalid_slot);
  for (const auto entry : staged.roots) {
    auto &root = p.roots[entry.value];
    if (root == invalid_slot) {
      NodeMeta n;
      n.root = entry;
      n.coordinate = {Dyadic{0, 0}, Dyadic{1, 0}, Dyadic{0, 0}, Dyadic{1, 0}};
      n.controls = p.source.roots()[entry.value].controls;
      root = p.make_node(n);
    }
    p.frontier.push_back(root);
  }
  std::sort(p.frontier.begin(), p.frontier.end(),
            [&](auto a, auto b) { return p.less_node(a, b); });
  p.phase = SurfaceCacheCore::Phase::prepare;
}
inline void begin_core(SurfaceCacheCore &p, const PatchQuery &request, bool subset,
                       bool replacement = false) {
  commit_begin_core(p, prepare_begin_core(p, request, subset, replacement));
}
inline void retire_core_pages(SurfaceCacheCore &p) {
  check(p.phase == SurfaceCacheCore::Phase::idle && !p.jobs && !p.ticket, ErrorCode::invalid_source,
        "adaptive retirement requires a drained idle owner");
  p.frozen.reset();
  p.mesh.reset();
  p.query_values.reset();
  p.nodes.clear();
  p.controls.resize(p.source.controls().size());
  p.points.clear();
  p.point_index.clear();
  p.point_stages.clear();
  p.resident_stages.clear();
  p.roots.clear();
  p.frontier.clear();
  p.selected.clear();
  p.next.clear();
  p.frozen_point_ids.clear();
  p.active_bounds.clear();
  p.bound_tiers.clear();
  p.allowances.clear();
  p.tolerance_down.clear();
  p.sample_tasks.clear();
  p.sample_answers.clear();
  p.restriction_tasks.clear();
  p.node_tasks.clear();
  p.node_answers.clear();
  p.facet_tasks.clear();
  p.facet_answers.clear();
  p.operation = SurfaceCacheCore::Operation::none;
}

inline StepState step_core(SurfaceCacheCore &p, JobContext &context) {
  context.budget.charge(0);
  p.stopped();
  check(p.phase != SurfaceCacheCore::Phase::idle && p.phase != SurfaceCacheCore::Phase::closed,
        ErrorCode::invalid_source, "adaptive query not active");
  switch (p.phase) {
  case SurfaceCacheCore::Phase::prepare:
    p.prepare_points();
    break;
  case SurfaceCacheCore::Phase::sample_fold:
    if (!p.ready(p.sample_answers.data(), narrow(p.sample_answers.size())))
      return StepState::ready;
    p.fold_samples();
    break;
  case SurfaceCacheCore::Phase::bounds:
    if (p.operation == SurfaceCacheCore::Operation::samples) {
      if (!p.ready(p.sample_answers.data(), narrow(p.sample_answers.size())))
        return StepState::ready;
      for (const auto &task : p.sample_tasks)
        p.resident_stages[task.destination] = p.point_stages[task.destination];
    }
    p.prepare_nodes();
    break;
  case SurfaceCacheCore::Phase::bound_fold:
    if (!p.ready(p.node_answers.data(), narrow(p.node_answers.size())))
      return StepState::ready;
    p.fold_nodes();
    break;
  case SurfaceCacheCore::Phase::restrict_nodes:
    if (!p.ready(nullptr, 0))
      return StepState::ready;
    p.phase =
        p.frontier.empty() ? SurfaceCacheCore::Phase::freeze : SurfaceCacheCore::Phase::prepare;
    break;
  case SurfaceCacheCore::Phase::freeze:
    p.freeze_selection();
    break;
  case SurfaceCacheCore::Phase::facet_fold:
    if (!p.ready(p.facet_answers.data(), narrow(p.facet_answers.size())))
      return StepState::ready;
    p.fold_facets();
    break;
  case SurfaceCacheCore::Phase::done:
    return StepState::done;
  default:
    throw Error(ErrorCode::internal_error, "invalid adaptive phase");
  }
  return p.phase == SurfaceCacheCore::Phase::done ? StepState::done : StepState::ready;
}

inline CacheStats stats_core(const SurfaceCacheCore &p) noexcept {
  auto result = p.counts;
  result.host_reserved_bytes = p.budget.live();
  result.host_peak_bytes = p.budget.peak();
  if (p.workers) {
    const auto reserved =
        uint64_t{p.limits.cpu_threads} * std::min<uint64_t>(p.limits.child_bytes, 65536);
    result.host_reserved_bytes += reserved;
    result.host_peak_bytes += reserved;
  }
  if (p.device && p.device->session && p.device->api) {
    DeviceStats external{};
    DeviceError error{};
    if (!p.device->api->stats(p.device->session, &external, &error)) {
      result.host_reserved_bytes += external.host_reserved_bytes;
      result.host_peak_bytes += external.host_peak_bytes;
      result.device_reserved_bytes = external.device_reserved_bytes;
      result.device_peak_bytes = external.device_peak_bytes;
      result.uploaded_bytes = external.uploaded_bytes;
      result.downloaded_bytes = external.downloaded_bytes;
    }
  }
  return result;
}

inline void close_core(SurfaceCacheCore &p) {
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
      device_check(owner->api->destroy(owner->session, &error), error);
      owner->session = nullptr;
    }
    check(owner->leases == nullptr, ErrorCode::device_failure,
          "adaptive external leases remain after destroy");
    if (owner->module)
      dlclose(owner->module);
    owner->~DeviceOwner();
    p.resource->deallocate(owner, sizeof(DeviceOwner), alignof(DeviceOwner));
    p.device = nullptr;
  }
  p.phase = SurfaceCacheCore::Phase::closed;
}

} // namespace bgmesh::adaptive::detail::surface
#endif
