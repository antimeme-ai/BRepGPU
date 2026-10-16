#ifndef BG_OM10_TYPES_HPP
#define BG_OM10_TYPES_HPP
#include <cstdint>
namespace bgom10 {
constexpr uint32_t PROJECT_ONLY = 1, DUMP_PROJECTED = 2, DUMP_SAMPLES = 4;
constexpr uint32_t SOURCE_GEOMETRY_PROFILE = 1;
constexpr uint32_t STORED_XYZ = 1, REVERSE_NORMAL = 2, ANALYTIC_NORMAL_LIMIT = 4,
                   NEGATIVE_RADIAL_SHEET = 8, SPHERE_SOUTH = 16, COFACTOR_NORMAL = 32;
constexpr uint32_t SINGULAR_PARAMETER = 1, COFACTOR_INSTANCE_NORMAL = 2;
struct Vec {
  double x, y, z;
};
struct Definition {
  uint64_t id, first_vertex, vertex_count, first_triangle, triangle_count;
};
struct Instance {
  uint64_t id;
  uint32_t definition, reserved;
  double matrix[12], explosion[3];
};
struct Vertex {
  double xyz[3], uv[2];
  uint32_t carrier, flags;
  uint64_t physical;
};
struct Triangle {
  uint32_t vertex[3], material;
  uint64_t face;
};
struct Carrier {
  uint64_t id;
  uint32_t tag, flags;
  double origin[3], x[3], y[3], z[3], r0, r1, uv[4];
  uint64_t control_offset, control_count;
  uint32_t p, q;
  uint64_t reserved;
};
struct Material {
  double rgb[3], roughness;
};
struct Control {
  double h[4];
};
struct VertexRef {
  uint32_t source, instance;
};
struct PrimitiveRef {
  uint32_t triangle, instance, vertex_base, reserved;
};
struct Request {
  uint32_t width, height, supersample, reserved;
  double alpha, basis[9], center[3], pixel_center[2], scale, background[3];
  uint32_t flags, reserved2;
  double reserved3[2];
};
struct Jet {
  double position[3], du[3], dv[3], normal[3], used[3];
  uint32_t status, flags;
};
struct Projected {
  uint64_t instance, definition, source_vertex;
  int64_t x, y;
  double depth, world[3], normal[3];
};
struct Sample {
  double depth;
  uint64_t primitive, instance;
};
struct Prepared {
  int64_t x[3], y[3];
  double z[3];
  uint32_t vertex[3], material;
  uint64_t primitive, instance;
  int64_t area;
  int32_t xmin, xmax, ymin, ymax;
};
struct SceneView {
  const Definition *definitions;
  uint64_t ndefinitions;
  const Instance *instances;
  uint64_t ninstances;
  const Vertex *vertices;
  uint64_t nvertices;
  const Triangle *triangles;
  uint64_t ntriangles;
  const Carrier *carriers;
  uint64_t ncarriers;
  const Material *materials;
  uint64_t nmaterials;
  const Control *controls;
  uint64_t ncontrols;
  const VertexRef *vertex_refs;
  uint64_t nplaced_vertices;
  const PrimitiveRef *primitive_refs;
  uint64_t nprimitives;
};
struct FrameOutput {
  uint8_t *rgba;
  double *depth;
  uint64_t *primitive;
  uint64_t *instance;
  Projected *projected;
  Sample *samples;
};
struct Error {
  char stage[48], message[384];
  int64_t row;
  int32_t native_code;
};
struct Times {
  double create = 0, upload = 0, evaluate = 0, project = 0, setup = 0, bins = 0,
         display = 0, download = 0, close = 0;
};
struct Api {
  uint32_t version, size;
  bool (*create)(const SceneView *, uint32_t, int32_t, void **, Error *,
                 Times *);
  bool (*evaluate)(void *, Jet *, Error *, Times *);
  bool (*render)(void *, const Request *, FrameOutput *, Error *, Times *);
  bool (*close)(void *, Error *, Times *);
};
static_assert(sizeof(Request) == 192 && sizeof(Carrier) == 192 &&
                  sizeof(Vertex) == 56,
              "native record layout");
static_assert(sizeof(Jet) == 128 && sizeof(Projected) == 96 &&
                  sizeof(Sample) == 24 && sizeof(Prepared) == 128,
              "output layout");
const Api *cpu_api();
const Api *cpu_api_v2();
} // namespace bgom10
#endif
