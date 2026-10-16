#ifndef BG_OM10_SCENE_HPP
#define BG_OM10_SCENE_HPP
#include "validation.hpp"
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
namespace bgom10 {
struct Bytes {
  std::vector<uint8_t> bytes;
  size_t at = 0;
  explicit Bytes(const std::string &path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
      throw Failure("decode: cannot read input");
    auto n = f.tellg();
    if (n < 0 || static_cast<uint64_t>(n) > SIZE_MAX ||
        static_cast<uint64_t>(n) >
            static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()))
      throw Failure("decode: file size overflow");
    bytes.resize(static_cast<size_t>(n));
    f.seekg(0);
    if (!bytes.empty() &&
        !f.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
      throw Failure("decode: short input");
  }
  uint64_t integer(unsigned n) {
    if (n > bytes.size() - at)
      throw Failure("decode: truncated record");
    uint64_t x = 0;
    for (unsigned i = 0; i < n; ++i)
      x |= uint64_t(bytes[at++]) << (8 * i);
    return x;
  }
  uint32_t u32() { return static_cast<uint32_t>(integer(4)); }
  uint64_t u64() { return integer(8); }
  double number() {
    uint64_t x = u64();
    double d;
    static_assert(sizeof(d) == 8, "binary64");
    std::memcpy(&d, &x, 8);
    if (!finite(d))
      throw Failure("decode: nonfinite input");
    return d;
  }
  void numbers(double *p, unsigned n) {
    for (unsigned i = 0; i < n; ++i)
      p[i] = number();
  }
  void magic(const char *text) {
    if (bytes.size() - at < 8 || std::memcmp(bytes.data() + at, text, 8))
      throw Failure("decode: wrong magic");
    at += 8;
  }
  void done() {
    if (at != bytes.size())
      throw Failure("decode: trailing bytes");
  }
};
struct Scene {
  uint32_t source_profile = 0;
  std::vector<Definition> definitions;
  std::vector<Instance> instances;
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
  std::vector<Carrier> carriers;
  std::vector<Material> materials;
  std::vector<Control> controls;
  std::vector<VertexRef> vertex_refs;
  std::vector<PrimitiveRef> primitive_refs;
  SceneView view() const {
    return {definitions.data(),    definitions.size(),   instances.data(),   instances.size(),
            vertices.data(),       vertices.size(),      triangles.data(),   triangles.size(),
            carriers.data(),       carriers.size(),      materials.data(),   materials.size(),
            controls.data(),       controls.size(),      vertex_refs.data(), vertex_refs.size(),
            primitive_refs.data(), primitive_refs.size()};
  }
  explicit Scene(const std::string &path) {
    Bytes b(path);
    b.magic("BGMESH01");
    require(b.u32() == 1, "decode: unsupported scene version");
    source_profile = b.u32();
    require((source_profile & ~SOURCE_GEOMETRY_PROFILE) == 0, "decode: unsupported scene flags");
    uint64_t n[7];
    for (auto &x : n)
      x = b.u64();
    require(b.u64() == 0, "decode: scene reserved field");
    uint64_t total = 80;
    const uint64_t sizes[7] = {40, 136, 56, 24, 192, 32, 32};
    for (int i = 0; i < 7; ++i) {
      require(n[i] <= UINT32_MAX, "decode: scene count exceeds native slot range");
      total = checked_add(total, checked_mul(n[i], sizes[i]));
    }
    require(total == b.bytes.size(), "decode: scene section sizes do not match file");
    definitions.resize(n[0]);
    instances.resize(n[1]);
    vertices.resize(n[2]);
    triangles.resize(n[3]);
    carriers.resize(n[4]);
    materials.resize(n[5]);
    controls.resize(n[6]);
    for (auto &d : definitions)
      d = {b.u64(), b.u64(), b.u64(), b.u64(), b.u64()};
    for (auto &i : instances) {
      i.id = b.u64();
      i.definition = b.u32();
      i.reserved = b.u32();
      b.numbers(i.matrix, 12);
      b.numbers(i.explosion, 3);
    }
    for (auto &v : vertices) {
      b.numbers(v.xyz, 3);
      b.numbers(v.uv, 2);
      v.carrier = b.u32();
      v.flags = b.u32();
      v.physical = b.u64();
    }
    for (auto &t : triangles) {
      for (auto &v : t.vertex)
        v = b.u32();
      t.material = b.u32();
      t.face = b.u64();
    }
    for (auto &c : carriers) {
      c.id = b.u64();
      c.tag = b.u32();
      c.flags = b.u32();
      b.numbers(c.origin, 3);
      b.numbers(c.x, 3);
      b.numbers(c.y, 3);
      b.numbers(c.z, 3);
      c.r0 = b.number();
      c.r1 = b.number();
      b.numbers(c.uv, 4);
      c.control_offset = b.u64();
      c.control_count = b.u64();
      c.p = b.u32();
      c.q = b.u32();
      c.reserved = b.u64();
    }
    for (auto &m : materials) {
      b.numbers(m.rgb, 3);
      m.roughness = b.number();
    }
    for (auto &c : controls)
      b.numbers(c.h, 4);
    b.done();
    validate();
  }
  void validate() {
    validate_scene_records(view(), source_profile != 0);
    vertex_refs.clear();
    primitive_refs.clear();
    for (uint64_t i = 0; i < instances.size(); ++i) {
      const auto &instance = instances[static_cast<std::size_t>(i)];
      const auto &definition = definitions[instance.definition];
      require(checked_add(vertex_refs.size(), definition.vertex_count) <= UINT32_MAX &&
                  checked_add(primitive_refs.size(), definition.triangle_count) <= UINT32_MAX,
              "decode: placed slot count overflow");
      const auto base = static_cast<uint32_t>(vertex_refs.size());
      for (uint64_t v = 0; v < definition.vertex_count; ++v)
        vertex_refs.push_back(
            {static_cast<uint32_t>(definition.first_vertex + v), static_cast<uint32_t>(i)});
      for (uint64_t t = 0; t < definition.triangle_count; ++t)
        primitive_refs.push_back({static_cast<uint32_t>(definition.first_triangle + t),
                                  static_cast<uint32_t>(i), base, 0});
    }
    validate_scene_placements(view());
  }
};
inline std::vector<Request> load_requests(const std::string &path) {
  Bytes b(path);
  b.magic("BGVIEW01");
  require(b.u32() == 1, "decode: view version");
  uint32_t n = b.u32();
  require(checked_add(16, checked_mul(n, 192)) == b.bytes.size(), "decode: view section length");
  std::vector<Request> out(n);
  for (auto &r : out) {
    r.width = b.u32();
    r.height = b.u32();
    r.supersample = b.u32();
    r.reserved = b.u32();
    r.alpha = b.number();
    b.numbers(r.basis, 9);
    b.numbers(r.center, 3);
    b.numbers(r.pixel_center, 2);
    r.scale = b.number();
    b.numbers(r.background, 3);
    r.flags = b.u32();
    r.reserved2 = b.u32();
    b.numbers(r.reserved3, 2);
    validate_request(r);
  }
  b.done();
  return out;
}
} // namespace bgom10
#endif
