#ifndef BREPGPU_MESHER_MESH_WALK_HPP
#define BREPGPU_MESHER_MESH_WALK_HPP
#include "mesh_schema.hpp"
#include <algorithm>
#include <cmath>

namespace bgmesh::mesh_detail {
[[noreturn]] inline void invalid(const char *message) {
  throw Error(ErrorCode::invalid_source, message);
}
template <class T> bool source_exists(const NativeMeshData &d, SourceId<T> value) {
  const auto &s = d.source.view();
  if constexpr (std::is_same_v<T, StyleTag>) {
    if (!value.value)
      return true;
  }
  // clang-format off
#define BGNMESH_SOURCE(type, member)                                                               \
  if constexpr (std::is_same_v<T, type>) {                                                         \
    const auto &entries = s.member.entries;                                                        \
    const auto it =                                                                                \
        std::lower_bound(entries.begin(), entries.end(), value,                                    \
                         [](const auto &entry, auto key) { return entry.first < key; });           \
    return it != entries.end() && it->first == value;                                              \
  }
  BGNMESH_SOURCE(DefinitionTag, definition_index)
  BGNMESH_SOURCE(FaceTag, face_index)
  BGNMESH_SOURCE(VertexTag, vertex_index)
  BGNMESH_SOURCE(EdgeTag, edge_index)
  BGNMESH_SOURCE(CoedgeTag, coedge_index)
  BGNMESH_SOURCE(LoopTag, loop_index)
  BGNMESH_SOURCE(CarrierTag, carrier_index)
  BGNMESH_SOURCE(CurveTag, curve_index)
  BGNMESH_SOURCE(ProductTag, product_index)
  BGNMESH_SOURCE(UsageTag, usage_index)
  BGNMESH_SOURCE(StyleTag, style_index)
#undef BGNMESH_SOURCE
  // clang-format on
  return false;
}
template <class D> struct MeshObserver {
  const NativeMeshData &data;
  bgfd::Budget &budget;
  D &self() { return static_cast<D &>(*this); }
  void require(bool value, const char *message) {
    if (!value)
      invalid(message);
  }
  template <class... T> void fields(T &...v) { (transfer(self(), v), ...); }
  void zero32() {
    uint32_t zero = 0;
    self().u32(zero);
  }
  void account_items(uint64_t) {}
  template <class T> void source(SourceId<T> &v) {
    require(source_exists(data, v), "mesh source ID absent from original source");
    self().u64(v.value);
  }
  template <class T> void reference(MeshRef<T> &v) {
    require(v.value && v.value <= rows<T>(data).size(), "mesh row reference outside table");
    self().u64(v.value);
  }
  template <class T> void optional_reference(MeshRef<T> &v) {
    if (v.value)
      reference(v);
    else
      self().u64(v.value);
  }
  void optional_exact(std::optional<ExactRef> &v) {
    if (v)
      self().exact(*v);
    else {
      uint64_t zero = 0;
      self().u64(zero);
    }
  }
  template <class T> void list(Range<T> &v) {
    const auto &p = pool<T>(data);
    require(v.first <= p.size() && v.count <= p.size() - v.first, "mesh list outside pool");
    self().account_items(v.count);
    self().u64(v.count);
    for (uint64_t i = 0; i < v.count; ++i) {
      auto item = p[static_cast<std::size_t>(v.first + i)];
      self().fields(item);
    }
  }
  template <class... T> void variant(std::variant<T...> &v) {
    require(!v.valueless_by_exception(), "valueless mesh variant");
    constexpr bool none =
        std::is_same_v<std::variant_alternative_t<0, std::variant<T...>>, std::monostate>;
    auto tag = static_cast<uint32_t>(v.index() + (none ? 0 : 1));
    self().u32(tag);
    self().zero32();
    std::visit([&](auto &item) { self().fields(item); }, v);
  }
};
} // namespace bgmesh::mesh_detail
#endif
