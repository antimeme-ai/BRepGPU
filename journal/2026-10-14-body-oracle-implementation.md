# Body sampling oracle implementation

Implemented the reviewed independent truth/caller unit in
`python/brepgpu/body_oracles.py` and `tests/test_body.py`.

The oracle checks exact submitted binary UV trims with Fraction, canonical
positions and oriented unit normals, complete rational edge-to-face identities
by polynomial cross-products, typed references/opposed uses/ordered UV loops,
outer rectangle domains and hole-sensitive Euler counts. An independent struct
decoder checks every declared packed table's fields, geometry tags, padding,
control offsets and adjacency against its logical records. Geometry expectations
come from canonical dimensions/roles, rather than native CPU parity, production
classification or face orientation fields.

Nine direct tests cover canonical wire/topology and immutable records, exact
trim adversaries and seams, public curve/UV parameter evaluation, 17-by-17 grids
plus mixed reordered requests, positive power-of-two frames, selected/empty
faces, validation and the actual 660490-row construction cap, exact snapshot
binding, closed lifetimes, and primary-error preservation during two actual
native-child cleanup failures. Scatter assertions compare submitted UV bytes,
including signed zero/NaN, to the original immutable request.

The coordinator recorded the first direct red with a healthy existing Engine
and missing body API before production implementation. The initial three-row
box test subsequently passed on the fleet (one accepted, two invalid; expected
position and normal). The cleanup fault test was supplied before its sampler
fix; coordinator owns its actual red/green execution. Local checks here were
stdlib AST parsing only. Full fleet execution and review are pending; no local
native execution, dependencies or product edits outside this owned unit.

Source review corrected an accidental tuple-negation expression before
qualification and removed an incorrect Curve.id assumption: curves are
geometry slots, while topology entities carry typed IDs. Root requested full
packed decoding and explicit outer UV-domain checks; both are included.

Numerical source review identified missing explicit binding between the ideal
analytic proof and actual carrier controls. Resolved by checking every actual
homogeneous net exactly against independently derived canonical plane/wall
coefficients before the complete edge-to-face identity checks. No numerical
engine supplies expected values.

The coordinator ran all nine tests on CPU24: PASS in 3.543 seconds. The
cleanup-fault injections left artificial close methods attached after consuming
their handles, causing destructor-only unraisable exceptions. Corrected this
test hygiene by restoring each original close method before its real close and
one-shot injected exception. Sampling still encounters both cleanup failures;
later destruction uses ordinary idempotent close. No product change. AST parsing
passes; final CPU/P100 campaign remains coordinator-owned.
