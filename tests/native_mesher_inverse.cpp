#include "../native/mesher/geometry_inverse.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace {
using namespace bgmesh;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
void check(const std::filesystem::path &path) {
  const auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
  const auto source = load_source(path, resource);
  bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 36}, resource.get());
  const auto geometry =
      compile_geometry(source, resource, {{OwnerKind::geometry, 0, 140}, 0, 0}, budget);
  GeometryContext context(budget);
  const auto chart = choose_chart(
      context, geometry,
      {.face = {0},
       .generation = 1,
       .positional_allowance = Rational(budget, 1),
       .node_source_allowance = Rational(budget, 1).divide(Rational(budget, 1000000))});
  require(chart.plane.has_value(), "authored nonlinear plane admits actual inverse chart");
  const std::array<Rational, 2> xy{Rational(budget, 1).divide(Rational(budget, 2)),
                                   Rational(budget, 1).divide(Rational(budget, 4))};
  const auto inverse = recover_plane_chart_locator(context, chart, xy);
  // Independent POWER f(u)=3u/2-u²/2; no source evaluator or producer upper.
  const auto f = [&](const Rational &u) {
    return u.multiply(Rational(budget, 3)).subtract(u.multiply(u)).divide(Rational(budget, 2));
  };
  require(f(inverse.enclosure[0]).compare(xy[0]) <= 0 &&
              f(inverse.enclosure[1]).compare(xy[0]) >= 0 &&
              inverse.enclosure[2].compare(xy[1]) <= 0 && inverse.enclosure[3].compare(xy[1]) >= 0,
          "rigorous inverse enclosure brackets unique POWER root and exact v");
  require(inverse.enclosure[1]
                  .subtract(inverse.enclosure[0])
                  .compare(Rational(budget, 1).divide(Rational(budget, 1LL << 38))) < 0,
          "normalised explicit locator precision, independent of preceding cache work");
  static_cast<void>(context.pi(256));
  context.refine();
  const auto again = recover_plane_chart_locator(context, chart, xy);
  for (std::size_t i = 0; i < 2; ++i)
    require(inverse.chosen[i].compare(again.chosen[i]) == 0,
            "chosen inverse locator independent of context history");
  bool refused = false;
  try {
    static_cast<void>(recover_plane_chart_locator(
        context, chart, std::array<Rational, 2>{Rational(budget, 2), Rational(budget, 0)}));
  } catch (const Error &error) {
    refused = error.code == ErrorCode::unsupported;
  }
  require(refused, "outside-Q request never claims an original inverse");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected fixture directory");
    check(std::filesystem::path(argv[1]) / "warped-plane.bgsource");
    std::cout << "PASS\toriginal projected inverse locator\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  }
}
