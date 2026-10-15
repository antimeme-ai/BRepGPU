# Actual Boolean-result mesh artifact and cost review

Accepted the saved caller/benchmark artifacts from results/cut-mesh-2026-10-15
against the stable measured source. No open findings. This caller-owner review
checks publication and measurement correspondence; independent mesh geometry is
established by the separate fleet qualifier and semantic review, not this review.
No product geometry was executed locally; static TSV/hash inspection only.

Recomputed all20 summary rows directly from28 saved first/warmup/repeat rows:
four independently qualified first outputs, four excluded warmups and20 measured
repeats. Every row is PASS. Complete medians [min,max] are37.965[31.318,38.170],
220.970[210.892,237.657],433.318[408.434,704.553] and
234.100[231.019,246.736]ms. The translated case has a visible665.329ms
construction outlier and704.553ms complete maximum; retaining its range is
appropriate. Separate phase medians are not a summed complete median.

Counts match saved vertex/triangle inventories and the declared shell resource
relations: n1/32/64/32, vertices16/264/520/264, triangles32/528/1040/528,
fifteen physical edges and thirty distinct coedge uses for every case. Saved
qualification reports seven faces and genus one. Exact curve plus actual rounded
bounds equal reported total bounds and remain within the submitted exact world
tolerances. Both benchmark and standalone source pins match current measured
source hashes.

Inspected every saved case's wall corners and boundary uses: raw V is1/4 and3/4;
the two physical-seam occurrences retain atlas U0/4 and intrinsic result-edge
parameters0/1. Result carrier controls and frame bits match their actual original
box charts0..5 and cutter charts2..5 exactly for every case. Actual output wall
patch scales equal cutter scales, twice the topological box frame. No UV
renormalization, canonical plate, carrier refit or packed CutSolid appears.
Each retained history has85 input dispositions,81 output origins,14 deleted
sources and10 generated targets. Original cropped-seam ranges and source UV
bands remain in the actual reverse-origin tables.

Viewed assets/cut-meshing/through-bore.png produced from the actual saved fine
mesh SVG. The left panel draws actual top triangles with an open hole; the right
panel draws the complete indexed wire geometry and visible wall strips. Counts,
world tolerance, error bound and result-face/coedge labels are legible; no
hidden-line claim is made. Original and result namespaces remain distinct in the
named source exports.

Useful walls include mesh construction, indexed rendering buffers and complete
typed history lookups. First independent qualification441.239/886.859/
1503.973/930.045ms lies outside those walls, as do operand preparation,
CPU Boolean construction, repeat correspondence and filesystem export. Separate
standalone observation directly measures254.171ms prepared-result consumption,
853.318ms independent qualification and1220.573ms preparation-through-export.
Those overlapping scopes must not be summed. No native/GPU meshing or transfer
occurs; no whole-CAD or GPU speedup follows.

Final publication review: accepted REPORT.md, CUT_MESHING API page, README
connection section and delivery journal against these saved artifacts. Numbers,
CPU-only scope and exclusions match. Prose retains the translated maximum and
states no old/new baseline or GPU comparison. It accurately distinguishes the
archived canonical SVG's three historical caption positions from unchanged
geometry and preserved current defaults. No scope, dependency or alpha claim
was widened. No measured source edits were made during this review.
