#include "adaptive_scene.hpp"
#include "../om10/validation.hpp"
#include "adaptive_geometry.hpp"
#include <bit>
#include <cmath>
#include <tuple>

namespace bgmesh::adaptive {
AdaptiveSceneData::AdaptiveSceneData(std::shared_ptr<RunResource> owner,
                                     const AdaptiveSceneRequest &request)
    : SceneBuffers(std::move(owner)), source(request.assembly.view().source),
      assembly(request.assembly), scope(request.scope), components(resource.get()),
      original_definitions(resource.get()), original_instances(resource.get()),
      physical_keys(resource.get()), material_styles(resource.get()),
      vertex_sources(resource.get()), facet_sources(resource.get()) {}
namespace {
void require(bool okay, const char *message) {
  if (!okay)
    throw Error(ErrorCode::invalid_source, message);
}
uint32_t narrow(uint64_t value) {
  if (value >= UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "adaptive scene slot capacity exhausted");
  return static_cast<uint32_t>(value);
}
bool same_xyz(const Vec3 &a, const Vec3 &b) {
  for (uint32_t axis = 0; axis < 3; ++axis)
    if (std::bit_cast<uint64_t>(a[axis]) != std::bit_cast<uint64_t>(b[axis]))
      return false;
  return true;
}
using UV = std::array<Rational, 2>;
Rational cross(const UV &a, const UV &b, const UV &c) {
  return b[0]
      .subtract(a[0])
      .multiply(c[1].subtract(a[1]))
      .subtract(b[1].subtract(a[1]).multiply(c[0].subtract(a[0])));
}
// A cosmetic root-side selection only: virtual points never become mesh sites.
bool positive_overlap(std::span<const UV, 3> triangle, const RootDescriptor &root,
                      bgfd::Budget &budget) {
  std::pmr::vector<UV> polygon(triangle.begin(), triangle.end(), &budget), next(&budget);
  for (uint32_t side = 0; side < 4 && !polygon.empty(); ++side) {
    const auto axis = side / 2;
    const auto bound = Rational::from_double(budget, root.original_box[side].lower);
    const auto sign = side % 2 ? -1 : 1;
    next.clear();
    for (uint32_t i = 0; i < polygon.size(); ++i) {
      budget.charge(1);
      const auto &a = polygon[i], &b = polygon[(i + 1) % polygon.size()];
      const auto da = a[axis].subtract(bound), db = b[axis].subtract(bound);
      const bool ina = da.sign() * sign >= 0, inb = db.sign() * sign >= 0;
      if (ina)
        next.push_back(a);
      if (ina != inb) {
        const auto t = da.divide(da.subtract(db));
        next.push_back(
            {a[0].add(t.multiply(b[0].subtract(a[0]))), a[1].add(t.multiply(b[1].subtract(a[1])))});
      }
    }
    polygon.swap(next);
  }
  for (uint32_t i = 1; i + 1 < polygon.size(); ++i)
    if (cross(polygon[0], polygon[i], polygon[i + 1]).sign() > 0)
      return true;
  return false;
}
struct RootIndex final {
  const SourceCache *cache;
  std::pmr::vector<uint32_t> row_end;
  RootIndex(const SourceCache &source, bgfd::Budget &budget)
      : cache(&source), row_end(source.original_roots().size(), 0, &budget) {
    const auto roots = source.original_roots();
    for (uint32_t first = 0; first < roots.size();) {
      auto end = first + 1;
      while (end < roots.size() && roots[end].source_slot == roots[first].source_slot &&
             roots[end].span_u == roots[first].span_u)
        ++end;
      for (auto i = first; i < end; ++i) {
        budget.charge(1);
        row_end[i] = end;
      }
      first = end;
    }
  }
  OriginalRootSlot choose(Slot<CarrierTag> carrier, const UV &uv, std::span<const UV, 3> triangle,
                          bgfd::Budget &budget) const {
    const auto roots = cache->original_roots();
    const auto initial = find_carrier_root_slot(roots, carrier, uv, RootBoxKind::original);
    std::array<uint32_t, 4> candidates{};
    uint32_t count = 0;
    const auto add_row = [&](uint32_t first) {
      auto end = row_end[first], last = end;
      while (first < last) {
        budget.charge(1);
        const auto mid = first + (last - first) / 2;
        if (Rational::from_double(budget, roots[mid].original_box[3].lower).compare(uv[1]) < 0)
          first = mid + 1;
        else
          last = mid;
      }
      require(
          first < end &&
              Rational::from_double(budget, roots[first].original_box[2].lower).compare(uv[1]) <= 0,
          "adaptive normal locator outside original V span");
      candidates[count++] = first;
      if (first + 1 < end &&
          Rational::from_double(budget, roots[first].original_box[3].lower).compare(uv[1]) == 0)
        candidates[count++] = first + 1;
    };
    add_row(initial);
    const auto next = row_end[initial];
    if (next < roots.size() && roots[next].source_slot == carrier.value &&
        Rational::from_double(budget, roots[initial].original_box[1].lower).compare(uv[0]) == 0)
      add_row(next);
    if (count == 1)
      return {initial};
    const UV centroid{
        triangle[0][0].add(triangle[1][0]).add(triangle[2][0]).divide(Rational(budget, 3)),
        triangle[0][1].add(triangle[1][1]).add(triangle[2][1]).divide(Rational(budget, 3))};
    std::optional<uint32_t> fallback;
    for (uint32_t i = 0; i < count; ++i) {
      const auto index = candidates[i];
      const auto &root = roots[index];
      if (!positive_overlap(triangle, root, budget))
        continue;
      if (!fallback)
        fallback = index;
      bool interior_direction = true;
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto direction = centroid[axis].compare(uv[axis]);
        if ((Rational::from_double(budget, root.original_box[2 * axis].lower).compare(uv[axis]) ==
                 0 &&
             direction <= 0) ||
            (Rational::from_double(budget, root.original_box[2 * axis + 1].lower)
                     .compare(uv[axis]) == 0 &&
             direction >= 0))
          interior_direction = false;
      }
      if (interior_direction)
        return {index};
    }
    if (!fallback)
      throw Error(ErrorCode::invalid_source,
                  "adaptive corner has no incident original normal sector");
    return {fallback.value()};
  }
};
struct Physical final {
  PhysicalNodeKey key;
  Vec3 xyz;
};
struct UsedRoot final {
  uint32_t carrier, u, v, display;
  auto key() const { return std::tuple{carrier, u, v}; }
};
struct Candidate final {
  bgom10::Vertex vertex;
  AdaptiveSceneVertexSource source;
  uint64_t corner;
  auto key() const {
    return std::tuple{vertex.physical, source.face, source.site, vertex.carrier, vertex.flags};
  }
};
class Builder final {
  bgfd::Budget &budget;
  AdaptiveSceneData &out;
  const SourceData &source;
  std::pmr::vector<uint32_t> analytic;
  std::pmr::vector<UsedRoot> used;
  std::pmr::vector<RootIndex> indexes;
  std::pmr::vector<Physical> physical;
  uint32_t material(StyleId style) const {
    return style ? source.style_index.find(style).value + 1 : 0;
  }
  uint64_t physical_id(const PhysicalNodeKey &key) const {
    const auto it =
        std::lower_bound(physical.begin(), physical.end(), key,
                         [](const auto &row, const auto &value) { return row.key < value; });
    require(it != physical.end() && it->key == key, "adaptive scene physical site absent");
    return static_cast<uint64_t>(it - physical.begin()) + 1;
  }
  RootIndex &index(const SourceCache &cache) {
    for (auto &entry : indexes)
      if (&entry.cache->view() == &cache.view())
        return entry;
    indexes.emplace_back(cache, budget);
    return indexes.back();
  }
  uint32_t normal_carrier(const SourceCache &cache, Slot<CarrierTag> slot,
                          OriginalRootSlot original, const Vec2 &uv) {
    const auto &carrier = source.carriers[slot.value];
    if (carrier.kind == 6) {
      const auto &root = cache.original_roots()[original.value];
      const auto key = std::tuple{slot.value, root.span_u, root.span_v};
      const auto found =
          std::lower_bound(used.begin(), used.end(), key,
                           [](const auto &row, const auto &value) { return row.key() < value; });
      if (found != used.end() && found->key() == key)
        return found->display;
      const auto net = surface_detail::original_net_exact(cache, original, budget);
      require(net.p <= 3 && net.q <= 3, "adaptive display source degree outside profile");
      bgom10::Carrier record{};
      record.id = carrier.id.value;
      record.tag = 6;
      record.p = net.p;
      record.q = net.q;
      for (uint32_t axis = 0; axis < 4; ++axis)
        record.uv[axis] = net.bounds[axis].to_double();
      record.control_offset = out.controls.size();
      require(net.controls.size() % 4 == 0, "adaptive source net has incomplete controls");
      record.control_count = net.controls.size() / 4;
      for (uint64_t i = 0; i < record.control_count; ++i) {
        bgom10::Control value{};
        for (uint32_t axis = 0; axis < 4; ++axis) {
          value.h[axis] = net.controls[4 * i + axis].to_double();
          if (!std::isfinite(value.h[axis]))
            throw Error(ErrorCode::unresolved, "adaptive display source control not representable");
        }
        if (value.h[3] <= 0)
          throw Error(ErrorCode::unresolved, "adaptive display source weight not representable");
        out.controls.push_back(value);
      }
      const auto display = narrow(out.carriers.size());
      out.carriers.push_back(record);
      used.insert(found, {slot.value, root.span_u, root.span_v, display});
      return display;
    }
    auto &display = analytic[slot.value];
    if (display == UINT32_MAX) {
      require(carrier.kind >= 1 && carrier.kind <= 5,
              "adaptive analytic display carrier unsupported");
      bgom10::Carrier record{};
      record.id = carrier.id.value;
      record.tag = carrier.kind;
      std::copy_n(carrier.frame.begin(), 3, record.origin);
      std::copy_n(carrier.frame.begin() + 3, 3, record.x);
      std::copy_n(carrier.frame.begin() + 6, 3, record.y);
      std::copy_n(carrier.frame.begin() + 9, 3, record.z);
      record.r0 = carrier.kind == 1 ? 0 : carrier.radii[0];
      record.r1 = carrier.kind == 3 || carrier.kind == 5 ? carrier.radii[1] : 0;
      record.uv[0] = record.uv[1] = uv[0];
      record.uv[2] = record.uv[3] = uv[1];
      display = narrow(out.carriers.size());
      out.carriers.push_back(record);
    } else {
      auto &record = out.carriers[display];
      for (uint32_t axis = 0; axis < 2; ++axis) {
        record.uv[2 * axis] = std::min(record.uv[2 * axis], uv[axis]);
        record.uv[2 * axis + 1] = std::max(record.uv[2 * axis + 1], uv[axis]);
      }
    }
    return display;
  }

public:
  Builder(bgfd::Budget &owner, AdaptiveSceneData &scene)
      : budget(owner), out(scene), source(scene.source.view()),
        analytic(source.carriers.size(), UINT32_MAX, &owner), used(&owner), indexes(&owner),
        physical(&owner) {}
  void build(const AdaptiveSceneRequest &request) {
    const auto &assembly = out.assembly.view();
    require(!request.components.empty(), "adaptive scene has no complete components");
    static_cast<void>(narrow(source.styles.size() + 1));
    std::pmr::vector<uint8_t> assembly_used(source.definitions.size(), 0, &budget);
    for (const auto &instance : assembly.instances) {
      budget.charge(1);
      require(instance.definition.value < assembly_used.size(),
              "adaptive assembly instance definition outside source");
      assembly_used[instance.definition.value] = 1;
    }
    out.components.assign(request.components.begin(), request.components.end());
    for (const auto &component : out.components)
      require(component && component->resource == out.resource &&
                  &component->source.source().view() == &source &&
                  component->query.definition.value < source.definitions.size(),
              "adaptive scene component/source mismatch");
    std::sort(out.components.begin(), out.components.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return source.definitions[a->query.definition.value].id <
             source.definitions[b->query.definition.value].id;
    });
    std::pmr::vector<uint32_t> definition_remap(source.definitions.size(), UINT32_MAX, &budget);
    for (uint32_t c = 0; c < out.components.size(); ++c) {
      const auto &component = *out.components[c];
      const auto def = component.query.definition;
      require(definition_remap[def.value] == UINT32_MAX &&
                  component.query.generation == out.components.front()->query.generation,
              "adaptive scene duplicated definition or mixed quality generation");
      const auto allowance = std::find_if(assembly.definitions.begin(), assembly.definitions.end(),
                                          [&](const auto &row) { return row.definition == def; });
      require(allowance != assembly.definitions.end(),
              "adaptive scene definition has no assembly allowance");
      require(import_exact(budget, component.query.values.view(component.query.positional))
                          .compare(import_exact(
                              budget, assembly.values.view(allowance->positional))) <= 0 &&
                  import_exact(budget, component.query.values.view(component.query.node_source))
                          .compare(import_exact(budget,
                                                assembly.values.view(allowance->node_source))) <= 0,
              "adaptive scene component exceeds assembly allowance");
      const auto inventory = source.definition_faces.at(def.value);
      require(component.faces.size() == inventory.size(),
              "adaptive scene incomplete definition face inventory");
      for (uint32_t f = 0; f < inventory.size(); ++f) {
        const auto &face = component.faces[f];
        require(face && face->resource == out.resource && face->construction &&
                    &face->construction->source.view() == &source &&
                    face->construction->key.definition == def &&
                    face->construction->key.face.value == inventory[f] &&
                    face->construction->key.query == component.query.generation,
                "adaptive scene component contains a foreign/incomplete face");
        for (uint32_t s = 0; s < face->construction->sites.size(); ++s) {
          budget.charge(1);
          physical.push_back(
              {component_site_key(component, {f, s}), face->construction->sites[s].actual_xyz});
        }
      }
      definition_remap[def.value] = c;
      out.original_definitions.push_back(def);
    }
    if (request.scope == SceneScope::complete_assembly) {
      require(out.components.size() == static_cast<std::size_t>(std::count(
                                           assembly_used.begin(), assembly_used.end(), uint8_t{1})),
              "adaptive scene incomplete assembly definition inventory");
      for (uint32_t definition = 0; definition < assembly_used.size(); ++definition)
        if (assembly_used[definition])
          require(definition_remap[definition] != UINT32_MAX,
                  "adaptive scene missing assembly definition");
    } else
      require(request.scope == SceneScope::selected_components, "invalid adaptive scene scope");
    std::sort(physical.begin(), physical.end(), [&](const auto &a, const auto &b) {
      budget.charge(1);
      return a.key < b.key;
    });
    auto destination = physical.begin();
    for (auto it = physical.begin(); it != physical.end(); ++it) {
      budget.charge(1);
      if (destination != physical.begin() && (destination - 1)->key == it->key)
        require(same_xyz((destination - 1)->xyz, it->xyz),
                "adaptive scene physical key changed actual XYZ");
      else
        *destination++ = *it;
    }
    physical.erase(destination, physical.end());
    for (const auto &point : physical)
      out.physical_keys.push_back(point.key);
    out.materials.push_back({{.7, .7, .7}, .5});
    out.material_styles.push_back({});
    for (const auto &style : source.styles) {
      bgom10::Material value{{.7, .7, .7}, .5};
      if (style.has_color)
        std::copy_n(style.rgba.begin(), 3, value.rgb);
      out.materials.push_back(value);
      out.material_styles.push_back(style.id);
    }
    for (uint32_t c = 0; c < out.components.size(); ++c) {
      const auto &component = *out.components[c];
      bgom10::Definition definition{source.definitions[component.query.definition.value].id.value,
                                    out.vertices.size(), 0, out.triangles.size(), 0};
      std::pmr::vector<Candidate> candidates(&budget);
      std::pmr::vector<bgom10::Triangle> triangles(&budget);
      for (uint32_t f = 0; f < component.faces.size(); ++f) {
        const auto &mesh = *component.faces[f]->construction;
        const auto &face = source.faces[mesh.key.face.value];
        const auto slot = source.carrier_index.find(face.carrier_id);
        const auto rational = source.carriers[slot.value].kind == 6;
        std::pmr::vector<UV> locations(&budget);
        std::pmr::vector<Vec2> rounded(&budget);
        for (const auto &site : mesh.sites) {
          locations.push_back({import_exact(budget, mesh.values.view(site.uv[0])),
                               import_exact(budget, mesh.values.view(site.uv[1]))});
          rounded.push_back({locations.back()[0].to_double(), locations.back()[1].to_double()});
        }
        for (uint32_t t = 0; t < mesh.facets.size(); ++t) {
          budget.charge(1);
          const auto &facet = mesh.facets[t];
          if (facet.key.generator != FacetGenerator::packet &&
              facet.key.generator != FacetGenerator::band)
            throw Error(ErrorCode::unsupported,
                        "adaptive mapped-facet display profile unavailable");
          for (auto s : facet.sites)
            require(s < mesh.sites.size(), "adaptive scene facet site outside construction");
          const std::array<UV, 3> triangle{locations[facet.sites[0]], locations[facet.sites[1]],
                                           locations[facet.sites[2]]};
          require(cross(triangle[0], triangle[1], triangle[2]).sign() > 0,
                  "adaptive scene requires positive source roles");
          bgom10::Triangle output{};
          output.face = face.id.value;
          output.material =
              face.style_id ? material(face.style_id) : bgom10::INHERIT_OCCURRENCE_MATERIAL;
          for (uint32_t role = 0; role < 3; ++role) {
            const auto s = facet.sites[role];
            const auto &site = mesh.sites[s];
            const auto original =
                rational ? index(mesh.surface_source).choose(slot, locations[s], triangle, budget)
                         : OriginalRootSlot{invalid_slot};
            bgom10::Vertex vertex{};
            std::copy(site.actual_xyz.begin(), site.actual_xyz.end(), vertex.xyz);
            std::copy(rounded[s].begin(), rounded[s].end(), vertex.uv);
            vertex.carrier = normal_carrier(mesh.surface_source, slot, original, rounded[s]);
            vertex.flags = bgom10::STORED_XYZ | bgom10::COFACTOR_NORMAL |
                           (face.orientation < 0 ? bgom10::REVERSE_NORMAL : 0);
            vertex.physical = physical_id(component_site_key(component, {f, s}));
            const auto corner = candidates.size();
            output.vertex[role] = narrow(corner);
            candidates.push_back({vertex, {c, f, s, original, site.uv}, corner});
          }
          if (face.orientation < 0)
            std::swap(output.vertex[1], output.vertex[2]);
          triangles.push_back(output);
          out.facet_sources.push_back({c, f, t});
        }
      }
      std::pmr::vector<uint32_t> remap(candidates.size(), UINT32_MAX, &budget);
      std::sort(candidates.begin(), candidates.end(), [&](const auto &a, const auto &b) {
        budget.charge(1);
        return std::tuple{a.key(), a.corner} < std::tuple{b.key(), b.corner};
      });
      std::optional<decltype(candidates.front().key())> previous;
      uint32_t local = 0;
      for (const auto &candidate : candidates) {
        if (!previous || *previous != candidate.key()) {
          local = narrow(out.vertices.size() - definition.first_vertex);
          narrow(out.vertices.size());
          out.vertices.push_back(candidate.vertex);
          out.vertex_sources.push_back(candidate.source);
          previous = candidate.key();
        }
        remap[candidate.corner] = local;
      }
      for (auto triangle : triangles) {
        budget.charge(1);
        for (auto &vertex : triangle.vertex)
          vertex = remap[vertex];
        narrow(out.triangles.size());
        out.triangles.push_back(triangle);
      }
      definition.vertex_count = out.vertices.size() - definition.first_vertex;
      definition.triangle_count = out.triangles.size() - definition.first_triangle;
      require(definition.vertex_count && definition.triangle_count,
              "adaptive scene has empty definition");
      out.definitions.push_back(definition);
    }
    require(request.presentation.explosion.empty() ||
                request.presentation.explosion.size() == assembly.instances.size(),
            "adaptive scene explosion inventory mismatch");
    std::pmr::vector<uint8_t> placed(out.definitions.size(), 0, &budget);
    for (uint32_t i = 0; i < assembly.instances.size(); ++i) {
      const auto &original = assembly.instances[i];
      require(original.definition.value < definition_remap.size(),
              "adaptive scene instance definition outside source");
      const auto mapped = definition_remap[original.definition.value];
      if (mapped == UINT32_MAX)
        continue;
      placed[mapped] = 1;
      bgom10::Instance instance{};
      instance.id = uint64_t{i} + 1;
      instance.definition = mapped;
      instance.reserved = material(original.style) + 1;
      std::copy(original.matrix.begin(), original.matrix.end(), instance.matrix);
      if (!request.presentation.explosion.empty())
        std::copy(request.presentation.explosion[i].begin(),
                  request.presentation.explosion[i].end(), instance.explosion);
      out.instances.push_back(instance);
      out.original_instances.push_back(i);
      const auto &definition = out.definitions[mapped];
      const auto base = narrow(out.vertex_refs.size());
      narrow(out.vertex_refs.size() + definition.vertex_count);
      narrow(out.primitive_refs.size() + definition.triangle_count);
      for (uint64_t v = 0; v < definition.vertex_count; ++v) {
        budget.charge(1);
        out.vertex_refs.push_back(
            {narrow(definition.first_vertex + v), narrow(out.instances.size() - 1)});
      }
      for (uint64_t t = 0; t < definition.triangle_count; ++t) {
        budget.charge(1);
        out.primitive_refs.push_back(
            {narrow(definition.first_triangle + t), narrow(out.instances.size() - 1), base, 0});
      }
    }
    require(std::all_of(placed.begin(), placed.end(), [](auto value) { return value != 0; }),
            "adaptive scene definition is not placed");
  }
};
} // namespace
AdaptiveScene make_adaptive_scene(bgfd::Budget &budget,
                                  const std::shared_ptr<RunResource> &resource,
                                  const AdaptiveSceneRequest &request) {
  require(resource && request.assembly.view().resource == resource &&
              request.assembly.view().source.view().resource == resource,
          "adaptive scene assembly resource mismatch");
  admit_numeric_environment();
  auto result = std::allocate_shared<AdaptiveSceneData>(RunAllocator<AdaptiveSceneData>(resource),
                                                        resource, request);
  Builder builder(budget, *result);
  builder.build(request);
  const auto view = result->view();
  bgom10::validate_scene_view(&view, true, {&budget, &budget, [](void *state, uint64_t count) {
                                              static_cast<bgfd::Budget *>(state)->charge(count);
                                            }});
  return result;
}
} // namespace bgmesh::adaptive
