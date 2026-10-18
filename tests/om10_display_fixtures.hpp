// Authored fixtures shared by the CPU and CUDA display bin tests: bin families
// of prepared triangles, the sequential bin model, and one display scene.
#ifndef BG_OM10_DISPLAY_FIXTURES_HPP
#define BG_OM10_DISPLAY_FIXTURES_HPP
#include "../native/om10/types.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace om10_fixtures {
using namespace bgom10;

inline void require_test(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
struct Lcg {
  uint64_t state;
  uint32_t next(uint32_t bound) {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<uint32_t>((state >> 33) % bound);
  }
};
inline uint64_t fnv(const void *data, size_t bytes) {
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < bytes; ++i)
    h = (h ^ static_cast<const uint8_t *>(data)[i]) * 1099511628211ull;
  return h;
}

struct BinFamily {
  std::string name;
  uint32_t tiles_x = 0, tiles_y = 0;
  std::vector<Prepared> triangles;
  // Appends a primitive covering tiles [tx0, tx1] x [ty0, ty1]; area 0 is skipped.
  void add(uint32_t tx0, uint32_t ty0, uint32_t tx1, uint32_t ty1, bool empty, uint32_t jitter) {
    Prepared p{};
    p.primitive = triangles.size();
    p.area = empty ? 0 : 1;
    p.xmin = int32_t(tx0 * 16 + jitter % 8);
    p.xmax = int32_t(tx1 * 16 + 8 + jitter % 8);
    p.ymin = int32_t(ty0 * 16 + jitter % 5);
    p.ymax = int32_t(ty1 * 16 + 8 + jitter % 7);
    triangles.push_back(p);
  }
};

inline std::vector<BinFamily> bin_families() {
  std::vector<BinFamily> out;
  // No primitives, and three primitives: exercise the CPU chunk clamp
  // (chunks <= primitives) and the CUDA empty and tiny paths.
  out.push_back(BinFamily{"tiny_empty", 4, 4, {}});
  {
    BinFamily f{"tiny_three", 4, 4, {}};
    f.add(0, 0, 3, 3, false, 0);
    f.add(1, 1, 1, 1, false, 1);
    f.add(0, 2, 2, 3, false, 2);
    out.push_back(std::move(f));
  }
  {
    // Skewed work: the first 5% of primitives cover 16x16 tiles each, the rest
    // one tile, so equal-count index ranges carry very unequal reference counts.
    BinFamily f{"skewed", 128, 128, {}};
    Lcg g{5};
    for (uint32_t i = 0; i < 200000; ++i) {
      const uint32_t x = g.next(112), y = g.next(112);
      if (i < 10000)
        f.add(x, y, x + 15, y + 15, false, i);
      else
        f.add(x, y, x, y, false, i);
    }
    out.push_back(std::move(f));
  }
  {
    // High contention: many primitives over a 4x4 tile corner, from many blocks.
    BinFamily f{"dense_corner", 64, 64, {}};
    Lcg g{1};
    for (uint32_t i = 0; i < 131072; ++i) {
      // Each draw is its own declarator, so the sequence is fixed by the language.
      const uint32_t x = g.next(4), y = g.next(4), w = g.next(2), h = g.next(2);
      f.add(x, y, std::min(3u, x + w), std::min(3u, y + h), false, i);
    }
    out.push_back(std::move(f));
  }
  {
    // Mixed rectangle sizes, some zero-area primitives.
    BinFamily f{"mixed_random", 128, 128, {}};
    Lcg g{2};
    for (uint32_t i = 0; i < 100000; ++i) {
      uint32_t w = 1 + g.next(g.next(8) ? 3 : 24), h = 1 + g.next(g.next(8) ? 3 : 24);
      uint32_t x = g.next(128), y = g.next(128);
      f.add(x, y, std::min(127u, x + w - 1), std::min(127u, y + h - 1), g.next(20) == 0, i);
    }
    out.push_back(std::move(f));
  }
  {
    // Bin lengths at every sort-kernel boundary (0, 1, 2, 32, 33, 256, 257,
    // 4096, 4097) and 12000, interleaved in index order. 3000 primitives cover
    // tiles [0,11] x [0,11]; single-tile runs bring tile (3,3) to 12000, (5,5)
    // to 4096 and (6,6) to 4097; rows 12-15 hold the short lists, the rest of
    // those rows stay empty.
    BinFamily f{"long_bins", 16, 16, {}};
    struct Run {
      uint32_t tile, count;
    } runs[] = {{3 * 16 + 3, 9000}, {5 * 16 + 5, 4096 - 3000}, {6 * 16 + 6, 4097 - 3000},
                {13 * 16 + 13, 1}, {14 * 16 + 14, 2},       {12 * 16 + 1, 32},
                {12 * 16 + 2, 33},  {12 * 16 + 3, 256},     {12 * 16 + 4, 257}};
    Lcg g{3};
    std::vector<int> order;
    for (int r = 0; r < 9; ++r)
      for (uint32_t k = 0; k < runs[r].count; ++k)
        order.push_back(r);
    for (int k = 0; k < 3000; ++k)
      order.push_back(-1);
    for (size_t i = order.size(); i > 1; --i)
      std::swap(order[i - 1], order[g.next(uint32_t(i))]);
    for (size_t i = 0; i < order.size(); ++i) {
      if (order[i] < 0)
        f.add(0, 0, 11, 11, false, uint32_t(i));
      else {
        uint32_t t = runs[order[i]].tile;
        f.add(t % 16, t / 16, t % 16, t / 16, false, uint32_t(i));
      }
    }
    out.push_back(std::move(f));
  }
  {
    // Largest admitted sample grid (8192 x 8192): 262144 tiles, so the scan's
    // second level is full; three primitives cover every tile.
    BinFamily f{"max_grid", 512, 512, {}};
    Lcg g{4};
    for (uint32_t i = 0; i < 20000; ++i) {
      if (i % 6667 == 17)
        f.add(0, 0, 511, 511, false, i);
      else {
        const uint32_t x = g.next(512), y = g.next(512), w = g.next(6), h = g.next(6);
        f.add(x, y, std::min(511u, x + w), std::min(511u, y + h), false, i);
      }
    }
    out.push_back(std::move(f));
  }
  return out;
}

inline void model_bins(const BinFamily &f, std::vector<uint64_t> &offsets, std::vector<uint32_t> &bins) {
  const uint64_t ntiles = uint64_t(f.tiles_x) * f.tiles_y;
  std::vector<std::vector<uint32_t>> lists(ntiles);
  for (uint64_t i = 0; i < f.triangles.size(); ++i) {
    const Prepared &p = f.triangles[i];
    if (!p.area)
      continue;
    for (int y = p.ymin / 16; y <= p.ymax / 16; ++y)
      for (int x = p.xmin / 16; x <= p.xmax / 16; ++x)
        lists[uint64_t(y) * f.tiles_x + x].push_back(uint32_t(i));
  }
  offsets.assign(1, 0);
  bins.clear();
  for (const auto &list : lists) {
    bins.insert(bins.end(), list.begin(), list.end());
    offsets.push_back(bins.size());
  }
}

// Authored display scene: one plane definition (a 32x32 quad grid plus eight
// covering triangles at the same depth), 48 rigid placements with repeated
// depths, so samples see many equal-depth candidates per tile.
struct DisplayScene {
  Carrier carrier{};
  Definition definition{};
  std::vector<Instance> instances;
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
  std::vector<Material> materials;
  std::vector<VertexRef> vertex_refs;
  std::vector<PrimitiveRef> primitive_refs;
  SceneView view() const {
    return {&definition,        1,
            instances.data(),   instances.size(),
            vertices.data(),    vertices.size(),
            triangles.data(),   triangles.size(),
            &carrier,           1,
            materials.data(),   materials.size(),
            nullptr,            0,
            vertex_refs.data(), vertex_refs.size(),
            primitive_refs.data(), primitive_refs.size()};
  }
};

inline DisplayScene display_scene() {
  DisplayScene d;
  d.carrier.id = 1;
  d.carrier.tag = 1;
  d.carrier.x[0] = d.carrier.y[1] = d.carrier.z[2] = 1;
  d.carrier.uv[0] = d.carrier.uv[2] = -1;
  d.carrier.uv[1] = d.carrier.uv[3] = 1;
  const uint32_t g = 32;
  auto vertex = [&](double u, double v) {
    Vertex x{};
    x.xyz[0] = x.uv[0] = u;
    x.xyz[1] = x.uv[1] = v;
    x.flags = STORED_XYZ | COFACTOR_NORMAL;
    x.physical = d.vertices.size() + 1;
    d.vertices.push_back(x);
    return uint32_t(d.vertices.size() - 1);
  };
  for (uint32_t j = 0; j <= g; ++j)
    for (uint32_t i = 0; i <= g; ++i)
      vertex(-1 + 2. * i / g, -1 + 2. * j / g);
  uint64_t face = 1;
  for (uint32_t j = 0; j < g; ++j)
    for (uint32_t i = 0; i < g; ++i) {
      uint32_t a = j * (g + 1) + i, b = a + 1, c = a + g + 1, e = c + 1;
      d.triangles.push_back({{a, b, e}, INHERIT_OCCURRENCE_MATERIAL, face});
      d.triangles.push_back({{a, e, c}, (i + j) % 3, face++});
    }
  for (uint32_t k = 0; k < 8; ++k) {
    double s = .25 * k / 8;
    uint32_t a = vertex(-1 + s, -1), b = vertex(1, -1 + s), c = vertex(-1 + s, 1);
    d.triangles.push_back({{a, b, c}, k % 3, face++});
  }
  d.definition = {1, 0, d.vertices.size(), 0, d.triangles.size()};
  d.materials = {{{.8, .1, .1}, .2}, {{.1, .7, .2}, .5}, {{.1, .2, .8}, .9}};
  for (uint32_t k = 0; k < 48; ++k) {
    Instance in{};
    in.id = 100 + k;
    in.definition = 0;
    in.reserved = 1 + k % 3;
    in.matrix[0] = in.matrix[5] = in.matrix[10] = 1;
    in.matrix[3] = -.35 + .1 * (k % 8);
    in.matrix[7] = -.25 + .1 * (k / 8);
    in.matrix[11] = (k % 3) ? 0 : .125 * (k % 4);
    in.explosion[0] = .01 * k;
    d.instances.push_back(in);
    const uint32_t base = uint32_t(d.vertex_refs.size());
    for (uint32_t v = 0; v < d.vertices.size(); ++v)
      d.vertex_refs.push_back({v, k});
    for (uint32_t t = 0; t < d.triangles.size(); ++t)
      d.primitive_refs.push_back({t, k, base, 0});
  }
  return d;
}

struct Frame {
  std::vector<uint8_t> rgba;
  std::vector<double> depth;
  std::vector<uint64_t> primitive, instance;
  std::vector<Sample> samples;
  uint64_t digest() const {
    return fnv(rgba.data(), rgba.size()) ^ fnv(depth.data(), depth.size() * sizeof(double)) * 3 ^
           fnv(primitive.data(), primitive.size() * 8) * 5 ^
           fnv(instance.data(), instance.size() * 8) * 7 ^
           fnv(samples.data(), samples.size() * sizeof(Sample)) * 11;
  }
  bool operator==(const Frame &o) const {
    auto same = [](const auto &a, const auto &b) {
      return a.size() == b.size() &&
             std::memcmp(a.data(), b.data(), a.size() * sizeof(a[0])) == 0;
    };
    return same(rgba, o.rgba) && same(depth, o.depth) && same(primitive, o.primitive) &&
           same(instance, o.instance) && same(samples, o.samples);
  }
};

// An empty scene: one definition with no geometry and no placements. Every
// sample must be background.
struct EmptyScene {
  Definition definition{1, 0, 0, 0, 0};
  SceneView view() const {
    SceneView v{};
    v.definitions = &definition;
    v.ndefinitions = 1;
    return v;
  }
};
// sRGB transfer (IEC 61966-2-1) of one linear channel to an 8-bit code,
// written from the standard rather than from the display's encoder.
inline uint8_t srgb_byte(double linear) {
  const double v = linear <= .0031308 ? 12.92 * linear : 1.055 * std::pow(linear, 1 / 2.4) - .055;
  return static_cast<uint8_t>(std::floor(255 * v + .5));
}
inline Request display_request() {
  Request r{};
  r.width = r.height = 512;
  r.supersample = 2;
  r.alpha = .5;
  r.basis[0] = r.basis[4] = r.basis[8] = 1;
  r.pixel_center[0] = r.pixel_center[1] = 256;
  r.scale = 180;
  r.background[0] = r.background[1] = r.background[2] = .01;
  r.flags = DUMP_SAMPLES;
  return r;
}

// One fresh session: create, evaluate, render, close.
template <class Scene>
Frame render_frame(const Api *api, const Scene &d, const Request &r, uint32_t threads,
                   int32_t device) {
  const SceneView view = d.view();
  const uint64_t npixels = uint64_t(r.width) * r.height,
                 nsamples = npixels * r.supersample * r.supersample;
  Frame f{std::vector<uint8_t>(npixels * 4), std::vector<double>(npixels),
          std::vector<uint64_t>(npixels), std::vector<uint64_t>(npixels),
          std::vector<Sample>(nsamples)};
  FrameOutput out{f.rgba.data(), f.depth.data(), f.primitive.data(), f.instance.data(), nullptr,
                  f.samples.data()};
  Error e{};
  Times t{};
  void *token = nullptr;
  require_test(api->create(&view, threads, device, &token, &e, &t) && token,
               std::string("display create: ") + e.message);
  const bool ok = api->evaluate(token, nullptr, &e, &t) && api->render(token, &r, &out, &e, &t);
  Error closing{};
  const bool closed = api->close(token, &closing, &t);
  require_test(ok, std::string("display render: ") + e.message);
  require_test(closed, std::string("display close: ") + closing.message);
  return f;
}
// Checks an empty-scene frame against the background specification.
inline void check_empty_frame(const Frame &f, const Request &r) {
  for (uint64_t i = 0; i < f.primitive.size(); ++i) {
    require_test(f.primitive[i] == UINT64_MAX && f.instance[i] == UINT64_MAX &&
                     f.depth[i] == -INFINITY,
                 "empty scene: covered pixel");
    for (int c = 0; c < 3; ++c)
      require_test(f.rgba[4 * i + c] == srgb_byte(r.background[c]), "empty scene: background");
    require_test(f.rgba[4 * i + 3] == 255, "empty scene: alpha");
  }
  for (const Sample &x : f.samples)
    require_test(x.depth == -INFINITY && x.primitive == UINT64_MAX && x.instance == UINT64_MAX,
                 "empty scene: covered sample");
}
} // namespace om10_fixtures
#endif
