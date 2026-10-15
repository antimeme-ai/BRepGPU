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
