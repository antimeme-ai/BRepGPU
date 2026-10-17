#include "../native/mesher/geometry_charts.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using bgmesh::Rational;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
Rational f(bgfd::Budget &b, int64_t n, int64_t d = 1) {
  return Rational(b, n).divide(Rational(b, d));
}
void check(const std::filesystem::path &path, bool bubble, bool mapped) {
  const auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 27);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 35}, resource.get());
  const auto source = bgmesh::load_source(path, resource);
  const auto store =
      bgmesh::compile_geometry(source, resource, {{bgmesh::OwnerKind::geometry, 0, 127}, 0, 0}, b);
  bgmesh::GeometryContext context(b);
  const auto env =
      bgmesh::prepare_definition_envelope(context, store, {.definition = {0}, .generation = 9});
  require(env.definition.value == 0 && env.generation == 9 && env.faces.size() == 1,
          "definition envelope retains complete original face inventory");
  require(env.xyz[0].sign() == 0 && env.xyz[1].compare(f(b, 1)) == 0 && env.xyz[2].sign() == 0 &&
              env.xyz[3].compare(f(b, 1)) == 0 && env.xyz[4].sign() == 0,
          "complete source box covers original vertices/edges/natural carrier");
  require(bubble ? (env.xyz[5].compare(f(b, 1, 16)) >= 0 && env.xyz[5].compare(f(b, 1, 4)) <= 0)
                 : env.xyz[5].sign() == 0,
          "whole carrier interior contributes to source envelope even with flat boundary");
  const auto &face = env.faces[0];
  require(face.uv[0].sign() == 0 && face.uv[1].compare(f(b, 1)) == 0 && face.uv[2].sign() == 0 &&
              face.uv[3].compare(f(b, 1)) == 0 && face.uses.size() == 4,
          "natural NURBS rectangle and every original coedge retained");
  if (mapped) {
    require(face.uses[0].mapped_parameter_range[0].compare(f(b, 1, 4)) == 0 &&
                face.uses[0].mapped_parameter_range[1].compare(f(b, 3, 4)) == 0 &&
                face.uses[0].uv[1].compare(f(b, 3, 4)) == 0,
            "piecewise map envelope includes interior extrema, not only mapped endpoints");
  }
}
void reject(const std::filesystem::path &path,
            bgmesh::ErrorCode expected = bgmesh::ErrorCode::unsupported) {
  const auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 27);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 35}, resource.get());
  const auto source = bgmesh::load_source(path, resource);
  const auto store =
      bgmesh::compile_geometry(source, resource, {{bgmesh::OwnerKind::geometry, 0, 128}, 0, 0}, b);
  bgmesh::GeometryContext context(b);
  bool rejected = false;
  try {
    static_cast<void>(
        bgmesh::prepare_face_envelope(context, store, {.face = {0}, .generation = 0}));
  } catch (const bgmesh::Error &e) {
    rejected = e.code == expected;
  }
  require(
      rejected,
      "unproved continuation or absent NURBS periodic metadata cannot define a source envelope");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected authored chart source fixture directory");
    const std::filesystem::path path(argv[1]);
    check(path / "warped-plane.bgsource", false, false);
    check(path / "interior-bubble.bgsource", true, false);
    check(path / "mapped-guide.bgsource", false, true);
    reject(path / "unsupported-map.bgsource");
    reject(path / "unsupported-nurbs-lift.bgsource");
    reject(path / "cone-apex-zero.bgsource");
    reject(path / "cone-apex-uncertain.bgsource", bgmesh::ErrorCode::unresolved);
    std::cout << "PASS\toriginal face and definition envelopes\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
