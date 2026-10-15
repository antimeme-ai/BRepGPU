# Actual extrusion-result mesh: independent oracle and direct test study

The bounded claim is a complete CPU triangle mesh of the actual existing
`ExtrusionResult`, preserving its solid/history identity, original carriers,
trim domains and all original edge/coedge parameters. This is an exact planar
polyhedron; subdivision and a circular segment budget add no value.

## Independent mechanism

Use `extrusion_oracles.qualify_result` once for the original lattice geometry,
whole source graph/control maps and six-kind history. The mesh qualifier must
not call product mesher helpers or use its fan/diagonal construction as truth.
Reuse only the existing type-agnostic independent source-curve, source-UV,
Bernstein patch and integer boundary-chain predicates in `meshing_oracles`.

Reconstruct world vertex positions directly from the original profile, height
and original Frame. Require exactly the 2N original result vertex generators,
actual nearest binary64 rounded coordinates and dense output identity. Require
all 3N original edges with exact `(0,1)` parameters and shared endpoint indices;
all 6N original coedge occurrences retain intrinsic endpoint parameters, sign,
face/edge identity and original lifted UV values. Reconstruct complete face
boundary words from original loops rather than the triangle list.

For every triangle check its actual original chart ownership, exact source
patch/UV composition and positive face orientation. Caps project to XY. Oblique
sides project along their actual original edge and Z, checking the complete
point lies in that affine plane. Sum directed triangle boundary chains and
compare against the complete original face boundary chain. Strict positive
projected triangles and equality of complete boundary chains establish whole
convex-domain coverage; vertex-only membership and area equality alone do not.
Check opposite degree-two mesh-edge incidence, single cyclic vertex links, one
connected shell, and Euler2 as a supporting invariant.

Independently inspect the exact ratios of saved binary64 coordinates. Paired
lower/upper vertices must retain identical XY and two strictly ordered cap
heights; the rounded profile must retain strict global convex support. Each
rounded side remains the vertical extrusion of its rounded source edge. Check
positive rounded triangle orientation and complete face chains using those
actual coordinates. Axis-aligned plane assumptions from the through-bore
qualifier do not apply to these oblique sides.

The exact mesh equals the original polyhedral boundary, so `curve_bound=0`.
Recompute `rounding_bound` as the maximum exact L1 displacement of a saved
vertex. Barycentric affine interpolation bounds displacement on every complete
triangle by the same maximum; require `error_bound=rounding_bound<=tolerance`.
The rounded polyhedron need not lie exactly in the original supporting planes.

## Bounded direct tests

Use independent expected counts (2N vertices, 4N-4 triangles, 3N edge chains,
6N boundary occurrences), whole-domain qualification and explicit history
identity in actual 3-, 6-, 16- and maximum32-vertex inputs. Exercise a narrow
nearly collinear admitted lattice profile and non-axis sides, height extremes,
all admitted scale values, frame extrema and subnormal origin. A subnormal
origin provides actual nonzero output rounding: accept its exact maximum
rounding tolerance and reject an immediately smaller positive exact tolerance.

Reject malformed/unsupported result and tolerance types, nonfinite/nonpositive
requests, and budgets below actual rounding. Confirm prior source/output
records survive failure, factory-only construction, immutable leaf values,
lazy result adapter and unchanged canonical/Boolean type admission. Write direct
output fault cases only where they expose geometric claims: omitted/duplicated/
flipped triangles, foreign source/chart/UV, split shared source identity, wrong
coedge direction/parameter, changed actual rounded coordinates and false bounds.
These exercise contract fault classes; no mutation campaign or oracle-validation
framework is planned.

Product geometry and tests execute on the fleet. One relevant green direct suite
and affected compatibility suite, then actual caller qualification, are enough
unless a concrete failure requires another run. No GPU mesher, external-alpha
claim, numerical/native rebuild or general B-rep certification follows.

## Existing sources inspected

- `python/brepgpu/extrusion.py`, `extrusion_oracles.py`: actual source/result
  contract and independently reconstructed whole source ancestry.
- `python/brepgpu/cut_meshing.py`, `cut_meshing_oracles.py`: immutable leaf reuse,
  retained actual result identity and full boundary-chain coverage pattern.
- `python/brepgpu/meshing_oracles.py`: independent original maps, Bernstein patch
  evaluation and directed chain operations.
- `tests/test_cut_meshing.py`, `docs/EXTRUSION.md`,
  `examples/extrude_profile.py`: actual failure paths and distinct input families.

## Design review

Reviewed `docs/EXTRUSION_MESH_DESIGN.md`. Accepted: the actual directed source
loop already accounts for the lower-cap orientation, Gram-system inversion
handles oblique sides, and rounded embedding is checked as the rounded prism.
Constant reader properties `curve_bound=Fraction(0)` and `quarter_segments=0`
are a harmless explicit compatibility surface; there is no subdivision budget.
`.feature_result`, `.body` and `.history` identities are the retained contract.
The design has a sufficient direct coverage/error mechanism and an appropriate
bounded stopping rule. No open oracle finding.

## Plan review

Reviewed `docs/EXTRUSION_MESH_PLAN.md`. Accepted: genuine missing-API red precedes
production; the independent qualifier owns whole face domains and actual float
ratios; affected compatibility and three distinct actual callers address real
integration obligations. No GPU/mutation/secondary certification pass is added.
Proceeding with the bounded qualifier and direct tests only.
