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
  std::function<void(uint64_t)> job;
  void drain() {
    for (;;) {
      uint64_t begin = next.fetch_add(32);
      if (begin >= count)
        break;
      for (uint64_t i = begin; i < std::min(begin + 32, count); ++i)
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
  void parallel(uint64_t n, std::function<void(uint64_t)> f) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      job = std::move(f);
      count = n;
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
  std::vector<Sample> samples;
  bool evaluated = false, poisoned = false;
  Session(SceneView s, uint32_t n) : scene(s), pool(n), jets(s.nvertices) {}
};
void first_error(std::atomic<uint64_t> &out, uint64_t i) {
  uint64_t old = out.load();
  while (i < old && !out.compare_exchange_weak(old, i)) {
  }
}
bool create(const SceneView *scene, uint32_t threads, int32_t, void **out,
            Error *e, Times *times) {
  *out = nullptr;
  auto t = Clock::now();
  bool ok = guard(e, "session_create", [&] {
    if (!scene || threads < 1 || threads > 256) {
      error(e, "session_create", "invalid CPU scene/thread count");
      return false;
    }
    *out = new Session(*scene, threads);
    return true;
  });
  times->create = elapsed(t);
  return ok;
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
      if (!evaluate(s.scene.carriers[v.carrier], s.scene.controls, v,
                    s.jets[i])) {
        s.jets[i].status = 1;
        first_error(bad, i);
      }
    });
    times->evaluate = elapsed(t);
    if (bad != UINT64_MAX) {
      s.poisoned = true;
      error(e, "source_evaluate", "nonfinite or singular source first jet",
            bad);
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
bool render_cpu(void *token, const Request *request, FrameOutput *output,
                Error *e, Times *times) {
  return guard(e, "render", [&] {
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
      error(e, "instance_transform_project",
            "nonfinite or out-of-range projection", bad);
      return false;
    }
    if (output->projected)
      std::copy(s.points.begin(), s.points.end(), output->projected);
    if (r.flags & PROJECT_ONLY)
      return true;
    uint32_t width = r.width * r.supersample, height = r.height * r.supersample,
             tiles_x = (width + 15) / 16, tiles_y = (height + 15) / 16;
    uint64_t ntiles = uint64_t(tiles_x) * tiles_y,
             npixels = uint64_t(width) * height;
    t = Clock::now();
    s.triangles.resize(s.scene.nprimitives);
    s.pool.parallel(s.scene.nprimitives, [&](uint64_t i) {
      setup(s.scene, r, i, s.points.data(), s.triangles[i]);
    });
    times->setup = elapsed(t);
    t = Clock::now();
    std::unique_ptr<std::atomic<uint32_t>[]> counts(
        new std::atomic<uint32_t>[ntiles]);
    for (uint64_t i = 0; i < ntiles; ++i)
      counts[i] = 0;
    s.pool.parallel(s.triangles.size(), [&](uint64_t i) {
      const auto &p = s.triangles[i];
      if (!p.area)
        return;
      for (int32_t y = p.ymin / 16; y <= p.ymax / 16; ++y)
        for (int32_t x = p.xmin / 16; x <= p.xmax / 16; ++x)
          counts[uint64_t(y) * tiles_x + x].fetch_add(
              1, std::memory_order_relaxed);
    });
    s.offsets.resize(ntiles + 1);
    s.offsets[0] = 0;
    for (uint64_t i = 0; i < ntiles; ++i) {
      s.offsets[i + 1] = checked_add(s.offsets[i], counts[i].load());
      counts[i] = 0;
    }
    require(s.offsets.back() <= SIZE_MAX / sizeof(uint32_t),
            "tile bins size overflow");
    s.bins.resize(s.offsets.back());
    s.pool.parallel(s.triangles.size(), [&](uint64_t i) {
      const auto &p = s.triangles[i];
      if (!p.area)
        return;
      for (int32_t y = p.ymin / 16; y <= p.ymax / 16; ++y)
        for (int32_t x = p.xmin / 16; x <= p.xmax / 16; ++x) {
          uint64_t tile = uint64_t(y) * tiles_x + x;
          s.bins[s.offsets[tile] +
                 counts[tile].fetch_add(1, std::memory_order_relaxed)] = i;
        }
    });
    times->bins = elapsed(t);
    t = Clock::now();
    s.samples.resize(npixels);
    s.pool.parallel(ntiles, [&](uint64_t tile) {
      uint32_t tx = tile % tiles_x, ty = tile / tiles_x, x0 = tx * 16,
               y0 = ty * 16, x1 = std::min(width, x0 + 16),
               y1 = std::min(height, y0 + 16);
      for (uint32_t y = y0; y < y1; ++y)
        for (uint32_t x = x0; x < x1; ++x)
          s.samples[uint64_t(y) * width + x] = {-INFINITY, UINT64_MAX,
                                                UINT64_MAX};
      for (uint64_t k = s.offsets[tile]; k < s.offsets[tile + 1]; ++k) {
        const Prepared &p = s.triangles[s.bins[k]];
        for (int32_t y = std::max<int32_t>(y0, p.ymin);
             y < std::min<int32_t>(y1, p.ymax + 1); ++y)
          for (int32_t x = std::max<int32_t>(x0, p.xmin);
               x < std::min<int32_t>(x1, p.xmax + 1); ++x) {
            double w[3], z;
            if (!covered(p, x, y, w, z))
              continue;
            if (!finite(z)) {
              first_error(bad, uint64_t(y) * width + x);
              continue;
            }
            Sample &sample = s.samples[uint64_t(y) * width + x];
            if (z > sample.depth ||
                (z == sample.depth && p.primitive < sample.primitive))
              sample = {z, p.primitive, p.instance};
          }
      }
    });
    s.pool.parallel(uint64_t(r.width) * r.height, [&](uint64_t index) {
      uint32_t x = index % r.width, y = index / r.width;
      Vec color{};
      const Sample &chosen =
          s.samples[uint64_t(y * r.supersample) * width + x * r.supersample];
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
      error(e, "visibility_shading_resolve",
            "nonfinite depth or singular interpolated normal", bad);
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
} // namespace
const Api *cpu_api() { return &api; }
} // namespace bgom10
