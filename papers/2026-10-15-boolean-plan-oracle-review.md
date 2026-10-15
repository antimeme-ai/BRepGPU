# Through-bore plan: independent qualification review

2026-10-15. Reviewed `docs/BOOLEAN_PLAN.md` against the accepted design and actual
existing body sampling/oracle records. No product implementation or execution.
The operation/history, sampling consumer and independent qualification are
coherent whole units. One numerical acceptance detail needs to be made explicit.

## BPO1 — State observable sample position/normal limits

The proposed CutSamples exposes actual positions and oriented unit normals,
without retained raw first jets. The current choice of "normalized grades or
propagated budgets" does not yet specify a check available from those rows.
Use explicit per-component absolute limits of 2^-36 for source-world positions
and 2^-30 for oriented unit normals, evaluated independently at the actual copied
float UV using each original source patch/frame. These are the established body
consumer's direct observed-output acceptance limits. They must not be described
as following automatically from a 2^-30 native partial grade: normalizing a cross
product changes the error conditioning. Retaining and qualifying raw jets would
be a different stronger consumer contract, unnecessary for this bounded unit.

Original box source formulas and exact rational quarter-circle unit normals
supply direct truth, including the wall's reversed face sense. Check every
admitted row; rejected/invalid rows must retain native_status−1 and all NaN
fields. The consumer independently checks actual native info, descriptors,
slots/UV bits and lineage before scattering, as planned.

## Accepted qualification obligations

The full seven-domain analytic source correspondence and complete section/region
selection support actual material/embedding truth. Whole map identities and
trim domains precede sampled consumer checks. Original per-patch frames are
explicit: a common output frame must never replace the cutter patch frame.
All85 source dispositions/all81 output origins are reconstructed by actual
source roles and mappings, not inferred from totals. Same-face seam use identity,
closed-edge endpoint occurrences, complete loop words, cyclic vertex links,
one shell and the annular-hole Euler correction remain required.

Rational cap-rim UV is particularly useful: exact u=5/6,v=1/2 is on the hole
boundary, while its actual copied binary64 u lies strictly on one side. The
sampler must classify that copied float, while the exact CutSolid API must honor
the original rational. This tests that boundary snapping or mismatched source
parameters do not hide inside a positional tolerance.

Topology qualification resolves complete typed result identities and incidence
under input-slot permutations; snapshots=() does not imply canonical geometry
qualification. The separate result oracle supplies geometry/history truth.
Late constructor and consumer exceptions have different rollback boundaries:
no new CutResult on failed construction, no partial CutSamples on consumer
failure, and original inputs/prior results remain usable. A private real
construction boundary is sufficient for bounded failure injection; no production
fault flags/native rebuild are needed.

Source/candidate ownership, actual reference limitations and separate caller/
qualification/export costs remain explicit. Existing affected compatibility and
bounded direct fault witnesses fit the changed unit. No required external alpha
corpus is waived or broadened, and no GPU Boolean construction claim is made.

## Final disposition

Re-read the revised plan: BPO1 is integrated as direct component limits at the
actual copied binary64 UV, using each original source Patch frame. The text
explicitly separates observed normal limits from native partial-error grades.
Accepted for implementation, with no remaining blocker.
