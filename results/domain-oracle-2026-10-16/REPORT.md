# Independent original-domain reader

Complete438-face mesh qualification reached its300s bound; a bounded stack
sample located segment-pair enumeration in the original exact domain checker.
Its candidate search
now uses independently owned exact coordinate ranks and an adaptive-axis closed
box sweep. Original Fraction area, intersection, adjacency and hole-containment
semantics remain unchanged. It consumes neither producer validity flags nor the
producer's BVH.

The work-property test first failed against the actual old checker: 590,566
Fraction comparisons for a valid 512-site strip, above the 32,768 bound. Six
semantic test methods passed on that old checker before implementation. All
eight focused methods then passed with the new sweep; all 80 affected domain,
CDT, external-reference and imported-mesh methods passed in 18.675s. Plan and
code received independent numerical review before the new complete-part replay.

Two already captured original domains completed independent input admission:

| Original sites | Admission | Sweep axis | Active visits | Peak active | Process peak KiB |
| --- | --- | --- | --- | --- | --- |
| 1,114 | .040738s | X | 2,233 | 4 | 27,940 |
| 8,300 | .274259s | Y | 20,737 | 4 | 32,572 |

Both have one loop. All closed-box candidates were immediate or closing source
neighbors, so the historical exemption removed them before nonadjacent exact
intersection tests. Complete signed-area/collapse/domain admission still ran.
These are original-domain admission times, not complete mesh qualification or
triangulation times. No old/new speedup ratio is inferred from the censored
whole-part qualification.

Runs used one fresh process per captured input, CPU4 on the shared Xeon
E5-2697v2 host, under a 30s process deadline. Other project work remained on the
host, so these are observations rather than robust timing statistics. Source
pins and machine-readable outcomes accompany this report. Private source
coordinates and acquired snapshots are excluded.

The sweep visits the smaller projection-overlap count after O(N log N)
preprocessing. Dense overlapping projections can still make the active scan
quadratic. Closed-touch rejection, equal coordinates, zero-width boxes,
original source ordering and rotated inputs are tested explicitly; these two
captures do not establish a universal complexity bound.
