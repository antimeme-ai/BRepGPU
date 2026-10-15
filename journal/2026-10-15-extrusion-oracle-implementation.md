# Independent extrusion qualification delivered

Implemented `python/brepgpu/extrusion_oracles.py` and the eleven-method
`tests/test_extrusion.py` as one whole independent qualification unit.
Expected answers use actual original Fraction lattice endpoints, height and
Frame; no production feature/trim/validation/history helper supplies truth.

The model checks complete profile/output graph words and exact control bits,
whole affine edge/both-UV identities, all typed ownership and native lineage,
strict-support product-domain completeness, opposed edge uses, one shell and
cyclic vertex links. Counts/Euler/volume supplement that source geometry proof.
All six-kind history dispositions, every output origin and reciprocal parent
link are reconstructed independently, including reversed lower cap coedge IDs
and every intrinsic affine/axial map. Actual material, trim, edge and coedge
readers consume exact non-dyadic and copied-float requests.

Native qualification checks every scatter identity/query bit/label, finite
admitted positions and oriented normals, explicit invalid/rejected NaNs and
complete gather. Exact source positions have component limit2^-36; original
integer edge deltas and isqrt128 rational intervals independently bound unit
normal component errors by2^-30. Actual ABI5 memberships and public
edge-kind/mate/neighbor readers are checked under all three slot permutations.

Direct witnesses cover asymmetric triangle/chamfer/verified32-gon, source-order
rotation, all Frame scales and translations, minimum edge/height and maximum
bounds, irrational and perfect-square normals, all support strata and tiny/ULP
offsets, hostile positive-turn winding star, malformed/overlong/failed iterators,
unsupported heights, geometry-equal distinct identities, forged geometry/map/
typed/history/sample outputs and late publication failure with healthy retry.
These are explicit direct fault witnesses, not a claimed broad mutation score.

Actual fleet red: existing healthy box native sample status0 and exact
(-.75,-.75,.5), then missing `brepgpu.extrusion` ModuleNotFoundError;
one test /0.038s. No product operation existed for that red.

First full CPU run exposed a test setup error: dataclass `replace` correctly
rejected a bool origin orientation before the qualifier could inspect it.
Changed that deliberately forged witness to frozen-object alteration, preserving
the intended assertion. Independent semantics review also requested actual
native sampling of the minimum1/256-edge triangle at Frame scales.25 and4;
added those to the existing native method before the final qualification runs.
No production defect was needed to make these tests pass.

Final eleven tests, zero skips, existing native binaries:

| Placement | Tests | unittest seconds | Outcome |
|---|---:|---:|---|
| CPU1 |11|7.471|GREEN|
| CPU24 |11|7.385|GREEN|
| P100 device0 |11|7.987|GREEN|
| P100 device1 |11|8.390|GREEN|

All execution was on the owned fleet checkout; GPU runs were sequential after
the sampler owner released its lane. The lane was returned to the parent after
device1. Local checks were source/AST/diff only; no dependency, native build or
Git mutation. Whole oracle/test semantics review accepted after both findings
were resolved. Separately reviewed the whole constructor source and saved
`papers/2026-10-15-extrusion-construction-oracle-code-review.md`: accepted,
no blocking finding. Source is stable for actual caller/export/cost review.
