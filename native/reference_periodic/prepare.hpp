#pragma once
#include "../source_uv/source_uv.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
class BinTools_ShapeSet;
namespace brepgpu {
struct PeriodicStage {
  std::string name;
  double seconds;
};
struct PeriodicHistory {
  std::string role;
  uint64_t source_index, prepared_index;
  double source_first = 0, source_last = 0, first = 0, last = 0, period = 0;
  int direction = 0;
  uint64_t order = 0, first_vertex = 0, last_vertex = 0;
  uint64_t source_first_vertex = 0, source_last_vertex = 0;
};
struct PeriodicTolerance {
  std::string role;
  uint64_t index;
  double original = 0, prepared = 0;
};
struct PeriodicRangeRestoration {
  uint64_t source_index, prepared_index;
  double old_first, old_last, first, last;
  bool old_same_range, old_same_parameter;
};
struct PreparedPeriodic {
  TopoDS_Shape shape;
  std::vector<PeriodicStage> stages;
  std::vector<PeriodicHistory> history;
  std::vector<PeriodicTolerance> tolerances;
  std::vector<PeriodicRangeRestoration> range_restorations;
  SourceUVStats raw_stats;
  uint64_t chart_approximations = 0, original_faces = 0, prepared_faces = 0;
  uint64_t original_shapes = 0, prepared_shapes = 0;
};
// Nominal OCCT API period, exact dyadic parameter words; no geometric sampler.
bool periodic_image_equal(double first, double last, double period,
                          std::span<const std::array<double, 2>> pieces,
                          std::span<const int> directions = {});
PreparedPeriodic prepare_periodic(const TopoDS_Shape &, const SourceUVOptions & = {},
                                  const BinTools_ShapeSet *source_indices = nullptr);
} // namespace brepgpu
