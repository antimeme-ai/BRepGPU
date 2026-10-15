# Interchange design: source and semantics review

Disposition: **accepted**. Source-only review of `docs/INTERCHANGE_DESIGN.md`
against the pinned NVIDIA schema, exporter, evaluator and original mouse fixture,
the acquired OpenUSD grammar, and the source study. No blocking finding remains.

The explicit owned ASCII subset consumes the complete file and rejects unknown
syntax, composition, transforms, duplicate declarations and trailing tokens.
Typed immutable values preserve numeric types and uninterpreted trim evidence.
This is a declared narrow source boundary, not an implicit fallback parser.

The geometry contract matches the actual NVIDIA records: Euclidean unweighted
XYZ, separate positive weights, U-major/V-fast control order, degree equal to
order minus one, and packed NURBS offsets advanced for every NURBS face. The
unchanged mouse has four original faces, two admissible single spans and two
unsupported multispans. Original face ranges do not replace natural carrier
knot domains; neither rectangular trim labels nor faceuse orientation invent
trim membership or a unique outward normal.

Content hash, prim path and original face index remain distinct from numeric
native namespace/face/chart IDs. Duplicate geometry retains distinct identity;
all original faces and uses survive. Unsupported analytic geometry does not
become an invented patch. Source coordinates and optional unit/up-axis metadata
remain unchanged. File bytes used for hashing, parsing and export are identical.

The numerical envelope is conservative. With unchanged positive basis weights,
maximum Cartesian control displacement delta bounds whole-carrier position
conversion error. The stated first-partial bound 4*p*kappa*delta/L bounds the
numerator and denominator derivative contributions. Exact affine original UV
queries, rounded scalar-reference UV queries and rounded native frame factors
are separate quantities. Independent product-rounding rho cannot be inflated
by the observed converted output; bitwise prescribed-product checks additionally
detect wrong native frame conversion. Degree-zero axes require no regular-normal
claim.

The native evaluator remains the owned dependency. Source studies do not adopt
NVIDIA/OpenUSD code. Carrier conversion explicitly leaves trims and solid import
unsupported, and exploratory results cannot close the external alpha gate.

References and exact source hashes: `2026-10-15-interchange-semantics-study.md`
and `interchange-semantics-sources.tsv`.
