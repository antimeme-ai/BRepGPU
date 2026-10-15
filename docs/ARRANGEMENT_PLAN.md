# Deliver exact regions and shared uses

The three design tribunals accept the complete bounded arrangement design.
Integrated arithmetic clarifications: a circle-bearing target has equal
nonzero absolute U/V spans and matching derived radii; boundary sources carry
oriented original curve parameters t0/t1, possibly decreasing. Canonical edge
parameter maps directly by(1-t)*t0+t*t1; do not apply source coedge orientation
again. This written whole-unit plan is reviewed before implementation.

1. Publish studies, accepted design and this plan on feat/planar-arrangements,
   issue27/draftPR28. Adversarial semantic/runtime/oracle plan review follows.
2. Root writes a direct first test: existing exact intersection/classification
   APIs first establish a healthy full circle and interior/boundary witness,
   then call absent BodySnapshot.arrange_face. Run on the fleet and record the
   intended missing-entry red before product or oracle implementation.
3. One owner implements the whole product in arrangements.py: bounded exact
   input/section validation, actual source rectangle/boundary/circle geometry,
   semantic event identity, unique line/full-circle edges, opposite uses,
   complete ordered loops/regions/exterior, hole association and exact area/
   witnesses. Same owner supplies exact edge point/UV and all-incident region
   readers. Root adds only the lazy BodySnapshot adapter. No source invariant
   is split among owners, no dependency/native/GPU/lifetime change.
4. Independent owner implements arrangement_oracles.py and direct tests:
   enumerate exact source cell-minus-disks/disk region domains, prove bijection,
   complete directed edge/loop boundary words and original curve/map parameter
   coverage, witnesses/area, incidence/reader results. Do not call product
   rectangle/circle/classification construction helpers for expected output.
   Source intersection oracle may qualify retained original maps, while the new
   model independently derives circle center/radius/orientation and topology.
   Check bare rectangle, line-only grids on every original rectangular face,
   degree4 crossing/degree3 cut-boundary, simple one-circle cap, multiple holes
   in one cell, mixed cells, source operand reversal and bottom face orientation,
   frame translations/scales, exact subnormal/one-ULP admission boundaries,
   immutable sources/retained results, invalid/capped/failed iterators, wrong
   source face/revision/body and all unsupported circle contact families.
5. Root integrates/direct greens and existing classifier/body/pair/section/
   adjacency compatibility on the fleet. Tribunal reviews complete product,
   independent oracle/tests and real caller; fix every valid finding. No old
   native sanitizer rerun solely because unchanged binaries are used elsewhere.
6. Root runs at least seven focused actual production mutants in isolated fleet
   copies, each a named geometry/topology claim: missing disk/region, wrong hole
   parent/orientation, bad twin/next, wrong source-edge parameter direction,
   rounded/incorrect separation, lost circle source side or incomplete closed
   boundary point-location. Require actual geometry AssertionError, unittest
   exit1 and completed failures-only summary; healthy baseline/retry per site,
   no pycache reuse or import/crash credit. Preserve protected native libraries.
7. Root owns whole actual caller/cost unit in examples/arrange_face.py. Five
   recipes: bare rectangle; one-circle cap; mixed2×2 cells with3circles including
   two in one cell;64disjoint circles in one cell;32×32 grid with64circles.
   Prepare actual snapshots/complete exact sections once. The larger recipes
   use a box cap with scale4 at z0 and cylinder sections of scale1/4,64centers
   at(-3+6*(4i+1/2)/32,-3+6*(4j+1/2)/32), i,j=0..7. Each closed disk fits one
  32×32 cell strictly and is separated from every other disk.
   One first call after imports, one excluded warmup, five repeats; fresh owned
   arrangements each call with retained prepared inputs. Complete wall includes
   construction, actual full table/loop traversal, source edge geometry/UV
   consumption and exact point-location queries. Attribute construction and
   each actual reader phase separately. Preparation includes exact section
   construction. Independent whole-output/input-correspondence qualification
   occurs afterward with explicit costs; no owned sync/test/mutant overlaps
   measured workers. Pin actual product/source dependencies before/after.
   Export small/mixed exact graph tables, all typed source records/maps/events,
   loop words, regions/areas/witnesses and query results to TSV. Large output
   may retain aggregates/counts with reproducible inputs, explicitly scoped.
   SVG reads actual graph/region/curve output, with distinct circle hole/disk
   and shared crossing views. Independent saved artifact/statistics review.
8. Publish API/results with CPU-only/no-Boolean performance limits, bibliography,
   retained reviews, README graphics, roadmap/theme status and round-two notes.
   Preserve all substantive commits and branch when mergingPR28, closeissue27,
   sync clean main to the owned fleet checkout without rebuilding protected
   libraries. Next uncovered themes include trim-aware meshing and interchange;
   tangent/clipped arrangements and integration/Python decisions stay later.

Source/ref reading and static AST/artifact review are allowed locally; all
product execution, tests, mutations and measurements run on the fleet. Scope
is a complete correct beachhead, not a general arrangement library.
