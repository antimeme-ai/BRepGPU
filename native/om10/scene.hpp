#ifndef BG_OM10_SCENE_HPP
#define BG_OM10_SCENE_HPP
#include "math.hpp"
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace bgom10 {
struct Failure : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct Bytes {
  std::vector<uint8_t> bytes;
  size_t at = 0;
  explicit Bytes(const std::string &path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
      throw Failure("decode: cannot read input");
    auto n = f.tellg();
    if (n < 0 || static_cast<uint64_t>(n) > SIZE_MAX)
      throw Failure("decode: file size overflow");
    bytes.resize(static_cast<size_t>(n));
    f.seekg(0);
    if (!bytes.empty() &&
        !f.read(reinterpret_cast<char *>(bytes.data()), bytes.size()))
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
inline uint64_t checked_add(uint64_t a, uint64_t b) {
  if (a > UINT64_MAX - b)
    throw Failure("decode: size addition overflow");
  return a + b;
}
inline uint64_t checked_mul(uint64_t a, uint64_t b) {
  if (b && a > UINT64_MAX / b)
    throw Failure("decode: size product overflow");
  return a * b;
}
inline void require(bool value, const char *message) {
  if (!value)
    throw Failure(message);
}
inline bool orthogonal(const double *x, const double *y, const double *z,
                       bool right) {
  Vec a = vec(x), b = vec(y), c = vec(z);
  double tol = 2e-10;
  return fabs(dot(a, a) - 1) < tol && fabs(dot(b, b) - 1) < tol &&
         fabs(dot(c, c) - 1) < tol && fabs(dot(a, b)) < tol &&
         fabs(dot(a, c)) < tol && fabs(dot(b, c)) < tol &&
         (right ? fabs(dot(cross(a, b), c) - 1) < tol
                : fabs(fabs(dot(cross(a, b), c)) - 1) < tol);
}
inline bool independent_axes(const double *x, const double *y, const double *z, bool positive) {
  Vec a=vec(x), b=vec(y), c=vec(z);
  const double scale=maxd(maximum_component(a),maxd(maximum_component(b),maximum_component(c)));
  if(!finite(scale)||scale==0)return false;
  a=divided(a,scale);b=divided(b,scale);c=divided(c,scale);
  const double determinant=dot(cross(a,b),c);
  // At most six triple products of normalized components. This conservative
  // absolute guard refuses uncertain signs, including underflow; it does not
  // treat a rounded zero as proof of source degeneracy.
  return finite(determinant) && (positive ? determinant>64*DBL_EPSILON
                                         : fabs(determinant)>64*DBL_EPSILON);
}
inline bool independent_plane_axes(const double *x,const double *y) {
  const Vec a=vec(x),b=vec(y);
  const double scale=maxd(maximum_component(a),maximum_component(b));
  if(!finite(scale)||scale==0)return false;
  const Vec normal=cross(divided(a,scale),divided(b,scale));
  return good(normal)&&maximum_component(normal)>16*DBL_EPSILON;
}
struct Scene {
  uint32_t source_profile=0;
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
    return {definitions.data(), definitions.size(),    instances.data(),
            instances.size(),   vertices.data(),       vertices.size(),
            triangles.data(),   triangles.size(),      carriers.data(),
            carriers.size(),    materials.data(),      materials.size(),
            controls.data(),    controls.size(),       vertex_refs.data(),
            vertex_refs.size(), primitive_refs.data(), primitive_refs.size()};
  }
  explicit Scene(const std::string &path) {
    Bytes b(path);
    b.magic("BGMESH01");
    require(b.u32() == 1,"decode: unsupported scene version");
    source_profile=b.u32();
    require((source_profile & ~SOURCE_GEOMETRY_PROFILE)==0,
            "decode: unsupported scene flags");
    uint64_t n[7];
    for (auto &x : n)
      x = b.u64();
    require(b.u64() == 0, "decode: scene reserved field");
    uint64_t total = 80;
    const uint64_t sizes[7] = {40, 136, 56, 24, 192, 32, 32};
    for (int i = 0; i < 7; ++i) {
      require(n[i] <= UINT32_MAX,
              "decode: scene count exceeds native slot range");
      total = checked_add(total, checked_mul(n[i], sizes[i]));
    }
    require(total == b.bytes.size(),
            "decode: scene section sizes do not match file");
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
    uint64_t vo = 0, to = 0;
    std::set<uint64_t> dids, iids;
    for (const auto &d : definitions) {
      require(dids.insert(d.id).second, "decode: duplicate definition ID");
      require(d.first_vertex == vo && d.first_triangle == to,
              "decode: nonpartitioned definition ranges");
      vo = checked_add(vo, d.vertex_count);
      to = checked_add(to, d.triangle_count);
      require(vo <= vertices.size() && to <= triangles.size(),
              "decode: definition range");
      for (uint64_t j = d.first_triangle; j < to; ++j) {
        const Triangle &t = triangles[j];
        for (auto k : t.vertex)
          require(k < d.vertex_count, "decode: triangle local vertex index");
        require(t.vertex[0] != t.vertex[1] && t.vertex[1] != t.vertex[2] &&
                    t.vertex[0] != t.vertex[2],
                "decode: repeated triangle vertex");
        require(t.material < materials.size(),
                "decode: triangle material index");
        Vec a = vec(vertices[d.first_vertex + t.vertex[0]].xyz),
            bb = vec(vertices[d.first_vertex + t.vertex[1]].xyz),
            c = vec(vertices[d.first_vertex + t.vertex[2]].xyz), normal;
        require(unit(cross(sub(bb, a), sub(c, a)), normal),
                "decode: collapsed/nonfinite canonical triangle");
      }
    }
    require(vo == vertices.size() && to == triangles.size(),
            "decode: unowned geometry records");
    for (const auto &i : instances) {
      require(i.id != UINT64_MAX && iids.insert(i.id).second,
              "decode: duplicate/reserved instance ID");
      require(i.definition < definitions.size() && i.reserved == 0,
              "decode: instance definition/reserved");
      require(source_profile ? independent_axes(i.matrix,i.matrix+4,i.matrix+8,true)
                             : orthogonal(i.matrix, i.matrix + 4, i.matrix + 8, true),
              "decode: nonrigid instance matrix");
      const Definition &d = definitions[i.definition];
      require(checked_add(vertex_refs.size(), d.vertex_count) <= UINT32_MAX &&
                  checked_add(primitive_refs.size(), d.triangle_count) <=
                      UINT32_MAX,
              "decode: placed slot count overflow");
      uint32_t base = vertex_refs.size(), slot = &i - instances.data();
      for (uint64_t v = 0; v < d.vertex_count; ++v)
        vertex_refs.push_back(
            {static_cast<uint32_t>(d.first_vertex + v), slot});
      for (uint64_t t = 0; t < d.triangle_count; ++t)
        primitive_refs.push_back(
            {static_cast<uint32_t>(d.first_triangle + t), slot, base, 0});
    }
    for (const auto &c : carriers) {
      require(c.tag >= 1 && c.tag <= 6 && c.flags == 0 && c.reserved == 0,
              "decode: carrier kind/flags");
      require(c.uv[0] <= c.uv[1] && c.uv[2] <= c.uv[3],
              "decode: reversed carrier UV bounds");
      if (c.tag == 6) {
        require(c.p <= 3 && c.q <= 3 &&
                    c.control_count == (c.p + 1) * (c.q + 1),
                "decode: unsupported Bezier degree/control count");
        require(c.control_offset <= controls.size() &&
                    c.control_count <= controls.size() - c.control_offset,
                "decode: Bezier controls range");
        require(c.uv[0] < c.uv[1] && c.uv[2] < c.uv[3],
                "decode: collapsed Bezier span");
        require(c.r0 == 0 && c.r1 == 0, "decode: unused Bezier radius");
        for (int j = 0; j < 3; ++j)
          require(c.origin[j] == 0 && c.x[j] == 0 && c.y[j] == 0 && c.z[j] == 0,
                  "decode: unused Bezier frame");
      } else {
        require(c.control_count == 0 && c.control_offset == 0 && c.p == 0 &&
                    c.q == 0,
                "decode: analytic control fields");
        require(source_profile ? (c.tag==1?independent_plane_axes(c.x,c.y):independent_axes(c.x,c.y,c.z,false))
                               : orthogonal(c.x, c.y, c.z, false),
                "decode: invalid analytic axes");
        if (c.tag == 1)
          require(c.r0 == 0 && c.r1 == 0, "decode: unused plane radius");
        else if (c.tag == 3)
          require(c.r0 >= 0, "decode: negative cone reference radius");
        else
          require(c.r0 > 0, "decode: nonpositive analytic radius");
        if (c.tag == 2 || c.tag == 4)
          require(c.r1 == 0, "decode: unused analytic radius");
        if (c.tag == 3)
          require(c.r1 != 0 && fabs(c.r1) < 1.5707963267948966,
                  "decode: invalid cone angle");
        if (c.tag == 5)
          require(c.r1 > 0, "decode: invalid torus minor radius");
      }
    }
    for (const auto &c : controls)
      require(c.h[3] > 0, "decode: nonpositive homogeneous weight");
    for (const auto &v : vertices) {
      require(v.carrier < carriers.size(), "decode: vertex carrier");
      require(source_profile ? source_vertex_flags(carriers[v.carrier],v.flags) : (v.flags & ~3u) == 0,
              "decode: vertex carrier/flags");
      const Carrier &c = carriers[v.carrier];
      require(v.uv[0] >= c.uv[0] && v.uv[0] <= c.uv[1] && v.uv[1] >= c.uv[2] &&
                  v.uv[1] <= c.uv[3],
              "decode: source UV outside carrier domain");
    }
    for (const auto &m : materials) {
      for (auto c : m.rgb)
        require(c >= 0 && c <= 1, "decode: material color range");
      require(m.roughness > 0 && m.roughness <= 1,
              "decode: material roughness range");
    }
  }
};
inline std::vector<Request> load_requests(const std::string &path) {
  Bytes b(path);
  b.magic("BGVIEW01");
  require(b.u32() == 1, "decode: view version");
  uint32_t n = b.u32();
  require(checked_add(16, checked_mul(n, 192)) == b.bytes.size(),
          "decode: view section length");
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
    require(r.width && r.height && r.width <= 8192 && r.height <= 8192 &&
                (r.supersample == 1 || r.supersample == 2) &&
                r.width * r.supersample <= 8192 &&
                r.height * r.supersample <= 8192,
            "decode: view dimensions");
    require(r.reserved == 0 && r.reserved2 == 0 && r.reserved3[0] == 0 &&
                r.reserved3[1] == 0 && (r.flags & ~7u) == 0,
            "decode: view flags/reserved");
    require(r.alpha >= 0 && r.alpha <= 1 && r.scale > 0,
            "decode: alpha/scale range");
    require(orthogonal(r.basis, r.basis + 3, r.basis + 6, true),
            "decode: invalid camera basis");
    for (double c : r.background)
      require(c >= 0 && c <= 1, "decode: background range");
  }
  b.done();
  return out;
}
} // namespace bgom10
#endif
