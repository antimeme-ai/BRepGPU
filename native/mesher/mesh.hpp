#ifndef BREPGPU_MESHER_MESH_HPP
#define BREPGPU_MESHER_MESH_HPP
#include "exact_store.hpp"
#include "mesh_values.hpp"
#include "source.hpp"
#include <variant>

namespace bgmesh {
// Wire/result rows are1-based;0 denotes an explicitly optional association.
// They are distinct from source slots and live mutable-topology handles.
template <class Tag> struct MeshRef final {
  uint64_t value = 0;
  auto operator<=>(const MeshRef &) const = default;
};
struct MeshDefinition;
struct MeshFace;
struct MeshNode;
struct MeshSite;
struct MeshLineage;
struct MeshEdge;
struct MeshEdgeSample;
struct MeshJoint;
struct MeshUse;
struct MeshUseSample;
struct MeshLoop;
struct MeshChart;
struct MeshDomainMap;
struct MeshSupport;
struct MeshProfile;
struct MeshCell;
struct MeshCellMap;
struct MeshCellFacet;
struct MeshFacet;
struct MeshCorner;
struct MeshQuotient;
struct MeshClosure;
struct MeshComponent;
struct MeshAppearance;
using MDef = MeshRef<MeshDefinition>;
using MFace = MeshRef<MeshFace>;
using MNode = MeshRef<MeshNode>;
using MSite = MeshRef<MeshSite>;
using MLineage = MeshRef<MeshLineage>;
using MEdge = MeshRef<MeshEdge>;
using MEdgeSample = MeshRef<MeshEdgeSample>;
using MJoint = MeshRef<MeshJoint>;
using MUse = MeshRef<MeshUse>;
using MUseSample = MeshRef<MeshUseSample>;
using MLoop = MeshRef<MeshLoop>;
using MChart = MeshRef<MeshChart>;
using MDomainMap = MeshRef<MeshDomainMap>;
using MSupport = MeshRef<MeshSupport>;
using MProfile = MeshRef<MeshProfile>;
using MCell = MeshRef<MeshCell>;
using MCellMap = MeshRef<MeshCellMap>;
using MCellFacet = MeshRef<MeshCellFacet>;
using MFacet = MeshRef<MeshFacet>;
using MCorner = MeshRef<MeshCorner>;
using MQuotient = MeshRef<MeshQuotient>;
using MClosure = MeshRef<MeshClosure>;
using MComponent = MeshRef<MeshComponent>;
using MAppearance = MeshRef<MeshAppearance>;
using MeshPoint2 = std::array<ExactRef, 2>;
struct MeshDefinition final {
  DefinitionId source;
  Range<MFace> faces;
  Range<MNode> nodes;
  Range<MSite> sites;
  Range<MFacet> facets;
};
struct MeshFace final {
  FaceId source;
  MDef definition;
  MChart chart;
  MDomainMap domain_map;
  uint64_t generation;
  Range<MLoop> loops;
  Range<MCell> cells;
  Range<MFacet> facets;
};
struct OriginalVertexGenerator final {
  VertexId vertex;
};
struct OriginalEdgeGenerator final {
  MEdge edge;
  ExactRef parameter;
};
struct FaceSiteGenerator final {
  MFace face;
  MSite site;
  MLineage lineage;
};
struct QuotientGenerator final {
  MQuotient quotient;
};
using MeshNodeGenerator = std::variant<OriginalVertexGenerator, OriginalEdgeGenerator,
                                       FaceSiteGenerator, QuotientGenerator>;
struct MeshNode final {
  MDef definition;
  Vec3 xyz;
  MeshNodeGenerator generator;
  MQuotient quotient;
};
using MeshSiteBranch = std::variant<std::monostate, MUse, MJoint>;
struct MeshSite final {
  MFace face;
  MChart chart;
  MeshPoint2 xy;
  MNode node;
  MeshSiteBranch branch;
  MLineage lineage;
};
enum class MeshLineageOperation : uint32_t {
  initial = 1,
  circumcenter = 2,
  midpoint = 3,
  optimization = 4
};
struct MeshLineage final {
  MFace face;
  uint64_t generation;
  MeshLineageOperation operation;
  Range<MLineage> parents;
  Range<MSite> parent_sites;
};
struct MeshEdge final {
  EdgeId source;
  MDef definition;
  uint64_t generation;
  Range<MEdgeSample> samples;
  Range<MComponent> components;
};
struct MeshEdgeSample final {
  MEdge edge;
  ExactRef parameter;
  MNode node;
  Range<uint32_t> interval_path;
};
enum class MeshLocatorKind : uint32_t {
  direct_original_uv = 1,
  projection_inverse = 2,
  analytic_branch = 3,
  unavailable_guide = 4
};
struct MeshLocator final {
  MeshLocatorKind kind = MeshLocatorKind::unavailable_guide;
  Vec2 chosen{};
  std::array<double, 4> enclosure{};
  MChart chart;
  std::array<int32_t, 2> lift{};
  MQuotient quotient;
  uint32_t unavailable_reason = 0;
};
struct MeshJoint final {
  MFace face;
  VertexId source_vertex;
  MChart chart;
  uint64_t branch;
  MSite site;
  Range<MUse> uses;
  MeshLocator recovery;
  MQuotient quotient;
};
struct MeshUse final {
  CoedgeId source;
  LoopId source_loop;
  MFace face;
  MEdge edge;
  int32_t sense;
  bool reconstructed;
  uint32_t parameter_map_kind;
  std::array<int32_t, 2> lift;
  Range<MUseSample> samples;
  Range<MProfile> profiles;
  Range<MComponent> components;
};
struct MeshUseSample final {
  MUse use;
  MEdgeSample edge_sample;
  MSite site;
  std::optional<ExactRef> pcurve_parameter;
  MeshLocator raw, recovery;
};
struct MeshLoop final {
  LoopId source;
  MFace face;
  bool outer;
  int32_t traversal_adapter;
  Range<MUse> uses;
  Range<MSite> normalized_sites;
  Range<MClosure> closures;
};
struct OriginalUVChart final {
  std::array<ExactRef, 4> rectangle;
};
struct UnwrappedUVChart final {
  std::array<ExactRef, 4> rectangle;
  std::array<int32_t, 2> lift;
};
struct PhysicalPlaneChart final {
  std::array<ExactRef, 4> rectangle;
  std::array<ExactRef, 6> projection;
  std::array<ExactRef, 4> preconditioner;
  std::array<ExactRef, 3> origin;
  std::array<ExactRef, 6> basis;
  std::array<ExactRef, 8> quadrilateral;
};
struct MeshChart final {
  CarrierId source;
  std::variant<OriginalUVChart, UnwrappedUVChart, PhysicalPlaneChart> chart;
};
enum class MeshReferencePolicy : uint32_t { authoritative_original = 1, hybrid_pullback = 2 };
struct MeshDomainMap final {
  MFace face;
  MeshReferencePolicy policy;
  Range<MSupport> fans, strips;
};
struct OriginalPcurvePoint final {
  MUse use;
  ExactRef parameter;
};
struct ProjectedPcurvePoint final {
  MChart chart;
  MUse use;
  ExactRef parameter;
};
struct NodalFanSupport final {
  std::variant<OriginalPcurvePoint, ProjectedPcurvePoint> source_point;
  MSite chosen;
  Range<MeshPoint2> polygon;
};
struct GraphStripSupport final {
  MUse use;
  std::array<ExactRef, 2> edge_interval;
  Range<MSupport> pre_fans;
  std::array<MSite, 2> endpoints;
  MeshPoint2 origin, x_covector, y_covector;
  ExactRef extent, c, w0;
};
struct MeshSupport final {
  MFace face;
  std::variant<NodalFanSupport, GraphStripSupport> support;
};
struct AffineParameterProfile final {};
struct GeometricLineProfile final {
  std::array<ExactRef, 3> covector;
  std::array<ExactRef, 2> scalar;
  std::array<MNode, 2> endpoints;
  int32_t traversal;
};
struct CylinderArcProfile final {
  std::array<ExactRef, 12> dual_frame;
  int32_t orientation, period_side;
  std::array<double, 8> endpoint_rays;
  MeshPoint2 halfplane_anchor;
};
struct MeshProfile final {
  MFace face;
  MUse use;
  CurveId source_curve;
  CarrierId source_carrier;
  std::array<ExactRef, 2> interval;
  std::variant<AffineParameterProfile, GeometricLineProfile, CylinderArcProfile> correspondence;
  Range<MComponent> components;
};
struct MeshCell final {
  MFace face;
  MChart chart;
  MLineage lineage;
  MCellMap map;
  Range<MCellFacet> facets;
  Range<MComponent> components;
};
enum class MeshPole : uint32_t { none = 0, sphere_north = 1, sphere_south = 2, cone_apex = 3 };
struct UVTriangleMap final {
  std::array<MSite, 3> sites;
};
struct PlaneTriangleMap final {
  std::array<MSite, 3> sites;
  MChart chart;
};
struct PoleFanMap final {
  MQuotient quotient;
  MeshPole pole;
  std::array<ExactRef, 2> angular_limits;
  std::array<MNode, 2> edge_nodes;
  std::array<ExactRef, 4> edge_uv;
  Range<MSite> cap_cut_chain;
  std::array<ExactRef, 2> xi, rho;
  uint64_t generation;
};
struct MeshCellMap final {
  MCell cell;
  std::variant<UVTriangleMap, PlaneTriangleMap, PoleFanMap> map;
};
enum class MeshInterpolant : uint32_t { barycentric_triangle = 1, pole_bilinear_fan = 2 };
struct MeshCellFacet final {
  MCell cell;
  MFacet facet;
  MeshInterpolant interpolant;
};
struct MeshFacet final {
  MFace face;
  std::array<MNode, 3> nodes;
  std::array<MCorner, 3> corners;
  MAppearance appearance;
};
struct MeshCorner final {
  MFace face;
  MSite site;
  CarrierId carrier;
  int32_t orientation;
  MeshLocator uv;
};
enum class MeshQuotientKind : uint32_t { joint = 1, seam = 2, pole = 3, apex = 4 };
struct MeshQuotient final {
  MFace face;
  CarrierId carrier;
  MeshQuotientKind kind;
  Range<VertexId> source_vertices;
  Range<MUse> uses;
  Range<MSite> sites;
  MNode node;
  Range<MCell> cells;
  MeshPole pole;
};
enum class MeshClosureKind : uint32_t { joint = 1, seam = 2, pole = 3, apex = 4 };
struct MeshClosure final {
  MFace face;
  LoopId loop;
  VertexId vertex;
  MUse incoming, outgoing;
  std::array<MSite, 2> endpoints;
  MeshClosureKind kind;
  MQuotient quotient;
  Range<MComponent> components;
};
enum class MeshComponentKind : uint32_t {
  definition_positional = 1,
  definition_node_source = 2,
  node_carrier_residual = 3,
  node_endpoint_curve_residual = 4,
  trace_position_residual = 5,
  chord_position_residual = 6,
  domain_displacement = 7,
  actual_cell_position = 8,
  pole_band_position = 9
};
enum class MeshValueRole : uint32_t { allowance = 1, canonical_upper = 2 };
struct MeshComponent final {
  std::variant<MDef, MFace, MEdge, MUse, MCell> owner;
  MeshComponentKind kind;
  MeshValueRole role;
  ExactRef value;
  MProfile profile;
};
struct MeshQuality final {
  double positional, node_source;
  uint32_t policy_revision = 1;
};
struct MeshInstance final {
  Range<UsageId> path;
  ProductId product;
  MDef definition;
  Frame matrix;
  Vec3 explosion;
  MAppearance appearance;
};
struct MeshAppearance final {
  StyleId source;
  Vec3 rgb;
  double roughness;
  ProductId label_product;
};
struct NativeMeshData final {
  std::shared_ptr<RunResource> resource;
  SourceStore source;
  std::pmr::vector<ExactTable> exact_tables;
  std::pmr::vector<MeshDefinition> definitions;
  std::pmr::vector<MeshFace> faces;
  std::pmr::vector<MeshNode> nodes;
  std::pmr::vector<MeshSite> sites;
  std::pmr::vector<MeshLineage> lineage;
  std::pmr::vector<MeshEdge> edges;
  std::pmr::vector<MeshEdgeSample> edge_samples;
  std::pmr::vector<MeshJoint> joints;
  std::pmr::vector<MeshUse> uses;
  std::pmr::vector<MeshUseSample> use_samples;
  std::pmr::vector<MeshLoop> loops;
  std::pmr::vector<MeshChart> charts;
  std::pmr::vector<MeshDomainMap> domain_maps;
  std::pmr::vector<MeshSupport> supports;
  std::pmr::vector<MeshProfile> profiles;
  std::pmr::vector<MeshCell> cells;
  std::pmr::vector<MeshCellMap> cell_maps;
  std::pmr::vector<MeshCellFacet> cell_facets;
  std::pmr::vector<MeshFacet> facets;
  std::pmr::vector<MeshCorner> corners;
  std::pmr::vector<MeshQuotient> quotients;
  std::pmr::vector<MeshClosure> closures;
  std::pmr::vector<MeshComponent> components;
  MeshQuality quality{};
  std::pmr::vector<MeshInstance> instances;
  std::pmr::vector<MeshAppearance> appearances;
  std::pmr::vector<MFace> face_refs;
  std::pmr::vector<MNode> node_refs;
  std::pmr::vector<MSite> site_refs;
  std::pmr::vector<MFacet> facet_refs;
  std::pmr::vector<MLoop> loop_refs;
  std::pmr::vector<MCell> cell_refs;
  std::pmr::vector<MLineage> lineage_refs;
  std::pmr::vector<MEdgeSample> edge_sample_refs;
  std::pmr::vector<MUse> use_refs;
  std::pmr::vector<MUseSample> use_sample_refs;
  std::pmr::vector<MProfile> profile_refs;
  std::pmr::vector<MComponent> component_refs;
  std::pmr::vector<MClosure> closure_refs;
  std::pmr::vector<MSupport> support_refs;
  std::pmr::vector<MCellFacet> cell_facet_refs;
  std::pmr::vector<MeshPoint2> points;
  std::pmr::vector<uint32_t> interval_paths;
  std::pmr::vector<VertexId> source_vertex_refs;
  std::pmr::vector<UsageId> usage_paths;
  NativeMeshData(std::shared_ptr<RunResource>, SourceStore);
  NativeMeshData(const NativeMeshData &) = delete;
  NativeMeshData &operator=(const NativeMeshData &) = delete;
};
class NativeMeshBuilder;
class NativeMesh final {
  std::shared_ptr<const NativeMeshData> data_;
  explicit NativeMesh(std::shared_ptr<const NativeMeshData> data) : data_(std::move(data)) {}
  friend class NativeMeshBuilder;

public:
  [[nodiscard]] const NativeMeshData &view() const noexcept { return *data_; }
};
class NativeMeshBuilder final {
  std::shared_ptr<NativeMeshData> data_;

public:
  NativeMeshBuilder(const std::shared_ptr<RunResource> &, SourceStore);
  NativeMeshBuilder(const NativeMeshBuilder &) = delete;
  NativeMeshBuilder &operator=(const NativeMeshBuilder &) = delete;
  NativeMeshBuilder(NativeMeshBuilder &&) noexcept = default;
  NativeMeshBuilder &operator=(NativeMeshBuilder &&) noexcept = default;
  [[nodiscard]] NativeMeshData &data();
  [[nodiscard]] NativeMesh seal();
  [[nodiscard]] NativeMesh seal(bgfd::Budget &);
};
} // namespace bgmesh
#endif
