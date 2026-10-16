# Cached occurrence-trace construction unit

Implementation plan, 2026-10-16; scope accepted shared-curves checkpoint. Own
adaptive_traces.hpp/.cpp and native_adaptive_traces.cpp. Runtime owns frozen
adaptive_curves_types.hpp rows and physical forest/session; geometry owns source
curve/carrier caches and numerical kernels. No alternate evaluator, old
GeometryStore, PatchSession, mutable forest callback or private boundary split.

## Grounding and concrete operation

Read papers/native-adaptive-curves-construction.md and scoped bibliography for
exact pinned source functions and full-paper scopes. Each mechanism maps to:

| Mechanism | Source/formula and selected difference |
|---|---|
| One physical chain, every occurrence | NVIDIA SmTess::TessellateEdge3009–3066 and OCCT EdgeDiscret::Tessellate2d286–339. Borrow the settled forest snapshot; preserve all coedges and source traversal. |
| Literal t, original map and lift | Existing values.hpp/geometry_boundary.cpp contracts: literal edge range already follows endpoint IDs. Identity and original PL maps only; no second SameSense reversal. Analytic periodic lift retained as source descriptor. |
| Common face joints | Original loop-adjacent occurrence sector is identity; source-authoritative endpoints must actually agree. Seam u0/stored2pi may be distinct sectors despite equal physical key. No coordinate weld. |
| Finite recovery | Exact plane Gram dual; checked cylinder iso/frame identity; otherwise original-guide candidate plus ORIGINAL guard. OCCT predictor/projection is a proposal, not a whole interval theorem. No promised generic inverse. |
| Node guard | ORIGINAL V/Ce versus original carrier at chosen dyadic UV; endpoint V–Ce and alias obligations separately included. Actual Q is excluded. |
| Whole selected trace | Geometry direct original-span affine UV restriction degree<=6 and N-W*SAME actual L, or Filip1986 vector h²M2/8 with actual endpoint departures. No peruse 2D mesher. |
| Avoid speed-only splits | Exact homogeneous collinearity + scalar monotone image; derivative numerator nonnegative with positive endpoint progress permits stationary t³ endpoints. Retain explicit scalar map. |
| Authoritative flattening | Original 2D pcurve/chord residual plus source carrier derivative transport; Balazs2004 composition caution and Marussig/Hughes competing representations. This output is not full face-domain homeomorphism. |
| Complete publication | One current-generation immutable result or canonical physical BoundaryKey requests. Runtime unions complete horizons before sole-owner mutation. |

## Finite staged seam and ownership

TraceFaceRequest retains SourceStore, CurveSourceCache, SourceCache,
DefinitionBoundarySnapshot, face, query/face/boundary generations, exact allowance
table/refs and output exact-table keys. No owner-local Rational escapes.
A face operation owns original ordered loops/use inventory and common junctions.
A trace belongs to one use but joints are settled for both adjacent uses first.

The stages are prepare_trace_sites, complete_trace_sites,
prepare_trace_intervals and complete_trace_intervals, as in the research report.
Runtime batches original curve/pcurve samples across face requests, then carrier
samples and numerical interval work. Work/results retain immutable exact tables,
source caches and full identities. Complete functions accept only their own
ordered result pool; reject wrong source, size, generation, nonfinite interval
and stale phase before reading rows. No callback or scheduler is embedded here.

Site preparation maps each literal t to original s before sampling. Kind1 is
s=t, kind2 uses exact original affine pieces with deterministic knot tie; kind3
is unsupported. Closed original endpoints stay separate occurrences. Endpoint
source obligations use original source endpoints, never rounded Q. Select guide
UV (with supported symbolic period lift), or finite plane/cylinder recovery;
choose deterministic dyadic coordinates, preserve original branch, then bound
original-source residuals. Common joints use one chosen site on both adjacent
uses; authoritatives require source equality, not numerical enclosure overlap.
Unsupported shifts/NURBS periodicity/unknown inverse fail the whole face result.

Interval preparation retains actual BoundaryKey and original traversal; virtual
original-span crossings are bound pieces, not physical nodes. Physical owner
accepts Ce→actualQ. Each use independently accepts carrier-selected-trace→SAMEQ;
Ce→Sselected is a DIFFERENT retained bound, never a nodal-cap test. Store parameter
or checked image correspondence and its actual payload. Authoritative policy
requires raw UV component +physical domain displacement bounds; absence is not0.
A geometric failure returns canonical source interval requests; source/precision
floors return typed failure. No fixed minimum parameter forces success.

Standalone frozen rows live in adaptive_curves_types.hpp to avoid session/trace
include cycles. Geometry numerical PODs remain CUDA17 in adaptive_curve_types.h.
Runtime owns integration, numerical reuse and device submission. Construction
never installs physical Q or changes a constructed revision.

## Direct RED → delivery

1. Write missing-operation RED against actual source mapping/trace interface on
   the fleet source checkout. Direct unit target consumes original SourceStore
   fixtures; no fake SourceCache or injected successful product results.
2. Implement exact map/lift/junction inventory and finite staged site construction
   with common geometry services. Extend REDs before completing interval/image/
   authoritative paths. Publish actual headers promptly for runtime integration.
3. Direct tests attack distinct claims: descending literal t with false SameSense,
   PL-map knot ties/coverage, unknown map3, same physical node separate seam UVs,
   authoritative joint disagreement, original guard disagreement despite Q lying
   on carrier, stationary nonlinear line image and whole interval bulge. Root's
   single actual-output reader independently checks complete source geometry;
   this fixture must not duplicate that whole checker.
4. Meaningful ordinary face/use operation must finish through real source caches
   and common sample/trace bounds, not merely expose map utilities. Failures and
   requests retain source/current-generation identity; stale/foreign/capacity
   cases attack ownership/resource faults.
5. Fleet-only strict release, scoped ASan/UBSan/leaks, actual-source tidy/format
   and independent source review. CPU/P100 whole-operation parity and direct
   source oracle are root's integration checks, not a second local campaign.
   Stop when affected checks pass. No watch rerun, extra dependency or tuning.

Root CMake targets requested: brepgpu_mesher_adaptive_traces from
native/mesher/adaptive_traces.cpp; brepgpu_native_adaptive_traces from
tests/native_adaptive_traces.cpp, SOURCE fixture argument. Dependencies are
source/exact_store, SourceCache and CurveSourceCache/affine-trace geometry,
shared frozen curve rows; no prior eager geometry library dependency.
