# Arrangement performance runtime review

2026-10-14. Independently inspected saved TSV artifacts using only Python's
standard library. No product import, benchmark, test or mutation ran locally;
no measured source changed during this review.

The campaign has five recipes, 35 qualified fresh arrangements and 25 repeated
calls: first-after-imports, one excluded warmup and five repeats per recipe.
All 35 raw rows say PASS and each recipe's consumed reader digest is stable.
All 30 median/minimum/maximum summary rows exactly match recomputation from the
five repeated raw rows. In every raw row the four disjoint construction, graph,
geometry and location times sum to the complete wall within numeric precision.
The medians of separate phases need not sum to the median complete wall.

The largest grid has 1,153 vertices, 2,176 edges, 4,352 uses, 1,153 loops and
1,089 regions including exterior: 1,024 cells plus 64 disks are 1,088 bounded
regions. Complete median is 2,400.587 ms [2,327.950, 2,605.276]. Separate phase
medians are construction 471.046 ms, graph 9.069 ms, geometry 1,866.881 ms and
location 51.761 ms. Independent qualification outside that complete wall has
median 10,570.279 ms. The one-cell 64-hole recipe has 65 bounded regions;
complete median 2,091.441 ms [2,066.417, 2,288.753], with independent outside
qualification median 9,786.943 ms. These are exact CPU caller costs; there is
no GPU, complete CAD model or Boolean-solid performance comparison.

The geometry phase includes actual original boundary curve/coedge UV evaluation
and comparison at directed source parameters, arrangement world/UV readers,
and every retained circle piece's spatial and both source UV readers. Thus it
includes consumer-side correspondence checks, while the complete independent
source-domain/graph/location qualification is outside the complete wall. The
`edge_reader_queries` count names arrangement world/UV query pairs; additional
source-piece/map evaluations are real timed work but are not included in that
count. Export and SVG generation occur after the measured calls.

Each recipe retains prepared snapshots and exact sections for its seven fresh
arrangements. The first row is first after imports/preparation, not a fresh
process or complete preparation cost. For the 64-circle recipes total preparation
is 890.424 ms and 922.280 ms respectively; the exact-section portions are
870.681 ms and 898.272 ms. Prepared-source reuse is explicit. Qualification and
export do not disappear from the workflow merely because the complete caller
timer excludes them.

All 25 exported graph-table extents match the five counts rows; the five saved
location tables match their named query counts. Across the 35 actual calls the
caller issued 16,975 location queries and 53,172 arrangement world/UV query
pairs, in addition to the retained source-piece evaluations. The 11 saved
source hashes match the current actual files. Full source maps, controls,
events and exact graph tables accompany the large outputs, rather than only
aggregate topology counts.

All eight saved focused mutations have a healthy exit0 baseline, an exit1
ASSERTION_KILLED row and a healthy exit0 retry. The five protected native binary
before/after hashes are identical. The reviewed controller admits completed
failures-only assertion outcomes and excludes errors/crashes; the saved campaign
rows agree with that scoped claim. These eight sites describe named fault
classes rather than exhaustive mutation coverage.

Final report review: accepted `results/arrangements-2026-10-14/REPORT.md` after
publication. Its recipe and phase tables agree with the saved observations;
first-call numbers and aggregate final export counts also match independent
recomputation (1,168 regions including five exteriors, 4,536 directed uses).
It distinguishes complete retained-input callers, original source preparation,
inside-reader correspondence checks and outside independent qualification;
retains all five full final graphs; and scopes the first calls, display sampling,
mutation evidence and absent GPU/Boolean comparison accurately. The closed cell
removes open disk interiors and shares the circle boundary with its closed disk
region, matching all-incident lookup semantics. No remaining finding. No tuning
or integration lane is opened by this performance inspection.
