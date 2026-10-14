# Complete analytic face sections

2026-10-14. Delivered exact CPU intersections of closed factory planar faces
and finite cylindrical walls, with complete components, named trim/seam events,
physical spans and coupled spatial/two-UV maps. The existing C++/CUDA position
evaluator consumes samples on both source surfaces. **GPU intersection
construction and a complete-consumer speedup are not established.**

[API](../../docs/INTERSECTIONS.md) · [design](../../docs/INTERSECTION_DESIGN.md) ·
[plan](../../docs/INTERSECTION_PLAN.md) ·
[source studies](../../papers/README.md#analytic-intersection-beachhead).

![Actual four-arc P100 section in world, plane UV and wall atlas UV](caller/p100-0-clipped/section.svg)

## Geometry and qualification

Supported operands are one closed rectangle/disk/rectangle-minus-open-disk
plane face and one closed finite-height cylindrical wall, in either order,
under existing translated/power-of-two-scaled Frames. Exact quadratic root/
cell decisions return every circle, clipped arc, generator segment, tangent
generator or isolated contact in that family. Boundary-coincident curves are
included; unsupported families raise explicitly. No sampled XYZ welding or
seed trace establishes topology. span_id groups multiple chart occurrences of
one physical generator; ordered components retain both source seam lifts.

CPU1/CPU24 and both P100s passed **17 direct tests per backend**, in two scopes:

| Run | Tests | Process seconds | Device errors / leaked bytes |
| --- | ---: | ---: | --- |
| CPU1 initial new+body/pair regression | 38, including 16 new; 1 GPU-only skip | 6.641 | Not a device run |
| CPU24 initial intersections | 16 | 3.362 | Not a device run |
| P1000 initial intersections, Compute Sanitizer | 16 | 4.953 | 0 / 0 |
| P1001 initial intersections, Compute Sanitizer | 16 | 7.547 | 0 / 0 |
| CPU1 additional consumer geometry classes | 1 | 1.447 | Not a device run |
| CPU24 additional consumer geometry classes | 1 | 1.130 | Not a device run |
| P1000 additional consumer classes, Compute Sanitizer | 1 | 1.551 | 0 / 0 |
| P1001 additional consumer classes, Compute Sanitizer | 1 | 2.024 | 0 / 0 |

The added method covers actual consumption of irrational x/y generators,
cardinal/tangent duplicated chart occurrences, height/trim points, source swaps,
maximum scale4 and origin16. It leaves the previous16 methods unchanged; those
completed checks were not repeated. Other direct cases include subnormal offsets
on both sides of carrier tangency, exact irrational trim endpoints, retained
internal trim tangency, lens/hole boundaries, unsupported/empty input, exact
rounding, preparation-budget refusal/recovery and actual cleanup/corruption.
Scoped lower budgets exercise failure paths; no claim that the production4096-
subdivision cap was exhausted. Existing native binaries/math were reused, so
unchanged native ASan/ABI/deep-cache campaigns were not repeated.

Independent truth reads original homogeneous controls/frames and raw scalar
fields. It proves whole-piece spatial/UV identities, exact closed domains and
ordered physical-span graphs. Literal configurations separately prove complete
counts/endpoints. Actual sample qualification binds the prepared request, reads
native descriptor/slot/UV buffers and independently grades both positions at
2^-36. Generic map validity alone is not an arbitrary-configuration component-
count oracle. Supported Python execution uses RN/ties-even and gradual underflow.

[Initial qualification](qualification.tsv),
[additional class qualification](qualification-sampling-classes.tsv),
[initial versions](qualified-initial-sources.tsv),
[additional versions](qualified-sampling-class-sources.tsv),
[environment](environment.tsv), [binary hashes](binaries.tsv).

All **five actual-production mutants** failed at direct assertions: omitted
component, ignored hole, split seam span, wrong source UV and reversed component
order. Baselines and preserved libraries stayed healthy; isolated trees/logs
remain private. [Mutation outcomes and exact source hashes](mutations.tsv).

## Actual callers

Four fresh CPU1/CPU24/P1000/P1001 callers cover one closed circle and four
rectangle-clipped arcs. Each consumes the complete four-bit chart-pair domain,
groups one face pair, constructs all supported face components and samples260
occurrence candidates/520 native rows. Exports retain exact coefficients/events/
request, original controls/frames, actual native source bytes, samples, masks,
resident boxes, code/binary identities and SVGs. Across callers: **1,040 paired
samples, 2,080 native rows, 400 map coefficients, 24 events and 48 polylines**.

Standalone serialized analysis reconstructed literal controls, whole maps,
sqrt7 endpoints and original source images. All actual source buffers, four
complete mask bits, conservative resident boxes and plotted coordinates agree.
Caller whole wall includes non-timing exports; final timing-file and console
writes are excluded. Pair/sample metric names are separate. Initial exports
were retained privately when the naming ambiguity was corrected; final exports
were regenerated by actual callers. Export/qualification overhead is outside
the repeated consumer benchmark below.

## Matched repeated consumer costs

Two recipes: radius1/4 section circle at z=0; the same circle clipped by a
square of half-width3/16 into four disjoint arcs. Both have four chart pieces.
At resolutions65/4097, each emits260/16,388 paired candidates and520/32,776
native rows. Sixteen serial fresh processes, initial independent qualification,
one excluded warmup, then five reused calls retaining result/request/Engine/
PatchSet. **80 reused calls, 960 raw phase rows, 320 summaries.** No other owned
copy/build/qualification/mutation/performance job overlapped the timed campaign.
The host is shared; these are descriptive medians and observed min/max ranges.

![All complete-consumer medians and ranges](matrix/costs.svg)

Large-case **external sample wall**, milliseconds; includes publication,
gather/upload, evaluation, source download, conversion/rows, exact grades and
child cleanup. Exact construction/request preparation are separate.

| Recipe | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| Circle, 16,388 candidates | 2298.9 [2124.4,2649.2] | 2325.8 [2087.3,2334.0] | 2336.6 [2103.2,2363.5] | 2320.8 [2149.6,2791.0] |
| Four arcs, 16,388 candidates | 2501.8 [2231.3,2549.7] | 2489.5 [2217.8,2520.7] | 2478.9 [2306.8,2554.3] | 2505.6 [2325.6,2552.8] |

Ranges overlap. **No complete-consumer gain is established.** World row/
reference construction takes roughly1.50–1.63s of the large medians and exact
grades another0.74–0.82s; source conversion/download and native work are much
smaller. CPU24 threads accelerate native evaluation, not these Python stages.

Large-case **native evaluation host span**, milliseconds:

| Recipe | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| Circle | 2.181 | 1.983 | 0.128 | 0.113 |
| Four arcs | 2.197 | 1.912 | 0.125 | 0.114 |

P100 versus CPU24 ratios are about **15.3–17.5× for this supplied-row evaluator
span**. Events are disabled; the span includes native allocation/launch/
synchronous evaluation and is not a kernel-only timer. It excludes query upload,
download and CPU consumer stages. It does not accelerate branch construction.
The tiny native batch favors CPU1 (about0.049ms versus0.064–0.073ms P100).
Tiny complete-consumer medians are33.2–38.6ms, again dominated by host work.

## Cold path and bytes

Exact large-case component construction is12.94–14.28ms for circles and
19.49–20.22ms for arcs. CPU preparation of all16,388 exact candidates is
26.76–28.09s and30.83–31.96s respectively. Cold-to-first-batch spans are
29.14–30.89s circles and33.40–34.91s arcs, including construction/request/
engine/geometry/first consumption. **Initial independent qualification is
separate** at roughly40.7–45.6s for large cases; it is outside reused timings
and cold-to-first-batch. Final engine cleanup is separately recorded: roughly
32–68ms GPU in this matrix versus0.01–0.02ms CPU, without attributing that
span to one lifecycle mechanism. Fresh process imports/CLI startup are outside
worker timing. Single cold observations are not repeated medians.

These results distinguish fast exact component construction from expensive
per-sample Fraction/Scalar/row work. They motivate a later owned bulk consumer,
not an active integration optimization lane. No sum of unrelated phase medians
is presented as wall latency.

| CUDA payload | 260 candidates | 16,388 candidates |
| --- | ---: | ---: |
| Geometry once | 976 B | 976 B |
| Query upload per call, 40*N | 10,400 B | 655,520 B |
| Position/status download per call, 56*N | 14,560 B | 917,728 B |

Geometry is896 control bytes+80 compact device descriptors. The680 source
descriptor bytes and40*N slot/UV bytes returned by native source APIs are
host metadata, not extra CUDA transfer. Topology upload is zero. The separate
actual caller also uploads240 pair-index bytes and downloads4 mask+240 input-
bound bytes. These movements are host/device, not server/network traffic.

[Raw iterations](matrix/raw.tsv), [summaries](matrix/summary.tsv),
[timed source versions](matrix/sources.tsv), [timed binaries](matrix/binaries.tsv).
All before/per-case/after hashes matched. The timed helper versions are at
checkpoint00455eb. Subsequent caller changes rename export metrics and refine
the SVG footer; their fresh manifests identify those versions. They do not
change the imported recipe/helpers or consumer behavior used by the matrix.

## Next broad theme and return pass

Continue to uncovered themes: planar arrangements/classification, GPU packed
adjacency and narrow interchange, followed by the remaining meshing/solid/
feature beachheads. General SSI, edited-body history and imported models are
outside this delivered family. Preserve
[round-two notes](../../papers/2026-10-14-intersection-round2-notes.md) and
[integration backlog](../../docs/RETURN_PASS.md). Bulk native parameter/UV/row
consumption and GPU lifetime/residency polish wait until after broad-pass big
rocks; this delivery does not start that tuning lane.
