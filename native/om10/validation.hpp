#ifndef BG_OM10_VALIDATION_HPP
#define BG_OM10_VALIDATION_HPP
#include "vector_math.hpp"
#include <algorithm>
#include <memory_resource>
#include <stdexcept>
#include <vector>
namespace bgom10 {
struct Failure : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct ValidationContext {
  std::pmr::memory_resource *resource = std::pmr::get_default_resource();
  void *state = nullptr;
  void (*work)(void *, uint64_t) = nullptr;
  void charge(uint64_t count) const {
    if (work)
      work(state, count);
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
inline bool orthogonal(const double *x, const double *y, const double *z, bool right) {
  Vec a = vec(x), b = vec(y), c = vec(z);
  double tol = 2e-10;
  return fabs(dot(a, a) - 1) < tol && fabs(dot(b, b) - 1) < tol && fabs(dot(c, c) - 1) < tol &&
         fabs(dot(a, b)) < tol && fabs(dot(a, c)) < tol && fabs(dot(b, c)) < tol &&
         (right ? fabs(dot(cross(a, b), c) - 1) < tol : fabs(fabs(dot(cross(a, b), c)) - 1) < tol);
}
inline bool independent_axes(const double *x, const double *y, const double *z, bool positive) {
  Vec a = vec(x), b = vec(y), c = vec(z);
  const double scale = maxd(maximum_component(a), maxd(maximum_component(b), maximum_component(c)));
  if (!finite(scale) || scale == 0)
    return false;
  a = divided(a, scale);
  b = divided(b, scale);
  c = divided(c, scale);
  const double determinant = dot(cross(a, b), c);
  // At most six triple products of normalized components. This conservative
  // absolute guard refuses uncertain signs, including underflow; it does not
  // treat a rounded zero as proof of source degeneracy.
  return finite(determinant) &&
         (positive ? determinant > 64 * DBL_EPSILON : fabs(determinant) > 64 * DBL_EPSILON);
}
inline bool independent_plane_axes(const double *x, const double *y) {
  const Vec a = vec(x), b = vec(y);
  const double scale = maxd(maximum_component(a), maximum_component(b));
  if (!finite(scale) || scale == 0)
    return false;
  const Vec normal = cross(divided(a, scale), divided(b, scale));
  return good(normal) && maximum_component(normal) > 16 * DBL_EPSILON;
}

template <class T> struct BorrowedRecords {
  const T *pointer;
  uint64_t count;
  const T *begin() const { return pointer; }
  const T *end() const { return count ? pointer + count : pointer; }
  const T *data() const { return pointer; }
  uint64_t size() const { return count; }
  const T &operator[](uint64_t index) const { return pointer[index]; }
};
template <class T> BorrowedRecords<T> records(const T *pointer, uint64_t count) {
  require(count <= UINT32_MAX && count <= SIZE_MAX / sizeof(T),
          "decode: display slot count overflow");
  require(!count || pointer, "decode: missing display record array");
  return {pointer, count};
}
template <std::size_t N> void finite_fields(const double (&values)[N]) {
  for (double x : values)
    require(finite(x), "decode: nonfinite display scalar");
}
template <class T> void unique_ids(BorrowedRecords<T> values, ValidationContext context) {
  std::pmr::vector<uint64_t> ids(context.resource);
  ids.reserve(static_cast<std::size_t>(values.size()));
  for (const auto &value : values) {
    context.charge(1);
    ids.push_back(value.id);
  }
  std::sort(ids.begin(), ids.end(), [&](uint64_t a, uint64_t b) {
    context.charge(1);
    return a < b;
  });
  for (std::size_t i = 1; i < ids.size(); ++i) {
    context.charge(1);
    require(ids[i - 1] != ids[i], "decode: duplicate display identity");
  }
}
// Checks finite record shape and the existing display profile, not source
// geometry or mesher approximation. Borrows arrays; scratch holds IDs only.
inline void validate_scene_records(const SceneView &scene, bool source_profile,
                                   ValidationContext context = {}) {
  require(context.resource, "decode: missing validation scratch resource");
  const auto definitions = records(scene.definitions, scene.ndefinitions);
  const auto instances = records(scene.instances, scene.ninstances);
  const auto vertices = records(scene.vertices, scene.nvertices);
  const auto triangles = records(scene.triangles, scene.ntriangles);
  const auto carriers = records(scene.carriers, scene.ncarriers);
  const auto materials = records(scene.materials, scene.nmaterials);
  const auto controls = records(scene.controls, scene.ncontrols);
  unique_ids(definitions, context);
  unique_ids(instances, context);
  for (const auto &v : vertices) {
    context.charge(1);
    finite_fields(v.xyz);
    finite_fields(v.uv);
  }
  for (const auto &i : instances) {
    context.charge(1);
    finite_fields(i.matrix);
    finite_fields(i.explosion);
  }
  for (const auto &c : carriers) {
    context.charge(1);
    finite_fields(c.origin);
    finite_fields(c.x);
    finite_fields(c.y);
    finite_fields(c.z);
    finite_fields(c.uv);
    require(finite(c.r0) && finite(c.r1), "decode: nonfinite display radius");
  }
  for (const auto &c : controls) {
    context.charge(1);
    finite_fields(c.h);
  }
  for (const auto &m : materials) {
    context.charge(1);
    finite_fields(m.rgb);
    require(finite(m.roughness), "decode: nonfinite roughness");
  }

  uint64_t vo = 0, to = 0;
  for (const auto &d : definitions) {
    context.charge(1);
    require(d.first_vertex == vo && d.first_triangle == to,
            "decode: nonpartitioned definition ranges");
    vo = checked_add(vo, d.vertex_count);
    to = checked_add(to, d.triangle_count);
    require(vo <= vertices.size() && to <= triangles.size(), "decode: definition range");
    for (uint64_t j = d.first_triangle; j < to; ++j) {
      const Triangle &t = triangles[j];
      context.charge(1);
      for (auto k : t.vertex)
        require(k < d.vertex_count, "decode: triangle local vertex index");
      require(t.vertex[0] != t.vertex[1] && t.vertex[1] != t.vertex[2] &&
                  t.vertex[0] != t.vertex[2],
              "decode: repeated triangle vertex");
      require(t.material < materials.size() ||
                  (source_profile && t.material == INHERIT_OCCURRENCE_MATERIAL),
              "decode: triangle material index");
      Vec a = vec(vertices[d.first_vertex + t.vertex[0]].xyz),
          bb = vec(vertices[d.first_vertex + t.vertex[1]].xyz),
          c = vec(vertices[d.first_vertex + t.vertex[2]].xyz), normal;
      require(unit(cross(sub(bb, a), sub(c, a)), normal),
              "decode: collapsed/nonfinite canonical triangle");
    }
  }
  require(vo == vertices.size() && to == triangles.size(), "decode: unowned geometry records");
  for (const auto &i : instances) {
    context.charge(1);
    require(i.id != UINT64_MAX, "decode: reserved instance ID");
    require(i.definition < definitions.size() &&
                (source_profile ? i.reserved <= materials.size() : i.reserved == 0),
            "decode: instance definition/reserved");
    require(source_profile ? independent_axes(i.matrix, i.matrix + 4, i.matrix + 8, true)
                           : orthogonal(i.matrix, i.matrix + 4, i.matrix + 8, true),
            "decode: nonrigid instance matrix");
  }
  for (const auto &c : carriers) {
    context.charge(1);
    require(c.tag >= 1 && c.tag <= 6 && c.flags == 0 && c.reserved == 0,
            "decode: carrier kind/flags");
    require(c.uv[0] <= c.uv[1] && c.uv[2] <= c.uv[3], "decode: reversed carrier UV bounds");
    if (c.tag == 6) {
      require(c.p <= 3 && c.q <= 3 && c.control_count == (c.p + 1) * (c.q + 1),
              "decode: unsupported Bezier degree/control count");
      require(c.control_offset <= controls.size() &&
                  c.control_count <= controls.size() - c.control_offset,
              "decode: Bezier controls range");
      require(c.uv[0] < c.uv[1] && c.uv[2] < c.uv[3], "decode: collapsed Bezier span");
      require(c.r0 == 0 && c.r1 == 0, "decode: unused Bezier radius");
      for (int j = 0; j < 3; ++j)
        require(c.origin[j] == 0 && c.x[j] == 0 && c.y[j] == 0 && c.z[j] == 0,
                "decode: unused Bezier frame");
    } else {
      require(c.control_count == 0 && c.control_offset == 0 && c.p == 0 && c.q == 0,
              "decode: analytic control fields");
      require(source_profile ? (c.tag == 1 ? independent_plane_axes(c.x, c.y)
                                           : independent_axes(c.x, c.y, c.z, false))
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
        require(c.r1 != 0 && fabs(c.r1) < 1.5707963267948966, "decode: invalid cone angle");
      if (c.tag == 5)
        require(c.r1 > 0, "decode: invalid torus minor radius");
    }
  }
  for (const auto &c : controls)
    require(c.h[3] > 0, "decode: nonpositive homogeneous weight");
  for (const auto &v : vertices) {
    context.charge(1);
    require(v.carrier < carriers.size(), "decode: vertex carrier");
    require(source_profile ? source_vertex_flags(carriers[v.carrier], v.flags)
                           : (v.flags & ~3u) == 0,
            "decode: vertex carrier/flags");
    const Carrier &c = carriers[v.carrier];
    require(v.uv[0] >= c.uv[0] && v.uv[0] <= c.uv[1] && v.uv[1] >= c.uv[2] && v.uv[1] <= c.uv[3],
            "decode: source UV outside carrier domain");
  }
  for (const auto &m : materials) {
    context.charge(1);
    for (auto c : m.rgb)
      require(c >= 0 && c <= 1, "decode: material color range");
    require(m.roughness > 0 && m.roughness <= 1, "decode: material roughness range");
  }
}
// Canonical flat placements are shared by file decoding and direct factories.
// Base records must have passed validate_scene_records first.
inline void validate_scene_placements(const SceneView &scene, ValidationContext context = {}) {
  const auto vertices = records(scene.vertex_refs, scene.nplaced_vertices);
  const auto primitives = records(scene.primitive_refs, scene.nprimitives);
  uint64_t nv = 0, nt = 0;
  for (uint64_t i = 0; i < scene.ninstances; ++i) {
    context.charge(1);
    const auto &instance = scene.instances[i];
    const auto &definition = scene.definitions[instance.definition];
    const auto ve = checked_add(nv, definition.vertex_count),
               te = checked_add(nt, definition.triangle_count);
    require(ve <= vertices.size() && te <= primitives.size(),
            "decode: missing placed display rows");
    for (uint64_t v = 0; v < definition.vertex_count; ++v) {
      context.charge(1);
      const auto &ref = vertices[nv + v];
      require(ref.source == definition.first_vertex + v && ref.instance == i,
              "decode: noncanonical placed vertex");
    }
    for (uint64_t t = 0; t < definition.triangle_count; ++t) {
      context.charge(1);
      const auto &ref = primitives[nt + t];
      require(ref.triangle == definition.first_triangle + t && ref.instance == i &&
                  ref.vertex_base == nv && ref.reserved == 0,
              "decode: noncanonical placed primitive");
      require(scene.triangles[ref.triangle].material != INHERIT_OCCURRENCE_MATERIAL ||
                  instance.reserved != 0,
              "decode: inherited triangle has no occurrence appearance");
    }
    nv = ve;
    nt = te;
  }
  require(nv == vertices.size() && nt == primitives.size(), "decode: unowned placed display rows");
}
inline void validate_scene_view(const SceneView *scene, bool source_profile,
                                ValidationContext context = {}) {
  require(scene, "decode: absent scene");
  validate_scene_records(*scene, source_profile, context);
  validate_scene_placements(*scene, context);
}
inline void validate_request(const Request &r) {
  finite_fields(r.basis);
  finite_fields(r.center);
  finite_fields(r.pixel_center);
  finite_fields(r.background);
  finite_fields(r.reserved3);
  require(finite(r.alpha) && finite(r.scale), "decode: nonfinite request scalar");
  require(r.width && r.height && r.width <= 8192 && r.height <= 8192 &&
              (r.supersample == 1 || r.supersample == 2) && r.width * r.supersample <= 8192 &&
              r.height * r.supersample <= 8192,
          "decode: view dimensions");
  require(r.reserved == 0 && r.reserved2 == 0 && r.reserved3[0] == 0 && r.reserved3[1] == 0 &&
              (r.flags & ~7u) == 0,
          "decode: view flags/reserved");
  require(r.alpha >= 0 && r.alpha <= 1 && r.scale > 0, "decode: alpha/scale range");
  require(orthogonal(r.basis, r.basis + 3, r.basis + 6, true), "decode: invalid camera basis");
  for (double c : r.background)
    require(c >= 0 && c <= 1, "decode: background range");
}
} // namespace bgom10
#endif
