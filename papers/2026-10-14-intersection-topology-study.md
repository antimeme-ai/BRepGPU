# Analytic plane/cylinder intersection beachhead

2026-10-14. Research and proposed scope only. No intersection operation is
delivered by this study. Read the current roadmap, body/pair guides and owned
body records, plus pinned OCCT source and primary literature. No dependency,
product code, native execution, performance run or new agent was introduced.

## Complete useful scope

Recommend a **finite closed-face intersection** between one supported planar
face and one cylindrical wall face from the canonical immutable snapshots,
in their common world frame. Current translations and positive uniform scales
keep wall axes vertical and planar normals aligned with x, y or z. Admit both
operand orders. Include circles, clipped arcs, generator segments, tangent
generators and isolated trim contacts; report unsupported geometry separately.
This directly supplies section curves and spatial/UV events for later planar
arrangements. CPU exact construction is a valid first placement; GPU placement
and cost need their own measured delivery.

An infinite supporting-plane result is not the complete finite-face answer.
Every returned component must satisfy both carriers' finite domains and both
supported trims. Current planar trims are rectangles, horizontal disks and
horizontal rectangles with one circular hole; walls have finite axial bounds
and a periodic seam. These closed regions include boundary points. General
trims, tilted planes/ellipses, cylinder/cylinder intersections, poles and
two-dimensional surface coincidence are outside this first family.

Prefer face-pair requests over independent chart-pair requests. Pair masks
supply conservative survivors, not intersection components. A face-pair can
be safely skipped only when every required chart pair is covered and clear.
All four quarter charts of a wall belong to one face; their joins cannot
create four fabricated spatial edges or four duplicated circles.

## Owned formulas and clipping inference

Let a wall have world center `(cx,cy)`, radius `R=scale/4`, and height interval
`[oz-scale/2, oz+scale/2]`. Interpret submitted binary64 frames exactly.

* A horizontal plane `z=h` intersects the infinite wall in one circle. First
  check the wall's closed height interval exactly. Use four existing rational
  quadrants: `C(s)=((1-s²)/(1+s²), 2s/(1+s²))`, `0<=s<=1`, rotate by the quadrant,
  then apply center/radius. These formulas match the owned homogeneous controls;
  they are not equal-angle parameterization.
* For `x=p`, set `d=p-cx`, `D=R²-d²`. Exact `D<0` means empty; `D=0` gives one
  tangent generator; `D>0` gives two generators at `y=cy±sqrt(D)`. The y-plane
  case exchanges coordinates. Clip each against wall height and the planar
  rectangle. Constants may be algebraic even though the inputs are dyadic.

On a horizontal circle, rectangular inequalities become degree-at-most-two
polynomials per quadrant after multiplying by positive `1+s²`. A disk/hole
test simplifies before expansion: for trim center `b`,
`|c-b|²+R²-r²+2R(c-b)·C(s)`. Thus its boundary is also at most quadratic per
quadrant. Disk membership uses `<=0`; being outside a hole uses `>=0`. These
are our deductions for this restricted family, not a theorem attributed to
OCCT or general NURBS research.

Enumerate all boundary roots, quarter endpoints and axial events. Prove root
order/equality, then classify every open parameter interval and each event
itself. Exact sign is constant between consecutive roots. Repeated roots can
produce an isolated accepted contact with no accepted neighboring interval;
an identically zero boundary polynomial means a curve lies on an existing trim.
Neither case can be dropped by midpoint-only interval classification.

Own a small rational/algebraic event representation: rational values or
normalized low-degree polynomials with isolating intervals and root identity.
Root equality/order needs exact arithmetic or a supported isolation certificate,
not coordinate epsilon welding. If bounded work cannot establish order,
membership or correspondence, return an unresolved complete operation, never
an empty or silently partial answer. Arbitrarily close trim events are possible
with admitted binary64 frame inputs; a fixed sample grid does not establish
completeness. Fixed geometry degree makes this narrower than general SSI, but
does not justify rounding algebraic endpoints into exact trim decisions.

## Spatial curves, both UV maps and identity

One component owns a spatial curve and an ordered set of representation pieces.
Each piece references both exact source snapshots and their complete typed
body/revision/face/chart lineage, its common curve-parameter interval, and both
UV maps. Chart pieces are representation subdivisions; connected components
are geometric outputs. Retain provenance for trim endpoints, original edges
and boundary-coincident pieces before any future topology construction.

For a circle piece in quadrant `k`, use canonical parameter `a=k+s`. Its wall
atlas map is `(a,v0)` and its native chart map `(s,v0)`. The planar UV map is the
exact inverse of that plane's owned affine frame/basis applied to the same
spatial curve. For a generator, use a common axial parameter: the wall map has
constant atlas `a`, and planar UV is affine in height. In a rotated quadrant,
invert the rational circle with `s=y/(R+x)` in quadrant-local coordinates;
this positive denominator avoids treating angle/2π as the owned native UV.

The analytic maps satisfy `world(Sleft(UVleft(t))) = C(t) =
world(Sright(UVright(t)))` identically. Rounded evaluations are approximate
outputs and need separately stated correspondence bounds. The delivered
plate's circular UV trim is generally non-dyadic: a rounded sample may classify
inside or outside even when its exact symbolic event lies on the boundary.
Do not relabel that sampled UV as exact boundary. Cardinal chart joins can
have two valid occurrences; local derivatives are one-sided and differently
scaled, while the spatial normal agrees.

A full circle is one closed component with a common designated endpoint; its
wall lifts begin at `a=0` and end at `a=4`. Do not normalize both lifts to zero
and discard seam incidence. An arc crossing this seam remains one component
with split chart pieces. A generator exactly on the seam has both wall-chart
occurrences; it remains one spatial curve. Preserve point contacts as points,
not fabricated zero-length edges. Mark tangent generators and boundary-coincident
curves explicitly; they are not ordinary crossing arcs for Boolean classification.

Spatial event keys use operand-order-normalized typed face sources plus analytic
branch/component/event provenance. Per-source occurrence keys add chart/lift
and orientation. Dense output slots, page flat indices and addresses are not
event identity. A future edited body receives its own namespace/history;
this beachhead emits immutable intersection records without changing inputs.

## Direct qualification and actual value

The independent unit should prove expected component counts and connectivity,
analytic equations, exact trim event membership and both original-control
surface images. Include a full circle, a seam-crossing clipped arc, two
disconnected retained arcs, two generator segments, one tangent generator,
empty height/radial separation, a boundary-only point, corner/multiple-boundary
events, coincident circle/hole rim, and events arbitrarily close to chart joins.
Swap operands and vary independent frames; compare semantic components and
per-side provenance rather than output addresses. Explicitly attack incomplete
pair-page coverage and duplicate chart-generated circles.

A runnable caller should turn the measured broad-phase survivors into supported
face-pair sections, display unsupported pairs, and export exact event/curve/UV
records plus approximate samples. CPU/native jet checks of sampled images are
useful numerical checks, but all-component topology comes from the independent
analytic/event oracle. A plot of curves or agreement with OCCT cannot replace
that oracle. Report CPU event/clipping work, GPU evaluation if used, transfer,
setup and complete caller cost; no general SSI or modeling speedup is implied.

## Primary sources and actual entry points

1. **Patrikalakis, Maekawa and Cho, _Shape Interrogation_, Hyperbook (2009).**
   [Intersection overview](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node56.html)
   motivates section curves and B-rep boundary evaluation. Its
   [classification](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node57.html)
   separates solution features/dimensions and numerical reliability. The
   [all-branch starting-point discussion](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node105.html)
   distinguishes boundary, turning and singular starts. These are foundations,
   not a finite-precision completeness certificate for our implementation.
2. **OCCT pinned source `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`.**
   Read `IntAna_QuadQuadGeo.cxx:543`, plane/cylinder `Perform`, under
   `src/ModelingData/TKGeomBase/IntAna`. It emits empty/one-or-two lines/circle/
   ellipse and uses tolerances to classify tangency and nearly parallel/circular
   cases. This tolerance behavior differs from our proposed exact supported
   classification. [Pinned source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/IntAna/IntAna_QuadQuadGeo.cxx).
3. **OCCT analytic line and domain records.** Read `IntPatch_ImpImpIntersection.cxx:3157`,
   `IntPCy`, and `IntPatch_GLine.hxx`/`IntPatch_Point.hxx` in
   `src/ModelingAlgorithms/TKGeomAlgo/IntPatch`. The plane/cylinder dispatch
   treats a one-generator answer as tangency; intersection points carry both
   surface parameters and domain arc/vertex membership. These support retaining
   geometry plus occurrences and transitions, rather than only sampled points.
   [Pinned dispatch](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKGeomAlgo/IntPatch/IntPatch_ImpImpIntersection.cxx).
4. **OCCT spatial/pcurve construction.** Read `GeomInt_IntSS_1.cxx:275`
   `MakeCurve`, `:1172` `BuildPCurves`, and `:1333` `TrimILineOnSurfBoundaries`.
   It constructs trimmed spatial curves and per-surface pcurves, intersects
   parameter curves with rectangular surface bounds, then sorts parameters.
   Rectangular clipping is not a proof of arbitrary face-hole clipping.
   [Pinned source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKGeomAlgo/GeomInt/GeomInt_IntSS_1.cxx).
5. **OCCT parameter and seam contracts.** Read `ElSLib.cxx:1547/1558`:
   plane coordinates are affine, but cylinder U is normalized `atan2`, not our
   rational atlas. Read `BRep_Tool.cxx:852/939/950`, `BRep_TEdge.hxx` and
   `BRep_CurveOnClosedSurface.hxx`: shared edges retain curve representations,
   same-parameter/range flags and two seam pcurves. The
   [primary seam API](https://occt3d.com/dev/doc/refman/html/class_b_rep___curve_on_closed_surface.html)
   explicitly represents two pcurves on one closed surface. These are read-only
   design references; no OCCT code is adopted or copied.

## Round-two limits

The intact pinned OCCT archive and clean extraction already exist; no new
acquisition is needed for this analytic family. The older
[topology report](2026-10-13-gpu-topology.md) and
[shopping list](gpu-topology-shopping-list.md) still distinguish actual full
texts from slides/abstracts for Yang/Jia, interval-algebraic topology and paired
UV fitting. This pass did not acquire those missing full papers or verify their
general SSI theorems. Their assumptions remain research prerequisites for a
later wider family, not hidden dependencies of the exact axis-aligned beachhead.
