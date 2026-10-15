# Cut-result mesh design: independent semantics review

2026-10-15. **ACCEPTED; no blocking findings.** Reviewed
`docs/CUT_MESH_DESIGN.md` against actual `boolean.py`, `meshing.py`, both
independent oracle modules and the source-grounded
[semantics study](2026-10-15-cut-mesh-semantics-study.md). No implementation or
execution occurred in this review.

The design retains the exact CutResult and its actual CutSolid/history envelope,
rejects bare geometry and preserves separate output and original-operand identity.
Shared immutable mesh records introduce no canonical type widening. Immediate
mesh source IDs belong to the result body/revision; typed unchanged history
retains section/arrangement provenance rather than flattening it.

Construction reads actual output edge/coedge maps and ordered loops, samples
physical edges before occurrences and faces, retains a single ring anchor at
t0/1 and preserves separate lifted seam U0/4 occurrences. Plane inversions use
each actual patch frame. Wall corners keep the original cutter frame and raw
V1/4..3/4; neither radius1/4 plate substitution nor wall V0/1 normalization can
satisfy the specified independent checks.

The geometric bound explicitly covers analytic/chord trim discrepancy, shared
binary64 displacement and complete shell distance, without claiming straight
cap triangles stay exactly outside the analytic hole. The independent qualifier
checks positive full integer boundary-chain equality in every plane and affine
wall-chord strip, so it does not mistake point sampling, Euler or area for domain
coverage. Exact and rounded domains are separate, with simple convex nested
rings, matching top/bottom XY, strict heights and preserved common planes to
establish the bounded extruded shell's embedding.

The planned fault classes target actual correspondence/coverage/embedding claims,
including lost wall-frame and raw-V information. Performance scopes distinguish
already-built-result meshing from Boolean creation, consumption, qualification
and export. No GPU/native speedup, universal triangulator, external-suite green
status or new dependency is implied.

Implementation review should verify these promises against actual buffers and
maps, especially rounded face-domain chain coverage and per-patch world frames;
this design acceptance is not a substitute for that review.
