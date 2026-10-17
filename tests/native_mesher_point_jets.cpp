#include "../native/mesher/geometry_jets.hpp"
#include "../native/mesher/geometry_enclosure.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using bgmesh::Rational;
Rational f(bgfd::Budget &b, int64_t n, int64_t d = 1) {
  return Rational(b, n).divide(Rational(b, d));
}
void require(bool ok, const char *m) {
  if (!ok)
    throw std::runtime_error(m);
}
void exact(const bgmesh::ExactEnclosure &x, int64_t n, int64_t d = 1) {
  const auto expected = f(x.lower.budget(), n, d);
  require(x.lower.compare(expected) == 0 && x.upper.compare(expected) == 0,
          "original rational point jet equals exact quotient derivative");
}
void point_jets(const char *path) {
  const auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 26);
  const auto source = bgmesh::load_source(path, resource);
  bgfd::Budget b({uint64_t{1} << 27, uint64_t{1} << 34}, resource.get());
  const auto store =
      bgmesh::compile_geometry(source, resource, {{bgmesh::OwnerKind::geometry, 0, 126}, 0, 0}, b);
  bgmesh::GeometryContext context(b);
  // Original source S=(u,2v/(1+v),2uv/(1+v)).
  const auto jet = bgmesh::enclose_original_surface_point_jet(
      context, store, bgmesh::Slot<bgmesh::CarrierTag>{0}, f(b, 1, 4), f(b, 1, 2));
  exact(jet.position[0], 1, 4);
  exact(jet.position[1], 2, 3);
  exact(jet.position[2], 1, 6);
  exact(jet.du[0], 1);
  exact(jet.du[2], 2, 3);
  exact(jet.dv[1], 8, 9);
  exact(jet.dv[2], 2, 9);
  exact(jet.duu[0], 0);
  exact(jet.duv[2], 8, 9);
  exact(jet.dvv[1], -32, 27);
  exact(jet.dvv[2], -8, 27);
  const auto tie = bgmesh::enclose_original_surface_point_jet(
      context, store, bgmesh::Slot<bgmesh::CarrierTag>{1}, f(b, 1, 2), f(b, 1, 2));
  exact(tie.du[0], 1);
  exact(tie.du[2], 1, 2);
  exact(tie.duv[2], 1);
  const auto image=bgmesh::enclose_original_curve_range(context,store,bgmesh::Slot<bgmesh::CurveTag>{0},{f(b,0),f(b,1)});
  require(image.dimension==2 && image.coordinates[0].lower.sign()==0 && image.coordinates[1].lower.sign()==0 &&
          image.coordinates[0].upper.compare(f(b,2))==0 && image.coordinates[1].upper.compare(f(b,2))==0,
          "position-only original curve image covers both exact C0 source spans");
  const auto roots=bgmesh::intersecting_curve_roots(b,store,bgmesh::Slot<bgmesh::CurveTag>{0},{f(b,1,2),f(b,1,2)});
  require(roots.size()==2,"closed curve source interval enumerates both touching roots");
  bool outside=false;
  try {static_cast<void>(bgmesh::enclose_original_curve_range(context,store,bgmesh::Slot<bgmesh::CurveTag>{0},{f(b,-1,4),f(b,1,4)}));}
  catch(const bgmesh::Error &e){outside=e.code==bgmesh::ErrorCode::unsupported;}
  require(outside,"position-only range must reject partial natural-domain clipping");
  const std::array<bgmesh::ExactEnclosure,3> vector{bgmesh::ExactEnclosure(f(b,-3),f(b,3)),bgmesh::ExactEnclosure(f(b,4)),bgmesh::ExactEnclosure(f(b,0))};
  require(bgmesh::geometry_detail::enclosure_norm_upper(b,vector,96).compare(f(b,5))==0,
          "shared exact enclosure norm uses coordinate extrema and exact square root");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected original geometry source fixture");
    point_jets(argv[1]);
    std::cout << "PASS\toriginal point jets\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << "FAIL\t" << e.what() << '\n';
    return 1;
  }
}
