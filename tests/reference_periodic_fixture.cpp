#include "../native/reference_periodic/prepare.hpp"
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepLib.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BinTools_ShapeSet.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Plane.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <gp_Pln.hxx>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool yes, const char *message) {
  if (!yes)
    throw std::runtime_error(message);
}
TopoDS_Shape authored(bool cone, double phase, bool reflected_axis, bool gap) {
  const gp_Ax3 frame(gp_Pnt(.125, -.25, .5), reflected_axis ? gp_Dir(0, 0, -1) : gp_Dir(0, 0, 1),
                     gp_Dir(0, 1, 0));
  Handle(Geom_Surface) surface;
  if (cone)
    surface = new Geom_ConicalSurface(frame, .2, .5);
  else
    surface = new Geom_CylindricalSurface(frame, .5);
  const auto side =
      BRepBuilderAPI_MakeFace(surface, phase, phase + 2 * std::acos(-1.), 0, 1, 1.e-7).Face();
  BRepBuilderAPI_Sewing sewing(1.e-7);
  sewing.Add(side);
  unsigned circles = 0;
  for (TopExp_Explorer it(side, TopAbs_EDGE); it.More(); it.Next()) {
    const auto edge = TopoDS::Edge(it.Current());
    double first = 0, last = 0;
    const auto curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull())
      continue;
    GeomAdaptor_Curve adaptor(curve);
    if (adaptor.GetType() != GeomAbs_Circle)
      continue;
    const auto circle = adaptor.Circle();
    auto cap = BRepBuilderAPI_MakeFace(gp_Pln(circle.Position()),
                                       BRepBuilderAPI_MakeWire(edge).Wire(), true)
                   .Face();
    sewing.Add(cap);
    ++circles;
  }
  require(circles == 2, "authored truncated carrier must have two circle rims");
  sewing.Perform();
  const auto sewn = sewing.SewedShape();
  require(sewn.ShapeType() == TopAbs_SHELL, "authored periodic shell missing");
  auto solid = BRepBuilderAPI_MakeSolid(TopoDS::Shell(sewn)).Solid();
  require(BRepLib::OrientClosedSolid(solid), "authored solid orientation failed");
  if (gap) {
    TopExp_Explorer vertex(solid, TopAbs_VERTEX);
    require(vertex.More(), "authored seam vertex absent");
    const auto v = TopoDS::Vertex(vertex.Current());
    const auto p = BRep_Tool::Pnt(v);
    BRep_Builder builder;
    builder.UpdateVertex(v, gp_Pnt(p.X() + 1.e-5, p.Y(), p.Z()), 2.e-5);
  }
  return solid;
}
void write(const TopoDS_Shape &shape, const std::filesystem::path &path) {
  BinTools_ShapeSet set;
  set.SetFormatNb(BinTools_FormatVersion_VERSION_4);
  set.SetWithTriangles(false);
  set.SetWithNormals(false);
  set.Add(shape);
  std::ofstream stream(path, std::ios::binary);
  set.Write(stream);
  set.Write(shape, stream);
  stream.close();
  require(!stream.fail(), "authored BRep write failed");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "fixture requires new output directory");
    const std::filesystem::path output = argv[1];
    require(!std::filesystem::exists(output) && std::filesystem::create_directory(output),
            "fixture output must be new");
    const double p = 2 * std::acos(-1.);
    const std::array<std::array<double, 2>, 2> rotated{{{p / 2, p}, {p, p * 1.5}}};
    require(brepgpu::periodic_image_equal(0, p, p, rotated), "cyclic full-image identity rejected");
    const std::array<int, 2> reversed_cycle{{-1, -1}};
    require(!brepgpu::periodic_image_equal(0, p, p, rotated, reversed_cycle),
            "reversed full cycle accepted");
    const std::array<std::array<double, 2>, 2> overlap{{{0, p / 2}, {0, p / 2}}};
    require(!brepgpu::periodic_image_equal(0, p, p, overlap), "cyclic multiplicity loss accepted");
    const std::array<std::array<double, 2>, 1> partial{{{.25, .75}}};
    require(brepgpu::periodic_image_equal(.25, .75, p, partial), "partial image identity rejected");
    require(!brepgpu::periodic_image_equal(.25, .875, p, partial), "partial image loss accepted");
    const std::array<std::array<double, 2>, 1> linear{{{-.5, 1.25}}};
    require(brepgpu::periodic_image_equal(-.5, 1.25, 0, linear), "linear image identity rejected");
    struct Case {
      const char *name;
      bool cone;
      double phase;
      bool reflected;
      bool gap;
    };
    const std::array<Case, 6> cases{{{"cylinder-zero", false, 0, false, false},
                                     {"cylinder-shift", false, p / 2, false, true},
                                     {"cylinder-reverse", false, -p / 2, true, false},
                                     {"cone-zero", true, 0, false, false},
                                     {"cone-shift", true, p / 2, false, true},
                                     {"cone-reverse", true, -p / 2, true, false}}};
    for (const auto &item : cases)
      write(authored(item.cone, item.phase, item.reflected, item.gap),
            output / (std::string(item.name) + ".brep"));
    write(BRepPrimAPI_MakeCylinder(.5, 1, p / 4).Shape(), output / "cylinder-partial.brep");
    auto bad_vertex = authored(false, 0, false, false);
    TopExp_Explorer vertex(bad_vertex, TopAbs_VERTEX);
    const auto v = TopoDS::Vertex(vertex.Current());
    const auto q = BRep_Tool::Pnt(v);
    BRep_Builder builder;
    builder.UpdateVertex(v, gp_Pnt(q.X() + .01, q.Y(), q.Z()), .01);
    write(bad_vertex, output / "invalid-vertex.brep");
    auto bad_radius = authored(false, 0, false, false);
    for (TopExp_Explorer face(bad_radius, TopAbs_FACE); face.More(); face.Next()) {
      const auto f = TopoDS::Face(face.Current());
      auto cylinder = Handle(Geom_CylindricalSurface)::DownCast(BRep_Tool::Surface(f));
      if (!cylinder.IsNull()) {
        auto replacement = Handle(Geom_CylindricalSurface)::DownCast(cylinder->Copy());
        replacement->SetRadius(.6);
        builder.UpdateFace(f, replacement, TopLoc_Location(), BRep_Tool::Tolerance(f));
        break;
      }
    }
    write(bad_radius, output / "invalid-radius.brep");
    const std::array<std::array<double, 2>, 1> winding{{{0, 2 * p}}};
    require(!brepgpu::periodic_image_equal(0, 2 * p, p, winding), "multi-turn profile accepted");
    std::cout << "authored_sources\t" << cases.size() + 3 << '\n';
    return 0;
  } catch (const Standard_Failure &error) {
    std::cerr << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
  }
  return 1;
}
