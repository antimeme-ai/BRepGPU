#include "../bound_filter.hpp"
#include "bound_api.h"
#include <cstdio>
#include <cstring>
#include <cuda_runtime.h>
#include <limits>
#include <new>

namespace {
struct Failure {
  int32_t status;
  const char *message;
  cudaError_t cuda = cudaSuccess;
};
void require(bool condition, const char *message) {
  if (!condition)
    throw Failure{1, message};
}
void check(cudaError_t status, const char *message) {
  if (status != cudaSuccess)
    throw Failure{3, message, status};
}
void error(bgmb_error *out, Failure value) noexcept {
  if (out) {
    out->status = value.status;
    std::snprintf(out->message, sizeof(out->message), "%s%s%s", value.message,
                  value.cuda == cudaSuccess ? "" : ": ",
                  value.cuda == cudaSuccess ? "" : cudaGetErrorString(value.cuda));
  }
}
struct DeviceGuard {
  int previous = -1;
  bool selected = false;
  explicit DeviceGuard(int device) {
    check(cudaGetDevice(&previous), "read caller CUDA device");
    const auto status = cudaSetDevice(device);
    if (status != cudaSuccess) {
      const auto restored = cudaSetDevice(previous);
      if (restored != cudaSuccess)
        throw Failure{3, "bound selection and caller restoration failed", restored};
      throw Failure{3, "select bound CUDA device", status};
    }
    selected = true;
  }
  void finish() {
    if (selected) {
      check(cudaSetDevice(previous), "restore caller CUDA device");
      selected = false;
    }
  }
  ~DeviceGuard() {
    if (selected)
      static_cast<void>(cudaSetDevice(previous));
  }
};
uint64_t bytes(uint64_t n, uint64_t size) {
  if (n > std::numeric_limits<uint64_t>::max() / size)
    throw Failure{2, "bound allocation size overflow"};
  return n * size;
}
void add_bytes(uint64_t &total, uint64_t amount) {
  if (amount > std::numeric_limits<uint64_t>::max() - total)
    throw Failure{2, "bound allocation sum overflow"};
  total += amount;
}
struct DeviceTask {
  bgmb_task task;
  uint32_t output, reserved;
};
using namespace bgmesh_filter;
__device__ Interval shuffle(Interval x, int source) {
  return {__shfl_sync(0xffffffffu, x.lower, source), __shfl_sync(0xffffffffu, x.upper, source)};
}
__device__ void publish(uint32_t local, const DeviceTask &task, bgmb_answer *answers) {
  const auto failed = __ballot_sync(0xffffffffu, local == 1);
  const auto unresolved = __ballot_sync(0xffffffffu, local == 2);
  if ((threadIdx.x & 31u) == 0)
    answers[task.output] = {task.task.key, failed ? 1u : unresolved ? 2u : 0u, 0};
}
template <unsigned P, unsigned Q>
__global__ void surface_kernel(const bgmb_root *roots, const Interval *coefficients,
                               const DeviceTask *tasks, uint32_t begin, uint32_t count,
                               bgmb_answer *answers) {
  const uint32_t warp = threadIdx.x / 32, lane = threadIdx.x & 31u, slot = blockIdx.x * 4 + warp;
  if (slot >= count)
    return;
  const auto &task = tasks[begin + slot];
  const auto root = roots[task.task.root];
  Interval uv[6];
  if (!coordinates(root, task.task, uv)) {
    if (lane == 0)
      answers[task.output] = {task.task.key, 2, 0};
    return;
  }
  constexpr uint32_t degree = P + Q, net_count = (degree + 1) * (degree + 2) / 2;
  __shared__ Interval banks[4][36 * 4];
  auto *net = banks[warp];
  if (lane < net_count) {
    uint32_t i = 0, j = lane;
    while (j > degree - i) {
      j -= degree - i + 1;
      ++i;
    }
    for (uint32_t axis = 0; axis < 4; ++axis)
      net[lane * 4 + axis] =
          surface_coefficient<P, Q>(coefficients + root.offset, {i, j, axis}, uv);
  }
  __syncwarp();
  const auto threshold = limit(net, net_count, task.task.tolerance);
  uint32_t local = 0;
  constexpr uint32_t output_count = (degree + 2) * (degree + 3) / 2;
  for (uint32_t at = lane; at < output_count; at += 32) {
    uint32_t i = 0, j = at;
    while (j > degree + 1 - i) {
      j -= degree + 2 - i;
      ++i;
    }
    const auto decision = triangle_decision(net, degree, task.task, threshold, i, j);
    if (decision == 1)
      local = 1;
    else if (decision == 2 && local != 1)
      local = 2;
  }
  publish(local, task, answers);
}
template <unsigned N>
__global__ void curve_kernel(const bgmb_root *roots, const Interval *coefficients,
                             const DeviceTask *tasks, uint32_t begin, uint32_t count,
                             bgmb_answer *answers) {
  const uint32_t warp = threadIdx.x / 32, lane = threadIdx.x & 31u, slot = blockIdx.x * 4 + warp;
  if (slot >= count)
    return;
  const auto &task = tasks[begin + slot];
  const auto root = roots[task.task.root];
  Interval parameters[6];
  if (!coordinates(root, task.task, parameters)) {
    if (lane == 0)
      answers[task.output] = {task.task.key, 2, 0};
    return;
  }
  const auto fraction = divide_positive(parameters[0], parameters[1]);
  if (!valid(fraction)) {
    if (lane == 0)
      answers[task.output] = {task.task.key, 2, 0};
    return;
  }
  __shared__ Interval banks[4][36 * 4];
  auto *net = banks[warp];
  const auto *source = coefficients + root.offset;
  for (uint32_t axis = 0; axis < 4; ++axis) {
    Interval current = lane <= N ? source[lane * 4 + axis] : Interval{0, 0};
    const Interval extra = N == 32 && lane == 0 ? source[32 * 4 + axis] : Interval{0, 0};
    Interval left = lane == 0 ? current : Interval{0, 0}, left_extra{0, 0};
    for (uint32_t level = 1; level <= N; ++level) {
      const auto next = shuffle(current, static_cast<int>((lane + 1) & 31u));
      const auto tail = shuffle(extra, 0);
      const auto neighbor = lane == 31 ? tail : next;
      const auto blended = lerp(current, neighbor, parameters[1]);
      current = lane <= N - level ? blended : Interval{0, 0};
      const auto first = shuffle(current, 0);
      if (lane == level)
        left = first;
      if (level == 32 && lane == 0)
        left_extra = first;
    }
    current = left;
    Interval right = lane == N ? current : Interval{0, 0};
    const Interval right_extra = N == 32 && lane == 0 ? left_extra : Interval{0, 0};
    for (uint32_t level = 1; level <= N; ++level) {
      const auto next = shuffle(current, static_cast<int>((lane + 1) & 31u));
      const auto tail = shuffle(left_extra, 0);
      const auto neighbor = lane == 31 ? tail : next;
      const auto blended = lerp(current, neighbor, fraction);
      current = lane <= N - level ? blended : Interval{0, 0};
      if (lane == N - level)
        right = current;
    }
    if (lane <= N)
      net[lane * 4 + axis] = right;
    if (N == 32 && lane == 0)
      net[32 * 4 + axis] = right_extra;
  }
  __syncwarp();
  const auto threshold = limit(net, N + 1, task.task.tolerance);
  uint32_t local = 0;
  for (uint32_t at = lane; at <= N + 1; at += 32) {
    const auto decision = curve_decision(net, N, task.task, threshold, at);
    if (decision == 1)
      local = 1;
    else if (decision == 2 && local != 1)
      local = 2;
  }
  publish(local, task, answers);
}
} // namespace
struct bgmb_session {
  int device = 0;
  uint32_t capacity = 0, root_count = 0, count = 0;
  uint64_t ticket = 0, remaining_work = 0;
  bool active = false, poisoned = false;
  bgmb_root *host_roots = nullptr, *roots = nullptr;
  bgmb_interval *coefficients = nullptr;
  DeviceTask *host_tasks = nullptr, *tasks = nullptr;
  bgmb_answer *host_answers = nullptr, *answers = nullptr;
  cudaStream_t stream = nullptr;
  cudaEvent_t event = nullptr;
};
namespace {
template <typename T> void allocate(T *&value, uint64_t count) {
  check(cudaMalloc(reinterpret_cast<void **>(&value),
                   static_cast<std::size_t>(bytes(count, sizeof(T)))),
        "allocate bound device buffer");
}
template <typename T> void pinned(T *&value, uint64_t count) {
  check(cudaMallocHost(reinterpret_cast<void **>(&value),
                       static_cast<std::size_t>(bytes(count, sizeof(T)))),
        "allocate bound pinned staging");
}
template <typename T> void free_device(T *&value) {
  if (value) {
    check(cudaFree(value), "release bound device buffer");
    value = nullptr;
  }
}
template <typename T> void free_host(T *&value) {
  if (value) {
    check(cudaFreeHost(value), "release bound pinned staging");
    value = nullptr;
  }
}
void release(bgmb_session &s) {
  if (s.stream)
    check(cudaStreamSynchronize(s.stream), "drain bound stream before release");
  free_device(s.answers);
  free_device(s.tasks);
  free_device(s.coefficients);
  free_device(s.roots);
  free_host(s.host_answers);
  free_host(s.host_tasks);
  if (s.event) {
    check(cudaEventDestroy(s.event), "release bound event");
    s.event = nullptr;
  }
  if (s.stream) {
    check(cudaStreamDestroy(s.stream), "release bound stream");
    s.stream = nullptr;
  }
}
void rollback(bgmb_session *s, bgmb_session **output) noexcept {
  if (!s)
    return;
  try {
    DeviceGuard guard(s->device);
    release(*s);
    guard.finish();
    delete[] s->host_roots;
    delete s;
  } catch (...) {
    s->poisoned = true;
    if (output)
      *output = s;
  }
}
template <unsigned P, unsigned Q = 0>
void launch_surface(uint32_t q, bgmb_session &s, uint32_t begin, uint32_t count) {
  if (q == Q)
    surface_kernel<P, Q><<<(count + 3) / 4, 128, 0, s.stream>>>(s.roots, s.coefficients, s.tasks,
                                                                begin, count, s.answers);
  else if constexpr (Q < 3)
    launch_surface<P, Q + 1>(q, s, begin, count);
}
template <unsigned N = 0>
void launch_curve(uint32_t n, bgmb_session &s, uint32_t begin, uint32_t count) {
  if (n == N)
    curve_kernel<N><<<(count + 3) / 4, 128, 0, s.stream>>>(s.roots, s.coefficients, s.tasks, begin,
                                                           count, s.answers);
  else if constexpr (N < 32)
    launch_curve<N + 1>(n, s, begin, count);
}
int32_t create(const bgmb_config *config, const bgmb_root *roots, uint32_t root_count,
               const bgmb_interval *coefficients, uint64_t coefficient_count, bgmb_session **output,
               bgmb_error *out) noexcept {
  if (output)
    *output = nullptr;
  bgmb_session *s = nullptr;
  try {
    require(output && config && roots && coefficients && root_count != 0,
            "null/empty bound create input");
    require(config->abi == BGMESH_BOUND_ABI && config->size == sizeof(bgmb_config) &&
                config->reserved == 0 && config->task_capacity != 0 &&
                config->task_capacity <= std::numeric_limits<uint32_t>::max() - 3,
            "invalid bound configuration");
    uint64_t device = bytes(root_count, sizeof(bgmb_root)), host = sizeof(bgmb_session);
    add_bytes(device, bytes(coefficient_count, sizeof(bgmb_interval)));
    add_bytes(device, bytes(config->task_capacity, sizeof(DeviceTask) + sizeof(bgmb_answer)));
    add_bytes(host, bytes(root_count, sizeof(bgmb_root)));
    add_bytes(host, bytes(config->task_capacity, sizeof(DeviceTask) + sizeof(bgmb_answer)));
    if (device > config->device_bytes || host > config->host_bytes)
      throw Failure{2, "bound session budget exhausted"};
    for (uint32_t i = 0; i < root_count; ++i) {
      const auto &r = roots[i];
      require(r.kind <= 1 && r.reserved == 0 && r.p <= (r.kind == 0 ? 3u : 32u) &&
                  r.q <= (r.kind == 0 ? 3u : 0u) && r.offset <= coefficient_count &&
                  r.count <= coefficient_count - r.offset &&
                  r.count == 4 * (r.p + 1) * (r.kind == 0 ? r.q + 1 : 1),
              "invalid bound root descriptor");
    }
    DeviceGuard guard(config->device);
    cudaDeviceProp properties{};
    check(cudaGetDeviceProperties(&properties, config->device), "query bound device");
    require(properties.major == 6 && properties.minor == 0, "bound profile requires sm60");
    s = new bgmb_session;
    s->device = config->device;
    s->capacity = config->task_capacity;
    s->remaining_work = config->work_limit;
    s->root_count = root_count;
    s->host_roots = new bgmb_root[root_count];
    std::memcpy(s->host_roots, roots, root_count * sizeof(bgmb_root));
    check(cudaStreamCreateWithFlags(&s->stream, cudaStreamNonBlocking), "create bound stream");
    check(cudaEventCreateWithFlags(&s->event, cudaEventDisableTiming), "create bound event");
    allocate(s->roots, root_count);
    allocate(s->coefficients, coefficient_count);
    allocate(s->tasks, s->capacity);
    allocate(s->answers, s->capacity);
    pinned(s->host_tasks, s->capacity);
    pinned(s->host_answers, s->capacity);
    check(cudaMemcpyAsync(s->roots, roots,
                          static_cast<std::size_t>(bytes(root_count, sizeof(bgmb_root))),
                          cudaMemcpyHostToDevice, s->stream),
          "upload bound roots");
    check(cudaMemcpyAsync(s->coefficients, coefficients,
                          static_cast<std::size_t>(bytes(coefficient_count, sizeof(bgmb_interval))),
                          cudaMemcpyHostToDevice, s->stream),
          "upload bound coefficients");
    check(cudaStreamSynchronize(s->stream), "complete bound root upload");
    guard.finish();
    *output = s;
    return 0;
  } catch (const Failure &failure) {
    error(out, failure);
    rollback(s, output);
    return failure.status;
  } catch (const std::bad_alloc &) {
    rollback(s, output);
    error(out, {2, "bound host allocation failed"});
    return 2;
  } catch (...) {
    rollback(s, output);
    error(out, {4, "unexpected bound create exception"});
    return 4;
  }
}
int32_t submit(bgmb_session *s, const bgmb_task *tasks, uint32_t count, uint64_t *ticket,
               bgmb_error *out) noexcept {
  try {
    require(s && tasks && ticket && count != 0 && count <= s->capacity,
            "invalid bound submit input");
    require(!s->poisoned && !s->active, "bound session is active or poisoned");
    require(s->ticket != std::numeric_limits<uint64_t>::max(), "bound ticket exhausted");
    uint32_t counts[49]{}, offsets[49]{}, positions[49]{};
    uint64_t required_work = 0;
    for (uint32_t i = 0; i < count; ++i) {
      require(tasks[i].root < s->root_count && tasks[i].reserved == 0,
              "invalid bound task root/reserved");
      const auto &r = s->host_roots[tasks[i].root];
      add_bytes(required_work, work(r));
      ++counts[r.kind == 0 ? r.p * 4 + r.q : 16 + r.p];
    }
    if (required_work > s->remaining_work)
      throw Failure{2, "bound session work exhausted"};
    s->remaining_work -= required_work;
    for (uint32_t i = 1; i < 49; ++i)
      offsets[i] = offsets[i - 1] + counts[i - 1];
    std::memcpy(positions, offsets, sizeof(offsets));
    for (uint32_t i = 0; i < count; ++i) {
      const auto &r = s->host_roots[tasks[i].root];
      const auto bucket = r.kind == 0 ? r.p * 4 + r.q : 16 + r.p;
      s->host_tasks[positions[bucket]++] = {tasks[i], i, 0};
    }
    DeviceGuard guard(s->device);
    check(cudaMemcpyAsync(s->tasks, s->host_tasks, count * sizeof(DeviceTask),
                          cudaMemcpyHostToDevice, s->stream),
          "upload bound task batch");
    for (uint32_t bucket = 0; bucket < 49; ++bucket)
      if (counts[bucket] != 0) {
        if (bucket >= 16)
          launch_curve(bucket - 16, *s, offsets[bucket], counts[bucket]);
        else
          switch (bucket / 4) {
          case 0:
            launch_surface<0>(bucket % 4, *s, offsets[bucket], counts[bucket]);
            break;
          case 1:
            launch_surface<1>(bucket % 4, *s, offsets[bucket], counts[bucket]);
            break;
          case 2:
            launch_surface<2>(bucket % 4, *s, offsets[bucket], counts[bucket]);
            break;
          case 3:
            launch_surface<3>(bucket % 4, *s, offsets[bucket], counts[bucket]);
            break;
          }
        check(cudaGetLastError(), "launch bound restriction/residual kernel");
      }
    check(cudaMemcpyAsync(s->host_answers, s->answers, count * sizeof(bgmb_answer),
                          cudaMemcpyDeviceToHost, s->stream),
          "download bound decisions");
    check(cudaEventRecord(s->event, s->stream), "record bound completion");
    guard.finish();
    s->count = count;
    s->active = true;
    *ticket = ++s->ticket;
    return 0;
  } catch (const Failure &failure) {
    if (s && failure.status == 3)
      s->poisoned = true;
    error(out, failure);
    return failure.status;
  } catch (...) {
    if (s)
      s->poisoned = true;
    error(out, {4, "unexpected bound submit exception"});
    return 4;
  }
}
int32_t poll(bgmb_session *s, uint64_t ticket, bgmb_answer *answers, uint32_t capacity,
             uint32_t *ready, bgmb_error *out) noexcept {
  if (ready)
    *ready = 0;
  try {
    require(s && answers && ready && s->active && !s->poisoned && ticket == s->ticket &&
                capacity >= s->count,
            "invalid/stale bound poll");
    DeviceGuard guard(s->device);
    const auto status = cudaEventQuery(s->event);
    if (status == cudaErrorNotReady) {
      guard.finish();
      return 0;
    }
    check(status, "query bound completion");
    guard.finish();
    std::memcpy(answers, s->host_answers, s->count * sizeof(bgmb_answer));
    s->active = false;
    *ready = 1;
    return 0;
  } catch (const Failure &failure) {
    if (s && failure.status == 3)
      s->poisoned = true;
    error(out, failure);
    return failure.status;
  } catch (...) {
    if (s)
      s->poisoned = true;
    error(out, {4, "unexpected bound poll exception"});
    return 4;
  }
}
int32_t destroy(bgmb_session *s, bgmb_error *out) noexcept {
  if (!s)
    return 0;
  try {
    DeviceGuard guard(s->device);
    release(*s);
    guard.finish();
    delete[] s->host_roots;
    delete s;
    return 0;
  } catch (const Failure &failure) {
    s->poisoned = true;
    error(out, failure);
    return failure.status;
  } catch (...) {
    s->poisoned = true;
    error(out, {4, "unexpected bound destroy exception"});
    return 4;
  }
}
const bgmb_api api{BGMESH_BOUND_ABI, sizeof(bgmb_api), create, submit, poll, destroy};
} // namespace
extern "C" const bgmb_api *brepgpu_mesher_bound_cuda_v1() { return &api; }
