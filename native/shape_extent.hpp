#ifndef BREPGPU_SHAPE_EXTENT_HPP
#define BREPGPU_SHAPE_EXTENT_HPP
// A box that holds every point of a translated shape (B2 .37.39), from its
// faces' carriers and 3D edges only: no pcurves, no UV hint and no declared
// uncertainty, so it serves before the source-UV stage (the G6 UR-7
// usability test of a declared uncertainty against the definition's
// diagonal) and in the reference exporter, which has no UV hints.
// BRepBndLib boxes a face without triangulation over the UV box of its
// pcurves: where they are missing it boxes the whole unbounded carrier (about
// 1e100 mm), and where they are present it misses the region beyond them (a
// cone's apex, bounded by its base circle alone); it boxes a B-spline curve
// from samples reduced against its poles, which is no guaranteed bound. Each
// face is bounded here by the convex-hull, line and sweep arguments below; a
// carrier or curve with no such argument makes the box whole (unbounded),
// which never makes a claim it cannot hold.
// Written for the STEP source adapter; the reference exporter calls it too.
#include <BRepBndLib.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <BndLib_Add3dCurve.hxx>
#include <BndLib_AddSurface.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Surface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <algorithm>
#include <cmath>

namespace brepgpu::shape_extent {

// A curve over [a, b]: conics and lines by OCCT's exact boxes, Bezier and
// B-spline curves by their poles' hull (positive weights), anything else, or
// an unbounded range, whole.
inline Bnd_Box curve_bound(const Handle(Geom_Curve)& curve, double a, double b) {
  Bnd_Box box;
  if (curve.IsNull() || !(std::isfinite(a) && std::isfinite(b))) {
    box.SetWhole();
    return box;
  }
  GeomAdaptor_Curve adaptor(curve, a, b);
  switch (adaptor.GetType()) {
  case GeomAbs_Line:
  case GeomAbs_Circle:
  case GeomAbs_Ellipse:
  case GeomAbs_Hyperbola:
  case GeomAbs_Parabola:
    BndLib_Add3dCurve::Add(adaptor, 0., box);
    return box;
  case GeomAbs_BSplineCurve: {
    const auto spline = adaptor.BSpline();
    for (int i = 1; i <= spline->NbPoles(); ++i)
      box.Add(spline->Pole(i));
    return box;
  }
  case GeomAbs_BezierCurve: {
    const auto bezier = adaptor.Bezier();
    for (int i = 1; i <= bezier->NbPoles(); ++i)
      box.Add(bezier->Pole(i));
    return box;
  }
  default:
    box.SetWhole();
    return box;
  }
}

// An edge: its 3D curve's bound over its range, enlarged by its tolerance.
inline Bnd_Box edge_bound(const TopoDS_Edge& edge) {
  double a = 0, b = 0;
  TopLoc_Location location;
  Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, location, a, b);
  Bnd_Box box;
  if (curve.IsNull()) {
    // A degenerate edge is a point at its vertex; any other has no bound.
    if (BRep_Tool::Degenerated(edge))
      BRepBndLib::Add(edge, box);
    else
      box.SetWhole();
    return box;
  }
  if (!location.IsIdentity())
    curve = Handle(Geom_Curve)::DownCast(curve->Transformed(location.Transformation()));
  box = curve_bound(curve, a, b);
  if (!box.IsWhole())
    box.Enlarge(BRep_Tool::Tolerance(edge));
  return box;
}

namespace detail {
// The axial range of a box's corners on an axis (origin o, unit z), in units of
// `scale` per parameter.
inline void axial(const Bnd_Box& box, const gp_Pnt& o, const gp_Dir& z, double scale, double& lo, double& hi) {
  double x[2], y[2], w[2];
  box.Get(x[0], y[0], w[0], x[1], y[1], w[1]);
  lo = INFINITY;
  hi = -INFINITY;
  for (int c = 0; c < 8; ++c) {
    const double t = gp_Vec(o, gp_Pnt(x[c & 1], y[(c >> 1) & 1], w[(c >> 2) & 1])).Dot(gp_Vec(z)) / scale;
    lo = std::min(lo, t);
    hi = std::max(hi, t);
  }
}
// The bounding cylinder about an axis of a box swept about it: the corners'
// largest distance from the axis and their axial range.
inline Bnd_Box swept(const Bnd_Box& box, const gp_Ax1& axis) {
  double x[2], y[2], w[2];
  box.Get(x[0], y[0], w[0], x[1], y[1], w[1]);
  double lo = INFINITY, hi = -INFINITY, radius = 0;
  const gp_Vec z(axis.Direction());
  for (int c = 0; c < 8; ++c) {
    const gp_Vec q(axis.Location(), gp_Pnt(x[c & 1], y[(c >> 1) & 1], w[(c >> 2) & 1]));
    const double t = q.Dot(z);
    lo = std::min(lo, t);
    hi = std::max(hi, t);
    radius = std::max(radius, (q - t * z).Magnitude());
  }
  // The cylinder's box: its two end disks' boxes.
  Bnd_Box out;
  const gp_Dir d = axis.Direction();
  for (double t : {lo, hi}) {
    const gp_Pnt c = axis.Location().Translated(t * z);
    for (int k = 0; k < 3; ++k) {
      const double n = d.Coord(k + 1);
      const double extent = radius * std::sqrt(std::max(0., 1. - n * n));
      gp_Pnt a = c, b = c;
      a.SetCoord(k + 1, c.Coord(k + 1) - extent);
      b.SetCoord(k + 1, c.Coord(k + 1) + extent);
      out.Add(a);
      out.Add(b);
    }
  }
  return out;
}
} // namespace detail

// A surface's points on one face, given the box of the face's edges (edged:
// the face has an edge that bounds it):
// - plane: its region lies in the convex hull of its boundary, so its edges'
//   box holds it;
// - cylinder: the generator through any point of a bounded face leaves the
//   face through its boundary on both sides, so the face lies on the cylinder
//   between the extreme axial coordinates of its edges' box;
// - cone: likewise, except that toward the apex the generator may end at the
//   apex inside the face, so the apex is included;
// - linear extrusion of a curve C along d: likewise along its straight
//   generators, so every point is C(u) + v d with its coordinate along d in
//   the edges' range; the box of C swept along d over that v-range holds it;
// - revolution of a bounded generatrix about an axis: the generatrix's box
//   swept about the axis (a bounding cylinder);
// - offset by o of a basis: the basis's bound over the axial range grown by
//   |o| (its points move by |o| along their normals), grown by |o|;
// - sphere, torus: the whole surface; Bezier and B-spline: their poles' hull
//   over the natural domain;
// - any other carrier, or a curved face with no edge to bound it on an
//   unbounded carrier: whole.
inline Bnd_Box surface_bound(const Handle(Geom_Surface)& surface, const Bnd_Box& edges, bool edged, double grow = 0.) {
  Bnd_Box box;
  GeomAdaptor_Surface adaptor(surface);
  const bool usable = edged && !edges.IsOpen() && !edges.IsWhole() && !edges.IsVoid();
  switch (adaptor.GetType()) {
  case GeomAbs_Plane:
    if (!usable)
      box.SetWhole();
    else
      box = edges;
    return box;
  case GeomAbs_Cylinder: {
    if (!usable) {
      box.SetWhole();
      return box;
    }
    const auto axis = adaptor.Cylinder().Position();
    double lo, hi;
    detail::axial(edges, axis.Location(), axis.Direction(), 1., lo, hi);
    BndLib_AddSurface::Add(GeomAdaptor_Surface(surface, 0., 2 * M_PI, lo - grow, hi + grow), 0., box);
    box.Add(edges);
    return box;
  }
  case GeomAbs_Cone: {
    if (!usable) {
      box.SetWhole();
      return box;
    }
    const auto cone = adaptor.Cone();
    const double c = std::cos(cone.SemiAngle());
    double lo, hi;
    detail::axial(edges, cone.Position().Location(), cone.Position().Direction(), c, lo, hi);
    const double apex = -cone.RefRadius() / std::sin(cone.SemiAngle());
    lo = std::min(lo - grow / c, apex);
    hi = std::max(hi + grow / c, apex);
    BndLib_AddSurface::Add(GeomAdaptor_Surface(surface, 0., 2 * M_PI, lo, hi), 0., box);
    box.Add(edges);
    return box;
  }
  case GeomAbs_SurfaceOfExtrusion: {
    const auto extrusion = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(surface);
    if (!usable || extrusion.IsNull()) {
      box.SetWhole();
      return box;
    }
    const Handle(Geom_Curve) curve = extrusion->BasisCurve();
    const Bnd_Box base = curve_bound(curve, curve->FirstParameter(), curve->LastParameter());
    if (base.IsWhole() || base.IsOpen() || base.IsVoid()) {
      box.SetWhole();
      return box;
    }
    const gp_Dir d = extrusion->Direction();
    double elo, ehi, clo, chi;
    detail::axial(edges, gp::Origin(), d, 1., elo, ehi);
    detail::axial(base, gp::Origin(), d, 1., clo, chi);
    // v = P.d - C(u).d lies in [elo - chi, ehi - clo]: the box of C at both ends.
    double x0, y0, z0, x1, y1, z1;
    base.Get(x0, y0, z0, x1, y1, z1);
    for (double v : {elo - chi - grow, ehi - clo + grow}) {
      const gp_Vec shift = v * gp_Vec(d);
      box.Add(gp_Pnt(x0, y0, z0).Translated(shift));
      box.Add(gp_Pnt(x1, y1, z1).Translated(shift));
    }
    box.Add(base);
    box.Add(edges);
    box.Enlarge(grow);
    return box;
  }
  case GeomAbs_SurfaceOfRevolution: {
    const auto revolution = Handle(Geom_SurfaceOfRevolution)::DownCast(surface);
    if (revolution.IsNull()) {
      box.SetWhole();
      return box;
    }
    const Handle(Geom_Curve) curve = revolution->BasisCurve();
    const Bnd_Box base = curve_bound(curve, curve->FirstParameter(), curve->LastParameter());
    if (base.IsWhole() || base.IsOpen() || base.IsVoid()) {
      box.SetWhole();
      return box;
    }
    box = detail::swept(base, revolution->Axis());
    if (usable)
      box.Add(edges);
    box.Enlarge(grow);
    return box;
  }
  case GeomAbs_OffsetSurface: {
    const auto offset = Handle(Geom_OffsetSurface)::DownCast(surface);
    if (offset.IsNull()) {
      box.SetWhole();
      return box;
    }
    const double o = std::abs(offset->Offset());
    // The edges lie on the offset surface; the basis points under the face lie
    // within |o| of them, so the basis is bounded with the axial range grown by
    // |o|, and the result grown by |o|.
    box = surface_bound(offset->BasisSurface(), edges, edged, grow + o);
    if (!box.IsWhole())
      box.Enlarge(o);
    return box;
  }
  case GeomAbs_Sphere:
  case GeomAbs_Torus:
  case GeomAbs_BezierSurface:
  case GeomAbs_BSplineSurface: {
    double u0, u1, v0, v1;
    surface->Bounds(u0, u1, v0, v1);
    if (!(std::isfinite(u0) && std::isfinite(u1) && std::isfinite(v0) && std::isfinite(v1))) {
      box.SetWhole();
      return box;
    }
    BndLib_AddSurface::Add(adaptor, 0., box);
    if (usable)
      box.Add(edges);
    box.Enlarge(grow);
    return box;
  }
  default:
    box.SetWhole();
    return box;
  }
}

// One face: its carrier's bound on it, with its edges' box.
inline Bnd_Box face_bound(const TopoDS_Face& face) {
  Bnd_Box edges;
  bool edged = false;
  for (TopExp_Explorer e(face, TopAbs_EDGE); e.More(); e.Next())
    if (!BRep_Tool::Degenerated(TopoDS::Edge(e.Current()))) {
      edges.Add(edge_bound(TopoDS::Edge(e.Current())));
      edged = true;
    }
  TopLoc_Location location;
  Handle(Geom_Surface) surface = BRep_Tool::Surface(face, location);
  if (surface.IsNull()) {
    Bnd_Box box;
    box.SetWhole();
    return box;
  }
  if (!location.IsIdentity())
    surface = Handle(Geom_Surface)::DownCast(surface->Transformed(location.Transformation()));
  return surface_bound(surface, edges, edged);
}

// A shape: its faces' bounds; a shape without faces, its edges and vertices.
inline Bnd_Box shape_bound(const TopoDS_Shape& shape) {
  Bnd_Box box;
  bool faced = false;
  for (TopExp_Explorer f(shape, TopAbs_FACE); f.More(); f.Next()) {
    box.Add(face_bound(TopoDS::Face(f.Current())));
    faced = true;
  }
  if (!faced && !shape.IsNull()) {
    for (TopExp_Explorer e(shape, TopAbs_EDGE); e.More(); e.Next())
      box.Add(edge_bound(TopoDS::Edge(e.Current())));
    for (TopExp_Explorer v(shape, TopAbs_VERTEX); v.More(); v.Next())
      BRepBndLib::Add(v.Current(), box);
  }
  return box;
}

// The diagonal UR-7 compares a declared uncertainty with: infinite when the
// bound is open (then no declared value is usable), 0 for an empty shape.
inline double shape_diagonal(const TopoDS_Shape& shape) {
  const Bnd_Box box = shape_bound(shape);
  if (box.IsVoid())
    return 0.;
  if (box.IsOpen() || box.IsWhole())
    return INFINITY;
  return std::sqrt(box.SquareExtent());
}

} // namespace brepgpu::shape_extent

#endif
