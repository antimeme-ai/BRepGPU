# Adaptive mesher design/plan: runtime review

2026-10-16. Read actual docs/ADAPTIVE_MESHER_DESIGN.md and
ADAPTIVE_MESHER_PLAN.md, all three focused reports, existing owned runtime/
interval/point code and pinned reference source. Review only; no product code,
new compute, test or benchmark execution. This is not implementation acceptance.

## Verdict

The dependency plan and first checkpoint are accepted in scope: original source
patches must reach bounded, conforming actual triangles on CPU/P100. Merely
allocating a cache, evaluating points or rendering an existing mesh would not
complete it. Later checkpoints deliver the shared original boundary, trims,
finite maps/quotients and complete native operation. Existing source/assembly/
publication machinery remains useful without imposing the old constructor.

The current draft still says the split/layout/point/reclamation and shared
numeric policy remain decisions. It therefore needs the following concrete
selections integrated before executable unit plans/product code. These are
constructive closures from the reports, not another research stage.

## Required closures and actual resolutions

1. **One numeric algorithm on both backends.** Select
   `rn64_anchor32_p128_v1` in the runtime report: explicit RN+identical IEEE
   padding, exact-converted owned P128 anchor/Chebyshev intervals and uniform
   remainder, bounded integer quadrant locator with whole reduced-domain checks,
   fixed Horner/reconstruction and source expression order. Fixed128/192/256
   and original exact are unresolved paths. No native libm or device RD/RU
   substitution. Geometry's original nonorthogonal-frame formulas and error
   terms remain one source of truth. Source delta remains original Ce/V versus
   original S, not new Q. Departures belong to actual epsilon exactly once.
   This resolves the report's earlier fixed-integer-primary disagreement.

2. **Point authority versus private retry.** A semantic chart sample key is
   source/carrier/chart + exact original parameter + policy/profile generation.
   Common knot endpoints have a canonical incident root and fixed evaluation
   order. Evaluate from that ORIGINAL root; restriction paths are not point
   authorities. A failed numeric floor proposes a shared-authority promotion,
   installed from the complete sorted request set at a generation barrier.
   Invalidate dependent bounds and incident face selections. No first-leaf
   winner, face-private epsilon overwrite or completion-driven promotion.
   Original vertices keep literal source XYZ; physical-edge authority remains
   distinct from sharing a carrier evaluation. Cache reuse never welds keys.

3. **Packed identities and stale work.** Node identity is original root plus
   complete split-axis/side path, not its box or reusable slot. Each submitted
   epoch/ambiguity result retains source generation, cache owner/generation,
   query generation, numerical profile, root/path and operation ordinal.
   Validate the complete key before applying results. Existing triangle-slot
   generation fields must not be reused with a different meaning. Logical
   identity survives compaction; physical slot revisions protect device access.
   A checked inline path + spill pool supports configured depth; capacity/depth
   exhaustion refuses explicitly rather than truncating or changing geometry.

4. **Bounded bulk mutation and actual device work.** Select degree/family
   bucketed segmented frontier kernels with resident memory. Produce canonical
   split/accept/ambiguous commands, count/scan required nodes/controls/points/
   output, reserve all next-epoch capacity BEFORE applying any mutation, then
   write/compact. Capacity failure is a whole operation outcome, never which
   racing owners fit. Charge two frontiers, scan/sort scratch, root/control/
   point/output and compact exact-child reservations. Source/kernel/table
   leases survive cancellation and DMA; reuse the actual preregistered poisoned
   owner rule on failed final drain. No perleaf allocation or runtime creation.
   Analytic point/curvature and rational restriction/residual must actually run
   on P100 for the advertised first-checkpoint families; compact ambiguity is
   permitted, all-analytic host processing is not adaptive P100 delivery.

5. **History-independent selection.** Separate retained cache from query-active
   leaves, edge intervals and construction sites. New quality queries start at
   canonical roots/profile and select the same decisions as an empty cache;
   cached descendants or stronger point tiers are hits only when demanded.
   Source-only restrictions/summary checkpoints may survive quality/assembly
   changes, trim classification cannot survive its boundary generation.
   Reconstruct evicted control nets along the SAME split path and operation
   order. Eviction must not silently substitute a direct restriction with
   different interval widths. Repeated identical, fine→coarse→fine and cache
   eviction replay comparisons directly check this contract within the normal
   cache test; no second validation framework is needed.

6. **Refinement changes geometry; uncertainty changes precision.** Adopt the
   algorithm report's binary kd midpoint policy, exact active-axis proof and
   finite fairness rule, with canonical ties. Retain e(source−actualB), actual
   twist and numerical-floor parts separately. Tighten a conservative bound on
   the SAME emitted facet before geometrically refining it. Evaluation floors
   request a numeric stage/rebuild, not unbounded subdivision. A genuine bad
   boundary facet installs a strictly interior source-cache anchor and rebuilds
   its connected region in bulk, preserving physical source chains; splitting
   only a source-bound cache cannot improve unchanged L. Warm cache selection
   must recompute these query operations canonically as well.

7. **First checkpoint and literal coordinate scope.** Include actual analytic
   and positive-rational families, shared C0 root-side schedules, hanging sides
   and emitted positive facets; compare original formulas independently as well
   as CPU/P100 semantic bytes. Current binary64 source endpoints are dyadic,
   root-relative dyadic subdivision stays dyadic; retain arbitrary-precision
   exact coordinates when binary64 interior points no longer exist. Future
   genuinely nondyadic source sites require an explicit rational predicate
   bridge before admission, not a rounded integer adapter or global LCM claim.
   Keep first-patch output distinct from complete imported-component success.

8. **Reference warm scope is actual API behavior.** Pinned SmTess.cpp3069–3121
   creates/builds a fresh face tessellation cache;12134–12157 removes/deletes it.
   Retained imported object/context is not established retained tessellation
   tree reuse. Label reference warm-import/cold-tessellation when that is what
   the API supports. Compare the complete original source and quality; separate
   source adaptation/import, cache work, mesh output, display and exit wall.

## Nonblocking choices

A packed LBVH is appropriate for already-known trim/contact primitives; Karras
radix construction does not discover adaptive source leaves. Scan-frontier slab
pools are a concrete initial fit for monotone refinement. CBT rank/select and
block-local donation remain serious measured alternatives for churn/tail costs;
neither mesh-shader support nor pointer-tree imitation is required on P100.
No GPU victory is presumed and no numerical SLA is introduced.

## Integrated reread closure

Root integrated the concrete packed layout, common RN64 profile, exact coordinate
bridge, side construction, heuristic split/fairness, full task identity and bulk
capacity/lifetime rules into the actual design and explicit first-checkpoint
plan. Scoped design/dependency-plan ACCEPT after root's accepted normative
clarification: keep BASE and promoted root/control/bound/point records distinct;
new queries execute the identical base→escalation policy, independent of cached
tighter data. Promotion creates a new actual record generation and invalidates
all dependent users. No exact transcendental escalation for a heuristic axis tie.
These clarifications are also incorporated into the runtime implementation unit
plan; product code still follows that unit's review and genuine RED.
