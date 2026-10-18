#include "math.hpp"
#include "scene.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
namespace bgom10 {
namespace {
using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
void error(Error *e, const char *stage, const char *message, int64_t row = -1) {
  if (e) {
    std::snprintf(e->stage, sizeof(e->stage), "%s", stage);
    std::snprintf(e->message, sizeof(e->message), "%s", message);
    e->row = row;
    e->native_code = 0;
  }
}
template <class F> bool guard(Error *e, const char *stage, F f) {
  try {
    return f();
  } catch (const std::exception &ex) {
    error(e, stage, ex.what());
    return false;
  } catch (...) {
    error(e, stage, "unknown native exception");
    return false;
  }
}
class Pool {
  std::vector<std::thread> workers;
  std::mutex mutex;
  std::condition_variable ready, done;
  bool stop = false;
  uint64_t generation = 0, finished = 0, count = 0;
  std::atomic<uint64_t> next{0};
  uint64_t grain = 32;
  std::function<void(uint64_t)> job;
  void drain() {
    for (;;) {
      uint64_t begin = next.fetch_add(grain);
      if (begin >= count)
        break;
      for (uint64_t i = begin; i < std::min(begin + grain, count); ++i)
        job(i);
    }
  }
  void worker() {
    uint64_t seen = 0;
    for (;;) {
      std::unique_lock<std::mutex> lock(mutex);
      ready.wait(lock, [&] { return stop || generation != seen; });
      if (stop)
        return;
      seen = generation;
      lock.unlock();
      drain();
      lock.lock();
      ++finished;
      done.notify_one();
    }
  }
  void close() {
    {
      std::lock_guard<std::mutex> lock(mutex);
      stop = true;
    }
    ready.notify_all();
    for (auto &w : workers)
      if (w.joinable())
        w.join();
  }

public:
  explicit Pool(uint32_t n) {
    try {
      for (uint32_t i = 1; i < n; ++i)
        workers.emplace_back([this] { worker(); });
    } catch (...) {
      close();
      throw;
    }
  }
  ~Pool() { close(); }
  uint32_t size() const { return uint32_t(workers.size() + 1); }
  // Runs f(0..n-1); workers claim `step` consecutive indices at a time.
  void parallel(uint64_t n, std::function<void(uint64_t)> f, uint64_t step = 32) {
    require(step > 0, "pool: zero claim size");
    {
      std::lock_guard<std::mutex> lock(mutex);
      job = std::move(f);
      count = n;
      grain = step;
      next = 0;
      finished = 0;
      ++generation;
    }
    ready.notify_all();
    drain();
    std::unique_lock<std::mutex> lock(mutex);
    done.wait(lock, [&] { return finished == workers.size(); });
    job = {};
  }
};
struct Session {
  SceneView scene;
  Pool pool;
  std::vector<Jet> jets;
  std::vector<Projected> points;
  std::vector<Prepared> triangles;
  std::vector<uint64_t> offsets;
  std::vector<uint32_t> bins;
  // Bin builder scratch, kept across frames: per-primitive reference counts,
  // chunk boundaries, block prefixes of the counts, per-chunk tile rows;
  // bin_chunks is the last chunk count.
  std::vector<uint32_t> bin_references, bin_rows;
  std::vector<uint64_t> bin_first, bin_block_prefix;
  uint64_t bin_chunks = 0;
  std::vector<Sample> samples;
  bool evaluated = false, poisoned = false;
  Session(SceneView s, uint32_t n) : scene(s), pool(n), jets(s.nvertices) {}
};
void first_error(std::atomic<uint64_t> &out, uint64_t i) {
  uint64_t old = out.load();
  while (i < old && !out.compare_exchange_weak(old, i)) {
  }
}
// Builds complete CSR tile bins for s.triangles: s.offsets[0..ntiles] and
// s.bins[0..offsets[ntiles]), each tile's entries in ascending primitive order.
// The primitives split into contiguous index ranges ("chunks") holding about
// equal numbers of tile references, from a prefix over per-primitive counts.
// Each chunk counts its references into a private row, a per-tile scan over the
// rows in chunk order gives every chunk its start inside each tile, and each
// chunk then fills its slots walking its range in ascending order. No slot comes
// from an atomic. The chunk count depends on the worker count; the result does
// not, since any chunking yields the sequential order (determinism paper,
// section 2.2: item 3, a decomposition that does not influence results; item 4,
// canonical publication; item 5, no slot-allocating atomics). Rows cost
// 4 * chunks * ntiles bytes, held to ROW_BUDGET entries (64 MiB).
constexpr uint64_t ROW_BUDGET = uint64_t(1) << 24, REF_BLOCK = 4096;
void build_bins(Session &s, uint32_t tiles_x, uint64_t ntiles) {
  const uint64_t n = s.triangles.size();
  const uint64_t chunks = std::max<uint64_t>(
      1, std::min<uint64_t>({s.pool.size(), std::max<uint64_t>(1, ROW_BUDGET / ntiles), n}));
  s.bin_chunks = chunks;
  // Per-primitive reference counts, with a sum per block of REF_BLOCK
  // primitives so the prefix over blocks is short.
  const uint64_t nblocks = (n + REF_BLOCK - 1) / REF_BLOCK;
  s.bin_references.resize(n);
  s.bin_block_prefix.resize(nblocks + 1);
  s.pool.parallel(
      nblocks,
      [&](uint64_t block) {
        uint64_t sum = 0;
        for (uint64_t i = block * REF_BLOCK; i < std::min(n, (block + 1) * REF_BLOCK); ++i) {
          const auto &p = s.triangles[i];
          s.bin_references[i] = p.area ? uint32_t(p.xmax / 16 - p.xmin / 16 + 1) *
                                             uint32_t(p.ymax / 16 - p.ymin / 16 + 1)
                                       : 0;
          sum += s.bin_references[i];
        }
        s.bin_block_prefix[block + 1] = sum;
      },
      1);
  s.bin_block_prefix[0] = 0;
  for (uint64_t block = 0; block < nblocks; ++block)
    s.bin_block_prefix[block + 1] += s.bin_block_prefix[block];
  // Chunk c starts at the first primitive i whose preceding references,
  // prefix(i), reach c * total / chunks (written so the product cannot
  // overflow): a binary search over block prefixes, then a scan inside one block.
  const uint64_t total = s.bin_block_prefix[nblocks];
  s.bin_first.assign(chunks + 1, n);
  s.bin_first[0] = 0;
  for (uint64_t c = 1; c < chunks; ++c) {
    const uint64_t goal = total / chunks * c + total % chunks * c / chunks;
    const uint64_t block =
        std::lower_bound(s.bin_block_prefix.begin(), s.bin_block_prefix.end(), goal) -
        s.bin_block_prefix.begin();
    uint64_t i = 0;
    if (block) {
      i = (block - 1) * REF_BLOCK;
      uint64_t seen = s.bin_block_prefix[block - 1];
      while (i < n && seen < goal)
        seen += s.bin_references[i++];
    }
    s.bin_first[c] = i;
  }
  s.bin_rows.resize(chunks * ntiles);
  const auto each_reference = [&](uint64_t c, auto f) {
    for (uint64_t i = s.bin_first[c]; i < s.bin_first[c + 1]; ++i) {
      const auto &p = s.triangles[i];
      if (!p.area)
        continue;
      for (int32_t y = p.ymin / 16; y <= p.ymax / 16; ++y)
        for (int32_t x = p.xmin / 16; x <= p.xmax / 16; ++x)
          f(i, uint64_t(y) * tiles_x + x);
    }
  };
  s.pool.parallel(
      chunks,
      [&](uint64_t c) {
        uint32_t *row = s.bin_rows.data() + c * ntiles;
        std::fill(row, row + ntiles, 0u);
        each_reference(c, [&](uint64_t, uint64_t tile) { ++row[tile]; });
      },
      1);
  s.offsets.resize(ntiles + 1);
  s.pool.parallel(
      ntiles,
      [&](uint64_t tile) {
        uint64_t sum = 0;
        for (uint64_t c = 0; c < chunks; ++c) {
          uint32_t &k = s.bin_rows[c * ntiles + tile];
          const uint32_t count = k;
          k = static_cast<uint32_t>(sum);
          sum += count;
        }
        s.offsets[tile + 1] = sum;
      },
      1024);
  s.offsets[0] = 0;
  for (uint64_t i = 0; i < ntiles; ++i)
    s.offsets[i + 1] = checked_add(s.offsets[i], s.offsets[i + 1]);
  require(s.offsets.back() <= SIZE_MAX / sizeof(uint32_t), "tile bins size overflow");
  s.bins.resize(s.offsets.back());
  s.pool.parallel(
      chunks,
      [&](uint64_t c) {
        uint32_t *row = s.bin_rows.data() + c * ntiles;
        each_reference(c, [&](uint64_t i, uint64_t tile) {
          s.bins[s.offsets[tile] + row[tile]++] = static_cast<uint32_t>(i);
        });
      },
      1);
}
bool create_profile(const SceneView *scene, uint32_t threads, void **out, Error *e, Times *times,
                    bool source_profile) {
  if (!out || !times) {
    error(e, "session_create", "missing CPU output/times");
    return false;
  }
  *out = nullptr;
  auto t = Clock::now();
  bool ok = guard(e, "session_create", [&] {
    validate_scene_view(scene, source_profile);
    if (threads < 1 || threads > 256) {
      error(e, "session_create", "invalid CPU scene/thread count");
      return false;
    }
    *out = new Session(*scene, threads);
    return true;
  });
  times->create = elapsed(t);
  return ok;
}
bool create(const SceneView *scene, uint32_t threads, int32_t, void **out, Error *e, Times *times) {
  return create_profile(scene, threads, out, e, times, false);
}
bool create_v2(const SceneView *scene, uint32_t threads, int32_t, void **out, Error *e,
               Times *times) {
  return create_profile(scene, threads, out, e, times, true);
}
bool evaluate_cpu(void *token, Jet *output, Error *e, Times *times) {
  return guard(e, "source_evaluate", [&] {
    Session &s = *static_cast<Session *>(token);
    if (s.poisoned) {
      error(e, "source_evaluate", "session unusable");
      return false;
    }
    auto t = Clock::now();
    std::atomic<uint64_t> bad{UINT64_MAX};
    s.pool.parallel(s.scene.nvertices, [&](uint64_t i) {
      const Vertex &v = s.scene.vertices[i];
      if (!evaluate(s.scene.carriers[v.carrier], s.scene.controls, v, s.jets[i])) {
        s.jets[i].status = 1;
        first_error(bad, i);
      }
    });
    times->evaluate = elapsed(t);
    if (bad != UINT64_MAX) {
      s.poisoned = true;
      error(e, "source_evaluate", "nonfinite or singular source first jet", bad);
      return false;
    }
    s.evaluated = true;
    if (output) {
      t = Clock::now();
      std::copy(s.jets.begin(), s.jets.end(), output);
      times->download += elapsed(t);
    }
    return true;
  });
}
bool render_cpu(void *token, const Request *request, FrameOutput *output, Error *e, Times *times) {
  return guard(e, "render", [&] {
    require(token && request && output && times, "render: missing required argument");
    validate_request(*request);
    require((request->flags & PROJECT_ONLY) ||
                (output->rgba && output->depth && output->primitive && output->instance),
            "render: missing frame output arrays");
    Session &s = *static_cast<Session *>(token);
    const Request &r = *request;
    if (s.poisoned || !s.evaluated) {
      error(e, "render", "unevaluated or unusable session");
      return false;
    }
    auto t = Clock::now();
    std::atomic<uint64_t> bad{UINT64_MAX};
    s.points.resize(s.scene.nplaced_vertices);
    s.pool.parallel(s.scene.nplaced_vertices, [&](uint64_t i) {
      if (!project(s.scene, r, i, s.jets.data(), s.points[i]))
        first_error(bad, i);
    });
    times->project = elapsed(t);
    if (bad != UINT64_MAX) {
      s.poisoned = true;
      error(e, "instance_transform_project", "nonfinite or out-of-range projection", bad);
      return false;
    }
    if (output->projected) {
      t = Clock::now();
      std::copy(s.points.begin(), s.points.end(), output->projected);
      times->download += elapsed(t);
    }
    if (r.flags & PROJECT_ONLY)
      return true;
    uint32_t width = r.width * r.supersample, height = r.height * r.supersample,
             tiles_x = (width + 15) / 16, tiles_y = (height + 15) / 16;
    uint64_t ntiles = uint64_t(tiles_x) * tiles_y, npixels = uint64_t(width) * height;
    t = Clock::now();
    s.triangles.resize(s.scene.nprimitives);
    s.pool.parallel(s.scene.nprimitives,
                    [&](uint64_t i) { setup(s.scene, r, i, s.points.data(), s.triangles[i]); });
    times->setup = elapsed(t);
    t = Clock::now();
    build_bins(s, tiles_x, ntiles);
    times->bins = elapsed(t);
    t = Clock::now();
    s.samples.resize(npixels);
    s.pool.parallel(ntiles, [&](uint64_t tile) {
      uint32_t tx = tile % tiles_x, ty = tile / tiles_x, x0 = tx * 16, y0 = ty * 16,
               x1 = std::min(width, x0 + 16), y1 = std::min(height, y0 + 16);
      for (uint32_t y = y0; y < y1; ++y)
        for (uint32_t x = x0; x < x1; ++x)
          s.samples[uint64_t(y) * width + x] = {-INFINITY, UINT64_MAX, UINT64_MAX};
      for (uint64_t k = s.offsets[tile]; k < s.offsets[tile + 1]; ++k) {
        const Prepared &p = s.triangles[s.bins[k]];
        for (int32_t y = std::max<int32_t>(y0, p.ymin); y < std::min<int32_t>(y1, p.ymax + 1); ++y)
          for (int32_t x = std::max<int32_t>(x0, p.xmin); x < std::min<int32_t>(x1, p.xmax + 1);
               ++x) {
            double w[3], z;
            if (!covered(p, x, y, w, z))
              continue;
            if (!finite(z)) {
              first_error(bad, uint64_t(y) * width + x);
              continue;
            }
            Sample &sample = s.samples[uint64_t(y) * width + x];
            if (z > sample.depth || (z == sample.depth && p.primitive < sample.primitive))
              sample = {z, p.primitive, p.instance};
          }
      }
    });
    s.pool.parallel(uint64_t(r.width) * r.height, [&](uint64_t index) {
      uint32_t x = index % r.width, y = index / r.width;
      Vec color{};
      const Sample &chosen = s.samples[uint64_t(y * r.supersample) * width + x * r.supersample];
      output->depth[index] = chosen.depth;
      output->primitive[index] = chosen.primitive;
      output->instance[index] = chosen.instance;
      for (uint32_t sy = 0; sy < r.supersample; ++sy)
        for (uint32_t sx = 0; sx < r.supersample; ++sx) {
          uint32_t xx = x * r.supersample + sx, yy = y * r.supersample + sy;
          const Sample &sample = s.samples[uint64_t(yy) * width + xx];
          Vec c = vec(r.background);
          if (sample.primitive != UINT64_MAX) {
            const Prepared &p = s.triangles[sample.primitive];
            double w[3], z;
            if (!covered(p, xx, yy, w, z) ||
                !shade(p, s.points.data(), s.scene.materials, r, w, c)) {
              first_error(bad, index);
              return;
            }
          }
          color = add(color, c);
        }
      color = mul(color, 1.0 / (r.supersample * r.supersample));
      if (!good(color)) {
        first_error(bad, index);
        return;
      }
      output->rgba[4 * index] = encode(color.x);
      output->rgba[4 * index + 1] = encode(color.y);
      output->rgba[4 * index + 2] = encode(color.z);
      output->rgba[4 * index + 3] = 255;
    });
    times->display = elapsed(t);
    if (bad != UINT64_MAX) {
      s.poisoned = true;
      error(e, "visibility_shading_resolve", "nonfinite depth or singular interpolated normal",
            bad);
      return false;
    }
    if (output->samples) {
      t = Clock::now();
      std::copy(s.samples.begin(), s.samples.end(), output->samples);
      times->download += elapsed(t);
    }
    return true;
  });
}
bool close_cpu(void *token, Error *e, Times *times) {
  auto t = Clock::now();
  bool ok = guard(e, "session_close", [&] {
    delete static_cast<Session *>(token);
    return true;
  });
  times->close = elapsed(t);
  return ok;
}
const Api api = {1, sizeof(Api), create, evaluate_cpu, render_cpu, close_cpu};
const Api api_v2 = {2, sizeof(Api), create_v2, evaluate_cpu, render_cpu, close_cpu};
} // namespace
const Api *cpu_api() { return &api; }
const Api *cpu_api_v2() { return &api_v2; }
} // namespace bgom10
