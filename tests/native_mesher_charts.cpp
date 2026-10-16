#include "../native/mesher/geometry_charts.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using bgmesh::Rational;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
Rational f(bgfd::Budget &b, int64_t n, int64_t d = 1) {
  return Rational(b, n).divide(Rational(b, d));
}
void family(const char *path, bool expected_plane) {
  const auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 27);
  const auto source = bgmesh::load_source(path, resource);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 35}, resource.get());
  const auto store =
      bgmesh::compile_geometry(source, resource, {{bgmesh::OwnerKind::geometry, 0, 125}, 0, 0}, b);
  bgmesh::GeometryContext context(b);
  bgmesh::FaceChartRequest request{.face = {0},
                                   .generation = 7,
                                   .positional_allowance = f(b, 1, 200),
                                   .node_source_allowance = f(b, 1, 1000)};
  const auto chart = bgmesh::choose_chart(context, store, request);
  require(chart.envelope.face.value == 0 && chart.envelope.carrier.value == 0 &&
              chart.envelope.generation == 7,
          "chosen chart retains original owners and face generation");
  const auto &bounds = chart.envelope.uv;
  require(bounds[0].compare(f(b, 0)) == 0 && bounds[1].compare(f(b, 1)) == 0 &&
              bounds[2].compare(f(b, 0)) == 0 && bounds[3].compare(f(b, 1)) == 0,
          "NURBS face envelope uses full original natural domain, not adapter hint");
  require(chart.envelope.uses.size() == 4, "complete original coedge envelope inventory");
  require((chart.kind == bgmesh::WorkingChartKind::physical_plane_convex_q) == expected_plane,
          "geometric plane profile accepts warped plane and rejects interior height bubble");
  if (expected_plane) {
    require(chart.plane.has_value() && chart.plane->height_upper.sign() == 0,
            "whole original plane height is exactly zero");
    const auto &q = chart.plane->quadrilateral;
    const std::array<int64_t, 8> expected{0, 0, 1, 0, 1, 1, 0, 1};
    for (std::size_t i = 0; i < q.size(); ++i)
      require(q[i].compare(f(b, expected[i])) == 0,
              "complete projected natural boundary is the exact convex square");
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 5, "expected warped-plane, bubble, fold and curved-boundary source fixtures");
    family(argv[1], true);
    family(argv[2], false);
    family(argv[3], false);
    family(argv[4], false);
    std::cout << "PASS\tchosen original-source charts\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
