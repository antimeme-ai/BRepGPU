# Adaptive mesher: scoped algorithm design and plan review

Read the revised actual `docs/ADAPTIVE_MESHER_DESIGN.md` and selected
first-checkpoint procedures in `docs/ADAPTIVE_MESHER_PLAN.md`, following the
algorithm report and peer geometry/runtime interfaces. Scoped design and
dependency-plan ACCEPT. This accepts a new cached-subdivision primary mesher,
not the previous full-face CDT/refinement default.

The earlier concrete omissions now have selected resolutions: cheap
curvature/control variation plus twist scores, fixed U ties and a longest-active
original-root-width split every fourth failing split; PointPool authority and
bounded common CPU/P100 construction; exact shared side events with a no-center
zipper; monotone demotion of point-touch packet rims; strict interior
cache-derived region anchors installed in bulk before one region rebuild per
horizon; a single common denominator on both coordinate axes; and an explicit
adaptive lineage operation. The actual SourceStore entry bypasses mandatory old
eager GeometryStore/PreparedRun construction. These resolve the architectural
blockers rather than postponing them as implementation menus.

Split and shorter-diagonal choices are heuristic policy, not geometric oracles.
The selected zipper score is common RN64 squared distance, saturating overflow
to infinity, then stable PointKey tie; neither exact transcendental escalation
nor Rational XYZ imports are required to choose a valid convex ear. Original
whole-cell bounds and exact topology remain the direct correctness mechanisms.

The first checkpoint must emit real conforming triangles, including unbalanced
interfaces and original spans, and run actual CPU/P100 geometry/frontier work.
It cannot imply complete trimmed B-rep or watch delivery. Finite product units
still need their own shared schema/direct RED/code review, as the plan requires;
this is scoped acceptance of those concrete procedures, not a claim that code,
performance, alpha qualification or universal termination already exists.
