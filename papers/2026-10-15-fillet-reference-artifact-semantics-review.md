# Fillet same-world foreign reference: final artifact review

2026-10-15. **ACCEPTED; all three declared comparisons GREEN.** Read the complete
reference script and actual final reference-small/mid/large TSV exports, binding
them to their saved CPU1-r1 owned cases. Stdlib artifact analysis only; no product,
foreign geometry or benchmark execution and no source edits.

Every script/input hash matches current source and the actual owned recipe,
quality, source vertices, exact edge points and native samples. The recorded
world lower corner, dimensions, radius, all eight vertices and selected endpoints
equal their exact original Frame transforms and binary64 hex values. No reference
boundary conversion silently rounds these submitted source values. The selected
copied curve's endpoint probes equal the submitted original selected edge. Actual
script selection requires exactly one copied edge with those exact endpoints.

The script creates and probes the original, makes a copy, selects only on that
copy, mutates it through selective fillet_edges with circular cross-section1,
checks the mutator returned that copy, and discards its borrowed selected handle
before any later access. Saved original body volume/manifold/tight bounds and all
12 edge endpoint/range/three-point probe records are identical before/after, for
every recipe. Those original edges match all12 actual submitted source edges.
This establishes the declared original-preservation probes; it is not foreign
typed ancestry, a proof of every point of an arbitrary curve, or a general
failure-atomicity guarantee for the mutable foreign API.

All131 witnesses per recipe bind exactly and in order to75 actual owned exact
edge samples (five per physical edge) and56 actual saved native interior rows
(eight spread queries per face, all seven faces). All393 submitted witness points,
foreign closest points, distances and residuals are finite. Residual recomputation
matches exactly except five cross-platform results differing by one ULP; none
changes a criterion. All saved maximum errors and limit comparisons reconcile.

| Actual recipe | Relative volume error | Maximum boundary distance/residual |
| --- | ---: | ---: |
| small: edge9,r1/16,scale1 | 7.305522177389586e-15 | 2.2887833992611187e-16 |
| mid: edge11,r3/8,scale1/2 | 5.41752351275284e-12 | 2.3914935841127266e-15 |
| large: edge12,r1/2,scale2 | 5.991743010050966e-11 | 2.8108601893111292e-15 |

All five outcomes per recipe pass: original preservation, manifold, volume,
bounds and boundary. The volume comparison uses the original exact rational/pi
coefficients converted to its declared floating analytic value. Requested foreign
volume accuracy and relative comparison limit remain1e-8; bounds1e-8*scale and
boundary1e-7*scale remain unchanged. The exported criteria and script's nonzero
failure path agree; no failed outcome is hidden or tolerance widened.

The installed reference identifies usd-brep1.0.0. The separately studied pinned
reference source informs semantics; it does not substitute for runtime identity.
These finite boundary/manifold/bounds/volume comparisons provide foreign geometry
agreement for the three actual cases. They do not qualify foreign material/
history, general fillets, GPU feature construction, general result meshing or
external-alpha corpora. The preceding revolution volume disagreement remains
open; these unrelated green results do not resolve it. Repeated performance
claims belong to the separately reviewed48-call campaign.
