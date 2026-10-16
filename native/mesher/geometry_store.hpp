#ifndef BREPGPU_MESHER_GEOMETRY_STORE_HPP
#define BREPGPU_MESHER_GEOMETRY_STORE_HPP
#include "exact_store.hpp"
#include "geometry.hpp"
#include "source.hpp"
#include <memory>

namespace bgmesh {
struct CurveGeometryRootTag;
struct SurfaceGeometryRootTag;
using CurveRootSlot = Slot<CurveGeometryRootTag>;
using SurfaceRootSlot = Slot<SurfaceGeometryRootTag>;

// Indices/ranges below are local to this retained GeometryStore's one immutable
// ExactTable. Materialize qualified ExactRefs only at a value/transfer
// boundary.
struct CurveGeometryRoot final {
  Slot<CurveTag> source;
  uint32_t dimension, degree;
  std::array<uint64_t, 2> bounds;
  Range<ExactEntry> controls;
};
struct SurfaceGeometryRoot final {
  Slot<CarrierTag> source;
  uint32_t p, q;
  std::array<uint64_t, 4> bounds;
  Range<ExactEntry> controls;
};
struct CurveGeometry final {
  CurveId source_id;
  uint32_t kind, dimension, degree;
  Range<ExactEntry> axis;
  Range<CurveGeometryRoot> roots;
};
struct CarrierGeometry final {
  CarrierId source_id;
  uint32_t kind, p, q;
  Range<ExactEntry> u_axis, v_axis;
  Range<SurfaceGeometryRoot> roots;
};
struct GeometryData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  ExactTable table;
  std::pmr::vector<CurveGeometry> curves;
  std::pmr::vector<CarrierGeometry> carriers;
  std::pmr::vector<CurveGeometryRoot> curve_roots;
  std::pmr::vector<SurfaceGeometryRoot> surface_roots;
  GeometryData(std::shared_ptr<RunResource>, SourceStore, ExactTable);
  GeometryData(const GeometryData &) = delete;
  GeometryData &operator=(const GeometryData &) = delete;
};
class GeometryStore final {
  std::shared_ptr<const GeometryData> data_;
  explicit GeometryStore(std::shared_ptr<const GeometryData> data) : data_(std::move(data)) {}
  friend GeometryStore compile_geometry(const SourceStore &, const std::shared_ptr<RunResource> &,
                                        ExactTableKey, bgfd::Budget &);

public:
  [[nodiscard]] const GeometryData &view() const noexcept { return *data_; }
  // Borrowed until this Store's table lease dies. Import synchronously, or
  // retain a Store/Table handle along with any submitted view; no naked view is
  // a lease.
  [[nodiscard]] ExactView exact(uint64_t local_index) const;
};

[[nodiscard]] GeometryStore compile_geometry(const SourceStore &,
                                             const std::shared_ptr<RunResource> &, ExactTableKey,
                                             bgfd::Budget &);
[[nodiscard]] CurveRootSlot find_curve_root(const GeometryStore &, Slot<CurveTag>,
                                            const Rational &);
[[nodiscard]] SurfaceRootSlot find_surface_root(const GeometryStore &, Slot<CarrierTag>,
                                                const Rational &u, const Rational &v);
[[nodiscard]] std::pmr::vector<SurfaceRootSlot>
// Full-coverage query: the entire closed bbox must lie in the original natural
// rectangle. Partial/full outside queries are unsupported, never clipped.
intersecting_surface_roots(bgfd::Budget &, const GeometryStore &, Slot<CarrierTag>,
                           const std::array<Rational, 4> &);
[[nodiscard]] BezierCurve import_curve_root(bgfd::Budget &, const GeometryStore &, CurveRootSlot);
[[nodiscard]] BezierSurface import_surface_root(bgfd::Budget &, const GeometryStore &,
                                                SurfaceRootSlot);
} // namespace bgmesh
#endif
