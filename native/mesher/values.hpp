#ifndef BREPGPU_MESHER_VALUES_HPP
#define BREPGPU_MESHER_VALUES_HPP
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace bgmesh {
enum class ErrorCode { invalid_source, unsupported, resource_exhausted, unresolved, cancelled,
                       device_failure, io_failure, internal_error };
class Error final : public std::runtime_error {
public:
  ErrorCode code;
  Error(ErrorCode value, const std::string &message) : std::runtime_error(message), code(value) {}
};
template <class Tag> struct SourceId final {
  uint64_t value = 0;
  auto operator<=>(const SourceId &) const = default;
  [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
};
template <class Tag> struct Slot final {
  uint32_t value = 0;
  auto operator<=>(const Slot &) const = default;
};
template <class T> struct Range final {
  uint64_t first = 0, count = 0;
};
struct ProductTag;
struct DefinitionTag;
struct UsageTag;
struct VertexTag;
struct EdgeTag;
struct CoedgeTag;
struct LoopTag;
struct FaceTag;
struct CarrierTag;
struct CurveTag;
struct StyleTag;
using ProductId = SourceId<ProductTag>;
using DefinitionId = SourceId<DefinitionTag>;
using UsageId = SourceId<UsageTag>;
using VertexId = SourceId<VertexTag>;
using EdgeId = SourceId<EdgeTag>;
using CoedgeId = SourceId<CoedgeTag>;
using LoopId = SourceId<LoopTag>;
using FaceId = SourceId<FaceTag>;
using CarrierId = SourceId<CarrierTag>;
using CurveId = SourceId<CurveTag>;
using StyleId = SourceId<StyleTag>;
using Vec2 = std::array<double, 2>;
using Vec3 = std::array<double, 3>;
using Frame = std::array<double, 12>;
using SourceDigest = std::array<std::byte, 32>;

struct Product final {
  ProductId id;
  DefinitionId definition_id;
  StyleId style_id;
  Range<char> name, reference;
};
struct Definition final {
  DefinitionId id;
  ProductId product_id;
  Range<FaceId> faces;
  Range<EdgeId> edges;
  Range<VertexId> vertices;
};
struct Usage final {
  UsageId id;
  ProductId parent_product_id, child_product_id;
  StyleId style_id;
  Range<char> name;
  Frame transform;
};
struct Vertex final {
  VertexId id;
  Vec3 xyz;
  double tolerance;
};
struct Edge final {
  EdgeId id;
  std::array<VertexId, 2> vertex_ids;
  CurveId curve_id;
  Vec2 parameter_range;
  bool curve_same_sense, same_parameter, degenerate;
  double tolerance;
};
struct Coedge final {
  CoedgeId id;
  EdgeId edge_id;
  LoopId loop_id;
  CurveId curve_id;
  int32_t sense;
  bool reconstructed;
  Vec2 parameter_range;
  std::array<int32_t, 2> periodic_shift;
  uint32_t parameter_map_kind;
  Range<Vec2> parameter_map;
};
struct Loop final {
  LoopId id;
  FaceId face_id;
  bool outer;
  Range<CoedgeId> coedges;
};
struct Face final {
  FaceId id;
  DefinitionId definition_id;
  CarrierId carrier_id;
  StyleId style_id;
  int32_t orientation;
  double tolerance;
  Range<LoopId> loops;
};
struct SurfaceSpan final {
  std::array<double, 4> bounds;
  std::array<uint32_t, 2> degrees;
  Range<double> controls;
};
struct Carrier final {
  CarrierId id;
  uint32_t kind;
  Frame frame;
  Vec2 radii;
  std::array<double, 4> uv_bounds;
  std::array<uint32_t, 2> degrees, control_shape;
  Range<double> knot_u, knot_v, controls;
  Range<uint32_t> mult_u, mult_v;
  Range<SurfaceSpan> spans;
};
struct CurveSpan final {
  Vec2 parameter_range;
  uint32_t degree;
  Range<double> controls;
};
struct Curve final {
  CurveId id;
  uint32_t dimension, kind;
  Frame frame;
  Vec2 radii, parameter_range;
  uint32_t degree;
  Range<double> knots, controls;
  Range<uint32_t> multiplicities;
  Range<CurveSpan> spans;
};
struct Style final {
  StyleId id;
  Range<uint64_t> source_ids;
  std::array<double, 4> rgba;
  bool has_color, has_density;
  Range<char> material_name, density_unit;
  double density;
};
struct Diagnostic final {
  uint32_t severity;
  Range<char> stage, message;
  Range<uint64_t> source_ids;
};
} // namespace bgmesh
#endif
