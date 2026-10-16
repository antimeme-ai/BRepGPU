#include "../adaptive_numeric.hpp"
#include "adaptive_api.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <cuda_runtime.h>
#include <limits>
#include <new>

namespace bgmesh::adaptive {
namespace {
constexpr unsigned tile = 256, warps = 4, warp_threads = warps * 32;
struct Failure {
  int32_t status;
  const char *message;
};
void require(bool ok, const char *text) {
  if (!ok)
    throw Failure{1, text};
}
void checked(cudaError_t status) {
  if (status != cudaSuccess)
    throw Failure{3, cudaGetErrorString(status)};
}
uint64_t product(uint64_t a, uint64_t b) {
  if (b && a > UINT64_MAX / b)
    throw Failure{2, "device array size overflow"};
  return a * b;
}
struct Buffer {
  void *pointer = nullptr, *lease = nullptr;
  uint64_t bytes = 0;
  bool host = false;
};
enum Pool : unsigned {
  roots,
  originals,
  controls,
  points,
  nodes,
  input,
  result,
  frontier,
  next_frontier,
  marks,
  prefix,
  sums,
  sum_prefix,
  host_input,
  host_output,
  host_roots,
  host_originals,
  host_point_valid,
  host_control_valid,
  validation,
  pool_count
};
struct Span {
  uint32_t begin, end;
};
enum class Operation { none, samples, restrictions, nodes, facets };
template <class T> T *data(Buffer &buffer) { return static_cast<T *>(buffer.pointer); }
unsigned blocks(uint32_t count, unsigned width = tile) {
  return count / width + (count % width != 0);
}

__global__ void sample_kernel(const RootDescriptor *source, const Interval *net,
                              const DeviceSample *tasks, uint32_t count, SampleResult *pool,
                              SampleResult *output) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= count)
    return;
  const auto &task = tasks[i];
  const auto &root = source[task.input.original.value];
  const auto value =
      task.supplied
          ? task.supplied_result
          : numeric::evaluate_sample_rn64(
                root, root.controls.count ? net + root.controls.first : nullptr, task.input);
  pool[task.destination] = value;
  output[i] = value;
}
__global__ void restrict_kernel(const RootDescriptor *roots, Interval *pool,
                                const DeviceRestriction *tasks, uint32_t count) {
  __shared__ Interval shared[warps][64];
  const auto warp = threadIdx.x / 32, lane = threadIdx.x % 32;
  const auto i = blockIdx.x * warps + warp;
  if (i >= count)
    return;
  const auto &task = tasks[i];
  const auto &root = roots[task.root.value];
  for (uint32_t j = lane; j < task.parent.count; j += 32)
    shared[warp][j] = pool[task.parent.first + j];
  __syncwarp();
  for (uint32_t j = lane; j < task.parent.count; j += 32)
    pool[task.destination + j] =
        numeric::restricted_coefficient_rn64(root, shared[warp], task.axis, {task.child, j});
}
__global__ void install_nodes(const DeviceNode *input, uint32_t count, DeviceNode *pool,
                              uint32_t *frontier) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) {
    pool[input[i].slot] = input[i];
    frontier[i] = input[i].slot;
  }
}
__device__ DeviceNodeAnswer decide(const DeviceNode &node, const RootDescriptor &root,
                                   const NodeBounds &bounds) {
  DeviceNodeAnswer answer{};
  answer.bounds = bounds;
  if (bounds.status != NumericStatus::ready) {
    answer.action = 2;
    return answer;
  }
  if (bounds.regularity_proved && bounds.packet_upper <= node.tolerance_down)
    return answer;
  if (root.affine_separable_axes == 3) {
    answer.action = 2;
    return answer;
  }
  answer.action = 1;
  auto axis = node.fair_axis == 0                ? SplitAxis::u
              : node.fair_axis == 1              ? SplitAxis::v
              : bounds.score_u >= bounds.score_v ? SplitAxis::u
                                                 : SplitAxis::v;
  if (root.affine_separable_axes == 1)
    axis = SplitAxis::v;
  if (root.affine_separable_axes == 2)
    axis = SplitAxis::u;
  answer.axis = axis;
  return answer;
}
__global__ void analytic_nodes(const RootDescriptor *roots, const DeviceNode *pool,
                               const uint32_t *frontier, uint32_t count, const SampleResult *points,
                               DeviceNodeAnswer *out) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= count)
    return;
  const auto &node = pool[frontier[i]];
  const auto &root = roots[node.input.root.value];
  if (!node.cached && root.family == GeometryFamily::rational_tensor)
    return;
  auto predicate = node.input;
  bool ready = true;
  if (!node.cached)
    for (uint32_t c = 0; c < 4; ++c) {
      const auto &p = points[node.points[c]];
      ready = ready && p.status == NumericStatus::ready;
      predicate.corner_departures[c] = p.departure_upper;
      for (uint32_t a = 0; a < 3; ++a)
        predicate.actual_corners[c * 3 + a] = p.actual_xyz[a];
    }
  NodeBounds bound = node.cached ? node.cached_bounds
                     : ready     ? numeric::bound_node_rn64(root, nullptr, predicate)
                                 : NodeBounds{};
  out[i] = decide(node, root, bound);
}
__global__ void rational_nodes(const RootDescriptor *roots, const Interval *controls,
                               const DeviceNode *pool, const uint32_t *frontier, uint32_t count,
                               const SampleResult *points, DeviceNodeAnswer *out) {
  __shared__ Interval nets[warps][64];
  __shared__ NodePredicate predicates[warps];
  __shared__ double anchors[warps][3];
  __shared__ uint32_t ready[warps];
  const auto warp = threadIdx.x / 32, lane = threadIdx.x % 32;
  const auto i = blockIdx.x * warps + warp;
  if (i >= count)
    return;
  const auto &node = pool[frontier[i]];
  const auto &root = roots[node.input.root.value];
  if (node.cached || root.family != GeometryFamily::rational_tensor)
    return;
  for (uint32_t j = lane; j < node.input.controls.count; j += 32)
    nets[warp][j] = controls[node.input.controls.first + j];
  if (!lane) {
    predicates[warp] = node.input;
    ready[warp] = 1;
    for (uint32_t c = 0; c < 4; ++c) {
      const auto &p = points[node.points[c]];
      if (p.status != NumericStatus::ready)
        ready[warp] = 0;
      predicates[warp].corner_departures[c] = p.departure_upper;
      for (uint32_t a = 0; a < 3; ++a)
        predicates[warp].actual_corners[c * 3 + a] = p.actual_xyz[a];
    }
  }
  __syncwarp();
  if (!lane)
    for (uint32_t a = 0; a < 3; ++a) {
      const auto x = numeric::div(nets[warp][a], nets[warp][3]);
      anchors[warp][a] =
          numeric::rn_add(numeric::rn_mul(x.lower, .5), numeric::rn_mul(x.upper, .5));
    }
  __syncwarp();
  double maximum = 0.;
  const auto coefficients = (root.degree_u + 2) * (root.degree_v + 2);
  if (lane < coefficients && ready[warp]) {
    Interval xyz[3]{};
    const auto u = lane / (root.degree_v + 2), v = lane % (root.degree_v + 2);
    for (uint32_t a = 0; a < 3; ++a)
      xyz[a] = numeric::residual_coefficient_rn64(root, nets[warp], predicates[warp].actual_corners,
                                                  anchors[warp], {u, v, a});
    maximum = numeric::norm_upper(xyz);
  }
  for (unsigned offset = 16; offset; offset /= 2)
    maximum = numeric::maximum(maximum, __shfl_down_sync(0xffffffffU, maximum, offset));
  if (!lane) {
    const auto bound = ready[warp]
                           ? numeric::bound_node_rn64(root, nets[warp], predicates[warp], maximum)
                           : NodeBounds{};
    out[i] = decide(node, root, bound);
  }
}
__global__ void mark_nodes(const DeviceNodeAnswer *answers, uint32_t count, uint32_t *marks) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count)
    marks[i] = answers[i].action == 1;
}
__global__ void scan_tiles(const uint32_t *input, uint32_t *output, uint32_t count,
                           uint32_t *sums) {
  __shared__ uint32_t values[tile];
  const auto lane = threadIdx.x, i = blockIdx.x * tile + lane;
  values[lane] = i < count ? input[i] : 0;
  __syncthreads();
  for (unsigned stride = 1; stride < tile; stride *= 2) {
    const auto target = (lane + 1) * 2 * stride - 1;
    if (target < tile)
      values[target] += values[target - stride];
    __syncthreads();
  }
  if (!lane) {
    sums[blockIdx.x] = values[tile - 1];
    values[tile - 1] = 0;
  }
  __syncthreads();
  for (unsigned stride = tile / 2; stride; stride /= 2) {
    const auto target = (lane + 1) * 2 * stride - 1;
    if (target < tile) {
      const auto left = values[target - stride];
      values[target - stride] = values[target];
      values[target] += left;
    }
    __syncthreads();
  }
  if (i < count)
    output[i] = values[lane];
}
__global__ void add_prefix(uint32_t *output, uint32_t count, const uint32_t *prefix) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count)
    output[i] += prefix[i / tile];
}
__global__ void compact_nodes(DeviceNodeAnswer *answers, const uint32_t *marks,
                              const uint32_t *prefix, uint32_t count, uint32_t *next) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= count)
    return;
  answers[i].split_prefix = prefix[i];
  if (marks[i]) {
    next[2 * prefix[i]] = i;
    next[2 * prefix[i] + 1] = i;
  }
}
__global__ void publish_compact(DeviceNodeAnswer *answers, const uint32_t *next,
                                const uint32_t *marks, const uint32_t *prefix, uint32_t count) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  const auto total = 2 * (prefix[count - 1] + marks[count - 1]);
  if (i < total && !(i & 1))
    answers[next[i]].split_prefix = i / 2;
}
__global__ void facet_kernel(const ActualFacetInput *input, uint32_t count,
                             ActualFacetBounds *output) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count)
    output[i] = numeric::bound_actual_facet_rn64(input[i]);
}
} // namespace

struct DeviceSession {
  DeviceConfig config{};
  DeviceStats counters{};
  std::array<Buffer, pool_count> buffers{};
  Buffer pending{};
  std::array<Buffer, 512> retired{};
  uint32_t retired_count = 0, root_count = 0, original_count = 0;
  cudaStream_t stream = nullptr;
  cudaEvent_t event = nullptr;
  void *owner_lease = nullptr;
  uint64_t ticket = 0, work = 0;
  uint32_t count = 0, result_width = 0;
  Operation operation = Operation::none;
  bool broken = false;
};
namespace {
void charge(DeviceSession &s, uint64_t amount) {
  if (amount > s.config.work - s.work)
    throw Failure{2, "adaptive device work exhausted"};
  s.work += amount;
}
void release_buffer(DeviceSession &s, Buffer &b) {
  if (b.pointer) {
    checked(b.host ? cudaFreeHost(b.pointer) : cudaFree(b.pointer));
    b.pointer = nullptr;
  }
  if (b.lease) {
    s.config.release(s.config.reservation_context, b.lease);
    auto &live = b.host ? s.counters.host_reserved_bytes : s.counters.device_reserved_bytes;
    live -= b.bytes;
    b = {};
  }
}
void grow(DeviceSession &s, Pool index, uint64_t need, bool host, bool preserve = false) {
  auto &old = s.buffers[index];
  if (old.bytes >= need)
    return;
  require(!s.pending.lease && s.retired_count < s.retired.size(), "device growth owner capacity");
  auto &live = host ? s.counters.host_reserved_bytes : s.counters.device_reserved_bytes;
  auto &peak = host ? s.counters.host_peak_bytes : s.counters.device_peak_bytes;
  const auto cap = host ? s.config.host_bytes : s.config.device_bytes;
  if (live > cap)
    throw Failure{2, "adaptive live allocation exceeds capacity"};
  auto capacity = old.bytes && old.bytes <= UINT64_MAX / 2 ? std::max(need, old.bytes * 2) : need;
  if (capacity > cap - live)
    capacity = need;
  if (capacity > cap - live)
    throw Failure{2, "adaptive allocation capacity exhausted"};
  auto *lease = s.config.reserve(s.config.reservation_context, capacity);
  if (!lease)
    throw Failure{2, "native aggregate allocation reservation refused"};
  s.pending = {nullptr, lease, capacity, host};
  live += capacity;
  peak = std::max(peak, live);
  checked(
      host ? cudaHostAlloc(&s.pending.pointer, static_cast<size_t>(capacity), cudaHostAllocDefault)
           : cudaMalloc(&s.pending.pointer, static_cast<size_t>(capacity)));
  if (host)
    std::memset(s.pending.pointer, 0, static_cast<size_t>(capacity));
  if (preserve && old.pointer) {
    if (host)
      std::memcpy(s.pending.pointer, old.pointer, static_cast<size_t>(old.bytes));
    else
      checked(cudaMemcpy(s.pending.pointer, old.pointer, static_cast<size_t>(old.bytes),
                         cudaMemcpyDeviceToDevice));
  }
  // Register both allocations before free; a failed free leaves both owned.
  s.retired[s.retired_count++] = old;
  old = s.pending;
  s.pending = {};
  release_buffer(s, s.retired[s.retired_count - 1]);
  --s.retired_count;
}
void idle(DeviceSession &s) {
  require(!s.broken && s.operation == Operation::none,
          "device session unavailable or operation outstanding");
  checked(cudaSetDevice(s.config.device));
}
void control_range(DeviceSession &s, ScalarRange range) {
  require(range.first <= s.config.controls && range.count <= s.config.controls - range.first,
          "device control range exceeds capacity");
  const auto &valid = s.buffers[host_control_valid];
  require(uint64_t{range.first} + range.count <= valid.bytes,
          "device parent controls not resident");
  const auto *flags = static_cast<const uint8_t *>(valid.pointer);
  for (uint32_t i = 0; i < range.count; ++i)
    require(flags[range.first + i], "device control hole");
}
void unique_slots(DeviceSession &s, uint32_t count) {
  auto *spans = data<Span>(s.buffers[validation]);
  std::sort(spans, spans + count, [](const Span &a, const Span &b) { return a.begin < b.begin; });
  for (uint32_t i = 1; i < count; ++i)
    require(spans[i - 1].end <= spans[i].begin, "duplicate/overlapping device destinations");
}
void upload(DeviceSession &s, const void *source, uint64_t bytes) {
  grow(s, host_input, bytes, true);
  grow(s, input, bytes, false);
  std::memcpy(s.buffers[host_input].pointer, source, static_cast<size_t>(bytes));
  checked(cudaMemcpyAsync(s.buffers[input].pointer, s.buffers[host_input].pointer,
                          static_cast<size_t>(bytes), cudaMemcpyHostToDevice, s.stream));
  s.counters.uploaded_bytes += bytes;
}
void output_capacity(DeviceSession &s, uint32_t count, uint32_t width) {
  const auto bytes = product(count, width);
  grow(s, result, bytes, false);
  grow(s, host_output, bytes, true);
}
void finish(DeviceSession &s, Operation op, uint32_t count, uint32_t width, uint64_t *ticket) {
  checked(cudaGetLastError());
  if (width) {
    const auto bytes = product(count, width);
    checked(cudaMemcpyAsync(s.buffers[host_output].pointer, s.buffers[result].pointer,
                            static_cast<size_t>(bytes), cudaMemcpyDeviceToHost, s.stream));
    s.counters.downloaded_bytes += bytes;
  }
  checked(cudaEventRecord(s.event, s.stream));
  s.operation = op;
  s.count = count;
  s.result_width = width;
  if (++s.ticket == 0)
    throw Failure{2, "device operation ticket exhausted"};
  *ticket = s.ticket;
}
template <class F> int32_t call(DeviceSession *s, DeviceError *error, F &&function) noexcept {
  try {
    require(error != nullptr, "null device error record");
    *error = {};
    function();
    return 0;
  } catch (const Failure &failure) {
    if (s && failure.status == 3)
      s->broken = true;
    if (error) {
      error->status = failure.status;
      std::snprintf(error->message, sizeof(error->message), "%s", failure.message);
    }
    return failure.status;
  } catch (...) {
    if (s)
      s->broken = true;
    if (error) {
      error->status = 2;
      std::snprintf(error->message, sizeof(error->message), "%s",
                    "adaptive device allocation failure");
    }
    return 2;
  }
}
int32_t create(const DeviceConfig *config, const RootDescriptor *prepared, uint32_t nr,
               const RootDescriptor *original, uint32_t no, const Interval *coefficients,
               uint32_t nc, DeviceSession **output, DeviceError *error) {
  if (output)
    *output = nullptr;
  return call(nullptr, error, [&] {
    require(config && output && prepared && original && nr && no, "invalid device create inputs");
    require(config->abi == adaptive_cuda_abi && config->size >= sizeof(DeviceConfig) &&
                config->device >= 0 && config->points && config->controls && config->nodes &&
                config->frontier && config->frontier <= UINT32_MAX / 2 && config->reserve &&
                config->release,
            "unsupported adaptive device configuration");
    require(nc <= config->controls && (!nc || coefficients),
            "source controls exceed device capacity");
    if (config->host_bytes < sizeof(DeviceSession))
      throw Failure{2, "adaptive owner exceeds host capacity"};
    int device_count = 0;
    checked(cudaGetDeviceCount(&device_count));
    require(config->device < device_count, "adaptive device index outside installed devices");
    auto *lease = config->reserve(config->reservation_context, sizeof(DeviceSession));
    if (!lease)
      throw Failure{2, "device owner reservation refused"};
    DeviceSession *owner = nullptr;
    try {
      owner = new DeviceSession;
    } catch (...) {
      config->release(config->reservation_context, lease);
      throw;
    }
    *output = owner;
    auto &s = *owner;
    s.config = *config;
    s.owner_lease = lease;
    s.broken = true;
    s.counters.host_reserved_bytes = s.counters.host_peak_bytes = sizeof(DeviceSession);
    s.root_count = nr;
    s.original_count = no;
    checked(cudaSetDevice(config->device));
    cudaDeviceProp properties{};
    checked(cudaGetDeviceProperties(&properties, config->device));
    require(properties.major == 6 && properties.minor == 0, "adaptive module requires sm60 P100");
    checked(cudaStreamCreateWithFlags(&s.stream, cudaStreamNonBlocking));
    checked(cudaEventCreateWithFlags(&s.event, cudaEventDisableTiming));
    grow(s, host_roots, product(nr, sizeof(RootDescriptor)), true);
    grow(s, host_originals, product(no, sizeof(RootDescriptor)), true);
    std::memcpy(s.buffers[host_roots].pointer, prepared,
                static_cast<size_t>(product(nr, sizeof(RootDescriptor))));
    std::memcpy(s.buffers[host_originals].pointer, original,
                static_cast<size_t>(product(no, sizeof(RootDescriptor))));
    for (uint32_t set = 0; set < 2; ++set) {
      const auto *source = set ? original : prepared;
      const auto count = set ? no : nr;
      for (uint32_t i = 0; i < count; ++i) {
        require(static_cast<uint32_t>(source[i].family) <=
                    static_cast<uint32_t>(GeometryFamily::rational_tensor),
                "unknown adaptive geometry family");
        require(source[i].degree_u <= 3 && source[i].degree_v <= 3,
                "unsupported device rational degree");
        require(source[i].family != GeometryFamily::rational_tensor ||
                    source[i].controls.count ==
                        (source[i].degree_u + 1) * (source[i].degree_v + 1) * 4,
                "invalid original/prepared rational control shape");
        require(source[i].family == GeometryFamily::rational_tensor ||
                    source[i].controls.count == 0,
                "analytic source unexpectedly owns a rational net");
        require(source[i].controls.first <= nc &&
                    source[i].controls.count <= nc - source[i].controls.first,
                "invalid original/prepared source control range");
      }
    }
    grow(s, roots, product(nr, sizeof(RootDescriptor)), false);
    grow(s, originals, product(no, sizeof(RootDescriptor)), false);
    checked(cudaMemcpy(s.buffers[roots].pointer, prepared,
                       static_cast<size_t>(product(nr, sizeof(RootDescriptor))),
                       cudaMemcpyHostToDevice));
    checked(cudaMemcpy(s.buffers[originals].pointer, original,
                       static_cast<size_t>(product(no, sizeof(RootDescriptor))),
                       cudaMemcpyHostToDevice));
    s.counters.uploaded_bytes += product(uint64_t{nr} + no, sizeof(RootDescriptor));
    if (nc) {
      grow(s, controls, product(nc, sizeof(Interval)), false);
      grow(s, host_control_valid, nc, true);
      std::memset(s.buffers[host_control_valid].pointer, 1, nc);
      checked(cudaMemcpy(s.buffers[controls].pointer, coefficients,
                         static_cast<size_t>(product(nc, sizeof(Interval))),
                         cudaMemcpyHostToDevice));
      s.counters.uploaded_bytes += product(nc, sizeof(Interval));
    }
    s.broken = false;
  });
}
int32_t samples(DeviceSession *s, const DeviceSample *tasks, uint32_t count, uint64_t *ticket,
                DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier,
            "invalid sample operation");
    idle(*s);
    charge(*s, product(count, 128));
    grow(*s, validation, product(count, sizeof(Span)), true);
    uint32_t end = 0;
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.destination < s->config.points &&
                  task.input.original.value < s->original_count && task.supplied <= 1,
              "sample destination/source outside capacity");
      const auto &root =
          data<RootDescriptor>(s->buffers[host_originals])[task.input.original.value];
      for (uint32_t a = 0; a < 2; ++a)
        require(numeric::valid(task.input.uv[a]) &&
                    task.input.uv[a].lower >= root.original_box[2 * a].lower &&
                    task.input.uv[a].upper <= root.original_box[2 * a + 1].upper,
                "sample outside original root");
      if (task.supplied) {
        require(task.supplied_result.status == NumericStatus::ready &&
                    numeric::finite(task.supplied_result.departure_upper) &&
                    task.supplied_result.departure_upper >= 0,
                "invalid supplied canonical sample");
        for (uint32_t a = 0; a < 3; ++a)
          require(numeric::valid(task.supplied_result.original_xyz[a]) &&
                      numeric::finite(task.supplied_result.actual_xyz[a]),
                  "invalid supplied point coordinate");
      }
      data<Span>(s->buffers[validation])[i] = {task.destination, task.destination + 1};
      end = std::max(end, task.destination + 1);
    }
    unique_slots(*s, count);
    grow(*s, points, product(end, sizeof(SampleResult)), false, true);
    grow(*s, host_point_valid, end, true, true);
    output_capacity(*s, count, sizeof(SampleResult));
    upload(*s, tasks, product(count, sizeof(DeviceSample)));
    sample_kernel<<<blocks(count), tile, 0, s->stream>>>(
        data<RootDescriptor>(s->buffers[originals]), data<Interval>(s->buffers[controls]),
        data<DeviceSample>(s->buffers[input]), count, data<SampleResult>(s->buffers[points]),
        data<SampleResult>(s->buffers[result]));
    for (uint32_t i = 0; i < count; ++i)
      data<uint8_t>(s->buffers[host_point_valid])[tasks[i].destination] = 1;
    finish(*s, Operation::samples, count, sizeof(SampleResult), ticket);
  });
}
int32_t restrictions(DeviceSession *s, const DeviceRestriction *tasks, uint32_t count,
                     uint64_t *ticket, DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier,
            "invalid restriction operation");
    idle(*s);
    charge(*s, product(count, 512));
    grow(*s, validation, product(count, sizeof(Span)), true);
    uint32_t end = 0;
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.root.value < s->root_count && task.child <= 1 &&
                  (task.axis == SplitAxis::u || task.axis == SplitAxis::v),
              "invalid restriction root/child/axis");
      const auto &root = data<RootDescriptor>(s->buffers[host_roots])[task.root.value];
      require(root.family == GeometryFamily::rational_tensor &&
                  task.parent.count == (root.degree_u + 1) * (root.degree_v + 1) * 4,
              "restriction net shape mismatch");
      control_range(*s, task.parent);
      require(task.destination <= s->config.controls &&
                  task.parent.count <= s->config.controls - task.destination,
              "restriction destination capacity");
      const auto limit = task.destination + task.parent.count;
      data<Span>(s->buffers[validation])[i] = {task.destination, limit};
      end = std::max(end, limit);
    }
    unique_slots(*s, count);
    const auto *destinations = data<Span>(s->buffers[validation]);
    for (uint32_t i = 0; i < count; ++i) {
      const auto parent = tasks[i].parent;
      const auto *next =
          std::lower_bound(destinations, destinations + count, parent.first,
                           [](const Span &range, uint32_t at) { return range.end <= at; });
      require(next == destinations + count || next->begin >= parent.first + parent.count,
              "restriction destination overlaps batch parent read");
    }
    grow(*s, controls, product(end, sizeof(Interval)), false, true);
    grow(*s, host_control_valid, end, true, true);
    upload(*s, tasks, product(count, sizeof(DeviceRestriction)));
    restrict_kernel<<<blocks(count, warps), warp_threads, 0, s->stream>>>(
        data<RootDescriptor>(s->buffers[roots]), data<Interval>(s->buffers[controls]),
        data<DeviceRestriction>(s->buffers[input]), count);
    for (uint32_t i = 0; i < count; ++i)
      std::memset(data<uint8_t>(s->buffers[host_control_valid]) + tasks[i].destination, 1,
                  tasks[i].parent.count);
    finish(*s, Operation::restrictions, count, 0, ticket);
  });
}
void scan(DeviceSession &s, const uint32_t *input, uint32_t *output, uint32_t count,
          uint64_t offset = 0) {
  const auto n = blocks(count);
  auto *sum = data<uint32_t>(s.buffers[sums]) + offset;
  auto *pre = data<uint32_t>(s.buffers[sum_prefix]) + offset;
  scan_tiles<<<n, tile, 0, s.stream>>>(input, output, count, sum);
  if (n > 1) {
    scan(s, sum, pre, n, offset + n);
    add_prefix<<<blocks(count), tile, 0, s.stream>>>(output, count, pre);
  }
}
int32_t node_operation(DeviceSession *s, const DeviceNode *tasks, uint32_t count, uint64_t *ticket,
                       DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier, "invalid node operation");
    idle(*s);
    charge(*s, product(count, 8192));
    grow(*s, validation, product(count, sizeof(Span)), true);
    uint32_t end = 0;
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.slot < s->config.nodes && task.input.root.value < s->root_count &&
                  task.fair_axis <= 2 && task.cached <= 1 && numeric::finite(task.tolerance_down) &&
                  task.tolerance_down >= 0,
              "invalid node slot/root/tolerance");
      const auto &root = data<RootDescriptor>(s->buffers[host_roots])[task.input.root.value];
      for (uint32_t a = 0; a < 4; ++a) {
        require(numeric::valid(task.input.box[a]), "invalid node box");
        if (!task.cached)
          require(task.points[a] < s->buffers[host_point_valid].bytes &&
                      data<uint8_t>(s->buffers[host_point_valid])[task.points[a]],
                  "node point not resident");
      }
      for (uint32_t a = 0; a < 2; ++a)
        require(task.input.box[2 * a].lower >= root.prepared_box[2 * a].lower &&
                    task.input.box[2 * a + 1].upper <= root.prepared_box[2 * a + 1].upper,
                "node box outside prepared source rectangle");
      if (!task.cached && root.family == GeometryFamily::rational_tensor) {
        require(task.input.controls.count == (root.degree_u + 1) * (root.degree_v + 1) * 4,
                "node control shape");
        control_range(*s, task.input.controls);
      }
      if (task.cached)
        require(task.cached_bounds.status == NumericStatus::ready &&
                    numeric::finite(task.cached_bounds.packet_upper) &&
                    task.cached_bounds.packet_upper >= 0,
                "invalid cached node bounds");
      data<Span>(s->buffers[validation])[i] = {task.slot, task.slot + 1};
      end = std::max(end, task.slot + 1);
    }
    unique_slots(*s, count);
    grow(*s, nodes, product(end, sizeof(DeviceNode)), false, true);
    grow(*s, frontier, product(count, sizeof(uint32_t)), false);
    grow(*s, next_frontier, product(uint64_t{count} * 2, sizeof(uint32_t)), false);
    grow(*s, marks, product(count, sizeof(uint32_t)), false);
    grow(*s, prefix, product(count, sizeof(uint32_t)), false);
    uint64_t hierarchy = 0;
    for (auto n = count;;) {
      n = blocks(n);
      hierarchy += n;
      if (n == 1)
        break;
    }
    grow(*s, sums, product(hierarchy, sizeof(uint32_t)), false);
    grow(*s, sum_prefix, product(hierarchy, sizeof(uint32_t)), false);
    output_capacity(*s, count, sizeof(DeviceNodeAnswer));
    upload(*s, tasks, product(count, sizeof(DeviceNode)));
    install_nodes<<<blocks(count), tile, 0, s->stream>>>(data<DeviceNode>(s->buffers[input]), count,
                                                         data<DeviceNode>(s->buffers[nodes]),
                                                         data<uint32_t>(s->buffers[frontier]));
    analytic_nodes<<<blocks(count), tile, 0, s->stream>>>(
        data<RootDescriptor>(s->buffers[roots]), data<DeviceNode>(s->buffers[nodes]),
        data<uint32_t>(s->buffers[frontier]), count, data<SampleResult>(s->buffers[points]),
        data<DeviceNodeAnswer>(s->buffers[result]));
    rational_nodes<<<blocks(count, warps), warp_threads, 0, s->stream>>>(
        data<RootDescriptor>(s->buffers[roots]), data<Interval>(s->buffers[controls]),
        data<DeviceNode>(s->buffers[nodes]), data<uint32_t>(s->buffers[frontier]), count,
        data<SampleResult>(s->buffers[points]), data<DeviceNodeAnswer>(s->buffers[result]));
    mark_nodes<<<blocks(count), tile, 0, s->stream>>>(data<DeviceNodeAnswer>(s->buffers[result]),
                                                      count, data<uint32_t>(s->buffers[marks]));
    scan(*s, data<uint32_t>(s->buffers[marks]), data<uint32_t>(s->buffers[prefix]), count);
    compact_nodes<<<blocks(count), tile, 0, s->stream>>>(
        data<DeviceNodeAnswer>(s->buffers[result]), data<uint32_t>(s->buffers[marks]),
        data<uint32_t>(s->buffers[prefix]), count, data<uint32_t>(s->buffers[next_frontier]));
    publish_compact<<<blocks(count * 2), tile, 0, s->stream>>>(
        data<DeviceNodeAnswer>(s->buffers[result]), data<uint32_t>(s->buffers[next_frontier]),
        data<uint32_t>(s->buffers[marks]), data<uint32_t>(s->buffers[prefix]), count);
    finish(*s, Operation::nodes, count, sizeof(DeviceNodeAnswer), ticket);
  });
}
int32_t facets(DeviceSession *s, const ActualFacetInput *tasks, uint32_t count, uint64_t *ticket,
               DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier,
            "invalid facet operation");
    idle(*s);
    charge(*s, product(count, 256));
    for (uint32_t i = 0; i < count; ++i) {
      require(numeric::finite(tasks[i].source_to_bilinear_upper) &&
                  tasks[i].source_to_bilinear_upper >= 0,
              "invalid facet source bound");
      for (auto x : tasks[i].actual_corners)
        require(numeric::finite(x), "invalid actual B corner");
      for (auto x : tasks[i].uv)
        require(numeric::valid(x) && x.lower >= 0 && x.upper <= 1,
                "invalid virtual facet parameter");
      for (auto x : tasks[i].affine_xyz)
        require(numeric::valid(x), "invalid actual L coordinate");
    }
    output_capacity(*s, count, sizeof(ActualFacetBounds));
    upload(*s, tasks, product(count, sizeof(ActualFacetInput)));
    facet_kernel<<<blocks(count), tile, 0, s->stream>>>(
        data<ActualFacetInput>(s->buffers[input]), count,
        data<ActualFacetBounds>(s->buffers[result]));
    finish(*s, Operation::facets, count, sizeof(ActualFacetBounds), ticket);
  });
}
int32_t poll(DeviceSession *s, uint64_t ticket, uint32_t *ready, void *output, uint32_t capacity,
             DeviceError *error) {
  return call(s, error, [&] {
    require(s && ready && !s->broken && s->operation != Operation::none && ticket == s->ticket,
            "stale/foreign device ticket");
    require(!s->result_width || (output && capacity >= s->count),
            "device poll output record capacity");
    *ready = 0;
    checked(cudaSetDevice(s->config.device));
    const auto status = cudaEventQuery(s->event);
    if (status == cudaErrorNotReady)
      return;
    checked(status);
    if (s->result_width)
      std::memcpy(output, s->buffers[host_output].pointer,
                  static_cast<size_t>(product(s->count, s->result_width)));
    *ready = 1;
    s->operation = Operation::none;
  });
}
int32_t stats(DeviceSession *s, DeviceStats *output, DeviceError *error) {
  return call(s, error, [&] {
    require(s && output, "invalid device stats output");
    *output = s->counters;
  });
}
int32_t destroy(DeviceSession *s, DeviceError *error) {
  return call(s, error, [&] {
    require(s, "null device owner");
    if (s->stream || s->event)
      checked(cudaSetDevice(s->config.device));
    if (s->stream)
      checked(cudaStreamSynchronize(s->stream));
    for (auto &buffer : s->buffers)
      release_buffer(*s, buffer);
    release_buffer(*s, s->pending);
    for (auto &buffer : s->retired)
      release_buffer(*s, buffer);
    if (s->event) {
      checked(cudaEventDestroy(s->event));
      s->event = nullptr;
    }
    if (s->stream) {
      checked(cudaStreamDestroy(s->stream));
      s->stream = nullptr;
    }
    const auto config = s->config;
    auto *lease = s->owner_lease;
    delete s;
    config.release(config.reservation_context, lease);
  });
}
const AdaptiveCudaAPI api{adaptive_cuda_abi,
                          sizeof(AdaptiveCudaAPI),
                          create,
                          samples,
                          restrictions,
                          node_operation,
                          facets,
                          poll,
                          stats,
                          destroy};
} // namespace
} // namespace bgmesh::adaptive
extern "C" const bgmesh::adaptive::AdaptiveCudaAPI *brepgpu_adaptive_cuda_v1() {
  return &bgmesh::adaptive::api;
}
