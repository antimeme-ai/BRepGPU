#include "adaptive_component.hpp"
#include <bit>
#include <chrono>
#include <cstdio>
#include <thread>

namespace bgmesh::adaptive {
namespace {
using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}
void require(bool okay, ErrorCode code, const char *message) {
  if (!okay)
    throw Error(code, message);
}
uint32_t count32(std::size_t value) {
  require(value < UINT32_MAX, ErrorCode::resource_exhausted, "component index exhausted");
  return static_cast<uint32_t>(value);
}
bool same_xyz(const Vec3 &a, const Vec3 &b) {
  for (uint32_t axis = 0; axis < 3; ++axis)
    if (std::bit_cast<uint64_t>(a[axis]) != std::bit_cast<uint64_t>(b[axis]))
      return false;
  return true;
}
struct AssignmentLocation final {
  uint32_t owner, face;
};
class FaceStep final : public NativeTask {
public:
  std::shared_ptr<FaceTrimSession> owner;
  uint32_t slot, face;
  TrimFaceProgress progress = TrimFaceProgress::progressed;
  FaceStep(NativeTaskKey identity, std::shared_ptr<FaceTrimSession> worker,
           AssignmentLocation location)
      : NativeTask(identity), owner(std::move(worker)), slot(location.owner), face(location.face) {}
  StepState step(JobContext &context) override {
    progress = owner->step(context);
    return progress == TrimFaceProgress::complete || progress == TrimFaceProgress::needs_boundary
               ? StepState::done
               : StepState::ready;
  }
};
class ContactStep final : public NativeTask {
  ContactPairBatch input_;
  std::optional<ContactPairCompleted> output_;

public:
  ContactStep(NativeTaskKey identity, ContactPairBatch input)
      : NativeTask(identity), input_(std::move(input)) {}
  StepState step(JobContext &context) override {
    require(!output_, ErrorCode::internal_error, "component contact child repeated");
    output_ = classify_contact_batch(context.budget, input_);
    return StepState::done;
  }
  const ContactPairCompleted &result() const {
    if (!output_)
      throw Error(ErrorCode::internal_error, "component contact child incomplete");
    return *output_;
  }
};
// All submitted work is drained and records are taken BEFORE any fold throws.
// This also keeps every task/owner alive through a partial submit failure.
std::pmr::vector<StepCompletion> wave(std::span<const std::shared_ptr<NativeTask>> tasks,
                                      NativeScheduler &scheduler, JobContext &context) {
  std::exception_ptr failure;
  uint32_t submitted = 0;
  try {
    for (const auto &task : tasks) {
      scheduler.submit(task);
      ++submitted;
    }
  } catch (...) {
    failure = std::current_exception();
  }
  scheduler.drain();
  std::pmr::vector<StepCompletion> result(&context.budget);
  while (auto completion = scheduler.take())
    result.push_back(std::move(*completion));
  if (failure)
    std::rethrow_exception(failure);
  require(result.size() == submitted, ErrorCode::internal_error, "component wave lost completion");
  std::sort(result.begin(), result.end(), [&](const auto &a, const auto &b) {
    context.budget.charge(1);
    return a.task->key < b.task->key;
  });
  for (const auto &completion : result) {
    context.budget.charge(completion.exact_work);
    completion.rethrow();
    require(completion.state != StepState::failed, ErrorCode::internal_error,
            "component task failed without exception");
  }
  return result;
}
std::pmr::vector<uint64_t> face_generations(const SharedCurveSnapshot &snapshot,
                                            bgfd::Budget &budget) {
  std::pmr::vector<uint64_t> generations(snapshot->source.view().faces.size(), UINT64_MAX, &budget);
  for (const auto &use : snapshot->uses) {
    budget.charge(1);
    require(use.face.value < generations.size() && use.boundary_generation != UINT64_MAX,
            ErrorCode::invalid_source, "component trace face/generation outside profile");
    auto &generation = generations[use.face.value];
    require(generation == UINT64_MAX || generation == use.boundary_generation,
            ErrorCode::invalid_source, "component face has mixed boundary generations");
    generation = use.boundary_generation;
  }
  return generations;
}
FaceFrontierIdentity identity(const FaceConstructionResult &face) {
  return {{OwnerKind::face, face->key.definition.value,
           face->source.view().faces[face->key.face.value].id.value},
          face->key.construction_revision,
          face->key.boundary_generation};
}
struct SiteRow final {
  PhysicalNodeKey key;
  Vec3 xyz;
  uint64_t address;
};
struct ProtectedPair final {
  std::array<uint32_t, 2> sites;
  BoundaryKey key;
};
std::array<uint32_t, 2> pair(uint32_t a, uint32_t b) { return {std::min(a, b), std::max(a, b)}; }

ContactEpochStats check_contacts(ComponentResultData &output, ContactEpochLimits limits,
                                 uint32_t batch_size, NativeScheduler &scheduler,
                                 JobContext &context) {
  auto &budget = context.budget;
  std::pmr::vector<uint64_t> offsets(&budget);
  std::pmr::vector<SiteRow> sites(&budget);
  offsets.push_back(0);
  for (uint32_t at = 0; at < output.faces.size(); ++at) {
    const auto &result = output.faces[at];
    const auto &face = *result->construction;
    count32(face.sites.size());
    for (uint32_t i = 0; i < face.sites.size(); ++i) {
      budget.charge(1);
      const auto &site = face.sites[i];
      const auto key = component_site_key(output, {at, i});
      if (const auto *boundary = std::get_if<BoundarySiteSource>(&site.source)) {
        require(boundary->boundary_node < result->boundary->boundary->nodes.size(),
                ErrorCode::invalid_source, "component physical boundary row missing");
        const auto &node = result->boundary->boundary->nodes[boundary->boundary_node];
        require(same_xyz(node.xyz, site.actual_xyz), ErrorCode::invalid_source,
                "component physical boundary XYZ differs from captured node");
      }
      sites.push_back({key, site.actual_xyz, sites.size()});
    }
    offsets.push_back(sites.size());
  }
  std::sort(sites.begin(), sites.end(), [&](const auto &a, const auto &b) {
    budget.charge(1);
    return a.key < b.key;
  });
  std::pmr::vector<NodeSlot> node_ids(sites.size(), NodeSlot{}, &budget);
  uint32_t node = 0;
  for (std::size_t i = 0; i < sites.size(); ++i) {
    budget.charge(1);
    if (i && sites[i - 1].key == sites[i].key) {
      require(same_xyz(sites[i - 1].xyz, sites[i].xyz), ErrorCode::invalid_source,
              "component physical key has inconsistent XYZ words");
    } else if (i) {
      require(node < UINT32_MAX - 1, ErrorCode::resource_exhausted,
              "component physical node index exhausted");
      ++node;
    }
    node_ids[sites[i].address] = {node};
  }
  ContactEpochOwner contacts(budget, output.resource, output.source.source(),
                             output.query.definition, limits);
  std::pmr::vector<FaceFrontierIdentity> identities(&budget);
  for (const auto &face : output.faces)
    identities.push_back(identity(face->construction));
  const auto epoch = contacts.begin(identities);
  uint32_t facet_slot = 0;
  for (uint32_t at = 0; at < output.faces.size(); ++at) {
    const auto &mesh = *output.faces[at]->construction;
    std::pmr::vector<ProtectedPair> source_edges(&budget);
    for (const auto &edge : mesh.edges) {
      budget.charge(1);
      if (const auto *key = std::get_if<BoundaryKey>(&edge.provenance))
        source_edges.push_back({pair(edge.sites[0], edge.sites[1]), *key});
    }
    std::sort(source_edges.begin(), source_edges.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a.sites < b.sites;
    });
    for (std::size_t i = 1; i < source_edges.size(); ++i)
      require(source_edges[i - 1].sites != source_edges[i].sites ||
                  source_edges[i - 1].key == source_edges[i].key,
              ErrorCode::invalid_source, "component source edge has mixed provenance");
    const auto face = identities[at];
    auto delta = std::allocate_shared<ContactFaceDeltaData>(
        RunAllocator<ContactFaceDeltaData>(output.resource), output.resource, face);
    count32(mesh.facets.size());
    for (uint32_t f = 0; f < mesh.facets.size(); ++f) {
      budget.charge(1);
      require(facet_slot < UINT32_MAX, ErrorCode::resource_exhausted,
              "component physical facet index exhausted");
      ContactFacetInput input{};
      input.facet = {{facet_slot++}, 1};
      input.cell = {face, {f, mesh.key.construction_revision}, 0};
      input.geometry.definition = output.query.definition.value;
      const auto &facet = mesh.facets[f];
      for (uint32_t i = 0; i < 3; ++i) {
        const auto a = facet.sites[i], b = facet.sites[(i + 1) % 3];
        require(a < mesh.sites.size() && b < mesh.sites.size(), ErrorCode::invalid_source,
                "component facet references missing site");
        input.geometry.nodes[i] = node_ids[offsets[at] + a];
        input.geometry.xyz[i] = mesh.sites[a].actual_xyz;
        const auto endpoint = pair(a, b);
        const auto found = std::lower_bound(source_edges.begin(), source_edges.end(), endpoint,
                                            [&](const auto &edge, const auto &key) {
                                              budget.charge(1);
                                              return edge.sites < key;
                                            });
        const auto physical_pair =
            pair(node_ids[offsets[at] + a].value, node_ids[offsets[at] + b].value);
        input.edges[i] = {ContactEdgeKind::internal_face, face.owner.id, face.face_generation,
                          (uint64_t{physical_pair[0]} << 32) | physical_pair[1]};
        if (found != source_edges.end() && found->sites == endpoint) {
          require(found->key.kind == BoundaryOwnerKind::source_edge, ErrorCode::unsupported,
                  "component source face carries a generated face-cut edge");
          input.edges[i] = {ContactEdgeKind::source_edge, found->key.owner, 0, found->key.interval};
        }
        input.node_keys[i] = component_site_key(output, {at, a});
      }
      delta->inserted.push_back(input);
    }
    contacts.arrive(epoch, delta);
  }
  contacts.prepare(epoch);
  uint64_t serial = 0;
  for (;;) {
    std::pmr::vector<std::shared_ptr<NativeTask>> tasks(&budget);
    for (uint32_t i = 0; i < scheduler.limits().task_capacity; ++i) {
      auto batch = contacts.take_batch(epoch, batch_size);
      if (!batch)
        break;
      require(serial != UINT64_MAX, ErrorCode::resource_exhausted,
              "component contact task ordinal exhausted");
      tasks.push_back(std::allocate_shared<ContactStep>(
          RunAllocator<ContactStep>(output.resource),
          NativeTaskKey{{OwnerKind::contact, output.query.definition.value, ++serial},
                        output.query.generation,
                        0},
          std::move(*batch)));
    }
    if (tasks.empty())
      break;
    for (const auto &completion : wave(tasks, scheduler, context)) {
      const auto child = std::static_pointer_cast<ContactStep>(completion.task);
      require(completion.state == StepState::done && contacts.complete(child->result()),
              ErrorCode::internal_error, "component contact completion stale");
    }
  }
  const auto forbidden = contacts.finish(epoch);
  if (!forbidden.empty())
    throw Error(ErrorCode::unresolved, "component forbidden contact at original face " +
                                           std::to_string(forbidden.front().face.owner.id) +
                                           " facet " +
                                           std::to_string(forbidden.front().triangle.slot));
  return contacts.stats();
}
} // namespace

ComponentResultData::ComponentResultData(std::shared_ptr<RunResource> owner, SourceCache bank,
                                         const ComponentQuery &request)
    : resource(std::move(owner)), source(std::move(bank)), query(request), faces(resource.get()),
      owners(resource.get()) {}
PhysicalNodeKey component_site_key(const ComponentResultData &component, ComponentSiteSlot slot) {
  const auto face = slot.face, site = slot.site;
  require(face < component.faces.size() && component.faces[face] &&
              component.faces[face]->construction && component.faces[face]->boundary,
          ErrorCode::invalid_source, "component site key requires a complete face");
  const auto &result = *component.faces[face];
  const auto &mesh = *result.construction;
  require(site < mesh.sites.size() && mesh.key.definition == component.query.definition &&
              mesh.key.query == component.query.generation,
          ErrorCode::invalid_source, "component site key has foreign face/site/query");
  if (const auto *boundary = std::get_if<BoundarySiteSource>(&mesh.sites[site].source)) {
    require(result.boundary->boundary &&
                boundary->boundary_node < result.boundary->boundary->nodes.size(),
            ErrorCode::invalid_source, "component site key has missing captured physical node");
    return result.boundary->boundary->nodes[boundary->boundary_node].key;
  }
  return {component.query.definition.value, NodeGenerator::face_site,
          mesh.source.view().faces.at(mesh.key.face.value).id.value, component.query.generation,
          site};
}
struct ComponentMesher::Impl final {
  struct StopForward final {
    std::stop_source *source;
    void operator()() const noexcept { source->request_stop(); }
  };
  std::shared_ptr<RunResource> resource;
  SourceCache source;
  ComponentLimits limits;
  std::optional<CacheDevice> device;
  std::stop_source cancellation;
  std::optional<std::stop_callback<StopForward>> external_stop;
  std::pmr::vector<std::shared_ptr<FaceTrimSession>> pool;
  uint64_t last_query = 0, assignment = 0;
  enum class Phase { idle, active, failed, closed } phase = Phase::idle;
  Impl(std::shared_ptr<RunResource> owner, SourceCache bank, ComponentLimits caps,
       std::optional<CacheDevice> placement, const std::stop_token &token)
      : resource(std::move(owner)), source(std::move(bank)), limits(caps),
        device(std::move(placement)), pool(resource.get()) {
    require(resource && source.source().view().resource == resource &&
                source.purpose() == SourceCachePurpose::regular_patch && limits.face_owners &&
                limits.maximum_boundary_rounds && limits.contacts.pair_capacity &&
                limits.contact_batch,
            ErrorCode::invalid_source, "component source owner/profile/limits invalid");
    limits.face.surface.cpu_threads = 1;
    external_stop.emplace(token, StopForward{&cancellation});
  }
  void stopped() const {
    if (cancellation.stop_requested())
      throw Error(ErrorCode::cancelled, "component mesher cancelled");
    require(phase == Phase::idle, ErrorCode::invalid_source, "component mesher is not idle");
  }
};
ComponentMesher::ComponentMesher(std::shared_ptr<RunResource> owner, SourceCache source,
                                 ComponentLimits limits, std::optional<CacheDevice> device,
                                 const std::stop_token &stop)
    : resource_(std::move(owner)), impl_(nullptr) {
  require(resource_ != nullptr, ErrorCode::invalid_source, "component resource required");
  RunAllocator<Impl> allocator(resource_);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, resource_, std::move(source), limits, std::move(device), stop);
  } catch (...) {
    allocator.deallocate(impl_, 1);
    impl_ = nullptr;
    throw;
  }
}
ComponentMesher::~ComponentMesher() {
  if (!impl_)
    return;
  try {
    close();
  } catch (...) {
    std::fputs("component close failed; face services retain their drain protocol\n", stderr);
  }
  std::destroy_at(impl_);
  RunAllocator<Impl>(resource_).deallocate(impl_, 1);
}
ComponentResult ComponentMesher::mesh(const ComponentQuery &query, NativeScheduler &scheduler,
                                      JobContext &context) {
  auto &p = *impl_;
  p.stopped();
  auto &budget = context.budget;
  budget.charge(0);
  const auto &original = p.source.source().view();
  require(query.generation && query.generation >= p.last_query &&
              query.definition.value < original.definitions.size() &&
              &query.curves.source().view() == &original && query.values.resource() == resource_ &&
              query.curves.profile().revision == p.source.profile().revision && scheduler.idle(),
          ErrorCode::invalid_source, "component source/quality/scheduler preflight invalid");
  require(import_exact(budget, query.values.view(query.positional)).sign() > 0 &&
              import_exact(budget, query.values.view(query.node_source)).sign() > 0,
          ErrorCode::invalid_source, "component allowances must be positive");
  const auto inventory = original.definition_faces.at(query.definition.value);
  require(!inventory.empty(), ErrorCode::unsupported, "component definition has no faces");
  count32(inventory.size());
  std::pmr::vector<uint8_t> carriers(original.carriers.size(), uint8_t{0}, &budget);
  for (const auto &root : p.source.roots()) {
    budget.charge(1);
    require(root.source_slot < carriers.size(), ErrorCode::invalid_source,
            "component source root carrier missing");
    carriers[root.source_slot] = 1;
  }
  for (const auto face : inventory) {
    budget.charge(1);
    const auto carrier = original.carrier_index.find(original.faces.at(face).carrier_id);
    require(carriers[carrier.value], ErrorCode::invalid_source,
            "component fixed source bank omits an original face carrier");
  }
  const auto capacity =
      std::min({p.limits.face_owners, count32(inventory.size()), scheduler.limits().task_capacity});
  require(capacity != 0, ErrorCode::invalid_source, "component scheduler has no task capacity");
  auto output = std::allocate_shared<ComponentResultData>(
      RunAllocator<ComponentResultData>(resource_), resource_, p.source, query);
  output->faces.resize(inventory.size());
  std::stop_callback caller_stop(context.stop, Impl::StopForward{&p.cancellation});
  p.phase = Impl::Phase::active;
  p.last_query = query.generation;
  try {
    auto tick = Clock::now();
    while (p.pool.size() < capacity) {
      auto owner = std::allocate_shared<FaceTrimSession>(RunAllocator<FaceTrimSession>(resource_),
                                                         resource_, p.source, p.limits.face,
                                                         p.cancellation.get_token());
      if (p.device)
        owner->open_device(*p.device);
      p.pool.push_back(std::move(owner));
    }
    output->stats.owner_setup_seconds = elapsed(tick);
    tick = Clock::now();
    AdaptiveBoundarySession boundary(resource_, p.source.source(), query.definition, query.curves,
                                     p.source, p.limits.curves, p.cancellation.get_token());
    if (p.device)
      boundary.open_device(*p.device);
    output->stats.boundary_setup_seconds = elapsed(tick);
    tick = Clock::now();
    boundary.begin({query.generation, query.values, query.positional, query.node_source, {}});
    std::pmr::vector<uint64_t> generations(&budget);
    const auto settle = [&] {
      while (boundary.step(context) != StepState::done) {
        budget.charge(1);
        std::this_thread::yield();
      }
      output->boundary = boundary.result();
      require(output->boundary->carrier_source &&
                  &output->boundary->carrier_source->view() == &p.source.view(),
              ErrorCode::internal_error, "component boundary changed retained carrier source bank");
      generations = face_generations(output->boundary, budget);
      for (const auto face : inventory) {
        budget.charge(1);
        require(generations[face] != UINT64_MAX, ErrorCode::unsupported,
                "component face has no boundary uses");
      }
    };
    settle();
    output->stats.boundary_seconds += elapsed(tick);
    for (uint64_t round = 0;; ++round) {
      budget.charge(1);
      require(round < p.limits.maximum_boundary_rounds, ErrorCode::unresolved,
              "component boundary round limit reached");
      std::pmr::vector<uint32_t> pending(&budget);
      for (uint32_t i = 0; i < output->faces.size(); ++i)
        if (!output->faces[i])
          pending.push_back(i);
      std::pmr::vector<BoundaryKey> requests(&budget);
      std::pmr::vector<std::shared_ptr<NativeTask>> tasks(&budget);
      uint32_t next_face = 0;
      const auto assign = [&](uint32_t slot) -> std::shared_ptr<NativeTask> {
        const auto index = pending[next_face++];
        require(p.assignment < UINT64_MAX, ErrorCode::resource_exhausted,
                "component face assignment epoch exhausted");
        const auto epoch = ++p.assignment;
        p.pool[slot]->begin({query.generation,
                             query.definition,
                             {inventory[index]},
                             output->boundary,
                             query.values,
                             query.positional,
                             query.node_source,
                             epoch});
        ++output->stats.assignments;
        return std::allocate_shared<FaceStep>(
            RunAllocator<FaceStep>(resource_),
            NativeTaskKey{{OwnerKind::face, query.definition.value,
                           original.faces[inventory[index]].id.value},
                          query.generation,
                          epoch},
            p.pool[slot], AssignmentLocation{slot, index});
      };
      for (uint32_t i = 0; i < capacity && next_face < pending.size(); ++i)
        tasks.push_back(assign(i));
      tick = Clock::now();
      while (!tasks.empty()) {
        auto completions = wave(tasks, scheduler, context);
        tasks.clear();
        for (const auto &completion : completions) {
          budget.charge(1);
          ++output->stats.face_steps;
          const auto task = std::static_pointer_cast<FaceStep>(completion.task);
          if (completion.state == StepState::ready) {
            tasks.push_back(task);
            continue;
          }
          require(completion.state == StepState::done, ErrorCode::internal_error,
                  "component face returned unexpected wait state");
          if (task->progress == TrimFaceProgress::complete)
            output->faces[task->face] = task->owner->result();
          else {
            require(task->progress == TrimFaceProgress::needs_boundary, ErrorCode::internal_error,
                    "component face completion has no complete/request state");
            const auto wanted = task->owner->boundary_requests();
            require(!wanted.empty(), ErrorCode::unresolved,
                    "component face requested an empty physical refinement horizon");
            requests.insert(requests.end(), wanted.begin(), wanted.end());
          }
          task->owner->retire_assignment();
          if (next_face < pending.size())
            tasks.push_back(assign(task->slot));
        }
        if (!tasks.empty())
          std::this_thread::yield();
      }
      output->stats.face_seconds += elapsed(tick);
      if (requests.empty())
        break;
      std::sort(requests.begin(), requests.end(), [&](const auto &a, const auto &b) {
        budget.charge(1);
        return a < b;
      });
      requests.erase(std::unique(requests.begin(), requests.end()), requests.end());
      output->stats.requested_intervals += requests.size();
      tick = Clock::now();
      boundary.refine(output->boundary, requests);
      settle();
      ++output->stats.boundary_rounds;
      for (uint32_t i = 0; i < output->faces.size(); ++i) {
        auto &face = output->faces[i];
        if (!face)
          continue;
        if (face->construction->key.boundary_generation != generations[inventory[i]])
          face.reset();
        else
          ++output->stats.retained_faces;
      }
      output->stats.boundary_seconds += elapsed(tick);
    }
    for (uint32_t i = 0; i < output->faces.size(); ++i) {
      const auto &face = output->faces[i];
      require(face && face->construction->key.face.value == inventory[i] &&
                  face->construction->key.boundary_generation == generations[inventory[i]],
              ErrorCode::internal_error, "component final face horizon is missing/stale");
      output->stats.facets += face->construction->facets.size();
    }
    output->stats.curves = boundary.stats();
    tick = Clock::now();
    output->stats.contacts =
        check_contacts(*output, p.limits.contacts, p.limits.contact_batch, scheduler, context);
    output->stats.contact_seconds = elapsed(tick);
    for (const auto &owner : p.pool)
      output->owners.push_back(owner->stats());
    tick = Clock::now();
    boundary.close();
    output->stats.boundary_close_seconds = elapsed(tick);
    p.phase = Impl::Phase::idle;
    return output;
  } catch (...) {
    p.phase = Impl::Phase::failed;
    cancel();
    scheduler.stop();
    scheduler.drain();
    while (scheduler.take()) {
    }
    throw;
  }
}
void ComponentMesher::cancel() noexcept { impl_->cancellation.request_stop(); }
void ComponentMesher::close() {
  auto &p = *impl_;
  if (p.phase == Impl::Phase::closed)
    return;
  require(p.phase != Impl::Phase::active, ErrorCode::invalid_source,
          "component close requires a drained coordinator");
  std::exception_ptr failure;
  for (auto &owner : p.pool) {
    try {
      owner->close();
    } catch (...) {
      if (!failure)
        failure = std::current_exception();
    }
  }
  if (failure)
    std::rethrow_exception(failure);
  p.phase = Impl::Phase::closed;
}
} // namespace bgmesh::adaptive
