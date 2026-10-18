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
#include <gp_Pln.hxx>
#include <gp_Cylinder.hxx>
#include <BinTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <NCollection_IndexedMap.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include "../native/reference_source/orientation_verdict.hpp"
#include "../native/shape_extent.hpp"
#include <TopAbs.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <Geom_Plane.hxx>
#include <gp_Pln.hxx>
#include <iomanip>
#include <iostream>
#include <map>
#include <cstdlib>
#include <BRepBuilderAPI_Copy.hxx>
#include <vector>
#include <cmath>
#include <array>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Iterator.hxx>
#include <typeinfo>
#include <STEPControl_Reader.hxx>
#include <ShapeProcess.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <TransferBRep.hxx>
#include <StepData_StepModel.hxx>
#include <StepShape_ManifoldSolidBrep.hxx>
#include <StepShape_BrepWithVoids.hxx>
#include <StepShape_OrientedClosedShell.hxx>
#include <StepShape_ConnectedFaceSet.hxx>
#include <StepShape_AdvancedFace.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <gp_Lin.hxx>
#include <string>
#include <BRepCheck_Result.hxx>
#include <BRepCheck_ListOfStatus.hxx>

int main(int argc,char** argv){
  // Validity and enclosed volume of one exported definition: BRepCheck's
  // verdict per face (subject index in face map order) and for the whole solid,
  // and the volume by Gauss integration of the exact faces. An independent
  // reader of the exported bytes, never of the exporter's own state.
  // The exporter's orientation verdict (native/reference_source/
  // orientation_verdict.hpp) on authored configurations of an OCCT box, one per
  // verdict, built from known material: for each face as used in the solid,
  // repaired (OCCT's use is the box's, the raw use the opposite), turned
  // (OCCT's use opposite the box's, the raw use the box's), reversed together
  // (the raw surface reversed with the use), and undecidable (the raw surface
  // moved off the face).
  if(argc==2&&std::string(argv[1])=="--verdicts"){
    namespace ro=brepgpu::reference_orientation;
    const TopoDS_Shape solid=BRepPrimAPI_MakeBox(gp_Pnt(.125,.25,.5),1.,2.,3.).Solid();int k=0;
    for(TopExp_Explorer e(solid,TopAbs_FACE);e.More();e.Next(),++k){const TopoDS_Face face=TopoDS::Face(e.Current());const auto o=face.Orientation();
      const double s=o==TopAbs_REVERSED?-1.:1.;const Handle(Geom_Surface) surface=BRep_Tool::Surface(face);
      std::cout<<"face "<<k<<" repaired "<<ro::face_verdict({{face,o}},surface,-s,solid)<<"\n";
      std::cout<<"face "<<k<<" turned "<<ro::face_verdict({{face,TopAbs::Reverse(o)}},surface,s,solid)<<"\n";
      std::cout<<"face "<<k<<" together "<<ro::face_verdict({{face,o}},surface->UReversed(),-s,solid)<<"\n";
      std::cout<<"face "<<k<<" moved "<<ro::face_verdict({{face,o}},Handle(Geom_Surface)::DownCast(surface->Translated(gp_Vec(1.,1.,1.))),s,solid)<<"\n";}
    return 0;
  }
  // face_verdict's multi-piece branches on constructed pieces of known material:
  // the faces lying on the plane z=1 of a fused pair of boxes, taken as the
  // pieces of one original face whose raw surface is that plane.
  // side-by-side: [0,1]x[0,1]x[0,1] and [1,2]x[0,1]x[0,1], two top faces with
  //   material below: repaired (raw normal -z, OCCT as in the solid), a piece
  //   left inward (one piece's use turned), reversed together (raw +z), turned
  //   against the shell (raw +z, one piece turned);
  // offset: [0,1.5]x[0,1]x[0,1] and [1,2]x[0,1]x[1,2], a top face (material
  //   below) and a bottom face (material above) on z=1: the raw +z side is
  //   outward on one piece and into the material on the other.
  if(argc==2&&std::string(argv[1])=="--verdict-pieces"){
    namespace ro=brepgpu::reference_orientation;
    const Handle(Geom_Surface) plane=new Geom_Plane(gp_Pln(gp_Pnt(0,0,1),gp_Dir(0,0,1)));
    auto pieces_on_plane=[](const TopoDS_Shape& solid){std::vector<std::pair<TopoDS_Face,TopAbs_Orientation>> out;
      for(TopExp_Explorer e(solid,TopAbs_FACE);e.More();e.Next()){BRepAdaptor_Surface s(TopoDS::Face(e.Current()));
        if(s.GetType()==GeomAbs_Plane&&std::abs(std::abs(s.Plane().Axis().Direction().Z())-1)<1e-12&&std::abs(s.Plane().Location().Z()-1)<1e-12)out.push_back({TopoDS::Face(e.Current()),e.Current().Orientation()});}
      return out;};
    auto fused=[](const TopoDS_Shape& a,const TopoDS_Shape& b){BRepAlgoAPI_Fuse f(a,b);TopExp_Explorer e(f.Shape(),TopAbs_SOLID);return e.Current();};
    const TopoDS_Shape side=fused(BRepPrimAPI_MakeBox(gp_Pnt(0,0,0),gp_Pnt(1,1,1)).Shape(),BRepPrimAPI_MakeBox(gp_Pnt(1,0,0),gp_Pnt(2,1,1)).Shape());
    const auto two=pieces_on_plane(side);
    if(two.size()!=2){std::cerr<<"side-by-side: "<<two.size()<<" pieces on z=1\n";return 1;}
    auto turned_one=two;turned_one[1].second=TopAbs::Reverse(turned_one[1].second);
    std::cout<<"repaired "<<ro::face_verdict(two,plane,-1.,side)<<"\n";
    std::cout<<"left_inward "<<ro::face_verdict(turned_one,plane,-1.,side)<<"\n";
    std::cout<<"together "<<ro::face_verdict(two,plane,1.,side)<<"\n";
    std::cout<<"turned "<<ro::face_verdict(turned_one,plane,1.,side)<<"\n";
    const TopoDS_Shape offset=fused(BRepPrimAPI_MakeBox(gp_Pnt(0,0,0),gp_Pnt(1.5,1,1)).Shape(),BRepPrimAPI_MakeBox(gp_Pnt(1,0,1),gp_Pnt(2,1,2)).Shape());
    const auto mixed=pieces_on_plane(offset);
    if(mixed.size()!=2){std::cerr<<"offset: "<<mixed.size()<<" pieces on z=1\n";return 1;}
    std::cout<<"mixed "<<ro::face_verdict(mixed,plane,1.,offset)<<"\n";
    return 0;
  }
  // The exporter's verification of a definition with corrected sides
  // (orientation_verdict.hpp verify_solid), and with --statuses BRepCheck's
  // failing statuses by shape type on the same pcurve-completed copy.
  // --pieces INPUT.stp FACE...: what the exporter's face-side verdict sees for
  // each original face: OCCT's default-read pieces of it, each with its
  // surface type, area, orientation in the processed solid, the sample point,
  // whether OCCT's outward normal there equals or opposes the raw use's, the
  // probe offset, how many of the seven rays from each side's probe have odd
  // parity, OCCT's classifier state of each probe, and raw_use_side's verdict;
  // and whether the raw face's outer bound contradicts it (S1).
  if(argc>=4&&std::string(argv[1])=="--pieces"){
    namespace ro=brepgpu::reference_orientation;
    STEPControl_Reader processed,raw;
    for(auto* r:{&processed,&raw})if(r->ReadFile(argv[2])!=IFSelect_RetDone)return 1;
    processed.SetSystemLengthUnit(1.0);raw.SetSystemLengthUnit(1.0);raw.SetShapeProcessFlags(ShapeProcess::OperationsFlags());
    processed.TransferRoots();raw.TransferRoots();
    const auto pm=processed.StepModel(),rm=raw.StepModel();const auto ptp=processed.WS()->TransferReader()->TransientProcess(),rtp=raw.WS()->TransferReader()->TransientProcess();
    std::map<int,Handle(Standard_Transient)> pe;for(int i=1;i<=pm->NbEntities();++i)if(const int l=pm->IdentLabel(pm->Value(i));l>0)pe[l]=pm->Value(i);
    std::cout<<std::setprecision(10);
    for(int i=1;i<=rm->NbEntities();++i){auto solid=Handle(StepShape_ManifoldSolidBrep)::DownCast(rm->Value(i));if(solid.IsNull()||solid->Outer().IsNull())continue;
      const int si=rm->IdentLabel(solid);const TopoDS_Shape P=pe.count(si)?TransferBRep::ShapeResult(ptp,pe.at(si)):TopoDS_Shape();
      std::vector<std::pair<Handle(StepShape_ConnectedFaceSet),bool>> shells;Handle(StepShape_ConnectedFaceSet) outer=solid->Outer();
      if(auto o=Handle(StepShape_OrientedClosedShell)::DownCast(outer);!o.IsNull())outer=o->ClosedShellElement();
      shells.push_back({outer,true});
      if(auto v=Handle(StepShape_BrepWithVoids)::DownCast(solid);!v.IsNull())for(int k=1;k<=v->NbVoids();++k)shells.push_back({v->VoidsValue(k)->ClosedShellElement(),v->VoidsValue(k)->Orientation()});
      for(const auto& [set,forward]:shells)for(int k=1;k<=set->NbCfsFaces();++k){auto f=Handle(StepShape_AdvancedFace)::DownCast(set->CfsFacesValue(k));if(f.IsNull())continue;const int fi=rm->IdentLabel(f);
        bool want=false;for(int a=3;a<argc;++a)want|=std::stoi(argv[a])==fi;if(!want)continue;
        try{
        const auto t=TransferBRep::ShapeResult(rtp,f);const TopoDS_Shape r=pe.count(fi)?TransferBRep::ShapeResult(ptp,pe.at(fi)):TopoDS_Shape();
        int nshells=0;if(!P.IsNull())for(TopExp_Explorer e(P,TopAbs_SHELL);e.More();e.Next())++nshells;
        std::cout<<"face "<<fi<<" solid "<<si<<" processed "<<(P.IsNull()?"none":TopAbs::ShapeTypeToString(P.ShapeType()))<<" shells "<<nshells
                 <<" bound_contradicts "<<(t.IsNull()?-2:ro::bound_contradicts(TopoDS::Face(t)))<<" raw_surface "<<(t.IsNull()?"-":BRep_Tool::Surface(TopoDS::Face(t))->DynamicType()->Name())<<"\n";
        if(t.IsNull()||r.IsNull()||P.IsNull())continue;
        const double sign=f->SameSense()==forward?1.:-1.;const auto rs=BRep_Tool::Surface(TopoDS::Face(t));
        std::vector<std::pair<TopoDS_Face,TopAbs_Orientation>> pieces;
        for(TopExp_Explorer e(r,TopAbs_FACE);e.More();e.Next()){const TopoDS_Face piece=TopoDS::Face(e.Current());TopAbs_Orientation in_solid=TopAbs_EXTERNAL;
          for(TopExp_Explorer q(P,TopAbs_FACE);q.More();q.Next())if(q.Current().IsSame(piece))in_solid=q.Current().Orientation();
          pieces.push_back({piece,in_solid});
          GProp_GProps area;BRepGProp::SurfaceProperties(piece,area);BRepAdaptor_Surface type(piece);
          const auto smp=ro::sample(piece,rs,sign);
          std::cout<<"  piece "<<pieces.size()<<" type "<<int(type.GetType())<<" area "<<area.Mass()<<" in_solid "<<TopAbs::ShapeOrientationToString(in_solid)<<" wires "<<[&]{int n=0;for(TopExp_Explorer w(piece,TopAbs_WIRE);w.More();w.Next())++n;return n;}();
          if(!smp.sampled){std::cout<<" unsampled\n";continue;}
          gp_Vec occt=smp.occt_surface_normal;if(in_solid==TopAbs_REVERSED)occt.Reverse();
          Bnd_Box pb;BRepBndLib::Add(piece,pb);const double offset=std::max(1e-4*std::sqrt(pb.SquareExtent()),10*std::max(BRep_Tool::Tolerance(piece),Precision::Confusion()));
          std::cout<<" point "<<smp.point.X()<<","<<smp.point.Y()<<","<<smp.point.Z()<<" near "<<smp.near<<" dot "<<occt.Dot(smp.raw_normal)<<" offset "<<offset;
          try{IntCurvesFace_ShapeIntersector crossings;crossings.Load(P,Precision::Confusion());
          for(int side=0;side<2;++side){const gp_Pnt probe=smp.point.Translated(smp.raw_normal*(side==0?offset:-offset));int odd=0;
            for(int k=0;k<7;++k){const double z=1-2*(k+0.5)/7,rr=std::sqrt(1-z*z),phi=2.399963229728653*k+0.5;crossings.Perform(gp_Lin(probe,gp_Dir(rr*std::cos(phi),rr*std::sin(phi),z)),0.,Precision::Infinite());odd+=crossings.NbPnt()%2;}
            BRepClass3d_SolidClassifier c(P,probe,Precision::Confusion());
            std::cout<<(side==0?" along":" against")<<" odd "<<odd<<"/7 state "<<(c.State()==TopAbs_IN?"IN":c.State()==TopAbs_OUT?"OUT":c.State()==TopAbs_ON?"ON":"UNKNOWN");}}catch(const Standard_Failure&){std::cout<<" probe failed";}
          std::cout<<" side "<<ro::raw_use_side(piece,rs,sign,P)<<"\n";}
        std::cout<<"  verdict "<<ro::face_verdict(pieces,rs,sign,P)<<"\n";
        }catch(const Standard_Failure& failure){std::cout<<"  failed "<<typeid(failure).name()<<" "<<failure.what()<<"\n";}}}
    return 0;
  }
  // --diagonals FILE: OCCT's box diagonal of one exported solid (BRepBndLib,
  // what UR-7 used before B2 .37.39) and native/shape_extent.hpp's.
  // --wire2d FILE: on the verification copy, each face, whether its wires
  // close in the parameter plane and BRepCheck passes it, with its edge uses'
  // 2D ends in stored order.
  // --seams [--completed] FILE: each seam use of each face, as the face uses it: the 3D
  // direction of its traversal at its middle and the u of its pcurve there,
  // u relative to the face's smallest seam u (so two reads that place the
  // same pairing a period apart compare equal).
  if(argc>=3&&std::string(argv[1])=="--seams"){
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[argc-1]))return 1;
    // --completed: on the verification copy (a seam without pcurves gets them).
    if(argc==4&&std::string(argv[2])=="--completed")shape=brepgpu::reference_orientation::with_pcurves(shape);
    int n=0;
    for(TopExp_Explorer f(shape,TopAbs_FACE);f.More();f.Next(),++n){const TopoDS_Face face=TopoDS::Face(f.Current());
      std::vector<std::pair<std::array<long,3>,double>> uses;
      for(TopExp_Explorer e(face,TopAbs_EDGE);e.More();e.Next()){const TopoDS_Edge edge=TopoDS::Edge(e.Current());if(!BRep_Tool::IsClosed(edge,face))continue;
        double a=0,b=0;const auto pc=BRep_Tool::CurveOnSurface(edge,face,a,b);if(pc.IsNull())continue;
        BRepAdaptor_Curve curve(edge);gp_Pnt p;gp_Vec d;curve.D1((curve.FirstParameter()+curve.LastParameter())/2,p,d);if(edge.Orientation()==TopAbs_REVERSED)d.Reverse();d.Normalize();
        uses.push_back({{std::lround(d.X()*1e6),std::lround(d.Y()*1e6),std::lround(d.Z()*1e6)},pc->Value((a+b)/2).X()});}
      if(uses.empty())continue;double lo=uses[0].second;for(const auto& u:uses)lo=std::min(lo,u.second);
      for(const auto& u:uses)std::cout<<"seam "<<n<<" "<<u.first[0]<<" "<<u.first[1]<<" "<<u.first[2]<<" "<<std::lround((u.second-lo)*1e6)<<"\n";}
    return 0;
  }
  if(argc==3&&std::string(argv[1])=="--wire2d"){
    namespace ro=brepgpu::reference_orientation;
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[2]))return 1;
    const TopoDS_Shape copy=ro::with_pcurves(shape);int n=0;
    for(TopExp_Explorer f(copy,TopAbs_FACE);f.More();f.Next(),++n){const TopoDS_Face face=TopoDS::Face(f.Current());BRepCheck_Analyzer check(face);
      std::cout<<"face "<<n<<" "<<(face.Orientation()==TopAbs_REVERSED?"REVERSED":"FORWARD")<<" closes "<<ro::wires_close_2d(face)<<" valid "<<check.IsValid()<<"\n";
      for(TopExp_Explorer w(face,TopAbs_WIRE);w.More();w.Next())for(TopoDS_Iterator it(w.Current());it.More();it.Next()){const TopoDS_Edge e=TopoDS::Edge(it.Value());
        double a=0,b=0;if(BRep_Tool::CurveOnSurface(e,face,a,b).IsNull()){std::cout<<"  edge without pcurve\n";continue;}
        const gp_Pnt2d p0=ro::use_end(e,face,true),p1=ro::use_end(e,face,false);
        std::cout<<"  edge "<<(e.Orientation()==TopAbs_REVERSED?"R":"F")<<" seam "<<BRep_Tool::IsClosed(e,face)<<" "<<p0.X()<<","<<p0.Y()<<" -> "<<p1.X()<<","<<p1.Y()<<"\n";}}
    return 0;
  }
  // --extent INPUT.stp: for each solid of OCCT's translation without
  // ShapeProcess, how far a box misses the solid: the placement extent the
  // exporter uses (native/shape_extent.hpp shape_bound) and, for comparison,
  // OCCT's box of the pcurve-completed copy (round 1's extent). The solid's
  // points: OCCT's optimal box of its default read's processed solid and 65
  // samples along each of its raw edges (the review's diag/extent oracle).
  if(argc==3&&std::string(argv[1])=="--extent"){
    namespace ro=brepgpu::reference_orientation;
    STEPControl_Reader processed,raw;
    for(auto* r:{&processed,&raw})if(r->ReadFile(argv[2])!=IFSelect_RetDone)return 1;
    processed.SetSystemLengthUnit(1.0);raw.SetSystemLengthUnit(1.0);raw.SetShapeProcessFlags(ShapeProcess::OperationsFlags());
    processed.TransferRoots();raw.TransferRoots();
    const auto pm=processed.StepModel(),rm=raw.StepModel();const auto ptp=processed.WS()->TransferReader()->TransientProcess(),rtp=raw.WS()->TransferReader()->TransientProcess();
    std::map<int,Handle(Standard_Transient)> pe;for(int i=1;i<=pm->NbEntities();++i)if(const int l=pm->IdentLabel(pm->Value(i));l>0)pe[l]=pm->Value(i);
    auto miss=[](const Bnd_Box& box,const Bnd_Box& truth)->double{if(box.IsWhole()||box.IsOpen())return 0.;if(box.IsVoid())return INFINITY;double a[6],b[6];box.Get(a[0],a[1],a[2],a[3],a[4],a[5]);truth.Get(b[0],b[1],b[2],b[3],b[4],b[5]);
      double out=0;for(int k=0;k<3;++k){out=std::max(out,a[k]-b[k]);out=std::max(out,b[k+3]-a[k+3]);}return out;};
    std::cout<<std::setprecision(6);
    for(int i=1;i<=rm->NbEntities();++i){auto solid=Handle(StepShape_ManifoldSolidBrep)::DownCast(rm->Value(i));if(solid.IsNull())continue;const int si=rm->IdentLabel(solid);
      const TopoDS_Shape r=TransferBRep::ShapeResult(rtp,solid),p=pe.count(si)?TransferBRep::ShapeResult(ptp,pe.at(si)):TopoDS_Shape();if(r.IsNull())continue;
      Bnd_Box truth;if(!p.IsNull())BRepBndLib::AddOptimal(p,truth,false,false);
      for(TopExp_Explorer e(r,TopAbs_EDGE);e.More();e.Next()){if(!BRep_Tool::IsGeometric(TopoDS::Edge(e.Current())))continue;BRepAdaptor_Curve c(TopoDS::Edge(e.Current()));
        for(int k=0;k<=64;++k)truth.Add(c.Value(c.FirstParameter()+(c.LastParameter()-c.FirstParameter())*k/64.));}
      const Bnd_Box bound=brepgpu::shape_extent::shape_bound(r);Bnd_Box copy;try{BRepBndLib::Add(ro::with_pcurves(r),copy,false);}catch(const Standard_Failure&){copy.SetWhole();}
      std::cout<<"solid "<<si<<" bound_miss_mm "<<miss(bound,truth)<<" bound_whole "<<(bound.IsWhole()||bound.IsOpen())<<" copy_miss_mm "<<miss(copy,truth)<<"\n";}
    return 0;
  }
  // --classify FILE: where a definition's verification fails on the completed
  // copy, whether each failing face is OCCT's own translation (no pcurve added
  // to it; BRepCheck fails on the face as translated) or one the completion
  // touched; and whether the shell or solid fails beyond its faces.
  if(argc==3&&std::string(argv[1])=="--classify"){
    namespace ro=brepgpu::reference_orientation;
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[2]))return 1;
    const TopoDS_Shape copy=ro::with_pcurves(shape);
    NCollection_IndexedMap<TopoDS_Shape,TopTools_ShapeMapHasher> raw_faces,copy_faces;TopExp::MapShapes(shape,TopAbs_FACE,raw_faces);TopExp::MapShapes(copy,TopAbs_FACE,copy_faces);
    if(raw_faces.Extent()!=copy_faces.Extent())return 1;
    int failing=0,untouched=0,untouched_raw_fails=0,touched=0;
    for(int i=1;i<=copy_faces.Extent();++i){const TopoDS_Face raw=TopoDS::Face(raw_faces(i)),done=TopoDS::Face(copy_faces(i));
      if(BRepCheck_Analyzer(done).IsValid())continue;++failing;
      int missing=0;for(TopExp_Explorer e(raw,TopAbs_EDGE);e.More();e.Next()){double a=0,b=0;if(BRep_Tool::CurveOnSurface(TopoDS::Edge(e.Current()),raw,a,b).IsNull())++missing;}
      if(missing){++touched;continue;}++untouched;if(!BRepCheck_Analyzer(raw).IsValid())++untouched_raw_fails;}
    // Each failing edge use: was its pcurve on that face added by the copy, and
    // which surface type is the face.
    {BRepCheck_Analyzer check(copy);std::map<std::string,int> uses;
     for(int i=1;i<=copy_faces.Extent();++i){const TopoDS_Face raw=TopoDS::Face(raw_faces(i)),done=TopoDS::Face(copy_faces(i));
       const int type=int(BRepAdaptor_Surface(done,false).GetType());
       TopExp_Explorer re(raw,TopAbs_EDGE);for(TopExp_Explorer e(done,TopAbs_EDGE);e.More()&&re.More();e.Next(),re.Next()){
         const auto& result=check.Result(e.Current());if(result.IsNull())continue;
         for(result->InitContextIterator();result->MoreShapeInContext();result->NextShapeInContext()){if(!result->ContextualShape().IsSame(done))continue;
           for(const auto status:result->StatusOnShape())if(status!=BRepCheck_NoError){double a=0,b=0;const bool added=BRep_Tool::CurveOnSurface(TopoDS::Edge(re.Current()),raw,a,b).IsNull();
             ++uses["edge_use status "+std::to_string(int(status))+" surface "+std::to_string(type)+(added?" added":" existing")];}}}
       const auto& face=check.Result(done);if(!face.IsNull())for(const auto status:face->Status())if(status!=BRepCheck_NoError){int missing=0;for(TopExp_Explorer e(raw,TopAbs_EDGE);e.More();e.Next()){double a=0,b=0;if(BRep_Tool::CurveOnSurface(TopoDS::Edge(e.Current()),raw,a,b).IsNull())++missing;}
         ++uses["face status "+std::to_string(int(status))+" surface "+std::to_string(type)+(missing?" with added pcurves":" untouched")];}}
     for(const auto& u:uses)std::cout<<u.first<<" "<<u.second<<"\n";}
    // Each face whose wires do not close on the copy: does re-pairing its
    // existing seam pcurves (which the copy never does) close them, and do its
    // wires close on OCCT's own translation where no pcurve was added.
    {int open=0,repairs=0,untouched_open=0;
     for(int i=1;i<=copy_faces.Extent();++i){const TopoDS_Face done=TopoDS::Face(copy_faces(i));if(ro::wires_close_2d(done))continue;++open;
       int missing=0;const TopoDS_Face raw=TopoDS::Face(raw_faces(i));for(TopExp_Explorer e(raw,TopAbs_EDGE);e.More();e.Next()){double a=0,b=0;if(BRep_Tool::CurveOnSurface(TopoDS::Edge(e.Current()),raw,a,b).IsNull())++missing;}
       if(!missing)++untouched_open;
       {BRepAdaptor_Surface sf(done,false);const double up=sf.IsUPeriodic()?sf.UPeriod():0,vp=sf.IsVPeriodic()?sf.VPeriod():0;double worst=0;
        for(TopExp_Explorer w(done,TopAbs_WIRE);w.More();w.Next()){const auto edges=ro::traversal(w.Current());for(std::size_t k=0;k<edges.size();++k){double a=0,b=0;
          if(BRep_Tool::CurveOnSurface(edges[k],done,a,b).IsNull()||BRep_Tool::CurveOnSurface(edges[(k+1)%edges.size()],done,a,b).IsNull())continue;
          worst=std::max(worst,ro::use_end(edges[k],done,false).Distance(ro::use_end(edges[(k+1)%edges.size()],done,true)));}}
        std::cout<<"open_face surface "<<int(sf.GetType())<<" added "<<missing<<" worst_gap "<<worst<<" u_period "<<up<<" v_period "<<vp<<" brepcheck "<<BRepCheck_Analyzer(done).IsValid()<<"\n";
        if(std::getenv("BREPGPU_FIXTURE_TRAVERSAL")&&up>0)for(TopExp_Explorer w(done,TopAbs_WIRE);w.More();w.Next()){std::cout<<"  wire "<<(w.Current().Orientation()==TopAbs_REVERSED?"R":"F")<<"\n";
          for(const auto& e:ro::traversal(w.Current())){double a=0,b=0;const bool had=!BRep_Tool::CurveOnSurface(e,raw,a,b).IsNull();
            std::cout<<"    "<<(had?"had":"added")<<" seam "<<BRep_Tool::IsClosed(e,done)<<" closed3d "<<BRep_Tool::IsClosed(e)<<" u/2pi "<<std::lround(ro::use_end(e,done,true).X()/up*100)/100.<<" -> "<<std::lround(ro::use_end(e,done,false).X()/up*100)/100.<<"\n";}}}
       const TopoDS_Face probe=TopoDS::Face(BRepBuilderAPI_Copy(raw).Shape());if(ro::pair_seams(probe))++repairs;}
     std::cout<<"open_faces "<<open<<" untouched "<<untouched_open<<" closed_by_repairing_existing_seams "<<repairs<<"\n";}
    BRepCheck_Analyzer whole(copy);
    std::cout<<"faces_failing "<<failing<<" untouched "<<untouched<<" untouched_failing_as_translated "<<untouched_raw_fails<<" touched "<<touched<<" solid_valid "<<whole.IsValid()<<" verify "<<(ro::verify_solid(shape).empty()?"ok":ro::verify_solid(shape))<<"\n";
    return 0;
  }
  // --tolerances FILE: what the verification copy did to each edge's
  // tolerance: edges with no added pcurve and how many kept OCCT's tolerance
  // exactly, edges with an added pcurve and how many were raised, and the
  // largest tolerance raised to.
  if(argc==3&&std::string(argv[1])=="--tolerances"){
    namespace ro=brepgpu::reference_orientation;
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[2]))return 1;
    ro::Completion report;ro::with_pcurves(shape,&report);
    int plain=0,kept=0,added=0,raised=0,lowered=0;
    for(const auto& e:report.edges){if(e.added){++added;if(e.after>e.before)++raised;if(e.after<e.before)++lowered;}else{++plain;if(e.after==e.before)++kept;}}
    std::cout<<std::setprecision(17)<<"edges_without_added "<<plain<<" kept "<<kept<<" edges_with_added "<<added<<" raised "<<raised<<" lowered "<<lowered<<" largest_raised_tolerance_mm "<<report.largest_raised_tolerance_mm<<"\n";
    return 0;
  }
  if(argc==3&&std::string(argv[1])=="--diagonals"){
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[2]))return 1;
    Bnd_Box occt;BRepBndLib::Add(shape,occt,false);
    std::cout<<std::setprecision(17)<<"occt_diagonal "<<(occt.IsVoid()?0.:std::sqrt(occt.SquareExtent()))<<"\nshape_extent_diagonal "<<brepgpu::shape_extent::shape_diagonal(shape)<<"\n";
    for(TopExp_Explorer f(shape,TopAbs_FACE);f.More();f.Next()){const TopoDS_Face face=TopoDS::Face(f.Current());const Bnd_Box b=brepgpu::shape_extent::face_bound(face);int edges=0;for(TopExp_Explorer e(face,TopAbs_EDGE);e.More();e.Next())++edges;
      std::cout<<"face surface "<<int(BRepAdaptor_Surface(face,false).GetType())<<" edges "<<edges<<" bounded "<<!(b.IsWhole()||b.IsOpen()||b.IsVoid())<<"\n";}
    return 0;
  }
  if(argc>=3&&std::string(argv[1])=="--verify"){
    namespace ro=brepgpu::reference_orientation;
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[argc-1]))return 1;
    const std::string failure=ro::verify_solid(shape);
    std::cout<<"verify "<<(failure.empty()?"ok":failure)<<"\n";
    {GProp_GProps properties;BRepGProp::VolumeProperties(ro::with_pcurves(shape),properties);std::cout<<std::setprecision(17)<<"completed_volume "<<properties.Mass()<<"\n";}
    if(argc==4&&std::string(argv[2])=="--statuses"){
      const TopoDS_Shape checked=ro::with_pcurves(shape);BRepCheck_Analyzer analyzer(checked);
      std::map<std::string,int> counts;
      for(auto type:{TopAbs_VERTEX,TopAbs_EDGE,TopAbs_WIRE,TopAbs_FACE,TopAbs_SHELL,TopAbs_SOLID}){
        NCollection_IndexedMap<TopoDS_Shape,TopTools_ShapeMapHasher> all;TopExp::MapShapes(checked,type,all);
        for(int i=1;i<=all.Extent();++i){const auto& result=analyzer.Result(all(i));if(result.IsNull())continue;
          for(const auto status:result->Status())if(status!=BRepCheck_NoError)++counts[std::string(TopAbs::ShapeTypeToString(type))+" "+std::to_string(int(status))];
          for(result->InitContextIterator();result->MoreShapeInContext();result->NextShapeInContext())
            for(const auto status:result->StatusOnShape())if(status!=BRepCheck_NoError)++counts[std::string(TopAbs::ShapeTypeToString(type))+" in "+TopAbs::ShapeTypeToString(result->ContextualShape().ShapeType())+" "+std::to_string(int(status))];}}
      for(const auto& c:counts)std::cout<<"status "<<c.first<<" "<<c.second<<"\n";
    }
    return 0;
  }
  if(argc==3&&std::string(argv[1])=="--check"){
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[2]))return 1;
    std::cout<<std::setprecision(17);
    NCollection_IndexedMap<TopoDS_Shape,TopTools_ShapeMapHasher> faces;TopExp::MapShapes(shape,TopAbs_FACE,faces);
    BRepCheck_Analyzer analyzer(shape);
    int invalid=0;
    for(int i=1;i<=faces.Extent();++i){BRepCheck_Analyzer face(faces(i));if(!face.IsValid())++invalid;std::cout<<"face "<<i<<" "<<(face.IsValid()?1:0)<<"\n";}
    int shells=0;for(TopExp_Explorer e(shape,TopAbs_SHELL);e.More();e.Next())++shells;
    GProp_GProps properties;BRepGProp::VolumeProperties(shape,properties);
    std::cout<<"faces "<<faces.Extent()<<"\nshells "<<shells<<"\ninvalid_faces "<<invalid<<"\nvalid "<<(analyzer.IsValid()?1:0)<<"\nvolume "<<properties.Mass()<<"\n";
    return 0;
  }
  if(argc==3&&std::string(argv[1])=="--inspect"){
    TopoDS_Shape shape;
    if(!BinTools::Read(shape,argv[2]))return 1;
    std::cout<<std::setprecision(17);
    if(shape.ShapeType()!=TopAbs_SOLID||!shape.Location().IsIdentity()||shape.Orientation()!=TopAbs_FORWARD)return 1;
    for(TopExp_Explorer e(shape,TopAbs_VERTEX);e.More();e.Next()){
      auto p=BRep_Tool::Pnt(TopoDS::Vertex(e.Current()));
      std::cout<<"vertex "<<p.X()<<" "<<p.Y()<<" "<<p.Z()<<"\n";
    }
    for(TopExp_Explorer e(shape,TopAbs_FACE);e.More();e.Next()){
      auto face=TopoDS::Face(e.Current());TopLoc_Location location;
      if(!BRep_Tool::Triangulation(face,location).IsNull())return 1;
      BRepAdaptor_Surface surface(face);
      int sign=face.Orientation()==TopAbs_FORWARD?1:-1;
      if(surface.GetType()==GeomAbs_Plane){
        auto plane=surface.Plane();auto p=plane.Location();auto n=plane.Axis().Direction();
        std::cout<<"plane "<<sign<<" "<<p.X()<<" "<<p.Y()<<" "<<p.Z()<<" "<<n.X()<<" "<<n.Y()<<" "<<n.Z()<<"\n";
      }else if(surface.GetType()==GeomAbs_Cylinder){
        auto c=surface.Cylinder();auto p=c.Location();auto n=c.Axis().Direction();
        std::cout<<"cylinder "<<sign<<" "<<p.X()<<" "<<p.Y()<<" "<<p.Z()<<" "<<n.X()<<" "<<n.Y()<<" "<<n.Z()<<" "<<c.Radius()<<"\n";
      }else return 1;
    }
    for(TopExp_Explorer e(shape,TopAbs_EDGE);e.More();e.Next()){
      TopLoc_Location location;
      if(!BRep_Tool::Polygon3D(TopoDS::Edge(e.Current()),location).IsNull())return 1;
    }
    return 0;
  }
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
