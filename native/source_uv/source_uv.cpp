#include "source_uv.hpp"
#include <BRepBuilderAPI_Copy.hxx>
#include <Standard_Failure.hxx>
#include <NCollection_IndexedMap.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Torus.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <ShapeConstruct_ProjectCurveOnSurface.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include <ShapeFix_Edge.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeExtend_WireData.hxx>
#include <NCollection_Array1.hxx>
#include <Bnd_Box2d.hxx>
#include <BndLib_Add2dCurve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2d_BezierCurve.hxx>


#include <gp_Cone.hxx>
#include <gp_Sphere.hxx>
#include <gp_Vec2d.hxx>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace brepgpu {
namespace {
using Clock=std::chrono::steady_clock;
[[noreturn]] void fail(const std::string& s){throw std::runtime_error("source UV: "+s);}
void require(bool b,const std::string& s){if(!b)fail(s);}
bool finite(const gp_Pnt2d& p){return std::isfinite(p.X())&&std::isfinite(p.Y());}
double grade(double scale){double x=std::max(1.,std::abs(scale));return 128*(std::nextafter(x,INFINITY)-x);}
std::string context(const SourceUVFace& f){return "face "+std::to_string(f.id)+" carrier "+std::to_string(f.carrier_id);}
// The one edge loop of a nonplanar face; vertex loops have no uses to project.
template<class Face> auto& edge_loop(Face& f){auto it=std::find_if(f.loops.begin(),f.loops.end(),[](const SourceUVLoop& l){return !l.pole_id;});require(it!=f.loops.end(),context(f)+": no edge loop");return *it;}
Handle(Geom2d_Curve) copy2(const Handle(Geom2d_Curve)& c){require(!c.IsNull(),"null pcurve");return Handle(Geom2d_Curve)::DownCast(c->Copy());}
Handle(Geom_Surface) world_surface(const TopoDS_Face& f){TopLoc_Location l;auto s=BRep_Tool::Surface(f,l);return l.IsIdentity()?s:Handle(Geom_Surface)::DownCast(s->Transformed(l.Transformation()));}
Handle(Geom_Curve) world_curve(const TopoDS_Edge& e,double& a,double& b){TopLoc_Location l;auto c=BRep_Tool::Curve(e,l,a,b);return c.IsNull()||l.IsIdentity()?c:Handle(Geom_Curve)::DownCast(c->Transformed(l.Transformation()));}
bool pole(const Handle(Geom_Surface)& surface,const gp_Pnt& p,double& v);
struct PC {Handle(Geom2d_Curve) curve;double first=0,last=0;};
PC pcurve(const TopoDS_Edge& e,const TopoDS_Face& f){PC p;p.curve=BRep_Tool::CurveOnSurface(e,f,p.first,p.last);return p;}
struct FaceState {
 TopoDS_Face face;TopLoc_Location location;Handle(Geom_Surface) surface;TopAbs_Orientation orientation;double tolerance;bool free;
 explicit FaceState(const TopoDS_Face& f):face(f),surface(BRep_Tool::Surface(f,location)),orientation(f.Orientation()),tolerance(BRep_Tool::Tolerance(f)),free(f.Free()){}
 void check(const std::string& c)const{TopLoc_Location l;require(BRep_Tool::Surface(face,l)==surface&&l==location&&face.Orientation()==orientation&&BRep_Tool::Tolerance(face)==tolerance&&face.Free()==free,c+": source face changed");}
};
struct EdgeState {
  TopoDS_Edge edge;Handle(Geom_Curve) curve;TopLoc_Location location;double first,last,tolerance;
  TopoDS_Vertex a,b;gp_Pnt pa,pb;double ta,tb;bool same,range,deg;
  explicit EdgeState(const TopoDS_Edge& e):edge(e){curve=BRep_Tool::Curve(e,location,first,last);tolerance=BRep_Tool::Tolerance(e);TopExp::Vertices(e,a,b);pa=BRep_Tool::Pnt(a);pb=BRep_Tool::Pnt(b);ta=BRep_Tool::Tolerance(a);tb=BRep_Tool::Tolerance(b);same=BRep_Tool::SameParameter(e);range=BRep_Tool::SameRange(e);deg=BRep_Tool::Degenerated(e);}
  void check(const std::string& c)const{TopLoc_Location l;double x,y;auto current=BRep_Tool::Curve(edge,l,x,y);TopoDS_Vertex av,bv;TopExp::Vertices(edge,av,bv);
    require(current==curve&&l==location&&x==first&&y==last&&av.IsSame(a)&&bv.IsSame(b)&&BRep_Tool::Pnt(av).IsEqual(pa,0)&&BRep_Tool::Pnt(bv).IsEqual(pb,0)&&BRep_Tool::Tolerance(av)==ta&&BRep_Tool::Tolerance(bv)==tb&&BRep_Tool::Tolerance(edge)==tolerance&&BRep_Tool::SameParameter(edge)==same&&BRep_Tool::SameRange(edge)==range&&BRep_Tool::Degenerated(edge)==deg,c+": physical source edge changed");}
};
std::vector<SourceUVFace> mapped_faces(const std::vector<SourceUVFace>& faces,const BRepBuilderAPI_Copy& copier){
  auto map=[&](const TopoDS_Shape& s){auto d=copier.ModifiedShape(s);require(!d.IsNull(),"copier has no source binding");return d.Oriented(s.Orientation());};
  auto result=faces;for(auto& f:result){f.face=TopoDS::Face(map(f.face));for(auto& w:f.unexplained)w=TopoDS::Wire(map(w));for(auto& l:f.loops){if(!l.wire.IsNull())l.wire=TopoDS::Wire(map(l.wire));if(!l.pole.IsNull())l.pole=TopoDS::Vertex(map(l.pole));for(auto& u:l.uses){u.edge=TopoDS::Edge(map(u.edge));u.start=TopoDS::Vertex(map(u.start));u.end=TopoDS::Vertex(map(u.end));}}}return result;
}
std::vector<double> samples(const Handle(Geom_Curve)& c,double a,double b){std::vector<double> cuts{a,b};auto basis=c;while(!Handle(Geom_TrimmedCurve)::DownCast(basis).IsNull())basis=Handle(Geom_TrimmedCurve)::DownCast(basis)->BasisCurve();auto spline=Handle(Geom_BSplineCurve)::DownCast(basis);if(!spline.IsNull())for(int i=1;i<=spline->NbKnots();++i)if(a<spline->Knot(i)&&spline->Knot(i)<b)cuts.push_back(spline->Knot(i));std::sort(cuts.begin(),cuts.end());std::vector<double> result;for(std::size_t i=1;i<cuts.size();++i)for(int j=0;j<=16;++j)result.push_back(j==0?cuts[i-1]:j==16?cuts[i]:cuts[i-1]+(cuts[i]-cuts[i-1])*j/16);return result;}
// Recover only finite evaluator roundoff over a proven in-domain positive hull.
// This detached probe never changes the stored curve or its original interval.
gp_Pnt2d spline_probe(const PC& pc,const Handle(Geom_Surface)& surface,gp_Pnt2d uv,const std::string& where){
 require(finite(uv),where+": nonfinite spline probe");double bounds[4];surface->Bounds(bounds[0],bounds[1],bounds[2],bounds[3]);
 if(uv.X()>=bounds[0]&&uv.X()<=bounds[1]&&uv.Y()>=bounds[2]&&uv.Y()<=bounds[3])return uv;
 auto basis=Handle(Geom2d_BSplineCurve)::DownCast(pc.curve);require(!basis.IsNull()&&!basis->IsRational()&&!basis->IsPeriodic(),where+": unsupported finite-probe recovery basis");
 auto active=Handle(Geom2d_BSplineCurve)::DownCast(basis->Copy());active->Segment(pc.first,pc.last);require(active->FirstParameter()==pc.first&&active->LastParameter()==pc.last,where+": finite-probe active interval changed");
 for(int i=1;i<=active->NbPoles();++i){auto p=active->Pole(i);require(finite(p)&&p.X()>=bounds[0]&&p.X()<=bounds[1]&&p.Y()>=bounds[2]&&p.Y()<=bounds[3],where+": finite-probe coefficient hull leaves original domain");}
 for(int axis=0;axis<2;++axis){double x=uv.Coord(axis+1),lo=bounds[2*axis],hi=bounds[2*axis+1],epsilon=grade(std::max(std::abs(lo),std::abs(hi)));if(x<lo){require(lo-x<=epsilon,where+": finite-probe evaluation exceeds global recovery grade");uv.SetCoord(axis+1,lo);}else if(x>hi){require(x-hi<=epsilon,where+": finite-probe evaluation exceeds global recovery grade");uv.SetCoord(axis+1,hi);}}
 return uv;
}
void check_correspondence(const SourceUVFace& f,const SourceUVOptions& options,double& maximum,bool domain){auto surface=world_surface(f.face);for(auto& loop:f.loops)for(auto& u:loop.uses){double a,b;auto c=world_curve(u.edge,a,b);require(!c.IsNull(),context(f)+" use "+std::to_string(u.id)+": original 3D curve absent");auto pc=pcurve(u.edge,f.face);require(!pc.curve.IsNull()&&pc.first==a&&pc.last==b,context(f)+" use "+std::to_string(u.id)+": pcurve original-t mismatch");
  for(double t:samples(c,a,b)){auto uv=pc.curve->Value(t);require(finite(uv),context(f)+": nonfinite projected point");if(domain){auto s=Handle(Geom_BSplineSurface)::DownCast(surface);if(!s.IsNull())uv=spline_probe(pc,surface,uv,context(f)+" use "+std::to_string(u.id));GeomAdaptor_Surface sa(surface);if(sa.GetType()==GeomAbs_Sphere)require(uv.Y()>=-std::acos(-1.)/2&&uv.Y()<=std::acos(-1.)/2,context(f)+": query outside sphere V domain");}
    double residual=c->Value(t).Distance(surface->Value(uv.X(),uv.Y()));require(std::isfinite(residual)&&residual<=options.source_residual_mm,context(f)+" use "+std::to_string(u.id)+": C/S residual "+std::to_string(residual));maximum=std::max(maximum,residual);}}
}
void attach(const TopoDS_Edge& e,const TopoDS_Face& f,const Handle(Geom2d_Curve)& pc,double a,double b){BRep_Builder builder;builder.UpdateEdge(e,pc,f,0.);builder.Range(e,f,a,b);}
}
TopoDS_Shape PreparedSourceUV::mapped(const TopoDS_Shape& original)const{require(copies.IsBound(original),"unmapped original source node");return copies.Find(original).Oriented(original.Orientation());}
TopoDS_Shape PreparedSourceUV::query_mapped(const TopoDS_Shape& original)const{require(query_copies.IsBound(original),"unmapped original query source node");return query_copies.Find(original).Oriented(original.Orientation());}
void require_source_uv_uses(const SourceUVFace& f,const std::vector<SourceUVConnector>& connectors){
 std::vector<TopoDS_Wire> actual_wires;for(TopExp_Explorer w(f.face,TopAbs_WIRE);w.More();w.Next())actual_wires.push_back(TopoDS::Wire(w.Current()));
 require(actual_wires.size()==f.unexplained.size()+std::size_t(std::count_if(f.loops.begin(),f.loops.end(),[](const SourceUVLoop& l){return !l.wire.IsNull();})),context(f)+": native face loop inventory changed");
 for(const auto& w:f.unexplained)require(std::count_if(actual_wires.begin(),actual_wires.end(),[&](const TopoDS_Wire& a){return a.IsSame(w)&&a.Orientation()==w.Orientation();})==1,context(f)+": unexplained wire of a refused face changed");
 for(const auto& l:f.loops){
  require(!l.wire.IsNull()||l.pole_id,context(f)+" loop "+std::to_string(l.id)+": edge loop without native wire");
  if(l.wire.IsNull())continue;
  std::vector<TopoDS_Wire> matches;for(const auto& wire:actual_wires)if(wire.IsSame(l.wire)&&wire.Orientation()==l.wire.Orientation())matches.push_back(wire);
  require(matches.size()==1,context(f)+" loop "+std::to_string(l.id)+": actual native wire binding/orientation changed");
  if(l.pole_id){
   // A vertex loop has no uses. Its wire is OCCT's single degenerated pole
   // edge, or the natural bounds OCCT builds for a sole vertex-loop bound.
   require(l.uses.empty(),context(f)+" vertex loop "+std::to_string(l.id)+": uses changed");
   std::vector<TopoDS_Edge> edges;for(TopExp_Explorer e(matches[0],TopAbs_EDGE);e.More();e.Next())edges.push_back(TopoDS::Edge(e.Current()));
   bool pole_edge=false;if(edges.size()==1&&!l.pole.IsNull()){TopoDS_Vertex a,b;TopExp::Vertices(edges[0],a,b);double x,y;pole_edge=BRep_Tool::Degenerated(edges[0])&&BRep_Tool::Curve(edges[0],x,y).IsNull()&&a.IsSame(l.pole)&&b.IsSame(l.pole);}
   require(pole_edge||f.loops.size()==1,context(f)+" vertex loop "+std::to_string(l.id)+": native wire is neither the pole edge nor natural bounds");
   continue;}
  for(const auto& use:l.uses){TopoDS_Vertex a,b;TopExp::Vertices(use.edge,a,b,true);require(a.IsSame(use.start)&&b.IsSame(use.end),context(f)+" coedge "+std::to_string(use.id)+": native directed endpoints differ from original use");}
  require(!l.uses.empty(),context(f)+": empty original loop");Handle(ShapeExtend_WireData) data=new ShapeExtend_WireData(matches[0],false);std::vector<TopoDS_Edge> physical;std::set<const SourceUVConnector*> observed;
  for(int index=1;index<=data->NbEdges();++index){auto e=data->Edge(index);const SourceUVConnector* declared=nullptr;for(const auto& c:connectors)if(c.face_id==f.id&&c.loop_id==l.id&&c.edge.IsSame(e)){require(!declared,context(f)+": duplicate declared chart connector");declared=&c;}
   if(!declared){physical.push_back(e);continue;}
   const auto& c=*declared;require(e.Orientation()==c.edge.Orientation()&&observed.insert(declared).second,context(f)+": chart connector multiplicity/orientation changed");
   auto incoming=std::find_if(l.uses.begin(),l.uses.end(),[&](const SourceUVUse& u){return u.id==c.incoming_id;});require(incoming!=l.uses.end(),context(f)+": pole incoming use absent");auto outgoing=l.uses.begin()+(std::size_t(incoming-l.uses.begin())+1)%l.uses.size();auto actual_in=data->Edge(index==1?data->NbEdges():index-1),actual_out=data->Edge(index==data->NbEdges()?1:index+1);require(actual_in.IsSame(incoming->edge)&&actual_in.Orientation()==incoming->edge.Orientation()&&actual_out.IsSame(outgoing->edge)&&actual_out.Orientation()==outgoing->edge.Orientation(),context(f)+": chart connector actual neighbors differ from original joint");require(outgoing->id==c.outgoing_id&&incoming->end_id==c.vertex_id&&outgoing->start_id==c.vertex_id&&incoming->end.IsSame(outgoing->start),context(f)+": pole original joint ownership changed");
   double v;require(pole(world_surface(f.face),BRep_Tool::Pnt(incoming->end),v),context(f)+": connector has no original analytic pole");TopoDS_Vertex a,b;TopExp::Vertices(e,a,b,true);double x,y;auto curve=world_curve(e,x,y);auto pc=pcurve(e,f.face);auto linear=Handle(Geom2d_BSplineCurve)::DownCast(pc.curve);
   require(curve.IsNull()&&BRep_Tool::Degenerated(e)&&a.IsSame(incoming->end)&&b.IsSame(incoming->end)&&!linear.IsNull()&&linear->Degree()==1&&linear->NbPoles()==2&&!linear->IsRational(),context(f)+": unsupported physical chart quotient");
   auto p=pc.curve->Value(pc.first),q=pc.curve->Value(pc.last);require(p.X()==c.u0&&q.X()==c.u1&&p.Y()==v&&q.Y()==v&&c.v0==v&&c.v1==v&&std::abs(q.X()-p.X())<=2*std::acos(-1.)+grade(2*std::acos(-1.)),context(f)+": connector left proven canonical pole locus");
  }
  for(const auto& c:connectors)if(c.face_id==f.id&&c.loop_id==l.id)require(observed.count(&c),context(f)+": declared pole connector absent from actual wire");
  require(physical.size()==l.uses.size(),context(f)+" loop "+std::to_string(l.id)+": original-use multiplicity changed");
  std::vector<bool> matched(l.uses.size(),false);for(std::size_t i=0;i<physical.size();++i){auto e=physical[i];std::vector<std::size_t> candidates;for(std::size_t j=0;j<l.uses.size();++j)if(e.IsSame(l.uses[j].edge)&&e.Orientation()==l.uses[j].edge.Orientation())candidates.push_back(j);require(candidates.size()==1,context(f)+": ambiguous original oriented-use ownership");auto j=candidates[0];require(!matched[j],context(f)+": duplicate original oriented use");matched[j]=true;auto expected=l.uses[(j+1)%l.uses.size()].edge;auto next=physical[(i+1)%physical.size()];require(next.IsSame(expected)&&next.Orientation()==expected.Orientation(),context(f)+": original ordered edge-use word changed");}
  for(std::size_t i=0;i<l.uses.size();++i){const auto& a=l.uses[i];const auto& b=l.uses[(i+1)%l.uses.size()];require(a.id&&a.edge_id&&a.end_id==b.start_id&&a.end.IsSame(b.start),context(f)+": original directed vertex cycle is not closed");}
 }
}
void require_source_uv_period_translation(const Handle(Geom2d_Curve)& before,const Handle(Geom2d_Curve)& after,const Handle(Geom_Surface)& surface,double first,double last){
 require(!before.IsNull()&&!after.IsNull(),"period operation lost pcurve");auto p=before->Value(first),q=after->Value(first);gp_Vec2d shift(p,q);double period[2]={surface->IsUPeriodic()?surface->UPeriod():0.,surface->IsVPeriodic()?surface->VPeriod():0.};for(int axis=0;axis<2;++axis){double d=axis?shift.Y():shift.X();if(period[axis]){double n=std::nearbyint(d/period[axis]);require(std::isfinite(n)&&std::abs(d-n*period[axis])<=grade(std::max(std::abs(d),period[axis])),"shift is not a true carrier period");}else require(std::abs(d)<=grade(std::max(std::abs(p.Coord(axis+1)),std::abs(q.Coord(axis+1)))),"nonperiodic shift, including spherical V");}
 auto equal=[&](const gp_Pnt2d& a,const gp_Pnt2d& b){return std::abs((b.X()-a.X())-shift.X())<=grade(std::max(std::abs(a.X()),std::abs(b.X())))&&std::abs((b.Y()-a.Y())-shift.Y())<=grade(std::max(std::abs(a.Y()),std::abs(b.Y())));};
 auto a=Handle(Geom2d_Line)::DownCast(before),b=Handle(Geom2d_Line)::DownCast(after);if(!a.IsNull()&&!b.IsNull()){require(a->Direction().IsEqual(b->Direction(),0.)&&equal(a->Location(),b->Location()),"period operation changed line basis");return;}
 auto x=Handle(Geom2d_BSplineCurve)::DownCast(before),y=Handle(Geom2d_BSplineCurve)::DownCast(after);require(!x.IsNull()&&!y.IsNull()&&x->Degree()==y->Degree()&&x->NbPoles()==y->NbPoles()&&x->NbKnots()==y->NbKnots()&&x->IsPeriodic()==y->IsPeriodic(),"period operation changed pcurve basis");for(int i=1;i<=x->NbKnots();++i)require(x->Knot(i)==y->Knot(i)&&x->Multiplicity(i)==y->Multiplicity(i),"period operation changed pcurve knots");for(int i=1;i<=x->NbPoles();++i)require(x->Weight(i)==y->Weight(i)&&equal(x->Pole(i),y->Pole(i)),"period operation is not a constant whole-pcurve translation");require(equal(before->Value(last),after->Value(last)),"period operation changed endpoint");
}
namespace {
struct QueryFaceEdit {
 TopoDS_Face face;bool saved_free;
 explicit QueryFaceEdit(const TopoDS_Face& f):face(f),saved_free(f.Free()){face.Free(true);}
 ~QueryFaceEdit(){face.Free(saved_free);}
 QueryFaceEdit(const QueryFaceEdit&)=delete;
 QueryFaceEdit& operator=(const QueryFaceEdit&)=delete;
};
struct QueryBudget {
 const SourceUVOptions& options;SourceUVStats& stats;std::map<SourceID,double> cumulative;
 void charge(const SourceUVFace& f,const SourceUVUse& use,double amount){require(std::isfinite(amount)&&amount>=0,context(f)+": nonfinite query perturbation");double total=cumulative[use.id]+amount;if(amount>0)total=std::nextafter(total,INFINITY);require(total<=options.tolerance_mm/8,context(f)+" coedge "+std::to_string(use.id)+": cumulative query perturbation exceeds chord allocation");cumulative[use.id]=total;stats.max_query_uv_metric_mm=std::max(stats.max_query_uv_metric_mm,total);}
};
std::array<double,2> derivative_metric(const Handle(Geom_Surface)& surface,double cone_v0=0,double cone_v1=0){
 GeomAdaptor_Surface sa(surface);double lu=0,lv=0;
 if(sa.GetType()==GeomAbs_Plane){lu=lv=1;}
 else if(sa.GetType()==GeomAbs_Cylinder){lu=sa.Cylinder().Radius();lv=1;}
 else if(sa.GetType()==GeomAbs_Sphere){lu=lv=sa.Sphere().Radius();}
 else if(sa.GetType()==GeomAbs_Torus){lu=sa.Torus().MajorRadius()+sa.Torus().MinorRadius();lv=sa.Torus().MinorRadius();}
 else if(sa.GetType()==GeomAbs_Cone){auto cone=sa.Cone();lu=std::max(std::abs(cone.RefRadius()+cone_v0*std::sin(cone.SemiAngle())),std::abs(cone.RefRadius()+cone_v1*std::sin(cone.SemiAngle())));lv=1;}
 else {
  auto s=Handle(Geom_BSplineSurface)::DownCast(surface);require(!s.IsNull(),"unsupported query derivative carrier");
  // Positive-weight rational derivative control hull, recentered at one pole.
  std::vector<double> ku,kv;for(int i=1;i<=s->NbUKnots();++i)for(int j=0;j<s->UMultiplicity(i);++j)ku.push_back(s->UKnot(i));for(int i=1;i<=s->NbVKnots();++i)for(int j=0;j<s->VMultiplicity(i);++j)kv.push_back(s->VKnot(i));
  double wmin=INFINITY,amax=0;gp_Pnt origin=s->Pole(1,1);for(int i=1;i<=s->NbUPoles();++i)for(int j=1;j<=s->NbVPoles();++j){double w=s->Weight(i,j);require(w>0&&std::isfinite(w),"nonpositive query carrier weight");wmin=std::min(wmin,w);amax=std::max(amax,origin.Distance(s->Pole(i,j))*w);}
  for(int axis=0;axis<2;++axis){double da=0,dw=0;int n=axis?s->NbVPoles():s->NbUPoles(),m=axis?s->NbUPoles():s->NbVPoles(),p=axis?s->VDegree():s->UDegree();const auto& k=axis?kv:ku;
   for(int i=0;i<n-1;++i){double denominator=k[i+p+1]-k[i+1];require(denominator>0,"unsupported zero-span derivative hull");double factor=p/denominator;for(int j=1;j<=m;++j){int u=axis?j:i+1,v=axis?i+1:j,un=axis?j:i+2,vn=axis?i+2:j;double w=s->Weight(u,v),wn=s->Weight(un,vn);gp_Vec a(origin,s->Pole(u,v)),b(origin,s->Pole(un,vn));da=std::max(da,(b*wn-a*w).Magnitude()*factor);dw=std::max(dw,std::abs(wn-w)*factor);}}
   double value=(da+amax/wmin*dw)/wmin;if(axis)lv=value;else lu=value;
  }
 }
 require(std::isfinite(lu)&&std::isfinite(lv)&&lu>=0&&lv>=0,"nonfinite query derivative bound");return {std::nextafter(lu,INFINITY),std::nextafter(lv,INFINITY)};
}
Handle(Geom2d_BSplineCurve) line_span(const Handle(Geom2d_Curve)& curve,double a,double b){
 NCollection_Array1<gp_Pnt2d> poles(1,2);poles(1)=curve->Value(a);poles(2)=curve->Value(b);NCollection_Array1<double> knots(1,2);knots(1)=a;knots(2)=b;NCollection_Array1<int> mult(1,2);mult.Init(2);return new Geom2d_BSplineCurve(poles,knots,mult,1);
}
Handle(Geom2d_BSplineCurve) editable(const PC& value){
 auto c=Handle(Geom2d_BSplineCurve)::DownCast(value.curve);if(c.IsNull()){require(!Handle(Geom2d_Line)::DownCast(value.curve).IsNull(),"unsupported query endpoint pcurve representation");return line_span(value.curve,value.first,value.last);}
 require(!c->IsRational()&&!c->IsPeriodic(),"unsupported rational or periodic query endpoint edit");auto copy=Handle(Geom2d_BSplineCurve)::DownCast(c->Copy());copy->Segment(value.first,value.last);require(copy->FirstParameter()==value.first&&copy->LastParameter()==value.last,"endpoint edit changed original t");return copy;
}
Handle(Geom2d_Curve) conic_source_chart(const SourceUVFace& face,const SourceUVUse& use,
 const Handle(Geom_Curve)& original,const Handle(Geom_Surface)& surface,
 const Handle(Geom2d_Curve)& guide,double a,double b,bool seam,
 const SourceUVOptions& options,PreparedSourceUV& output){
 auto tensor=Handle(Geom_BSplineSurface)::DownCast(surface);if(tensor.IsNull())return guide;
 GeomAdaptor_Curve adaptor(original,a,b);if(adaptor.GetType()!=GeomAbs_Circle&&adaptor.GetType()!=GeomAbs_Ellipse)return guide;
 auto basis=Handle(Geom2d_BSplineCurve)::DownCast(guide);if(basis.IsNull()||basis->IsRational()||basis->IsPeriodic())return guide;auto selected=editable({guide,a,b});
 double bounds[4];surface->Bounds(bounds[0],bounds[1],bounds[2],bounds[3]);bool escaped=false;
 for(int i=1;i<=selected->NbPoles();++i)for(int axis=0;axis<2;++axis){double x=selected->Pole(i).Coord(axis+1),lo=bounds[2*axis],hi=bounds[2*axis+1],epsilon=grade(std::max(std::abs(lo),std::abs(hi)));if(x<lo-epsilon||x>hi+epsilon)escaped=true;}
 if(!escaped)return guide;
 auto where=context(face)+" coedge "+std::to_string(use.id)+" conic source chart";
 require(!seam&&std::count_if(edge_loop(face).uses.begin(),edge_loop(face).uses.end(),[&](const SourceUVUse& u){return u.edge.IsSame(use.edge);})==1,where+": unsupported repeated source use");
 require(!tensor->IsUPeriodic()&&!tensor->IsVPeriodic()&&tensor->UMultiplicity(1)==tensor->UDegree()+1&&tensor->UMultiplicity(tensor->NbUKnots())==tensor->UDegree()+1&&tensor->VMultiplicity(1)==tensor->VDegree()+1&&tensor->VMultiplicity(tensor->NbVKnots())==tensor->VDegree()+1,where+": unsupported unclamped or periodic carrier");
 auto first=selected->Value(a),last=selected->Value(b);require(finite(first)&&finite(last),where+": nonfinite guide endpoints");
 std::vector<int> sides;for(int k=0;k<4;++k){int axis=k/2;double epsilon=grade(std::max(std::abs(bounds[2*axis]),std::abs(bounds[2*axis+1])));if(std::abs(first.Coord(axis+1)-bounds[k])<=epsilon&&std::abs(last.Coord(axis+1)-bounds[k])<=epsilon)sides.push_back(k);}
 require(sides.size()==1,where+": ambiguous natural-side endpoints");int k=sides[0],axis=k/2,other=1-axis;double delta=0;
 for(int i=1;i<=selected->NbPoles();++i){auto p=selected->Pole(i);require(finite(p)&&p.Coord(other+1)>=bounds[2*other]&&p.Coord(other+1)<=bounds[2*other+1],where+": other-coordinate hull leaves original domain");delta=std::max(delta,std::abs(p.Coord(axis+1)-bounds[k]));p.SetCoord(axis+1,bounds[k]);selected->SetPole(i,p);}
 auto metric=derivative_metric(surface);double amount=std::nextafter(metric[axis]*delta,INFINITY);require(std::isfinite(amount)&&amount<=options.tolerance_mm/8,where+": complete guide perturbation exceeds chord allocation");
 TopoDS_Vertex va,vb;TopExp::Vertices(use.edge,va,vb);require(!va.IsNull()&&!vb.IsNull(),where+": original geometric endpoints missing");
 for(int endpoint=0;endpoint<2;++endpoint){double t=endpoint?b:a;auto vertex=BRep_Tool::Pnt(endpoint?vb:va);auto uv=spline_probe({selected,a,b},surface,selected->Value(t),where);auto cp=original->Value(t),sp=surface->Value(uv.X(),uv.Y());require(cp.Distance(vertex)<=options.source_residual_mm&&sp.Distance(vertex)<=options.source_residual_mm,where+": original endpoint correspondence exceeds source grade");}
 SourceUVChartApproximation record;record.face_id=face.id;record.coedge_id=use.id;record.axis=axis;record.side=bounds[k];record.max_guide_coefficient_delta=delta;record.uv_metric_mm=amount;
 for(double t:samples(original,a,b)){auto p=original->Value(t);auto raw=guide->Value(t),uv=spline_probe({selected,a,b},surface,selected->Value(t),where);require(finite(raw),where+": nonfinite original guide probe");double raw_residual=p.Distance(surface->Value(raw.X(),raw.Y())),residual=p.Distance(surface->Value(uv.X(),uv.Y()));require(std::isfinite(raw_residual)&&std::isfinite(residual)&&residual<=options.source_residual_mm,where+": selected original-t correspondence exceeds source grade");record.max_guide_extension_residual_mm=std::max(record.max_guide_extension_residual_mm,raw_residual);record.max_selected_residual_mm=std::max(record.max_selected_residual_mm,residual);}
 output.chart_approximations.push_back(record);return selected;
}
// B2 .37.31. A B-spline surface is defined on its knot rectangle only: a UV
// point outside it is no point of the carrier but the end span's polynomial
// extended. A projected UV curve is a fit through in-domain projections and can
// overshoot the rectangle between them (or OCCT's 2D line can sit past a side).
// Such a curve is not a curve on the carrier, and every consumer of a B-spline
// carrier works on the knot rectangle, so the curve is corrected, never the
// carrier's hint widened. Every coefficient of the curve on its interval is
// moved into the rectangle (the curve then lies in it by the convex hull
// property; one function of the coefficient value, so curves meeting at a
// joint still meet). The guarantee is the sampled original-t correspondence
// of the corrected curve, which must stay within the source residual (at the
// samples, not between them). The derivative-metric term is a charge to the
// chord allocation, as a conic chart's is, and not a bound: the metric is taken
// over the knot rectangle while the original curve's escaped points lie off it,
// on the end spans' polynomial extension (review 9b). A curve whose own sampled
// residual already exceeds the source residual is left as it is, so the C/S
// residual check names that cause, as before (review 5). Escapes within the
// recovery grade are left to the finite probe, as before.
Handle(Geom2d_Curve) domain_source_chart_impl(const SourceUVFace& face,const SourceUVUse& use,
 const Handle(Geom_Curve)& original,const Handle(Geom_Surface)& surface,
 const Handle(Geom2d_Curve)& guide,double a,double b,
 const SourceUVOptions& options,PreparedSourceUV& output){
 auto tensor=Handle(Geom_BSplineSurface)::DownCast(surface);if(tensor.IsNull())return guide;
 // A curve whose own sampled residual fails is left for the C/S residual check.
 for(double t:samples(original,a,b)){auto uv=guide->Value(t);if(!finite(uv))return guide;if(!(original->Value(t).Distance(surface->Value(uv.X(),uv.Y()))<=options.source_residual_mm))return guide;}
 // Only the forms editable() takes; any other projected form is left as it was.
 auto spline=Handle(Geom2d_BSplineCurve)::DownCast(guide);if(spline.IsNull()?Handle(Geom2d_Line)::DownCast(guide).IsNull():(spline->IsRational()||spline->IsPeriodic())){require_natural_domain(guide,a,b,surface,face.id,use.id);return guide;}
 double bounds[4];surface->Bounds(bounds[0],bounds[1],bounds[2],bounds[3]);
 auto where=context(face)+" coedge "+std::to_string(use.id)+" natural domain";
 auto selected=editable({guide,a,b});bool escaped=false;
 for(int i=1;i<=selected->NbPoles();++i)for(int axis=0;axis<2;++axis){double x=selected->Pole(i).Coord(axis+1),lo=bounds[2*axis],hi=bounds[2*axis+1],epsilon=grade(std::max(std::abs(lo),std::abs(hi)));require(std::isfinite(x),where+": nonfinite coefficient");if(x<lo-epsilon||x>hi+epsilon)escaped=true;}
 if(!escaped)return guide;
 require(!tensor->IsUPeriodic()&&!tensor->IsVPeriodic()&&!tensor->IsUClosed()&&!tensor->IsVClosed(),where+": unsupported closed or periodic carrier");
 require(std::count_if(edge_loop(face).uses.begin(),edge_loop(face).uses.end(),[&](const SourceUVUse& u){return u.edge.IsSame(use.edge);})==1,where+": unsupported repeated source use");
 SourceUVDomainClamp record;record.face_id=face.id;record.coedge_id=use.id;
 for(int i=1;i<=selected->NbPoles();++i){auto p=selected->Pole(i);for(int axis=0;axis<2;++axis){double x=p.Coord(axis+1),y=std::min(std::max(x,bounds[2*axis]),bounds[2*axis+1]);(axis?record.max_v_move:record.max_u_move)=std::max(axis?record.max_v_move:record.max_u_move,std::abs(y-x));p.SetCoord(axis+1,y);}selected->SetPole(i,p);}
 auto metric=derivative_metric(surface);record.uv_metric_mm=std::nextafter(metric[0]*record.max_u_move+metric[1]*record.max_v_move,INFINITY);
 require(std::isfinite(record.uv_metric_mm)&&record.uv_metric_mm<=options.tolerance_mm/8,where+": complete domain correction exceeds chord allocation");
 for(double t:samples(original,a,b)){auto uv=selected->Value(t);require(finite(uv),where+": nonfinite correction probe");double residual=original->Value(t).Distance(surface->Value(uv.X(),uv.Y()));require(std::isfinite(residual)&&residual<=options.source_residual_mm,where+": corrected original-t correspondence exceeds source grade");record.max_residual_mm=std::max(record.max_residual_mm,residual);}
 output.domain_clamps.push_back(record);return selected;
}
void replace_branch(const SourceUVUse& use,const TopoDS_Face& face,const Handle(Geom2d_Curve)& value,double a,double b){
 BRep_Builder builder;if(!BRep_Tool::IsClosed(use.edge,face)){attach(use.edge,face,value,a,b);return;}
 auto other=pcurve(TopoDS::Edge(use.edge.Reversed()),face);require(!other.curve.IsNull(),"query edit lost other seam branch");bool second=(use.edge.Orientation()==TopAbs_REVERSED)!=(face.Orientation()==TopAbs_REVERSED);
 builder.UpdateEdge(use.edge,second?other.curve:value,second?value:other.curve,face,0.);builder.Range(use.edge,face,a,b);
}
double endpoint_t(const SourceUVUse& use,bool start){double a,b;BRep_Tool::Range(use.edge,a,b);bool starts_first=use.edge.Orientation()==TopAbs_FORWARD;return (start==starts_first)?a:b;}
void edit_endpoint(const SourceUVFace& face,const SourceUVUse& use,bool start,const gp_Pnt2d& uv,const SourceUVOptions& options,SourceUVStats& stats,QueryBudget& budget){
 auto old=pcurve(use.edge,face.face);auto curve=editable(old);double t=endpoint_t(use,start);int index=t==old.first?1:curve->NbPoles();require(t==old.first||t==old.last,context(face)+": endpoint not original interval end");auto previous=curve->Pole(index);double vmin=uv.Y(),vmax=uv.Y();for(int i=1;i<=curve->NbPoles();++i){vmin=std::min(vmin,curve->Pole(i).Y());vmax=std::max(vmax,curve->Pole(i).Y());}auto metric=derivative_metric(world_surface(face.face),vmin,vmax);double bound=metric[0]*std::abs(uv.X()-previous.X())+metric[1]*std::abs(uv.Y()-previous.Y());require(std::isfinite(bound)&&bound<=options.tolerance_mm/8,context(face)+" coedge "+std::to_string(use.id)+": query endpoint perturbation exceeds chord allocation");budget.charge(face,use,bound);curve->SetPole(index,uv);replace_branch(use,face.face,curve,old.first,old.last);++stats.query_joint_endpoints;
}
struct Column {double u,v0,v1;gp_Pnt a,b;int axis;};
std::vector<Column> columns(const Handle(Geom_BSplineSurface)& s){std::vector<Column> out;if(s.IsNull()||s->IsUPeriodic()||s->IsVPeriodic()||s->VDegree()!=1||s->NbVPoles()!=2||s->NbVKnots()!=2||s->VMultiplicity(1)!=2||s->VMultiplicity(2)!=2||s->UMultiplicity(1)!=s->UDegree()+1||s->UMultiplicity(s->NbUKnots())!=s->UDegree()+1)return out;
 for(int side=0;side<2;++side){int row=side?s->NbUPoles():1;if(s->Weight(row,1)<=0||s->Weight(row,1)!=s->Weight(row,2))continue;auto a=s->Pole(row,1),b=s->Pole(row,2);int axis=0;for(int j=1;j<=3;++j)if(a.Coord(j)!=b.Coord(j)){if(axis){axis=-1;break;}axis=j;}if(axis>0)out.push_back({s->UKnot(side?s->NbUKnots():1),s->VKnot(1),s->VKnot(2),a,b,axis});}return out;
}
bool column_point(const Column& c,const gp_Pnt& p,double& v){for(int j=1;j<=3;++j)if(j!=c.axis&&p.Coord(j)!=c.a.Coord(j))return false;v=c.v0+(p.Coord(c.axis)-c.a.Coord(c.axis))*(c.v1-c.v0)/(c.b.Coord(c.axis)-c.a.Coord(c.axis));return std::isfinite(v)&&v>=c.v0&&v<=c.v1;}
std::vector<double> guide_u(const PC& pc){std::vector<double> out;if(!Handle(Geom2d_Line)::DownCast(pc.curve).IsNull())return {pc.curve->Value(pc.first).X(),pc.curve->Value(pc.last).X()};auto c=Handle(Geom2d_BSplineCurve)::DownCast(pc.curve);require(!c.IsNull(),"unsupported source-column branch guide");for(int i=1;i<=c->NbPoles();++i){require(c->Weight(i)>0,"nonpositive source-column guide weight");out.push_back(c->Pole(i).X());}return out;}
std::map<SourceID,Column> query_columns(SourceUVFace& face,const SourceUVOptions& options,SourceUVStats& stats,QueryBudget& budget){
 auto surface=world_surface(face.face);auto s=Handle(Geom_BSplineSurface)::DownCast(surface);auto models=columns(s);std::map<SourceID,Column> chosen;if(models.empty())return chosen;
 for(auto& loop:face.loops)for(auto& use:loop.uses){double a,b;auto curve=world_curve(use.edge,a,b);auto line=Handle(Geom_Line)::DownCast(curve);if(line.IsNull())continue;std::vector<Column> admitted;auto pc=pcurve(use.edge,face.face);auto guide=guide_u(pc);double half=(s->UKnot(s->NbUKnots())-s->UKnot(1))/2;
  for(const auto& c:models){double ignored;auto origin=line->Position().Location();auto direction=line->Position().Direction();bool exact=true;for(int j=1;j<=3;++j)if(j!=c.axis&&(origin.Coord(j)!=c.a.Coord(j)||direction.Coord(j)!=0))exact=false;if(!exact||direction.Coord(c.axis)==0)continue;if(!column_point(c,curve->Value(a),ignored)||!column_point(c,curve->Value(b),ignored)||!column_point(c,BRep_Tool::Pnt(use.start),ignored)||!column_point(c,BRep_Tool::Pnt(use.end),ignored))continue;if(std::all_of(guide.begin(),guide.end(),[&](double u){return std::abs(u-c.u)<half;}))admitted.push_back(c);}
  if(admitted.empty())continue;require(admitted.size()==1,context(face)+": ambiguous query natural source column");auto c=admitted[0];chosen.emplace(use.id,c);auto replacement=line_span(pc.curve,a,b);double va,vb;require(column_point(c,curve->Value(a),va)&&column_point(c,curve->Value(b),vb),context(face)+": source column endpoint escaped");replacement->SetPole(1,gp_Pnt2d(c.u,va));replacement->SetPole(2,gp_Pnt2d(c.u,vb));auto metric=derivative_metric(surface);double du=0,dv=0;auto old=editable(pc);for(int i=1;i<=old->NbPoles();++i){double t=0;for(int j=1;j<=old->Degree();++j)t+=old->KnotSequence()(i+j)/old->Degree();auto expected=replacement->Value(t);du=std::max(du,std::abs(expected.X()-old->Pole(i).X()));dv=std::max(dv,std::abs(expected.Y()-old->Pole(i).Y()));}double bound=metric[0]*du+metric[1]*dv;require(std::isfinite(bound)&&bound<=options.tolerance_mm/8,context(face)+": source column query perturbation exceeds allocation");budget.charge(face,use,bound);replace_branch(use,face.face,replacement,a,b);++stats.query_column_edges;
 }
 for(auto& loop:face.loops)for(std::size_t i=0;i<loop.uses.size();++i){auto& in=loop.uses[i];auto& out=loop.uses[(i+1)%loop.uses.size()];for(int side=0;side<2;++side){const auto& provider=side?out:in;auto it=chosen.find(provider.id);if(it==chosen.end())continue;const auto& neighbor=side?in:out;if(!chosen.count(neighbor.id)){double x,y;auto curve=world_curve(neighbor.edge,x,y);if(!Handle(Geom_Line)::DownCast(curve).IsNull())continue;}const auto& c=it->second;double v;require(column_point(c,BRep_Tool::Pnt(in.end),v),context(face)+": source joint left declared column");auto raw=pcurve((side?in:out).edge,face.face);auto seed=raw.curve->Value(endpoint_t(side?in:out,!side));require(std::abs(seed.X()-c.u)<(s->UKnot(s->NbUKnots())-s->UKnot(1))/2,context(face)+": source joint column branch ambiguous");auto uv=gp_Pnt2d(c.u,v);edit_endpoint(face,in,false,uv,options,stats,budget);edit_endpoint(face,out,true,uv,options,stats,budget);}}
 return chosen;
}
void finite_spline_joints(SourceUVFace& face,const std::map<SourceID,Column>& providers,const SourceUVOptions& options,SourceUVStats& stats,QueryBudget& budget){
 auto surface=world_surface(face.face);auto s=Handle(Geom_BSplineSurface)::DownCast(surface);if(s.IsNull()||s->IsUPeriodic()||s->IsVPeriodic()||s->UMultiplicity(1)!=s->UDegree()+1||s->UMultiplicity(s->NbUKnots())!=s->UDegree()+1||s->VMultiplicity(1)!=s->VDegree()+1||s->VMultiplicity(s->NbVKnots())!=s->VDegree()+1)return;
 for(auto& loop:face.loops)for(std::size_t i=0;i<loop.uses.size();++i){auto& a=loop.uses[i];auto& b=loop.uses[(i+1)%loop.uses.size()];if(providers.count(a.id)||providers.count(b.id))continue;auto pa=pcurve(a.edge,face.face),pb=pcurve(b.edge,face.face);double ta=endpoint_t(a,false),tb=endpoint_t(b,true);auto ua=pa.curve->Value(ta),ub=pb.curve->Value(tb);if(ua.Distance(ub)<=std::ldexp(1.,-32)*std::max({1.,std::abs(ua.X()),std::abs(ub.X())}))continue;bool domain=true;for(auto uv:{ua,ub})if(uv.X()<s->UKnot(1)||uv.X()>s->UKnot(s->NbUKnots())||uv.Y()<s->VKnot(1)-grade(s->VKnot(1))||uv.Y()>s->VKnot(s->NbVKnots())+grade(s->VKnot(s->NbVKnots())))domain=false;if(!domain)continue;bool cell=false;for(int k=1;k<s->NbUKnots();++k)if(ua.X()>=s->UKnot(k)&&ub.X()>=s->UKnot(k)&&ua.X()<=s->UKnot(k+1)&&ub.X()<=s->UKnot(k+1))cell=true;bool v_end=false;for(double v:{s->VKnot(1),s->VKnot(s->NbVKnots())})if(std::abs(ua.Y()-v)<=grade(v)&&std::abs(ub.Y()-v)<=grade(v))v_end=true;if(!cell||!v_end)continue;
  auto point=BRep_Tool::Pnt(a.end);double af,al,bf,bl;auto ca=world_curve(a.edge,af,al),cb=world_curve(b.edge,bf,bl);auto xa=surface->Value(ua.X(),ua.Y()),xb=surface->Value(ub.X(),ub.Y());double da=xa.Distance(point),db=xb.Distance(point);require(ca->Value(ta).Distance(point)<=options.source_residual_mm&&cb->Value(tb).Distance(point)<=options.source_residual_mm&&xa.Distance(ca->Value(ta))<=options.source_residual_mm&&xb.Distance(cb->Value(tb))<=options.source_residual_mm&&std::min(da,db)<=options.source_residual_mm,context(face)+": finite joint exceeds source residual");auto chosen=da<=db?ua:ub;edit_endpoint(face,a,false,chosen,options,stats,budget);edit_endpoint(face,b,true,chosen,options,stats,budget);
 }
}
bool pole(const Handle(Geom_Surface)& surface,const gp_Pnt& p,double& v){GeomAdaptor_Surface sa(surface);if(sa.GetType()==GeomAbs_Sphere){auto s=sa.Sphere();for(double sign:{-1.,1.})if(p.Distance(s.Location().Translated(gp_Vec(s.Position().Direction())*(sign*s.Radius())))<=std::ldexp(1.,-32)){v=sign*std::acos(-1.)/2;return true;}}
 if(sa.GetType()==GeomAbs_Cone){auto c=sa.Cone();v=-c.RefRadius()/std::sin(c.SemiAngle());if(p.Distance(c.Location().Translated(gp_Vec(c.Axis().Direction())*(v*std::cos(c.SemiAngle()))))<=std::ldexp(1.,-32))return true;}return false;}
void query_poles(SourceUVFace& face,PreparedSourceUV& output,const SourceUVOptions& options,QueryBudget& budget){auto surface=world_surface(face.face);BRep_Builder builder;
 for(auto& loop:face.loops){std::map<SourceID,TopoDS_Edge> after;for(std::size_t i=0;i<loop.uses.size();++i){auto& in=loop.uses[i];auto& out=loop.uses[(i+1)%loop.uses.size()];double v;if(!pole(surface,BRep_Tool::Pnt(in.end),v))continue;auto a=pcurve(in.edge,face.face),b=pcurve(out.edge,face.face);auto ua=a.curve->Value(endpoint_t(in,false)),ub=b.curve->Value(endpoint_t(out,true));edit_endpoint(face,in,false,gp_Pnt2d(ua.X(),v),options,output.stats,budget);edit_endpoint(face,out,true,gp_Pnt2d(ub.X(),v),options,output.stats,budget);if(std::abs(ua.X()-ub.X())<=grade(std::max(std::abs(ua.X()),std::abs(ub.X()))))continue;require(std::abs(ua.X()-ub.X())<=2*std::acos(-1.)+grade(2*std::acos(-1.)),context(face)+": pole branch exceeds one period");TopoDS_Edge edge;builder.MakeEdge(edge);auto first=in.end.Oriented(TopAbs_FORWARD),last=in.end.Oriented(TopAbs_REVERSED);builder.Add(edge,first);builder.Add(edge,last);builder.Degenerated(edge,true);NCollection_Array1<gp_Pnt2d> poles(1,2);poles(1)=gp_Pnt2d(ua.X(),v);poles(2)=gp_Pnt2d(ub.X(),v);NCollection_Array1<double> knots(1,2);knots(1)=0;knots(2)=1;NCollection_Array1<int> mult(1,2);mult.Init(2);Handle(Geom2d_Curve) pc=new Geom2d_BSplineCurve(poles,knots,mult,1);attach(edge,face.face,pc,0,1);after.emplace(in.id,edge);output.connectors.push_back({face.id,loop.id,in.end_id,in.id,out.id,ua.X(),v,ub.X(),v,edge});++output.stats.query_connectors;}
  if(after.empty())continue;std::vector<TopoDS_Edge> sequence;for(const auto& u:loop.uses){sequence.push_back(u.edge);if(after.count(u.id))sequence.push_back(after.at(u.id));}if(loop.wire.Orientation()==TopAbs_REVERSED){std::reverse(sequence.begin(),sequence.end());for(auto& e:sequence)e.Reverse();}TopoDS_Wire new_wire;builder.MakeWire(new_wire);for(auto& e:sequence)builder.Add(new_wire,e);new_wire.Orientation(loop.wire.Orientation());QueryFaceEdit edit(face.face);builder.Remove(face.face,loop.wire);try{builder.Add(face.face,new_wire);}catch(const Standard_Failure& e){fail(context(face)+" add query pole wire, parentFree="+std::to_string(face.face.Free())+": "+e.ExceptionType()+" "+e.what());}for(decltype(output.query_copies)::Iterator it(output.query_copies);it.More();it.Next())if(it.Value().IsSame(loop.wire))it.ChangeValue()=new_wire;loop.wire=new_wire;
 }
}
void query_domain(SourceUVFace& face,const SourceUVOptions& options,SourceUVStats& stats,QueryBudget& budget){auto surface=world_surface(face.face);auto s=Handle(Geom_BSplineSurface)::DownCast(surface);if(s.IsNull())return;double bounds[4];surface->Bounds(bounds[0],bounds[1],bounds[2],bounds[3]);for(auto& loop:face.loops)for(auto& use:loop.uses){auto pc=pcurve(use.edge,face.face);auto c=editable(pc);bool changed=false;double du=0,dv=0;for(int i=1;i<=c->NbPoles();++i){auto p=c->Pole(i),q=p;for(int axis=0;axis<2;++axis){double x=p.Coord(axis+1),lo=bounds[2*axis],hi=bounds[2*axis+1],epsilon=grade(std::max(std::abs(lo),std::abs(hi)));if(x<lo){require(lo-x<=epsilon,context(face)+" coedge "+std::to_string(use.id)+": unsupported query natural-domain escape");q.SetCoord(axis+1,lo);}if(x>hi){require(x-hi<=epsilon,context(face)+" coedge "+std::to_string(use.id)+": unsupported query natural-domain escape");q.SetCoord(axis+1,hi);}}if(!p.IsEqual(q,0)){changed=true;du=std::max(du,std::abs(p.X()-q.X()));dv=std::max(dv,std::abs(p.Y()-q.Y()));c->SetPole(i,q);}}if(changed){auto metric=derivative_metric(surface);double bound=metric[0]*du+metric[1]*dv;require(bound<=options.tolerance_mm/8,context(face)+": global-domain query correction exceeds allocation");budget.charge(face,use,bound);replace_branch(use,face.face,c,pc.first,pc.last);}}
}
}

double require_natural_domain(const Handle(Geom2d_Curve)& curve,double a,double b,const Handle(Geom_Surface)& surface,SourceID face_id,SourceID coedge_id){
 auto tensor=Handle(Geom_BSplineSurface)::DownCast(surface);if(tensor.IsNull()||curve.IsNull())return 0;
 double bounds[4];surface->Bounds(bounds[0],bounds[1],bounds[2],bounds[3]);Bnd_Box2d box;BndLib_Add2dCurve::Add(curve,a,b,0.,box);require(!box.IsVoid(),"face "+std::to_string(face_id)+" coedge "+std::to_string(coedge_id)+": empty UV curve box");
 double lo[2],hi[2];box.Get(lo[0],lo[1],hi[0],hi[1]);double escape=0;bool beyond=false;
 for(int axis=0;axis<2;++axis){double e=std::max(bounds[2*axis]-lo[axis],hi[axis]-bounds[2*axis+1]);escape=std::max(escape,e);if(e>grade(std::max(std::abs(bounds[2*axis]),std::abs(bounds[2*axis+1]))))beyond=true;}
 if(!beyond)return std::max(escape,0.);
 Handle(Geom2d_Curve) basis=curve;bool trimmed=false;while(auto t=Handle(Geom2d_TrimmedCurve)::DownCast(basis)){basis=t->BasisCurve();trimmed=true;}
 Geom2dAdaptor_Curve adaptor(basis);std::uint32_t kind=0;bool rational=false,periodic=basis->IsPeriodic();const char* name="curve of another form";
 switch(adaptor.GetType()){case GeomAbs_Line:kind=1;name="line";break;case GeomAbs_Circle:kind=2;name="circle";break;case GeomAbs_Ellipse:kind=3;name="ellipse";break;
  case GeomAbs_BezierCurve:kind=4;rational=adaptor.Bezier()->IsRational();name="Bezier curve";break;case GeomAbs_BSplineCurve:kind=5;rational=adaptor.BSpline()->IsRational();periodic=adaptor.BSpline()->IsPeriodic();name="B-spline curve";break;default:break;}
 std::ostringstream message;message<<std::setprecision(17)<<"source UV: face "<<face_id<<" coedge "<<coedge_id<<" natural domain: projected "<<(rational?"rational ":"")<<(periodic?"periodic ":"")<<(trimmed?"trimmed ":"")<<name<<" leaves the B-spline carrier's knot rectangle by "<<escape<<" and is not a form the domain correction takes; refused";
 throw SourceUVDomainEscape(message.str(),face_id,coedge_id,kind,rational,periodic,trimmed,escape);
}
PreparedSourceUV prepare_source_uv(const TopoDS_Shape& original,const std::vector<SourceUVFace>& bindings,const SourceUVOptions& options){
 require(std::isfinite(options.projection_precision_mm)&&options.projection_precision_mm>0&&std::isfinite(options.source_residual_mm)&&options.source_residual_mm>0&&std::isfinite(options.tolerance_mm)&&options.tolerance_mm>0,"invalid source conversion grades");auto begin=Clock::now();for(const auto& f:bindings)require_source_uv_uses(f);BRepBuilderAPI_Copy copier(original,true,false);PreparedSourceUV output;output.raw_shape=copier.Shape();auto faces=mapped_faces(bindings,copier);
 auto remember=[&](const TopoDS_Shape& s){if(!output.copies.IsBound(s))output.copies.Bind(s,copier.ModifiedShape(s));};remember(original);for(const auto& f:bindings){remember(f.face);for(const auto& w:f.unexplained)remember(w);for(const auto& l:f.loops){if(!l.wire.IsNull())remember(l.wire);if(!l.pole.IsNull())remember(l.pole);for(const auto& u:l.uses){remember(u.edge);remember(u.start);remember(u.end);}}}
 std::vector<EdgeState> states;NCollection_IndexedMap<TopoDS_Shape,TopTools_ShapeMapHasher> state_index;
 std::vector<FaceState> face_states;std::map<SourceID,std::size_t> face_state_index;for(const auto& f:faces){face_state_index[f.id]=face_states.size();face_states.emplace_back(f.face);for(const auto& l:f.loops)for(const auto& u:l.uses)if(!state_index.Contains(u.edge)){state_index.Add(u.edge);states.emplace_back(u.edge);}}
 struct Support{TopoDS_Edge edge;Handle(Geom_Surface) surface;TopLoc_Location location;SourceID face_id;};std::vector<std::vector<Support>> supports;NCollection_IndexedMap<TopoDS_Shape,TopTools_ShapeMapHasher> support_storage_index;
 for(const auto& f:faces){TopLoc_Location l;auto s=BRep_Tool::Surface(f.face,l);for(const auto& loop:f.loops)for(const auto& u:loop.uses){auto relative=l.Predivided(u.edge.Location());auto key=u.edge.Located(TopLoc_Location());int slot=support_storage_index.Add(key);if(std::size_t(slot)>supports.size())supports.resize(slot);auto& on_edge=supports.at(slot-1);for(const auto& previous:on_edge)if(previous.face_id!=f.id&&previous.edge.TShape()==u.edge.TShape()&&previous.surface==s&&previous.location==relative)fail(context(f)+": unsupported cross-face edge/support alias with face "+std::to_string(previous.face_id));on_edge.push_back({u.edge,s,relative,f.id});}}
 struct FinalPC{SourceID face_id;TopoDS_Face face;SourceUVUse use;PC value;};std::vector<FinalPC> final_pcurves;
 for(auto& f:faces){require_source_uv_uses(f);auto surface=world_surface(f.face);GeomAdaptor_Surface sa(surface);if(sa.GetType()==GeomAbs_Plane)continue;
  // Vertex loops carry no pcurve, so only edge loops need relative period
  // placement; a face bounded by vertex loops alone has nothing to project.
  const auto edge_loops=std::count_if(f.loops.begin(),f.loops.end(),[](const SourceUVLoop& l){return !l.pole_id;});if(!edge_loops)continue;
  require(edge_loops==1,context(f)+": unsupported nonplanar multiple-wire conversion");auto bs=Handle(Geom_BSplineSurface)::DownCast(surface);require(bs.IsNull()||(!bs->IsUClosed()&&!bs->IsVClosed()&&!bs->IsUPeriodic()&&!bs->IsVPeriodic()),context(f)+": unsupported closed/periodic spline chart");Handle(ShapeAnalysis_Surface) analysis=new ShapeAnalysis_Surface(surface);Handle(ShapeConstruct_ProjectCurveOnSurface) projector=new ShapeConstruct_ProjectCurveOnSurface;projector->Init(analysis,options.projection_precision_mm);projector->AdjustOverDegenMode()=1;
  auto& l=edge_loop(f);NCollection_IndexedMap<TopoDS_Shape,TopTools_ShapeMapHasher> projected;for(auto& u:l.uses){++output.stats.original_uses;if(projected.Contains(u.edge))continue;projected.Add(u.edge);bool seam=std::count_if(l.uses.begin(),l.uses.end(),[&](const SourceUVUse& b){return u.edge.IsSame(b.edge);})==2;auto pc=pcurve(u.edge,f.face);if(pc.curve.IsNull()){double a,b;auto c=world_curve(u.edge,a,b);require(!c.IsNull()&&std::isfinite(a)&&std::isfinite(b)&&a<b,context(f)+": original curve/range missing");Handle(Geom2d_Curve) result;bool okay=projector->Perform(c,a,b,result,BRep_Tool::Tolerance(TopExp::FirstVertex(u.edge)),BRep_Tool::Tolerance(TopExp::LastVertex(u.edge)));require(okay&&!projector->Status(ShapeExtend_FAIL)&&!result.IsNull(),context(f)+" coedge "+std::to_string(u.id)+": projection failed");result=conic_source_chart(f,u,c,surface,result,a,b,seam,options,output);if(!seam)result=domain_source_chart_impl(f,u,c,surface,result,a,b,options,output);else require_natural_domain(result,a,b,surface,f.id,u.id);attach(u.edge,f.face,result,a,b);++output.stats.projected_edges;}
   if(seam&&!BRep_Tool::IsClosed(u.edge,f.face)){ShapeAnalysis_Edge edge_analysis;require(edge_analysis.HasPCurve(u.edge,f.face)&&!pcurve(u.edge,f.face).curve.IsNull(),context(f)+": seam single-pcurve readback missing");ShapeFix_Edge fix;fix.FixAddPCurve(u.edge,f.face,true,analysis,options.projection_precision_mm);require(!fix.Status(ShapeExtend_FAIL)&&BRep_Tool::IsClosed(u.edge,f.face),context(f)+": seam pair failed");++output.stats.seam_pairs;}
   states.at(state_index.FindIndex(u.edge)-1).check(context(f));
  }
  auto working_wire=l.wire;if(f.face.Orientation()==TopAbs_REVERSED)working_wire.Reverse();Handle(ShapeExtend_WireData) data=new ShapeExtend_WireData(working_wire,true);ShapeFix_Wire wire;wire.Load(data);auto forward=TopoDS::Face(f.face.Oriented(TopAbs_FORWARD));wire.SetFace(forward);wire.SetPrecision(options.projection_precision_mm);for(int i=1;i<=data->NbEdges();++i){wire.FixSeam(i);require(!wire.LastFixStatus(ShapeExtend_FAIL),context(f)+": seam ordering failed");}
  std::vector<PC> previous;for(const auto& u:l.uses){auto p=pcurve(u.edge,f.face);p.curve=copy2(p.curve);previous.push_back(p);}wire.FixShifted();require(!wire.LastFixStatus(ShapeExtend_FAIL),context(f)+": period alignment failed");require_source_uv_uses(f);for(std::size_t i=0;i<l.uses.size();++i){auto after=pcurve(l.uses[i].edge,f.face);require(previous[i].first==after.first&&previous[i].last==after.last,context(f)+": period operation changed source interval");require_source_uv_period_translation(previous[i].curve,after.curve,surface,after.first,after.last);}for(const auto& u:l.uses){states.at(state_index.FindIndex(u.edge)-1).check(context(f));auto value=pcurve(u.edge,f.face);value.curve=copy2(value.curve);final_pcurves.push_back({f.id,f.face,u,value});}
  face_states.at(face_state_index.at(f.id)).check(context(f));
 }
 for(const auto& state:states)state.check("final representation readback");
 for(const auto& state:face_states)state.check("final representation readback");
 for(const auto& saved:final_pcurves){auto current=pcurve(saved.use.edge,saved.face);require(current.first==saved.value.first&&current.last==saved.value.last,("face "+std::to_string(saved.face_id))+": later support operation changed range");require_source_uv_period_translation(saved.value.curve,current.curve,world_surface(saved.face),current.first,current.last);auto a=saved.value.curve->Value(current.first),b=current.curve->Value(current.first);require(std::abs(a.X()-b.X())<=grade(std::max(std::abs(a.X()),std::abs(b.X())))&&std::abs(a.Y()-b.Y())<=grade(std::max(std::abs(a.Y()),std::abs(b.Y()))),("face "+std::to_string(saved.face_id))+": later operation overwrote original branch");}
 for(const auto& f:faces)check_correspondence(f,options,output.stats.max_residual_mm,false);
 output.stats.projection_seconds=std::chrono::duration<double>(Clock::now()-begin).count();
 // Query-only source-linked endpoint/column/pole construction follows below.
 if(options.prepare_query){auto query_begin=Clock::now();BRepBuilderAPI_Copy query_copier(output.raw_shape,true,false);output.query_shape=query_copier.Shape();for(decltype(output.copies)::Iterator it(output.copies);it.More();it.Next())output.query_copies.Bind(it.Key(),query_copier.ModifiedShape(it.Value()));auto queries=mapped_faces(faces,query_copier);std::vector<EdgeState> query_states;NCollection_IndexedMap<TopoDS_Shape,TopTools_ShapeMapHasher> recorded;std::vector<FaceState> query_face_states;for(const auto& f:queries){query_face_states.emplace_back(f.face);for(const auto& l:f.loops)for(const auto& u:l.uses)if(!recorded.Contains(u.edge)){recorded.Add(u.edge);query_states.emplace_back(u.edge);}}QueryBudget budget{options,output.stats,{}};for(const auto& record:output.chart_approximations){budget.cumulative[record.coedge_id]=record.uv_metric_mm;output.stats.max_query_uv_metric_mm=std::max(output.stats.max_query_uv_metric_mm,record.uv_metric_mm);}for(const auto& record:output.domain_clamps){SourceUVFace face;face.id=record.face_id;SourceUVUse use;use.id=record.coedge_id;budget.charge(face,use,record.uv_metric_mm);}for(auto& f:queries){const char* stage="source columns";try{auto providers=query_columns(f,options,output.stats,budget);stage="finite joints";finite_spline_joints(f,providers,options,output.stats,budget);stage="natural domain";query_domain(f,options,output.stats,budget);stage="analytic pole topology";query_poles(f,output,options,budget);stage="actual original use quotient";require_source_uv_uses(f,output.connectors);stage="final correspondence";check_correspondence(f,options,output.stats.max_query_residual_mm,true);}catch(const Standard_Failure& e){fail(context(f)+" query "+stage+": "+e.ExceptionType()+" "+e.what());}}for(const auto& state:query_states)state.check("query final physical source");for(const auto& state:query_face_states)state.check("query final physical source");output.stats.query_seconds=std::chrono::duration<double>(Clock::now()-query_begin).count();}
 return output;
}
// Measured edge-t to pcurve-s correspondence for a use whose edge is not
// SameParameter, by the method of Approx_SameParameter (OCCT
// Approx_SameParameter.cxx: BuildInitialDistribution, CheckSameParameter and
// the Newton step of ProjectPointOnCurve): sample the curve on surface S(P(s))
// over the pcurve range (16 per pcurve knot span), project each sample onto
// the 3D curve from the affine seed, add a node at every interior knot of the
// 3D curve, and publish the piecewise-linear map as kind 4. The residual
// |C(t) - S(P(s(t)))| is SAMPLED at every node and at 1/4, 1/2, 3/4 of every
// piece; it is not a bound between samples. A piece whose samples exceed
// source_residual_mm is split at its s midpoint. A node off the curve, a
// failed or nonmonotone projection, or the node cap refuses the use.
SourceParameterMap measure_source_parameter_map(const TopoDS_Edge& use_edge,const TopoDS_Face& face,double ta,double tb,const SourceUVOptions& options){
 const std::string where="parameter map: ";double first=0,last=0;auto curve=world_curve(use_edge,first,last);
 require(!curve.IsNull(),where+"edge has no 3D curve to correspond with");
 require(std::isfinite(first)&&std::isfinite(last)&&first<last&&((ta==first&&tb==last)||(ta==last&&tb==first)),where+"use range differs from the edge range");
 auto pc=pcurve(use_edge,face);require(!pc.curve.IsNull()&&std::isfinite(pc.first)&&std::isfinite(pc.last)&&pc.first<pc.last,where+"missing or empty pcurve");
 const double bound=options.source_residual_mm;require(std::isfinite(bound)&&bound>0,where+"invalid residual bound");
 auto surface=world_surface(face);SourceParameterMap result;result.kind=4;
 auto on_surface=[&](double s){auto uv=pc.curve->Value(s);require(finite(uv),where+"nonfinite pcurve point");return surface->Value(uv.X(),uv.Y());};
 auto residual=[&](double t,double s){++result.probes;double r=curve->Value(t).Distance(on_surface(s));require(std::isfinite(r),where+"nonfinite residual");return r;};
 auto over=[&](double r,const char* why){std::ostringstream m;m<<std::setprecision(17)<<where<<why<<": sampled residual "<<r<<" mm exceeds bound "<<bound<<" mm";fail(m.str());};
 auto unmatched=[&](double s,double seed){std::ostringstream m;m<<std::setprecision(17)<<where<<"projection of the curve-on-surface point at s="<<s<<" onto the 3D curve failed or is not monotone (affine-seed residual "<<residual(seed,s)<<" mm)";fail(m.str());};
 auto project=[&](const gp_Pnt& q,double t,double lo,double hi)->double{
  for(int i=0;i<30;++i){gp_Pnt p;gp_Vec d1,d2;curve->D2(t,p,d1,d2);gp_Vec v(p,q);double f=v.Dot(d1);if(std::abs(f)<=options.projection_precision_mm*d1.Magnitude())return t>lo&&t<hi?t:NAN;double df=v.Dot(d2)-d1.Dot(d1);if(!(std::abs(df)>1e-12))return NAN;t=std::min(std::max(t-f/df,lo),hi);}
  return NAN;};
 // Same Newton step, admitting the closed bracket [lo, hi] (used only to
 // classify a sample against a knot, never as a published node).
 auto project_closed=[&](const gp_Pnt& q,double t,double lo,double hi)->double{
  for(int i=0;i<30;++i){gp_Pnt p;gp_Vec d1,d2;curve->D2(t,p,d1,d2);gp_Vec v(p,q);double f=v.Dot(d1);if(std::abs(f)<=options.projection_precision_mm*d1.Magnitude())return t;double df=v.Dot(d2)-d1.Dot(d1);if(!(std::abs(df)>1e-12))return NAN;t=std::min(std::max(t-f/df,lo),hi);}
  return NAN;};
 auto knots_of=[](auto basis,double lo,double hi){std::vector<double> k;if(!basis.IsNull())for(int i=1;i<=basis->NbKnots();++i)if(lo<basis->Knot(i)&&basis->Knot(i)<hi)k.push_back(basis->Knot(i));return k;};
 Handle(Geom2d_Curve) basis2=pc.curve;while(!Handle(Geom2d_TrimmedCurve)::DownCast(basis2).IsNull())basis2=Handle(Geom2d_TrimmedCurve)::DownCast(basis2)->BasisCurve();
 Handle(Geom_Curve) basis3=curve;while(!Handle(Geom_TrimmedCurve)::DownCast(basis3).IsNull())basis3=Handle(Geom_TrimmedCurve)::DownCast(basis3)->BasisCurve();
 std::vector<double> cuts=knots_of(Handle(Geom2d_BSplineCurve)::DownCast(basis2),pc.first,pc.last);cuts.push_back(pc.first);cuts.push_back(pc.last);
 std::sort(cuts.begin(),cuts.end());struct Node{double s,t;};std::vector<Node> nodes{{pc.first,first}};
 for(std::size_t i=1;i<cuts.size();++i)for(int j=1;j<=16;++j){double s=j==16?cuts[i]:cuts[i-1]+(cuts[i]-cuts[i-1])*j/16;
  if(s==pc.last){nodes.push_back({s,last});continue;}double seed=first+(last-first)*(s-pc.first)/(pc.last-pc.first);double t=project(on_surface(s),seed,nodes.back().t,last);
  if(!(t>nodes.back().t))unmatched(s,seed);nodes.push_back({s,t});}
 // A 3D-curve knot is a node with that exact t. A node already within the
 // projection tolerance of the knot (scaled by curve speed) moves onto it;
 // otherwise bisect s on the projected t, where a projection landing on the
 // bracket end counts as being on that side of the knot.
 for(double knot:knots_of(Handle(Geom_BSplineCurve)::DownCast(basis3),first,last)){
  auto at=std::upper_bound(nodes.begin(),nodes.end(),knot,[](double k,const Node& n){return k<n.t;});if(at==nodes.begin()||at==nodes.end())continue;
  gp_Pnt p;gp_Vec d1;curve->D1(knot,p,d1);const double speed=d1.Magnitude();const double near=speed>0?options.projection_precision_mm/speed:0;
  auto below=at-1;if(below->t==knot)continue;
  if(below!=nodes.begin()&&knot-below->t<=near){below->t=knot;continue;}
  if(at+1!=nodes.end()&&at->t-knot<=near){at->t=knot;continue;}
  Node a=*below,b=*at;double lo=a.s,hi=b.s;
  for(int i=0;i<200&&lo<hi;++i){double s=lo+(hi-lo)/2;if(s<=lo||s>=hi)break;const double seed=a.t+(b.t-a.t)*(s-a.s)/(b.s-a.s);double t=project(on_surface(s),seed,a.t,b.t);
   if(std::isnan(t))t=project_closed(on_surface(s),seed,a.t,b.t);if(std::isnan(t))unmatched(s,seed);if(t<knot)lo=s;else hi=s;}
  nodes.insert(at,Node{lo+(hi-lo)/2,knot});}
 for(std::size_t i=1;i<nodes.size();++i)if(!(nodes[i].t>nodes[i-1].t))fail(where+"nodes are not strictly increasing in t after placing 3D-curve knots");
 std::vector<double> node_residual;for(const auto& n:nodes){double r=residual(n.t,n.s);if(r>bound)over(r,"curve-on-surface point is off the 3D curve");node_residual.push_back(r);}
 const std::size_t cap=std::size_t(1)<<16;struct Out{Node node;double node_residual,piece_residual;};std::vector<Out> out{{nodes.front(),node_residual.front(),0}};
 struct Piece{Node a,b;double rb;};std::vector<Piece> pending;for(std::size_t i=nodes.size()-1;i>0;--i)pending.push_back({nodes[i-1],nodes[i],node_residual[i]});
 while(!pending.empty()){auto piece=pending.back();pending.pop_back();const auto& a=piece.a;const auto& b=piece.b;double worst=0;
  for(double f:{.25,.5,.75})worst=std::max(worst,residual(a.t+(b.t-a.t)*f,a.s+(b.s-a.s)*f));
  if(worst<=bound){out.push_back({b,piece.rb,worst});continue;}
  if(out.size()+pending.size()+2>cap)over(worst,"piecewise-linear map needs more than the node cap");
  Node m{(a.s+b.s)/2,0};const double seed=(a.t+b.t)/2;m.t=project(on_surface(m.s),seed,a.t,b.t);if(!(m.t>a.t&&m.t<b.t))unmatched(m.s,seed);
  double r=residual(m.t,m.s);if(r>bound)over(r,"curve-on-surface point is off the 3D curve");
  pending.push_back({m,b,piece.rb});pending.push_back({a,m,r});}
 // Row residual: the sampled maximum at the row's node and inside the piece
 // that ends at it in published order; the first published row has its node.
 const bool reversed=ta!=first;const std::size_t n=out.size();
 for(std::size_t k=0;k<n;++k){std::size_t i=reversed?n-1-k:k;double r=out[i].node_residual;if(k>0)r=std::max(r,reversed?out[i+1].piece_residual:out[i].piece_residual);
  result.rows.push_back({out[i].node.t,out[i].node.s,r});result.max_residual_mm=std::max(result.max_residual_mm,r);}
 return result;
}
}
