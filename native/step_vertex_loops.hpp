#ifndef BREPGPU_STEP_VERTEX_LOOPS_HPP
#define BREPGPU_STEP_VERTEX_LOOPS_HPP
// Vertex-loop face bounds (AP242 advanced_face WR6, WR7) as OCCT translates
// them, shared by the STEP source adapter and the reference source exporter so
// both name the same refusals.
//
// OCCT's rule (StepToTopoDS_TranslateFace.cxx:547-662) reads the surface it
// built for the face (StepToGeom::MakeSurface, then ConvertToPeriodic for a
// B-spline), so a rectangular_trimmed_surface stays a
// Geom_RectangularTrimmedSurface and is never its basis:
//   - the sole bound on a Geom_SphericalSurface, Geom_BSplineSurface or
//     Geom_SurfaceOfRevolution: the natural bounds of that surface;
//   - else on a Geom_ToroidalSurface or a Geom_Plane: dropped;
//   - else StepToTopoDS_TranslateVertexLoop: one degenerated edge at the pole
//     vertex, or no wire when that translation fails.
// A loop is admitted only when all of these hold, tested in this order:
//   1. OCCT's result is the one its rule states for that surface;
//   2. the solid declares a usable length uncertainty (G6 UR-7), the only
//      threshold the tests below use (never a defaulted one);
//   3. a sole bound lies on a closed surface (ISO 10303-42 face_surface: "the
//      underlying surface shall be closed");
//   4. the pole lies on the surface within that uncertainty;
//   5. unless OCCT bound natural bounds, the pole is a singular point of it.
// Each refusal names its reason; a refused loop refuses its face, not the file.
#include <BRep_Tool.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_Conic.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Plane.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_Surface.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Precision.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace brepgpu::step_vertex_loops {
enum class Rule { natural_bounds, dropped_torus, dropped_plane, degenerated_edge };
// Codes of the source_face_refused record's "reason" field.
enum class Reason : std::uint32_t {
  admitted = 0,
  dropped = 1,             // OCCT drops the bound (plane, torus, or no wire)
  translation_differs = 2, // OCCT's result is not what its rule states
  not_evaluable = 3,       // no usable length uncertainty (UR-7)
  not_closed = 4,          // a sole bound on a surface that is not closed
  off_surface = 5,         // the pole is not on the surface
  not_singular = 6,        // the pole is not a singular point of the surface
  // B2 .37.36, not a vertex-loop reason: the face's same_sense contradicts its
  // own outer bound and the closed shell's edge uses do not decide its side.
  orientation_undecided = 7,
};
inline const char *name(Reason r) {
  switch (r) {
  case Reason::admitted: return "admitted";
  case Reason::dropped: return "dropped_by_occt";
  case Reason::translation_differs: return "translation_differs";
  case Reason::not_evaluable: return "not_evaluable";
  case Reason::not_closed: return "sole_bound_not_closed";
  case Reason::off_surface: return "pole_off_surface";
  case Reason::not_singular: return "pole_not_singular";
  case Reason::orientation_undecided: return "orientation_undecided";
  }
  return "unknown";
}
inline constexpr Reason reasons[] = {Reason::dropped,      Reason::translation_differs,
                                     Reason::not_evaluable, Reason::not_closed,
                                     Reason::off_surface,   Reason::not_singular,
                                     Reason::orientation_undecided};

// TranslateFace.cxx:614-649, on the surface OCCT built.
inline Rule rule(const Handle(Geom_Surface) &built, int bounds) {
  const bool closed_kind = built->IsKind(STANDARD_TYPE(Geom_SphericalSurface)) ||
                           built->IsKind(STANDARD_TYPE(Geom_BSplineSurface)) ||
                           built->IsKind(STANDARD_TYPE(Geom_SurfaceOfRevolution));
  if (closed_kind && bounds == 1)
    return Rule::natural_bounds;
  if (built->IsKind(STANDARD_TYPE(Geom_ToroidalSurface)))
    return Rule::dropped_torus;
  if (built->IsKind(STANDARD_TYPE(Geom_Plane)))
    return Rule::dropped_plane;
  return Rule::degenerated_edge;
}

// Empty when the surface is closed: every parametric side either meets the
// opposite side or collapses to a point, within eps. A sphere and a torus are
// closed. For a B-spline the proof is on the control net (a boundary curve of
// a clamped side is the B-spline of that side's pole row, a convex combination
// of its poles for positive weights): two rows with equal weights whose poles
// pair within eps give boundary curves within eps; one row whose poles lie
// within eps of its first gives a side within eps of a point. For a revolution
// the sides are the circles its generatrix ends sweep: they meet when the ends
// meet within eps, and collapse when both ends lie on the axis within eps.
// Anything else is not proven closed, and a rectangular_trimmed_surface is a
// bounded surface.
inline std::string open_surface(const Handle(Geom_Surface) &s, double eps) {
  if (s->IsKind(STANDARD_TYPE(Geom_SphericalSurface)) ||
      s->IsKind(STANDARD_TYPE(Geom_ToroidalSurface)))
    return {};
  if (auto b = Handle(Geom_BSplineSurface)::DownCast(s); !b.IsNull()) {
    const int nu = b->NbUPoles(), nv = b->NbVPoles();
    auto pole = [&](bool u_row, int k, int i) { return u_row ? b->Pole(k, i) : b->Pole(i, k); };
    auto weight = [&](bool u_row, int k, int i) { return u_row ? b->Weight(k, i) : b->Weight(i, k); };
    auto collapsed = [&](bool u_row, int k) {
      const int n = u_row ? nv : nu;
      for (int i = 1; i <= n; ++i)
        if (!(pole(u_row, k, i).Distance(pole(u_row, k, 1)) <= eps))
          return false;
      return true;
    };
    auto meets = [&](bool u_row, int a, int z) {
      const int n = u_row ? nv : nu;
      for (int i = 1; i <= n; ++i)
        if (weight(u_row, a, i) != weight(u_row, z, i) ||
            !(pole(u_row, a, i).Distance(pole(u_row, z, i)) <= eps))
          return false;
      return true;
    };
    for (const bool u : {true, false}) {
      const bool periodic = u ? b->IsUPeriodic() : b->IsVPeriodic();
      if (periodic)
        continue;
      const int degree = u ? b->UDegree() : b->VDegree();
      const int first = u ? b->UMultiplicity(1) : b->VMultiplicity(1);
      const int last = u ? b->UMultiplicity(b->NbUKnots()) : b->VMultiplicity(b->NbVKnots());
      const char *axis = u ? "u" : "v";
      if (first != degree + 1 || last != degree + 1)
        return std::string("the B-spline surface is not clamped in ") + axis +
               ", so its closure is not proven from its control net";
      const int n = u ? nu : nv;
      if (!meets(u, 1, n) && !(collapsed(u, 1) && collapsed(u, n)))
        return std::string("the B-spline surface has an open side in ") + axis +
               " (its first and last pole rows neither pair nor collapse within the context "
               "uncertainty)";
    }
    return {};
  }
  if (auto r = Handle(Geom_SurfaceOfRevolution)::DownCast(s); !r.IsNull()) {
    // Only an untrimmed periodic conic or B-spline generatrix is closed by its
    // kind; any other (a trimmed arc, an offset curve) is closed only when its
    // ends meet or both lie on the axis, never by IsPeriodic, which a trimmed
    // or offset curve answers from its basis.
    const auto curve = r->BasisCurve();
    if (curve->IsPeriodic() &&
        (curve->IsKind(STANDARD_TYPE(Geom_Conic)) || curve->IsKind(STANDARD_TYPE(Geom_BSplineCurve))))
      return {};
    const double f = curve->FirstParameter(), l = curve->LastParameter();
    if (Precision::IsInfinite(f) || Precision::IsInfinite(l))
      return "the surface of revolution sweeps an unbounded generatrix";
    const gp_Lin axis(r->Axis());
    const auto a = curve->Value(f), z = curve->Value(l);
    if (a.Distance(z) <= eps || (axis.Distance(a) <= eps && axis.Distance(z) <= eps))
      return {};
    return "the surface of revolution has an open side (a generatrix end is off the axis "
           "and the ends do not meet, within the context uncertainty)";
  }
  if (s->IsKind(STANDARD_TYPE(Geom_RectangularTrimmedSurface)))
    return "a rectangular_trimmed_surface is a bounded surface";
  return std::string("a ") + s->DynamicType()->Name() + " is not a closed surface";
}

struct Assessment {
  Rule rule = Rule::degenerated_edge;
  TopoDS_Wire wire; // OCCT's wire for this loop, claimed from `remaining`
  Reason reason = Reason::admitted;
  std::string translation, refusal;
};

// One vertex loop of a face. `remaining` holds the translated face's wires no
// edge loop and no earlier vertex loop owns; the matching one is claimed.
// `point` is the raw pole in millimetres, `pole` OCCT's translated vertex (null
// when OCCT translated none), `uncertainty` the solid's usable value, if any.
inline Assessment assess(const TopoDS_Face &face, std::vector<TopoDS_Wire> &remaining,
                         int bounds, const TopoDS_Vertex &pole, const gp_Pnt &point,
                         std::optional<double> uncertainty) {
  TopLoc_Location location;
  const Handle(Geom_Surface) &built = BRep_Tool::Surface(face, location);
  const Handle(Geom_Surface) geometry =
      location.IsIdentity()
          ? built
          : Handle(Geom_Surface)::DownCast(built->Transformed(location.Transformation()));
  Assessment a;
  a.rule = rule(built, bounds);
  std::vector<std::size_t> matches;
  if (!pole.IsNull())
    for (std::size_t wi = 0; wi < remaining.size(); ++wi) {
      std::vector<TopoDS_Edge> edges;
      for (TopExp_Explorer e(remaining[wi], TopAbs_EDGE); e.More(); e.Next())
        edges.push_back(TopoDS::Edge(e.Current()));
      if (edges.size() != 1 || !BRep_Tool::Degenerated(edges[0]))
        continue;
      TopoDS_Vertex v0, v1;
      TopExp::Vertices(edges[0], v0, v1);
      if (v0.IsSame(pole) && v1.IsSame(pole))
        matches.push_back(wi);
    }
  const std::string surface = built->DynamicType()->Name();
  auto refuse = [&](Reason reason, const std::string &why) {
    a.reason = reason;
    a.refusal = why;
  };
  switch (a.rule) {
  case Rule::natural_bounds:
    if (matches.empty() && remaining.size() == 1) {
      a.wire = remaining[0];
      remaining.clear();
      a.translation = "natural bounds of the " + surface;
    } else {
      a.translation = "not the natural bounds OCCT's rule states for a sole bound on a " + surface;
      refuse(Reason::translation_differs, "OCCT did not translate the bound as its rule states");
    }
    break;
  case Rule::dropped_torus:
  case Rule::dropped_plane:
    a.translation = std::string("dropped by OCCT (") +
                    (a.rule == Rule::dropped_plane ? "plane" : "torus") + ")";
    if (matches.empty())
      refuse(Reason::dropped, std::string("OCCT drops a vertex_loop bound on a ") +
                                  (a.rule == Rule::dropped_plane ? "plane" : "torus"));
    else
      refuse(Reason::translation_differs, "OCCT made a pole wire its rule says it drops");
    break;
  case Rule::degenerated_edge:
    if (matches.size() == 1) {
      a.wire = remaining[matches[0]];
      remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(matches[0]));
      a.translation = "one degenerated edge at the pole vertex on a " + surface;
    } else if (matches.empty()) {
      a.translation = "no wire: OCCT's vertex-loop translation failed on a " + surface;
      refuse(Reason::dropped, "OCCT's vertex-loop translation gave no wire, so the bound is lost");
    } else {
      a.translation = "ambiguous: several degenerated-edge wires at the pole vertex";
      refuse(Reason::translation_differs, "OCCT did not translate the bound as its rule states");
    }
    break;
  }
  if (a.reason != Reason::admitted)
    return a;
  if (!uncertainty) {
    refuse(Reason::not_evaluable,
           "not evaluable: the solid declares no usable length uncertainty (G6 UR-7), so the "
           "pole is not tested against its surface");
    return a;
  }
  const double eps = *uncertainty;
  if (bounds == 1)
    if (auto open = open_surface(geometry, eps); !open.empty()) {
      refuse(Reason::not_closed, "a sole vertex_loop bound needs a closed surface (ISO 10303-42 "
                                 "face_surface); " + open);
      return a;
    }
  ShapeAnalysis_Surface analysis(geometry);
  analysis.ValueOfUV(point, eps);
  if (!(analysis.Gap() <= eps))
    refuse(Reason::off_surface, "the pole vertex is not on the face surface within the context "
                                "uncertainty");
  else if (a.rule != Rule::natural_bounds && !analysis.IsDegenerated(point, eps))
    refuse(Reason::not_singular, "the pole vertex is not a singular point of the face surface "
                                 "within the context uncertainty");
  return a;
}
} // namespace brepgpu::step_vertex_loops
#endif
