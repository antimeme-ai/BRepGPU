# Tolerant source incidence and UV recovery

Root reference reading during the complete OM10 source pass,2026-10-15.
Read the retained OCCT8.0.1 source at pinned commit
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`.

[ShapeAnalysis_Surface::ValueOfUV](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKShHealing/ShapeAnalysis/ShapeAnalysis_Surface.cxx#L1245)
uses analytic inverse parameters for planes, cylinders, cones, spheres and tori.
Periodic angles are lifted near the selected parameter-domain center.
NextValueOfUV uses a previous UV as a local Newton seed for spline-like carriers,
handles crossed C0 knots separately and checks the resulting spatial gap before
accepting or considering alternate isoline recovery. These are useful patterns
for owned, source-linked recovery with explicit branch and residual checks;
they do not establish a globally unique spline projection branch.

[ShapeAnalysis_Wire::CheckConnected](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKShHealing/ShapeAnalysis/ShapeAnalysis_Wire.cxx#L693)
first recognizes identical topological vertices. Its later distance-based
reconnection paths concern different identities. OM10 recovery concerns the
first case: the original topology already declares a common vertex. Preserve
that identity and examine its incident geometric residuals explicitly.

[BRepCheck_Edge::InContext](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKTopAlgo/BRepCheck/BRepCheck_Edge.cxx#L411)
validates the spatial curve against its pcurve-on-surface representation using
the edge tolerance, even when SameParameter is set. A true flag supplies a
parameterization convention, not measured agreement for our chosen budget.
Every actual owned boundary sample still needs its direct source residual check.

Actual source face165500 gives the concrete distinction: an original circle
and seam line reference the same source vertex, but the line endpoint differs
from it by about6.27e-7mm. On a radius0.045mm cylinder this becomes an angular
gap of1.394e-5rad, much larger than a floating UV closure grade. Source-vertex-
driven analytic inverse recovery is therefore under reviewed design. Raw
pcurve/curve parameters remain available, and the unchanged selected spatial
residual remains the acceptance criterion. This is a source-incidence issue,
not permission to merge nearby vertices or discard a trim.

Independent investigation of face165590/coedge112915 found the foreign pcurve
is a degree-one UV line. At the failing original parameter, exact rational
original-knot curve evaluation gives a curve-on-foreign-UV residual of
0.002472328592687374mm; radial projection onto the original radius2mm cylinder
has residual0.0000268980830712478mm. A1,001-point scan observes maximum original
curve/cylinder radial residual0.0001409593mm, versus0.0066102067mm for the foreign
same-parameter UV line. These finite probes diagnose the case; they are not a
universal curve-incidence bound.

Bounded minimization over the whole foreign line parameter interval still misses
the selected1μm residual at the failing point. Scalar reparameterization cannot
repair that trim approximation. Reviewed continuation therefore studies an
owned original-cylinder inverse at canonical source edge/vertex samples, with
retained raw foreign UV and a separate recovered-trim chord bound. The original
spatial curve, source identities and selected residual budget stay intact.
The input's declared0.01mm uncertainty was not used to widen that budget.
