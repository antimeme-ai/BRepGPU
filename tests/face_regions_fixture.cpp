// Drives native/step_source/face_regions.hpp directly (review round 4, item 2):
// faces on closed surfaces whose same_sense chooses between two complementary
// regions, which no file reaches yet (the source-UV stage refuses them).
// Prints one tab-separated line per case: name, decided, inside, area, flux,
// and the reason when undecided. The flux is about the origin.
#include "face_contact.hpp"
#include "face_regions.hpp"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_Plane.hxx>
#include <Geom2d_Line.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <TopExp.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomProjLib.hxx>
#include <Geom_Circle.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <Standard_Failure.hxx>
#include <gp_Ax3.hxx>
#include <gp_Circ.hxx>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace brepgpu::face_regions;

namespace {
std::vector<PoleCandidate> singularities(const Handle(Geom_Surface)& s){
  ShapeAnalysis_Surface analysis(s);std::vector<PoleCandidate> out;const int n=analysis.NbSingularities(1e-7);
  for(int k=1;k<=n;++k){double preci=0,fp=0,lp=0;gp_Pnt p;gp_Pnt2d a,b;bool uiso=false;if(analysis.Singularity(k,preci,p,a,b,fp,lp,uiso))out.push_back({uiso?0:1,uiso?a.X():a.Y()});}
  return out;
}
void report(const std::string& name,const TwoRegions& r){
  std::cout<<std::setprecision(17)<<name<<'\t'<<r.decided<<'\t'<<r.inside<<'\t'<<r.area<<'\t'<<r.flux<<'\t'<<r.why<<'\n';
}
// The x-axis sphere of radius 5 (the reviewer's truncated sphere): its poles are
// at x = +-5, and the circle z = -3 (radius 4) winds neither of them.
Handle(Geom_Surface) x_sphere(){return new Geom_SphericalSurface(gp_Ax3(gp_Pnt(0,0,0),gp_Dir(1,0,0),gp_Dir(0,0,1)),5.);}
UvUse projected_circle(const Handle(Geom_Surface)& s,const gp_Circ& circle,bool increasing){
  Handle(Geom_Circle) c=new Geom_Circle(circle);double tolerance=1e-9;auto pc=GeomProjLib::Curve2d(c,0.,2*M_PI,s,tolerance);
  if(pc.IsNull())throw std::runtime_error("projection failed");return {1,pc,pc->FirstParameter(),pc->LastParameter(),increasing};
}
}  // namespace

namespace {
// Review round 7: a torus patch (R 4, r 1; u in [0, 1], v in [0, pi/2]) read
// as its complement, against a cylinder (radius 5, coaxial, tangent to the
// torus along its outer equator) sharing the patch's equator edge and
// reaching z = -0.5, over u in [0, 1]: it touches the complement only along
// the common edge (tangent there: no meeting). With end > 1 it continues to u
// = end below z = -notch, an L shape: there the complement, below the equator,
// comes within notch^2 / 2 of it (4.5e-8 mm for notch 3e-4, under the 1e-7 mm
// resolution) about 1 mm from the common edge, without crossing it. With
// notch = 0 it stays tangent along the equator past the patch's corner.
int cylinder_contact(double end,double notch=0.,double target=INFINITY,const char* label="contact"){
  Handle(Geom_ToroidalSurface) torus=new Geom_ToroidalSurface(gp_Ax3(),4.,1.);
  const TopoDS_Face patch=BRepBuilderAPI_MakeFace(torus,0.,1.,0.,M_PI/2,1e-7).Face();
  TopoDS_Edge equator;for(TopExp_Explorer e(patch,TopAbs_EDGE);e.More();e.Next()){BRepAdaptor_Curve c(TopoDS::Edge(e.Current()));const gp_Pnt m=c.Value(.5*(c.FirstParameter()+c.LastParameter()));
    if(std::abs(m.Z())<1e-9&&std::abs(std::hypot(m.X(),m.Y())-5.)<1e-9)equator=TopoDS::Edge(e.Current());}
  if(equator.IsNull())throw std::runtime_error("no equator edge");
  TopoDS_Vertex a,b;TopExp::Vertices(equator,a,b);const gp_Pnt pa=BRep_Tool::Pnt(a);if(std::abs(pa.Y())>1e-9)std::swap(a,b);
  const gp_Pnt p0=BRep_Tool::Pnt(a),p1=BRep_Tool::Pnt(b);const gp_Pnt p2(5*std::cos(end),5*std::sin(end),0.),q2(5*std::cos(end),5*std::sin(end),-.5),q0(5.,0.,-.5);
  const gp_Circ top(gp_Ax2(gp_Pnt(0,0,0),gp_Dir(0,0,1)),5.),bottom(gp_Ax2(gp_Pnt(0,0,-.5),gp_Dir(0,0,1)),5.);
  BRepBuilderAPI_MakeWire wire;wire.Add(equator);
  TopoDS_Vertex corner=b;
  if(end>1.+1e-12&&notch>0){const gp_Circ low(gp_Ax2(gp_Pnt(0,0,-notch),gp_Dir(0,0,1)),5.);const TopoDS_Vertex n1=BRepBuilderAPI_MakeVertex(gp_Pnt(5*std::cos(1.),5*std::sin(1.),-notch)).Vertex(),n2=BRepBuilderAPI_MakeVertex(gp_Pnt(5*std::cos(end),5*std::sin(end),-notch)).Vertex();
    wire.Add(BRepBuilderAPI_MakeEdge(b,n1).Edge());wire.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(low,1.,end,true).Value(),n1,n2).Edge());corner=n2;}
  else if(end>1.+1e-12){const TopoDS_Vertex v2=BRepBuilderAPI_MakeVertex(p2).Vertex();wire.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(top,1.,end,true).Value(),b,v2).Edge());corner=v2;}
  const TopoDS_Vertex w2=BRepBuilderAPI_MakeVertex(q2).Vertex(),w0=BRepBuilderAPI_MakeVertex(q0).Vertex();
  wire.Add(BRepBuilderAPI_MakeEdge(corner,w2).Edge());
  wire.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(bottom,0.,end,true).Value(),w0,w2).Edge());
  wire.Add(BRepBuilderAPI_MakeEdge(w0,a).Edge());
  if(!wire.IsDone())throw std::runtime_error("cylinder wire");
  Handle(Geom_CylindricalSurface) cylinder=new Geom_CylindricalSurface(gp_Ax3(),5.);
  BRepBuilderAPI_MakeFace made(cylinder,wire.Wire(),true);if(!made.IsDone())throw std::runtime_error("cylinder face");
  ShapeFix_Face fix(made.Face());fix.Perform();const TopoDS_Face neighbour=fix.Face();
  // A translated file's tolerances.
  ShapeFix_ShapeTolerance().SetTolerance(neighbour,1e-7);
  TwoRegions region;region.decided=true;region.inside=false;region.axis=1;region.pu=region.pv=2*M_PI;
  region.polylines={{gp_Pnt2d(0,0),gp_Pnt2d(1,0),gp_Pnt2d(1,M_PI/2),gp_Pnt2d(0,M_PI/2)}};
  const auto contact=brepgpu::face_contact::region_meets(torus,region,false,patch,{{neighbour,TopoDS_Face()}},1e-7,target);
  std::cout<<label<<'\t'<<end<<'\t'<<notch<<'\t'<<contact.result<<'\t'<<contact.crossings_only<<'\n';return 0;
}
// The same against a plane: the patch u in [0, 1], v in [pi/2, pi] (inner upper
// quadrant), read as its complement; the plane z = 1, tangent to the torus
// along its top circle, sharing the patch's top edge and reaching out to
// radius 5 over u in [0, 1]; with end > 1 it continues to u = end outside
// radius 4 + notch, where the complement comes within notch^2 / 2 of it.
int plane_contact(double end,double notch){
  Handle(Geom_ToroidalSurface) torus=new Geom_ToroidalSurface(gp_Ax3(),4.,1.);
  const TopoDS_Face patch=BRepBuilderAPI_MakeFace(torus,0.,1.,M_PI/2,M_PI,1e-7).Face();
  TopoDS_Edge top;for(TopExp_Explorer e(patch,TopAbs_EDGE);e.More();e.Next()){BRepAdaptor_Curve c(TopoDS::Edge(e.Current()));const gp_Pnt m=c.Value(.5*(c.FirstParameter()+c.LastParameter()));
    if(std::abs(m.Z()-1.)<1e-9&&std::abs(std::hypot(m.X(),m.Y())-4.)<1e-9)top=TopoDS::Edge(e.Current());}
  if(top.IsNull())throw std::runtime_error("no top edge");
  TopoDS_Vertex a,b;TopExp::Vertices(top,a,b);if(std::abs(BRep_Tool::Pnt(a).Y())>1e-9)std::swap(a,b);
  auto at=[](double radius,double u){return BRepBuilderAPI_MakeVertex(gp_Pnt(radius*std::cos(u),radius*std::sin(u),1.)).Vertex();};
  auto arc=[](double radius,double u0,double u1,const TopoDS_Vertex& v0,const TopoDS_Vertex& v1){return BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(gp_Circ(gp_Ax2(gp_Pnt(0,0,1),gp_Dir(0,0,1)),radius),u0,u1,true).Value(),v0,v1).Edge();};
  BRepBuilderAPI_MakeWire wire;wire.Add(top);
  const TopoDS_Vertex outer0=at(5.,0.);TopoDS_Vertex last;
  if(end<0){// a detached lobe: a bridge outside radius 4.5 over u in [1, 2], then outside 4 + notch over [2, 2.5]
    const TopoDS_Vertex m1=at(4.5,1.),m2=at(4.5,2.),n2=at(4.+notch,2.),n3=at(4.+notch,2.5),o3=at(5.,2.5);
    wire.Add(BRepBuilderAPI_MakeEdge(b,m1).Edge());wire.Add(arc(4.5,1.,2.,m1,m2));wire.Add(BRepBuilderAPI_MakeEdge(m2,n2).Edge());wire.Add(arc(4.+notch,2.,2.5,n2,n3));wire.Add(BRepBuilderAPI_MakeEdge(n3,o3).Edge());wire.Add(arc(5.,0.,2.5,outer0,o3));}
  else if(end>1.+1e-12){const TopoDS_Vertex n1=at(4.+notch,1.),n2=at(4.+notch,end),o2=at(5.,end);
    wire.Add(BRepBuilderAPI_MakeEdge(b,n1).Edge());wire.Add(arc(4.+notch,1.,end,n1,n2));wire.Add(BRepBuilderAPI_MakeEdge(n2,o2).Edge());wire.Add(arc(5.,0.,end,outer0,o2));}
  else{const TopoDS_Vertex o1=at(5.,1.);wire.Add(BRepBuilderAPI_MakeEdge(b,o1).Edge());wire.Add(arc(5.,0.,1.,outer0,o1));}
  wire.Add(BRepBuilderAPI_MakeEdge(outer0,a).Edge());
  if(!wire.IsDone())throw std::runtime_error("plane wire");
  Handle(Geom_Plane) plane=new Geom_Plane(gp_Pnt(0,0,1),gp_Dir(0,0,1));
  BRepBuilderAPI_MakeFace made(plane,wire.Wire(),true);if(!made.IsDone())throw std::runtime_error("plane face");
  ShapeFix_Face fix(made.Face());fix.Perform();const TopoDS_Face neighbour=fix.Face();
  // A translated file's tolerances.
  ShapeFix_ShapeTolerance().SetTolerance(neighbour,1e-7);
  TwoRegions region;region.decided=true;region.inside=false;region.axis=1;region.pu=region.pv=2*M_PI;
  region.polylines={{gp_Pnt2d(0,M_PI/2),gp_Pnt2d(1,M_PI/2),gp_Pnt2d(1,M_PI),gp_Pnt2d(0,M_PI)}};
  const auto contact=brepgpu::face_contact::region_meets(torus,region,false,patch,{{neighbour,TopoDS_Face()}},1e-7);
  double tolerance=0;for(TopExp_Explorer e(neighbour,TopAbs_EDGE);e.More();e.Next())tolerance=std::max(tolerance,BRep_Tool::Tolerance(TopoDS::Edge(e.Current())));for(TopExp_Explorer v(neighbour,TopAbs_VERTEX);v.More();v.Next())tolerance=std::max(tolerance,BRep_Tool::Tolerance(TopoDS::Vertex(v.Current())));
  std::cout<<"plane\t"<<end<<'\t'<<notch<<'\t'<<contact.result<<'\t'<<contact.crossings_only<<'\t'<<tolerance<<'\t'<<BRep_Tool::Tolerance(neighbour)<<'\n';return 0;
}
// Review round 8, finding 2: a cone face bounded by its base circle alone
// (its pcurve the line v = base), as OCCT translates a cone with an apex
// vertex loop: BRepBndLib boxes it over that pcurve, flat at the base, missing
// the apex. Its apex points down at the torus top circle (in the complement
// of the patch u in [0, 1], v in [pi/2, pi]) from gap above; it shares nothing.
int apex_contact(double gap){
  Handle(Geom_ToroidalSurface) torus=new Geom_ToroidalSurface(gp_Ax3(),4.,1.);
  const TopoDS_Face patch=BRepBuilderAPI_MakeFace(torus,0.,1.,M_PI/2,M_PI,1e-7).Face();
  const gp_Pnt apex(4*std::cos(2.),4*std::sin(2.),1.+gap);const double semi=M_PI/4,height=1.;
  // The cone opens upward from its apex: axis +z, reference radius at the base.
  Handle(Geom_ConicalSurface) cone=new Geom_ConicalSurface(gp_Ax3(apex.Translated(gp_Vec(0,0,height)),gp_Dir(0,0,1)),semi,height*std::tan(semi));
  const gp_Circ base(gp_Ax2(apex.Translated(gp_Vec(0,0,height)),gp_Dir(0,0,1)),height*std::tan(semi));
  TopoDS_Edge circle=BRepBuilderAPI_MakeEdge(base).Edge();
  BRep_Builder builder;Handle(Geom2d_Line) line=new Geom2d_Line(gp_Pnt2d(0,0),gp_Dir2d(1,0));builder.UpdateEdge(circle,line,cone,TopLoc_Location(),1e-7);builder.Range(circle,cone,TopLoc_Location(),0.,2*M_PI);
  TopoDS_Wire wire;builder.MakeWire(wire);builder.Add(wire,circle);TopoDS_Face face;builder.MakeFace(face,cone,1e-7);builder.Add(face,wire);
  Bnd_Box occt;BRepBndLib::Add(face,occt);double x0,y0,z0,x1,y1,z1;occt.Get(x0,y0,z0,x1,y1,z1);
  TwoRegions region;region.decided=true;region.inside=false;region.axis=1;region.pu=region.pv=2*M_PI;
  region.polylines={{gp_Pnt2d(0,M_PI/2),gp_Pnt2d(1,M_PI/2),gp_Pnt2d(1,M_PI),gp_Pnt2d(0,M_PI)}};
  // Measured on its carrier over its UV box to the apex, as the adapter does for a pole face.
  const double v_apex=-cone->RefRadius()/std::sin(semi);
  const auto contact=brepgpu::face_contact::region_meets(torus,region,false,patch,{{face,brepgpu::face_contact::pole_proxy(cone,0.,2*M_PI,v_apex,0.)}},1e-7);
  std::cout<<std::setprecision(17)<<"apex\t"<<gap<<'\t'<<0<<'\t'<<contact.result<<'\t'<<contact.crossings_only<<'\t'<<z0<<'\n';return 0;
}
}  // namespace

int main(int argc,char** argv){
  try{
    if(argc>1){const std::string only=argv[1];auto clock=[]{return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();};double t=clock();
      if(only=="uncertain")cylinder_contact(1.0003,3e-4,INFINITY,"uncertain");else if(only=="undecided")cylinder_contact(1.,0.,1e-6,"undecided");else if(only=="apex")apex_contact(5e-8);
      std::cerr<<only<<" seconds "<<clock()-t<<"\n";return 0;}
    const double pi=M_PI;
    auto sphere=x_sphere();
    std::cout<<"closed\tsphere\t"<<closed_surface(sphere,singularities(sphere))<<'\n';
    Handle(Geom_Surface) torus=new Geom_ToroidalSurface(gp_Ax3(),4.,1.);
    std::cout<<"closed\ttorus\t"<<closed_surface(torus,singularities(torus))<<'\n';
    Handle(Geom_Surface) cylinder=new Geom_CylindricalSurface(gp_Ax3(),2.);
    std::cout<<"closed\tcylinder\t"<<closed_surface(cylinder,singularities(cylinder))<<'\n';
    Handle(Geom_Surface) cone=new Geom_ConicalSurface(gp_Ax3(),pi/6,2.);
    std::cout<<"closed\tcone\t"<<closed_surface(cone,singularities(cone))<<'\n';
    // The circle z = -3 about the z axis, radius 4, run either way, each same_sense.
    const gp_Circ cut(gp_Ax2(gp_Pnt(0,0,-3),gp_Dir(0,0,1)),4.);
    for(bool increasing:{true,false})for(int same:{1,-1}){
      const auto use=projected_circle(sphere,cut,increasing);
      report(std::string("cap\t")+(increasing?"forward":"backward")+'\t'+(same>0?"T":"F"),two_regions({use},sphere,same));}
    // The loop cut in two uses, the second moved by a whole period in u (the
    // same chart once unwrapped: decided as the unmoved loop) or by 0.5 (no
    // chart: refused).
    {auto use=projected_circle(sphere,cut,true);const double mid=.5*(use.first+use.last);
      for(double shift:{2*pi,.5}){Handle(Geom2d_Curve) shifted=Handle(Geom2d_Curve)::DownCast(use.curve->Translated(gp_Vec2d(shift,0)));
        report(std::string("cap\t")+(shift>1?"period":"split")+"\tT",two_regions({{1,use.curve,use.first,mid,true},{1,shifted,mid,use.last,true}},sphere,1));}}
    // On the torus: a small UV square loop (winds nothing), and a meridian (v
    // runs a whole period at u = 1: it winds and separates nothing).
    {auto seg=[&](double a,double b,double c,double d){Handle(Geom2d_TrimmedCurve) t=GCE2d_MakeSegment(gp_Pnt2d(a,b),gp_Pnt2d(c,d)).Value();Handle(Geom2d_Curve) l=t;return UvUse{1,l,l->FirstParameter(),l->LastParameter(),true};};
      report("torus\tsquare\tT",two_regions({seg(1,1,2,1),seg(2,1,2,2),seg(2,2,1,2),seg(1,2,1,1)},torus,1));
      Handle(Geom2d_Curve) meridian=new Geom2d_Line(gp_Pnt2d(1,0),gp_Dir2d(0,1));
      report("torus\tmeridian\tT",two_regions({{1,meridian,0.,2*pi,true}},torus,1));
      // A band between two parallel circles (v = 1 and v = 2), run opposite ways:
      // both bands are regions; each same_sense chooses one.
      Handle(Geom2d_Curve) low=new Geom2d_Line(gp_Pnt2d(0,1),gp_Dir2d(1,0)),high=new Geom2d_Line(gp_Pnt2d(0,2),gp_Dir2d(-1,0));
      for(int same:{1,-1})report(std::string("torus\tband\t")+(same>0?"T":"F"),two_regions({{1,low,0.,2*pi,true},{2,high,-2*pi,0.,true}},torus,same));
      // Twice around: irregular.
      report("torus\ttwice\tT",two_regions({{1,low,0.,4*pi,true},{2,high,-4*pi,0.,true}},torus,1));
      std::cout<<std::setprecision(17)<<"whole\ttorus\t"<<natural_flux(torus)<<'\n';}
    // A zone of the z-axis sphere between latitudes v = -0.3 and v = 0.4: its
    // complement is the two caps, not one region, so the zone is the only reading.
    {Handle(Geom_Surface) z_sphere=new Geom_SphericalSurface(gp_Ax3(),5.);Handle(Geom2d_Curve) low=new Geom2d_Line(gp_Pnt2d(0,-.3),gp_Dir2d(1,0)),high=new Geom2d_Line(gp_Pnt2d(0,.4),gp_Dir2d(-1,0));
      const auto zone=two_regions({{1,low,0.,2*pi,true},{2,high,-2*pi,0.,true}},z_sphere,-1);
      std::cout<<std::setprecision(17)<<"zone\t"<<zone.decided<<'\t'<<zone.unique<<'\t'<<zone.signed_area<<'\t'<<zone.signed_flux<<'\t'<<zone.why<<'\n';}
    cylinder_contact(1.);cylinder_contact(1.5);for(double notch:{3e-4,4.6e-4,2e-3})cylinder_contact(1.5,notch);
    cylinder_contact(1.0003,3e-4,INFINITY,"uncertain");cylinder_contact(1.,0.,1e-6,"undecided");apex_contact(5e-8);apex_contact(1e-3);
    plane_contact(1.,0.);for(double notch:{1e-4,3e-4,4.2e-4,4.6e-4,2e-3})plane_contact(1.5,notch);for(double notch:{1e-4,3e-4,4.6e-4,2e-3})plane_contact(-1.,notch);
    return 0;
  }catch(const Standard_Failure& e){std::cerr<<"face regions fixture: "<<e.what()<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
