# Actual OM10 cone apex CPU/P100 diagnosis

2026-10-15. Narrow nonvisual source evaluation; no native production change,
watch rasterization, performance claim or new qualification mechanism.

Source: OpenMovement OM10 MAN VA01.3, original STEP SHA256
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.
The retained source adapter snapshot identifies:

| Carrier | Face / orientation | Original apex vertex | Original spatial line / edge | Reference radius | Semicone angle |
|---|---|---|---|---|---|
|1215|166149 / +1|68670|39324 / 86907|0.3mm|1.0471975511966rad|
|1431|171148 / −1|78001|47467 / 101015|0.17725mm|1.0471975511966rad|

The original-UV cone map is
`S(U,V)=O+(R+V sin(a))·(X cos(U)+Y sin(U))+V cos(a)·Z`.
The selected source faces approach the apex from positive signed radius
`rho=R+V sin(a)`. Their source frames and face signs remain unchanged.

## Question and direct run

Could `V=-R/sin(a)` cancel to zero on CPU while CUDA's sine leaves a tiny
negative radius and reverses the normal? This is plausible in general and the
existing generic cone fixtures do not answer it for these actual source angles.

Built a tiny BGMESH01 from the two actual carriers, original frames, face signs
and source apex coordinates. Used zero-triangle geometry and BGVIEW01 with zero
view requests. Each native process evaluated source first jets and wrote the
existing128-byte jet records. It never projected or rasterized the watch.

Submitted48 query rows per backend: two carriers × three U values
`(−pi,0,pi/2)` × eight named V constructions:

- Original foreign-recovered source Vmin.
- CPU-computed `−R/sin(a)`.
- Nearest binary64 value from an85-digit apex calculation using the exact
  binary64 source R/a values and an independently summed high-precision sine.
- Each of the preceding two apex constructions advanced one binary64 value
  toward the positive-radius interior.
- The nearest apex value plus1e−10mm and plus1e−6mm in the slant parameter.
- V=0, the reference-radius section.

The two apex constructions coincide, as do their nextafter values, so this is
48 named query rows and36 distinct UV pairs. This count is not enlarged into a
claim about an entire cone parameter family. Ran native CPU24, P1000 and P1001
as separate processes with the same scene and zero-view bytes.

## Results

| Carrier | Canonical nearest apex V | Foreign recovered Vmin | Foreign Vmin minus real apex | Signed radius at foreign Vmin |
|---|---|---|---|---|
|1215|−0.34641016151377496|−0.34641016151378234|−7.3638676411465e−15mm|−6.38378239159465e−15mm|
|1431|−0.20467067042772205|−0.20467067042772288|−8.359221521936157e−16mm|−7.216449660063518e−16mm|

The real apex is generally not exactly representable in binary64. The canonical
submitted values differ from the high-precision real apex by+1.911547261079117e−17mm
and−3.254883724748309e−18mm respectively. These are explicit parameter rounding
quantities, not claims of exact transcendental equality.

For both canonical apex values, every backend produced signed radius0 in its
first derivative, set the existing limiting-normal flag1, and returned the
same normal as the positive-radius interior limit at that U. Nextafter toward
the interior, the1e−10/1e−6 offsets and V=0 retained that orientation. For example,
at U=0 the oriented normals are approximately:

- Carrier1215 / face166149: `(0.49999999999999795, 0.8660254037844398, 0)`.
- Carrier1431 / face171148: `(0, −0.499999999999998, 0.8660254037844397)`.

At the foreign recovered Vmin, all backends consistently selected the negative-
radius nappe and reversed these normals: dot with the positive-radius interior
normal is approximately−1. The old Vmin lies beyond the apex. Boundary canonical
XYZ retention does not itself correct a normal evaluated at that overshot UV.

All three complete48-row jet buffers were **byte-identical**, not merely close;
maximum numeric difference was0. Each buffer SHA256 is
`d8a4323d4b290e050ff9b61f9c567ba9d2d140a76b328f90f0019ffd96753c88`.
This diagnoses these actual carriers and probes. It does not establish universal
CPU/device transcendental agreement.

## Consequence

No native evaluator change is justified by this run: the suspected CPU/P100
sign disagreement was not reproduced. The concrete issue is a foreign UV
endpoint overshooting a verified source pole and therefore entering the wrong
nappe for the intended face. The source/mesh owner will canonicalize that verified
pole V to the declared nearest apex representative, retain the original recovered
endpoint and its perturbation budget, and validate the owned UV geometry. Do not
clamp arbitrary negative-radius cone queries or silently broaden the evaluator.

The private diagnostic retains its input scene/views, query labels, actual jets
and per-backend results under `context/om10-native/apex-actual-carriers-01`.
Original source bytes remain private. This paper records source entity IDs and
publicly inspectable numeric conclusions without operational host/path details.
