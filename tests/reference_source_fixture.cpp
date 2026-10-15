// Authored source for the external-reference boundary: literal independent
// dimensions/placements, not a copy or a tessellation of the private watch.
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Builder.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <STEPControl_StepModelType.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <iostream>

int main(int argc,char** argv){
  if(argc!=2)return 2;
  Handle(TDocStd_Document) doc;
  XCAFApp_Application::GetApplication()->NewDocument("BinXCAF",doc);
  auto shapes=XCAFDoc_DocumentTool::ShapeTool(doc->Main());
  auto box=BRepPrimAPI_MakeBox(gp_Pnt(.125,.25,.5),1.,2.,3.).Shape();
  auto cylinder=BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(-1.5,2.25,.125),gp_Dir(0,0,1)),.75,2.).Shape();
  auto b=shapes->AddShape(box,false),c=shapes->AddShape(cylinder,false);
  TDataStd_Name::Set(b,"fixture-box");TDataStd_Name::Set(c,"fixture-cylinder");
  BRep_Builder builder;TopoDS_Compound compound;builder.MakeCompound(compound);
  auto root=shapes->AddShape(compound,true);TDataStd_Name::Set(root,"fixture-assembly");
  gp_Trsf identity,placement;placement.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(0,0,1)),1.57079632679489661923);
  placement.SetTranslationPart(gp_Vec(4.,-3.,2.));
  auto first=shapes->AddComponent(root,b,TopLoc_Location(identity));
  auto second=shapes->AddComponent(root,b,TopLoc_Location(placement));
  auto third=shapes->AddComponent(root,c,TopLoc_Location(identity));
  TDataStd_Name::Set(first,"first-box");TDataStd_Name::Set(second,"rotated-box");TDataStd_Name::Set(third,"cylinder");
  shapes->UpdateAssemblies();
  STEPCAFControl_Writer writer;writer.SetNameMode(true);
  if(!writer.Transfer(doc,STEPControl_AsIs)||writer.Write(argv[1])!=IFSelect_RetDone)return 1;
  return 0;
}
