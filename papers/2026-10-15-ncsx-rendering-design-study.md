# First NCSX patch render: bounded construction design

2026-10-15. Recommend **authentic centerline-derived cubic ribbons first**, a
separate source-derived vessel only when actual Fourier/surface data arrives,
and reuse of the existing owned triangle depth-buffer renderer. This is an
illustrated surface assembly using the existing native P100 evaluator, not a
new CAD import/sweep/Boolean/meshing operation. No implementation or product
execution occurred during this review.

## Source and representation

The source-acquisition lane owns authentic ordered coil curves and any vessel
coefficients. Retain original bytes, coordinate units/frame, coil labels,
closure, transform/symmetry conventions and source version/hash. Render eighteen
curves only if original data or explicit authentic symmetries establish that
assembly. A documentary image is not an evaluable source model. Coil thickness
or ribbon width inferred for visibility must be labelled cosmetic, not authentic
winding-pack or casing dimensions. Do not invent a three-period vessel and call
it NCSX geometry because only photographs were available.

For closed ordered points P_i, uniform Catmull/Hermite segment controls can be
owned directly:

```
B0 = P_i
B1 = P_i + (P_(i+1)-P_(i-1))/6
B2 = P_(i+1) - (P_(i+2)-P_i)/6
B3 = P_(i+1)
```

Indices wrap only for a source curve declared closed. This is an interpolating
visual fit, not guaranteed CAD fidelity. For strongly nonuniform source knots,
use source parameters or an explicitly chosen chord-length tangent rule rather
than silently calling a uniform fit arc-length parameterized. Keep original
points; cap/resample only with an explicit retained selection/fit-residual record.
Bounds or residuals against sampled source points are not a certified continuous
Hausdorff error to an unknown original CAD curve.

Use smoothly transported frames instead of curvature-based Frenet normals,
handling nearly straight and antiparallel tangent cases explicitly. A closed
loop's transported frame may return with twist; distribute the closure rotation
by retained arc-length parameter before forming rails. Reject zero-length or
undefined tangent inputs rather than fabricating a frame. The primary report
by Hanson and Ma explains the inflection problem, incremental transport and
closed-loop correction; sections2 and3 were read, and the PDF was acquired in
ignored `papers/pdfs/hanson-ma-1995-parallel-transport-curve-framing.pdf`.
[Primary technical report](https://scholarworks.iu.edu/dspace/bitstreams/23d6fd59-b151-4064-88a7-f7e058b2cc69/download),
[institutional record](https://scholarworks.iu.edu/dspace/items/39fbe931-c2c8-43a1-9c9d-a207b92d551e).

## Ribbon versus tube

First choice: create source-fit left/right rail points C_i±w*U_i and fit each
cyclic rail cubically. A degree(3,1) polynomial patch with u-major/v-fast control
rows `(L_i,R_i)` interpolates those rails linearly across width. Eight homogeneous
controls of weight1 per segment fit the existing ABI. Native Ss/St provide the
actual shading normal; a ribbon is intentionally open across its two edges.
This minimizes patches and keeps width/frame fitting separate from evaluation.

If round tubes are visually necessary, use degree(3,2) patches with four exact
rational-circle quarters around each cubic centerline segment. Given cubic
coefficient/control fields C_i,U_i,V_i, quarter controls (A_j,B_j,W_j) give
homogeneous numerator `W_j*C_i+r*(A_j*U_i+B_j*V_i)` and weight W_j. Four patches
per segment cost four times the ribbon count. Interpolated frame fields need
not stay orthonormal between knots; this is a fitted sweep surface, not an exact
constant-radius pipe or a closed solid. Check nonzero native first-jet normals
and all longitudinal/quarter closure positions before rendering.

Normalize the entire assembled control set through one retained common frame.
Choose a power-of-two uniform scale after computing every final control bound,
including fitted-control overshoot and width. Ensure each actual homogeneous
numerator satisfies |XYZ|<=W and every weight is in1..256. Do not normalize each
coil independently and lose source placement. Native degree limits<=3 admit
both choices. If native frame conversion is used, actually consume its converted
positions/derivatives; otherwise render the named common normalized frame.

## Qualification and performance bounds

Separate **source-fit error** from **native evaluation error**. Dividing Hermite
tangents by6 and constructing transported frames generally produce controls
that are not exactly binary-representable. Quantize controls once, retain those
actual input bits, and check native output against an independent expanded
polynomial/rational evaluation of those actual bits. Comparing against the
ideal pre-quantization fit at the evaluator's2^-40/2^-30 grades would conflate
different errors. Use Fraction/Horner truth or the existing independent exact
expanded-polynomial machinery, not CPU/native parity or the same de Casteljau
routine. Check original input slot/UV bits, lineage/descriptor/count/order and
all statuses/finiteness, as in the existing actual trophy caller.

For ribbons, u5/v2 per segment is a useful initial bounded display grid; at128
segments per authentic curve, eighteen ribbons produce2,304 patches and23,040
native first-jet samples. This is a starting renderer resolution, not source-
curve or triangle chord certification. Preserve current exact evaluation
grades on every sampled position and both first derivatives. Closed rail
endpoints must agree; compare tangent direction and shading normals with their
declared parameter scaling. Reject degenerate sampled geometry. Record omitted
or unsupported source components explicitly.

If genuine vessel Fourier coefficients are available, evaluate the declared
periodicity/index convention and analytic derivatives to fit bicubic Hermite
surface patches. That fit and its sampled source residual need a separate
record; native patch qualification again uses actual final control bits.
Source period count comes from the file, not a visually convenient guess.
Vessel opacity/cutaway is a display choice and must not imply Boolean cuts.

Reuse `surface_png`'s orthographic triangle z-buffer as display only, passing
actual qualified native positions and native first-jet normals. Keep patch/
sample ownership and source identity out of that renderer. Depth interpolation,
Gouraud color, raster sampling and PNG encoding do not establish source geometry
or a tessellation bound. Label the artifact's authentic source components,
fitted/cosmetic surfaces and missing engineering detail plainly. Save original
data/recipe, controls/query bits, backend/device, raw jets, error summaries,
source pins and reproducible SVG/PNG. No runtime dependency, new kernel ABI,
complete NCSX CAD model or comparative speedup is selected by this proposal.
