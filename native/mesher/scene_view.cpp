#include "scene_view.hpp"
#include "../om10/validation.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <tuple>

namespace bgmesh {
NativeSceneData::NativeSceneData(NativeMesh original, GeometryStore store)
    : mesh(std::move(original)), geometry(std::move(store)),
      definitions(mesh.view().resource.get()), instances(mesh.view().resource.get()),
      vertices(mesh.view().resource.get()), triangles(mesh.view().resource.get()),
      carriers(mesh.view().resource.get()), materials(mesh.view().resource.get()),
      controls(mesh.view().resource.get()), vertex_refs(mesh.view().resource.get()),
      primitive_refs(mesh.view().resource.get()) {}
bgom10::SceneView NativeSceneData::view() const {
  return {definitions.data(),    definitions.size(),   instances.data(),   instances.size(),
          vertices.data(),       vertices.size(),      triangles.data(),   triangles.size(),
          carriers.data(),       carriers.size(),      materials.data(),   materials.size(),
          controls.data(),       controls.size(),      vertex_refs.data(), vertex_refs.size(),
          primitive_refs.data(), primitive_refs.size()};
}
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
uint32_t narrow(uint64_t count) {
  if (count >= UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "native display slot capacity exhausted");
  return static_cast<uint32_t>(count);
}
template <class T> const T &row(const std::pmr::vector<T> &table, MeshRef<T> ref) {
  if (!ref.value || ref.value > table.size())
    invalid("native display semantic reference outside mesh");
  return table[static_cast<std::size_t>(ref.value - 1)];
}
template <class T> std::span<const T> list(const std::pmr::vector<T> &pool, Range<T> range) {
  if (range.first > pool.size() || range.count > pool.size() - range.first)
    invalid("native display semantic list outside mesh");
  return std::span<const T>(pool).subspan(static_cast<std::size_t>(range.first),
                                          static_cast<std::size_t>(range.count));
}
struct Candidate final {
  bgom10::Vertex value;
  FaceId face;
  MCorner corner;
};
auto identity(const Candidate &v) {
  return std::tuple{v.value.physical,
                    v.value.carrier,
                    v.face.value,
                    v.value.flags,
                    std::bit_cast<uint64_t>(v.value.uv[0]),
                    std::bit_cast<uint64_t>(v.value.uv[1])};
}
class SceneBuilder final {
  bgfd::Budget &budget;
  NativeSceneData &out;
  const NativeMeshData &mesh;
  const GeometryData &geometry;
  const SourceData &source;
  std::pmr::vector<uint32_t> analytic, roots;
  std::pmr::vector<const ExactTable *> tables;
  std::pmr::vector<MCellMap> facet_map;
  GeometryContext context;
  Rational exact(ExactRef ref) {
    const auto it = std::lower_bound(tables.begin(), tables.end(), ref.table,
                                     [](auto table, auto key) { return table->key() < key; });
    if (it == tables.end() || (*it)->key() != ref.table)
      invalid("native display exact source map has no lease");
    return import_exact(budget, (*it)->view(ref));
  }
  uint32_t carrier(const MeshCorner &corner) {
    const auto slot = source.carrier_index.find(corner.carrier);
    const auto &original = source.carriers[slot.value];
    if (original.kind == 6) {
      const auto root =
          find_surface_root(out.geometry, slot, Rational::from_double(budget, corner.uv.chosen[0]),
                            Rational::from_double(budget, corner.uv.chosen[1]));
      auto &index = roots[root.value];
      if (index != UINT32_MAX)
        return index;
      const auto &patch = geometry.surface_roots[root.value];
      bgom10::Carrier record{};
      record.id = original.id.value;
      record.tag = 6;
      record.p = patch.p;
      record.q = patch.q;
      for (std::size_t i = 0; i < 4; ++i)
        record.uv[i] = import_exact(budget, out.geometry.exact(patch.bounds[i])).to_double();
      record.control_offset = out.controls.size();
      if (patch.controls.count % 4)
        invalid("display source root has incomplete homogeneous controls");
      record.control_count = patch.controls.count / 4;
      for (uint64_t i = 0; i < record.control_count; ++i) {
        bgom10::Control value{};
        for (uint64_t axis = 0; axis < 4; ++axis) {
          value.h[axis] =
              import_exact(budget, out.geometry.exact(patch.controls.first + 4 * i + axis))
                  .to_double();
          if (!std::isfinite(value.h[axis]))
            throw Error(ErrorCode::unresolved, "display source coefficient is not representable");
        }
        if (value.h[3] <= 0)
          throw Error(ErrorCode::unresolved, "display source weight is not representable");
        out.controls.push_back(value);
      }
      index = narrow(out.carriers.size());
      out.carriers.push_back(record);
      return index;
    }
    auto &index = analytic[slot.value];
    if (index == UINT32_MAX) {
      bgom10::Carrier record{};
      record.id = original.id.value;
      record.tag = original.kind;
      std::copy_n(original.frame.begin(), 3, record.origin);
      std::copy_n(original.frame.begin() + 3, 3, record.x);
      std::copy_n(original.frame.begin() + 6, 3, record.y);
      std::copy_n(original.frame.begin() + 9, 3, record.z);
      record.r0 = original.radii[0];
      record.r1 = original.radii[1];
      record.uv[0] = record.uv[1] = corner.uv.chosen[0];
      record.uv[2] = record.uv[3] = corner.uv.chosen[1];
      index = narrow(out.carriers.size());
      out.carriers.push_back(record);
    } else {
      auto &record = out.carriers[index];
      record.uv[0] = std::min(record.uv[0], corner.uv.chosen[0]);
      record.uv[1] = std::max(record.uv[1], corner.uv.chosen[0]);
      record.uv[2] = std::min(record.uv[2], corner.uv.chosen[1]);
      record.uv[3] = std::max(record.uv[3], corner.uv.chosen[1]);
    }
    return index;
  }
  uint32_t normal_flags(const MeshCorner &corner, MFacet facet) {
    uint32_t flags = bgom10::STORED_XYZ | bgom10::COFACTOR_NORMAL;
    if (corner.orientation < 0)
      flags |= bgom10::REVERSE_NORMAL;
    if (!corner.uv.quotient.value)
      return flags;
    const auto &q = row(mesh.quotients, corner.uv.quotient);
    if (q.kind != MeshQuotientKind::pole && q.kind != MeshQuotientKind::apex)
      return flags;
    const auto map_ref = facet_map[static_cast<std::size_t>(facet.value - 1)];
    if (!map_ref.value)
      invalid("declared pole corner has no source-cell map");
    const auto *map = std::get_if<PoleFanMap>(&row(mesh.cell_maps, map_ref).map);
    if (!map || map->quotient != corner.uv.quotient || q.pole != map->pole)
      invalid("declared pole corner disagrees with its retained fan");
    const auto &original = source.carriers[source.carrier_index.find(corner.carrier).value];
    const auto v0 = exact(map->edge_uv[1]), v1 = exact(map->edge_uv[3]);
    bool negative = false;
    for (;;) {
      std::array<int, 2> signs{};
      for (std::size_t i = 0; i < 2; ++i) {
        const auto &v = i == 0 ? v0 : v1;
        const auto scalar = [&]() {
          if (original.kind == 4)
            return enclose_sin_cos(context, v).cosine;
          if (original.kind != 3)
            invalid("pole normal limit has no matching original analytic carrier");
          return geometry_detail::sum(
              ExactEnclosure(Rational::from_double(budget, original.radii[0])),
              geometry_detail::times(
                  enclose_sin_cos(context, Rational::from_double(budget, original.radii[1])).sine,
                  v));
        }();
        signs[i] = scalar.lower.sign() > 0 ? 1 : scalar.upper.sign() < 0 ? -1 : 0;
      }
      if (signs[0] && signs[1]) {
        if (signs[0] != signs[1])
          invalid("declared pole fan spans two regular normal sheets");
        negative = signs[0] < 0;
        break;
      }
      context.refine();
    }
    flags |= bgom10::ANALYTIC_NORMAL_LIMIT;
    if (negative)
      flags |= bgom10::NEGATIVE_RADIAL_SHEET;
    if (q.pole == MeshPole::sphere_south)
      flags |= bgom10::SPHERE_SOUTH;
    return flags;
  }

public:
  SceneBuilder(bgfd::Budget &b, NativeSceneData &output)
      : budget(b), out(output), mesh(out.mesh.view()), geometry(out.geometry.view()),
        source(mesh.source.view()), analytic(source.carriers.size(), UINT32_MAX, &b),
        roots(geometry.surface_roots.size(), UINT32_MAX, &b), tables(&b),
        facet_map(mesh.facets.size(), MCellMap{}, &b), context(b) {
    for (const auto &table : mesh.exact_tables)
      tables.push_back(&table);
    std::sort(tables.begin(), tables.end(), [](auto a, auto c) { return a->key() < c->key(); });
    for (const auto &association : mesh.cell_facets) {
      const auto &cell = row(mesh.cells, association.cell);
      auto &mapped = facet_map[static_cast<std::size_t>(association.facet.value - 1)];
      if (mapped.value && mapped != cell.map)
        throw Error(ErrorCode::unsupported, "display facet has multiple incompatible source maps");
      mapped = cell.map;
    }
  }
  void build(ScenePresentation presentation) {
    if (!presentation.explosion.empty() && presentation.explosion.size() != mesh.instances.size())
      invalid("native display presentation has wrong occurrence inventory");
    for (const auto &material : mesh.appearances) {
      bgom10::Material value{};
      std::copy(material.rgb.begin(), material.rgb.end(), value.rgb);
      value.roughness = material.roughness;
      out.materials.push_back(value);
    }
    std::pmr::vector<uint32_t> corner_remap(mesh.corners.size(), UINT32_MAX, &budget);
    for (const auto &definition : mesh.definitions) {
      bgom10::Definition record{definition.source.value, out.vertices.size(), 0,
                                out.triangles.size(), 0};
      std::pmr::vector<Candidate> candidates(&budget);
      const auto facets = list(mesh.facet_refs, definition.facets);
      if (facets.size() > candidates.max_size() / 3)
        throw Error(ErrorCode::resource_exhausted, "native display corner count exhausted");
      candidates.reserve(facets.size() * 3);
      for (const auto facet_ref : facets) {
        budget.charge(1);
        const auto &facet = row(mesh.facets, facet_ref);
        for (std::size_t i = 0; i < 3; ++i) {
          const auto &corner = row(mesh.corners, facet.corners[i]);
          if (corner.uv.kind == MeshLocatorKind::unavailable_guide)
            throw Error(ErrorCode::unsupported, "display corner has no source normal locator");
          bgom10::Vertex vertex{};
          const auto &point = row(mesh.nodes, facet.nodes[i]);
          std::copy(point.xyz.begin(), point.xyz.end(), vertex.xyz);
          std::copy(corner.uv.chosen.begin(), corner.uv.chosen.end(), vertex.uv);
          vertex.carrier = carrier(corner);
          vertex.flags = normal_flags(corner, facet_ref);
          vertex.physical = facet.nodes[i].value;
          candidates.push_back({vertex, row(mesh.faces, facet.face).source, facet.corners[i]});
        }
      }
      std::sort(candidates.begin(), candidates.end(), [&](const auto &a, const auto &b) {
        budget.charge(1);
        return std::tuple{identity(a), a.corner.value} < std::tuple{identity(b), b.corner.value};
      });
      std::optional<decltype(identity(candidates.front()))> last;
      uint32_t local = 0;
      for (const auto &candidate : candidates) {
        budget.charge(1);
        const auto key = identity(candidate);
        if (!last || *last != key) {
          local = narrow(out.vertices.size() - record.first_vertex);
          static_cast<void>(narrow(out.vertices.size()));
          out.vertices.push_back(candidate.value);
          last = key;
        }
        corner_remap[static_cast<std::size_t>(candidate.corner.value - 1)] = local;
      }
      for (const auto facet_ref : facets) {
        const auto &facet = row(mesh.facets, facet_ref);
        bgom10::Triangle triangle{};
        for (std::size_t i = 0; i < 3; ++i)
          triangle.vertex[i] = corner_remap[static_cast<std::size_t>(facet.corners[i].value - 1)];
        triangle.face = row(mesh.faces, facet.face).source.value;
        const auto &face = source.faces[source.face_index.find(FaceId{triangle.face}).value];
        triangle.material = face.style_id ? narrow(facet.appearance.value - 1) : UINT32_MAX;
        static_cast<void>(narrow(out.triangles.size()));
        out.triangles.push_back(triangle);
      }
      record.vertex_count = out.vertices.size() - record.first_vertex;
      record.triangle_count = out.triangles.size() - record.first_triangle;
      out.definitions.push_back(record);
    }
    for (std::size_t i = 0; i < mesh.instances.size(); ++i) {
      budget.charge(1);
      const auto &original = mesh.instances[i];
      bgom10::Instance instance{};
      instance.id = i + 1;
      instance.definition = narrow(original.definition.value - 1);
      instance.reserved = narrow(original.appearance.value);
      std::copy(original.matrix.begin(), original.matrix.end(), instance.matrix);
      const auto &explosion =
          presentation.explosion.empty() ? original.explosion : presentation.explosion[i];
      for (std::size_t axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(explosion[axis]))
          invalid("nonfinite native display explosion offset");
        instance.explosion[axis] = explosion[axis];
      }
      out.instances.push_back(instance);
    }
    for (std::size_t i = 0; i < out.instances.size(); ++i) {
      const auto &instance = out.instances[i];
      const auto &definition = out.definitions[instance.definition];
      const auto base = narrow(out.vertex_refs.size());
      static_cast<void>(narrow(out.vertex_refs.size() + definition.vertex_count));
      static_cast<void>(narrow(out.primitive_refs.size() + definition.triangle_count));
      for (uint64_t v = 0; v < definition.vertex_count; ++v) {
        budget.charge(1);
        out.vertex_refs.push_back({narrow(definition.first_vertex + v), narrow(i)});
      }
      for (uint64_t t = 0; t < definition.triangle_count; ++t) {
        budget.charge(1);
        out.primitive_refs.push_back({narrow(definition.first_triangle + t), narrow(i), base, 0});
      }
    }
  }
};
class Writer final {
  bgfd::Budget &budget_;
  std::ofstream file_;
  std::array<char, 65536> buffer_{};
  std::size_t used_ = 0;
  void flush() {
    file_.write(buffer_.data(), static_cast<std::streamsize>(used_));
    used_ = 0;
    if (!file_)
      throw Error(ErrorCode::io_failure, "native display output write failed");
  }

public:
  Writer(const std::filesystem::path &path, bgfd::Budget &budget)
      : budget_(budget), file_(path, std::ios::binary) {
    if (!file_)
      throw Error(ErrorCode::io_failure, "cannot open native display output");
  }
  void raw(std::string_view value) {
    budget_.charge(value.size());
    for (const auto c : value) {
      if (used_ == buffer_.size())
        flush();
      buffer_[used_++] = c;
    }
  }
  void u64(uint64_t value) {
    std::array<char, 8> bytes{};
    for (uint32_t i = 0; i < 8; ++i)
      bytes[i] = static_cast<char>(value >> (i * 8));
    raw({bytes.data(), bytes.size()});
  }
  void u32(uint32_t value) {
    std::array<char, 4> bytes{};
    for (uint32_t i = 0; i < 4; ++i)
      bytes[i] = static_cast<char>(value >> (i * 8));
    raw({bytes.data(), bytes.size()});
  }
  void f64(double value) {
    if (!std::isfinite(value))
      invalid("nonfinite native display wire scalar");
    u64(std::bit_cast<uint64_t>(value));
  }
  void finish() {
    flush();
    file_.close();
    if (!file_)
      throw Error(ErrorCode::io_failure, "native display output close failed");
  }
};
} // namespace
NativeScene make_scene(bgfd::Budget &budget, NativeMesh mesh, GeometryStore geometry,
                       ScenePresentation presentation) {
  const auto &source = mesh.view().source.view();
  if (&source != &geometry.view().source.view() || mesh.view().resource != source.resource)
    invalid("native display original source/geometry ownership mismatch");
  auto result = std::allocate_shared<NativeSceneData>(
      RunAllocator<NativeSceneData>(source.resource), std::move(mesh), std::move(geometry));
  SceneBuilder builder(budget, *result);
  builder.build(presentation);
  const auto view = result->view();
  bgom10::validate_scene_view(&view, true, {&budget, &budget, [](void *state, uint64_t count) {
                                              static_cast<bgfd::Budget *>(state)->charge(count);
                                            }});
  return result;
}
void write_scene(const std::filesystem::path &path, const NativeScene &scene,
                 bgfd::Budget &budget) {
  if (!scene)
    invalid("native display scene is absent");
  write_scene(path, scene->view(), budget);
}
void write_scene(const std::filesystem::path &path, const bgom10::SceneView &scene,
                 bgfd::Budget &budget) {
  bgom10::validate_scene_view(&scene, true, {&budget, &budget, [](void *state, uint64_t count) {
                                               static_cast<bgfd::Budget *>(state)->charge(count);
                                             }});
  const auto definitions = bgom10::records(scene.definitions, scene.ndefinitions);
  const auto instances = bgom10::records(scene.instances, scene.ninstances);
  const auto vertices = bgom10::records(scene.vertices, scene.nvertices);
  const auto triangles = bgom10::records(scene.triangles, scene.ntriangles);
  const auto carriers = bgom10::records(scene.carriers, scene.ncarriers);
  const auto materials = bgom10::records(scene.materials, scene.nmaterials);
  const auto controls = bgom10::records(scene.controls, scene.ncontrols);
  Writer writer(path, budget);
  writer.raw("BGMESH01");
  writer.u32(1);
  writer.u32(bgom10::SOURCE_GEOMETRY_PROFILE);
  for (const uint64_t count :
       {definitions.size(), instances.size(), vertices.size(), triangles.size(), carriers.size(),
        materials.size(), controls.size(), std::size_t{0}})
    writer.u64(count);
  for (const auto &v : definitions) {
    writer.u64(v.id);
    writer.u64(v.first_vertex);
    writer.u64(v.vertex_count);
    writer.u64(v.first_triangle);
    writer.u64(v.triangle_count);
  }
  for (const auto &v : instances) {
    writer.u64(v.id);
    writer.u32(v.definition);
    writer.u32(v.reserved);
    for (double x : v.matrix)
      writer.f64(x);
    for (double x : v.explosion)
      writer.f64(x);
  }
  for (const auto &v : vertices) {
    for (double x : v.xyz)
      writer.f64(x);
    for (double x : v.uv)
      writer.f64(x);
    writer.u32(v.carrier);
    writer.u32(v.flags);
    writer.u64(v.physical);
  }
  for (const auto &v : triangles) {
    for (uint32_t x : v.vertex)
      writer.u32(x);
    writer.u32(v.material);
    writer.u64(v.face);
  }
  for (const auto &v : carriers) {
    writer.u64(v.id);
    writer.u32(v.tag);
    writer.u32(v.flags);
    for (const auto *vector : {v.origin, v.x, v.y, v.z})
      for (std::size_t i = 0; i < 3; ++i)
        writer.f64(vector[i]);
    writer.f64(v.r0);
    writer.f64(v.r1);
    for (double x : v.uv)
      writer.f64(x);
    writer.u64(v.control_offset);
    writer.u64(v.control_count);
    writer.u32(v.p);
    writer.u32(v.q);
    writer.u64(v.reserved);
  }
  for (const auto &v : materials) {
    for (double x : v.rgb)
      writer.f64(x);
    writer.f64(v.roughness);
  }
  for (const auto &v : controls)
    for (double x : v.h)
      writer.f64(x);
  writer.finish();
}
void write_views(const std::filesystem::path &path, std::span<const bgom10::Request> requests,
                 bgfd::Budget &budget) {
  for (const auto &request : requests) {
    budget.charge(1);
    bgom10::validate_request(request);
  }
  Writer writer(path, budget);
  writer.raw("BGVIEW01");
  writer.u32(1);
  writer.u32(narrow(requests.size()));
  for (const auto &v : requests) {
    writer.u32(v.width);
    writer.u32(v.height);
    writer.u32(v.supersample);
    writer.u32(v.reserved);
    writer.f64(v.alpha);
    for (double x : v.basis)
      writer.f64(x);
    for (double x : v.center)
      writer.f64(x);
    for (double x : v.pixel_center)
      writer.f64(x);
    writer.f64(v.scale);
    for (double x : v.background)
      writer.f64(x);
    writer.u32(v.flags);
    writer.u32(v.reserved2);
    for (double x : v.reserved3)
      writer.f64(x);
  }
  writer.finish();
}
} // namespace bgmesh
