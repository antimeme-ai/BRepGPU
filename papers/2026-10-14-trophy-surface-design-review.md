# Rational torus sculpture design review

2026-10-14. Accepted with the explicit envelope, seam and display constraints
below. Read actual `Patch`/Engine/JetBatch bindings, native patch admission and
the iteration-one numerical contract. Source-only review; no code or execution.

The quarter-circle homogeneous controls `(1,0,1),(1,1,1),(0,2,2)` expand to
`A=1-t²`, `B=2t`, `W=1+t²`. Thus `c=(1-t²)/(1+t²)`, `s=2t/(1+t²)` and
`c′=-4t/(1+t²)²`, `s′=2(1-t²)/(1+t²)²`. Signed quarter-turn permutations give
all four circle quadrants without irrational coefficients. The proposed tensor
controls have weights Wu*Wv, in1..4, and their expansion is exactly

```
P  = ((R+r*cv)*cu, (R+r*cv)*su, r*sv)
Pu = ((R+r*cv)*cu′, (R+r*cv)*su′, 0)
Pv = (r*cv′*cu, r*cv′*su, r*sv′)
```

Pack nine controls per degree(2,2) patch with v fastest, as required by the
existing evaluator. Sixteen patches cover each torus. Apply affine matrices
to homogeneous numerators as `Anew=M*A+b*W`; retain W. Position truth becomes
`M*P+b`, derivative truth `M*Pu`/`M*Pv`. Do not transform weights or translate
derivatives. Signed axis permutations with determinant+1 give rigid rotations;
arbitrary dyadic matrices may shear/scale and must be described as affine
surface images. Negative determinant reverses the cross-product normal relative
to transformed outward normals, so select proper rotations or explicitly account
for determinant sign.

Native admission requires each numerator component's magnitude <=W and each
weight in1..256. The tensor weights fit automatically, but translated sculpture
controls must still fit the local cube. One simple proof is dyadic R=1/2,
r=1/8, signed permutations and translations with each component <=1/4 in
magnitude: each transformed control ratio is <=7/8. Check every emitted control
exactly against this envelope before uploading. Require R>r>0 to avoid horn/
spindle degeneracy. If larger placements use Patch.origin/scale instead, obey
the existing frame contract and actually convert downloaded jets before using
them as world values; evaluate alone returns local jets. No frame conversion
is needed when transforms are baked into admitted controls and the frame is
identity.

For every actual submitted patch slot/parameter pair, compute independent
analytic P/Pu/Pv using Fraction from the actual binary64 u/v values and original
dyadic recipe, not the generated control net or another evaluator. Check exact
query/source binding, successful statuses and finite native outputs. Compare
the actual native binary64 values as exact ratios to the analytic fractions:
maximum componentwise errors <=2^-40 for position and <=2^-30 for first
derivatives, the declared iteration-one grades. Include endpoints, quadrant
joins and interior samples on every chart. The native derivatives used for
shading must be the checked Ss/St vectors, rather than analytic normals silently
substituted into the picture.

Check seam positions and oriented tangent directions/normals. Raw first
derivatives across adjacent quarter charts are **not equal**: endpoint circle
speed is1 whereas the next chart's start speed is2. This is a reparameterization
seam, with regular geometric tangent continuity. For unscaled R=1/2,r=1/8,
the cross-product magnitude is bounded below by r*(R-r)=3/64; rigid rotations
preserve it. Verify nonzero checked native cross products before normalization.
Deduplicated seam vertices are unnecessary for sampled display and must not be
presented as constructed B-rep topology.

Ten tori at n17 give46,240 native samples and81,920 display triangles; n25 gives
100,000 samples and184,320 triangles. n17 uses dyadic parameters and is a sensible
first bounded artifact. Record actual patches, queries, backend/device, controls
and maximum errors. Native upload/evaluation/download are the geometry-producing
operation; Fraction qualification, shading, SVG generation and browser PNG
conversion are separate costs, with no comparative performance claim.

Depth-sorted sampled triangles can illustrate this surface assembly, but mean-
depth painter sorting does not guarantee correct occlusion for intersecting or
cyclic overlaps. The result is a GPU-evaluated rational surface sculpture,
without fusion, Boolean-solid, trim-aware meshing, chord-tolerance or exact
visibility claims. No new evaluator, runtime dependency or product capability
is needed to create the artifact.
