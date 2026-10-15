# Arrangement saved geometry and result semantics review

2026-10-14. **Saved caller geometry accepted.** Reviewed the actual five-case
`results/arrangements-2026-10-14/caller` TSV/SVG artifacts using standalone
standard-library parsing and rational arithmetic. No product imports, local
product execution or measured source changes occurred. The final report was
subsequently reviewed and accepted as recorded below.

## Actual SVG geometry

Independently evaluated exported target-side UV numerator/denominator
polynomials for every circle sample, resolving the target operand from retained
body/revision/face identity. Line samples came from exported exact endpoint UVs.
Applied the documented panel transform and three-decimal display formatting.
Every one of the 2,268 actual edge polyline coordinate strings agrees exactly.
Circle strokes are distinguished from grid strokes.

Reconstructed every bounded region path from its exported outer loop, complete
hole list, ordered use IDs and use orientations. All 1,163 SVG region path
strings agree exactly, including reversed circle hole traversals and every
even-odd hole subpath. The one-cell 64-hole case retains all 64 disk holes; the
32×32 case retains its 1,024 cell regions and 64 disk regions. The largest
artifact contains 1,153 vertices, 2,176 edges, 4,352 opposed uses, 1,153 loops and
1,089 regions including the exterior. SVG count labels agree with the tables.

The SVG accurately labels CPU construction/readers, shared twins, hole winding
and retained exterior topology. It explicitly states that display samples do
not establish a mesh bound. A circle remains one closed physical edge with one
anchor; drawing its sampled source pieces does not change that identity. This
is a geometric and source-path review of the saved SVG, not an additional
product execution or a renderer screenshot review.

## Costs and provenance

All 35 saved outputs are marked PASS: first call, excluded warmup and five
repeats for each of five cases. All 30 median/minimum/maximum rows reproduce
exactly from the 25 timed repeats. Each raw complete-wall measurement equals
its construct, graph, geometry and location phase sum within floating timing
roundoff. All recorded measured-source hashes match the reviewed files.

The largest grid's median construction is 471.046 ms and complete caller wall
is 2,400.587 ms; geometry consumption alone is 1,866.881 ms. Its independent
qualification median is 10,570.279 ms outside that caller wall. The 64-hole
single-cell case has median construction 397.822 ms, geometry consumption
1,595.346 ms and complete wall 2,091.441 ms. Preparation occurs once before
these calls: grid32 records 898.272 ms for exact source sections and 922.280 ms
for complete preparation. Construction and reuse of existing source sections
must therefore remain distinct timing scopes.

The geometry consumer reads actual curve positions, both surface maps and
original-boundary correspondence; those correspondence grades occur inside
its timed phase. Independent full-arrangement/query qualification follows the
complete wall. Export and SVG writing follow the measurements. This is exact
CPU subdivision of one face with bounded interior disks, not arbitrary circle
overlay, assembled material selection, meshing or GPU construction. No GPU
performance gain or conclusion about the eventual shipping language follows.

## Final report disposition

Reviewed `results/arrangements-2026-10-14/REPORT.md` against the saved artifacts
and accepted API. **Accepted, with no remaining finding.** It correctly defines
closed cell domains by removing open disk interiors and retaining the shared
circle boundaries. Disk regions remain emitted domains, so neither the gold
circles nor the hole loops imply material removal or an assembled Boolean solid.
Whole circles retain one physical edge/anchor and complete original piece,
chart, map and event provenance.

The recipe counts, summed final exports (1,168 regions including five exteriors,
4,536 uses), timing medians/ranges, preparation and first-call numbers agree
with the saved tables. Every relative report link resolves. The report clearly
distinguishes 35 qualified in-memory trial outputs from five fully exported
final outputs, and retains full geometry for the large case. It separates
imports, source preparation, timed source-correspondence checks, complete
caller wall, independent qualification, export and display generation. It
attributes the expensive exact geometry reader without claiming a comparative
speedup or diagnosing its language cause. Native/GPU construction, arbitrary
overlay, meshing, Boolean assembly and the deferred Python architecture
decision retain their declared boundaries. No measured source change was
needed for this acceptance.
