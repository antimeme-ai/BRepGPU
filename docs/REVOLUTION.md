# Full profile revolution

`brepgpu.revolution.revolve(profile)` constructs a complete owned CPU solid from
the actual factory `PlanarProfile` supplied by `extrusion.make_profile`. Profiles
remain exact strict convex CCW /256 lattice polygons with3–32 vertices; all radial
X coordinates must additionally lie in[1/4,3/4]. Existing Y bounds/Frame apply.
Unsupported radial families raise UnsupportedRevolution; wrong source types
raise TypeError. No repair, source mutation, new native build or library is used.

The axis is source-local positive Y through its Frame origin. An original source
point(r,a,0) becomes(r*cos(theta),a,-r*sin(theta)); theta0 preserves the original
actual XY geometry. There are no full-turn caps. Axis contacts, partial turns,
general axes, curved or concave sources and holes remain unsupported.

Each source edge generates one complete swept face with four exact rational
degree2×1 quarter charts. Source radius/axial coordinates and rational weights
produce exact dyadic homogeneous controls; all original Frames remain. Raw face
coordinates(tau,v) span[0,4]×[0,1]; chartq uses normalized native(u,v), tau=q+u.
Quarter joins have matching positions/normals but unequal angular parameter
speeds, so raw first jets need not match. Existing native sampling is a separate
consumer, not GPU feature construction.

`RevolutionResult` retains profile,solid,history,timings. Factory-only immutable
`RevolvedSolid` owns profile/body_id/revision/frame, complete primitive tables,
patches and exact `Measure(0,pi_coefficient)` volume. The coefficient comes from
the original polygon radial first moment. No canonical shape/packed tag or
general result mesh is supplied.

Public readers are `classify(chart_slot,u,v)`, `classify_point(worldpoint)`,
`edge_point(one_based_edge_id,t)`, `coedge_uv(one_based_coedge_id,t)` and
`topology_input()`. Exact readers accept int/Fraction/finite binary64 excluding
bool. Trim slots are zero-based. Nonfinite/out-of-square trim queries return
INVALID; v0/v1 rings and globaltau0/4 seam occurrences are BOUNDARY. Internal
quarter joins are INTERIOR at interiorv. Material signs use exact rational
comparisons of signed radical supports, preserving nonsquare radial queries and
all supporting faces without a floating square root. Material returns the exact
submitted point, interior/boundary/exterior and every supporting face ID.

Rings occupy edge slots0..N−1 and use explicit circle_y(radius,axial) curves;
seams occupy slotsN..2N−1 and preserve source lines. Ring t∈[0,1] maps tau=4t,
including terminal endpointt1. Intrinsic coedge UV maps do not apply traversal
sign a second time. Each face's loop contains start ring+, seam at4+, end ring−,
seam at0−. Same-face seam occurrences retain distinct identity; quarter charts
introduce no physical edges. Every anchor has a four-germ cyclic link. The graph
has Nvertices/2Nedges/4Nuses/Nloops/Nfaces/4Ncharts and one genus-one shell.

History uses shared typed EntityKey and operation-owned Origin/HistoryRecord.
Every source vertex/edge/coedge is modified with1/1/2 same-kind images: original-
position anchor, seam line and its two distinct occurrences. Source loop/face/
chart are consumed as boundary images while retaining generated descendants.
All13N origins comprise4Nmodified and9Ngenerated entities, with precise original
contextual parents, roles, source_t/source_uv and periodic affine maps. Periodic
emits quarter-index tau rather than radians. Whole sourceface→solid provenance
is retained by result/profile/solid binding, outside the six boundary kinds.
History.from_input/origin reject foreign/nonkeys. Construction validates exact
domains, controls, ownership, ordered words, links and every source/history map
before atomic publication; late failure preserves profile and earlier results.

TopologyInput uses the existing ABI5 standalone typed representation with empty
snapshots: Nordinary opposed ring pairs and Nsame-face seam pairs. Existing
canonical body, Boolean, extrusion sampler and mesher guards remain scoped.
See the [design](REVOLUTION_DESIGN.md), [reviewed plan](REVOLUTION_PLAN.md) and
[qualification/cost report](../results/revolution-2026-10-15/REPORT.md). All 27
new methods pass across applicable CPU/both-P100 placements; complete costs and
optional foreign comparison outcomes are reported separately. The foreign
triangle volume comparison remains outside its declared limit. Required
external community alpha qualification remains incomplete.
Fillet and offset are the next independent breadth units.
