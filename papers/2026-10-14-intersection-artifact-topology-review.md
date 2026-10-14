# Serialized intersection caller review

2026-10-14. Geometry, native-buffer and SVG checks passed for the four retained
callers in `results/intersections-2026-10-14/caller`: `cpu1-circle`,
`cpu24-clipped`, `p100-0-clipped` and `p100-1-circle`. Accepted after fresh
caller regeneration resolved the quality-metric naming finding. This review used standalone
standard-library CSV, Fraction, integer-square-root, struct and XML analysis.
No product module was imported or run; no native/fleet/performance execution.

Each caller contains 260 prepared samples, 520 actual native rows, five source
charts and 28 original homogeneous controls. Totals are 1,040 samples, 2,080
native rows, 400 exported map coefficients and 24 events. Each native descriptor
buffer has five 136-byte records, totaling 680 bytes. Dense source order is
left slot 0 followed by right slots 2, 3, 4 and 5.

Literal controls were reconstructed independently: the box top is the unit-
weight bilinear square at local z=1/2; its frame puts the world plane at z=0.
Four wall charts rotate the homogeneous quarter-circle net with weights 1,1,2,
extruded over local z=-1/2..1/2. Every exported control equals that literal net.
Every descriptor equals the original degree, compact control offset/count,
typed body/face/chart/revision, chart rectangle and frame. Binary slots and UVs
equal their actual sample CSV values for every row.

The exact quarter section is
`x=(1-t²)/(4(1+t²)), y=t/(2(1+t²)), z=0`, rotated by quadrant.
Plane UV is `1/2+(x,y)/(3*scale/2)`; wall local UV is `(t,1/2)`.
Exported maps have degree at most two. Seven distinct rational substitutions
verify each map against these expressions: after cross multiplication the
polynomial degree is at most four, so these exact equalities establish the
whole coefficient identity. Requests and serialized references also match the
literal expressions at every rational or quadratic-algebraic parameter.
Radicals were interpreted independently in the basis `1,sqrt(7)`; d=28 aliases
become twice sqrt(7). Outward integer-square-root intervals establish rounding
and error comparisons without production scalar arithmetic.

Circle callers retain four full quarters, one ordered closed component, four
spans and four atlas events, including both lifts at 0/4. Clipped callers retain
four separate open arcs, four spans and eight exact rectangle-boundary events.
All clipped endpoints equal `sqrt(7)/7` and `(4-sqrt(7))/3`. Each piece has its
actual endpoint samples and 63 strictly increasing rational interior samples.
Component/span IDs, exact parameters and locations match request/sample rows.
No tangent-carrier or boundary-coincident label is falsely attached.

Actual positions were compared both to the literal exact requested point and
to an independent Fraction evaluation of the original homogeneous net at the
actual rounded UV, including its frame. All meet the stated 2^-36 budget. The
largest observed actual-position difference from that exact rounded-UV world
image is approximately 1.38778e-16; this is a descriptive maximum, not an
additional acceptance allowance.

All five downloaded pair boxes enclose every original Cartesian control after
the exact frame transform. Every plane/wall-chart box pair overlaps. All four
mask bits are set, the unused 28 bits are zero, and page metadata reports the
complete [0,4) domain with flags 3. Each SVG has twelve polylines: four pieces
in each of world XY, plane UV and wall atlas UV. Every plotted coordinate string
matches the exported actual position/UV at the renderer's three-decimal format;
no line joins separate clipped components.

The four ten-source manifests were identical and matched source bytes at the
initial check; fresh regenerated manifests also match current source bytes.
Their two binary manifests agree (core prefix `b63af7f6d64a`, CUDA plugin prefix
`4aeb0d7dd124`). TSVs contain no absolute workspace paths. Artifact inspection
does not independently establish cleanup, execution-device behavior or arbitrary
configuration completeness; direct tests and the admitted analytic family cover
those separate claims.

## IA1 — quality metric collision, resolved

`quality.tsv` combines pair and sample statistics under the name `count`.
Sample qualification replaces pair count 4 with sample count 260, while pair
`start`, `next_start` and `word_count` retain their meanings. Dedicated
`pair-page.tsv` and `input.tsv` were correct, and geometry was unaffected. Root
namespaced pair/sample metrics and regenerated all four actual caller exports
after the serial benchmark. The fresh `pair_count=4`, `sample_count=260` and
`sample_evaluated_count=520` are distinct; keys are unique and the ambiguous
unqualified count is absent. Other namespaced values equal the original values.

Seventeen geometry/input/native/pair/binary files per caller are byte-identical
to the independently checked initial outputs. All plotted polyline attributes
are unchanged. The clipped footer now describes its planar trim endpoints;
circle footers describe the actual seam lifts. Only the caller source changed
among the ten source hashes, and every fresh hash matches its current file.
Timing observations are fresh; no initial timings were treated as new runs.

Read `docs/INTERSECTIONS.md` and the result report against delivered source and
artifacts. Their named-face, axis-aligned analytic family, closed trim inclusion,
span/occurrence distinction and exact-CPU/GPU-evaluation split are accurate.
The admitted family is complete; generic oracle validity alone is correctly
qualified as insufficient for unknown-configuration component counts. The
report correctly avoids a GPU construction or complete-consumer speedup claim.
The retained matrix has sixteen conditions, 960 raw phase rows (800 reused and
160 single observations) and 320 summaries; all summary counts/medians/ranges
match the retained raw condition keys. No remaining artifact or topology-scope
blocker. Source and retained results were not edited by this reviewer.
