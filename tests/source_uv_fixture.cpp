// Literal native fixtures for the actual source converter, never a watch model.
#include "source_uv.hpp"
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BinTools.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shell.hxx>
#include <TopExp_Explorer.hxx>
#include <ShapeExtend_WireData.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom_Plane.hxx>
#include <Geom_BSplineCurve.hxx>
#include <NCollection_Array2.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <gp_Ax2d.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <NCollection_Array1.hxx>
#include <gp_Ax3.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Vec2d.hxx>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace brepgpu;
const double pi=std::acos(-1.);
SourceUVUse use(SourceID id,SourceID edge_id,const TopoDS_Edge& edge,SourceID a,SourceID b,const TopoDS_Vertex& start,const TopoDS_Vertex& end){return {id,edge_id,a,b,edge,start,end};}
SourceUVFace octant(){
 BRep_Builder builder;auto a=BRepBuilderAPI_MakeVertex(gp_Pnt(2.5,0,0)).Vertex(),b=BRepBuilderAPI_MakeVertex(gp_Pnt(0,2.5,0)).Vertex(),n=BRepBuilderAPI_MakeVertex(gp_Pnt(0,0,2.5)).Vertex();
 Handle(Geom_Curve) equator=new Geom_Circle(gp_Circ(gp_Ax2(gp_Pnt(),gp_Dir(0,0,1),gp_Dir(1,0,0)),2.5));
 Handle(Geom_Curve) yz=new Geom_Circle(gp_Circ(gp_Ax2(gp_Pnt(),gp_Dir(-1,0,0),gp_Dir(0,0,1)),2.5));
 Handle(Geom_Curve) xz=new Geom_Circle(gp_Circ(gp_Ax2(gp_Pnt(),gp_Dir(0,-1,0),gp_Dir(1,0,0)),2.5));
 auto e0=BRepBuilderAPI_MakeEdge(equator,a,b,0,pi/2).Edge(),e1=TopoDS::Edge(BRepBuilderAPI_MakeEdge(yz,n,b,0,pi/2).Edge().Reversed()),e2=TopoDS::Edge(BRepBuilderAPI_MakeEdge(xz,a,n,0,pi/2).Edge().Reversed());
 TopoDS_Wire wire;builder.MakeWire(wire);for(auto e:{e0,e1,e2})builder.Add(wire,e);TopoDS_Face face;builder.MakeFace(face,new Geom_SphericalSurface(gp_Ax3(),2.5),1.e-7);builder.Add(face,wire);
 return {1,2,face,{{3,wire,{use(10,20,e0,100,101,a,b),use(11,21,e1,101,102,b,n),use(12,22,e2,102,100,n,a)}}}};
}
SourceUVFace reversed_cylinder(){
 BRep_Builder builder;double r=.345,a=pi,b=3*pi,h=1.15;auto p=BRepBuilderAPI_MakeVertex(gp_Pnt(-r,0,0)).Vertex(),q=BRepBuilderAPI_MakeVertex(gp_Pnt(-r,0,h)).Vertex();Handle(Geom_Curve) bottom=new Geom_Circle(gp_Circ(gp_Ax2(gp_Pnt(),gp_Dir(0,0,1)),r)),top=new Geom_Circle(gp_Circ(gp_Ax2(gp_Pnt(0,0,h),gp_Dir(0,0,1)),r));Handle(Geom_Curve) axial=new Geom_Line(gp_Pnt(-r,0,0),gp_Dir(0,0,1));auto e0=BRepBuilderAPI_MakeEdge(bottom,p,p,a,b).Edge(),e1=BRepBuilderAPI_MakeEdge(axial,p,q,0,h).Edge(),e2=TopoDS::Edge(BRepBuilderAPI_MakeEdge(top,q,q,a,b).Edge().Reversed()),e3=TopoDS::Edge(e1.Reversed());TopoDS_Wire stored;builder.MakeWire(stored);for(auto e:{e0,e1,e2,e3})builder.Add(stored,e);stored.Reverse();TopoDS_Face face;builder.MakeFace(face,new Geom_CylindricalSurface(gp_Ax3(),r),1.e-7);builder.Add(face,stored);face.Reverse();auto wire=TopoDS::Wire(stored.Reversed());
 return {4,5,face,{{6,wire,{use(10,20,e0,100,100,p,p),use(11,21,e1,100,101,p,q),use(12,22,e2,101,101,q,q),use(13,21,e3,101,100,q,p)}}}};
}
SourceUVFace global_domain_controls(int ulps,bool evaluation_rounding=false){
 double lo=evaluation_rounding?-1.7889624832941899:0.,hi=lo+1,xend=evaluation_rounding?1.3925571192667849:1.;
 BRep_Builder builder;NCollection_Array2<gp_Pnt> poles(1,2,1,2);for(int i=1;i<=2;++i)for(int j=1;j<=2;++j)poles(i,j)=gp_Pnt(i==1?0:xend,j==1?lo:hi,0);NCollection_Array1<double> knots(1,2),vknots(1,2);knots(1)=0;knots(2)=xend;vknots(1)=lo;vknots(2)=hi;NCollection_Array1<int> mult(1,2);mult.Init(2);Handle(Geom_Surface) surface=new Geom_BSplineSurface(poles,knots,vknots,mult,mult,1,1);TopoDS_Face face;builder.MakeFace(face,surface,1.e-7);
 std::vector<TopoDS_Vertex> vertices;for(auto p:{gp_Pnt(0,lo,0),gp_Pnt(xend,lo,0),gp_Pnt(xend,hi,0),gp_Pnt(0,hi,0)})vertices.push_back(BRepBuilderAPI_MakeVertex(p).Vertex());TopoDS_Wire wire;builder.MakeWire(wire);SourceUVLoop loop;loop.id=3;
 for(int i=0;i<4;++i){auto e=BRepBuilderAPI_MakeEdge(vertices[i],vertices[(i+1)%4]).Edge();builder.Add(wire,e);loop.uses.push_back(use(10+i,20+i,e,100+i,100+(i+1)%4,vertices[i],vertices[(i+1)%4]));Handle(Geom2d_Curve) pc;
  if(i==0&&evaluation_rounding){NCollection_Array1<gp_Pnt2d> p(1,4);p(1)=gp_Pnt2d(0,lo);p(2)=gp_Pnt2d(.185,lo);p(3)=gp_Pnt2d((.37+xend)/2,lo);p(4)=gp_Pnt2d(xend,lo);NCollection_Array1<double> k(1,3);k(1)=0;k(2)=.37;k(3)=xend;NCollection_Array1<int> m(1,3);m(1)=3;m(2)=1;m(3)=3;pc=new Geom2d_BSplineCurve(p,k,m,2);}
  else if(i==0){NCollection_Array1<gp_Pnt2d> p(1,3);p(1)=gp_Pnt2d(0,lo);p(2)=gp_Pnt2d(xend/2,lo-ulps*(std::nextafter(1.,INFINITY)-1.));p(3)=gp_Pnt2d(xend,lo);NCollection_Array1<int> m(1,2);m.Init(3);pc=new Geom2d_BSplineCurve(p,knots,m,2);}
  else{auto a=BRep_Tool::Pnt(vertices[i]),b=BRep_Tool::Pnt(vertices[(i+1)%4]);pc=new Geom2d_Line(gp_Pnt2d(a.X(),a.Y()),gp_Dir2d(b.X()-a.X(),b.Y()-a.Y()));}builder.UpdateEdge(e,pc,face,0.);double first,last;BRep_Tool::Range(e,first,last);builder.Range(e,face,first,last);
 }builder.Add(face,wire);loop.wire=wire;return {1,2,face,{loop}};
}
SourceUVFace conic_boundary(double radius){
 BRep_Builder builder;NCollection_Array2<gp_Pnt> poles(1,2,1,2);for(int i=1;i<=2;++i)for(int j=1;j<=2;++j)poles(i,j)=gp_Pnt(i-1,j==1?-1:1,0);NCollection_Array1<double> ku(1,2),kv(1,2);ku(1)=0;ku(2)=1;kv(1)=-1;kv(2)=1;NCollection_Array1<int> mult(1,2);mult.Init(2);Handle(Geom_Surface) surface=new Geom_BSplineSurface(poles,ku,kv,mult,mult,1,1);TopoDS_Face face;builder.MakeFace(face,surface,1.e-7);
 double half=.02,center=1-radius*std::cos(half);Handle(Geom_Curve) circle=new Geom_Circle(gp_Circ(gp_Ax2(gp_Pnt(center,0,0),gp_Dir(0,0,1),gp_Dir(std::cos(half),-std::sin(half),0)),radius));auto a=circle->Value(0),b=circle->Value(2*half);std::vector<TopoDS_Vertex> vertices;for(auto p:{a,b,gp_Pnt(0,b.Y(),0),gp_Pnt(0,a.Y(),0)})vertices.push_back(BRepBuilderAPI_MakeVertex(p).Vertex());TopoDS_Wire wire;builder.MakeWire(wire);SourceUVLoop loop;loop.id=3;
 for(int i=0;i<4;++i){auto e=i==0?BRepBuilderAPI_MakeEdge(circle,vertices[0],vertices[1],0,2*half).Edge():BRepBuilderAPI_MakeEdge(vertices[i],vertices[(i+1)%4]).Edge();builder.Add(wire,e);loop.uses.push_back(use(10+i,20+i,e,100+i,100+(i+1)%4,vertices[i],vertices[(i+1)%4]));}builder.Add(face,wire);loop.wire=wire;return {1,2,face,{loop}};
}
Handle(Geom2d_BSplineCurve) example(){NCollection_Array1<gp_Pnt2d> p(1,3);p(1)=gp_Pnt2d(0,.1);p(2)=gp_Pnt2d(.5,.25);p(3)=gp_Pnt2d(1,.1);NCollection_Array1<double> k(1,2);k(1)=0;k(2)=1;NCollection_Array1<int> m(1,2);m.Init(3);return new Geom2d_BSplineCurve(p,k,m,2);}
// Line C(t)=(t,0,0), t in [0,1], on the plane z=0 with an authored pcurve and
// SameParameter=false. The step-source adapter cannot reach this state through
// OCCT's STEP translation, so the correspondence measure is driven directly.
// curve-knot: C is a degree-1 B-spline on the same line whose speed changes at
// the knot t=0.4 (x=0.3). inflection: P(s)=(g(s),0), g(s)=s+(s-m)^3 minus its
// chord, m=1/32, so the first s piece has zero error at its midpoint and
// 3h^3/64 at its quarter points. near-knot: as curve-knot with x(0.4) =
// 0.25 + 1e-10, so the knot lies 1.6e-10 in t from the node projected from
// the pcurve node s = 0.25: within projection tolerance, not bitwise equal.
int parameter_map(const std::string& mode){
 BRep_Builder builder;TopoDS_Face face;builder.MakeFace(face,new Geom_Plane(gp_Ax3()),1.e-7);
 auto a=BRepBuilderAPI_MakeVertex(gp_Pnt(0,0,0)).Vertex(),b=BRepBuilderAPI_MakeVertex(gp_Pnt(1,0,0)).Vertex();
 Handle(Geom_Curve) line=new Geom_Line(gp_Pnt(0,0,0),gp_Dir(1,0,0));
 if(mode=="parameter-map-curve-knot"||mode=="parameter-map-near-knot"){NCollection_Array1<gp_Pnt> poles(1,3);poles(1)=gp_Pnt(0,0,0);poles(2)=gp_Pnt(mode=="parameter-map-near-knot"?.25+1e-10:.3,0,0);poles(3)=gp_Pnt(1,0,0);NCollection_Array1<double> knots(1,3);knots(1)=0;knots(2)=.4;knots(3)=1;NCollection_Array1<int> mults(1,3);mults(1)=2;mults(2)=1;mults(3)=2;line=new Geom_BSplineCurve(poles,knots,mults,1);}
 auto edge=BRepBuilderAPI_MakeEdge(line,a,b,0,1).Edge();
 Handle(Geom2d_Curve) pc;if(mode=="parameter-map-identity"||mode=="parameter-map-curve-knot"||mode=="parameter-map-near-knot")pc=new Geom2d_Line(gp_Pnt2d(0,0),gp_Dir2d(1,0));
 else if(mode=="parameter-map-offset")pc=new Geom2d_Line(gp_Pnt2d(0,.01),gp_Dir2d(1,0));
 else if(mode=="parameter-map-inflection"){const double m=1./32,k=1.,a1=1+k*(3*m*m-((1-m)*(1-m)*(1-m)+m*m*m)),a2=-3*k*m,a3=k;
  NCollection_Array1<gp_Pnt2d> poles(1,4);poles(1)=gp_Pnt2d(0,0);poles(2)=gp_Pnt2d(a1/3,0);poles(3)=gp_Pnt2d(2*a1/3+a2/3,0);poles(4)=gp_Pnt2d(a1+a2+a3,0);pc=new Geom2d_BezierCurve(poles);}
 else{NCollection_Array1<gp_Pnt2d> poles(1,3);poles(1)=gp_Pnt2d(0,0);poles(2)=gp_Pnt2d(0,0);poles(3)=gp_Pnt2d(1,0);pc=new Geom2d_BezierCurve(poles);}
 builder.UpdateEdge(edge,pc,face,0.);builder.Range(edge,face,0,1);builder.SameParameter(edge,false);
 // Tighter bounds make the quadratic and the inflection maps split pieces.
 bool reversed=mode=="parameter-map-reversed";SourceUVOptions options;if(mode=="parameter-map-reparametrized"||reversed)options.source_residual_mm=1e-4;if(mode=="parameter-map-inflection")options.source_residual_mm=5e-6;
 auto map=measure_source_parameter_map(edge,face,reversed?1.:0.,reversed?0.:1.,options);std::cout<<std::setprecision(17)<<"parameter_map_bound_mm\t"<<options.source_residual_mm<<'\n';
 std::cout<<std::setprecision(17)<<"parameter_map_kind\t"<<map.kind<<"\nparameter_map_residual_mm\t"<<map.max_residual_mm<<"\nparameter_map_probes\t"<<map.probes<<'\n';
 for(const auto& row:map.rows)std::cout<<"parameter_map_row\t"<<row[0]<<'\t'<<row[1]<<'\t'<<row[2]<<'\n';return 0;
}
// B2 .37.31: projected UV curves of forms the domain correction does not take,
// on a bilinear B-spline over [0,1]^2, each leaving it (or, for the guard,
// staying inside). require_natural_domain must refuse each escaping form by
// name with its typed fields; exit 3 prints them, exit 0 the escape.
int natural_domain(const std::string& mode){
 NCollection_Array2<gp_Pnt> net(1,2,1,2);for(int i=1;i<=2;++i)for(int j=1;j<=2;++j)net(i,j)=gp_Pnt(i-1,j-1,0);NCollection_Array1<double> k(1,2);k(1)=0;k(2)=1;NCollection_Array1<int> m(1,2);m.Init(2);
 Handle(Geom_Surface) surface=new Geom_BSplineSurface(net,k,k,m,m,1,1);Handle(Geom2d_Curve) curve;double a=0,b=1;
 if(mode=="natural-domain-rational"||mode=="natural-domain-rational-inside"){bool inside=mode=="natural-domain-rational-inside";NCollection_Array1<gp_Pnt2d> poles(1,3);poles(1)=gp_Pnt2d(inside?.2:0,.1);poles(2)=gp_Pnt2d(inside?.5:-1.e-3,.5);poles(3)=gp_Pnt2d(inside?.8:0,.9);NCollection_Array1<double> w(1,3);w(1)=1;w(2)=2;w(3)=1;NCollection_Array1<int> mm(1,2);mm.Init(3);curve=new Geom2d_BSplineCurve(poles,w,k,mm,2);}
 else if(mode=="natural-domain-periodic"){NCollection_Array1<gp_Pnt2d> poles(1,4);poles(1)=gp_Pnt2d(1.01,.5);poles(2)=gp_Pnt2d(.5,1.01);poles(3)=gp_Pnt2d(-.01,.5);poles(4)=gp_Pnt2d(.5,-.01);NCollection_Array1<double> kk(1,5);for(int i=1;i<=5;++i)kk(i)=i-1;NCollection_Array1<int> mm(1,5);mm.Init(1);curve=new Geom2d_BSplineCurve(poles,kk,mm,2,true);b=4;}
 else if(mode=="natural-domain-trimmed")curve=new Geom2d_TrimmedCurve(new Geom2d_Circle(gp_Ax2d(gp_Pnt2d(.5,.5),gp_Dir2d(1,0)),.6),0,pi/2);
 else if(mode=="natural-domain-circle")curve=new Geom2d_Circle(gp_Ax2d(gp_Pnt2d(.5,.5),gp_Dir2d(1,0)),.6);
 else throw std::runtime_error("unknown natural-domain fixture");
 if(mode=="natural-domain-trimmed")b=pi/2;
 try{double escape=require_natural_domain(curve,a,b,surface,7,8);std::cout<<std::setprecision(17)<<"escape\t"<<escape<<"\n";return 0;}
 catch(const SourceUVDomainEscape& e){std::cout<<std::setprecision(17)<<"refused\t"<<e.face_id<<'\t'<<e.coedge_id<<'\t'<<e.kind<<'\t'<<e.rational<<'\t'<<e.periodic<<'\t'<<e.trimmed<<'\t'<<e.escape<<"\n";std::cerr<<e.what()<<"\n";return 3;}
}
int main(int argc,char** argv){try{if(argc!=3)throw std::runtime_error("fixture arguments");std::string mode=argv[1];if(mode.rfind("natural-domain-",0)==0)return natural_domain(mode);if(mode.rfind("parameter-map-",0)==0)return parameter_map(mode);if(mode=="sphere-v-shift"||mode=="period-basis-edit"){auto before=example();auto after=Handle(Geom2d_BSplineCurve)::DownCast(before->Copy());Handle(Geom_Surface) surface;if(mode=="sphere-v-shift"){surface=new Geom_SphericalSurface(gp_Ax3(),2.5);after->Translate(gp_Vec2d(0,pi));}else{surface=new Geom_CylindricalSurface(gp_Ax3(),.345);after->Translate(gp_Vec2d(2*pi,0));auto p=after->Pole(2);p.SetY(p.Y()+.02);after->SetPole(2,p);}require_source_uv_period_translation(before,after,surface,0,1);return 0;}
 auto binding=mode=="evaluation-rounding"?global_domain_controls(0,true):mode=="cylinder-reversed"?reversed_cylinder():mode.find("global-domain-controls")==0?global_domain_controls(mode=="global-domain-controls-too-far"?512:8):mode.find("conic-boundary")==0?conic_boundary(mode=="conic-boundary-too-far"?10:.01):octant();if(mode=="sphere-octant-reversed"){binding.face.Reverse();for(auto& loop:binding.loops){loop.wire.Reverse();std::reverse(loop.uses.begin(),loop.uses.end());for(auto& u:loop.uses){u.edge.Reverse();std::swap(u.start,u.end);std::swap(u.start_id,u.end_id);}}}if(mode=="dropped-use")binding.loops.front().uses.erase(binding.loops.front().uses.begin()+1);BRep_Builder owner_builder;TopoDS_Shell owner;owner_builder.MakeShell(owner);owner_builder.Add(owner,binding.face);SourceUVOptions options;options.prepare_query=true;if(mode=="conic-boundary-tight-budget")options.tolerance_mm=1.e-7;auto prepared=prepare_source_uv(owner,{binding},options);auto face=TopoDS::Face(prepared.query_mapped(binding.face));if(!BinTools::Write(prepared.query_shape,argv[2],false,false,BinTools_FormatVersion_VERSION_4))throw std::runtime_error("query write failed");std::cout<<std::setprecision(17)<<"input_face_free\t"<<binding.face.Free()<<"\nquery_face_free\t"<<face.Free()<<"\n";
 if(mode.find("conic-boundary")==0){for(const auto& r:prepared.chart_approximations)std::cout<<"source_chart\t"<<r.face_id<<'\t'<<r.coedge_id<<'\t'<<r.axis<<'\t'<<r.side<<'\t'<<r.max_guide_coefficient_delta<<'\t'<<r.uv_metric_mm<<'\t'<<r.max_guide_extension_residual_mm<<'\t'<<r.max_selected_residual_mm<<'\n';std::cout<<"query_uv_metric\t"<<prepared.stats.max_query_uv_metric_mm<<'\n';auto original=binding.loops.front().uses.front().edge;auto raw_face=TopoDS::Face(prepared.mapped(binding.face));auto raw_edge=TopoDS::Edge(prepared.mapped(original));double a,b;auto curve=Handle(Geom_Circle)::DownCast(BRep_Tool::Curve(raw_edge,a,b));std::cout<<"original_circle\t"<<curve->Radius()<<'\t'<<a<<'\t'<<b<<'\n';auto pc=BRep_Tool::CurveOnSurface(raw_edge,raw_face,a,b);for(int i=0;i<=16;++i){double t=i==0?a:i==16?b:a+(b-a)*i/16;auto uv=pc->Value(t);std::cout<<"raw_boundary\t"<<t<<'\t'<<uv.X()<<'\t'<<uv.Y()<<'\n';}}
 if(mode.find("global-domain-controls")==0||mode=="evaluation-rounding"){auto original=binding.loops.front().uses.front().edge;auto raw_face=TopoDS::Face(prepared.mapped(binding.face));auto raw_edge=TopoDS::Edge(prepared.mapped(original)),query_edge=TopoDS::Edge(prepared.query_mapped(original));double a,b;auto raw=Handle(Geom2d_BSplineCurve)::DownCast(BRep_Tool::CurveOnSurface(raw_edge,raw_face,a,b));auto query=Handle(Geom2d_BSplineCurve)::DownCast(BRep_Tool::CurveOnSurface(query_edge,face,a,b));std::cout<<"query_pc_basis\t"<<query->Degree()<<'\t'<<query->NbPoles()<<'\t'<<a<<'\t'<<b<<"\n";for(int i=1;i<=query->NbPoles();++i){auto p=query->Pole(i);std::cout<<"query_control\t"<<i<<'\t'<<p.X()<<'\t'<<p.Y()<<'\t'<<query->Weight(i)<<"\n";}std::cout<<"raw_middle_v\t"<<raw->Pole(2).Y()<<"\n";}
 for(auto& u:binding.loops.front().uses){auto e=TopoDS::Edge(prepared.query_mapped(u.edge));double a,b;auto curve=BRep_Tool::Curve(e,a,b);double pa,pb;auto pc=BRep_Tool::CurveOnSurface(e,face,pa,pb);auto surface=BRep_Tool::Surface(face);for(int i=0;i<=16;++i){double t=i==0?a:i==16?b:a+(b-a)*i/16;auto p=curve->Value(t);auto uv=pc->Value(t);auto s=surface->Value(uv.X(),uv.Y());std::cout<<"boundary\t"<<u.id<<'\t'<<t<<'\t'<<p.X()<<'\t'<<p.Y()<<'\t'<<p.Z()<<'\t'<<uv.X()<<'\t'<<uv.Y()<<'\t'<<s.X()<<'\t'<<s.Y()<<'\t'<<s.Z()<<'\n';}}
 for(const auto& c:prepared.connectors)std::cout<<"connector\t"<<c.u0<<'\t'<<c.v0<<'\t'<<c.u1<<'\t'<<c.v1<<'\n';
 std::cout<<"original_uses";int ordered=0;for(TopExp_Explorer w(face,TopAbs_WIRE);w.More();w.Next()){Handle(ShapeExtend_WireData) data=new ShapeExtend_WireData(TopoDS::Wire(w.Current()),true);for(int i=1;i<=data->NbEdges();++i){auto e=data->Edge(i);if(BRep_Tool::Degenerated(e))continue;bool found=false;for(const auto& u:binding.loops.front().uses){auto expected=prepared.query_mapped(u.edge);if(e.IsSame(expected)&&e.Orientation()==expected.Orientation()){std::cout<<'\t'<<u.id;found=true;break;}}if(!found)throw std::runtime_error("unowned query physical use");}for(BRepTools_WireExplorer e(TopoDS::Wire(w.Current()),face);e.More();e.Next())++ordered;}std::cout<<"\nordered_uses\t"<<ordered<<'\n';return 0;
 }catch(const Standard_Failure& e){std::cerr<<"source UV fixture: "<<e.what()<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
