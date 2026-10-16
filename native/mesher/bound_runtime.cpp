#include "bound_runtime.hpp"
#include "bound_filter.hpp"
#include "reservations.hpp"
#include <atomic>
#include <charconv>
#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <mutex>
#include <unistd.h>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void internal(const char *message) { throw Error(ErrorCode::internal_error, message); }
uint64_t sum(uint64_t a, uint64_t b) {
  if (a > std::numeric_limits<uint64_t>::max() - b)
    throw Error(ErrorCode::resource_exhausted, "bound runtime size/work overflow");
  return a + b;
}
void resource_required(const std::shared_ptr<RunResource> &resource) {
  if (!resource)
    internal("null bound runtime resource");
}
[[noreturn]] void translate(const bgfd::Failure &failure) {
  const auto code = failure.status == BGFD_RESOURCE_EXHAUSTED ? ErrorCode::resource_exhausted
                    : failure.status == BGFD_INVALID_INPUT    ? ErrorCode::invalid_source
                                                              : ErrorCode::internal_error;
  throw Error(code, std::string("native bound task: ") + failure.what());
}
void checked(int32_t status, const bgmb_error &error) {
  if (status == BGMB_OK)
    return;
  const auto code = status == BGMB_RESOURCE  ? ErrorCode::resource_exhausted
                    : status == BGMB_INVALID ? ErrorCode::invalid_source
                                             : ErrorCode::device_failure;
  const auto length = std::find(std::begin(error.message), std::end(error.message), '\0');
  throw Error(code, std::string("native bound device: ") +
                        std::string(std::begin(error.message), length));
}
const BoundTaskHeader &header(const BoundInput &input) {
  return std::visit([](const auto &value) -> const BoundTaskHeader & { return value.header; },
                    input);
}
BoundRootSlot root_slot(const BoundInput &input) {
  return std::visit([](const auto &value) { return value.root; }, input);
}
struct RootFrameRefs final {
  BoundRootSelection original;
  std::array<ExactRef, 3> anchor;
  std::array<ExactRef, 4> domain;
};
} // namespace

struct BoundBankData final {
  std::shared_ptr<RunResource> resource;
  GeometryStore geometry;
  ExactTable anchors;
  std::pmr::vector<RootFrameRefs> frames;
  std::pmr::vector<std::pair<BoundRootSelection, BoundRootSlot>> lookup;
  std::pmr::vector<bgmb_root> roots;
  std::pmr::vector<bgmb_interval> coefficients;
  BoundBankData(std::shared_ptr<RunResource> owner, GeometryStore source, ExactTable values)
      : resource(std::move(owner)), geometry(std::move(source)), anchors(std::move(values)),
        frames(resource.get()), lookup(resource.get()), roots(resource.get()),
        coefficients(resource.get()) {}
};
struct BoundBatchData final {
  std::shared_ptr<RunResource> resource;
  BoundRootBank bank;
  std::pmr::vector<ExactTable> tables;
  std::pmr::vector<BoundInput> inputs;
  BoundBatchData(std::shared_ptr<RunResource> owner, BoundRootBank source)
      : resource(std::move(owner)), bank(std::move(source)), tables(resource.get()),
        inputs(resource.get()) {}
  [[nodiscard]] ExactView exact(ExactRef ref) const {
    const auto found =
        std::lower_bound(tables.begin(), tables.end(), ref.table,
                         [](const auto &table, auto key) { return table.key() < key; });
    if (found == tables.end() || found->key() != ref.table)
      invalid("bound packet lacks exact table lease/generation");
    return found->view(ref);
  }
};
struct BoundFilteredData final {
  std::shared_ptr<RunResource> resource;
  BoundBatch batch;
  std::pmr::vector<BoundAnswer> answers;
  BoundFilteredData(std::shared_ptr<RunResource> owner, BoundBatch input)
      : resource(std::move(owner)), batch(std::move(input)), answers(resource.get()) {
    answers.resize(batch.inputs().size());
  }
};
struct BoundCompletedData final {
  std::shared_ptr<RunResource> resource;
  std::pmr::vector<BoundTaskHeader> headers;
  std::pmr::vector<BoundAnswer> answers;
  explicit BoundCompletedData(std::shared_ptr<RunResource> owner)
      : resource(std::move(owner)), headers(resource.get()), answers(resource.get()) {}
};
struct BoundModuleData final {
  void *handle = nullptr;
  const bgmb_api *api = nullptr;
  ~BoundModuleData() {
    if (handle)
      dlclose(handle);
  }
};

uint32_t BoundRootBank::size() const noexcept { return static_cast<uint32_t>(data_->roots.size()); }
const GeometryStore &BoundRootBank::geometry() const noexcept { return data_->geometry; }
BoundRootSlot BoundRootBank::find(BoundRootSelection selection) const {
  const auto found =
      std::lower_bound(data_->lookup.begin(), data_->lookup.end(), selection,
                       [](const auto &entry, const auto &key) { return entry.first < key; });
  if (found == data_->lookup.end() || found->first != selection)
    throw Error(ErrorCode::unsupported, "actual cell source root is not resident in bound bank");
  return found->second;
}
std::span<const BoundInput> BoundBatch::inputs() const noexcept { return data_->inputs; }
std::span<const BoundAnswer> BoundFiltered::answers() const noexcept { return data_->answers; }
std::span<const BoundTaskHeader> BoundCompleted::headers() const noexcept { return data_->headers; }
std::span<const BoundAnswer> BoundCompleted::answers() const noexcept { return data_->answers; }

BoundRootBank prepare_bound_roots(bgfd::Budget &budget,
                                  const std::shared_ptr<RunResource> &resource,
                                  GeometryStore geometry,
                                  std::span<const BoundRootSelection> selections,
                                  ExactTableKey key) {
  resource_required(resource);
  try {
    if (selections.empty() || selections.size() > std::numeric_limits<uint32_t>::max() ||
        key == geometry.view().table.key())
      invalid("invalid bound root bank selection/anchor identity");
    std::pmr::vector<BoundRootSelection> sorted(selections.begin(), selections.end(), &budget);
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
      invalid("duplicate bound root selection");
    ExactBuilder anchors(resource, key);
    std::pmr::vector<RootFrameRefs> frames(resource.get());
    std::pmr::vector<bgmb_root> roots(resource.get());
    std::pmr::vector<bgmb_interval> coefficients(resource.get());
    frames.reserve(selections.size());
    roots.reserve(selections.size());
    // Compute a single capacity reservation, not repeated exact-size growth.
    uint64_t count = 0;
    for (const auto &selection : selections)
      count = sum(count, std::visit(
                             [&](auto slot) -> uint64_t {
                               if constexpr (std::is_same_v<decltype(slot), SurfaceRootSlot>) {
                                 if (slot.value >= geometry.view().surface_roots.size())
                                   invalid("foreign surface root slot");
                                 return geometry.view().surface_roots[slot.value].controls.count;
                               } else {
                                 if (slot.value >= geometry.view().curve_roots.size())
                                   invalid("foreign curve root slot");
                                 return geometry.view().curve_roots[slot.value].controls.count;
                               }
                             },
                             selection));
    if (count > std::numeric_limits<std::size_t>::max())
      throw Error(ErrorCode::resource_exhausted, "bound bank coefficient count overflow");
    coefficients.resize(static_cast<std::size_t>(count));
    uint64_t offset = 0;
    for (const auto &selection : selections) {
      RootFrameRefs frame{selection, {}, {}};
      std::visit(
          [&](auto slot) {
            auto source = [&] {
              if constexpr (std::is_same_v<decltype(slot), SurfaceRootSlot>)
                return import_surface_root(budget, geometry, slot);
              else
                return import_curve_root(budget, geometry, slot);
            }();
            auto output = std::span(coefficients)
                              .subspan(static_cast<std::size_t>(offset), source.controls.size());
            roots.push_back(pack_bound_root(source, output, offset));
            for (std::size_t axis = 0; axis < 3; ++axis)
              frame.anchor[axis] = anchors.append(source.controls[axis].divide(source.controls[3]));
            const auto &bounds = [&]() -> const auto & {
              if constexpr (std::is_same_v<decltype(slot), SurfaceRootSlot>)
                return geometry.view().surface_roots[slot.value].bounds;
              else
                return geometry.view().curve_roots[slot.value].bounds;
            }();
            for (std::size_t axis = 0; axis < bounds.size(); ++axis)
              frame.domain[axis] = {geometry.view().table.key(), bounds[axis]};
            offset = sum(offset, source.controls.size());
          },
          selection);
      frames.push_back(frame);
    }
    auto data = std::allocate_shared<BoundBankData>(RunAllocator<BoundBankData>(resource), resource,
                                                    std::move(geometry), anchors.seal());
    data->frames = std::move(frames);
    data->lookup.reserve(selections.size());
    for (uint32_t i = 0; i < selections.size(); ++i)
      data->lookup.push_back({selections[i], {i}});
    std::sort(data->lookup.begin(), data->lookup.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    data->roots = std::move(roots);
    data->coefficients = std::move(coefficients);
    return BoundRootBank(std::move(data));
  } catch (const bgfd::Failure &failure) {
    translate(failure);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "bound root preparation cancelled");
  }
}

BoundBatch seal_bound_batch(const std::shared_ptr<RunResource> &resource, BoundRootBank bank,
                            std::span<const ExactTable> tables, std::span<const BoundInput> inputs,
                            BoundBatchLimits limits) {
  resource_required(resource);
  if (inputs.empty() || inputs.size() > limits.task_capacity ||
      tables.size() > limits.table_capacity)
    invalid("bound packet exceeds declared capacity");
  auto data = std::allocate_shared<BoundBatchData>(RunAllocator<BoundBatchData>(resource), resource,
                                                   std::move(bank));
  data->tables.assign(tables.begin(), tables.end());
  std::sort(data->tables.begin(), data->tables.end(),
            [](const auto &a, const auto &b) { return a.key() < b.key(); });
  for (std::size_t i = 1; i < data->tables.size(); ++i)
    if (data->tables[i - 1].key() == data->tables[i].key())
      invalid("duplicate packet exact table identity");
  data->inputs.assign(inputs.begin(), inputs.end());
  const auto &source = *data->bank.data_;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    const auto &h = header(inputs[i]);
    const auto slot = root_slot(inputs[i]).value;
    if (h.bound_revision != 1 ||
        h.source_generation != source.geometry.view().table.key().generation ||
        slot >= source.roots.size() || source.roots[slot].kind != inputs[i].index() ||
        (i != 0 && !(header(inputs[i - 1]).key < h.key)))
      invalid("invalid bound packet key/root/revision/generation");
    std::visit(
        [&](const auto &task) {
          const auto validate = [&](auto refs) {
            for (const auto ref : refs)
              static_cast<void>(data->exact(ref));
          };
          if constexpr (std::is_same_v<std::decay_t<decltype(task)>, SurfaceBoundInput>) {
            validate(task.original_uv);
            validate(task.actual_linear_xyz);
          } else {
            validate(task.original_parameter);
            validate(task.actual_endpoint_xyz);
          }
          static_cast<void>(data->exact(task.tolerance));
        },
        inputs[i]);
  }
  return BoundBatch(std::move(data));
}

BoundCudaModule load_bound_cuda(const std::shared_ptr<RunResource> &resource,
                                const std::filesystem::path &path) {
  resource_required(resource);
  auto data = std::allocate_shared<BoundModuleData>(RunAllocator<BoundModuleData>(resource));
  data->handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!data->handle)
    throw Error(ErrorCode::device_failure, std::string("load native bound module: ") + dlerror());
  void *symbol = dlsym(data->handle, "brepgpu_mesher_bound_cuda_v1");
  if (!symbol)
    throw Error(ErrorCode::device_failure, "native bound module lacks versioned entry");
  using Factory = const bgmb_api *(*)();
  static_assert(sizeof(Factory) == sizeof(symbol));
  Factory factory;
  std::memcpy(&factory, &symbol, sizeof(factory));
  data->api = factory();
  if (!data->api || data->api->abi != BGMESH_BOUND_ABI || data->api->size != sizeof(bgmb_api) ||
      !data->api->create || !data->api->submit || !data->api->poll || !data->api->destroy)
    throw Error(ErrorCode::device_failure, "native bound module ABI mismatch");
  return BoundCudaModule(std::move(data));
}

namespace {
template <std::size_t N, class Function, std::size_t... I>
std::array<Rational, N> import_array_impl(const std::array<ExactRef, N> &refs, Function function,
                                          std::index_sequence<I...>) {
  return {function(refs[I])...};
}
template <std::size_t N, class Function>
std::array<Rational, N> import_array(const std::array<ExactRef, N> &refs, Function function) {
  return import_array_impl(refs, function, std::make_index_sequence<N>{});
}
struct Child final {
  std::shared_ptr<ResourceReservation> reservation;
  bgfd::Budget budget;
  Child(const std::shared_ptr<RunResource> &resource, BoundRuntimeLimits limits,
        std::stop_token stop)
      : reservation(reserve_resource(resource, limits.child_bytes)),
        budget({limits.child_bytes, limits.child_work}, reservation.get(), std::move(stop)) {}
};
struct WorkCounter final {
  uint64_t limit, used = 0, reserved = 0;
  void acquire(uint64_t amount) {
    if (amount > limit - used - reserved)
      throw Error(ErrorCode::resource_exhausted, "bound runtime cumulative work exhausted");
    reserved += amount;
  }
  void settle(uint64_t amount, uint64_t actual) noexcept {
    if (actual > amount || amount > reserved)
      std::terminate();
    reserved -= amount;
    used += actual;
  }
};
struct WorkClaim final {
  WorkCounter &counter;
  uint64_t amount;
  bgfd::Budget &budget;
  WorkClaim(WorkCounter &owner, uint64_t maximum, bgfd::Budget &scratch)
      : counter(owner), amount(maximum), budget(scratch) {
    counter.acquire(amount);
  }
  ~WorkClaim() { counter.settle(amount, budget.work()); }
};
struct DeviceRecord final {
  std::shared_ptr<RunResource> resource;
  std::shared_ptr<ResourceReservation> reservation;
  std::shared_ptr<const BoundModuleData> module;
  BoundRootBank bank;
  bgmb_session *session = nullptr;
  DeviceRecord *next = nullptr, *previous = nullptr;
  uint64_t owner;
  bool poisoned = false;
  DeviceRecord(std::shared_ptr<RunResource> run, std::shared_ptr<ResourceReservation> cap,
               std::shared_ptr<const BoundModuleData> library, BoundRootBank roots, uint64_t id)
      : resource(std::move(run)), reservation(std::move(cap)), module(std::move(library)),
        bank(std::move(roots)), owner(id) {}
};
// Intrusive registration allocates nothing during failure teardown. Healthy
// close unlinks every record. Only failed final drains remain process-owned.
std::mutex device_registry_mutex;
DeviceRecord *device_registry = nullptr;
void retained_diagnostic(uint64_t owner) noexcept {
  char message[160] = "native bound runtime: retained poisoned device owner ";
  char *cursor = message + std::char_traits<char>::length(message);
  cursor = std::to_chars(cursor, std::end(message), owner).ptr;
  constexpr char suffix[] = " for process lifetime\n";
  std::copy(std::begin(suffix), std::end(suffix) - 1, cursor);
  cursor += sizeof(suffix) - 1;
  static_cast<void>(::write(STDERR_FILENO, message, static_cast<std::size_t>(cursor - message)));
}
void register_device(DeviceRecord *record) {
  const std::lock_guard lock(device_registry_mutex);
  record->next = device_registry;
  if (device_registry)
    device_registry->previous = record;
  device_registry = record;
}
void release_device(DeviceRecord *record) {
  {
    const std::lock_guard lock(device_registry_mutex);
    if (record->previous)
      record->previous->next = record->next;
    else
      device_registry = record->next;
    if (record->next)
      record->next->previous = record->previous;
  }
  auto allocator = RunAllocator<DeviceRecord>(record->resource);
  std::destroy_at(record);
  allocator.deallocate(record, 1);
}
std::atomic<uint64_t> runtime_ids{0};
uint64_t next_runtime_id() {
  auto previous = runtime_ids.load(std::memory_order_relaxed);
  do {
    if (previous == std::numeric_limits<uint64_t>::max())
      throw Error(ErrorCode::resource_exhausted, "bound runtime identity exhausted");
  } while (!runtime_ids.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed));
  return previous + 1;
}
} // namespace

struct BoundRuntime::Impl final {
  std::shared_ptr<RunResource> resource;
  BoundRootBank bank;
  BoundRuntimeLimits limits;
  std::stop_token stop;
  WorkCounter work;
  BoundRuntimeStats counters;
  uint64_t id, serial = 0, device_ticket = 0, active_work = 0;
  bool cancelled = false, closed = false;
  DeviceRecord *device = nullptr;
  std::shared_ptr<BoundFilteredData> active;
  std::pmr::vector<bgmb_task> packed;
  std::pmr::vector<bgmb_answer> raw_answers;
  Impl(std::shared_ptr<RunResource> run, BoundRootBank roots, BoundRuntimeLimits config,
       std::stop_token token)
      : resource(std::move(run)), bank(std::move(roots)), limits(config), stop(std::move(token)),
        work{config.total_work}, id(next_runtime_id()), packed(resource.get()),
        raw_answers(resource.get()) {}
  [[nodiscard]] const BoundBankData &source() const { return *bank.data_; }
  void ready() const {
    if (cancelled || stop.stop_requested())
      throw Error(ErrorCode::cancelled, "bound runtime cancelled");
    if (closed)
      internal("bound runtime is closed");
  }
  void validate(const BoundBatch &batch) const {
    ready();
    if (batch.data_->bank.data_ != bank.data_ ||
        batch.data_->inputs.size() > limits.task_capacity ||
        batch.data_->tables.size() > limits.table_capacity)
      invalid("foreign/oversized bound runtime packet");
  }
  uint64_t modeled(const BoundBatch &batch) const {
    uint64_t total = 0;
    for (const auto &input : batch.inputs())
      total = sum(total, bgmesh_filter::work(source().roots[root_slot(input).value]));
    return total;
  }
  bgmb_task pack(const BoundBatchData &batch, const BoundInput &input, bgfd::Budget &budget) const {
    const auto slot = root_slot(input).value;
    const auto &frame = source().frames[slot];
    auto anchor = import_array(
        frame.anchor, [&](auto ref) { return import_exact(budget, source().anchors.view(ref)); });
    const auto exact = [&](auto ref) { return import_exact(budget, batch.exact(ref)); };
    return std::visit(
        [&](const auto &task) {
          const auto tolerance = exact(task.tolerance);
          if constexpr (std::is_same_v<std::decay_t<decltype(task)>, SurfaceBoundInput>) {
            const auto domain = import_array(frame.domain, [&](auto ref) {
              return import_exact(budget, source().geometry.view().table.view(ref));
            });
            const auto uv = import_array(task.original_uv, exact);
            const auto xyz = import_array(task.actual_linear_xyz, exact);
            return pack_prepared_bound_task(task.header.key, slot, {0, domain, anchor}, uv, xyz,
                                            tolerance);
          } else {
            const std::array domain{
                import_exact(budget, source().geometry.view().table.view(frame.domain[0])),
                import_exact(budget, source().geometry.view().table.view(frame.domain[1]))};
            const auto parameter = import_array(task.original_parameter, exact);
            const auto xyz = import_array(task.actual_endpoint_xyz, exact);
            return pack_prepared_bound_task(task.header.key, slot, {1, domain, anchor}, parameter,
                                            xyz, tolerance);
          }
        },
        input);
  }
  BoundAnswer exact(const BoundBatchData &batch, const BoundInput &input,
                    bgfd::Budget &budget) const {
    const auto slot = root_slot(input).value;
    const auto values = [&](auto ref) { return import_exact(budget, batch.exact(ref)); };
    return std::visit(
        [&](const auto &task) {
          const auto tolerance = values(task.tolerance);
          if constexpr (std::is_same_v<std::decay_t<decltype(task)>, SurfaceBoundInput>) {
            const auto root =
                import_surface_root(budget, source().geometry,
                                    std::get<SurfaceRootSlot>(source().frames[slot].original));
            const auto uv = import_array(task.original_uv, values);
            const auto xyz = import_array(task.actual_linear_xyz, values);
            return surface_bound_exact(task.header.key, root, uv, xyz, tolerance);
          } else {
            const auto root = import_curve_root(
                budget, source().geometry, std::get<CurveRootSlot>(source().frames[slot].original));
            const auto parameter = import_array(task.original_parameter, values);
            const auto xyz = import_array(task.actual_endpoint_xyz, values);
            return curve_bound_exact(task.header.key, root, parameter[0], parameter[1], xyz,
                                     tolerance);
          }
        },
        input);
  }
};

BoundRuntime::BoundRuntime(const std::shared_ptr<RunResource> &resource, BoundRootBank bank,
                           BoundRuntimeLimits limits, std::stop_token stop)
    : resource_(resource), impl_(nullptr) {
  resource_required(resource);
  if (limits.task_capacity == 0 || limits.table_capacity == 0 || limits.child_bytes == 0)
    invalid("invalid bound runtime capacities");
  auto allocator = RunAllocator<Impl>(resource);
  impl_ = allocator.allocate(1);
  try {
    std::construct_at(impl_, resource, std::move(bank), limits, std::move(stop));
  } catch (...) {
    allocator.deallocate(impl_, 1);
    throw;
  }
}
BoundRuntime::~BoundRuntime() {
  try {
    close();
  } catch (...) {
    if (impl_->device) {
      impl_->device->poisoned = true;
      retained_diagnostic(impl_->device->owner);
      impl_->device = nullptr; // intrusive registry retains the complete owner
    }
  }
  std::destroy_at(impl_);
  RunAllocator<Impl>(resource_).deallocate(impl_, 1);
}
BoundFiltered BoundRuntime::filter_cpu(const BoundBatch &batch) {
  try {
    impl_->validate(batch);
    if (impl_->active)
      internal("bound runtime already has an active device batch");
    if (impl_->modeled(batch) > impl_->limits.child_work)
      throw Error(ErrorCode::resource_exhausted, "CPU bound batch exceeds child work cap");
    auto result = std::allocate_shared<BoundFilteredData>(
        RunAllocator<BoundFilteredData>(resource_), resource_, batch);
    Child child(resource_, impl_->limits, impl_->stop);
    WorkClaim claim(impl_->work, impl_->limits.child_work, child.budget);
    for (std::size_t i = 0; i < batch.inputs().size(); ++i) {
      const auto packed = impl_->pack(*batch.data_, batch.inputs()[i], child.budget);
      result->answers[i] = filter_bound_cpu(child.budget, impl_->source().roots[packed.root],
                                            impl_->source().coefficients, packed);
    }
    impl_->counters.cpu_tasks += batch.inputs().size();
    return BoundFiltered(std::move(result));
  } catch (const bgfd::Failure &failure) {
    translate(failure);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "CPU bound filter cancelled");
  }
}
void BoundRuntime::open_device(BoundCudaModule module, BoundDeviceLimits limits) {
  impl_->ready();
  if (impl_->device)
    internal("bound runtime device already open or poisoned");
  auto reservation = reserve_resource(resource_, sum(limits.host_bytes, limits.device_bytes));
  auto allocator = RunAllocator<DeviceRecord>(resource_);
  auto *record = allocator.allocate(1);
  try {
    std::construct_at(record, resource_, std::move(reservation), std::move(module.data_),
                      impl_->bank, impl_->id);
  } catch (...) {
    allocator.deallocate(record, 1);
    throw;
  }
  try {
    register_device(record);
  } catch (...) {
    std::destroy_at(record);
    allocator.deallocate(record, 1);
    throw;
  }
  impl_->device = record;
  // Allocate all adapter staging before a C create can acquire device resources.
  impl_->packed.resize(impl_->limits.task_capacity);
  impl_->raw_answers.resize(impl_->limits.task_capacity);
  const bgmb_config config{BGMESH_BOUND_ABI,
                           sizeof(bgmb_config),
                           limits.device,
                           impl_->limits.task_capacity,
                           limits.device_bytes,
                           limits.host_bytes,
                           impl_->limits.total_work,
                           0};
  bgmb_error error{};
  const auto &source = impl_->source();
  const auto status = record->module->api->create(
      &config, source.roots.data(), static_cast<uint32_t>(source.roots.size()),
      source.coefficients.data(), source.coefficients.size(), &record->session, &error);
  if (status != BGMB_OK && !record->session) {
    release_device(record);
    impl_->device = nullptr;
  } else if (status != BGMB_OK) {
    record->poisoned = true;
  }
  checked(status, error);
}
BoundTicket BoundRuntime::submit(const BoundBatch &batch) {
  try {
    impl_->validate(batch);
    if (!impl_->device || !impl_->device->session || impl_->device->poisoned || impl_->active)
      internal("bound runtime device absent or batch already active");
    if (impl_->serial == std::numeric_limits<uint64_t>::max())
      throw Error(ErrorCode::resource_exhausted, "bound ticket identity exhausted");
    auto result = std::allocate_shared<BoundFilteredData>(
        RunAllocator<BoundFilteredData>(resource_), resource_, batch);
    const auto modeled = impl_->modeled(batch);
    impl_->work.acquire(modeled);
    try {
      Child child(resource_, impl_->limits, impl_->stop);
      WorkClaim claim(impl_->work, impl_->limits.child_work, child.budget);
      for (std::size_t i = 0; i < batch.inputs().size(); ++i)
        impl_->packed[i] = impl_->pack(*batch.data_, batch.inputs()[i], child.budget);
    } catch (...) {
      impl_->work.settle(modeled, 0);
      throw;
    }
    // Retain inputs and the modeled-work claim even if submit poisons the C
    // session after starting transfers. Only drain/collect releases this state.
    impl_->active = std::move(result);
    impl_->active_work = modeled;
    ++impl_->serial;
    bgmb_error error{};
    const auto status = impl_->device->module->api->submit(
        impl_->device->session, impl_->packed.data(), static_cast<uint32_t>(batch.inputs().size()),
        &impl_->device_ticket, &error);
    if (status != BGMB_OK)
      impl_->device->poisoned = true;
    checked(status, error);
    return {impl_->id, impl_->serial};
  } catch (const bgfd::Failure &failure) {
    translate(failure);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "bound device packing cancelled");
  }
}
std::optional<BoundFiltered> BoundRuntime::poll(BoundTicket ticket) {
  if (!impl_->active || !impl_->device || ticket != BoundTicket{impl_->id, impl_->serial})
    invalid("stale/foreign bound runtime ticket");
  uint32_t ready = 0;
  bgmb_error error{};
  checked(impl_->device->module->api->poll(
              impl_->device->session, impl_->device_ticket, impl_->raw_answers.data(),
              static_cast<uint32_t>(impl_->active->answers.size()), &ready, &error),
          error);
  if (ready == 0)
    return std::nullopt;
  impl_->work.settle(impl_->active_work, impl_->active_work);
  impl_->active_work = 0;
  auto result = std::move(impl_->active);
  for (std::size_t i = 0; i < result->answers.size(); ++i) {
    const auto &answer = impl_->raw_answers[i];
    const BoundTaskKey key{answer.key.owner, answer.key.cell, answer.key.generation,
                           answer.key.ordinal};
    if (key != header(result->batch.inputs()[i]).key || answer.decision > 2 || answer.reserved != 0)
      internal("native bound device returned mismatched task identity/decision");
    result->answers[i] = {key, static_cast<BoundDecision>(answer.decision)};
  }
  impl_->counters.device_tasks += result->answers.size();
  if (impl_->cancelled || impl_->stop.stop_requested())
    throw Error(ErrorCode::cancelled, "bound runtime completion cancelled");
  return BoundFiltered(std::move(result));
}
BoundCompleted BoundRuntime::resolve(const BoundFiltered &filtered) {
  try {
    const auto &batch = filtered.data_->batch;
    impl_->validate(batch);
    auto result = std::allocate_shared<BoundCompletedData>(
        RunAllocator<BoundCompletedData>(resource_), resource_);
    result->answers.assign(filtered.answers().begin(), filtered.answers().end());
    result->headers.reserve(batch.inputs().size());
    for (const auto &input : batch.inputs())
      result->headers.push_back(header(input));
    for (std::size_t i = 0; i < result->answers.size(); ++i) {
      if (result->answers[i].decision != BoundDecision::unresolved)
        continue;
      Child child(resource_, impl_->limits, impl_->stop);
      WorkClaim claim(impl_->work, impl_->limits.child_work, child.budget);
      result->answers[i] = impl_->exact(*batch.data_, batch.inputs()[i], child.budget);
      ++impl_->counters.exact_tasks;
    }
    return BoundCompleted(std::move(result));
  } catch (const bgfd::Failure &failure) {
    translate(failure);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "bound exact fallback cancelled");
  }
}
void BoundRuntime::cancel() noexcept { impl_->cancelled = true; }
void BoundRuntime::close() {
  if (impl_->device) {
    auto *record = impl_->device;
    if (record->session) {
      bgmb_error error{};
      checked(record->module->api->destroy(record->session, &error), error);
      record->session = nullptr;
    }
    release_device(record);
    impl_->device = nullptr;
  }
  if (impl_->active) {
    impl_->work.settle(impl_->active_work, impl_->active_work);
    impl_->active_work = 0;
    impl_->active.reset();
  }
  std::pmr::vector<bgmb_task>(resource_.get()).swap(impl_->packed);
  std::pmr::vector<bgmb_answer>(resource_.get()).swap(impl_->raw_answers);
  impl_->closed = true;
}
BoundRuntimeStats BoundRuntime::stats() const noexcept {
  auto stats = impl_->counters;
  stats.charged_work = impl_->work.used;
  return stats;
}
} // namespace bgmesh
