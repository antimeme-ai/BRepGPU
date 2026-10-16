# Native cached shared-curve runtime unit

Finite implementation plan, 2026-10-16. Read the accepted architecture,
docs/adaptive-shared-curves-unit.md and papers/native-adaptive-curves-runtime.md.
This lane owns adaptive_curves.hpp/.cpp, fresh-query physical forest adapter,
narrow boundary.hpp/.cpp owner extensions and runtime-specific fixtures. Geometry
owns original curve caches/kernels; construction owns adaptive_traces.hpp/.cpp;
root owns versioned CurveCudaAPI/device implementation, caller and independent
whole actual-output reader. No product code before concrete interface review.

## Selected public closure

Use AdaptiveBoundarySession, CurveBoundaryQuery, FaceCurveAllowance, CurveLimits,
CurveStats and SharedCurveSnapshotData rows exactly as the runtime report.
Mandatory definition defaults cover wire edges; optional face caps restrict them.
Snapshot retains the actual SourceStore, full DefinitionBoundarySnapshot,
per-physical-interval parameter/image chord bounds (including wires),
constructed-revision pool, exact values, all original use rows and selected
trace sites/intervals. Every UseCurveInterval retains its actual BoundaryKey.
No old GeometryStore, PreparedRun, hidden Python callback or fake NativeMesh.

UseCurveInterval correspondence is finite and backed by its actual payload:
affine_parameter needs no image map; monotone-line image retains covector/scalar
intervals and traversal. CylinderAffineIdentity requires geometry's exact harmonic frame/height and
literal selected u=t plus its finite record; general ArcOrder is not emitted.
Each use interval distinguishes carrier_trace_to_actual_chord_upper from
Ce-to-selected-trace upper; the latter is never the nodal guard.
Authoritative policy requires original UV component and physical displacement
bounds. This is not a complete onto face-domain map.

## Implementation steps and exact dependencies

1. Geometry headers: agree original/prepared CurveRoot slots, immutable root
   lease, coefficient stride, common RN point/chord/paired-split procedures,
   precise fallback and finite image-map payload. Trace producer returns immutable
   results retaining ExactTable leases or canonical requests; mutable Rational
   work stays in its producer context, never crosses jobs. No mutable forest/Budget access.
2. Add missing session-operation and direct physical-owner RED fixtures before
   bodies. Literal original-source shared edge/wire cases must exercise actual
   begin/step/result, not injected geometry results. Parent whole-output reader
   independently owns original rational/analytic bounds and complete incidence.
3. Implement source-qualified persistent numerical cache keyed by original
   CurveSlot/span + exact prepared interval/full binary path, with immutable BASE
   and promoted point/bound tiers. Bound cache includes actual endpoint bits and
   demanded point revisions. Never key persistent entries by forest sample slot.
4. On begin validate immutable source identity/definition and complete allowances,
   create a NEW DefinitionBoundaryForest, reset selected query tiers, preserve
   original spatial-knot and map-t initial cuts/vertex aliases. An invalid item
   fails the query; no partial result or optional incident-use subset.
5. Drive pending physical evaluations through source-cache sample jobs, stable
   deduplicated across all edges. Supply one physical Q per sample and every
   use sees that key/value. Original vertices remain fixed. Evaluate spatial
   chord bounds before peruse trace horizons, trying checked line-image bounds
   before speed-only parameter interpolation triggers refinement.
6. Bind NativeScheduler jobs to private contexts and disjoint pre-sized result
   ranges. Use deterministic work-weighted prefix partitions (curve degree²,
   analytic constant-cost rows) rather than one host worker per edge. step polls
   and folds the full horizon; no nested waits or captured shared Budget.
7. P100 uses root's same resident module lifetime, mode=curve and separate
   versioned operation table. Upload original/prepared 1D banks once. Midpoint
   restriction emits paired children from one O(p²) convex pyramid. Source sample,
   chord bounds and frontier mark/count/scan run on device. Common coefficient
   order/RN padding matches CPU. Config caps are checked limits, not mandatory
   whole-cap allocation. All allocation/transfer bytes are counted.
8. For each current generation collect physical-bound failures, all peruse trace
   requests and point promotions. Every active face reaches a canonical request
   yield or settles; unchanged settled rows supply empty arrivals. Stable union
   of owner and use requests precedes count/reserve. Wires use owner-only split
   request, never an invented face. Cache restriction alone is not publication.
9. Stage guarded promotions and use existing complete_evaluations as the ONE
   atomic split+Q-revision installation. Validate expected key and constructed
   revision, current source/query/epoch, finite Q and nonvertex eligibility.
   Increment each affected incident-face generation once. Old snapshots remain
   immutable. Rerun only those incident uses; retain unaffected source/trace work.
10. Finish only with all original edges/uses represented, no pending split or
    promotion and all required chord/trace/nodal guards accepted. Freeze source
    order/keys, original use traversal, exact t/mapped-s/chosenUV and actual bounds.
    release_result starts another fresh selection over persistent numerical data.
    cancel/close drains every worker/ticket; reuse preregistered poisoned device
    owner retention on failed final drain, no duplicate lifetime implementation.

## Direct checks and completion

Use one real-source runtime fixture for lifecycle/ownership, capacity and actual
worker/device work. Independent parent reader checks geometry/source/inventory
from the real operation: shared many-use edge, wire, weighted/C0/nonclamped p32,
nonlinear monotone line image, descending t/map, source endpoint discrepancy and
literal cylinder seam. Coarse/fine/repeat/fresh histories compare semantic rows,
not timings. Exercise stale key/revision and foreign source rejection directly.

Release CPU1/parallel/both P100 closure plus focused host address/undefined/leak
and thread instrumentation, actual-source tidy/format and peer review cover the
new mechanisms. Root owns targeted device protocol/memory checks. Reuse the one
actual-output reader; do not write another geometry certifier. Stop once affected
checks pass. Report cold process versus retained source/cache query costs,
per-stage work, transfer/memory and refused/unsupported cases. Small GPU batches
may lose. The next dependency is trimmed boundary-region construction, not a
further runtime tuning/certification campaign.
