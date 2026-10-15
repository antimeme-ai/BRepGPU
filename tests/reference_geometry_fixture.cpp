// Authored analytic references only; no watch/source data or OCCT triangles.
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BinTools.hxx>
#include <BinTools_FormatVersion.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <Geom2d_Curve.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>
#include <filesystem>
#include <iostream>
namespace fs=std::filesystem;
int main(int argc,char **argv){
  try{
    if(argc!=2)return 2;
    fs::path out=argv[1];if(fs::exists(out))return 2;
    fs::create_directories(out);
    auto sphere=BRepPrimAPI_MakeSphere(1.).Shape();
    auto missing=BRepPrimAPI_MakeSphere(1.).Shape();
    BRep_Builder builder;
    for(TopExp_Explorer faces(missing,TopAbs_FACE);faces.More();faces.Next()){
      auto face=TopoDS::Face(faces.Current());
      for(TopExp_Explorer edges(face,TopAbs_EDGE);edges.More();edges.Next())
        builder.UpdateEdge(TopoDS::Edge(edges.Current()),Handle(Geom2d_Curve)(),face,1e-7);
    }
    auto box=BRepPrimAPI_MakeBox(gp_Pnt(-2.,-2.,0.),4.,4.,1.).Shape();
    auto outer=BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(0,0,0),gp_Dir(0,0,1)),2.))).Wire();
    auto inner=BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(0,0,0),gp_Dir(0,0,1)),1.))).Wire();
    inner.Reverse();
    BRepBuilderAPI_MakeFace face(gp_Pln(gp_Pnt(0,0,0),gp_Dir(0,0,1)),outer);
    face.Add(inner);
    auto annulus=BRepPrimAPI_MakePrism(face.Face(),gp_Vec(0,0,1)).Shape();
    for(const auto &item:std::initializer_list<std::pair<const char*,TopoDS_Shape>>{
        {"sphere",sphere},{"sphere-missing",missing},{"box",box},{"annulus",annulus}}){
      auto path=out/(std::string(item.first)+".brep");
      if(!BinTools::Write(item.second,path.c_str(),false,false,BinTools_FormatVersion_VERSION_4))return 1;
    }
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
