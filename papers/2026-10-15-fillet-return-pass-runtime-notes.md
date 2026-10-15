# Fillet return-pass notes: preserve the complete operation

2026-10-15. Notes from the accepted design, actual narrow sampler/caller and
observed direct CPU1/CPU24/P1000/P1001 tests. No optimization or wider family is
implemented here; offset remains the next breadth unit. Final caller performance
interpretation awaits its own saved campaign/report review.

Current exact construction is an exceptional useful geometric case: one complete
vertical edge on the actual canonical box, a constant dyadic radius, two orthogonal
planes and two planar limiting caps. One rational quarter-cylinder and six
unchanged plane carriers suffice. Full-length termination supplies exact cap arcs;
it does not solve a general rolling-ball endpoint, branch or corner transition.
Side contacts are G1 tangent while cap joins remain sharp. Four corner IDs and
radius[1/16,1/2] do not imply arbitrary geometry or multi-edge support.

For multiple parallel vertical corners, require a whole selected-edge set with
precise source identity, deterministic set ordering and complete overlap/contact
admission. Shared side trims and cap words must be assembled together rather
than applying the current one-edge constructor repeatedly to pretend a canonical
source still exists. Even disjoint blends change the source history/origin counts;
interacting radii can consume straight spans and create topology changes. Constant
radius on general face pairs additionally needs actual offset-surface/spine
construction, branch selection, support/contact maps and trim intersection.
Filleting tangent edge chains is a distinct contour selection/propagation contract.

Partial source edges need new limiting geometry and watertight end closures;
three-edge intersections need actual rolling-ball/corner patches, orientations,
trimming and complete shared vertex links. Variable radius needs a stated law,
parameter origin/units, radius positivity/collision conditions, real surface class
and bounded approximation if finite rational patches no longer represent it
exactly. Generalization may require non-dyadic coefficient construction and
certified conversion budgets; current exact coefficients cannot justify silent
rounding or fitted geometry. OCCT's acquired selected-contour/partial-result
interfaces and NVIDIA's mutator are references, not selected owned algorithms or
dependencies. Failed or partial blends remain explicit failure, not a publishable
closed result.

Keep actual original carrier bits, Frame, raw UV and source identity whenever a
surface survives. The current two changed side endpoints involve2r/3, showing
why exact trims must remain separate from native binary64 query admission. Keep
actual copied query bits and independent rounded-query truth; do not snap future
queries to source contacts. Source_t(spine) gives original axial correspondence,
not spatial coincidence. An arc sourceUV map is not an affine old-line subrange.
Multiple owners/curved supports need explicit typed map owner/domain and exact or
budgeted map semantics, not a generic unlabelled pointer. Physical tangent edges,
coedge occurrences and cap sharp joins remain separately identifiable even when
coordinates/normals agree. Extend complete forward/reverse ancestry and all map/
contact consumers as a whole, not just record counts.

Direct tests establish existing wrapper compatibility and ownership: actual
nonaxis points/normals, both incident native charts at representable contacts,
all four quarter orientations, unsnapped nonbinary joins, explicit nonadmitted
rows and real metadata/status rejection. Late host failure closes all3 actual
native children and same borrowed Engine/result retry succeeds. This does not
establish retained GPU result storage or GPU construction. The present child
lifetime is one complete synchronous call. Later residency should name who owns
source revisions, typed topology/patch/query storage, device context and output
buffers; reject stale-source references and preserve current cleanup/error
publication semantics. Measure residency as a separate admitted repeated-use
workload while retaining cold operation costs.

GPU construction, a native application/optional thin binding and Python
orchestration are deferred architecture questions. Current request admission,
exact material/history/contact reads, frame conversion and row/normal scatter
are real caller work. Profile the complete operation before relocating a stage;
remove no work from a reported useful wall merely by changing language or caching.
Preserve actual geometry/history/CSR output and independent qualification as the
comparison contract. Source-factory and prepared-source walls, full child wall,
setup/allocation/upload/evaluate/download/checks/normal/output/cleanup phases and
unprofiled kernel timing availability must remain explicit. Reused-device timing
cannot replace cold setup, and nested phase medians cannot be added to construct
a claimed complete cost. Three fixed-size radius/edge/Frame cases remain small
features; neither patch replication nor scale-only variation is a larger-model
benchmark.

Reference comparisons keep exact same-world source construction, copy before
mutating, selection on the copy, scalar original probes and discarded stale copy
handles. Preserve all unchanged geometric thresholds/outcomes before nonzero
failure exit; public volume accuracy clamps are not certified error enclosures.
The earlier revolution discrepancy remains open. Future reference additions must
select API/property/tolerance scope before observing outcomes. Required external
community alpha suites and unresolved adapters remain non-green irrespective of
local test success or attractive geometry.
