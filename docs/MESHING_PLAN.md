# Trim-aware meshing delivery plan

Issue32. Execute the accepted MESHING_DESIGN on a feature branch with meaningful
research, red-test, implementation, qualification and publication commits.
The unit is the complete exact CPU constructor and its budgeted shared binary64
output for existing canonical bodies. General triangulation/GPU placement and
external community qualification are not implicit successes of this delivery.

## 1 — Independent oracle and direct red

Write python/brepgpu/meshing_oracles.py plus tests/test_meshing.py and
 tests/test_meshing_entry.py. Tests must import the future meshing API directly;
record the genuine fleet failure while that module/adapter is absent. Do not
count a broken test syntax/import of unrelated dependencies as an oracle kill.
No product geometry executes locally. Static AST/source checks are allowed.

The qualifier owns whole-domain claims: source binding, typed generators,
original edge/coedge occurrence words, exact point/UV correspondence, chord
minor-arc/dot-product inequalities and actual rounding budget; every full plane
and wall chord-strip positive integer boundary chain; closed opposite mesh-edge
uses, cyclic vertex links and connected shell; exact and rounded embedding.
Never call the product mesher/refiner/readers to produce expected answers.
Reconstruct source boundary domains independently from canonical source facts.
Wall strip chains use actual affine chord lambda rather than rational arc s.
Preserve integer multiplicities and occurrence geometry across seam/chart cuts.
Check complete convexity/order of rounded rings (e.g strict angular quadrant
monotonic order AND all strictlypositive turns), not local turns alone.

Direct tests cover all shapes, n1/coarse and multiple refinements, every frame
scale, translated/extreme/subnormal origins, threshold equality and just below,
input admission/budget failure, immutable retained results/source binding and
body adapter. Include adversarial output corruption for missing/duplicate/folded
triangles, filled-hole triangles, split shared rim IDs, wrong seam UV/chart and
lost original source parameter. Such cases exercise the real geometric qualifier,
not a test-about-test metric. A small selected production fault campaign may
exercise distinct assertions after healthy tests; report only named sensitivity.

## 2 — Owned constructor

Implement python/brepgpu/meshing.py to the exact record schema in the design and
add the lazy BodySnapshot.tessellate adapter. Enforce tolerance/budget before
large allocation, choose dyadic n by exact R/(2n²)<=tolerance/2, and predict
bounded output sizes. Read original records with owned Fraction arithmetic.
Allocate original vertices and physical edge samples once. Preserve closure
parameter occurrences, original boundary maps and local chart corners.

Construct complete oriented rectangle, cap fan, plate quarter-corner/side fans
and cylindrical chord-strip templates. Do not discard triangles or smooth source
attributes. Compute actual shared rounded coordinates and exact L1 perturbation;
reject insufficient total budget or any rounded degeneracy/orientation loss.
Check rounded canonical ring order/convexity/nesting, paired heights/XY and
planarity, the cheap bounded prerequisites used by the construction proof.
Factory publishes recursively immutable results only after all steps succeed.
Existing native core/plugin binaries stay unchanged.

Run direct green on the fleet. Independent semantics/code/oracle review follows;
resolve every valid finding and rerun the affected direct tests. Run compatibility
once after meaningful product changes, using the qualified existing core.

## 3 — Actual consumer and cost cases

Implement examples/mesh_body.py: construct, follow every triangle/corner and
physical-edge/coedge record, consume shared rounded XYZ and bounds, qualify the
actual output separately, and write triangle PLY, SVG, original body/control
records and exact/rounded/source mesh tables. Full original parameters, both seam
occurrences and output bounds remain inspectable. Use no new dependencies.

Implement a serial bounded cost matrix over box/cylinder/plate and a finer
translated/scaled plate. One first observation, one excluded warmup and five
repeated complete construct+indexed-reader calls suffice. Time source preparation,
construction and actual returned-mesh reading separately, but also wrap complete
caller wall directly. Qualification and actual filesystem export each get
separate observed times. Do not sum separate medians or repeat independent
qualification inside the timed operation. Retain actual complete outputs and
source hashes. This CPU-only operation has no device-transfer costs or GPU
performance ranking. No perf-worker overlap with tests or fault campaigns.

## 4 — Review and publish

Inspect original saved mesh records and actual rendered triangle views, including
the plate through-hole and seam; verify captions/bounds and source identity.
Summarize whole-domain/chord/topology evidence first. Record applicable upstream
OCCT meshing cases/assertions and missing external runner/adapters as alpha debt;
selected faults are supporting detail, never community qualification.

Update API/roadmap/AGENTS/research index/issue32 and return-pass notes. Publish a
README mesh graphic, direct commands and actual cost/geometry report. Push PR,
preserve substantive commits in a merge commit, retain branch, synchronize owned
fleet checkout and verify clean source status/protected native hashes. Proceed
to the next uncovered theme after this complete bounded unit, without starting
setup/residency or Python architecture work.
