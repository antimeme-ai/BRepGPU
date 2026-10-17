# Constructing a shared-vertex seam cap

2026-10-17. DESIGN for review; no new product code or measurements. This is one
finite extension of the cached selected-face constructor. Geometry owns the
source map and bound in `adaptive-seam-wedge-geometry-plan.md`; runtime owns the
same-session transitions in `adaptive-cap-runtime-unit.md`. This paper selects
the intervening topology, physical point authority and progress procedure.

## Sources actually read

* NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05,
  `SmTess.cpp::sms_FillGapUVWithPolyEdge`1440–1595 and
  `SmTess::MakePolygon`1610–1760: one physical pole vertex has two UV polygon
  vertices and a coverage edge between them. The comments distinguish missing
  singular segments from ordinary source edgeuses. We preserve that distinction;
  their tolerance repair and polygon-edge resampling do not prove our source map.
* The same source, `TessellateEdge`3009–3066, `Phase1SetupBrep`11118–11866,
  `Phase2CreateFacePolygons`12023–12131: shared edge schedules precede per-face
  polygons; cached inside packets and remaining polygon regions coexist. We keep
  the physical owner and regular cached remainder, rather than rebuilding a
  complete face through a second tessellator.
* OCCT8.0.1 b8f597c677811d1f9f4d8a97f5ae2825c0353a42,
  `BRepMesh_EdgeDiscret.cxx::Tessellate3d`225–279 and `Tessellate2d`283 onward:
  original endpoints anchor one physical polygon and each pcurve consumes its
  parameter schedule through an occurrence provider. This supports shared cut
  endpoints with separate chart sectors, not private surface points on an edge.
* Filip–Magedson–Markot1986 §2 Theorems2/4,
  DOI10.1016/0167-8396(86)90005-1, as acquired in the geometry study: vector
  interpolation inequalities. Applying them to the finite-to-ideal cap map is
  our derivation, already isolated in the numerical plan; it is not a topology
  theorem from NVIDIA or OCCT.
* Owned actual `adaptive_traces.cpp::prepare_trace_sites`,
  `adaptive_regions.cpp::prepare_face_domain`, `face_metric`, directed region
  CDT and `geometry_poles.cpp::continue_pole_cut`190 onward. Current joint UV
  merging, equal-UV domain admission and ordinary-only facet tags explain the
  precise missing operation. The old cap controller is reference only.

## Selected admission and occurrence construction

Admit one outer loop with exactly three original uses: the SAME nondegenerate
meridian edge twice in opposite source traversal, and one regular opposite edge
whose endpoint is the other original vertex. The pole joint is the SAME original
vertex, not a coordinate coincidence or a declared-degenerate source edge. The
first profile is reconstructed selected UV on a supported analytic cone or one
sphere pole. General authoritative curved maps, multiple wedges, holes meeting
the cap and a fully mapped face remain named unsupported.

Before ordinary joint merging, produce a `SeamWedge` from complete source
loop/use/edge/vertex incidence and source-cap admission. Preserve its incoming
and outgoing shared-site occurrences, exact original t/mapped s, original raw
guides/lifts and both UV values. Other joints retain ordinary merging. Repeated
physical identity and current actual Q agree bitwise, while chart sites differ.

For each reconstructed meridian, propose constant U from its guide-derived
REGULAR-side endpoint and retain the guide-derived V sequence. A raw guide may
have genuinely variable U controls. This proposal does not assert raw-guide
identity. Require exact strict V order, correct source traversal, distinct
angular branches, retained source-envelope containment and the existing original
V/Ce-to-S node guards and whole selected trace-to-SAME-Q chord bounds. There is
no new inverse, tolerance split or Q movement. A failing node guard is unresolved;
a justified whole-interval failure requests the actual physical interval.

The wedge coverage connector is the straight chosen UV segment between the two
pole occurrences. It has an explicit construction provenance and NO original
CoedgeSlot or physical BoundaryKey. It must not be exported as an invented source
use. The complete original three-use inventory remains retained separately.

## Exact partition and emitted topology

Let the two selected meridians be U=u0,u1, with finite pole values v0,v1 and
current cut values c0,c1. Canonicalize u0<u1 with an explicit permutation back
to original incoming/outgoing occurrences; original source order is retained.
The source-only `admit_cap_map` proves a fixed nonzero
sign of g−vf and a common ideal regular sheet, where

    u(xi)=u0+xi*(u1−u0), vf(xi)=(1−xi)*v0+xi*v1,
    g(xi)=(1−xi)*c0+xi*c1.

The cap is the closed trapezoid between vf and g; the ordinary construction loop
replaces the connector and consumed pole-to-cut meridian intervals by the SAME
ordered cut chain, with opposite incidence. Exact source/loop order checks prove
the ordinary remainder lies on the other side. In particular the complete
opposite-edge selected chain must be strictly on the regular side of the cut;
its segments cannot cross the trapezoid. This is a finite exact graph admission,
not an inference from three-use incidence alone. The initial profile requires a
strict cut leaving nonempty regular remainder. No rectangular grid alignment is
required for the slanted cut. Cached packets and directed band CDT construct
only the ordinary remainder; no topology proxy enters that CDT or its metric.

Partition the angular range at every prepared source U-panel boundary and by
the numerical source-derived base cells of width at most its conservative pi/2
lower bound, then uniform dyadic child levels.
Within each base cell use the SAME exact midpoint subdivisions; cut sites remain
dyadic when both cell endpoints are dyadic. Mandatory prepared-U panel intersections can have
rational xi and rational V; retain them exactly through the EXISTING absolute
PointPool path and common-denominator CDT, without silently rounding onto g.
Subsequent children use exact midpoints. Each piece emits one facet with
source roles (POLE,LEFT,RIGHT), and the producer's positive-chart permutation.
Original face orientation is applied once by the final physical/display adapter.
The cap has n facets for n cut intervals. Interior fan spokes occur twice with
opposite incidence, each cut interval occurs once on cap and once on remainder,
and the two meridian spokes retain the actual source interval provenance. These
are local constructive incidence facts, checked by the SAME output-chain oracle.

The source map is S(u(xi),(1−rho)*vf(xi)+rho*g(xi)); actual L uses the captured
original Qpole and two cut Qs. The finite connector is collapsed by an explicitly
bounded approximation. It is not declared exactly singular. Source-map roles
and finite connector refs therefore remain in every mapped facet descriptor;
ConstructionSite.uv is only a locator for a mapped pole, never ordinary facet
coverage or a source normal. Geometry's whole-cap band bound covers the full V
hull of ideal pole, finite connector and BOTH cut endpoints.

## Physical points and retained cut progress

The radial cut endpoints MUST be the current pole-adjacent strict interior sample
on the repeated physical meridian chain, the same original t and physical Q for
both sectors. If none exists, request the actual pole-adjacent BoundaryKey. The
opposite original edge is never replaced with a private angular cut. Interior
angular cut sites alone are face-owned exact UV queries to the SAME Surface
PointPool; cap and ordinary remainder reference the SAME returned site/Q/revision.

Every actual failed cap horizon chooses angular refinement when its removable
angular upper dominates, radial otherwise (angular wins exact ties). An angular
step increments local level/domain revision and preserves the physical boundary
generation. A radial step requests the pole-adjacent physical interval, retaining
the previous two cut gaps. A replacement counts as contracted only when geometry
proves BOTH `4*new_gap <= 3*old_gap` against the symbolic pole. False means not
proved, not invalid source. The continuation persists across boundary epochs and
pooled owner retirement, using exact t/use/sector/key locators, never old Q/rows.

If only one side has contracted, publish no intermediate face and do not reuse
an old cut that would skip newly inserted meridian samples. Return the existing
needs_boundary continuation and request the next canonical pole-adjacent split.
When BOTH qualify, commit the current pole-adjacent cut. This also keeps each
cap meridian boundary equal to one current physical master interval. The old
control is the comparison baseline, not permission to publish stale topology.

An actual failure with no admitted positional/profile progress is typed
unresolved under cumulative work/depth/piece caps; an upper bound is never a
proof of geometric infeasibility. No fresh quality query inherits old fine cuts.
Local replacements retain the initial exact fixed face metric; retirement
continuation carries that small factor, not a cache or unfinished mesh.

## Output and display obligation

The result retains all original source-site bindings, two wedge sectors sharing
one physical pole, consumed source intervals, cut incidence, piece source map,
actual point revisions and full face/domain keys. Mapped facets use an explicit
generator. Pole occurrence bindings are explicitly tagged with wedge/sector:
their raw chosen UVs remain distinct although the actual pole site has one
locator UV. Ordinary bindings keep exact UV equality. Their normal descriptor
names analytic kind, sheet and piece angular
sector; sphere uses its admitted pole limit and cone uses the piece's canonical
sector limit. It is a cosmetic source-normal convention, not an error proof.
The existing adapter currently refuses mapped facets and must continue doing so
until root connects that descriptor. No proxy masquerades as ordinary UV.

The finite direct oracle is specified in `docs/adaptive-cap-construction-plan.md`.
This paper makes no pass, imported-model or performance claim.
