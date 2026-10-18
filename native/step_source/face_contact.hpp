#ifndef BREPGPU_FACE_CONTACT_HPP
#define BREPGPU_FACE_CONTACT_HPP
// Whether a face's reading on a closed surface meets another face of its part
// (B2 review rounds 5 to 7): the self-intersection test of the two-region rule
// (native/step_source/face_regions.hpp). The STEP source adapter calls it;
// tests/face_regions_fixture.cpp drives it directly.
#include "face_regions.hpp"
#include "shape_extent.hpp"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomLProp_SLProps.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <functional>
#include <limits>

namespace brepgpu::face_contact {

using face_regions::TwoRegions;

// A neighbour: its face (its edges and vertices give its common boundaries
// with the tested face, and shape_extent its box), and the face its distance
// and sections are measured on: the face itself, or for a pole face (a
// vertex-loop apex or pole, which OCCT leaves without a pcurve) its carrier
// over its UV hint, which reaches the pole (pole_proxy): BRepExtrema, like
// BRepBndLib, takes a face's domain from its pcurves' UV box.
struct Neighbour {
  TopoDS_Face face, measure;
};
inline TopoDS_Face pole_proxy(const Handle(Geom_Surface)& s, double u0, double u1, double v0, double v1) {
  BRepBuilderAPI_MakeFace made(s, u0, u1, v0, v1, Precision::Confusion());
  return made.IsDone() ? made.Face() : TopoDS_Face();
}

struct Contact {
  // 1 meets; 0 meets none (the whole region tested); 2 meets only within twice
  // a strip's half-width of a common boundary (uncertain: never grounds a
  // correction); 3 meets none, but a cell beside a strip was dropped before
  // reaching the blind-band target (undecided); -1 the test did not complete.
  int result = 0;
  int crossings_only = 0;   // neighbours held to crossings only (no bounded strip)
  // The band left blind beside common boundaries: the widest strip, and the
  // largest cell dropped at the last refinement level for touching a strip.
  double strip_mm = 0, cell_mm = 0;
};

namespace detail {
inline Handle(Geom_Surface) world_surface(const TopoDS_Face& f) {
  TopLoc_Location l;
  Handle(Geom_Surface) s = BRep_Tool::Surface(f, l);
  if (!s.IsNull() && !l.IsIdentity())
    s = Handle(Geom_Surface)::DownCast(s->Transformed(l.Transformation()));
  return s;
}
// The unit normal and the normal curvature in a tangent direction at a point
// of a surface (projected within the resolution); false where undefined.
inline bool curvature_at(const Handle(Geom_Surface)& s, const gp_Pnt& p, double resolution, gp_Dir& normal,
                         const std::function<gp_Dir(const gp_Dir&)>& direction, double& kappa) {
  ShapeAnalysis_Surface analysis(s);
  const gp_Pnt2d uv = analysis.ValueOfUV(p, resolution);
  GeomLProp_SLProps props(s, uv.X(), uv.Y(), 2, 1e-12);
  if (!props.IsNormalDefined() || !props.IsCurvatureDefined())
    return false;
  normal = props.Normal();
  const gp_Dir d = direction(normal);
  if (props.IsUmbilic()) {
    kappa = props.MeanCurvature();
    return true;
  }
  gp_Dir dmax, dmin;
  props.CurvatureDirections(dmax, dmin);
  const double c = d.Dot(dmax), t = d.Dot(dmin);
  kappa = props.MaxCurvature() * c * c + props.MinCurvature() * t * t;
  return true;
}
// The width over which a neighbour stays within the resolution of a surface
// that continues across their common point p with tangent t (an edge) or in
// every tangent direction (a vertex): linear separation at the angle between
// their normals, quadratic at the difference of their normal curvatures
// across t (signed by the normals' relative sense). Infinite where neither
// separates them (one surface, continued).
inline double contact_width(const Handle(Geom_Surface)& s, const Handle(Geom_Surface)& g, const gp_Pnt& p,
                            const gp_Dir* tangent, double resolution) {
  double worst = 0;
  const int directions = tangent ? 1 : 8;
  for (int k = 0; k < directions; ++k) {
    gp_Dir n1, n2;
    double k1 = 0, k2 = 0;
    auto across = [&](const gp_Dir& n) -> gp_Dir {
      if (tangent)
        return n.Crossed(*tangent);
      // Eight tangent directions of the first surface's tangent plane.
      gp_Dir a = std::abs(n.X()) < .9 ? gp_Dir(1, 0, 0) : gp_Dir(0, 1, 0);
      const gp_Dir u = n.Crossed(a), v = n.Crossed(u);
      const double angle = M_PI * k / directions;
      return gp_Dir(gp_Vec(u) * std::cos(angle) + gp_Vec(v) * std::sin(angle));
    };
    if (!curvature_at(s, p, resolution, n1, across, k1))
      return INFINITY;
    // The neighbour's cross direction is taken in its own tangent plane.
    gp_Dir d1 = across(n1);
    auto along = [&](const gp_Dir& n) -> gp_Dir {
      const gp_Vec v = gp_Vec(d1) - gp_Vec(n) * gp_Vec(d1).Dot(gp_Vec(n));
      return v.Magnitude() > 1e-12 ? gp_Dir(v) : d1;
    };
    if (!curvature_at(g, p, resolution, n2, along, k2))
      return INFINITY;
    if (n1.Dot(n2) < 0)
      k2 = -k2;
    const double sine = gp_Vec(n1).Crossed(gp_Vec(n2)).Magnitude(), delta = std::abs(k1 - k2);
    const double linear = sine > 1e-12 ? resolution / sine : INFINITY;
    const double quadratic = delta > 1e-12 ? std::sqrt(2 * resolution / delta) : INFINITY;
    worst = std::max(worst, std::min(linear, quadratic));
  }
  return worst;
}
// A bound on the distance from a UV cell's centre to any of its points: a
// path from the centre runs half the cell along u then half along v, so the
// distance is at most (du / 2) max |S_u| + (dv / 2) max |S_v| over the cell,
// the maxima exact for analytic carriers (plane 1, 1; cylinder R, 1; cone R(v)
// at the cell's larger end, 1; sphere R, R; torus R + r, r); for other
// carriers the cell box's diagonal.
inline double cell_radius(const Handle(Geom_Surface)& s, double u0, double u1, double v0, double v1, double diagonal) {
  GeomAdaptor_Surface a(s);
  const double du = .5 * std::abs(u1 - u0), dv = .5 * std::abs(v1 - v0);
  double su = INFINITY, sv = INFINITY;
  switch (a.GetType()) {
  case GeomAbs_Plane:
    su = sv = 1.;
    break;
  case GeomAbs_Cylinder:
    su = a.Cylinder().Radius();
    sv = 1.;
    break;
  case GeomAbs_Cone: {
    const auto c = a.Cone();
    su = std::max(std::abs(c.RefRadius() + v0 * std::sin(c.SemiAngle())), std::abs(c.RefRadius() + v1 * std::sin(c.SemiAngle())));
    sv = 1.;
    break;
  }
  case GeomAbs_Sphere:
    su = sv = a.Sphere().Radius();
    break;
  case GeomAbs_Torus:
    su = a.Torus().MajorRadius() + a.Torus().MinorRadius();
    sv = a.Torus().MinorRadius();
    break;
  default:
    return diagonal;
  }
  return std::min(diagonal, du * su + dv * sv);
}
} // namespace detail

// Review round 8, ruling 1: the tangent-edge fold test, run before any
// contact test. At an edge the face shares with a neighbour where the two are
// tangent (|n1 . n2| >= cos(angle) at every one of 17 samples along it), a
// reading whose outward normal there opposes the neighbour's (n1 . n2 < 0)
// folds back over it (a zero dihedral angle): no solid's boundary. The edge
// uses fix the loop's direction, so the other reading has the opposite normal
// there: of the two readings at most one is fold-free at a tangent edge.
// raw_side and the neighbours' sides are the written senses (+1: the surface
// normal outward).
struct Fold {
  int tangent_edges = 0, raw_smooth = 0, raw_folds = 0, mixed = 0;
  double min_abs_dot = 1;  // over the tangent edges' samples
};
inline Fold tangent_fold(const TopoDS_Face& face, const Handle(Geom_Surface)& s, int raw_side,
                         const std::vector<std::pair<TopoDS_Face, int>>& neighbours, double resolution, double angle) {
  Fold out;
  TopTools_IndexedMapOfShape own;
  TopExp::MapShapes(face, TopAbs_EDGE, own);
  const double threshold = std::cos(angle);
  ShapeAnalysis_Surface analysis(s);
  for (const auto& [other, side] : neighbours) {
    const Handle(Geom_Surface) g = detail::world_surface(other);
    if (g.IsNull())
      continue;
    ShapeAnalysis_Surface other_analysis(g);
    for (TopExp_Explorer e(other, TopAbs_EDGE); e.More(); e.Next()) {
      if (!own.Contains(e.Current()) || BRep_Tool::Degenerated(TopoDS::Edge(e.Current())))
        continue;
      BRepAdaptor_Curve curve(TopoDS::Edge(e.Current()));
      bool tangent = true;
      int positive = 0, negative = 0;
      double least = 1;
      for (int k = 0; k <= 16 && tangent; ++k) {
        const gp_Pnt p = curve.Value(curve.FirstParameter() + (curve.LastParameter() - curve.FirstParameter()) * k / 16.);
        const gp_Pnt2d a = analysis.ValueOfUV(p, resolution), b = other_analysis.ValueOfUV(p, resolution);
        GeomLProp_SLProps pa(s, a.X(), a.Y(), 1, 1e-12), pb(g, b.X(), b.Y(), 1, 1e-12);
        if (!pa.IsNormalDefined() || !pb.IsNormalDefined()) {
          tangent = false;
          break;
        }
        const double dot = raw_side * side * gp_Vec(pa.Normal()).Dot(gp_Vec(pb.Normal()));
        if (std::abs(dot) < threshold)
          tangent = false;
        (dot > 0 ? positive : negative)++;
        least = std::min(least, std::abs(dot));
      }
      if (!tangent)
        continue;
      ++out.tangent_edges;
      out.min_abs_dot = std::min(out.min_abs_dot, least);
      if (positive && negative)
        ++out.mixed;
      else if (positive)
        ++out.raw_smooth;
      else
        ++out.raw_folds;
    }
  }
  return out;
}

// Whether a face's reading (its region inside its loops, or the complement,
// or a pole's side) on a closed surface meets any of the other faces of its
// part away from the face's own boundary. Meeting is contact within the
// source resolution: a gap below it decides as touching does. A point meets
// the reading when it lies in the region (region_holds) farther than the edge
// exclusion from the face's edges: ten resolutions plus the edges'
// tolerance, the tolerance counted only up to a hundred resolutions (an edge
// tolerance beyond that is a sloppy file's, and contact it would excuse is not
// the boundary's).
// A neighbour that shares an edge or a vertex with the face meets its reading
// along their common boundary, and where they are tangent there (a fillet's
// neighbours, the reading continuing across the edge on the neighbour's
// side) the two stay within the resolution over a strip: half-width the edge
// exclusion plus twice the larger, over 17 points along the edge, of the
// smaller of resolution / sin(angle between normals) and sqrt(2 resolution /
// difference of normal curvatures across the edge) (the curvatures exact
// for analytic carriers); at a common vertex alone, a ball of that width over
// eight tangent directions. Outside the strips the neighbour is tested like
// any other face. A neighbour whose strip has no bound (one surface,
// continued across) or covers more than a quarter of the surface's size is
// held to crossings only, and counted.
// For each other face:
// - the whole surface's distance to it (BRepExtrema); beyond the resolution
//   it cannot meet the region;
// - crossings: the surface sectioned with it (BOPAlgo, fuzzy by the
//   resolution), each section edge sampled at a step of 1/1024 of the
//   surface's own size, so a short in-region crossing is not missed, each
//   sample's surface point held to the resolution by its own distance;
// - within the resolution, any closest point that meets; else the region, as
//   16 x 16 UV cells over its chart box, each cell's distance to it, cells
//   within the resolution refined three levels, so a contact elsewhere is not
//   hidden behind the boundary's; a cell touching a strip is split without a
//   distance call to the third level and dropped there, so the band left
//   blind beside a common boundary is its strip plus at most one third-level
//   cell (strip_mm, cell_mm: the region record states both).
inline Contact region_meets(const Handle(Geom_Surface)& s, const TwoRegions& r, bool inside, const TopoDS_Face& face,
                            const std::vector<Neighbour>& others, double resolution, double blind_target = INFINITY) {
  Contact out;
  try {
    double u0, u1, v0, v1;
    s->Bounds(u0, u1, v0, v1);
    BRepBuilderAPI_MakeFace whole(s, u0, u1, v0, v1, Precision::Confusion());
    if (!whole.IsDone()) {
      out.result = -1;
      return out;
    }
    TopoDS_Compound boundary;
    BRep_Builder builder;
    builder.MakeCompound(boundary);
    double edge_tolerance = 0;
    for (TopExp_Explorer e(face, TopAbs_EDGE); e.More(); e.Next()) {
      builder.Add(boundary, e.Current());
      edge_tolerance = std::max(edge_tolerance, BRep_Tool::Tolerance(TopoDS::Edge(e.Current())));
    }
    const double exclusion = 10 * resolution + std::min(edge_tolerance, 100 * resolution);
    ShapeAnalysis_Surface analysis(s);
    Bnd_Box surface_box;
    BRepBndLib::Add(whole.Face(), surface_box);
    const double size = std::sqrt(surface_box.SquareExtent());
    bool failed = false, uncertain = false, dropped_over_target = false;
    // Strips of the current neighbour: (edge or vertex, half-width).
    std::vector<std::pair<TopoDS_Shape, double>> strips;
    auto distance_to = [&](const gp_Pnt& p, const TopoDS_Shape& shape) {
      BRepExtrema_DistShapeShape d(BRepBuilderAPI_MakeVertex(p).Vertex(), shape);
      if (!d.IsDone()) {
        failed = true;
        return 0.;
      }
      return d.Value();
    };
    auto meets_at = [&](const gp_Pnt& p) -> bool {
      const auto uv = analysis.ValueOfUV(p, resolution);
      if (!face_regions::region_holds(r, uv.X(), uv.Y(), inside))
        return false;
      if (!(distance_to(p, boundary) > exclusion))
        return false;
      for (const auto& [shape, width] : strips) {
        const double d = distance_to(p, shape);
        if (!(d > width))
          return false;
        // Review round 8, finding 4: just outside a strip the strip's own
        // contact model is least sure; a meet there is uncertain.
        if (!(d > 2 * width)) {
          uncertain = true;
          return false;
        }
      }
      return !failed;
    };
    double a0 = u0, a1 = u1, b0 = v0, b1 = v1;
    if (inside) {
      a0 = b0 = INFINITY;
      a1 = b1 = -INFINITY;
      for (const auto& line : r.polylines)
        for (const auto& p : line) {
          a0 = std::min(a0, p.X());
          a1 = std::max(a1, p.X());
          b0 = std::min(b0, p.Y());
          b1 = std::max(b1, p.Y());
        }
      if (r.band) {
        if (r.pu > 0) {
          a0 = u0;
          a1 = u1;
        }
        if (r.pv > 0) {
          b0 = v0;
          b1 = v1;
        }
      }
    }
    auto cell_live = [&](double c0, double c1, double d0, double d1) -> bool {
      const int ku = r.pu > 0 ? 2 : 0, kv = r.pv > 0 ? 2 : 0;
      for (const auto& line : r.polylines)
        for (const auto& p : line)
          for (int n = -ku; n <= ku; ++n)
            for (int m = -kv; m <= kv; ++m) {
              const double x = p.X() + n * r.pu, y = p.Y() + m * r.pv;
              if (x >= c0 && x <= c1 && y >= d0 && y <= d1)
                return true;
            }
      return face_regions::region_holds(r, .5 * (c0 + c1), .5 * (d0 + d1), inside);
    };
    std::function<bool(const TopoDS_Face&, const Handle(Geom_Surface)&, const Bnd_Box&, double, double, double, double, int)> cell_meets =
        [&](const TopoDS_Face& other, const Handle(Geom_Surface)& other_surface, const Bnd_Box& other_box, double c0, double c1,
            double d0, double d1, int depth) -> bool {
      if (!cell_live(c0, c1, d0, d1))
        return false;
      const double e0 = s->IsUPeriodic() ? c0 : std::max(c0, u0), e1 = s->IsUPeriodic() ? c1 : std::min(c1, u1);
      const double f0 = s->IsVPeriodic() ? d0 : std::max(d0, v0), f1 = s->IsVPeriodic() ? d1 : std::min(d1, v1);
      if (!(e0 < e1 && f0 < f1))
        return false;
      const Bnd_Box cell_box = face_regions::face_extent(s, e0, e1, f0, f1);
      if (cell_box.Distance(other_box) > resolution)
        return false;
      const double diagonal = std::sqrt(cell_box.SquareExtent());
      const gp_Pnt centre = s->Value(.5 * (e0 + e1), .5 * (f0 + f1));
      const double radius = detail::cell_radius(s, e0, e1, f0, f1, diagonal);
      // A lower bound first: the other face is no nearer than its surface, and
      // no cell point is farther than the radius from the cell's centre.
      {
        GeomAPI_ProjectPointOnSurf projection(centre, other_surface);
        if (projection.NbPoints() > 0 && projection.LowerDistance() - radius > resolution)
          return false;
      }
      // A cell touching a strip is split without a distance call (its closest
      // points would lie in the strip, along the common boundary) down to the
      // last level, and dropped there: the band left blind is the strip and
      // at most one last-level cell (1/128 of the chart box) beside it.
      for (const auto& [shape, width] : strips)
        if (distance_to(centre, shape) <= width + radius) {
          // Review round 8, finding 3: refined until the dropped cell is no
          // larger than the blind-band target (at most ten levels); a cell
          // still larger then leaves the decision undecided.
          if (depth >= 3 && 2 * radius <= blind_target) {
            out.cell_mm = std::max(out.cell_mm, 2 * radius);
            return false;
          }
          // The cap is ten levels: a cell whose last-level descendants would
          // still exceed the target is dropped now, the decision undecided.
          if (depth >= 10 || 2 * radius / double(1 << (10 - depth)) > blind_target) {
            out.cell_mm = std::max(out.cell_mm, 2 * radius);
            dropped_over_target = true;
            return false;
          }
          const double cm = .5 * (c0 + c1), dm = .5 * (d0 + d1);
          return cell_meets(other, other_surface, other_box, c0, cm, d0, dm, depth + 1) ||
                 cell_meets(other, other_surface, other_box, cm, c1, d0, dm, depth + 1) ||
                 cell_meets(other, other_surface, other_box, c0, cm, dm, d1, depth + 1) ||
                 cell_meets(other, other_surface, other_box, cm, c1, dm, d1, depth + 1);
        }
      BRepBuilderAPI_MakeFace cell(s, e0, e1, f0, f1, Precision::Confusion());
      if (!cell.IsDone()) {
        failed = true;
        return false;
      }
      BRepExtrema_DistShapeShape distance(cell.Face(), other);
      if (!distance.IsDone()) {
        failed = true;
        return false;
      }
      if (distance.Value() > resolution)
        return false;
      for (int k = 1; k <= distance.NbSolution(); ++k)
        if (meets_at(distance.PointOnShape1(k)))
          return true;
      if (depth >= 3)
        return false;
      const double cm = .5 * (c0 + c1), dm = .5 * (d0 + d1);
      return cell_meets(other, other_surface, other_box, c0, cm, d0, dm, depth + 1) ||
             cell_meets(other, other_surface, other_box, cm, c1, d0, dm, depth + 1) ||
             cell_meets(other, other_surface, other_box, c0, cm, dm, d1, depth + 1) ||
             cell_meets(other, other_surface, other_box, cm, c1, dm, d1, depth + 1);
    };
    TopTools_IndexedMapOfShape own_edges, own_vertices;
    TopExp::MapShapes(face, TopAbs_EDGE, own_edges);
    TopExp::MapShapes(face, TopAbs_VERTEX, own_vertices);
    for (const auto& neighbour : others) {
      const TopoDS_Face& other = neighbour.face;
      const TopoDS_Face& measure = neighbour.measure.IsNull() ? neighbour.face : neighbour.measure;
      // Review round 8, finding 2: a box that holds the neighbour (BRepBndLib's
      // can miss a cone's apex).
      const Bnd_Box other_box = shape_extent::face_bound(other);
      if (!other_box.IsWhole() && surface_box.Distance(other_box) > resolution)
        continue;
      const Handle(Geom_Surface) other_surface = detail::world_surface(other);
      // The neighbour's strips: each common edge, each common vertex not on one.
      strips.clear();
      bool crossings_only = false;
      TopTools_IndexedMapOfShape on_edges;
      for (TopExp_Explorer e(other, TopAbs_EDGE); e.More() && !crossings_only; e.Next()) {
        if (!own_edges.Contains(e.Current()))
          continue;
        const TopoDS_Edge edge = TopoDS::Edge(e.Current());
        TopExp::MapShapes(edge, TopAbs_VERTEX, on_edges);
        if (BRep_Tool::Degenerated(edge))
          continue;
        BRepAdaptor_Curve curve(edge);
        double widest = 0;
        for (int k = 0; k <= 16 && !crossings_only; ++k) {
          gp_Pnt p;
          gp_Vec t;
          curve.D1(curve.FirstParameter() + (curve.LastParameter() - curve.FirstParameter()) * k / 16., p, t);
          if (t.Magnitude() <= 1e-12) {
            crossings_only = true;
            break;
          }
          const gp_Dir tangent(t);
          widest = std::max(widest, detail::contact_width(s, other_surface, p, &tangent, resolution));
          if (!std::isfinite(widest))
            crossings_only = true;
        }
        strips.push_back({edge, exclusion + 2 * widest});
      }
      for (TopExp_Explorer v(other, TopAbs_VERTEX); v.More() && !crossings_only; v.Next()) {
        if (!own_vertices.Contains(v.Current()) || on_edges.Contains(v.Current()))
          continue;
        const double width = detail::contact_width(s, other_surface, BRep_Tool::Pnt(TopoDS::Vertex(v.Current())), nullptr, resolution);
        if (!std::isfinite(width))
          crossings_only = true;
        else
          strips.push_back({v.Current(), exclusion + 2 * width});
      }
      for (const auto& strip : strips)
        if (strip.second > .25 * size)
          crossings_only = true;
      if (crossings_only) {
        ++out.crossings_only;
        strips.clear();
      }
      for (const auto& strip : strips)
        out.strip_mm = std::max(out.strip_mm, strip.second);
      BRepExtrema_DistShapeShape distance(whole.Face(), measure);
      if (!distance.IsDone()) {
        out.result = -1;
        return out;
      }
      if (distance.Value() > resolution)
        continue;
      // Crossings (cheaper than cells).
      BRepAlgoAPI_Section section(whole.Face(), measure, false);
      section.SetFuzzyValue(resolution);
      // BOPAlgo widens its inputs' tolerances in place otherwise: these are the
      // part's translated faces.
      section.SetNonDestructive(true);
      section.Approximation(true);
      section.Build();
      if (!section.IsDone()) {
        out.result = -1;
        return out;
      }
      for (TopExp_Explorer e(section.Shape(), TopAbs_EDGE); e.More(); e.Next()) {
        BRepAdaptor_Curve curve(TopoDS::Edge(e.Current()));
        const double t0 = curve.FirstParameter(), t1 = curve.LastParameter();
        const double length = GCPnts_AbscissaPoint::Length(curve);
        const int samples = std::max(8, int(std::ceil(length / std::max(size / 1024., resolution))));
        for (int k = 0; k <= samples; ++k) {
          // A section point stands for the surface point under it, which meets
          // only within the resolution of the other face (the fuzzy section's
          // own tolerance also counts the shapes' tolerances).
          const gp_Pnt2d uv = analysis.ValueOfUV(curve.Value(t0 + (t1 - t0) * k / double(samples)), resolution);
          const gp_Pnt on = s->Value(uv.X(), uv.Y());
          if (meets_at(on) && distance_to(on, measure) <= resolution) {
            out.result = 1;
            return out;
          }
        }
        if (failed) {
          out.result = -1;
          return out;
        }
      }
      if (crossings_only)
        continue;
      for (int k = 1; k <= distance.NbSolution(); ++k)
        if (meets_at(distance.PointOnShape1(k))) {
          out.result = 1;
          return out;
        }
      for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j)
          if (cell_meets(measure, other_surface, other_box, a0 + (a1 - a0) * i / 16., a0 + (a1 - a0) * (i + 1) / 16.,
                         b0 + (b1 - b0) * j / 16., b0 + (b1 - b0) * (j + 1) / 16., 0)) {
            out.result = 1;
            return out;
          }
      if (failed) {
        out.result = -1;
        return out;
      }
    }
    out.result = uncertain ? 2 : dropped_over_target ? 3 : 0;
    return out;
  } catch (const Standard_Failure&) {
    out.result = -1;
    return out;
  }
}

} // namespace brepgpu::face_contact

#endif
