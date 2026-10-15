# Exact mesh caller result review

2026-10-15. Artifact/source inspection using standard-library CSV, Fraction,
statistics, hashes and XML only. No product import, test, mesher or benchmark ran
locally. Reviewed `results/meshing-2026-10-15/caller` and actual source scopes.

The four cases contain28 PASS calls: first, excluded warmup and five repeats
each. Exactly four first outputs receive independent whole-domain qualification;
the24 later complete frozen outputs and actual indexed buffers are compared
directly with those first qualified values outside complete wall. The saved full
graphs are those FIRST qualified outputs. This is not28 independent oracle
runs, and the report must keep that distinction.

All16 summary rows exactly match independent median/minimum/maximum
recomputation from the20 repeated rows. Each raw construction plus indexed-reader
time equals its directly measured complete wall. CPU complete medians [min,max]
are box7.228ms [7.009,7.680], cylinder88.216ms [86.386,108.176], plate102.886ms
[98.771,154.283], and fine translated/scaled plate757.459ms [751.416,771.074].
Fine-plate construction and indexed-reader medians are722.285ms and38.534ms;
independent first qualification is a single2,182.195ms observation, and median
outside repeat correspondence is15.790ms. Separate medians need not sum.

Body preparation and filesystem export are four separate single observations;
fine-plate values are0.909ms and120.123ms. First-after-import/prepared-body
complete wall is761.381ms for that case. These are exact CPU construction and
consumer costs with no native calls, device transfers, CPU24 comparison or GPU
speedup ranking. Independent checking and filesystem IO are not hidden inside
the complete construction-plus-reader definition.

Counts match all saved tables and original input budgets: box8vertices/12triangles;
cylinder n16,130/256; plate n16,136/272; fine-frame plate n128,1032/2064. The four
first qualified full exports total1,306 vertices and2,604 triangles. Every
triangle has three exported chart-local corner records. Actual vertex-rounding
L1 maxima recompute exactly from Fraction positions and binary64 hex values;
curve_bound+rounding_bound equals the saved total and stays below each requested
tolerance. The largest rounding bound is4.406e-16 world units; it is separately
visible, not silently absorbed into the exact construction claim.

All four PLYs preserve every saved shared rounded vertex, triangle index and
original face ID; emitted facet normals are finite unit vectors. Original packed
body-table sizes match actual files. Saved source tables retain original curves,
maps, controls, faces/charts and both seam occurrences. All3,254 saved SVG
polygons match the actual exported coordinate/index records in their declared
top/wire projection. Plate top-face triangle polygons leave the central hole
unfilled in both plate cases; the wire view explicitly has no hidden-line
removal. The cap approximation caveat correctly states that inscribed hole
chords include a narrow strip inside the exact analytic hole.

Qualification records show11 direct methods passed in2.995s and83 affected
methods in15.542s:82passes and one existing GPU-only skip. These local/source-
family checks do not satisfy external alpha qualification. No claim of external
community green or broad mutation coverage accompanies these artifacts.

The benchmark source pin for `examples/mesh_body.py` is
`cc4d08826d78667349e0578f6e2e02f821f5322a1dc183991d731207917b1124`, exactly
the source at commit `b68875f`. Eight other saved source pins still match actual
files. Post-campaign commit `ebbef8f` moves only SVG caption baselines
555→570,585→600,615→632; its caller hash is
`dedb7f68de7db3d5f71fb292f5d4b5a8991435208b05aae3cd2e49ba8451ceb6`.
The raw measured SVG/source pin must remain intact. A later gallery/standalone
export should name its revised source separately. This display-only spacing
change does not justify rerunning the cost matrix or kernel tests.

The subsequent standalone run is also accepted. All nine source pins match the
post-caption source. Its actual clocks are preparation0.919ms,
construction122.451ms, reader4.856ms, complete construction+reader127.307ms,
independent checking602.817ms and filesystem export20.366ms. The directly wrapped
preparation-through-checking/export wall is751.409ms; disjoint phases sum to it.
This is one standalone observation, not another repeated performance comparison.

Standalone vertices/triangles/corners/polylines/coedges/bounds/counts/qualification
and PLY are byte-identical to the first qualified ordinary plate benchmark
geometry. Its SVG differs from the original measured plate SVG only at the three
caption baselines, and is byte-identical to `assets/meshing/plate.svg`. The PNG
has1740x975 dimensions. Visual inspection confirms an open top-cap hole, actual
indexed all-face wire geometry, visible inner wall, separated caption baselines
and no misleading hidden-line/solid-render claim. Raw measured SVG and its
original source pin remain unchanged. No remaining source/artifact/statistical
finding. Final source review accepts `results/meshing-2026-10-15/REPORT.md` and
`docs/MESHING.md`: all numeric tables/observations agree with saved records;
four independent first qualifications and24 correspondences,83/82/1 test scope,
first-call versus process-cold distinction, both caller revisions, standalone
751.409ms wall and CPU-only/approximate-trim limits are stated accurately.
The report explicitly leaves inspected external cases NOT_RUN and does not
promote selected fault witnesses or canonical models to external alpha green.
No remaining report/API wording finding; accepted for publication.
