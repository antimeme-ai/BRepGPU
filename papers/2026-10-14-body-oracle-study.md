# Narrow body sampling: independent oracle design

Scope: immutable owned box, capped cylinder and one-hole plate; face samples
and oriented normals from the existing CPU/P100 first-jet evaluator. This study
proposes direct checks for the [body design](../docs/BODY_SAMPLING_DESIGN.md).
It adds no product code, native execution, dependency, mesh, import or general
B-rep correctness claim. The [topology design](../docs/TOPOLOGY.md) and
[source study](2026-10-13-acceleration-roadmap-study.md) establish the distinction
between shared entities, coedge occurrences, carrier charts and trims.

## One independent analytic model

Use the declared body dimensions as exact rationals: box half-width 3/4,
half-height 1/2; cylinder/hole radius 1/4. Derive expected geometry from these
definitions rather than production control-net construction or curve evaluation.
For a quarter wall, let

`C(s)=((1-s²)/(1+s²), 2s/(1+s²))`,
`S(s,v)=(r*Q*C(s), -1/2+v)`,

where Q is the declared signed 90-degree rotation. Every supplied binary64
parameter, origin and scale has an exact `as_integer_ratio`; Fraction evaluates
the ideal map without a floating approximation to angle or pi. Plane maps are
affine. Source/chart ordering is checked against declared geometric roles, so
matching the evaluator to a wrongly constructed control net cannot certify a
body. Existing exact Bernstein jet machinery may check the actual net as an
additional construction correspondence, without supplying body geometry truth.

The ideal cylinder/hole normal is respectively `+(Q*C(s),0)` or its negative.
Plane outward normals are their known signed coordinate axes. This avoids
recomputing the same numerical cross/hypot operation as the caller. Check world
XYZ against the exact framed map and each normal against this independent
direction using the design's finite-corpus component targets, 2^-36 and 2^-30.
At edges/corners, normals belong to individual oriented faces: do not average
them or require opposed adjacent face normals. Uniform positive frame scale
does not change unit normals.

## Exact trim labels and the important hole-boundary limit

Classification is about submitted binary UV, not rounded XYZ. An independent
oracle converts u/v to Fraction and evaluates `(2u-1)^2+(2v-1)^2` against 1 for
disk caps and 1/9 for plate holes. This is a different implementation of the
product's cleared integer predicate. Rectangle range rejection precedes trim
membership. Outer rectangle edges and kept exact circle equality are BOUNDARY;
kept strict regions are INTERIOR; the other finite chart region is REJECTED;
NaN/Inf/out-of-domain values are INVALID. No tolerance band supplies equality.

For a cylinder cap, UV cardinal points `(0,1/2),(1,1/2),(1/2,0),(1/2,1)` are
exact circle boundaries. Their inward/outward representable neighbors and
rounded rational-arc UVs attack both signs and mistaken boundary promotion.

**No binary64 UV lies exactly on the plate's circular hole.** With dyadic u/v,
`9*((2u-1)^2+(2v-1)^2)=1` would, after clearing a power-of-two denominator,
require nine times an integer to equal a power of two. This is impossible.
Thus plane-hole boundary records are symbolic edge/coedge correspondences;
rounded UVs derived from those edges receive their actual INTERIOR/REJECTED
classification. Exact plate top/bottom BOUNDARY samples come from the outer
rectangle. In particular, rounded 1/3 or 2/3 cardinal UVs are not exact hole
boundaries. Check them and their nextafter neighbors explicitly.

A wall uses the lifted atlas a in [0,4], v in [0,1]. Native chart k supplies
s=a-k. Top/bottom v=0/1 and the physical periodic seam a=0/4 are BOUNDARY;
internal joins a=1/2/3 are INTERIOR unless on top/bottom. Check both chart
representations at each join. Their geometry and unit normals agree; derivative
magnitudes need not agree because this piecewise rational parameter is not
uniform angle. Do not infer boundary from every local s=0/1.

## Complete shared-boundary correspondence

For each line, check the affine spatial curve against each face UV map composed
with its analytic plane/wall map as a full expression. For circles, canonical
t in [0,1] gives a=4t and the four rational spans. Require exact quarter identity
`Cx²+Cy²=1`, span endpoint/closure equality and the declared rotations.
For a planar use, substitute

`u=(x(t)+a_plane)/(2*a_plane)`,
`v=(y(t)+a_plane)/(2*a_plane)`

into the plane map; it must equal the spatial circle identically. Retain the
original dyadic radius/half-width symbolically: a rounded 1/6 coefficient is
not an exact plate UV map. Wall uses map to `(4t,0/1)`; seam uses map to
`(0/4,t)`. Reversal composes canonical t with 1-t; it does not change shared
edge identity. Endpoint-only matching or comparing two production evaluators
does not establish correspondence.

The wall loop contains two distinct seam coedges referencing one spatial line
and the same face, at different atlas locations and with opposed traversal.
Cap circles also remain one closed edge each: four rational spans do not invent
four body edges or vertices. Finite floating map probes check implementation
rounding separately from these analytic identities, using stated allowances;
they do not turn rounded UV into exact-boundary witnesses.

## Direct immutable topology and request checks

Under the design's one-wall-face/seam convention:

| Body | Vertices | Edges | Faces | Loops | Coedges | Native charts |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Box | 8 | 12 | 6 | 6 | 24 | 6 |
| Cylinder | 2 | 3 | 3 | 3 | 6 | 6 |
| One-hole plate | 10 | 15 | 7 | 9 | 30 | 10 |

Check typed reference ranges, unique entity IDs within their namespace/kind,
ordered loop closure, correct outer/hole traversal relative to face orientation,
exactly two opposed uses per edge and the repeated same-face seam uses. The
plate's two planar holes matter: naive V-E+F gives 2; counting each hole in a
face's planar domain subtracts two and yields boundary Euler characteristic 0.
Box/cylinder give 2. Incidence/counts supplement geometric correspondence;
neither count equality nor a manifold flag alone supplies it.

Snapshots/prepared requests contain immutable records/tuples/bytes. Existing
`Patch.__post_init__` freezes controls through `_flat` into a tuple, so these
Patch values can be retained safely; temporary native arrays/ctypes descriptors
are not authoritative body data. Mutate original constructor inputs and
attempt ordinary nested mutation; subsequent samples must keep their source
geometry and labels. Prepared requests retain their exact snapshot authority,
including frame/chart ordering; foreign body/frame requests are refused before
gather or native work. Public entity identity still uses body/revision/typed ID,
not an object address.

Every candidate remains in the result: source body/revision/face/chart, submitted
UV, original row and trim/native status. REJECTED/INVALID rows have NaN XYZ and
normals and the not-evaluated sentinel. Admitted rows gather densely for the
existing SoA evaluator and scatter to original order. Deliberately mix accepted,
rejected and invalid rows across faces/charts; this attacks a wrong full-count
stride or scatter map. Native/conversion/normal failures raise rather than
publishing a partially successful body batch. Downloaded immutable results
remain readable after sampler/engine closure; use-after-close is refused.

## Minimal campaign

Use one small adversarial request set per body covering plane/circle cardinals,
nextafter trim sides, invalid values, wall joins/seams, repeated edge uses,
opposite cap/inner-wall normals and mixed gather/scatter. Add empty/selected
faces and foreign snapshot/frame cases. Compare the reused prepared sequence
with one-shot preparation by row/source identity and numerical contract.
Exercise frame scales 1/4,1,4 and bounded exact translations; labels/topology/UV
stay fixed, ideal positions transform and normals do not. Include one ordinary
17-grid per body; larger 65-grids belong to the measured caller with the same
direct oracle, not another control-degree/cache campaign. CPU1/CPU24 and each
P100 are requested explicitly; actual native runs occur on the fleet only.

Keep classification/preparation, native transfer/evaluation, provenance copy,
conversion, normal/scatter, cleanup and export/qualification costs visible.
A point cloud with normals establishes this caller's supported samples; it does
not establish triangle chord error, watertight mesh embedding, general trim
arrangements, arbitrary import, intersection or Boolean value.
