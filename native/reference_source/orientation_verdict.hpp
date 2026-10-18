#ifndef BREPGPU_REFERENCE_ORIENTATION_VERDICT_HPP
#define BREPGPU_REFERENCE_ORIENTATION_VERDICT_HPP
// Face sides (beads .37.35, .37.36; B2-reader-export review round 1).
// - sample: an interior point of an exported piece, OCCT's surface normal there,
//   and the raw use's outward normal (the raw surface's normal, from OCCT's
//   translation of the raw face without ShapeProcess, times same_sense composed
//   with its shell).
// - raw_use_side: which side the raw use's normal faces, by ray parity from
//   probes on both sides of the piece, confirmed by OCCT's solid classifier.
// - face_verdict: over every piece of a face, the raw side against OCCT's
//   reading (repaired, reversed together, turned against the shell, a piece
//   left inward, or not decidable).
// - bound_contradicts: whether the raw face's outer bound runs against the face
//   (S1) rather than the whole shell being inside out (S2).
// - verify_solid: BRepCheck and per-shell signed volumes of a corrected solid.
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Wire.hxx>
#include <BRep_Builder.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <NCollection_IndexedMap.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopAbs.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <ShapeFix_Edge.hxx>
#include <Geom2d_Curve.hxx>
#include <TopoDS_Wire.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <TopoDS_TShape.hxx>
#include <map>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomLProp_SLProps.hxx>
#include <Geom_Surface.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <algorithm>
#include <cmath>
#include <string>
#include <iomanip>
#include <sstream>
#include <limits>
#include <utility>
#include <vector>

namespace brepgpu::reference_orientation {
inline const char *const repaired = "turned: the raw use points into the material";
inline const char *const turned_correct = "turned: the raw use agrees with the closed shell";
inline const char *const left_inward = "turned: OCCT's default read leaves a piece of the face inward";

// An interior point of a piece, with OCCT's surface normal there (not yet
// oriented), and the raw surface's normal at its projection times the use's
// sign; whether the point lies on the raw surface within the piece's tolerance.
struct Sample {
  bool sampled = false, near = false;
  gp_Pnt point;
  gp_Vec occt_surface_normal, raw_normal;
};
inline Sample sample(const TopoDS_Face &piece, const Handle(Geom_Surface) &raw_surface, double used_sign) {
  Sample out;
  double u0 = 0, u1 = 0, v0 = 0, v1 = 0;
  BRepTools::UVBounds(piece, u0, u1, v0, v1);
  BRepTopAdaptor_FClass2d inside(piece, Precision::PConfusion());
  BRepAdaptor_Surface surface(piece);
  for (int a = 0; a < 8; ++a)
    for (int b = 0; b < 8; ++b) {
      // A golden-fraction shift keeps the grid off rational points of the face.
      const double u = u0 + (u1 - u0) * (a + 0.381966) / 8., v = v0 + (v1 - v0) * (b + 0.381966) / 8.;
      if (inside.Perform(gp_Pnt2d(u, v)) != TopAbs_IN)
        continue;
      gp_Pnt point;
      gp_Vec du, dv;
      surface.D1(u, v, point, du, dv);
      gp_Vec normal = du.Crossed(dv);
      if (normal.Magnitude() <= 1e-12)
        continue;
      normal.Normalize();
      GeomAPI_ProjectPointOnSurf projection(point, raw_surface);
      if (!projection.IsDone() || projection.NbPoints() == 0)
        continue;
      double ru = 0, rv = 0;
      projection.LowerDistanceParameters(ru, rv);
      GeomLProp_SLProps properties(raw_surface, ru, rv, 1, Precision::Confusion());
      if (!properties.IsNormalDefined())
        continue;
      out.raw_normal = gp_Vec(properties.Normal());
      out.raw_normal.Multiply(used_sign);
      out.near = projection.LowerDistance() <= std::max(BRep_Tool::Tolerance(piece), Precision::Confusion());
      out.occt_surface_normal = normal;
      out.point = point;
      out.sampled = true;
      return out;
    }
  return out;
}

// Which side the raw use's outward normal faces, from geometry alone. At an
// interior point of the piece, two probes, just off the face on each side along
// the raw use's normal, at an offset scaled to the piece (1e-4 of its box
// diagonal, at least ten times its tolerance). From each probe, rays in seven
// generic directions count crossings with the solid's faces: at least five of
// the seven must give the same parity, and the two probes opposite parities. OCCT's BRepClass3d_SolidClassifier must classify the two probes the
// same way (odd parity inside). "into_material" (the probe on the raw normal's
// side is inside), "outward", or "undecidable: ..." on any disagreement, and
// where a shell of the solid is not closed (parity then decides nothing).
inline std::string failed_near_face(const Standard_Failure &failure) {
  const std::string what = failure.what() ? failure.what() : "";
  return "undecidable: OCCT's evaluation failed near this face" + (what.empty() ? std::string() : " (" + what + ")");
}
inline std::string raw_use_side_unguarded(const TopoDS_Face &piece, const Handle(Geom_Surface) &raw_surface,
                                          double used_sign, const TopoDS_Shape &solid);
// An OCCT exception on the way (a ray or the classifier meeting a face it
// cannot evaluate; seen on the M8X25 screw's tip faces) makes the side
// undecidable, by name, rather than ending the export with OCCT's message.
inline std::string raw_use_side(const TopoDS_Face &piece, const Handle(Geom_Surface) &raw_surface,
                                double used_sign, const TopoDS_Shape &solid) {
  try {
    return raw_use_side_unguarded(piece, raw_surface, used_sign, solid);
  } catch (const Standard_Failure &failure) {
    return failed_near_face(failure);
  }
}
inline std::string raw_use_side_unguarded(const TopoDS_Face &piece, const Handle(Geom_Surface) &raw_surface,
                                          double used_sign, const TopoDS_Shape &solid) {
  const Sample s = sample(piece, raw_surface, used_sign);
  if (!s.sampled)
    return "undecidable: no interior point with a defined normal on its exported face";
  if (!s.near)
    return "undecidable: its exported face leaves the raw surface beyond its tolerance";
  for (TopExp_Explorer shell(solid, TopAbs_SHELL); shell.More(); shell.Next())
    if (!BRep_Tool::IsClosed(shell.Current()))
      return "undecidable: its solid has a shell that is not closed, so ray parity does not decide its material";
  Bnd_Box piece_box;
  BRepBndLib::Add(piece, piece_box);
  const double offset = std::max(1e-4 * std::sqrt(piece_box.SquareExtent()),
                                 10 * std::max(BRep_Tool::Tolerance(piece), Precision::Confusion()));
  IntCurvesFace_ShapeIntersector crossings;
  crossings.Load(solid, Precision::Confusion());
  int parity[2] = {-1, -1};
  TopAbs_State state[2] = {TopAbs_UNKNOWN, TopAbs_UNKNOWN};
  for (int side = 0; side < 2; ++side) {
    const gp_Pnt probe = s.point.Translated(s.raw_normal * (side == 0 ? offset : -offset));
    // Seven directions spread over the sphere by the golden angle (no rational
    // ratios between components, so no ray is aimed along the model's edges).
    // A ray grazing an edge can still count it twice: a strict majority of at
    // least five of the seven must agree.
    int odd = 0;
    for (int k = 0; k < 7; ++k) {
      const double z = 1 - 2 * (k + 0.5) / 7, r = std::sqrt(1 - z * z), phi = 2.399963229728653 * k + 0.5;
      crossings.Perform(gp_Lin(probe, gp_Dir(r * std::cos(phi), r * std::sin(phi), z)), 0., Precision::Infinite());
      odd += crossings.NbPnt() % 2;
    }
    if (odd >= 5)
      parity[side] = 1;
    else if (odd <= 2)
      parity[side] = 0;
    else
      return "undecidable: ray parity depends on the ray direction near this face";
    BRepClass3d_SolidClassifier classifier(solid, probe, Precision::Confusion());
    state[side] = classifier.State();
  }
  if (parity[0] == parity[1])
    return "undecidable: ray parity is the same on both sides of this face";
  for (int side = 0; side < 2; ++side)
    if (state[side] != (parity[side] ? TopAbs_IN : TopAbs_OUT))
      return "undecidable: OCCT's solid classifier disagrees with ray parity near this face";
  return parity[0] ? "into_material" : "outward";
}

// Projected pcurves on a copy, where OCCT's translation without ShapeProcess
// left an edge use without one (G1 E3): the checks below need them, and only
// the copy is read; nothing published changes. The copy only adds what is
// missing (B2-reader-export review round 2): each edge use without a pcurve on
// its face gets one projected (ShapeFix_Edge::FixAddPCurve; a seam its two),
// and each added pcurve of a closed surface is moved by whole periods to meet
// the end of a neighbour in its wire, starting from the pcurves that were
// there. A pcurve that was there is never moved, re-paired or made
// same-parameter; no edge is removed, split, reordered or joined, and no 3D
// curve is made or dropped, so the copy's topology, 3D geometry and existing
// parameter curves are the translation's.
inline gp_Pnt2d use_end(const TopoDS_Edge &edge, const TopoDS_Face &face, bool start) {
  double first = 0, last = 0;
  const Handle(Geom2d_Curve) curve = BRep_Tool::CurveOnSurface(edge, face, first, last);
  const bool forward = edge.Orientation() != TopAbs_REVERSED;
  return curve->Value(start == forward ? first : last);
}
// One wire of a face as traversed: its edge uses in order (a reversed wire is
// traversed against its stored order).
inline std::vector<TopoDS_Edge> traversal(const TopoDS_Shape &wire) {
  std::vector<TopoDS_Edge> edges;
  for (TopoDS_Iterator it(wire); it.More(); it.Next())
    edges.push_back(TopoDS::Edge(it.Value()));
  if (wire.Orientation() == TopAbs_REVERSED)
    std::reverse(edges.begin(), edges.end());
  return edges;
}
// Every wire of the face closes in the parameter plane: BRepCheck finds each
// wire closed there, and on a closed surface each edge use ends where the
// next begins up to a thousandth of the period in that direction. The
// periodic test separates the period a pcurve or a seam use lies in (wrong by
// a whole period, or a seam use on its other pcurve); it says nothing about
// the small gaps OCCT's own translation leaves between pcurve ends (BRepCheck
// reads closure through the vertices, and those are admitted there).
inline bool wires_close_2d(const TopoDS_Face &face) {
  BRepAdaptor_Surface surface(face, false);
  const double up = surface.IsUPeriodic() ? surface.UPeriod() : 0, vp = surface.IsVPeriodic() ? surface.VPeriod() : 0;
  for (TopExp_Explorer w(face, TopAbs_WIRE); w.More(); w.Next()) {
    Handle(BRepCheck_Wire) check = new BRepCheck_Wire(TopoDS::Wire(w.Current()));
    if (check->Closed2d(face) != BRepCheck_NoError)
      return false;
    if (!(up > 0 || vp > 0))
      continue;
    const auto edges = traversal(w.Current());
    const std::size_t n = edges.size();
    for (std::size_t i = 0; i < n; ++i) {
      double first = 0, last = 0;
      if (BRep_Tool::CurveOnSurface(edges[i], face, first, last).IsNull() ||
          BRep_Tool::CurveOnSurface(edges[(i + 1) % n], face, first, last).IsNull())
        return false;
      const gp_Vec2d gap(use_end(edges[i], face, false), use_end(edges[(i + 1) % n], face, true));
      if ((up > 0 && std::abs(gap.X()) > 1e-3 * up) || (vp > 0 && std::abs(gap.Y()) > 1e-3 * vp))
        return false;
    }
  }
  return true;
}
// Each seam use of a face on its own pcurve (review round 2): the pairing of
// the seam's two pcurves under which every wire of the face, completed where
// pcurves are missing, closes in the parameter plane. OCCT's default read
// pairs a repaired seam face this way. The pairing is changed on the face
// itself (its seam edges are its own: a seam is used twice by one face) only
// when the present one does not close; false when no pairing closes.
inline void swap_seam(const TopoDS_Edge &seam, const TopoDS_Face &face) {
  const TopoDS_Face forward = TopoDS::Face(face.Oriented(TopAbs_FORWARD));
  const TopoDS_Edge edge = TopoDS::Edge(seam.Oriented(TopAbs_FORWARD));
  double f1 = 0, l1 = 0, f2 = 0, l2 = 0;
  const Handle(Geom2d_Curve) c1 = BRep_Tool::CurveOnSurface(edge, forward, f1, l1);
  const Handle(Geom2d_Curve) c2 = BRep_Tool::CurveOnSurface(TopoDS::Edge(edge.Reversed()), forward, f2, l2);
  BRep_Builder().UpdateEdge(edge, c2, c1, forward, BRep_Tool::Tolerance(edge));
}
// An added seam's two pcurves moved together by a vector (whole periods).
inline void shift_seam(const TopoDS_Edge &seam, const TopoDS_Face &face, const gp_Vec2d &by) {
  const TopoDS_Face forward = TopoDS::Face(face.Oriented(TopAbs_FORWARD));
  const TopoDS_Edge edge = TopoDS::Edge(seam.Oriented(TopAbs_FORWARD));
  double f1 = 0, l1 = 0, f2 = 0, l2 = 0;
  Handle(Geom2d_Curve) c1 = Handle(Geom2d_Curve)::DownCast(BRep_Tool::CurveOnSurface(edge, forward, f1, l1)->Copy());
  Handle(Geom2d_Curve) c2 = Handle(Geom2d_Curve)::DownCast(
      BRep_Tool::CurveOnSurface(TopoDS::Edge(edge.Reversed()), forward, f2, l2)->Copy());
  c1->Translate(by);
  c2->Translate(by);
  BRep_Builder().UpdateEdge(edge, c1, c2, forward, BRep_Tool::Tolerance(edge));
}
// What the completion did to each edge of the shape (coordinator ruling after
// review round 2: a tolerance is raised only on an edge that carries an added
// pcurve, only to the measured deviation of its added pcurves, and the
// largest one used is recorded): whether it carries an added pcurve, OCCT's
// tolerance, and the copy's.
struct EdgeCompletion {
  bool added = false;
  double before = 0, after = 0;
};
struct Completion {
  std::vector<EdgeCompletion> edges;
  double largest_raised_tolerance_mm = 0, largest_added_deviation_mm = 0;
};
inline TopoDS_Shape with_pcurves(const TopoDS_Shape &shape, Completion *report = nullptr) {
  BRepBuilderAPI_Copy copy(shape);
  const TopoDS_Shape out = copy.Shape();
  ShapeFix_Edge fix;
  BRep_Builder builder;
  std::vector<std::pair<TopoDS_Edge, TopoDS_Face>> added;
  for (TopExp_Explorer f(out, TopAbs_FACE); f.More(); f.Next()) {
    const TopoDS_Face face = TopoDS::Face(f.Current());
    std::map<const TopoDS_TShape *, int> uses;
    for (TopExp_Explorer e(face, TopAbs_EDGE); e.More(); e.Next())
      ++uses[e.Current().TShape().get()];
    // Add each missing pcurve; keep the added non-seam projections to place.
    std::vector<TopoDS_Edge> added_seams;
    std::vector<std::pair<TopoDS_Edge, Handle(Geom2d_Curve)>> projected;
    std::vector<std::vector<TopoDS_Edge>> wires;
    for (TopExp_Explorer w(face, TopAbs_WIRE); w.More(); w.Next()) {
      wires.push_back(traversal(w.Current()));
      for (const auto &edge : wires.back()) {
        double first = 0, last = 0;
        if (!BRep_Tool::CurveOnSurface(edge, face, first, last).IsNull())
          continue;
        const bool seam = uses[edge.TShape().get()] > 1;
        fix.FixAddPCurve(edge, face, seam, Precision::Confusion());
        const Handle(Geom2d_Curve) curve = BRep_Tool::CurveOnSurface(edge, face, first, last);
        if (curve.IsNull())
          continue;
        added.push_back({edge, face});
        bool seen = false;
        for (const auto &e : added_seams)
          seen |= e.IsSame(edge);
        if (seam) {
          if (!seen)
            added_seams.push_back(edge);
        } else
          projected.push_back({edge, Handle(Geom2d_Curve)::DownCast(curve->Copy())});
      }
    }
    if (projected.empty() && added_seams.empty())
      continue;
    BRepAdaptor_Surface surface(face, false);
    const double up = surface.IsUPeriodic() ? surface.UPeriod() : 0, vp = surface.IsVPeriodic() ? surface.VPeriod() : 0;
    auto is_projected = [&](const TopoDS_Edge &edge) {
      for (const auto &p : projected)
        if (p.first.IsSame(edge))
          return true;
      return false;
    };
    // Each added non-seam pcurve, from its projection, moved by whole periods
    // to meet the end of its neighbour in the traversal, starting from the
    // pcurves that were there (and the added seams).
    auto place = [&] {
      for (const auto &p : projected)
        builder.UpdateEdge(p.first, Handle(Geom2d_Curve)::DownCast(p.second->Copy()), face, BRep_Tool::Tolerance(p.first));
      if (!(up > 0 || vp > 0))
        return;
      for (const auto &edges : wires) {
        const int n = int(edges.size());
        std::vector<int> state(n); // 0 fixed, 1 to place, -1 none
        bool anchored = false;
        for (int i = 0; i < n; ++i) {
          double first = 0, last = 0;
          state[i] = BRep_Tool::CurveOnSurface(edges[i], face, first, last).IsNull() ? -1 : is_projected(edges[i]) ? 1 : 0;
          anchored |= state[i] == 0;
        }
        if (!anchored)
          for (int i = 0; i < n; ++i)
            if (state[i] == 1) {
              state[i] = 0;
              break;
            }
        for (bool moved = true; moved;) {
          moved = false;
          for (int i = 0; i < n; ++i) {
            if (state[i] != 1)
              continue;
            const int previous = (i + n - 1) % n, next = (i + 1) % n;
            gp_Vec2d gap;
            if (state[previous] == 0)
              gap = gp_Vec2d(use_end(edges[i], face, true), use_end(edges[previous], face, false));
            else if (state[next] == 0)
              gap = gp_Vec2d(use_end(edges[i], face, false), use_end(edges[next], face, true));
            else
              continue;
            const double ku = up > 0 ? std::round(gap.X() / up) : 0, kv = vp > 0 ? std::round(gap.Y() / vp) : 0;
            if (ku != 0 || kv != 0) {
              double first = 0, last = 0;
              Handle(Geom2d_Curve) curve = Handle(Geom2d_Curve)::DownCast(
                  BRep_Tool::CurveOnSurface(edges[i], face, first, last)->Copy());
              curve->Translate(gp_Vec2d(ku * up, kv * vp));
              builder.UpdateEdge(edges[i], curve, face, BRep_Tool::Tolerance(edges[i]));
            }
            state[i] = 0;
            moved = true;
          }
        }
      }
    };
    place();
    // A seam whose pcurves were both missing gets the pairing, and the whole
    // period its pair lies in, under which the face's wires close (the added
    // pair has neither of its own yet; the projection picks a period).
    if (!added_seams.empty() && added_seams.size() <= 2 && !wires_close_2d(face)) {
      const gp_Vec2d step(up > 0 ? up : 0, up > 0 ? 0 : vp);
      // Six options per seam: kept or swapped, in its period or one either side.
      const int seams = int(added_seams.size()), options = seams == 1 ? 6 : 36;
      auto apply = [&](int option, int sign) {
        for (int k = 0, o = option; k < seams; ++k, o /= 6) {
          const int choice = o % 6;
          if (choice >= 3)
            swap_seam(added_seams[k], face);
          const int shift = choice % 3 == 1 ? 1 : choice % 3 == 2 ? -1 : 0;
          if (shift)
            shift_seam(added_seams[k], face, step * double(sign * shift));
        }
      };
      bool closed = false;
      for (int option = 1; option < options && !closed; ++option) {
        apply(option, 1);
        place();
        closed = wires_close_2d(face);
        if (!closed)
          apply(option, -1); // shifts undone; a swap undoes itself
      }
      if (!closed)
        place();
    }
  }
  // An added pcurve is a projection, apart from its 3D curve by the
  // projection's error, which can exceed the translated edge's tolerance
  // (OCCT's translation gives 1e-7 mm): BRepCheck would read the copy's own
  // projection as an invalid curve on surface. Each edge with an added pcurve
  // takes the largest deviation of its added pcurves (ShapeAnalysis_Edge,
  // 23 points) as its tolerance where that is larger, and its vertices cover
  // it; no pcurve, existing or added, is changed by this.
  ShapeAnalysis_Edge analysis;
  for (const auto &[edge, face] : added) {
    double deviation = 0;
    analysis.CheckSameParameter(edge, face, deviation);
    if (report)
      report->largest_added_deviation_mm = std::max(report->largest_added_deviation_mm, deviation);
    if (report)
      report->largest_raised_tolerance_mm = std::max(report->largest_raised_tolerance_mm,
                                                     deviation > BRep_Tool::Tolerance(edge) ? deviation : 0.);
    if (deviation > BRep_Tool::Tolerance(edge)) {
      builder.UpdateEdge(edge, deviation);
      for (TopExp_Explorer v(edge, TopAbs_VERTEX); v.More(); v.Next())
        if (BRep_Tool::Tolerance(TopoDS::Vertex(v.Current())) < deviation)
          builder.UpdateVertex(TopoDS::Vertex(v.Current()), deviation);
    }
  }
  if (report) {
    NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> originals;
    TopExp::MapShapes(shape, TopAbs_EDGE, originals);
    for (int i = 1; i <= originals.Extent(); ++i) {
      const TopoDS_Edge original = TopoDS::Edge(originals(i)), edge = TopoDS::Edge(copy.ModifiedShape(original));
      EdgeCompletion row;
      for (const auto &a : added)
        row.added |= a.first.IsSame(edge);
      row.before = BRep_Tool::Tolerance(original);
      row.after = BRep_Tool::Tolerance(edge);
      report->edges.push_back(row);
    }
  }
  return out;
}

// Whether a raw face's outer bound runs against the face (bead .37.36, S1): on
// a pcurve-completed copy of OCCT's translation of the raw face, the infinite
// point of its parameter plane classifies inside the face. 1 contradicts, 0
// agrees, -1 not decidable.
//
// The classification needs every wire closed in the parameter plane. A seam
// edge's two pcurves are chosen by its use composed with the face's
// orientation, so OCCT's translation of a seam face whose use runs against its
// bound can pair them so that the wire does not close. The S1 candidate (the
// face as an S1 correction writes it: turned, each wire reversed inside it, so
// every edge keeps its use in the shell), with each seam use on the pcurve that
// closes its wire (pair_seams), is classified then: infinite point outside
// means the candidate is consistent, so the raw use contradicted its bound.
inline bool pair_seams(const TopoDS_Face &face) {
  auto closes = [&] { return wires_close_2d(TopoDS::Face(with_pcurves(face))); };
  if (closes())
    return true;
  std::vector<TopoDS_Edge> seams;
  for (TopExp_Explorer e(face, TopAbs_EDGE); e.More(); e.Next()) {
    const TopoDS_Edge edge = TopoDS::Edge(e.Current());
    double first = 0, last = 0;
    if (!BRep_Tool::IsClosed(edge, face) || BRep_Tool::CurveOnSurface(edge, face, first, last).IsNull())
      continue;
    bool seen = false;
    for (const auto &s : seams)
      seen |= s.IsSame(edge);
    if (!seen)
      seams.push_back(edge);
  }
  if (seams.empty() || seams.size() > 3)
    return false;
  const unsigned options = 1u << seams.size();
  for (unsigned mask = 1; mask < options; ++mask) {
    for (std::size_t k = 0; k < seams.size(); ++k)
      if (mask & (1u << k))
        swap_seam(seams[k], face);
    if (closes())
      return true;
    for (std::size_t k = 0; k < seams.size(); ++k)
      if (mask & (1u << k))
        swap_seam(seams[k], face);
  }
  return false;
}
inline TopoDS_Face s1_candidate(const TopoDS_Face &face) {
  const TopoDS_Shape base = face.Oriented(TopAbs_FORWARD);
  TopoDS_Shape copy = base.EmptyCopied();
  BRep_Builder builder;
  for (TopoDS_Iterator it(base); it.More(); it.Next())
    builder.Add(copy, it.Value().Reversed());
  return TopoDS::Face(copy.Oriented(TopAbs::Reverse(face.Orientation())));
}
inline int bound_contradicts(const TopoDS_Face &raw_face) {
  try {
    const TopoDS_Face face = TopoDS::Face(with_pcurves(raw_face));
    if (wires_close_2d(face)) {
      const TopAbs_State state = BRepTopAdaptor_FClass2d(face, Precision::PConfusion()).PerformInfinitePoint();
      return state == TopAbs_IN ? 1 : state == TopAbs_OUT ? 0 : -1;
    }
    // The candidate on a copy of its own: pairing its seams changes its edges.
    const TopoDS_Face candidate = TopoDS::Face(BRepBuilderAPI_Copy(s1_candidate(face)).Shape());
    if (!pair_seams(candidate))
      return -1;
    const TopAbs_State state = BRepTopAdaptor_FClass2d(TopoDS::Face(with_pcurves(candidate)), Precision::PConfusion()).PerformInfinitePoint();
    return state == TopAbs_OUT ? 1 : -1;
  } catch (const Standard_Failure &) {
    return -1;
  }
}

// A definition with a corrected or repaired side, verified before it is
// published (dispatcher ruling, B2-reader-export review round 1): on a
// pcurve-completed copy, BRepCheck valid, and every shell's signed volume of
// the right sign (the largest by magnitude, the outer shell, positive; every
// other, a void, negative). Empty when it passes, else what failed.
// The cap (review round 3 ruling): a pcurve the copy added may deviate from
// its edge's 3D curve by at most the definition's usable declared uncertainty
// (or, where none is declared, the STEP source adapter's source residual
// grade); a larger deviation is a fault of the translated geometry (an edge
// off its face's surface), not of the projection, and fails verification by
// name rather than being covered by a raised tolerance.
inline std::string verify_solid(const TopoDS_Shape &solid, Completion *report = nullptr,
                                double cap_mm = std::numeric_limits<double>::infinity()) {
  try {
    Completion own;
    Completion &completion = report ? *report : own;
    const TopoDS_Shape checked = with_pcurves(solid, &completion);
    if (completion.largest_added_deviation_mm > cap_mm) {
      std::ostringstream message;
      message << std::setprecision(6) << "an added pcurve deviates " << completion.largest_added_deviation_mm
              << " mm from its edge's 3D curve, above the cap " << cap_mm << " mm";
      return message.str();
    }
    if (!BRepCheck_Analyzer(checked).IsValid())
      return "BRepCheck finds it invalid";
    std::vector<double> volumes;
    for (TopExp_Explorer shell(checked, TopAbs_SHELL); shell.More(); shell.Next()) {
      GProp_GProps properties;
      BRepGProp::VolumeProperties(shell.Current(), properties);
      volumes.push_back(properties.Mass());
    }
    if (volumes.empty())
      return "it has no shell";
    std::size_t outer = 0;
    for (std::size_t k = 1; k < volumes.size(); ++k)
      if (std::abs(volumes[k]) > std::abs(volumes[outer]))
        outer = k;
    for (std::size_t k = 0; k < volumes.size(); ++k)
      if (k == outer ? !(volumes[k] > 0) : !(volumes[k] < 0))
        return k == outer ? "its outer shell's signed volume is not positive"
                          : "a void shell's signed volume is not negative";
    return {};
  } catch (const Standard_Failure &failure) {
    return std::string("the check failed: ") + failure.what();
  }
}

// One original face, the pieces OCCT's default read made of it, each with its
// use in the processed solid. The raw use is one side of one surface, so its
// side is decided over every piece by parity: into the material on all, or
// outward on all, else not decidable (the raw face is not one-sided against
// the material). Against that, OCCT's outward normal on every piece:
// - raw inward, OCCT opposite on every piece: the processing repaired the file;
// - raw outward, OCCT equal on every piece: flag and surface reversed together;
// - raw outward, OCCT opposite on a piece: it turned a correct face;
// - raw inward, OCCT equal on a piece: it left that piece inward;
// - otherwise not decidable.
inline std::string face_verdict(const std::vector<std::pair<TopoDS_Face, TopAbs_Orientation>> &pieces,
                                const Handle(Geom_Surface) &raw_surface, double used_sign,
                                const TopoDS_Shape &solid) {
  int inward = 0, outward = 0, opposite = 0, equal = 0;
  for (const auto &[piece, in_solid] : pieces) {
    Sample s;
    try {
      s = sample(piece, raw_surface, used_sign);
    } catch (const Standard_Failure &failure) {
      return failed_near_face(failure);
    }
    if (!s.sampled)
      return "undecidable: no interior point with a defined normal on its exported face";
    if (!s.near)
      return "undecidable: its exported face leaves the raw surface beyond its tolerance";
    gp_Vec occt = s.occt_surface_normal;
    if (in_solid == TopAbs_REVERSED)
      occt.Reverse();
    const double dot = occt.Dot(s.raw_normal);
    if (dot > 1 - 1e-9)
      ++equal;
    else if (dot < -(1 - 1e-9))
      ++opposite;
    else
      return "undecidable: the outward normals of the exported face and the raw use are neither "
             "equal nor opposite";
    const std::string side = raw_use_side(piece, raw_surface, used_sign, solid);
    if (side == "into_material")
      ++inward;
    else if (side == "outward")
      ++outward;
    else
      return side;
  }
  const int n = int(pieces.size());
  if (inward && outward)
    return "undecidable: the raw use points into the material on some of OCCT's pieces of the face "
           "and out of it on others";
  if (inward == n)
    return opposite == n ? repaired : left_inward;
  return equal == n ? "reversed_together" : turned_correct;
}
} // namespace brepgpu::reference_orientation
#endif
