#pragma once
#include <array>
#include <cstdint>
#include <vector>

class SmBrep;
class SmPolyBrep;
using Vec = std::array<double,3>;
struct Corner { Vec point,normal;uint64_t physical; };
struct Triangle { std::array<uint32_t,3> corners;uint64_t face; };
struct Mesh {
  std::vector<Corner> corners;
  std::vector<Triangle> triangles;
  uint64_t physical_count=0;
  uint64_t original_polygons=0,original_triangles=0,rectangle_count=0,official_split_calls=0;
  double official_split_seconds=0;
};

// Source Brep and its face labels must remain alive. On any exception the caller
// must discard this PolyBrep; a failed official mutation is never reusable.
Mesh extract(SmBrep &brep,SmPolyBrep &poly);
