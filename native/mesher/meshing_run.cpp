#include "meshing_run.hpp"
#include "boundary_evaluation.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <deque>
#include <map>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void internal(const char *message) { throw Error(ErrorCode::internal_error, message); }
bool same_xyz(const Vec3 &a, const Vec3 &b) {
  for (std::size_t i = 0; i < 3; ++i)
    if (std::bit_cast<uint64_t>(a[i]) != std::bit_cast<uint64_t>(b[i]))
      return false;
  return true;
}
uint64_t mix(uint64_t value) {
  value ^= value >> 30;
  value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 27;
  value *= 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}
class NodeRegistry final {
  struct Row final { PhysicalNodeKey key; Vec3 xyz; };
  struct Bucket final { uint64_t hash = 0; uint32_t row = UINT32_MAX; };
  bgfd::Budget &budget_;
  std::pmr::vector<Row> rows_;
  std::pmr::vector<Bucket> buckets_;
  static uint64_t hash(const PhysicalNodeKey &key) {
    uint64_t h = mix((uint64_t{key.definition} << 32) | static_cast<uint32_t>(key.generator));
    h = mix(h ^ key.source);
    h = mix(h ^ key.generation);
    return mix(h ^ key.ordinal);
  }
  void insert(std::pmr::vector<Bucket> &table, Bucket item) {
    const auto mask = table.size() - 1;
    auto at = static_cast<std::size_t>(item.hash) & mask;
    std::size_t distance = 0;
    for (;;) {
      budget_.charge(1);
      auto &occupied = table[at];
      if (occupied.row == UINT32_MAX) {
        occupied = item;
        return;
      }
      const auto existing_distance = (at - (static_cast<std::size_t>(occupied.hash) & mask)) & mask;
      if (existing_distance < distance) {
        std::swap(occupied, item);
        distance = existing_distance;
      }
      at = (at + 1) & mask;
      ++distance;
    }
  }
public:
  explicit NodeRegistry(bgfd::Budget &budget) : budget_(budget), rows_(&budget), buckets_(&budget) {}
  NodeSlot find_or_insert(const PhysicalNode &value, bool existing) {
    budget_.charge(1);
    const auto h = hash(value.key);
    if (!buckets_.empty()) {
      const auto mask = buckets_.size() - 1;
      auto at = static_cast<std::size_t>(h) & mask;
      std::size_t distance = 0;
      while (buckets_[at].row != UINT32_MAX) {
        budget_.charge(1);
        const auto &item = buckets_[at];
        if (((at - (static_cast<std::size_t>(item.hash) & mask)) & mask) < distance)
          break;
        if (item.hash == h && rows_[item.row].key == value.key) {
          if (!same_xyz(rows_[item.row].xyz, value.xyz))
            invalid("canonical physical key has inconsistent XYZ words");
          return {item.row};
        }
        at = (at + 1) & mask;
        ++distance;
      }
    }
    if (existing)
      internal("fixed-site optimizer introduced an unknown physical node");
    if (rows_.size() >= UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "definition physical node slots exhausted");
    if (buckets_.empty() || rows_.size() + 1 > buckets_.size() - buckets_.size() / 4) {
      const auto capacity = buckets_.empty() ? std::size_t{16} : buckets_.size() * 2;
      if (capacity < buckets_.size() || capacity > buckets_.max_size())
        throw Error(ErrorCode::resource_exhausted, "physical node index capacity exhausted");
      std::pmr::vector<Bucket> next(capacity, Bucket{}, &budget_);
      for (const auto bucket : buckets_)
        if (bucket.row != UINT32_MAX) {
          budget_.charge(1);
          insert(next, bucket);
        }
      buckets_.swap(next);
    }
    const auto row = static_cast<uint32_t>(rows_.size());
    rows_.push_back({value.key, value.xyz});
    insert(buckets_, {h, row});
    return {row};
  }
};
class ContactPairJob final : public NativeTask {
  ContactPairBatch input_;
  std::optional<ContactPairCompleted> output_;
public:
  ContactPairJob(NativeTaskKey identity, ContactPairBatch input)
      : NativeTask(identity), input_(std::move(input)) {}
  StepState step(JobContext &job) override {
    if (output_)
      internal("contact child ran twice");
    output_ = classify_contact_batch(job.budget, input_);
    return StepState::done;
  }
  const ContactPairCompleted &result() const {
    if (!output_)
      internal("contact child did not finish");
    return *output_;
  }
};
struct Root final {
  PreparedRun run;
  NativeScheduler &scheduler;
  const MeshingRunLimits &limits;
  std::shared_ptr<ResourceReservation> lease;
  bgfd::Budget budget;
  MeshingRunStats stats;
  std::pmr::vector<std::shared_ptr<NumericalServiceJob>> services;
  uint64_t child_serial = 0;
  Root(PreparedRun input, NativeScheduler &executor, const MeshingRunLimits &cap)
      : run(std::move(input)), scheduler(executor), limits(cap),
        lease(reserve_resource(run->resource, cap.coordinator_bytes)),
        budget({cap.coordinator_bytes, cap.coordinator_work}, lease.get(), scheduler.stop_token()),
        services(&budget) {
    if (!cap.numerical_services || !cap.point_batch || !cap.contact_batch ||
        cap.face.cell_batch > cap.numerical.cell_capacity ||
        cap.preparation.batch.task_capacity > cap.numerical.runtime.task_capacity ||
        cap.preparation.batch.table_capacity > cap.numerical.runtime.table_capacity ||
        (cap.cuda.has_value() != !cap.devices.empty()))
      invalid("inconsistent whole-mesher capacities or device configuration");
    if (!scheduler.idle())
      invalid("whole mesher requires an empty scheduler");
    services.reserve(cap.numerical_services);
    for (uint32_t i = 0; i < cap.numerical_services; ++i) {
      auto service = std::allocate_shared<NumericalServiceJob>(
          RunAllocator<NumericalServiceJob>(run->resource), run->resource,
          NativeTaskKey{{OwnerKind::run, UINT32_MAX, uint64_t{i} + 1}, 0, 0}, run->bank,
          cap.numerical, scheduler.stop_token());
      if (cap.cuda && run->bank.size())
        service->open_device(*cap.cuda, cap.devices[i % cap.devices.size()]);
      services.push_back(std::move(service));
    }
  }
  uint64_t next_child() {
    budget.charge(1);
    if (child_serial == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "whole mesher child ordinal exhausted");
    return ++child_serial;
  }
  std::pmr::vector<StepCompletion> wave(std::span<const std::shared_ptr<NativeTask>> tasks) {
    if (tasks.empty() || tasks.size() > scheduler.limits().task_capacity)
      internal("invalid coordinator worker wave");
    for (const auto &task : tasks)
      scheduler.submit(task);
    scheduler.drain();
    std::pmr::vector<StepCompletion> completed(&budget);
    while (auto item = scheduler.take()) {
      if (std::find(tasks.begin(), tasks.end(), item->task) == tasks.end())
        internal("scheduler returned an unrelated caller task");
      stats.scheduler_work += item->exact_work;
      stats.scheduler_scratch_peak = std::max(stats.scheduler_scratch_peak, item->scratch_peak);
      if (const auto face = std::dynamic_pointer_cast<FaceOwnerJob>(item->task)) {
        const auto cost = face->take_owner_work();
        stats.owner_work += cost.exact_work;
        stats.owner_peak = std::max(stats.owner_peak, cost.peak_bytes);
      } else if (const auto prep = std::dynamic_pointer_cast<FacePreparationJob>(item->task)) {
        const auto cost = prep->take_owner_work();
        stats.owner_work += cost.exact_work;
        stats.owner_peak = std::max(stats.owner_peak, cost.peak_bytes);
      }
      completed.push_back(std::move(*item));
    }
    if (completed.size() != tasks.size())
      internal("scheduler omitted a submitted task completion");
    std::sort(completed.begin(), completed.end(),
              [](const auto &a, const auto &b) { return a.task->key < b.task->key; });
    for (const auto &item : completed)
      item.rethrow();
    return completed;
  }
  StepState execute(const std::shared_ptr<NativeTask> &task) {
    const std::array tasks{task};
    for (;;) {
      const auto completed = wave(tasks);
      if (completed[0].state != StepState::ready)
        return completed[0].state;
    }
  }
  NumericalOutput evaluate(NumericalInput input) {
    const NativeTaskKey key{{OwnerKind::run, UINT32_MAX, 0}, 0, next_child()};
    const auto &service = services[0];
    service->begin({key, std::move(input)});
    if (execute(service) != StepState::done || service->result().key != key)
      internal("numerical service returned a foreign coordinator request");
    auto result = service->result().output;
    service->release_result();
    return result;
  }
  void close_services() {
    for (const auto &service : services) {
      const auto cost = service->stats();
      stats.numerical_cpu_tasks += cost.runtime.cpu_tasks;
      stats.numerical_device_tasks += cost.runtime.device_tasks;
      stats.numerical_exact_tasks += cost.runtime.exact_tasks;
      stats.numerical_work += cost.runtime.charged_work;
      service->close();
    }
    services.clear();
  }
};
struct FaceState final {
  Slot<FaceTag> source;
  uint64_t generation = 0, boundary_generation = 0;
  SourceBoundarySnapshot snapshot;
  std::shared_ptr<FacePreparationJob> preparation;
  std::shared_ptr<FaceOwnerJob> owner;
  std::optional<BoundaryKey> request;
  FaceHorizon processed;
  std::pmr::vector<std::optional<ContactFacetInput>> facets;
  explicit FaceState(Slot<FaceTag> slot, bgfd::Budget &budget) : source(slot), facets(&budget) {}
  ContactFacetInput *find(TriangleHandle key) {
    if (key.slot >= facets.size())
      return nullptr;
    auto &row = facets[key.slot];
    return row && row->cell.triangle == key ? &*row : nullptr;
  }
  void insert(const ContactFacetInput &input) {
    const auto slot = input.cell.triangle.slot;
    if (slot == UINT32_MAX)
      internal("contact publication has an invalid face triangle slot");
    if (slot >= facets.size())
      facets.resize(static_cast<std::size_t>(slot) + 1);
    auto &current = facets[slot];
    if (current && current->cell.triangle != input.cell.triangle)
      internal("face contact slot changed generation without retirement");
    current = input;
  }
  void erase(TriangleHandle key) {
    if (find(key))
      facets[key.slot].reset();
  }
};
enum class RouteKind { preparation, face, candidate, service };
struct ReadyTask final {
  std::shared_ptr<NativeTask> task;
  RouteKind kind;
  uint32_t index;
};
struct NumericPending final {
  uint32_t face;
  bool preparation;
  NumericalInput input;
};
struct NumericRoute final {
  uint32_t face;
  bool preparation;
  NativeTaskKey request;
};
struct DefinitionOwner final {
  Root &root;
  Slot<DefinitionTag> definition;
  DefinitionBoundaryForest forest;
  ContactEpochOwner contacts;
  std::pmr::vector<FaceState> faces;
  NodeRegistry node_registry;
  std::pmr::vector<uint64_t> facet_generations;
  std::pmr::vector<uint32_t> free_facets;
  DefinitionOwner(Root &parent, Slot<DefinitionTag> slot)
      : root(parent), definition(slot),
        forest(root.budget, root.run->geometry.view().source, slot, root.run->resource),
        contacts(root.budget, root.run->resource, root.run->geometry.view().source, slot,
                 root.limits.contacts), faces(&root.budget), node_registry(root.budget),
        facet_generations(&root.budget), free_facets(&root.budget) {
    for (const auto face : source().definition_faces.at(slot.value))
      faces.emplace_back(Slot<FaceTag>{face}, root.budget);
    std::sort(faces.begin(), faces.end(), [&](const auto &a, const auto &b) {
      return source().faces[a.source.value].id < source().faces[b.source.value].id;
    });
  }
  const SourceData &source() const { return root.run->geometry.view().source.view(); }
  uint32_t face_index(Slot<FaceTag> face) const {
    const auto id = source().faces[face.value].id;
    const auto found = std::lower_bound(faces.begin(), faces.end(), id, [&](const auto &a, auto b) {
      return source().faces[a.source.value].id < b;
    });
    if (found == faces.end() || found->source != face)
      internal("definition update named a foreign source face");
    return static_cast<uint32_t>(found - faces.begin());
  }
  FaceFrontierIdentity identity(const FaceState &face) const {
    return {{OwnerKind::face, definition.value, source().faces[face.source.value].id.value},
            face.generation, face.boundary_generation};
  }
  BoundaryUpdate evaluate_boundary(const BoundaryEvaluationBatch &batch) {
    std::pmr::vector<BoundaryEvaluationResult> results(&root.budget);
    results.reserve(batch.tasks().size());
    uint64_t first = 0;
    while (first < batch.tasks().size()) {
      std::pmr::vector<std::shared_ptr<NativeTask>> work(&root.budget);
      while (first < batch.tasks().size() && work.size() < root.scheduler.limits().threads) {
        const auto count = std::min<uint64_t>(root.limits.point_batch, batch.tasks().size() - first);
        auto child = std::allocate_shared<BoundaryPointJob>(
            RunAllocator<BoundaryPointJob>(root.run->resource), root.run->resource,
            root.run->geometry, batch, Range<BoundaryEvaluationTask>{first, count},
            // Immutable chunks have independent arithmetic owners. Their
            // payload retains the original source/definition/epoch identity.
            NativeTaskKey{{OwnerKind::boundary, definition.value, root.next_child()},
                          batch.epoch().value, 0});
        work.push_back(std::move(child));
        first += count;
      }
      // Ordinals follow the original task ranges; complete_evaluations requires
      // that full order, independent of worker completion order.
      const auto completed = root.wave(work);
      for (const auto &item : completed) {
        const auto child = std::static_pointer_cast<BoundaryPointJob>(item.task);
        if (item.state != StepState::done)
          internal("boundary point child failed to finish");
        const auto points = child->results();
        results.insert(results.end(), points.begin(), points.end());
      }
    }
    root.stats.boundary_points += results.size();
    return forest.complete_evaluations(batch.epoch(), results);
  }
  void prepare(uint32_t at) {
    auto &face = faces[at];
    face.snapshot = forest.freeze_face_boundary(face.source);
    face.boundary_generation = face.snapshot->boundary_generation;
    face.request.reset();
    face.owner.reset();
    face.preparation = std::allocate_shared<FacePreparationJob>(
        RunAllocator<FacePreparationJob>(root.run->resource), root.run->resource,
        FacePreparationInput{root.run, face.snapshot, face.generation, root.limits.seed,
                             root.limits.allow_plane_profile},
        root.limits.preparation, root.scheduler.stop_token());
  }
  void drive(std::pmr::deque<ReadyTask> ready) {
    std::pmr::deque<NumericPending> pending(&root.budget);
    std::pmr::vector<std::optional<NumericRoute>> active(root.services.size(), {}, &root.budget);
    for (;;) {
      for (uint32_t i = 0; i < root.services.size() && !pending.empty(); ++i) {
        if (active[i])
          continue;
        auto request = std::move(pending.front());
        pending.pop_front();
        const auto &face = faces[request.face];
        const NativeTaskKey key{identity(face).owner, face.generation, root.next_child()};
        active[i] = NumericRoute{request.face, request.preparation, key};
        root.services[i]->begin({key, std::move(request.input)});
        ready.push_back({root.services[i], RouteKind::service, i});
      }
      if (ready.empty()) {
        if (!pending.empty() || std::any_of(active.begin(), active.end(), [](const auto &v) { return v.has_value(); }))
          internal("whole face coordinator lost a queued continuation");
        return;
      }
      std::pmr::vector<ReadyTask> routes(&root.budget);
      std::pmr::vector<std::shared_ptr<NativeTask>> tasks(&root.budget);
      while (!ready.empty() && tasks.size() < root.scheduler.limits().task_capacity) {
        auto value = std::move(ready.front());
        ready.pop_front();
        tasks.push_back(value.task);
        routes.push_back(std::move(value));
      }
      for (const auto &completion : root.wave(tasks)) {
        const auto found = std::find_if(routes.begin(), routes.end(), [&](const auto &r) {
          return r.task == completion.task;
        });
        if (found == routes.end())
          internal("worker completion lost its coordinator route");
        const auto route = *found;
        if (completion.state == StepState::ready) {
          ready.push_back(route);
          continue;
        }
        if (route.kind == RouteKind::service) {
          const auto current = active[route.index];
          if (!current || completion.state != StepState::done)
            internal("numerical service has no current face request");
          const auto &service = root.services[route.index];
          const auto &reply = service->result();
          if (reply.key != current->request)
            internal("numerical service echoed a foreign request");
          auto &face = faces[current->face];
          if (current->preparation) {
            face.preparation->finish_curves(std::get<BoundCompleted>(reply.output));
            ready.push_back({face.preparation, RouteKind::preparation, current->face});
          } else {
            face.owner->finish_cells(std::get<std::shared_ptr<const CellBatchResult>>(reply.output)->cells);
            ready.push_back({face.owner, RouteKind::face, current->face});
          }
          service->release_result();
          active[route.index].reset();
          continue;
        }
        auto &face = faces[route.index];
        if (route.kind == RouteKind::candidate) {
          const auto child = std::static_pointer_cast<CandidatePointJob>(route.task);
          if (completion.state != StepState::done)
            internal("geometry candidate failed to finish");
          face.owner->finish_candidate(child->result());
          ready.push_back({face.owner, RouteKind::face, route.index});
        } else if (route.kind == RouteKind::preparation) {
          const auto &event = face.preparation->event();
          if (event.kind == FacePreparationEventKind::ready) {
            auto input = face.preparation->result();
            face.preparation.reset();
            face.owner = std::allocate_shared<FaceOwnerJob>(
                RunAllocator<FaceOwnerJob>(root.run->resource), root.run->resource,
                std::move(input), root.limits.face, root.scheduler.stop_token());
            ready.push_back({face.owner, RouteKind::face, route.index});
          } else if (event.kind == FacePreparationEventKind::curves && event.curves)
            pending.push_back({route.index, true, *event.curves});
          else if (event.kind == FacePreparationEventKind::boundary_horizon && event.boundary)
            face.request = event.boundary;
          else
            internal("unexpected face preparation event");
        } else {
          const auto &event = face.owner->event();
          if (event.kind == FaceOwnerEventKind::cells)
            pending.push_back({route.index, false, event.cells});
          else if (event.kind == FaceOwnerEventKind::candidate) {
            auto child = std::allocate_shared<CandidatePointJob>(
                RunAllocator<CandidatePointJob>(root.run->resource), event.candidate);
            ready.push_back({std::move(child), RouteKind::candidate, route.index});
          } else if (event.kind == FaceOwnerEventKind::boundary_horizon && event.boundary)
            face.request = event.boundary;
          else if (event.kind != FaceOwnerEventKind::settled)
            internal("unexpected face refinement event");
        }
      }
    }
  }
  void boundary_horizon(std::pmr::deque<ReadyTask> ready) {
    for (;;) {
      root.budget.charge(1);
      const auto epoch = forest.begin_epoch();
      drive(std::move(ready));
      bool requested = false;
      for (const auto &face : faces) {
        if (face.request) {
          if (face.request->kind != BoundaryOwnerKind::source_edge)
            throw Error(ErrorCode::unsupported, "face-cut generation rebuild is not connected");
          const std::array one{*face.request};
          forest.arrive(epoch, face.source, face.boundary_generation, one);
          requested = true;
        } else {
          if (!face.owner || face.owner->event().kind != FaceOwnerEventKind::settled)
            internal("definition lost a face before the complete boundary horizon");
          forest.arrive(epoch, face.source, face.boundary_generation, {});
        }
      }
      const auto batch = forest.prepare_splits(epoch);
      const auto update = evaluate_boundary(batch);
      ++root.stats.boundary_rounds;
      if (!requested) {
        if (!update.changed_faces.empty())
          internal("empty request horizon changed a shared boundary");
        return;
      }
      if (update.changed_faces.empty())
        internal("shared request horizon did not advance any incident face");
      ready = std::pmr::deque<ReadyTask>(&root.budget);
      for (const auto &change : update.changed_faces) {
        const auto at = face_index(change.face);
        auto &face = faces[at];
        if (face.boundary_generation != change.old_generation || face.generation == UINT64_MAX)
          internal("boundary update did not advance the current face generation");
        ++face.generation;
        prepare(at);
        if (face.boundary_generation != change.generation)
          internal("fresh source boundary lost its generation");
        ready.push_back({face.preparation, RouteKind::preparation, at});
      }
    }
  }
  NodeSlot node(const PhysicalNode &value, bool existing = false) {
    return node_registry.find_or_insert(value, existing);
  }
  FacetHandle allocate_facet() {
    root.budget.charge(1);
    if (!free_facets.empty()) {
      const auto slot = free_facets.back();
      free_facets.pop_back();
      auto &generation = facet_generations[slot];
      if (generation == UINT64_MAX)
        throw Error(ErrorCode::resource_exhausted, "definition facet generation exhausted");
      return {{slot}, ++generation};
    }
    if (facet_generations.size() >= UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted, "definition facet slots exhausted");
    const auto slot = static_cast<uint32_t>(facet_generations.size());
    facet_generations.push_back(0);
    return {{slot}, 0};
  }
  ContactProposedFacet geometry(const std::array<PhysicalNode, 3> &nodes,
                                 const std::array<DeclaredContactEdge, 3> &edges,
                                 bool existing = false) {
    return {{definition.value, {node(nodes[0], existing), node(nodes[1], existing), node(nodes[2], existing)},
              {nodes[0].xyz, nodes[1].xyz, nodes[2].xyz}}, edges,
            {nodes[0].key, nodes[1].key, nodes[2].key}};
  }
  std::pmr::vector<CellResultKey> contact_horizon() {
    std::pmr::vector<FaceFrontierIdentity> identities(&root.budget);
    for (const auto &face : faces)
      identities.push_back(identity(face));
    const auto epoch = contacts.begin(identities);
    std::pmr::vector<uint32_t> retired(&root.budget);
    for (auto &face : faces) {
      const auto horizon = face.owner->event().horizon;
      if (!horizon || horizon->face != identity(face))
        internal("settled face has no matching immutable contact horizon");
      auto delta = std::allocate_shared<ContactFaceDeltaData>(
          RunAllocator<ContactFaceDeltaData>(root.run->resource), root.run->resource, identity(face));
      auto erase = [&](const ContactFacetInput &input) {
        delta->removed.push_back(input.facet);
        retired.push_back(input.facet.slot.value);
        face.erase(input.cell.triangle);
      };
      if (face.processed != horizon) {
        if (face.processed && face.processed->face != horizon->face) {
          for (auto &row : face.facets)
            if (row)
              erase(*row);
          face.facets.clear();
        } else
          for (const auto triangle : horizon->removed) {
            const auto *found = face.find(triangle);
            if (found)
              erase(*found);
          }
        for (const auto &cell : horizon->inserted) {
          const auto proposed = geometry(cell.nodes, cell.edges);
          const auto *found = face.find(cell.cell.triangle);
          const auto handle = found ? found->facet : allocate_facet();
          const ContactFacetInput input{handle, cell.cell, proposed.geometry, proposed.edges,
                                         proposed.node_keys};
          face.insert(input);
          delta->inserted.push_back(input);
        }
        face.processed = horizon;
      }
      contacts.arrive(epoch, std::move(delta));
    }
    contacts.prepare(epoch);
    // Reuse only after every face's removals have been applied. Earlier reuse
    // could collide with a still-live facet in a later face delta.
    free_facets.insert(free_facets.end(), retired.begin(), retired.end());
    for (;;) {
      std::pmr::vector<std::shared_ptr<NativeTask>> work(&root.budget);
      for (uint32_t i = 0; i < root.scheduler.limits().threads; ++i) {
        auto batch = contacts.take_batch(epoch, root.limits.contact_batch);
        if (!batch)
          break;
        work.push_back(std::allocate_shared<ContactPairJob>(
            RunAllocator<ContactPairJob>(root.run->resource),
            NativeTaskKey{{OwnerKind::contact, definition.value, root.next_child()},
                          epoch.value, 0}, std::move(*batch)));
      }
      if (work.empty())
        break;
      for (const auto &completion : root.wave(work)) {
        const auto child = std::static_pointer_cast<ContactPairJob>(completion.task);
        if (completion.state != StepState::done || !contacts.complete(child->result()))
          internal("contact child returned a stale definition epoch");
      }
    }
    ++root.stats.contact_rounds;
    return contacts.finish(epoch);
  }
  void optimize() {
    using QueueKey = std::pair<FaceId, uint64_t>;
    struct QueuedEdge final { uint32_t face; EdgeHandle edge; };
    std::pmr::map<QueueKey, QueuedEdge> queue(&root.budget);
    auto enqueue = [&](uint32_t at) {
      const auto &reply = faces[at].owner->event().optimizer;
      if (!reply)
        internal("optimizer owner omitted its immutable edge queue");
      for (const auto &edge : reply->edges)
        queue.insert_or_assign({source().faces[faces[at].source.value].id, edge.key},
                               QueuedEdge{at, edge.edge});
    };
    for (uint32_t at = 0; at < faces.size(); ++at) {
      faces[at].owner->begin_optimizer();
      if (root.execute(faces[at].owner) != StepState::wait_barrier)
        internal("optimizer did not reach its initial queue horizon");
      enqueue(at);
    }
    while (!queue.empty()) {
      root.budget.charge(1);
      const auto next = queue.begin()->second;
      queue.erase(queue.begin());
      auto &face = faces[next.face];
      face.owner->prepare_flip(next.edge);
      const auto state = root.execute(face.owner);
      if (state == StepState::wait_barrier)
        continue;
      const auto proposal = face.owner->event().optimization;
      if (state != StepState::wait_child || !proposal)
        internal("optimizer did not publish its same-engine proposal");
      ++root.stats.optimizer_proposals;
      const auto numerical = root.evaluate(proposal->cells);
      const auto results = std::get<std::shared_ptr<const CellBatchResult>>(numerical);
      if (results->cells.size() != 2)
        internal("optimizer numerical service changed its two-cell packet");
      if (results->cells[0].decision != BoundDecision::accept ||
          results->cells[1].decision != BoundDecision::accept) {
        face.owner->reject_flip();
        static_cast<void>(root.execute(face.owner));
        continue;
      }
      std::array<FacetHandle, 2> old;
      std::array<ContactProposedFacet, 2> proposed;
      for (std::size_t i = 0; i < 2; ++i) {
        const auto *found = face.find(proposal->old_cells[i].triangle);
        if (!found || found->cell != proposal->old_cells[i])
          internal("optimizer proposal references a noncurrent physical facet");
        old[i] = found->facet;
        proposed[i] = geometry(proposal->nodes[i], proposal->edges[i], true);
      }
      const auto approval = contacts.approve_replacement(proposal->ordinal, old, proposed);
      if (!approval) {
        face.owner->reject_flip();
        static_cast<void>(root.execute(face.owner));
        continue;
      }
      if (!contacts.approval_current(*approval))
        internal("sole optimizer transaction lost its scene revision");
      const std::array bounds{results->cells[0], results->cells[1]};
      face.owner->finish_flip(bounds, *approval);
      if (root.execute(face.owner) != StepState::wait_barrier)
        internal("optimizer commit failed to return its immutable replacement");
      const auto &reply = face.owner->event().optimizer;
      if (!reply || !reply->committed)
        internal("optimizer commit did not publish current replacement keys");
      std::array<ContactFacetInput, 2> replacement;
      for (std::size_t i = 0; i < 2; ++i)
        replacement[i] = {allocate_facet(), (*reply->committed)[i], proposed[i].geometry,
                           proposed[i].edges, proposed[i].node_keys};
      contacts.commit_replacement(*approval, replacement);
      for (std::size_t i = 0; i < 2; ++i) {
        face.erase(proposal->old_cells[i].triangle);
        free_facets.push_back(old[i].slot.value);
      }
      for (const auto &facet : replacement)
        face.insert(facet);
      ++root.stats.optimizer_flips;
      enqueue(next.face);
    }
  }
  void finish(MeshedRunData &output) {
    if (const auto initial = forest.pending_evaluations())
      static_cast<void>(evaluate_boundary(*initial));
    std::pmr::deque<ReadyTask> ready(&root.budget);
    for (uint32_t at = 0; at < faces.size(); ++at) {
      prepare(at);
      ready.push_back({faces[at].preparation, RouteKind::preparation, at});
    }
    boundary_horizon(std::move(ready));
    for (;;) {
      auto marks = contact_horizon();
      if (marks.empty())
        break;
      ready = std::pmr::deque<ReadyTask>(&root.budget);
      for (uint32_t at = 0; at < faces.size(); ++at) {
        const auto first = std::lower_bound(marks.begin(), marks.end(), identity(faces[at]),
                                             [](const auto &a, const auto &b) { return a.face < b; });
        const auto last = std::upper_bound(first, marks.end(), identity(faces[at]),
                                            [](const auto &a, const auto &b) { return a < b.face; });
        if (first != last) {
          faces[at].owner->mark_contacts(std::span<const CellResultKey>(first, last));
          ready.push_back({faces[at].owner, RouteKind::face, at});
        }
      }
      boundary_horizon(std::move(ready));
    }
    optimize();
    for (auto &face : faces) {
      face.owner->finish_generation();
      if (root.execute(face.owner) != StepState::done)
        internal("complete face did not publish its final semantic result");
      auto result = face.owner->result();
      root.stats.final_sites += result->sites.size();
      root.stats.final_cells += result->triangles.size();
      output.faces.push_back(std::move(result));
      face.owner.reset();
      face.preparation.reset();
    }
    root.stats.contact_candidates += contacts.stats().candidate_pairs;
    output.boundaries.push_back(forest.freeze_definition_boundary());
  }
};
} // namespace
MeshedRun mesh_prepared_run(const PreparedRun &input, NativeScheduler &scheduler,
                             const MeshingRunLimits &limits) {
  if (!input)
    invalid("whole native mesher requires a prepared original source");
  const auto started = std::chrono::steady_clock::now();
  Root root(input, scheduler, limits);
  try {
    auto result = std::allocate_shared<MeshedRunData>(RunAllocator<MeshedRunData>(input->resource), input);
    for (uint32_t i = 0; i < input->geometry.view().source.view().definitions.size(); ++i) {
      DefinitionOwner definition(root, {i});
      definition.finish(*result);
    }
    root.close_services();
    root.stats.owner_work += root.budget.work();
    root.stats.owner_peak = std::max(root.stats.owner_peak, root.budget.peak());
    root.stats.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    result->stats = root.stats;
    return result;
  } catch (...) {
    const auto failure = std::current_exception();
    scheduler.stop();
    scheduler.drain();
    while (scheduler.take()) {}
    for (const auto &service : root.services)
      service->close();
    std::rethrow_exception(failure);
  }
}
} // namespace bgmesh
