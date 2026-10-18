#include "math.hpp"
#include "validation.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cuda_runtime.h>
#include <limits>
#include <new>
#include <stdexcept>
namespace bgom10 {
namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
struct CudaFailure : std::runtime_error {
  int code;
  CudaFailure(cudaError_t c, const char *op)
      : std::runtime_error(std::string(op) + ": " + cudaGetErrorString(c)), code(c) {}
};
struct RowFailure : std::runtime_error {
  int64_t row;
  explicit RowFailure(uint64_t r)
      : std::runtime_error("nonfinite/singular arithmetic or out-of-range projection"), row(r) {}
};
void check(cudaError_t c, const char *op) {
  if (c != cudaSuccess)
    throw CudaFailure(c, op);
}
void record(Error *e, const char *stage, const char *msg, int code = 0, int64_t row = -1) {
  if (!e)
    return;
  std::snprintf(e->stage, sizeof(e->stage), "%s", stage);
  std::snprintf(e->message, sizeof(e->message), "%s", msg);
  e->native_code = code;
  e->row = row;
}
struct Session {
  int device = 0;
  cudaStream_t stream = nullptr;
  SceneView scene{}, host{};
  Jet *jets = nullptr;
  Projected *points = nullptr;
  Prepared *triangles = nullptr;
  uint32_t *counts = nullptr;
  uint64_t *offsets = nullptr;
  uint64_t *block_sums = nullptr;
  uint64_t *block_offsets = nullptr;
  uint64_t *long_scan = nullptr;
  uint32_t *long_tiles = nullptr;
  uint32_t *bins = nullptr;
  Sample *samples = nullptr;
  uint8_t *rgba = nullptr;
  double *depth = nullptr;
  uint64_t *primitive = nullptr;
  uint64_t *instance = nullptr;
  unsigned long long *bad = nullptr;
  uint64_t tile_capacity = 0, bin_capacity = 0, sample_capacity = 0, pixel_capacity = 0;
  // Multiprocessor count, read once by open_stream; long_grid is the block
  // count the last build_bins launched for long tiles (0 if it sorted nothing).
  uint32_t multiprocessors = 0, long_grid = 0;
  bool evaluated = false, poisoned = false;
  const char *stage = "session_create";
};
template <class F> bool selected(Session &s, Error *e, F fn) {
  int previous = -1;
  bool ok = true;
  try {
    check(cudaGetDevice(&previous), "read prior device");
    check(cudaSetDevice(s.device), "select device");
    fn();
  } catch (const RowFailure &ex) {
    record(e, s.stage, ex.what(), 0, ex.row);
    ok = false;
  } catch (const CudaFailure &ex) {
    record(e, s.stage, ex.what(), ex.code);
    ok = false;
  } catch (const std::exception &ex) {
    record(e, s.stage, ex.what());
    ok = false;
  } catch (...) {
    record(e, s.stage, "unknown CUDA plugin exception");
    ok = false;
  }
  if (previous >= 0) {
    cudaError_t r = cudaSetDevice(previous);
    if (r != cudaSuccess) {
      if (ok)
        record(e, s.stage, "restore prior device failed", r);
      else if (e) {
        size_t n = std::strlen(e->message);
        std::snprintf(e->message + n, sizeof(e->message) - n, "; restore failed: %s",
                      cudaGetErrorString(r));
      }
      ok = false;
    }
  }
  if (!ok)
    s.poisoned = true;
  return ok;
}
// Creates the session's stream on the selected device and reads its
// multiprocessor count.
void open_stream(Session &s) {
  check(cudaStreamCreateWithFlags(&s.stream, cudaStreamNonBlocking), "create stream");
  int sms = 0;
  check(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, s.device),
        "read multiprocessor count");
  s.multiprocessors = static_cast<uint32_t>(std::max(sms, 1));
}
template <class T> void allocate(T *&ptr, uint64_t n) {
  if (n > SIZE_MAX / sizeof(T))
    throw std::runtime_error("allocation size overflow");
  if (n)
    check(cudaMalloc(reinterpret_cast<void **>(&ptr), n * sizeof(T)), "allocate device storage");
}
template <class T> void copy_in(T *target, const T *source, uint64_t n, Session &s) {
  if (n)
    check(cudaMemcpyAsync(target, source, n * sizeof(T), cudaMemcpyHostToDevice, s.stream),
          "upload scene");
}
template <class T> void copy_out(T *target, const T *source, uint64_t n, Session &s) {
  if (n)
    check(cudaMemcpyAsync(target, source, n * sizeof(T), cudaMemcpyDeviceToHost, s.stream),
          "download result");
}
template <class T> void free_checked(T *&p) {
  if (p) {
    T *v = p;
    p = nullptr;
    check(cudaFree(v), "release prior device storage");
  }
}
void finish(Session &s) {
  check(cudaGetLastError(), "kernel launch");
  check(cudaStreamSynchronize(s.stream), "complete owned stream");
}
void reset_bad(Session &s) {
  check(cudaMemsetAsync(s.bad, 255, sizeof(*s.bad), s.stream), "reset error row");
}
void check_bad(Session &s) {
  unsigned long long row;
  copy_out(&row, s.bad, 1, s);
  finish(s);
  if (row != ~0ull)
    throw RowFailure(row);
}
__global__ void evaluate_kernel(SceneView scene, Jet *jets, unsigned long long *bad) {
  uint64_t i = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i >= scene.nvertices)
    return;
  const Vertex &v = scene.vertices[i];
  if (!evaluate(scene.carriers[v.carrier], scene.controls, v, jets[i])) {
    jets[i].status = 1;
    atomicMin(bad, static_cast<unsigned long long>(i));
  }
}
__global__ void project_kernel(SceneView scene, Request r, const Jet *jets, Projected *points,
                               unsigned long long *bad) {
  uint64_t i = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < scene.nplaced_vertices && !project(scene, r, i, jets, points[i]))
    atomicMin(bad, static_cast<unsigned long long>(i));
}
__global__ void setup_kernel(SceneView scene, Request r, const Projected *points,
                             Prepared *triangles) {
  uint64_t i = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < scene.nprimitives)
    setup(scene, r, i, points, triangles[i]);
}
__global__ void count_kernel(const Prepared *triangles, uint64_t n, uint32_t tiles_x,
                             uint32_t *counts) {
  uint64_t i = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i >= n)
    return;
  const Prepared &p = triangles[i];
  if (!p.area)
    return;
  for (int y = p.ymin / 16; y <= p.ymax / 16; ++y)
    for (int x = p.xmin / 16; x <= p.xmax / 16; ++x)
      atomicAdd(counts + uint64_t(y) * tiles_x + x, 1u);
}
// Fixed maximum8192sample image ->262144tiles ->512block sums.
template <class T> __global__ void scan512(const T *in, uint64_t *out, uint64_t *sums, uint64_t n) {
  __shared__ uint64_t data[512];
  unsigned tid = threadIdx.x;
  uint64_t start = uint64_t(blockIdx.x) * 512;
  data[tid] = start + tid < n ? in[start + tid] : 0;
  data[tid + 256] = start + tid + 256 < n ? in[start + tid + 256] : 0;
  __syncthreads();
  for (unsigned stride = 1; stride < 512; stride *= 2) {
    unsigned k = (tid + 1) * stride * 2 - 1;
    if (k < 512)
      data[k] += data[k - stride];
    __syncthreads();
  }
  if (tid == 0) {
    if (sums)
      sums[blockIdx.x] = data[511];
    data[511] = 0;
  }
  __syncthreads();
  for (unsigned stride = 256; stride; stride /= 2) {
    unsigned k = (tid + 1) * stride * 2 - 1;
    if (k < 512) {
      uint64_t t = data[k - stride];
      data[k - stride] = data[k];
      data[k] += t;
    }
    __syncthreads();
  }
  if (start + tid < n)
    out[start + tid] = data[tid];
  if (start + tid + 256 < n)
    out[start + tid + 256] = data[tid + 256];
}
__global__ void add_scan(uint64_t *offsets, const uint64_t *block_offsets, const uint64_t *sums,
                         uint64_t n, uint32_t blocks) {
  uint64_t i = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n)
    offsets[i] += block_offsets[i / 512];
  if (i == 0)
    offsets[n] = block_offsets[blocks - 1] + sums[blocks - 1];
}
__global__ void fill_kernel(const Prepared *triangles, uint64_t n, uint32_t tiles_x,
                            uint32_t *cursor, const uint64_t *offsets, uint32_t *bins) {
  uint64_t i = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i >= n)
    return;
  const Prepared &p = triangles[i];
  if (!p.area)
    return;
  for (int y = p.ymin / 16; y <= p.ymax / 16; ++y)
    for (int x = p.xmin / 16; x <= p.xmax / 16; ++x) {
      uint64_t tile = uint64_t(y) * tiles_x + x;
      bins[offsets[tile] + atomicAdd(cursor + tile, 1u)] = i;
    }
}
// fill_kernel's atomic cursors place a tile's entries in arrival order. The
// order kernels then sort each tile's entries ascending. A primitive appears at
// most once per tile, so the ascending list is unique: the published bins are a
// function of the triangles alone, as the determinism paper's section 2.2
// requires (item 4, canonical publication; item 5, no slot-allocating atomics
// in published layers). Every network is Batcher's bitonic sort, and each tile
// is sorted by exactly one path, chosen by its length k:
//   2..32       one warp, one entry per lane (warp_sort<1>);
//   33..256     one warp, eight entries per lane (warp_sort<8>);
//   257 and up  one block (order_long_bins_kernel), for tiles listed by scan.
constexpr uint32_t SHORT_BIN = 32, MEDIUM_BIN = 256, SORT_SHARED = 4096;
// One warp holds the list in registers, padded with UINT32_MAX. Primitive
// indices are below UINT32_MAX (validation caps the count at UINT32_MAX), so
// the padding sorts after every entry. Position p = lane * ITEMS + j; partners
// at distance half >= ITEMS sit in another lane and are read by shuffle.
template <uint32_t ITEMS> __device__ void warp_sort(uint32_t *list, uint32_t k, uint32_t lane) {
  static_assert(ITEMS && (ITEMS & (ITEMS - 1)) == 0, "power-of-two lane width");
  uint32_t e[ITEMS];
#pragma unroll
  for (uint32_t j = 0; j < ITEMS; ++j) {
    const uint32_t p = lane * ITEMS + j;
    e[j] = p < k ? list[p] : UINT32_MAX;
  }
#pragma unroll
  for (uint32_t size = 2; size <= 32 * ITEMS; size <<= 1)
#pragma unroll
    for (uint32_t half = size / 2; half; half >>= 1)
#pragma unroll
      for (uint32_t j = 0; j < ITEMS; ++j) {
        const uint32_t p = lane * ITEMS + j;
        const bool ascending = (p & size) == 0;
        if (half >= ITEMS) {
          const uint32_t other = __shfl_xor_sync(0xffffffffu, e[j], half / ITEMS);
          e[j] = ((p & half) == 0) == ascending ? min(e[j], other) : max(e[j], other);
        } else if ((j & half) == 0 && j + half < ITEMS) {
          const uint32_t a = e[j], b = e[j + half];
          e[j] = ascending ? min(a, b) : max(a, b);
          e[j + half] = ascending ? max(a, b) : min(a, b);
        }
      }
#pragma unroll
  for (uint32_t j = 0; j < ITEMS; ++j) {
    const uint32_t p = lane * ITEMS + j;
    if (p < k)
      list[p] = e[j];
  }
}
// One warp per tile. Sorts lists of 2..256 entries and sets long_flags[tile]
// for lists longer than MEDIUM_BIN; a scan of the flags then gathers those
// tiles, in tile order, for order_long_bins_kernel.
__global__ void order_warp_bins_kernel(const uint64_t *offsets, uint64_t ntiles, uint32_t *bins,
                                       uint32_t *long_flags) {
  const uint64_t tile = (uint64_t(blockIdx.x) * blockDim.x + threadIdx.x) / 32;
  const uint32_t lane = threadIdx.x % 32;
  if (tile >= ntiles)
    return;
  const uint64_t begin = offsets[tile], k = offsets[tile + 1] - begin;
  if (lane == 0)
    long_flags[tile] = k > MEDIUM_BIN;
  if (k < 2 || k > MEDIUM_BIN)
    return;
  if (k <= SHORT_BIN)
    warp_sort<1>(bins + begin, static_cast<uint32_t>(k), lane);
  else
    warp_sort<MEDIUM_BIN / 32>(bins + begin, static_cast<uint32_t>(k), lane);
}
__global__ void gather_long_kernel(const uint32_t *long_flags, const uint64_t *long_scan,
                                   uint64_t ntiles, uint32_t *long_tiles) {
  const uint64_t tile = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (tile < ntiles && long_flags[tile])
    long_tiles[long_scan[tile]] = static_cast<uint32_t>(tile);
}
// Single-direction bitonic form: each merge of width `size` first compares
// mirrored pairs, then halves. Every comparator puts the minimum at the lower
// index, so slots at or past k act as +infinity padding and comparators
// touching them are skipped. Index I is 32-bit for shared-memory lists and
// 64-bit for global ones. half is a power of two, so comparator c's lower slot
// is c with a zero bit inserted at half: ((c & ~(half - 1)) << 1) | (c & (half - 1)).
template <class I> __device__ void block_sort(uint32_t *data, I k) {
  I width = 1;
  while (width < k)
    width <<= 1;
  for (I size = 2; size <= width; size <<= 1)
    for (I half = size / 2; half; half >>= 1) {
      for (I c = threadIdx.x; c < width / 2; c += blockDim.x) {
        const I i = ((c & ~(half - 1)) << 1) | (c & (half - 1));
        const I j = half == size / 2 ? (i | (size - 1)) - (i & (size - 1)) : i + half;
        if (j < k && data[i] > data[j]) {
          const uint32_t t = data[i];
          data[i] = data[j];
          data[j] = t;
        }
      }
      __syncthreads();
    }
}
// Persistent grid over the gathered long tiles; *count is the gathered length.
// Lists of up to SORT_SHARED entries sort in shared memory, longer ones in place
// in global memory. The network costs O(k log^2 k / 256) steps per thread,
// against visibility_kernel's O(k) per thread on the same tile.
__global__ void order_long_bins_kernel(const uint64_t *offsets, const uint32_t *long_tiles,
                                       const uint64_t *count, uint32_t *bins) {
  __shared__ uint32_t local[SORT_SHARED];
  const uint64_t n = *count;
  for (uint64_t item = blockIdx.x; item < n; item += gridDim.x) {
    const uint32_t tile = long_tiles[item];
    const uint64_t begin = offsets[tile], k = offsets[tile + 1] - begin;
    if (k <= SORT_SHARED) {
      for (uint32_t i = threadIdx.x; i < k; i += blockDim.x)
        local[i] = bins[begin + i];
      __syncthreads();
      block_sort<uint32_t>(local, static_cast<uint32_t>(k));
      for (uint32_t i = threadIdx.x; i < k; i += blockDim.x)
        bins[begin + i] = local[i];
    } else
      block_sort<uint64_t>(bins + begin, k);
    __syncthreads();
  }
}
__global__ void visibility_kernel(const Prepared *triangles, const uint64_t *offsets,
                                  const uint32_t *bins, uint32_t width, uint32_t height,
                                  uint32_t tiles_x, Sample *samples, unsigned long long *bad) {
  uint32_t x = blockIdx.x * 16 + threadIdx.x, y = blockIdx.y * 16 + threadIdx.y;
  if (x >= width || y >= height)
    return;
  uint64_t tile = uint64_t(blockIdx.y) * tiles_x + blockIdx.x;
  Sample sample{-INFINITY, UINT64_MAX, UINT64_MAX};
  for (uint64_t i = offsets[tile]; i < offsets[tile + 1]; ++i) {
    const Prepared &p = triangles[bins[i]];
    if (x < static_cast<uint32_t>(p.xmin) || x > static_cast<uint32_t>(p.xmax) ||
        y < static_cast<uint32_t>(p.ymin) || y > static_cast<uint32_t>(p.ymax))
      continue;
    double w[3], z;
    if (!covered(p, x, y, w, z))
      continue;
    if (!finite(z)) {
      atomicMin(bad, static_cast<unsigned long long>(uint64_t(y) * width + x));
      continue;
    }
    if (z > sample.depth || (z == sample.depth && p.primitive < sample.primitive))
      sample = {z, p.primitive, p.instance};
  }
  samples[uint64_t(y) * width + x] = sample;
}
__global__ void resolve_kernel(const Prepared *triangles, const Projected *points,
                               const Material *materials, Request r, const Sample *samples,
                               uint8_t *rgba, double *depths, uint64_t *primitives,
                               uint64_t *instances, unsigned long long *bad) {
  uint64_t index = uint64_t(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index >= uint64_t(r.width) * r.height)
    return;
  uint32_t x = index % r.width, y = index / r.width, width = r.width * r.supersample;
  const Sample &chosen = samples[uint64_t(y * r.supersample) * width + x * r.supersample];
  depths[index] = chosen.depth;
  primitives[index] = chosen.primitive;
  instances[index] = chosen.instance;
  Vec color{};
  for (uint32_t sy = 0; sy < r.supersample; ++sy)
    for (uint32_t sx = 0; sx < r.supersample; ++sx) {
      uint32_t xx = x * r.supersample + sx, yy = y * r.supersample + sy;
      const Sample &sample = samples[uint64_t(yy) * width + xx];
      Vec c = vec(r.background);
      if (sample.primitive != UINT64_MAX) {
        const Prepared &p = triangles[sample.primitive];
        double w[3], z;
        if (!covered(p, xx, yy, w, z) || !shade(p, points, materials, r, w, c)) {
          atomicMin(bad, static_cast<unsigned long long>(index));
          return;
        }
      }
      color = add(color, c);
    }
  color = mul(color, 1.0 / (r.supersample * r.supersample));
  if (!good(color)) {
    atomicMin(bad, static_cast<unsigned long long>(index));
    return;
  }
  rgba[4 * index] = encode(color.x);
  rgba[4 * index + 1] = encode(color.y);
  rgba[4 * index + 2] = encode(color.z);
  rgba[4 * index + 3] = 255;
}
void cleanup(Session &s, Error *e, bool &ok) {
  auto release = [&](auto *&p) {
    if (p) {
      auto *v = p;
      p = nullptr;
      cudaError_t c = cudaFree(v);
      if (c != cudaSuccess) {
        if (ok)
          record(e, "session_close", "free device storage failed", c);
        ok = false;
      }
    }
  };
  if (s.stream) {
    cudaError_t c = cudaStreamSynchronize(s.stream);
    if (c != cudaSuccess) {
      if (ok)
        record(e, "session_close", "complete stream during close failed", c);
      ok = false;
    }
  }
  // Scene arrays are immutable in kernels, allocations remain owned here.
  auto *definitions = const_cast<Definition *>(s.scene.definitions);
  release(definitions);
  auto *instances = const_cast<Instance *>(s.scene.instances);
  release(instances);
  auto *vertices = const_cast<Vertex *>(s.scene.vertices);
  release(vertices);
  auto *triangles = const_cast<Triangle *>(s.scene.triangles);
  release(triangles);
  auto *carriers = const_cast<Carrier *>(s.scene.carriers);
  release(carriers);
  auto *materials = const_cast<Material *>(s.scene.materials);
  release(materials);
  auto *controls = const_cast<Control *>(s.scene.controls);
  release(controls);
  auto *vr = const_cast<VertexRef *>(s.scene.vertex_refs);
  release(vr);
  auto *pr = const_cast<PrimitiveRef *>(s.scene.primitive_refs);
  release(pr);
  s.scene = {};
  release(s.jets);
  release(s.points);
  release(s.triangles);
  release(s.counts);
  release(s.offsets);
  release(s.block_sums);
  release(s.block_offsets);
  release(s.long_scan);
  release(s.long_tiles);
  release(s.bins);
  release(s.samples);
  release(s.rgba);
  release(s.depth);
  release(s.primitive);
  release(s.instance);
  release(s.bad);
  if (s.stream) {
    cudaError_t c = cudaStreamDestroy(s.stream);
    s.stream = nullptr;
    if (c != cudaSuccess) {
      if (ok)
        record(e, "session_close", "destroy stream failed", c);
      ok = false;
    }
  }
}
bool create_profile(const SceneView *view, int32_t device, void **out, Error *e, Times *times,
                    bool source_profile) {
  if (!out || !times) {
    record(e, "session_create", "missing CUDA output/times");
    return false;
  }
  *out = nullptr;
  try {
    validate_scene_view(view, source_profile);
  } catch (const std::exception &failure) {
    record(e, "session_create", failure.what());
    return false;
  }
  Session *s = nullptr;
  try {
    s = new Session;
    s->device = device;
    s->host = *view;
    s->scene = *view;
    s->scene.definitions = nullptr;
    s->scene.instances = nullptr;
    s->scene.vertices = nullptr;
    s->scene.triangles = nullptr;
    s->scene.carriers = nullptr;
    s->scene.materials = nullptr;
    s->scene.controls = nullptr;
    s->scene.vertex_refs = nullptr;
    s->scene.primitive_refs = nullptr;
  } catch (...) {
    record(e, "session_create", "host allocation failed");
    return false;
  }
  auto start = Clock::now();
  bool ok = selected(*s, e, [&] {
    open_stream(*s);
    times->create = ms(start);
    s->stage = "scene_allocate_upload";
    auto t = Clock::now();
#define COPY_FIELD(field, count, Type)                                                             \
  {                                                                                                \
    Type *p = nullptr;                                                                             \
    allocate(p, view->count);                                                                      \
    s->scene.field = p;                                                                            \
    copy_in(p, view->field, view->count, *s);                                                      \
  }
    COPY_FIELD(definitions, ndefinitions, Definition);
    COPY_FIELD(instances, ninstances, Instance);
    COPY_FIELD(vertices, nvertices, Vertex);
    COPY_FIELD(triangles, ntriangles, Triangle);
    COPY_FIELD(carriers, ncarriers, Carrier);
    COPY_FIELD(materials, nmaterials, Material);
    COPY_FIELD(controls, ncontrols, Control);
    COPY_FIELD(vertex_refs, nplaced_vertices, VertexRef);
    COPY_FIELD(primitive_refs, nprimitives, PrimitiveRef);
#undef COPY_FIELD
    allocate(s->jets, view->nvertices);
    allocate(s->points, view->nplaced_vertices);
    allocate(s->triangles, view->nprimitives);
    allocate(s->bad, 1);
    finish(*s);
    times->upload = ms(t);
  });
  if (!ok) {
    Error secondary{};
    bool clean = true;
    // No allocation precedes stream creation. A rejected device owns nothing.
    bool cleaned = !s->stream || selected(*s, &secondary, [&] { cleanup(*s, &secondary, clean); });
    if ((!clean || !cleaned) && e) {
      size_t used = std::strlen(e->message);
      std::snprintf(e->message + used, sizeof(e->message) - used, "; cleanup: %s",
                    secondary.message);
    }
    delete s;
    return false;
  }
  *out = s;
  return true;
}
bool evaluate_gpu(void *token, Jet *out, Error *e, Times *times) {
  Session &s = *static_cast<Session *>(token);
  s.stage = "source_evaluate";
  if (s.poisoned) {
    record(e, s.stage, "session unusable");
    return false;
  }
  return selected(s, e, [&] {
    auto t = Clock::now();
    reset_bad(s);
    if (s.scene.nvertices)
      evaluate_kernel<<<(s.scene.nvertices + 255) / 256, 256, 0, s.stream>>>(s.scene, s.jets,
                                                                             s.bad);
    check_bad(s);
    times->evaluate = ms(t);
    s.evaluated = true;
    if (out) {
      s.stage = "source_jet_download";
      t = Clock::now();
      copy_out(out, s.jets, s.scene.nvertices, s);
      finish(s);
      times->download = ms(t);
    }
  });
}
void reserve_tiles(Session &s, uint64_t n) {
  if (n <= s.tile_capacity)
    return;
  free_checked(s.counts);
  free_checked(s.offsets);
  free_checked(s.block_sums);
  free_checked(s.block_offsets);
  free_checked(s.long_scan);
  free_checked(s.long_tiles);
  allocate(s.counts, n);
  allocate(s.long_scan, n + 1);
  allocate(s.long_tiles, n);
  allocate(s.offsets, n + 1);
  allocate(s.block_sums, (n + 511) / 512);
  allocate(s.block_offsets, (n + 511) / 512);
  s.tile_capacity = n;
}
// Builds complete CSR tile bins for the session's prepared triangles:
// s.offsets[0..ntiles] and s.bins[0..references), each tile's entries in
// ascending primitive order. Returns the reference count. The ordering runs as
// its own stage, "tile_bin_sort", so a failure there is attributed to it.
uint64_t build_bins(Session &s, uint64_t nprimitives, uint32_t tiles_x, uint64_t ntiles) {
  require(s.multiprocessors, "bins: session stream not opened");
  s.long_grid = 0;
  reserve_tiles(s, ntiles);
  check(cudaMemsetAsync(s.counts, 0, ntiles * sizeof(uint32_t), s.stream), "zero tile counts");
  if (nprimitives)
    count_kernel<<<(nprimitives + 255) / 256, 256, 0, s.stream>>>(s.triangles, nprimitives,
                                                                  tiles_x, s.counts);
  uint32_t blocks = (ntiles + 511) / 512;
  scan512<<<blocks, 256, 0, s.stream>>>(s.counts, s.offsets, s.block_sums, ntiles);
  scan512<<<1, 256, 0, s.stream>>>(s.block_sums, s.block_offsets, nullptr, blocks);
  add_scan<<<(ntiles + 255) / 256, 256, 0, s.stream>>>(s.offsets, s.block_offsets, s.block_sums,
                                                       ntiles, blocks);
  uint64_t references = 0;
  copy_out(&references, s.offsets + ntiles, 1, s);
  finish(s);
  if (references > s.bin_capacity) {
    free_checked(s.bins);
    allocate(s.bins, references);
    s.bin_capacity = references;
  }
  check(cudaMemsetAsync(s.counts, 0, ntiles * sizeof(uint32_t), s.stream), "zero tile cursors");
  if (nprimitives)
    fill_kernel<<<(nprimitives + 255) / 256, 256, 0, s.stream>>>(s.triangles, nprimitives, tiles_x,
                                                                 s.counts, s.offsets, s.bins);
  finish(s);
  if (references > 1) {
    const char *outer = s.stage;
    s.stage = "tile_bin_sort";
    order_warp_bins_kernel<<<static_cast<uint32_t>((ntiles + 7) / 8), 256, 0, s.stream>>>(
        s.offsets, ntiles, s.bins, s.counts);
    scan512<<<blocks, 256, 0, s.stream>>>(s.counts, s.long_scan, s.block_sums, ntiles);
    scan512<<<1, 256, 0, s.stream>>>(s.block_sums, s.block_offsets, nullptr, blocks);
    add_scan<<<(ntiles + 255) / 256, 256, 0, s.stream>>>(s.long_scan, s.block_offsets,
                                                         s.block_sums, ntiles, blocks);
    gather_long_kernel<<<(ntiles + 255) / 256, 256, 0, s.stream>>>(s.counts, s.long_scan, ntiles,
                                                                   s.long_tiles);
    // Four 16 KiB blocks fit one P100 SM; the grid loops over the gathered tiles.
    s.long_grid = 4 * s.multiprocessors;
    order_long_bins_kernel<<<s.long_grid, 256, 0, s.stream>>>(s.offsets, s.long_tiles,
                                                              s.long_scan + ntiles, s.bins);
    finish(s);
    s.stage = outer;
  }
  return references;
}
void reserve_frame(Session &s, uint64_t nsamples, uint64_t npixels) {
  if (nsamples > s.sample_capacity) {
    free_checked(s.samples);
    allocate(s.samples, nsamples);
    s.sample_capacity = nsamples;
  }
  if (npixels > s.pixel_capacity) {
    free_checked(s.rgba);
    free_checked(s.depth);
    free_checked(s.primitive);
    free_checked(s.instance);
    allocate(s.rgba, npixels * 4);
    allocate(s.depth, npixels);
    allocate(s.primitive, npixels);
    allocate(s.instance, npixels);
    s.pixel_capacity = npixels;
  }
}
bool render_gpu(void *token, const Request *request, FrameOutput *out, Error *e, Times *times) {
  try {
    require(token && request && out && times, "render: missing required argument");
    validate_request(*request);
    require((request->flags & PROJECT_ONLY) ||
                (out->rgba && out->depth && out->primitive && out->instance),
            "render: missing frame output arrays");
  } catch (const std::exception &failure) {
    record(e, "render", failure.what());
    return false;
  }
  Session &s = *static_cast<Session *>(token);
  const Request r = *request;
  s.stage = "instance_transform_project";
  if (s.poisoned || !s.evaluated) {
    record(e, s.stage, "unevaluated or unusable session");
    return false;
  }
  return selected(s, e, [&] {
    auto t = Clock::now();
    reset_bad(s);
    if (s.scene.nplaced_vertices)
      project_kernel<<<(s.scene.nplaced_vertices + 255) / 256, 256, 0, s.stream>>>(
          s.scene, r, s.jets, s.points, s.bad);
    check_bad(s);
    times->project = ms(t);
    if (out->projected) {
      s.stage = "projected_download";
      t = Clock::now();
      copy_out(out->projected, s.points, s.scene.nplaced_vertices, s);
      finish(s);
      times->download += ms(t);
    }
    if (r.flags & PROJECT_ONLY)
      return;
    uint32_t width = r.width * r.supersample, height = r.height * r.supersample,
             tx = (width + 15) / 16, ty = (height + 15) / 16;
    uint64_t ntiles = uint64_t(tx) * ty, npixels = uint64_t(r.width) * r.height,
             nsamples = uint64_t(width) * height;
    s.stage = "triangle_setup";
    t = Clock::now();
    if (s.scene.nprimitives)
      setup_kernel<<<(s.scene.nprimitives + 255) / 256, 256, 0, s.stream>>>(s.scene, r, s.points,
                                                                            s.triangles);
    finish(s);
    times->setup = ms(t);
    s.stage = "tile_count_scan_allocate_fill";
    t = Clock::now();
    build_bins(s, s.scene.nprimitives, tx, ntiles);
    times->bins = ms(t);
    s.stage = "visibility_shading_resolve";
    t = Clock::now();
    reserve_frame(s, nsamples, npixels);
    reset_bad(s);
    visibility_kernel<<<dim3(tx, ty), dim3(16, 16), 0, s.stream>>>(
        s.triangles, s.offsets, s.bins, width, height, tx, s.samples, s.bad);
    resolve_kernel<<<(npixels + 255) / 256, 256, 0, s.stream>>>(
        s.triangles, s.points, s.scene.materials, r, s.samples, s.rgba, s.depth, s.primitive,
        s.instance, s.bad);
    check_bad(s);
    times->display = ms(t);
    s.stage = "result_download";
    t = Clock::now();
    copy_out(out->rgba, s.rgba, npixels * 4, s);
    copy_out(out->depth, s.depth, npixels, s);
    copy_out(out->primitive, s.primitive, npixels, s);
    copy_out(out->instance, s.instance, npixels, s);
    if (out->samples)
      copy_out(out->samples, s.samples, nsamples, s);
    finish(s);
    times->download += ms(t);
  });
}
bool close_gpu(void *token, Error *e, Times *times) {
  Session *s = static_cast<Session *>(token);
  if (!s)
    return true;
  s->stage = "session_close";
  auto t = Clock::now();
  bool clean = true;
  bool selected_ok = selected(*s, e, [&] { cleanup(*s, e, clean); });
  delete s;
  times->close = ms(t);
  return clean && selected_ok;
}
bool create(const SceneView *view, uint32_t, int32_t device, void **out, Error *e, Times *times) {
  return create_profile(view, device, out, e, times, false);
}
bool create_v2(const SceneView *view, uint32_t, int32_t device, void **out, Error *e,
               Times *times) {
  return create_profile(view, device, out, e, times, true);
}
const Api api = {1, sizeof(Api), create, evaluate_gpu, render_gpu, close_gpu};
const Api api_v2 = {2, sizeof(Api), create_v2, evaluate_gpu, render_gpu, close_gpu};
} // namespace
} // namespace bgom10
extern "C" const bgom10::Api *brepgpu_om10_cuda_v1() { return &bgom10::api; }
extern "C" const bgom10::Api *brepgpu_om10_cuda_v2() { return &bgom10::api_v2; }
