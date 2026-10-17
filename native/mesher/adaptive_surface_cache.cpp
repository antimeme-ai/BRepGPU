#include "adaptive_surface_cache.hpp"
#include "adaptive_cache_detail.hpp"
#include "adaptive_regions.hpp"
#include "cuda/adaptive_assignment_api.h"
#include "cuda/adaptive_trim_api.h"

namespace bgmesh::adaptive {
using namespace detail::surface;
struct SurfaceCacheSession::Impl final : SurfaceCacheCore {
  struct ActiveTarget final {
    RootSlot root;
    std::array<Dyadic, 2> coordinate;
  };
  SurfaceFaceIdentity face;
  enum class AssignmentMode { unset, legacy, pooled };
  AssignmentMode assignment_mode = AssignmentMode::unset;
  uint64_t last_assignment = 0;
  bool pages_retired = true;
  const AssignmentCudaAPI *assignment_api = nullptr;
  CandidateBoxes candidate;
  SurfaceBoundView cover;
  uint32_t candidate_ordinal = 0;
  bool classified = false;
  bool provenance_set = false;
  std::shared_ptr<const FaceDomainData> classified_domain;
  bool controls_pending = false;
  std::pmr::vector<uint32_t> control_nodes;
  std::pmr::vector<SurfaceClass> node_classes;
  std::pmr::vector<ActiveTarget> required_targets;
  std::pmr::vector<RootSlot> query_roots;
  enum class PointPhase { none, waiting, install, reselect, complete };
  PointPhase point_phase = PointPhase::none;
  uint64_t point_ordinal = 0;
  SurfacePointResult point_output;
  std::pmr::vector<uint32_t> requested_points;
  std::pmr::unordered_multimap<uint64_t, uint32_t> absolute_index;
  bool point_reselect = false;
  static uint64_t hash_uv(uint32_t source, std::span<const Rational, 2> uv) {
    uint64_t h = mix(0, source);
    for (const auto &x : uv) {
      h = mix(h, static_cast<uint64_t>(x.sign()));
      for (auto limbs : {x.numerator().magnitude(), x.denominator().magnitude()}) {
        h = mix(h, limbs.size());
        for (auto word : limbs)
          h = mix(h, word);
      }
    }
    return h;
  }
  std::optional<Dyadic> compact(const Rational &value) {
    if (value.sign() < 0 || value.compare(Rational(budget, 1)) > 0)
      return {};
    const auto den = value.denominator().magnitude();
    uint32_t bits = 0, ones = 0;
    for (uint32_t i = 0; i < den.size(); ++i) {
      ones += static_cast<uint32_t>(std::popcount(den[i]));
      if (den[i])
        bits = i * 32 + static_cast<uint32_t>(std::countr_zero(den[i]));
    }
    if (ones != 1 || bits > 64)
      return {};
    const auto num = value.numerator().magnitude();
    if (num.size() > 2)
      return {};
    uint64_t n = num.empty() ? 0 : num[0];
    if (num.size() == 2)
      n |= uint64_t{num[1]} << 32;
    return Dyadic{n, bits};
  }
  uint32_t absolute_point(std::span<const Rational, 2> uv, ExactTable values,
                          const std::array<ExactRef, 2> &refs) {
    const auto carrier = source.source().view().carrier_index.find(
        source.source().view().faces[face.face.value].carrier_id);
    const auto root = find_carrier_root_slot(source.roots(), carrier, uv, RootBoxKind::prepared);
    const auto &r = source.roots()[root];
    std::array<std::optional<Dyadic>, 2> coordinates;
    for (uint32_t axis = 0; axis < 2; ++axis) {
      auto a = Rational::from_double(budget, r.prepared_box[2 * axis].lower);
      auto z = Rational::from_double(budget, r.prepared_box[2 * axis + 1].lower);
      coordinates[axis] = compact(uv[axis].subtract(a).divide(z.subtract(a)));
    }
    if (coordinates[0] && coordinates[1])
      return point(root, *coordinates[0], *coordinates[1]);
    const auto hash = hash_uv(carrier.value, uv);
    const auto range = absolute_index.equal_range(hash);
    for (auto at = range.first; at != range.second; ++at) {
      const auto &p = points[at->second];
      auto existing = exact_uv(p);
      if (existing[0].compare(uv[0]) == 0 && existing[1].compare(uv[1]) == 0) {
        ++counts.point_hits;
        return at->second;
      }
    }
    check(points.size() < limits.points, ErrorCode::resource_exhausted,
          "surface exact point capacity exhausted");
    SampleInput input;
    input.original = {
        find_carrier_root_slot(source.original_roots(), carrier, uv, RootBoxKind::original)};
    for (uint32_t axis = 0; axis < 2; ++axis) {
      const auto bounds = uv[axis].bounds();
      input.uv[axis] = {bounds.lower, bounds.upper};
    }
    const auto id = narrow(points.size());
    points.push_back({{}, root, {}, {}, input, {}, 0, AbsolutePoint{std::move(values), refs}});
    point_stages.push_back(0);
    resident_stages.push_back(invalid_slot);
    absolute_index.emplace(hash, id);
    return id;
  }
  enum class FacetPhase { none, prepare, base, source, plane, complete };
  FacetPhase facet_phase = FacetPhase::none;
  std::optional<SurfaceFacetRequest> facet_request;
  std::optional<TrimFacetWork> facet_work;
  SurfaceFacetResult facet_output;
  std::pmr::vector<uint32_t> selected_facets, piece_queue, plane_queue, batch_indices;
  std::pmr::vector<uint8_t> piece_accepted, plane_accepted;
  std::pmr::vector<SourceTriangleInput> source_inputs;
  std::pmr::vector<SourceTriangleBounds> source_answers;
  uint32_t bound_at = 0, bound_count = 0, precise_count = 0;
  uint64_t facet_work_spent = 0;
  bool bound_pending = false;
  const TrimCudaAPI *trim_api = nullptr;
  struct SourceJob final : NativeTask {
    Impl *owner;
    uint32_t first, end;
    SourceJob(NativeTaskKey key, Impl *p, Range<uint32_t> slice)
        : NativeTask(key), owner(p), first(static_cast<uint32_t>(slice.first)),
          end(static_cast<uint32_t>(slice.first + slice.count)) {}
    StepState step(JobContext &context) override {
      for (auto i = first; i < end; ++i) {
        context.budget.charge(16384);
        if (owner->cancelled.load(std::memory_order_relaxed) || owner->stop.stop_requested())
          throw Error(ErrorCode::cancelled, "surface source batch cancelled");
        owner->source_answers[i] =
            owner->facet_phase == FacetPhase::plane
                ? bound_original_plane(owner->source, owner->source_inputs[i])
                : bound_source_triangle(owner->source, owner->source_inputs[i]);
      }
      return StepState::done;
    }
  };
  void submit_source() {
    const auto count = narrow(source_inputs.size());
    source_answers.resize(count);
    budget.charge(uint64_t{count} * 16384u);
    if (device) {
      if (!trim_api) {
        using Getter = const TrimCudaAPI *(*)();
        auto get = reinterpret_cast<Getter>(dlsym(device->module, "brepgpu_adaptive_trim_cuda_v1"));
        check(get, ErrorCode::unsupported, "CUDA source triangle extension unavailable");
        trim_api = get();
        check(trim_api && trim_api->abi == adaptive_trim_cuda_abi &&
                  trim_api->size == sizeof(TrimCudaAPI) && trim_api->source_triangles,
              ErrorCode::unsupported, "invalid CUDA source triangle extension");
      }
      DeviceError error{};
      device_check(
          trim_api->source_triangles(device->session, source_inputs.data(), count, &ticket, &error),
          error);
    } else if (workers) {
      ++batch_serial;
      const auto number = std::min(count, limits.cpu_threads * 4);
      for (uint32_t i = 0; i < number; ++i) {
        const auto first = static_cast<uint32_t>(uint64_t{count} * i / number);
        const auto end = static_cast<uint32_t>(uint64_t{count} * (i + 1) / number);
        NativeTaskKey key{{OwnerKind::run, UINT32_MAX - 1, uint64_t{i} + 1}, batch_serial, serial};
        workers->submit(std::allocate_shared<SourceJob>(RunAllocator<SourceJob>(resource), key,
                                                        this, Range<uint32_t>{first, end - first}));
      }
      jobs = number;
    } else {
      JobContext ctx({limits.child_bytes, limits.child_work}, resource.get(), stop);
      NativeTaskKey key{{OwnerKind::run, UINT32_MAX - 1, 1}, ++batch_serial, serial};
      SourceJob job(key, this, Range<uint32_t>{0, count});
      (void)job.step(ctx);
    }
  }
  bool accepted_upper(double upper, const Rational &tau) {
    return std::isfinite(upper) && upper >= 0. &&
           Rational::from_double(budget, upper).compare(tau) <= 0;
  }
  bool precise_candidate(const SourceTriangleBounds &answer, double down) {
    if (answer.status != NumericStatus::ready)
      return true;
    if (!std::isfinite(answer.position_upper) || !std::isfinite(answer.uncertainty_upper))
      return true;
    const auto width = answer.uncertainty_upper;
    return width >= down * .25 || answer.position_upper - down <= 4. * width ||
           answer.position_upper <= down * (1. + 0x1p-40);
  }
  SurfaceProgress step_facets() {
    if (!facet_request)
      throw Error(ErrorCode::internal_error, "missing surface facet request");
    auto &request = facet_request.value();
    check(facet_work_spent < request.limits.work, ErrorCode::resource_exhausted,
          "surface facet cumulative work cap");
    JobContext context(
        {limits.child_bytes, std::min(limits.child_work, request.limits.work - facet_work_spent)},
        resource.get(), stop);
    const auto finish = [&](SurfaceProgress state) {
      facet_work_spent += context.budget.work();
      budget.charge(context.budget.work());
      return state;
    };
    const auto tau =
        import_exact(budget, request.allowances.values.view(request.allowances.face_position));
    if (facet_phase == FacetPhase::prepare) {
      check(table_ordinal < UINT32_MAX, ErrorCode::resource_exhausted,
            "surface bound table ordinal exhausted");
      facet_work = prepare_trim_facet_work(
          context, request.view, request.construction, request.domain, request.allowances,
          selected_facets, {{OwnerKind::run, UINT32_MAX - 2, serial}, query, ++table_ordinal},
          request.limits);
      piece_accepted.assign(facet_work.value().pieces().size(), uint8_t{0});
      plane_accepted.assign(facet_work.value().facets().size(), uint8_t{0});
      piece_queue.clear();
      plane_queue.clear();
      for (uint32_t i = 0; i < facet_work.value().pieces().size(); ++i)
        piece_queue.push_back(i);
      for (uint32_t i = 0; i < facet_work.value().facets().size(); ++i)
        if (facet_work.value().facets()[i].original_plane)
          plane_queue.push_back(i);
      facet_phase = FacetPhase::base;
      bound_at = 0;
    }
    if (bound_pending) {
      const bool base = facet_phase == FacetPhase::base;
      if (!ready(base ? static_cast<void *>(facet_answers.data())
                      : static_cast<void *>(source_answers.data()),
                 bound_count))
        return finish(SurfaceProgress::pending);
      for (uint32_t i = 0; i < bound_count; ++i) {
        const auto index = batch_indices[i];
        if (base) {
          piece_accepted[index] =
              static_cast<uint8_t>(facet_answers[i].status == NumericStatus::ready &&
                                   accepted_upper(facet_answers[i].total_upper, tau));
        } else {
          const auto &answer = source_answers[i];
          bool accepted =
              answer.status == NumericStatus::ready && accepted_upper(answer.position_upper, tau);
          if (!accepted && precise_candidate(answer, tau.bounds().lower)) {
            check(precise_count < request.limits.precise_tasks, ErrorCode::resource_exhausted,
                  "surface precise bound horizon cap");
            ++precise_count;
            ++counts.exact_fallbacks;
            auto exact = facet_phase == FacetPhase::plane
                             ? compare_trim_plane_precise(context, facet_work.value(), index)
                             : compare_trim_piece_precise(context, facet_work.value(), index,
                                                          TrimBoundMethod::original_source);
            accepted = exact.certified;
          }
          if (facet_phase == FacetPhase::plane)
            plane_accepted[index] = static_cast<uint8_t>(accepted);
          else
            piece_accepted[index] = static_cast<uint8_t>(accepted);
        }
      }
      bound_pending = false;
      bound_at += bound_count;
    }
    for (;;) {
      const auto &queue = facet_phase == FacetPhase::plane ? plane_queue : piece_queue;
      if (bound_at < queue.size()) {
        bound_count = static_cast<uint32_t>(
            std::min<uint64_t>(request.limits.batch_records, queue.size() - bound_at));
        batch_indices.assign(queue.begin() + bound_at, queue.begin() + bound_at + bound_count);
        if (facet_phase == FacetPhase::base) {
          auto batch = pack_base_pieces(context, facet_work.value(), batch_indices);
          facet_tasks.assign(batch.inputs.begin(), batch.inputs.end());
          facet_answers.resize(bound_count);
          submit(Operation::facets, bound_count);
        } else {
          if (facet_phase == FacetPhase::plane) {
            auto batch = pack_plane_facets(context, facet_work.value(), batch_indices);
            source_inputs.assign(batch.inputs.begin(), batch.inputs.end());
          } else {
            auto batch = pack_source_pieces(context, facet_work.value(), batch_indices);
            source_inputs.assign(batch.inputs.begin(), batch.inputs.end());
          }
          submit_source();
        }
        counts.actual_facet_tests += bound_count;
        if (facet_phase != FacetPhase::base)
          counts.source_triangle_tests += bound_count;
        bound_pending = true;
        return finish(SurfaceProgress::pending);
      }
      if (facet_phase == FacetPhase::base) {
        uint32_t keep = 0;
        for (auto index : piece_queue)
          if (!piece_accepted[index])
            piece_queue[keep++] = index;
        piece_queue.resize(keep);
        facet_phase = FacetPhase::source;
        bound_at = 0;
      } else if (facet_phase == FacetPhase::source) {
        facet_phase = FacetPhase::plane;
        bound_at = 0;
      } else
        break;
    }
    check(table_ordinal < UINT32_MAX, ErrorCode::resource_exhausted,
          "surface bound result table ordinal exhausted");
    ExactBuilder values(resource,
                        {{OwnerKind::run, UINT32_MAX - 2, serial}, query, ++table_ordinal});
    const auto upper = values.append(tau);
    auto result = std::allocate_shared<SurfaceFacetResultData>(
        RunAllocator<SurfaceFacetResultData>(resource), resource, facet_work.value(), values.seal(),
        request.ordinal);
    std::pmr::vector<uint32_t> failed(&budget);
    for (uint32_t i = 0; i < facet_work.value().facets().size(); ++i) {
      const auto &row = facet_work.value().facets()[i];
      bool accepted = row.physical_nonzero;
      if (row.original_plane)
        accepted = accepted && plane_accepted[i];
      else
        for (uint64_t at = row.pieces.first; at < row.pieces.first + row.pieces.count; ++at)
          accepted = accepted && piece_accepted[at];
      result->facets.push_back({row.construction_facet,
                                accepted ? std::optional<ExactRef>(upper) : std::nullopt,
                                accepted});
      if (!accepted) {
        failed.push_back(i);
        ++counts.actual_facet_failures;
      }
    }
    // Work uses canonical FacetKey order. Return exactly the captured caller
    // subset order rather than relying on facet allocation order.
    std::pmr::unordered_map<uint32_t, SurfaceFacetAnswer> answers(&budget);
    answers.reserve(result->facets.size());
    for (const auto &row : result->facets)
      answers.emplace(row.facet, row);
    result->facets.clear();
    for (auto index : selected_facets)
      result->facets.push_back(answers.at(index));
    if (!failed.empty()) {
      auto floors = trim_point_floor_requests(context, facet_work.value(), failed);
      result->point_floors.assign(floors.requests.begin(), floors.requests.end());
    }
    facet_output = std::move(result);
    facet_phase = FacetPhase::complete;
    return finish(SurfaceProgress::facets_complete);
  }
  Impl(std::shared_ptr<RunResource> owner, SourceCache source_roots, CacheLimits requested_limits,
       const std::stop_token &external_stop_token)
      : SurfaceCacheCore(std::move(owner), std::move(source_roots), requested_limits,
                         external_stop_token),
        control_nodes(&budget), node_classes(&budget), required_targets(&budget),
        query_roots(&budget), requested_points(&budget), absolute_index(&budget),
        selected_facets(&budget), piece_queue(&budget), plane_queue(&budget),
        batch_indices(&budget), piece_accepted(&budget), plane_accepted(&budget),
        source_inputs(&budget), source_answers(&budget) {
    selection_only = true;
  }
  CandidateBoxes make_candidates() {
    check(candidate_ordinal < UINT32_MAX, ErrorCode::resource_exhausted,
          "surface candidate ordinal exhausted");
    std::sort(frontier.begin(), frontier.end(), [&](auto a, auto b) { return less_node(a, b); });
    node_classes.resize(nodes.size(), SurfaceClass::outside);
    ExactBuilder exact(resource,
                       {{OwnerKind::run, UINT32_MAX - 6, serial}, query, ++candidate_ordinal});
    std::pmr::vector<CandidateBox> rows(&budget);
    std::pmr::vector<uint64_t> words(&budget);
    for (auto slot : frontier) {
      const auto &node = nodes[slot];
      CandidateBox row;
      row.key.root = node.root;
      row.key.depth = node.depth_u + node.depth_v;
      row.key.path = {words.size(), (row.key.depth + 31) / 32};
      words.resize(words.size() + row.key.path.count);
      std::array<uint32_t, 128> path{};
      uint32_t count = 0;
      for (auto at = slot; nodes[at].parent != invalid_slot; at = nodes[at].parent)
        path[count++] = nodes[at].step;
      for (uint32_t i = 0; i < count; ++i)
        words[row.key.path.first + i / 32] |= uint64_t{path[count - i - 1]} << (2 * (i % 32));
      auto box = exact_box(node);
      for (uint32_t axis = 0; axis < 4; ++axis)
        row.box[axis] = exact.append(box[axis]);
      rows.push_back(row);
    }
    const SurfaceViewKey identity{
        {serial, query, source.generation(), source.profile().revision, cover_revision}, face};
    auto result = std::allocate_shared<CandidateBoxesData>(
        RunAllocator<CandidateBoxesData>(resource), resource, source, exact.seal(), identity,
        candidate_ordinal);
    result->boxes = std::move(rows);
    result->path_words = std::move(words);
    return result;
  }
};
SurfaceCacheSession::SurfaceCacheSession(std::shared_ptr<RunResource> resource, SourceCache cache,
                                         CacheLimits limits, const std::stop_token &stop)
    : resource_(std::move(resource)), impl_(nullptr) {
  check(resource_ != nullptr, ErrorCode::internal_error, "null surface cache resource");
  auto *memory = resource_->allocate(sizeof(Impl), alignof(Impl));
  try {
    impl_ = new (memory) Impl(resource_, std::move(cache), limits, stop);
  } catch (...) {
    resource_->deallocate(memory, sizeof(Impl), alignof(Impl));
    throw;
  }
}
SurfaceCacheSession::~SurfaceCacheSession() {
  if (!impl_)
    return;
  try {
    close();
  } catch (...) {
    if (impl_->device) {
      retain_poison(impl_->device);
      impl_->device = nullptr;
    }
  }
  impl_->~Impl();
  resource_->deallocate(impl_, sizeof(Impl), alignof(Impl));
}
void SurfaceCacheSession::open_device(const CacheDevice &device) { open_core(*impl_, device); }
void SurfaceCacheSession::begin(const SurfaceQuery &query) {
  auto &p = *impl_;
  const auto &source = p.source.source().view();
  check(query.face.definition.value < source.definitions.size() &&
            query.face.face.value < source.faces.size(),
        ErrorCode::invalid_source, "surface face identity outside source");
  check(source.faces[query.face.face.value].definition_id ==
            source.definitions[query.face.definition.value].id,
        ErrorCode::invalid_source, "surface face definition mismatch");
  const auto carrier = source.carrier_index.find(source.faces[query.face.face.value].carrier_id);
  for (const auto &allowance : query.allowances)
    check(allowance.root.value < p.source.roots().size() &&
              p.source.roots()[allowance.root.value].source_slot == carrier.value,
          ErrorCode::invalid_source, "surface root does not belong to face carrier");
  const bool pooled = query.face.assignment_epoch != 0;
  check(pooled ? p.assignment_mode != Impl::AssignmentMode::legacy
               : p.assignment_mode != Impl::AssignmentMode::pooled,
        ErrorCode::invalid_source, "surface legacy/pooled assignment mode mismatch");
  const bool new_assignment = pooled && query.face.assignment_epoch > p.last_assignment;
  if (pooled)
    check(new_assignment ? p.pages_retired : query.face.assignment_epoch == p.last_assignment,
          ErrorCode::invalid_source, "nonmonotone or unretired surface assignment");
  const bool same_query = query.generation == p.query && p.query != 0;
  const bool continuation = same_query && !new_assignment;
  if (continuation)
    check(p.phase == Impl::Phase::idle && query.face.definition == p.face.definition &&
              query.face.face == p.face.face &&
              query.face.source_generation == p.face.source_generation &&
              query.face.assignment_epoch == p.face.assignment_epoch &&
              query.face.face_generation > p.face.face_generation &&
              query.face.boundary_generation > p.face.boundary_generation,
          ErrorCode::invalid_source, "stale same-query surface boundary replacement");
  if (pooled && !new_assignment)
    check(continuation && !p.pages_retired, ErrorCode::invalid_source,
          "stale pooled surface assignment");
  auto staged = prepare_begin_core(
      p, {query.generation, query.source, query.values, query.allowances}, true, same_query);
  p.query_roots.reserve(query.allowances.size());
  if (new_assignment && p.device) {
    if (!p.assignment_api) {
      using Getter = const AssignmentCudaAPI *(*)();
      const auto get =
          reinterpret_cast<Getter>(dlsym(p.device->module, "brepgpu_adaptive_assignment_cuda_v1"));
      check(get, ErrorCode::unsupported, "CUDA assignment retirement extension unavailable");
      p.assignment_api = get();
      check(p.assignment_api && p.assignment_api->abi == adaptive_assignment_cuda_abi &&
                p.assignment_api->size == sizeof(AssignmentCudaAPI) && p.assignment_api->retire,
            ErrorCode::unsupported, "invalid CUDA assignment retirement extension");
    }
    DeviceError error{};
    device_check(p.assignment_api->retire(p.device->session, p.last_assignment,
                                          query.face.assignment_epoch, &error),
                 error);
  }
  commit_begin_core(p, std::move(staged));
  p.assignment_mode = pooled ? Impl::AssignmentMode::pooled : Impl::AssignmentMode::legacy;
  if (new_assignment)
    p.last_assignment = query.face.assignment_epoch;
  p.pages_retired = false;
  p.query_roots.clear();
  p.query_roots.reserve(query.allowances.size());
  for (const auto &entry : query.allowances)
    p.query_roots.push_back(entry.root);
  std::sort(p.query_roots.begin(), p.query_roots.end(),
            [](auto a, auto b) { return a.value < b.value; });
  p.face = query.face;
  p.provenance_set = false;
  p.classified_domain.reset();
  p.classified = false;
  p.cover = {};
}
SurfaceProgress SurfaceCacheSession::step(JobContext &context) {
  auto &p = *impl_;
  p.stopped();
  context.budget.charge(0);
  if (p.facet_phase != Impl::FacetPhase::none) {
    if (p.facet_phase == Impl::FacetPhase::complete)
      return SurfaceProgress::facets_complete;
    return p.step_facets();
  }
  if (p.point_phase == Impl::PointPhase::complete)
    return SurfaceProgress::points_complete;
  if (p.point_phase == Impl::PointPhase::waiting) {
    if (!p.ready(p.sample_answers.data(), narrow(p.sample_answers.size())))
      return SurfaceProgress::pending;
    p.fold_samples();
    for (const auto &task : p.sample_tasks)
      if (p.point_stages[task.destination] > static_cast<uint32_t>(task.input.stage))
        p.point_reselect = true;
    p.point_phase = Impl::PointPhase::install;
  }
  if (p.point_phase == Impl::PointPhase::install) {
    if (p.operation == Impl::Operation::samples) {
      if (!p.ready(p.sample_answers.data(), narrow(p.sample_answers.size())))
        return SurfaceProgress::pending;
      for (const auto &task : p.sample_tasks)
        p.resident_stages[task.destination] = p.point_stages[task.destination];
    }
    if (p.point_reselect) {
      p.frontier.assign(p.selected.begin(), p.selected.end());
      p.selected.clear();
      p.cover = {};
      p.frozen.reset();
      p.phase = Impl::Phase::prepare;
      p.classified = true; // same retained classification; no geometry/domain change
    } else
      p.phase = Impl::Phase::done;
    p.point_phase = Impl::PointPhase::reselect;
  }
  if (p.candidate)
    return SurfaceProgress::needs_classification;
  if (p.phase == Impl::Phase::prepare && !p.classified) {
    p.candidate = p.make_candidates();
    return SurfaceProgress::needs_classification;
  }
  if (p.controls_pending) {
    if (!p.ready(nullptr, 0))
      return SurfaceProgress::pending;
    for (auto slot : p.control_nodes)
      p.nodes[slot].controls_ready = true;
    p.control_nodes.clear();
    p.restriction_tasks.clear();
    p.controls_pending = false;
  }
  if (p.phase == Impl::Phase::prepare && p.classified) {
    p.restriction_tasks.clear();
    p.control_nodes.clear();
    for (auto slot : p.frontier) {
      if (p.nodes[slot].controls_ready)
        continue;
      auto at = slot;
      while (p.nodes[at].parent != invalid_slot && !p.nodes[p.nodes[at].parent].controls_ready) {
        p.budget.charge(1);
        at = p.nodes[at].parent;
      }
      check(p.nodes[at].parent != invalid_slot, ErrorCode::internal_error,
            "surface original restriction root is not resident");
      p.control_nodes.push_back(at);
    }
    std::sort(p.control_nodes.begin(), p.control_nodes.end(),
              [&](auto a, auto b) { return p.less_node(a, b); });
    p.control_nodes.erase(std::unique(p.control_nodes.begin(), p.control_nodes.end()),
                          p.control_nodes.end());
    for (auto slot : p.control_nodes) {
      const auto &node = p.nodes[slot];
      const auto &parent = p.nodes[node.parent];
      p.restriction_tasks.push_back({node.root, parent.controls, node.controls.first,
                                     static_cast<SplitAxis>(node.step / 2), node.step & 1u});
    }
    if (!p.restriction_tasks.empty()) {
      p.counts.control_restrictions += p.restriction_tasks.size();
      if (p.device)
        p.counts.device_restrictions += p.restriction_tasks.size();
      p.submit(Impl::Operation::restrictions, narrow(p.restriction_tasks.size()));
      p.controls_pending = true;
      return SurfaceProgress::pending;
    }
  }
  const auto before = p.phase;
  const auto done = step_core(p, context) == StepState::done;
  if (before == Impl::Phase::prepare)
    p.classified = false;
  if (!done)
    return SurfaceProgress::progressed;
  if (!p.cover) {
    if (!p.required_targets.empty()) {
      std::pmr::unordered_map<uint32_t, uint8_t> active(&p.budget);
      active.reserve(p.frozen_point_ids.size());
      for (auto id : p.frozen_point_ids)
        active.emplace(id, uint8_t{1});
      for (const auto &target : p.required_targets) {
        const auto &root = p.source.roots()[target.root.value];
        const SampleKey key{root.source_slot, axis_key(root, 0, target.coordinate[0]),
                            axis_key(root, 1, target.coordinate[1])};
        const auto point = p.point_index.find(key);
        check(point != p.point_index.end() && active.contains(point->second), ErrorCode::unresolved,
              "forced packet target is not an active grid corner");
      }
      p.required_targets.clear();
    }
    auto data = std::allocate_shared<SurfaceBoundViewData>(
        RunAllocator<SurfaceBoundViewData>(p.resource), p.resource.get());
    data->selection = p.frozen;
    data->classified_domain = p.classified_domain;
    data->key = {p.frozen->key, p.face};
    data->prepared_roots.assign(p.query_roots.begin(), p.query_roots.end());
    for (auto slot : p.selected)
      data->classes.push_back(p.node_classes[slot]);
    p.cover = SurfaceBoundView(std::move(data));
  }
  if (p.point_phase == Impl::PointPhase::reselect) {
    check(p.table_ordinal < UINT32_MAX, ErrorCode::resource_exhausted,
          "surface point table ordinal exhausted");
    ExactBuilder values(p.resource,
                        {{OwnerKind::run, UINT32_MAX - 2, p.serial}, p.query, ++p.table_ordinal});
    std::pmr::vector<PointRecord> records(&p.budget);
    records.reserve(p.requested_points.size());
    for (auto id : p.requested_points) {
      const auto uv = p.exact_uv(p.points[id]);
      const std::array<ExactRef, 2> refs{values.append(uv[0]), values.append(uv[1])};
      const auto &sample = p.sample(id);
      records.push_back({{p.points[id].input.original, refs,
                          static_cast<ArithmeticStage>(p.point_stages[id]), p.point_stages[id] + 1},
                         refs,
                         refs,
                         {sample.actual_xyz[0], sample.actual_xyz[1], sample.actual_xyz[2]},
                         sample.departure_upper});
    }
    auto result = std::allocate_shared<SurfacePointResultData>(
        RunAllocator<SurfacePointResultData>(p.resource), p.resource, p.cover, p.point_ordinal,
        values.seal());
    result->points.assign(records.begin(), records.end());
    p.point_output = std::move(result);
    p.point_phase = Impl::PointPhase::complete;
    return SurfaceProgress::points_complete;
  }
  return SurfaceProgress::selected;
}
CandidateBoxes SurfaceCacheSession::candidates() const {
  check(impl_->candidate != nullptr, ErrorCode::invalid_source, "no surface candidate horizon");
  return impl_->candidate;
}
void SurfaceCacheSession::classify(CandidateClassification input) {
  auto &p = *impl_;
  p.stopped();
  check(p.candidate && input.candidates == p.candidate && input.labels.size() == p.frontier.size(),
        ErrorCode::invalid_source, "stale or partial surface classification");
  for (auto label : input.labels)
    check(label == SurfaceClass::outside || label == SurfaceClass::interior ||
              label == SurfaceClass::band,
          ErrorCode::invalid_source, "unknown surface classification");
  check(!p.provenance_set || input.domain == p.classified_domain, ErrorCode::invalid_source,
        "surface classification domain changed within query");
  if (input.domain) {
    const auto &key = input.domain->key;
    check(input.domain->resource == p.resource &&
              &input.domain->source.view() == &p.source.source().view() &&
              key.definition == p.face.definition && key.face == p.face.face &&
              key.source_generation == p.face.source_generation && key.query == p.query &&
              key.boundary_generation == p.face.boundary_generation &&
              key.chart_revision == p.face.chart_revision &&
              key.profile_revision == p.source.profile().revision,
          ErrorCode::invalid_source, "foreign surface classification domain");
  }
  p.provenance_set = true;
  p.classified_domain = input.domain;
  uint32_t retained = 0;
  for (uint32_t i = 0; i < p.frontier.size(); ++i) {
    p.node_classes[p.frontier[i]] = input.labels[i];
    if (input.labels[i] != SurfaceClass::outside)
      p.frontier[retained++] = p.frontier[i];
  }
  p.frontier.resize(retained);
  p.candidate.reset();
  p.classified = true;
}
SurfaceBoundView SurfaceCacheSession::selection() const {
  check(static_cast<bool>(impl_->cover), ErrorCode::invalid_source,
        "surface selection not complete");
  return impl_->cover;
}
void SurfaceCacheSession::refine(const SurfaceRefineRequest &input) {
  auto &p = *impl_;
  p.stopped();
  check(p.phase == Impl::Phase::done && p.cover && input.view &&
            input.view.selection() == p.frozen && input.view.key() == p.cover.key() && !p.jobs &&
            !p.ticket,
        ErrorCode::invalid_source, "stale or pending surface refinement horizon");
  const auto selection = p.frozen;
  check(p.selected.size() == selection->leaves.size(), ErrorCode::internal_error,
        "surface selected node/row mismatch");
  std::pmr::vector<uint8_t> affected(p.selected.size(), uint8_t{0}, &p.budget);
  std::pmr::vector<uint8_t> ordinary(p.selected.size(), uint8_t{0}, &p.budget);
  for (const auto &key : input.splits) {
    const auto at = std::lower_bound(selection->leaves.begin(), selection->leaves.end(), key,
                                     [](const auto &leaf, const auto &requested) {
                                       return std::tie(leaf.key.root.value, leaf.key.path.first) <
                                              std::tie(requested.root.value, requested.path.first);
                                     });
    check(at != selection->leaves.end() && at->key.root.value == key.root.value &&
              at->key.depth == key.depth && at->key.path.first == key.path.first &&
              at->key.path.count == key.path.count,
          ErrorCode::invalid_source, "unknown selected surface split");
    const auto found = narrow(static_cast<std::size_t>(at - selection->leaves.begin()));
    affected[found] = ordinary[found] = 1;
  }
  struct Target final {
    uint32_t leaf;
    std::array<Dyadic, 2> coordinate;
  };
  std::pmr::vector<Target> targets(&p.budget);
  auto value = [&](Dyadic coordinate) { return exact_parameter(p.budget, 0., 1., coordinate); };
  auto compare = [&](Dyadic a, Dyadic b) { return value(a).compare(value(b)); };
  for (const auto &request : input.packet_targets) {
    check(input.construction && input.construction->surface.key() == input.view.key() &&
              &input.construction->surface_source.view() == &p.source.view() &&
              request.surface == input.view.key() && request.leaf < selection->leaves.size() &&
              request.root.value == selection->leaves[request.leaf].key.root.value &&
              request.facet < input.construction->facets.size(),
          ErrorCode::invalid_source, "foreign surface packet target");
    const auto &facet = input.construction->facets[request.facet];
    check(facet.key == request.old_facet && facet.key.generator == FacetGenerator::packet,
          ErrorCode::invalid_source, "packet target does not name its captured facet");
    for (auto site : facet.sites)
      check(site < input.construction->sites.size(), ErrorCode::invalid_source,
            "packet target facet has a foreign site");
    Target target{request.leaf,
                  {Dyadic{request.numerator[0], request.depth[0]},
                   Dyadic{request.numerator[1], request.depth[1]}}};
    for (auto &coordinate : target.coordinate) {
      check(coordinate.exponent <= p.limits.max_axis_depth && coordinate.exponent <= 64 &&
                (coordinate.exponent == 64 ||
                 coordinate.numerator <= (uint64_t{1} << coordinate.exponent)),
            ErrorCode::unresolved, "packet target exceeds dyadic profile");
      while (coordinate.exponent && !(coordinate.numerator & 1u)) {
        coordinate.numerator >>= 1;
        --coordinate.exponent;
      }
    }
    for (uint32_t axis = 0; axis < 2; ++axis) {
      const auto &node = p.nodes[p.selected[request.leaf]];
      check(compare(target.coordinate[axis], node.coordinate[2 * axis]) > 0 &&
                compare(target.coordinate[axis], node.coordinate[2 * axis + 1]) < 0,
            ErrorCode::invalid_source, "packet target is outside its captured owning leaf");
    }
    const auto &root = p.source.roots()[request.root.value];
    std::array<Rational, 2> uv{exact_parameter(p.budget, root.prepared_box[0].lower,
                                               root.prepared_box[1].lower, target.coordinate[0]),
                               exact_parameter(p.budget, root.prepared_box[2].lower,
                                               root.prepared_box[3].lower, target.coordinate[1])};
    for (uint32_t i = 0; i < 3; ++i) {
      const auto &a = input.construction->sites[facet.sites[i]].uv;
      const auto &b = input.construction->sites[facet.sites[(i + 1) % 3]].uv;
      auto ax = import_exact(p.budget, input.construction->values.view(a[0]));
      auto ay = import_exact(p.budget, input.construction->values.view(a[1]));
      auto bx = import_exact(p.budget, input.construction->values.view(b[0]));
      auto by = import_exact(p.budget, input.construction->values.view(b[1]));
      check(bx.subtract(ax)
                    .multiply(uv[1].subtract(ay))
                    .subtract(by.subtract(ay).multiply(uv[0].subtract(ax)))
                    .sign() > 0,
            ErrorCode::invalid_source, "packet target is not strictly inside captured facet");
    }
    affected[target.leaf] = 1;
    targets.push_back(target);
  }
  std::sort(targets.begin(), targets.end(), [](const auto &a, const auto &b) {
    return std::tie(a.leaf, a.coordinate[0].exponent, a.coordinate[0].numerator,
                    a.coordinate[1].exponent, a.coordinate[1].numerator) <
           std::tie(b.leaf, b.coordinate[0].exponent, b.coordinate[0].numerator,
                    b.coordinate[1].exponent, b.coordinate[1].numerator);
  });
  targets.erase(std::unique(targets.begin(), targets.end(),
                            [](const auto &a, const auto &b) {
                              return a.leaf == b.leaf && a.coordinate == b.coordinate;
                            }),
                targets.end());
  std::pmr::vector<std::pair<uint32_t, uint32_t>> promoted(&p.budget);
  std::pmr::vector<uint8_t> marked(p.points.size(), uint8_t{0}, &p.budget);
  for (const auto &promotion : input.promotions) {
    uint32_t row = invalid_slot;
    for (uint32_t i = 0; i < selection->points.size(); ++i) {
      const auto &point = selection->points[i].key;
      if (point.original.value == promotion.point.original.value &&
          point.parameter == promotion.point.parameter && point.stage == promotion.point.stage &&
          point.constructed_generation == promotion.point.constructed_generation) {
        row = i;
        break;
      }
    }
    check(row != invalid_slot, ErrorCode::invalid_source,
          "unknown captured surface point promotion");
    const auto id = p.frozen_point_ids[row];
    const auto stage = static_cast<uint32_t>(promotion.demanded);
    check(stage <= 4 && stage > p.point_stages[id], ErrorCode::invalid_source,
          "surface promotion does not advance its current stage");
    promoted.emplace_back(id, stage);
    marked[id] = 1;
  }
  for (uint32_t i = 0; i < p.selected.size(); ++i)
    for (auto point : p.nodes[p.selected[i]].points)
      affected[i] |= marked[point];
  struct Cell final {
    NodeMeta node;
    uint32_t actual, parent = invalid_slot;
    uint32_t axis = invalid_slot;
    std::array<uint32_t, 2> children{invalid_slot, invalid_slot};
    bool ordinary;
    std::pmr::vector<uint32_t> targets;
    Cell(NodeMeta metadata, uint32_t slot, bool split, std::pmr::memory_resource *owner)
        : node(metadata), actual(slot), ordinary(split), targets(owner) {}
  };
  std::pmr::vector<Cell> cells(&p.budget);
  uint32_t target_cursor = 0;
  for (uint32_t i = 0; i < p.selected.size(); ++i) {
    if (!affected[i])
      continue;
    cells.emplace_back(p.nodes[p.selected[i]], p.selected[i], ordinary[i] != 0, &p.budget);
    while (target_cursor < targets.size() && targets[target_cursor].leaf < i)
      ++target_cursor;
    while (target_cursor < targets.size() && targets[target_cursor].leaf == i)
      cells.back().targets.push_back(target_cursor++);
  }
  check(!cells.empty(), ErrorCode::invalid_source, "empty surface refinement horizon");
  uint64_t missing_nodes = 0, missing_controls = 0;
  for (uint32_t at = 0; at < cells.size(); ++at) {
    auto remaining = [&](uint32_t axis) {
      uint32_t result = 0;
      for (auto id : cells[at].targets) {
        const auto t = targets[id].coordinate[axis];
        if (compare(t, cells[at].node.coordinate[2 * axis]) == 0 ||
            compare(t, cells[at].node.coordinate[2 * axis + 1]) == 0)
          continue;
        const auto depth = axis ? cells[at].node.depth_v : cells[at].node.depth_u;
        check(t.exponent > depth, ErrorCode::internal_error, "target grid depth mismatch");
        result = std::max(result, t.exponent - depth);
      }
      return result;
    };
    const auto u = remaining(0), v = remaining(1);
    uint32_t axis = invalid_slot;
    if (u || v)
      axis = v > u ? 1u : 0u;
    else if (cells[at].ordinary) {
      const auto fair = p.fair_axis(cells[at].node);
      const auto bounds = p.active_bounds[cells[at].actual];
      axis = fair < 2 ? fair : bounds.score_v > bounds.score_u ? 1u : 0u;
    }
    if (axis == invalid_slot)
      continue;
    const auto parent = cells[at].node;
    check((axis ? parent.depth_v : parent.depth_u) < p.limits.max_axis_depth, ErrorCode::unresolved,
          "forced packet target path exceeds depth cap");
    const auto mid = midpoint(parent.coordinate[2 * axis], parent.coordinate[2 * axis + 1]);
    cells[at].axis = axis;
    std::array<uint32_t, 2> children{};
    for (uint32_t side = 0; side < 2; ++side) {
      auto child = parent;
      child.depth_u += axis == 0;
      child.depth_v += axis == 1;
      child.coordinate[2 * axis + (side == 0 ? 1u : 0u)] = mid;
      const auto actual = cells[at].actual == invalid_slot
                              ? invalid_slot
                              : p.nodes[cells[at].actual].children[2 * axis + side];
      children[side] = narrow(cells.size());
      cells.emplace_back(child, actual, false, &p.budget);
      cells.back().parent = at;
      if (actual == invalid_slot) {
        ++missing_nodes;
        const auto &root = p.source.roots()[child.root.value];
        if (root.family == GeometryFamily::rational_tensor)
          missing_controls += uint64_t{(root.degree_u + 1) * (root.degree_v + 1) * 4};
      }
    }
    cells[at].children = children;
    for (auto id : cells[at].targets) {
      const auto side = compare(targets[id].coordinate[axis], mid) <= 0 ? 0u : 1u;
      cells[children[side]].targets.push_back(id);
    }
    check(missing_nodes <= p.limits.nodes - p.nodes.size() &&
              missing_controls <= p.limits.control_scalars - p.controls.size(),
          ErrorCode::resource_exhausted, "forced packet horizon exceeds node/control capacity");
  }
  uint64_t frontier_count = 0;
  std::pmr::vector<SampleKey> needed_points(&p.budget);
  for (const auto &cell : cells) {
    if (cell.axis != invalid_slot)
      continue;
    ++frontier_count;
    const auto &root = p.source.roots()[cell.node.root.value];
    for (uint32_t corner = 0; corner < 4; ++corner) {
      SampleKey key{root.source_slot, axis_key(root, 0, cell.node.coordinate[corner & 1u]),
                    axis_key(root, 1, cell.node.coordinate[2 + (corner >> 1)])};
      if (!p.point_index.contains(key))
        needed_points.push_back(key);
    }
  }
  std::sort(needed_points.begin(), needed_points.end());
  needed_points.erase(std::unique(needed_points.begin(), needed_points.end()), needed_points.end());
  check(frontier_count <= p.limits.frontier &&
            needed_points.size() <= p.limits.points - p.points.size(),
        ErrorCode::resource_exhausted, "forced packet horizon exceeds frontier/point capacity");
  p.nodes.reserve(p.nodes.size() + missing_nodes);
  p.active_bounds.reserve(p.nodes.size() + missing_nodes);
  p.controls.reserve(p.controls.size() + missing_controls);
  p.points.reserve(p.points.size() + needed_points.size());
  p.point_stages.reserve(p.points.size() + needed_points.size());
  p.resident_stages.reserve(p.points.size() + needed_points.size());
  p.point_index.reserve(p.points.size() + needed_points.size());
  p.frontier.reserve(frontier_count);
  p.next.reserve(cells.size());
  p.required_targets.reserve(targets.size());
  for (const auto &target : targets)
    p.required_targets.push_back({selection->leaves[target.leaf].key.root, target.coordinate});
  // Commit only after the complete multi-target/ordinary/promotion horizon fits.
  p.cover = {};
  p.phase = Impl::Phase::prepare;
  p.classified = false;
  for (const auto &[id, stage] : promoted)
    p.point_stages[id] = std::max(p.point_stages[id], stage);
  p.next.clear();
  for (uint32_t at = 0; at < cells.size(); ++at) {
    auto &cell = cells[at];
    if (cell.axis == invalid_slot)
      continue;
    p.split(cell.actual, static_cast<SplitAxis>(cell.axis));
    for (uint32_t side = 0; side < 2; ++side)
      cells[cell.children[side]].actual = p.nodes[cell.actual].children[2 * cell.axis + side];
  }
  p.frontier.clear();
  for (const auto &cell : cells)
    if (cell.axis == invalid_slot)
      p.frontier.push_back(cell.actual);
  uint32_t keep = 0;
  for (uint32_t i = 0; i < p.selected.size(); ++i)
    if (!affected[i])
      p.selected[keep++] = p.selected[i];
  p.selected.resize(keep);
  std::sort(p.frontier.begin(), p.frontier.end(),
            [&](auto a, auto b) { return p.less_node(a, b); });
}
void SurfaceCacheSession::sample(const SurfacePointRequest &request) {
  auto &p = *impl_;
  p.stopped();
  check(p.phase == Impl::Phase::done && p.cover && request.view &&
            request.view.key() == p.cover.key() &&
            request.view.selection() == p.cover.selection() &&
            p.point_phase == Impl::PointPhase::none && p.facet_phase == Impl::FacetPhase::none &&
            !p.jobs && !p.ticket && request.values.resource() == p.resource && request.ordinal,
        ErrorCode::invalid_source, "stale surface point request");
  check(request.points.size() <= p.limits.frontier * uint64_t{4}, ErrorCode::resource_exhausted,
        "surface point request cap");
  // Validate the complete request before installing points or demanded stages.
  for (const auto &row : request.points) {
    check(static_cast<uint32_t>(row.demanded) <= 4, ErrorCode::invalid_source,
          "invalid surface point stage");
    for (auto ref : row.original_uv)
      (void)request.values.view(ref);
  }
  const auto carrier = p.source.source().view().carrier_index.find(
      p.source.source().view().faces[p.face.face.value].carrier_id);
  struct Planned final {
    uint64_t hash;
    std::array<Rational, 2> uv;
  };
  std::pmr::vector<Planned> planned(&p.budget);
  planned.reserve(request.points.size());
  for (const auto &row : request.points) {
    std::array<Rational, 2> uv{import_exact(p.budget, request.values.view(row.original_uv[0])),
                               import_exact(p.budget, request.values.view(row.original_uv[1]))};
    (void)find_carrier_root_slot(p.source.roots(), carrier, uv, RootBoxKind::prepared);
    planned.push_back({Impl::hash_uv(carrier.value, uv), std::move(uv)});
  }
  std::pmr::vector<uint32_t> order(&p.budget);
  for (uint32_t i = 0; i < planned.size(); ++i)
    order.push_back(i);
  std::sort(order.begin(), order.end(), [&](auto a, auto b) {
    if (planned[a].hash != planned[b].hash)
      return planned[a].hash < planned[b].hash;
    const auto x = planned[a].uv[0].compare(planned[b].uv[0]);
    return x ? x < 0 : planned[a].uv[1].compare(planned[b].uv[1]) < 0;
  });
  uint64_t missing = 0;
  for (uint32_t i = 0; i < order.size(); ++i) {
    const auto &row = planned[order[i]];
    if (i && row.uv[0].compare(planned[order[i - 1]].uv[0]) == 0 &&
        row.uv[1].compare(planned[order[i - 1]].uv[1]) == 0)
      continue;
    const auto root =
        find_carrier_root_slot(p.source.roots(), carrier, row.uv, RootBoxKind::prepared);
    const auto &r = p.source.roots()[root];
    std::array<std::optional<Dyadic>, 2> coordinates;
    for (uint32_t axis = 0; axis < 2; ++axis) {
      const auto begin = Rational::from_double(p.budget, r.prepared_box[2 * axis].lower);
      coordinates[axis] = p.compact(row.uv[axis].subtract(begin).divide(
          Rational::from_double(p.budget, r.prepared_box[2 * axis + 1].lower).subtract(begin)));
    }
    bool hit = false;
    if (coordinates[0] && coordinates[1]) {
      const SampleKey key{r.source_slot, axis_key(r, 0, *coordinates[0]),
                          axis_key(r, 1, *coordinates[1])};
      hit = p.point_index.contains(key);
    } else {
      const auto range = p.absolute_index.equal_range(row.hash);
      for (auto at = range.first; at != range.second; ++at) {
        const auto uv = p.exact_uv(p.points[at->second]);
        if (uv[0].compare(row.uv[0]) == 0 && uv[1].compare(row.uv[1]) == 0) {
          hit = true;
          break;
        }
      }
    }
    if (!hit)
      ++missing;
  }
  check(missing <= p.limits.points - p.points.size(), ErrorCode::resource_exhausted,
        "surface exact point horizon exceeds capacity");
  const auto capacity = p.points.size() + missing;
  p.points.reserve(capacity);
  p.point_stages.reserve(capacity);
  p.resident_stages.reserve(capacity);
  p.point_index.reserve(capacity);
  p.absolute_index.reserve(p.absolute_index.size() + missing);
  p.sample_tasks.reserve(order.size());
  p.sample_answers.reserve(order.size());
  p.requested_points.reserve(order.size());
  std::pmr::vector<uint32_t> ids(&p.budget);
  ids.reserve(request.points.size());
  for (const auto &row : request.points) {
    std::array<Rational, 2> uv{import_exact(p.budget, request.values.view(row.original_uv[0])),
                               import_exact(p.budget, request.values.view(row.original_uv[1]))};
    ids.push_back(p.absolute_point(uv, request.values, row.original_uv));
  }
  std::pmr::unordered_map<uint32_t, uint32_t> stages(&p.budget);
  for (uint32_t i = 0; i < ids.size(); ++i) {
    auto [it, fresh] = stages.emplace(ids[i], p.point_stages[ids[i]]);
    (void)fresh;
    it->second = std::max(it->second, static_cast<uint32_t>(request.points[i].demanded));
  }
  p.point_reselect = false;
  for (auto id : p.frozen_point_ids) {
    const auto found = stages.find(id);
    if (found != stages.end() && found->second > p.point_stages[id])
      p.point_reselect = true;
  }
  p.sample_tasks.clear();
  p.sample_answers.clear();
  // Point order and completion never select the demanded tier.
  std::pmr::vector<uint32_t> unique(&p.budget);
  for (const auto &[id, stage] : stages) {
    (void)stage;
    unique.push_back(id);
  }
  std::sort(unique.begin(), unique.end());
  for (auto id : unique) {
    const auto stage = stages.at(id);
    if (stage > p.point_stages[id])
      p.point_reselect = true;
    auto &point = p.points[id];
    if (!(point.valid & (1u << stage)) && stage) {
      auto uv = p.exact_uv(point);
      point.tiers[stage] = evaluate_sample_precise(p.source, point.input.original, uv,
                                                   static_cast<ArithmeticStage>(stage), p.budget);
      ++p.counts.exact_fallbacks;
      point.valid |= 1u << stage;
    }
    p.point_stages[id] = stage;
    DeviceSample task{};
    task.destination = id;
    task.input = point.input;
    task.input.stage = static_cast<ArithmeticStage>(stage);
    if (point.valid & (1u << stage)) {
      ++p.counts.point_hits;
      if (!p.device || p.resident_stages[id] == stage)
        continue;
      task.supplied = 1;
      task.supplied_result = point.tiers[stage];
    }
    p.sample_tasks.push_back(task);
  }
  p.requested_points.assign(ids.begin(), ids.end());
  p.point_ordinal = request.ordinal;
  p.sample_answers.resize(p.sample_tasks.size());
  p.submit(Impl::Operation::samples, narrow(p.sample_tasks.size()));
  p.point_phase = Impl::PointPhase::waiting;
}
SurfacePointResult SurfaceCacheSession::point_result() const {
  check(impl_->point_phase == Impl::PointPhase::complete && impl_->point_output,
        ErrorCode::invalid_source, "surface point operation incomplete");
  return impl_->point_output;
}
void SurfaceCacheSession::release_operation() {
  auto &p = *impl_;
  check(
      p.phase == Impl::Phase::done && !p.ticket && !p.jobs &&
          (p.point_phase == Impl::PointPhase::none ||
           p.point_phase == Impl::PointPhase::complete) &&
          (p.facet_phase == Impl::FacetPhase::none || p.facet_phase == Impl::FacetPhase::complete),
      ErrorCode::invalid_source, "surface operation incomplete");
  p.facet_output.reset();
  p.facet_work.reset();
  p.facet_request.reset();
  p.facet_phase = Impl::FacetPhase::none;
  p.point_output.reset();
  p.point_phase = Impl::PointPhase::none;
  p.requested_points.clear();
}
void SurfaceCacheSession::bound(const SurfaceFacetRequest &request) {
  auto &p = *impl_;
  p.stopped();
  check(p.phase == Impl::Phase::done && p.cover && request.view &&
            request.view.key() == p.cover.key() &&
            request.view.selection() == p.cover.selection() &&
            p.point_phase == Impl::PointPhase::none && p.facet_phase == Impl::FacetPhase::none &&
            !p.jobs && !p.ticket && request.ordinal && request.construction && request.domain &&
            request.construction->surface && request.construction->surface.key() == p.cover.key() &&
            request.construction->surface.selection() == p.cover.selection() &&
            request.construction->domain == request.domain &&
            request.allowances.values.resource() == p.resource,
        ErrorCode::invalid_source, "stale surface facet request");
  check(request.limits.batch_records && request.limits.batch_records <= p.limits.frontier &&
            request.limits.precise_tasks && request.limits.work && request.limits.pieces &&
            request.limits.exact_entries && request.limits.candidate_visits,
        ErrorCode::invalid_source, "invalid surface facet limits");
  const auto tau =
      import_exact(p.budget, request.allowances.values.view(request.allowances.face_position));
  check(tau.sign() > 0, ErrorCode::invalid_source, "nonpositive surface facet allowance");
  std::pmr::vector<uint32_t> unique(request.selected_facets.begin(), request.selected_facets.end(),
                                    &p.budget);
  for (auto index : unique)
    check(index < request.construction->facets.size(), ErrorCode::invalid_source,
          "surface facet subset index outside captured construction");
  std::sort(unique.begin(), unique.end());
  check(std::adjacent_find(unique.begin(), unique.end()) == unique.end(), ErrorCode::invalid_source,
        "duplicate surface facet subset index");
  p.selected_facets.assign(request.selected_facets.begin(), request.selected_facets.end());
  p.facet_request = request;
  p.facet_request->selected_facets = p.selected_facets;
  p.precise_count = 0;
  p.facet_work_spent = 0;
  p.bound_pending = false;
  p.facet_phase = Impl::FacetPhase::prepare;
}
SurfaceFacetResult SurfaceCacheSession::facet_result() const {
  check(impl_->facet_phase == Impl::FacetPhase::complete && impl_->facet_output,
        ErrorCode::invalid_source, "surface facet operation incomplete");
  return impl_->facet_output;
}
CacheStats SurfaceCacheSession::stats() const noexcept { return stats_core(*impl_); }
void SurfaceCacheSession::release_query() {
  auto &p = *impl_;
  check(p.phase == Impl::Phase::done, ErrorCode::invalid_source, "surface operation incomplete");
  release_operation();
  p.frozen.reset();
  p.cover = {};
  p.query_values.reset();
  p.phase = Impl::Phase::idle;
}
void SurfaceCacheSession::retire_assignment() {
  auto &p = *impl_;
  p.stopped();
  check(p.assignment_mode != Impl::AssignmentMode::legacy &&
            (p.phase == Impl::Phase::idle || p.phase == Impl::Phase::done) && !p.jobs && !p.ticket,
        ErrorCode::invalid_source, "surface assignment retirement is not drained");
  if (p.phase == Impl::Phase::done)
    release_query();
  else
    check(p.point_phase == Impl::PointPhase::none && p.facet_phase == Impl::FacetPhase::none,
          ErrorCode::invalid_source, "idle surface owner retains an auxiliary operation");
  retire_core_pages(p);
  p.cover = {};
  p.candidate.reset();
  p.classified_domain.reset();
  p.classified = false;
  p.provenance_set = false;
  p.controls_pending = false;
  p.control_nodes.clear();
  p.node_classes.clear();
  p.required_targets.clear();
  p.query_roots.clear();
  p.absolute_index.clear();
  p.requested_points.clear();
  p.selected_facets.clear();
  p.piece_queue.clear();
  p.plane_queue.clear();
  p.batch_indices.clear();
  p.piece_accepted.clear();
  p.plane_accepted.clear();
  p.source_inputs.clear();
  p.source_answers.clear();
  p.pages_retired = true;
}
void SurfaceCacheSession::cancel() noexcept {
  impl_->cancelled.store(true, std::memory_order_relaxed);
  impl_->cancellation.request_stop();
  if (impl_->workers)
    impl_->workers->stop();
}
void SurfaceCacheSession::close() { close_core(*impl_); }
} // namespace bgmesh::adaptive
