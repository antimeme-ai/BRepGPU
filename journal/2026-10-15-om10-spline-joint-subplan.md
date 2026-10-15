# Spline joint/source boundary unit

Circle resampling is complete and separately checkpointable. New actual face
169982 reproduces a nonclosing UV joint under current code. Source/oracle study
shows original active spline curve intervals really disagree at their shared
declared source vertex; both raw UVs accurately track their own source endpoints.
The first existing UV candidate has smaller original-carrier-to-vertex residual.
Written design: [source study](../papers/2026-10-15-om10-spline-joint-recovery.md).

No production changes before root and oracle review. Proposed bounded sequence:

1. Meaningful fleet reds on an independently authored clamped spline rectangle
   with a split original boundary/off-curve shared vertex, plus displaced original
   LINE whose ideal/UV/cell errors vanish but canonical-end chord misses source.
   Preserve complete original intervals/provenance; no new permanent qualifier.
2. Precompute finite original-joint UV choices per DIRECTED original occurrence,
   source identity/domain/cell/natural-V/source-limit guards, and typed metadata.
   Use only existing endpoint candidates, deterministic residual choice; no Newton
   inverse or arbitrary weld. Apply shared endpoint overrides before sampling.
3. Add maximum endpoint UV perturbation to incident raw trim chord bounds. Add
   measured original C-to-canonical endpoint departure to ideal world chord bound
   in the explicit per-edge context, with finite/selected-source/fixed-floor gates.
   Actual51,742endpoint inventory max.000211863mm is below.000625allocation.
4. Direct greens attack original-vertex identity, wrong knot/chart/domain and
   impossible fixed perturbation. Keep independent source formulas/full coedge
   sample chains/existing exact-domain checks. Run existing affected integration
   fixtures once, actual169982, then adversarial code review.
5. Fresh complete retained original-source/assembly/mesh/native qualification is
   still required. No target raster or whole-watch pass is implied by this unit.

Selected limits remain.005mesh/.001source, max_refinement20 and16384edge samples.
No source binary/native layout, dependency, source vertex/curve or topology edit.

Oracle design review ACCEPT. Actual global derivative metric(.6437623,10)
times endpoint UV displacement gives.000111923386mm, within unchanged.000625
allocation. Two genuine fleet reds recorded before production: authored spline
plane splitjoint fails UV closure(.45,0)→(.45030000000000003,0), and an original
LINE atz=.0008 with unchanged sourceplane/canonical verticesz0 is wrongly
accepted above the chord allocation while remaining below.001source limit.
Run2methods/.016s, exit1; ignored log
context/om10-spline-joint-world-endpoint-red-v1.txt. Product file remains unchanged
while root final design admission is pending.

Root design and plan review ACCEPT after reading the complete source diagnosis,
finite candidate guards, endpoint bounds and direct red fixtures. The actual
global metric correction fits the unchanged allocation; no local derivative
mechanism is needed. The displaced-LINE red directly distinguishes the missing
physical chord term from surface/cell error. Implementation may now proceed
within this unit, followed by independent code review and actual-face checks.

Root final plan admission ACCEPT after confirming the global metric. Production
is complete: choices are per directed original endpoint and used in both shared
interval bounds and final occurrences, with raw pre-choice joint records. The
spatial world chord adds cached actual endpoint departure; an impossible fixed
departure fails before refinement. Chosen-only carrier-to-P admission aligns the
written contract; both source C-to-P/raw S-to-C checks remain. Source-column
metrics exclude the new endpoint-candidate family.

Final35 direct methods GREEN1.487s, including fixed trim-floor and original
knot-cell rejection. Independent two sensitivities plus existing five closed
geometry/native integrations GREEN0.665s. Both code reviews ACCEPT. Final actual
face169982 has62 vertices/60 cells in0.10245s, one declared joint, maximum edge
bound0.000556779mm, actual joint-cell bound0.000562711mm, canonical boundary
displacement0.000275294mm, candidate endpoint UV bound0.000111923mm. Product
SHA-256 e11168db3bdb1be4d8dca475daf52f08216c2e24b1a008b375b064189be01886
matches fleet. Product files stay unchanged during root's complete V3 qualification.

Root authorized stopping the old V7 diagnostic inventory rather than waiting
on two remaining old-code tasks: all9162 jobs had been scheduled, six workers
were idle and two still CPU-active. Thus9160 completed is inferred from worker
state, with95 recorded failures; two pending IDs could not be recovered from
its intentionally sparse log. Parent and all eight workers received TERM and
their PIDs are gone. Retain the original log and ignored stopped-status companion.
Failure classification:22 construction caps,60 source-LINE zeros,6 constructed/
mixed-interior zeros,4 spline endpoint overhangs,2 circular-boundary crossings,
and the1 spline joint now studied. This remains PARTIAL OLD-CODE diagnostics,
not source/mesh qualification or an assertion that all current faces pass.

V7 has no complete geometry counters or retained mesh. Its706 logged slow
successful isolated OLD meshes sum73,177,724 vertices/145,677,621 cells; quick
passes and failures are absent from those sums. Current Hessian/Lawson/shared
sampling differs substantially, so those old sums do not predict current whole
mesh memory or native admission. Complete current V3 must measure actual size
and cover every original face before any target rendering.
