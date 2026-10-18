// Minimal STEP -> .brep ingress for the NVIDIA arm: the steel-man ingress cost.
//
// NVIDIA's package reads only .brep (papers/2026-10-17-occt-ingress-translation-
// edits.md §5), so an OCCT user must first translate STEP. This is the least
// such a user runs to hand NVIDIA one .brep per solid definition, which the
// per-key accounting needs (G9 §0.2): STEPControl_Reader read, transfer of the
// roots, and one BinTools write per MANIFOLD_SOLID_BREP, named by its STEP
// entity label. No XDE document, names, colours, materials or properties, no
// hashing, and no unit, orientation or face-binding checks: the caller makes
// its checks after this process exits, outside NVIDIA's timed ingress. The
// shape-processing policy is the arm: none (OCCT translation without
// ShapeProcess, as our XDE exporter) or default (OCCT's default read).
//
// usage: brepgpu-nvidia-ingress --shape-processing none|default INPUT.stp NEW_DIRECTORY
//        brepgpu-nvidia-ingress --describe FILE.brep
// --describe prints a .brep file's topology counts, its face-edge uses with
// and without a pcurve, and every vertex point in map order, for the caller's
// geometry diff when NVIDIA's input differs from our XDE export.
// writes definition-<label>.brep, definitions.tsv (definition_id, filename,
// shape_type, identity_location, orientation: recorded after the timed phases
// for the caller to judge), phases.tsv (read, transfer, write seconds) and
// policy.tsv (the shape-processing flags as OCCT reports them after transfer).
#include <STEPControl_Reader.hxx>
#include <StepData_StepModel.hxx>
#include <StepShape_ManifoldSolidBrep.hxx>
#include <TransferBRep.hxx>
#include <Transfer_TransientProcess.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#include <BinTools.hxx>
#include <BinTools_ShapeSet.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <NCollection_IndexedMap.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS.hxx>
#include <Geom2d_Curve.hxx>
#include <gp_Pnt.hxx>
#include <BinTools_FormatVersion.hxx>
#include <TopoDS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
double seconds(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double>(b - a).count();
}
void require(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}
int describe(const char *path) {
  TopoDS_Shape shape;
  if (!BinTools::Read(shape, path)) {
    std::cerr << "cannot read " << path << '\n';
    return 1;
  }
  NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> faces, edges, vertices;
  TopExp::MapShapes(shape, TopAbs_FACE, faces);
  TopExp::MapShapes(shape, TopAbs_EDGE, edges);
  TopExp::MapShapes(shape, TopAbs_VERTEX, vertices);
  long uses = 0, with_pcurve = 0;
  for (int f = 1; f <= faces.Extent(); ++f) {
    const TopoDS_Face face = TopoDS::Face(faces(f));
    for (TopExp_Explorer e(face, TopAbs_EDGE); e.More(); e.Next()) {
      double first = 0, last = 0;
      ++uses;
      if (!BRep_Tool::CurveOnSurface(TopoDS::Edge(e.Current()), face, first, last).IsNull()) ++with_pcurve;
    }
  }
  std::cout << std::setprecision(17) << "faces\t" << faces.Extent() << "\nedges\t" << edges.Extent()
            << "\nvertices\t" << vertices.Extent() << "\nface_edge_uses\t" << uses
            << "\nface_edge_uses_with_pcurve\t" << with_pcurve << '\n';
  for (int v = 1; v <= vertices.Extent(); ++v) {
    const gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(vertices(v)));
    std::cout << "vertex\t" << p.X() << '\t' << p.Y() << '\t' << p.Z() << '\n';
  }
  return 0;
}
}  // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string(argv[1]) == "--describe") return describe(argv[2]);
  if (argc != 5 || std::string(argv[1]) != "--shape-processing" ||
      (std::string(argv[2]) != "none" && std::string(argv[2]) != "default")) {
    std::cerr << "usage: brepgpu-nvidia-ingress --shape-processing none|default INPUT.stp NEW_DIRECTORY\n";
    return 2;
  }
  try {
    const bool processing = std::string(argv[2]) == "default";
    const std::filesystem::path directory(argv[4]);
    require(std::filesystem::create_directory(directory), "output directory must be new");
    const auto begin = Clock::now();
    STEPControl_Reader reader;
    require(reader.ReadFile(argv[3]) == IFSelect_RetDone, "STEP read failed");
    // Same unit translation as the XDE exporter: model values in millimetres.
    reader.SetSystemLengthUnit(1.0);
    if (!processing) reader.SetShapeProcessFlags(ShapeProcess::OperationsFlags());
    const auto read_end = Clock::now();
    reader.TransferRoots();
    const auto transfer_end = Clock::now();
    auto model = reader.StepModel();
    auto process = reader.WS()->TransferReader()->TransientProcess();
    std::vector<std::pair<int, std::string>> written;
    std::vector<TopoDS_Shape> shapes;
    for (int i = 1; i <= model->NbEntities(); ++i) {
      auto solid = Handle(StepShape_ManifoldSolidBrep)::DownCast(model->Value(i));
      if (solid.IsNull()) continue;
      const TopoDS_Shape shape = TransferBRep::ShapeResult(process, solid);
      require(!shape.IsNull(), "untransferred solid");
      const int label = model->IdentLabel(solid);
      const std::string filename = "definition-" + std::to_string(label) + ".brep";
      BinTools_ShapeSet set;
      set.SetFormatNb(BinTools_FormatVersion_VERSION_4);
      set.SetWithTriangles(false);
      set.SetWithNormals(false);
      set.Add(shape);
      std::ofstream out(directory / filename, std::ios::binary);
      set.Write(out);
      set.Write(shape, out);
      out.close();
      require(bool(out), "definition write failed");
      written.emplace_back(label, filename);
      shapes.push_back(shape);
    }
    const auto write_end = Clock::now();
    // Outside the timed phases: what each written root is, for the caller.
    auto type = [](const TopoDS_Shape &shape) { return shape.ShapeType() == TopAbs_SOLID ? "solid" : "other"; };
    auto orientation = [](const TopoDS_Shape &shape) {
      return shape.Orientation() == TopAbs_FORWARD ? "forward"
             : shape.Orientation() == TopAbs_REVERSED ? "reversed" : "other";
    };
    std::ofstream definitions(directory / "definitions.tsv"), phases(directory / "phases.tsv"),
        policy(directory / "policy.tsv");
    definitions << "definition_id\tfilename\tshape_type\tidentity_location\torientation\n";
    for (std::size_t i = 0; i < written.size(); ++i)
      definitions << written[i].first << '\t' << written[i].second << '\t' << type(shapes[i]) << '\t'
                  << int(shapes[i].Location().IsIdentity()) << '\t' << orientation(shapes[i]) << '\n';
    phases << "phase\tseconds\n" << std::setprecision(17) << "read_step\t" << seconds(begin, read_end)
           << "\ntransfer_roots\t" << seconds(read_end, transfer_end) << "\nwrite_brep\t"
           << seconds(transfer_end, write_end) << '\n';
    const auto &flags = reader.GetShapeProcessFlags();
    policy << "field\tvalue\nshape_processing\t" << argv[2] << "\nflags_supplied\t" << flags.second
           << "\noperations\t" << flags.first.count() << "\noperation_bits\t" << flags.first.to_string() << '\n';
    for (auto *stream : {&definitions, &phases, &policy}) {
      stream->close();
      require(bool(*stream), "report write failed");
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "NVIDIA ingress failed: " << error.what() << '\n';
    return 1;
  }
}
