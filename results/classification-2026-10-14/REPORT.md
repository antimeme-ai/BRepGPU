# Exact material and finite-face classification

2026-10-14. CPU exact construction on the qualified execution fleet. This
breadth-first unit delivers closed material membership for the owned box,
cylinder and one-hole plate, plus complete closed contact of their rectangular
planar faces. No GPU placement, native rewrite or Boolean operation is claimed.
Python's shipping role is explicitly a decision
[after the breadth sweep](../../papers/2026-10-14-python-role-return-pass.md).

![Actual exact point labels and emitted face contact loci](caller/classification.svg)

Point panels read actual classified witnesses and original vertex/circle profiles.
The displayed cylinder-rim point with a subnormal positive y offset is exterior;
the corresponding plate point is interior. Its exact coordinates differ even
when the display pixels coincide. Contact miniatures read actual world vertices;
empty loci, points, segments and areas retain separate exact TSV records.

## Correctness and support

The [API](../../docs/CLASSIFICATION.md), [design](../../docs/CLASSIFICATION_DESIGN.md)
and [plan](../../docs/CLASSIFICATION_PLAN.md) define input and failure behavior.
Integer, Fraction and finite binary64 coordinates become exact rational values
before frame arithmetic. Material feasibility precedes collecting every true
supporting original face ID. Hole void, cap rim, slab corner and wall seam
semantics retain physical faces independently of chart multiplicity.

Finite rectangular contacts retain dimensions -1/0/1/2 and world/both source UV
coordinates. Carrier relation and oriented normals are separate from disjoint,
touch, crossing, overlap and full-domain coincidence. Disks, hole caps, walls,
algebraic points and general imported geometry are explicitly outside this API.
A transverse finite-face crossing is not a solid penetration decision.

Independent point truth reads original vertices/rim curves in world coordinates.
The contact oracle enumerates exact supporting-plane/edge-halfspace intersections
and affine rank, rather than repeating the product's interval intersection.
Literal fixtures anchor expected material/face lists and each finite dimension;
all box rectangular orientations, plate side faces, swapped sources, translated
scales, one-ULP/subnormal/huge-value inputs, unsupported cases, query ownership
and atomic iterator failures are exercised.

[Qualification](qualification.tsv) records the actual direct healthy-body/absent
entry red, initial green, 61 direct/new-and-compatibility tests in 9.371 seconds
with one existing GPU-only skip, and the final 12 new/body-adapter tests in
0.466 seconds. No new native source changed, so unrelated host/device sanitizer
campaigns were not repeated. [Protected binary hashes](protected-binaries.tsv)
remain unchanged.

[Seven focused mutants](mutations-final.tsv), with [target methods](mutation-targets.tsv),
were killed by geometry assertions: wrong hole sense, rounded radial sign,
rounded frame normalization, missing incident face, boundary segment called
crossing, omitted corner and wrong right UV. Every healthy baseline and retry
passed. The corrected controller requires exit1 and a completed failures-only
unittest summary; import/runtime/crash errors earn no kill. This is a named fault
class result, not a universal mutation score.

## Complete caller costs

The [actual caller](../../examples/classify_geometry.py) ran serially in one
already imported Python3.12.3 process on Xeon E5-2697v2/x86_64 Linux. This exact
operation uses one process, no CPU24 workers or CUDA library. Prepared bodies
and queries persist; every operation publishes fresh owned values. The caller
reads all point labels/faces/coordinates or contact metadata/world/bothUV values.
No owned sync, tests, builds or mutants overlapped measurement.

Twelve cases have one first call, one excluded warmup and five repeats: 84
qualified operation outputs, 60 repeated wall observations, 295,092 individual
query results. Larger point batches use deterministic rational clouds plus
literal boundary/subnormal/maximum-float witnesses. Contact batches repeat the
same eight prepared pairs; 512 means 512 complete requests, not 512 distinct
geometries or a more complicated single solid. All returned queries/source refs
are checked against their submitted originals before independent geometry truth.

Complete reused milliseconds, median [minimum, maximum] over five repeats:

| Request | 12 / 8 queries | 1,536 / 128 queries | 12,288 / 512 queries |
| --- | --- | --- | --- |
| Box material points | 0.416 [0.398, 0.438] | 54.47 [51.51, 72.92] | 402.61 [378.28, 497.85] |
| Cylinder material points | 0.421 [0.398, 0.453] | 47.51 [41.80, 123.00] | 364.95 [352.92, 377.01] |
| Plate material points | 0.480 [0.479, 0.518] | 63.75 [55.35, 76.67] | 489.56 [468.60, 516.22] |
| Rectangular-face contacts | 2.190 [1.997, 2.320] | 32.92 [32.51, 36.07] | 147.46 [129.45, 300.00] |

Operation time includes exact conversion/construction/result publication;
consumption is included in complete wall. Large point operation medians are
385.39/351.59/477.02 ms for box/cylinder/plate, with roughly 12.5–14.8 ms reading
the result. Large contact construction is 145.42 ms and reading 1.67 ms. Medians
of separate phases need not sum to the complete median.

Preparation is outside reused wall: large point bodies/queries cost
33.13–40.64 ms; the eight-face-pair pool and 512-entry request list cost 6.57 ms.
First-call complete spans are 394.21/360.44/501.09 ms for large box/cylinder/plate,
and 140.72 ms for 512 contacts. “First call” is after imports in this process,
not fresh-process startup. Imports, final process shutdown and export rendering
are not measured. No transfer or residency costs arise in this exact CPU API.

Independent checking runs **outside** complete caller wall and is expensive:
large repeated point-check medians are 1,365.81/900.53/1,722.96 ms, and the
512-contact check median is 1,527.96 ms. These are geometry plus original-query
binding checks, not a hidden part of the faster operation. Initial checking costs
are also in raw.tsv. Widened ranges show run variability; no cause or comparative
speedup is assigned. Exact arithmetic cost also depends on rational bit sizes.

[Raw observations](caller/raw.tsv), [phase summaries](caller/summary.tsv),
[preparation](caller/preparation.tsv), [environment](caller/environment.tsv)
and [unchanged source pins](caller/sources.tsv) retain every case. Exported
[box](caller/box-points.tsv), [cylinder](caller/cylinder-points.tsv),
[plate](caller/plate-points.tsv) witnesses and [contact vertices](caller/contacts.tsv)
with [source frames/faces](caller/contact-sources.tsv) allow independent review.
[Saved-artifact review](../../papers/2026-10-14-classification-artifact-review.md)
independently checks all36 exported points, eight complete contact loci and SVG
geometry/labels. Larger fleet outputs were qualified during execution; their
public saved artifacts retain aggregates, reproducible inputs and timing rather
than every large output tuple. [Performance review](../../papers/2026-10-14-classification-performance-review.md)
independently recomputes the reported statistics and accepts their scope.

Continue uncovered arrangements, meshing, interchange and solid/feature themes.
[Return-pass notes](../../papers/2026-10-14-classification-round2-notes.md) retain
algebraic witnesses, curved contact and certified accelerator filters without
starting the deferred integration/language work.
