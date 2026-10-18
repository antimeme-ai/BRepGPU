#ifndef BREPGPU_FACE_REGIONS_HPP
#define BREPGPU_FACE_REGIONS_HPP
// A face's region and side from its own loops, and its share of its shell's
// signed volume, by owned quadrature (B2 .37.36 and review rounds 2 to 4):
// pole faces (Green's theorem from a pole's iso-line), natural faces, faces
// on closed surfaces whose same_sense chooses between two complementary
// regions, and the face extent the placement agreement uses. The STEP source
// adapter calls these; tests/face_regions_fixture.cpp drives them directly.
#include <BndLib_AddSurface.hxx>
#include <Bnd_Box.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_Line.hxx>
#include <gp_Dir2d.hxx>
#include <Geom_Surface.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>
#include <gp_Vec2d.hxx>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace brepgpu::face_regions {

// B2 .37.36 review 2: owned divergence-theorem quadrature for faces whose
// side OCCT's machinery cannot evaluate (a vertex loop's pole wire has no
// pcurve; OCCT's volume of the apex cone is 174.53 against 261.80). The flux
// density g = (P - c) . (S_u x S_v) about the shell's box centre c; a face's share of its shell's
// signed volume is (1/3) integral of g over its UV region.
inline const double gauss_x[4]={-0.8611363115940526,-0.3399810435848563,0.3399810435848563,0.8611363115940526};
inline const double gauss_w[4]={0.3478548451374538,0.6521451548625461,0.6521451548625461,0.3478548451374538};
// About the shell's box centre c (review round 2, finding 2): far from the
// origin the shares' terms would otherwise cancel. g = (P - c) . (S_u x S_v).
inline gp_Pnt flux_centre;
inline double flux_density(const Handle(Geom_Surface)& s,double u,double v){gp_Pnt p;gp_Vec su,sv;s->D1(u,v,p,su,sv);return gp_Vec(flux_centre,p).Dot(su.Crossed(sv));}
// Integral of g along the free coordinate from ref to x, the other one fixed
// (axis 1: v free, u fixed; axis 0: u free, v fixed).
inline double flux_line(const Handle(Geom_Surface)& s,int axis,double fixed,double ref,double x){double sum=0;const int pieces=8;
  for(int k=0;k<pieces;++k){const double a=ref+(x-ref)*k/pieces,b=ref+(x-ref)*(k+1)/pieces;for(int i=0;i<4;++i){const double q=.5*(a+b)+.5*(b-a)*gauss_x[i];sum+=gauss_w[i]*.5*(b-a)*(axis==1?flux_density(s,fixed,q):flux_density(s,q,fixed));}}return sum;}
// Green's theorem over a face whose region lies between its edge loops and a
// pole on the iso-line at ref (the pole line contributes nothing there):
// area = the signed UV area to the left of the loops (positive when they run
// counter-clockwise in UV about the region), flux = the integral of g over that
// region, signed the same way. du-form when the pole fixes v (axis 1), dv-form
// when it fixes u (axis 0).
struct GreenSums{double area=0,flux=0;std::map<std::uint64_t,double> winding;}; // winding: each edge loop's change of the pole's free coordinate
inline void green_add(GreenSums& g,std::uint64_t loop,const Handle(Geom2d_Curve)& c,double a,double b,bool increasing,const Handle(Geom_Surface)& s,int axis,double ref){
  const int pieces=32;const double direction=increasing?1.:-1.;
  {const auto p=c->Value(a),q=c->Value(b);g.winding[loop]+=direction*(axis==1?q.X()-p.X():q.Y()-p.Y());}
  for(int k=0;k<pieces;++k){const double x0=a+(b-a)*k/pieces,x1=a+(b-a)*(k+1)/pieces;
    for(int i=0;i<4;++i){const double q=.5*(x0+x1)+.5*(x1-x0)*gauss_x[i],w=direction*gauss_w[i]*.5*(x1-x0);gp_Pnt2d uv;gp_Vec2d d;c->D1(q,uv,d);
      if(axis==1){g.area-=(uv.Y()-ref)*d.X()*w;g.flux-=flux_line(s,1,uv.X(),ref,uv.Y())*d.X()*w;}
      else{g.area+=(uv.X()-ref)*d.Y()*w;g.flux+=flux_line(s,0,uv.Y(),ref,uv.X())*d.Y()*w;}}}
}
// A pole candidate: the iso-line axis (1: v fixed, u free; 0: u fixed, v free)
// and its coordinate. A UV use: one coedge's UV curve, run in the loop's sense.
struct PoleCandidate{int axis=1;double ref=0;};
struct UvUse{std::uint64_t loop=0;Handle(Geom2d_Curve) curve;double first=0,last=0;bool increasing=true;};
// How a face's edge loops wind about a pole (review round 3, finding 5): each
// loop's change of the pole's free coordinate is one period of the surface
// (periodic or closed in that direction) or zero, within the recovery grade.
// The region between the loops and the pole is the strip from the one winding
// loop to the pole less the holes, which do not wind; several winding loops,
// or a loop that changes by anything else, leave the region not determined.
enum class Winding{none,one,several,irregular};
inline Winding pole_winding(const std::vector<UvUse>& uses,const Handle(Geom_Surface)& s,int axis){
  std::map<std::uint64_t,double> change;for(const auto& u:uses){const auto p=u.curve->Value(u.first),q=u.curve->Value(u.last);change[u.loop]+=(u.increasing?1.:-1.)*(axis==1?q.X()-p.X():q.Y()-p.Y());}
  double a0,a1,b0,b1;s->Bounds(a0,a1,b0,b1);const bool along_u=axis==1;const bool periodic=along_u?s->IsUPeriodic():s->IsVPeriodic(),closed=along_u?s->IsUClosed():s->IsVClosed();
  const double period=periodic?(along_u?s->UPeriod():s->VPeriod()):(along_u?a1-a0:b1-b0);const bool can_wind=(periodic||closed)&&std::isfinite(period)&&period>0;
  const double grade=1e-9*std::max(1.,can_wind?period:1.);int winding=0;
  for(const auto& [loop,c]:change){if(can_wind&&std::abs(std::abs(c)-period)<=grade)++winding;else if(!(std::abs(c)<=grade))return Winding::irregular;}
  return winding==0?Winding::none:winding==1?Winding::one:Winding::several;
}
inline GreenSums green_of(const std::vector<UvUse>& uses,const Handle(Geom_Surface)& s,const PoleCandidate& pole){GreenSums g;for(const auto& u:uses)green_add(g,u.loop,u.curve,u.first,u.last,u.increasing,s,pole.axis,pole.ref);return g;}
// A box that holds a surface over a UV rectangle (the extent of
// native/step_placements.hpp agree). OCCT bounds planes, cylinders, cones,
// spheres and tori exactly and Bezier and B-spline surfaces by their poles;
// it boxes any other type (offset, revolution, extrusion) from a sample
// grid, which bounds nothing, so such a face's extent is unbounded and the
// raw placement is written (review round 3, finding 4).
inline Bnd_Box face_extent(const Handle(Geom_Surface)& s,double u0,double u1,double v0,double v1){GeomAdaptor_Surface adaptor(s,u0,u1,v0,v1);Bnd_Box b;
  switch(adaptor.GetType()){case GeomAbs_Plane:case GeomAbs_Cylinder:case GeomAbs_Cone:case GeomAbs_Sphere:case GeomAbs_Torus:case GeomAbs_BezierSurface:case GeomAbs_BSplineSurface:BndLib_AddSurface::Add(adaptor,0.,b);break;default:b.SetWhole();}return b;}
// The integral of g over a surface's whole natural domain (a face bounded by
// vertex loops alone has natural bounds), composite Gauss 16 x 16 pieces.
inline double natural_flux(const Handle(Geom_Surface)& s){double u0,u1,v0,v1;s->Bounds(u0,u1,v0,v1);if(!(std::isfinite(u0)&&std::isfinite(u1)&&std::isfinite(v0)&&std::isfinite(v1)))return NAN;double sum=0;const int pieces=16;
  for(int i=0;i<pieces;++i)for(int j=0;j<pieces;++j){const double a0=u0+(u1-u0)*i/pieces,a1=u0+(u1-u0)*(i+1)/pieces,b0=v0+(v1-v0)*j/pieces,b1=v0+(v1-v0)*(j+1)/pieces;
    for(int p=0;p<4;++p)for(int q=0;q<4;++q)sum+=gauss_w[p]*gauss_w[q]*.25*(a1-a0)*(b1-b0)*flux_density(s,.5*(a0+a1)+.5*(a1-a0)*gauss_x[p],.5*(b0+b1)+.5*(b1-b0)*gauss_x[q]);}return sum;}

// Review round 4, item 2: a closed surface (compact: each parameter direction
// periodic or closed, or ending in a singular iso-line at both ends, as a
// sphere's v at its poles) is separated by a face's loops into two bounded,
// complementary regions. The face is the one to the left of its loops about
// its normal, so its same_sense chooses the region as well as the side, and
// it never contradicts its bound; OCCT's classifier assumes the region inside
// the loops in UV and would call a correct complement face contradicting.
inline bool closed_surface(const Handle(Geom_Surface)& s,const std::vector<PoleCandidate>& singular){
  double u0,u1,v0,v1;s->Bounds(u0,u1,v0,v1);if(!(std::isfinite(u0)&&std::isfinite(u1)&&std::isfinite(v0)&&std::isfinite(v1)))return false;
  auto pole_at=[&](int axis,double x){return std::any_of(singular.begin(),singular.end(),[&](const PoleCandidate& c){return c.axis==axis&&c.ref==x;});};
  const bool u_sealed=s->IsUPeriodic()||s->IsUClosed()||(pole_at(0,u0)&&pole_at(0,u1)),v_sealed=s->IsVPeriodic()||s->IsVClosed()||(pole_at(1,v0)&&pole_at(1,v1));
  return u_sealed&&v_sealed;
}
// The region a face on a closed surface reads. Each loop winds once or not at
// all, in one direction only, and runs continuously in one UV chart once its
// uses are moved by whole periods (each use starting where the one before it
// ends, within the recovery grade, or along a singular iso-line where the loop
// passes a pole; a winding loop closes with one period jump). Either no loop winds, or two
// loops wind the same direction once each, opposite ways (a band, as two
// parallel circles bound on a torus or a sphere), with any holes. Green's sums
// from an iso-line across the winding direction then give the signed integral
// over the region between the loops in UV, whatever the iso-line, since the
// windings cancel. Its sign against same_sense chooses that region or the
// complement (the natural domain less it). The complement of a band on a
// surface sealed by poles across it (a sphere's two caps) is not one region:
// there the band is the only reading (unique) and same_sense may contradict it.
// Otherwise the region is not determined soundly and the face is refused as
// region-ambiguous. flux is the integral of g over the chosen region with the
// surface's own normal; area its UV area; signed_area and signed_flux the sums
// with the loops' sense (for the unique reading).
// Also (review round 5): inside_flux and complement_flux, both regions'
// fluxes with the surface's normal; band when either region spans a period
// (two winding loops, or a loop that runs a whole period through a seam),
// where which region lies inside the loops is a matter of the chart; the
// loops as closed polylines in the unwrapped chart, the iso-line axis of the
// sums, and the periods, for region_holds.
struct TwoRegions{bool decided=false,unique=false,band=false;std::string why;double area=0,flux=0,signed_area=0,signed_flux=0,inside_flux=0,complement_flux=0;bool inside=true;
  std::vector<std::vector<gp_Pnt2d>> polylines;int axis=1;double pu=0,pv=0;};
inline TwoRegions two_regions(const std::vector<UvUse>& uses,const Handle(Geom_Surface)& s,int same_sign,const std::vector<PoleCandidate>& singular={}){
  TwoRegions r;double u0,u1,v0,v1;s->Bounds(u0,u1,v0,v1);const double gu=1e-9*std::max(1.,u1-u0),gv=1e-9*std::max(1.,v1-v0);
  const double pu=s->IsUPeriodic()?s->UPeriod():s->IsUClosed()?u1-u0:0.,pv=s->IsVPeriodic()?s->VPeriod():s->IsVClosed()?v1-v0:0.;
  std::vector<std::uint64_t> order;std::map<std::uint64_t,std::vector<const UvUse*>> loops;for(const auto& u:uses){if(!loops.count(u.loop))order.push_back(u.loop);loops[u.loop].push_back(&u);}
  if(order.empty()){r.why="it has no edge loop";return r;}
  auto start=[](const UvUse& u){return u.curve->Value(u.increasing?u.first:u.last);};auto end=[](const UvUse& u){return u.curve->Value(u.increasing?u.last:u.first);};
  // A whole number of periods within the grade, or none.
  auto periods=[](double gap,double period,double grade,int& k){if(std::abs(gap)<=grade){k=0;return true;}if(!(period>0))return false;k=int(std::lround(gap/period));return k!=0&&std::abs(gap-k*period)<=grade;};
  auto on_pole=[&](const gp_Pnt2d& p,const gp_Pnt2d& q){return std::any_of(singular.begin(),singular.end(),[&](const PoleCandidate& c){return c.axis==1?(std::abs(p.Y()-c.ref)<=gv&&std::abs(q.Y()-c.ref)<=gv):(std::abs(p.X()-c.ref)<=gu&&std::abs(q.X()-c.ref)<=gu);});};
  // Each loop as a closed path in one chart: its uses, each moved by whole
  // periods so that it starts where the one before it ends, and a straight UV
  // segment along a singular iso-line where the loop passes a pole (the pole
  // is one point; the segment adds nothing to its area or flux about an
  // iso-line through it, and closes the path for any other). A winding loop
  // closes with one jump of a period.
  std::vector<UvUse> path;int net_u=0,net_v=0,winding_u=0,winding_v=0;
  auto segment=[&](std::uint64_t loop,const gp_Pnt2d& p,const gp_Pnt2d& q){Handle(Geom2d_Curve) line=new Geom2d_Line(p,gp_Dir2d(gp_Vec2d(p,q)));path.push_back({loop,line,0.,p.Distance(q),true});};
  for(auto loop:order){const auto& l=loops.at(loop);gp_Vec2d offset(0,0);const auto first=start(*l.front());
    for(std::size_t k=0;k<l.size();++k){const UvUse& u=*l[k];UvUse moved=u;if(offset.Magnitude()>0)moved.curve=Handle(Geom2d_Curve)::DownCast(u.curve->Translated(offset));path.push_back(moved);
      const auto e=end(moved);const bool closing=k+1==l.size();const auto n=closing?first:start(*l[k+1]).Translated(offset);const double jx=n.X()-e.X(),jy=n.Y()-e.Y();
      if(std::abs(jx)<=gu&&std::abs(jy)<=gv)continue;
      if(on_pole(e,n)){segment(loop,e,n);continue;}
      int ku=0,kv=0;if(!periods(jx,pu,gu,ku)||!periods(jy,pv,gv,kv)){r.why="its loop's UV curves do not join in one chart, so the region inside them is not determined";return r;}
      if(!closing){offset-=gp_Vec2d(ku*pu,kv*pv);continue;}
      // The closing jump is the loop's winding, once in one direction.
      const int wu=-ku,wv=-kv;if(std::abs(wu)>1||std::abs(wv)>1||(wu&&wv)){r.why="a loop of it winds around the closed surface other than once in one direction, so the region it bounds is not determined";return r;}
      net_u+=wu;net_v+=wv;winding_u+=wu!=0;winding_v+=wv!=0;}}
  if(net_u||net_v||(winding_u&&winding_u!=2)||(winding_v&&winding_v!=2)||(winding_u&&winding_v)){r.why="its loops' windings around the closed surface do not cancel in one band, so they do not separate a region";return r;}
  // Across the winding direction: v fixed lines (du-form) for a band winding
  // in u, u fixed lines (dv-form) for one winding in v.
  const int axis=winding_v?0:1;GreenSums g;for(const auto& u:path)green_add(g,u.loop,u.curve,u.first,u.last,u.increasing,s,axis,axis==1?v0:u0);
  r.axis=axis;r.pu=pu;r.pv=pv;r.band=winding_u||winding_v;
  {std::map<std::uint64_t,std::size_t> slot;for(const auto& u:path){if(!slot.count(u.loop)){slot[u.loop]=r.polylines.size();r.polylines.emplace_back();}auto& line=r.polylines[slot.at(u.loop)];
     for(int k=0;k<=32;++k){const double t=u.increasing?u.first+(u.last-u.first)*k/32.:u.last-(u.last-u.first)*k/32.;line.push_back(u.curve->Value(t));}}
   for(auto& line:r.polylines){if(line.empty())continue;double a=line[0].X(),b=a,c=line[0].Y(),d=c;for(const auto& p:line){a=std::min(a,p.X());b=std::max(b,p.X());c=std::min(c,p.Y());d=std::max(d,p.Y());}
     if((pu>0&&b-a>=pu-gu)||(pv>0&&d-c>=pv-gv))r.band=true;}}
  if(!(std::abs(g.area)>0)){r.why="its loops enclose no area";return r;}
  r.signed_area=g.area;r.signed_flux=g.flux;const double domain=(u1-u0)*(v1-v0);
  // The only region: a band whose complement is cut by poles across it, or
  // loops enclosing the whole domain (the complement is empty).
  if((winding_u&&pv==0)||(winding_v&&pu==0)||!(domain-std::abs(g.area)>1e-9*domain)){r.unique=true;r.decided=std::isfinite(g.flux);if(!r.decided)r.why="its region's flux is not finite";return r;}
  const int sense=g.area>0?1:-1;const double inside_flux=sense*g.flux;r.inside=sense==same_sign;
  const double whole=natural_flux(s);if(!std::isfinite(whole)){r.why="its surface has no finite natural domain";return r;}r.inside_flux=inside_flux;r.complement_flux=whole-inside_flux;
  if(r.inside){r.flux=inside_flux;r.area=std::abs(g.area);}
  else{r.flux=whole-inside_flux;r.area=domain-std::abs(g.area);}
  r.decided=std::isfinite(r.flux);if(!r.decided)r.why="its region's flux is not finite";return r;
}

// Whether a UV point lies in the region inside the loops (inside) or in its
// complement: the signed count of the loops' crossings on the ray from the
// point across the winding direction (+v for the du-form sums, +u for the
// dv-form), over periodic copies of the loops and the point, is nonzero
// exactly inside, as in the sums themselves.
inline bool region_holds(const TwoRegions& r,double u,double v,bool inside){
  bool in=false;const int ku=r.pu>0?2:0,kv=r.pv>0?2:0;
  for(int m=-kv;m<=kv&&!in;++m)for(int n=-ku;n<=ku&&!in;++n){const double x=u+n*r.pu,y=v+m*r.pv;int count=0;
    for(const auto& line:r.polylines)for(std::size_t k=0;k<line.size();++k){const auto& a=line[k];const auto& b=line[(k+1)%line.size()];
      // The polyline of a winding loop does not close on itself.
      if(k+1==line.size()&&a.Distance(b)>.5*std::max(r.pu,r.pv))continue;
      const double a1=r.axis==1?a.X():a.Y(),b1=r.axis==1?b.X():b.Y(),a2=r.axis==1?a.Y():a.X(),b2=r.axis==1?b.Y():b.X(),p1=r.axis==1?x:y,p2=r.axis==1?y:x;
      if(!((a1<=p1&&p1<b1)||(b1<=p1&&p1<a1)))continue;const double at=a2+(b2-a2)*(p1-a1)/(b1-a1);if(at>p2)count+=b1>a1?-1:1;}
    in=count!=0;}
  return inside?in:!in;
}

// Review round 5: the region between a face's winding loop (and its holes) and
// a pole, as region_holds reads it: each loop sampled in one chart (its uses
// moved by whole periods to join), the winding loop closed along the pole's
// iso-line, so the closed polygon holds the strip between loop and pole.
inline TwoRegions pole_region(const std::vector<UvUse>& uses,const Handle(Geom_Surface)& s,const PoleCandidate& pole){
  TwoRegions r;r.decided=true;r.inside=true;r.axis=pole.axis;double u0,u1,v0,v1;s->Bounds(u0,u1,v0,v1);
  const double pu=s->IsUPeriodic()?s->UPeriod():s->IsUClosed()?u1-u0:0.,pv=s->IsVPeriodic()?s->VPeriod():s->IsVClosed()?v1-v0:0.;
  if(pole.axis==1)r.pu=pu;else r.pv=pv;const double period=pole.axis==1?pu:pv;
  std::vector<std::uint64_t> order;std::map<std::uint64_t,std::vector<const UvUse*>> loops;for(const auto& u:uses){if(!loops.count(u.loop))order.push_back(u.loop);loops[u.loop].push_back(&u);}
  for(auto loop:order){std::vector<gp_Pnt2d> line;gp_Vec2d offset(0,0);
    for(const auto* u:loops.at(loop)){const auto start=u->curve->Value(u->increasing?u->first:u->last).Translated(offset);
      if(!line.empty()&&period>0){const double gap=(pole.axis==1?line.back().X()-start.X():line.back().Y()-start.Y());const double k=std::round(gap/period);if(k!=0)offset+=pole.axis==1?gp_Vec2d(k*period,0):gp_Vec2d(0,k*period);}
      for(int k=0;k<=32;++k){const double t=u->increasing?u->first+(u->last-u->first)*k/32.:u->last-(u->last-u->first)*k/32.;line.push_back(u->curve->Value(t).Translated(offset));}}
    if(line.empty())continue;
    const double change=pole.axis==1?line.back().X()-line.front().X():line.back().Y()-line.front().Y();
    if(period>0&&std::abs(change)>.5*period){if(pole.axis==1){line.push_back(gp_Pnt2d(line.back().X(),pole.ref));line.push_back(gp_Pnt2d(line.front().X(),pole.ref));}
      else{line.push_back(gp_Pnt2d(pole.ref,line.back().Y()));line.push_back(gp_Pnt2d(pole.ref,line.front().Y()));}}
    r.polylines.push_back(line);}
  return r;
}

} // namespace brepgpu::face_regions

#endif
