// The STEP adapters' transfer under a non-millimetre global cascade unit.
// STEPCAFControl_Reader::Transfer takes the document's length unit, or the
// global xstep.cascade.unit for a document without one; the adapters must
// produce millimetres either way. Prints the largest absolute vertex
// coordinate of the transferred shapes, which the authored fixture states in
// millimetres.
#include "step_units.hpp"
#include <BRep_Tool.hxx>
#include <Interface_Static.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TDocStd_Document.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XSControl_WorkSession.hxx>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "usage: brepgpu-step-units-fixture CASCADE_UNIT INPUT.stp\n";
    return 2;
  }
  try {
    STEPCAFControl_Reader reader;
    // The reader's construction initialises the XSTEP statics; set the global
    // unit after it, as any other caller in the process could.
    if (!Interface_Static::SetCVal("xstep.cascade.unit", argv[1]))
      throw std::runtime_error("cannot set the global cascade unit");
    if (reader.ReadFile(argv[2]) != IFSelect_RetDone)
      throw std::runtime_error("STEP read failed");
    Handle(TDocStd_Document) doc;
    XCAFApp_Application::GetApplication()->NewDocument("BinXCAF", doc);
    brepgpu::step_units::prepare_millimetre_transfer(doc);
    if (!reader.Transfer(doc))
      throw std::runtime_error("STEP/XDE transfer failed");
    auto model = Handle(StepData_StepModel)::DownCast(reader.Reader().WS()->Model());
    double largest = 0;
    NCollection_Sequence<TDF_Label> labels;
    XCAFDoc_DocumentTool::ShapeTool(doc->Main())->GetFreeShapes(labels);
    for (int i = 1; i <= labels.Length(); ++i)
      for (TopExp_Explorer e(XCAFDoc_ShapeTool::GetShape(labels(i)), TopAbs_VERTEX); e.More();
           e.Next()) {
        const auto p = BRep_Tool::Pnt(TopoDS::Vertex(e.Current()));
        largest = std::max({largest, std::abs(p.X()), std::abs(p.Y()), std::abs(p.Z())});
      }
    std::cout << "vertex_max_abs\t" << std::setprecision(17) << largest << "\n";
    brepgpu::step_units::require_transferred_millimetre(doc, model);
    std::cout << "transfer_length_unit_mm\t1\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "step units fixture failed: " << e.what() << "\n";
    return 1;
  }
}
