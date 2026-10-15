# Boolean CPU unit artifacts: geometry and provenance review

Disposition: **accepted**, no finding. Read-only local analysis of saved
`results/boolean-2026-10-15/caller-unit` and `reference-unit`, using only standard
library TSV/tree/binary/XML readers. No product module, geometry constructor,
native library or reference runtime was imported or executed locally. This is
the CPU unit baseline; the forthcoming cost campaign is a separate review.

Decoded every retained tree row, with duplicate-path and complete tuple-field
consumption checks: graph 2,563 rows, forward history 1,231 rows and reverse
history 2,021 rows. The operand and result graph contains complete V/E/use/loop/
face/chart records; carrier controls, original UV, frame and Lineage are nested
under every chart rather than omitted. All ten output carriers retain source
control/UV/origin/scale bits: six box frames at scale 1 and four cutter frames
at scale 2, with fresh result lineage. The outer vertices, curves and edges
retain their actual box records. New circles have box-local radius 1/2 and
heights ±1/2; they are not the canonical plate's radius-1/4 recipe.

All 85 forward sources and 81 reverse origins are distinct typed identities,
with inverse images/generated-parent links agreeing. The 58 retained, 13
modified, 14 deleted and 10 generated counts match the saved maps, including
cropped seam/uses with intrinsic source range [1/4,3/4]. Both actual source cap/
wall sections retain eight total spatial/bothUV quarter pieces and eight named
events. The two serialized arrangements retain their exact sections, cell/disk/
exterior regions, complete halfedges and annulus area `1-pi/9` in original cap
UV. Thirteen selections retain seven boundary pieces and explicit discarded
regions/bands. Source namespace 1/2 references and output namespace 3 are
consistent with the actual recipe.

Read all native little-endian arrays. Sixteen offsets cover thirty members,
each coedge appears once, all mates are involutions with opposite sign, and every
neighbor is its actual mate's face. All fifteen public edge-use and thirty mate/
neighbor TSV rows agree with those bytes and the saved result graph. Fourteen
edges are ordinary opposed pairs; the cropped seam is one same-face pair.

The native sample table contains 2,890 rows, 2,712 admitted finite rows and 178
rejected rows with NaN geometry. Each carrier has 289 actual query rows; wall
queries use the original source V range [1/4,3/4]. Saved exact material probes
retain the hole boundary, cap/rim joint support and outer-corner support rather
than interpreting closed tool membership as simple negation.

Parsed the actual SVG as XML. Its fifteen wire paths correspond to twelve
original box lines, two generated cyan rims and one gold cropped seam. Recomputed
every path coordinate from saved curve records and the declared display
projection. All 1,356 visible sample circle centers and face colors agree with
saved admitted native positions/normals, including display ordering, to the
SVG's three-decimal precision. The captions accurately call this a boundary
wireframe plus native samples and explicitly say there is no result tessellation;
the image does not establish an opaque surface mesh or GPU construction.

All caller source hashes and reference-linked recipe/quality/edge/sample hashes
match the actual local source/artifacts. The native core hash agrees with the
previous interchange delivery. The optional usd-brep 1.0.0 experiment contains
146 actual boundary comparisons: ninety edge samples and fifty-six native face-
interior samples, eight per face. Saved maxima agree with individual rows:
distance/residual `2.7336071744532853e-16`, volume absolute difference
`4.0125125444490095e-10`. These meet its declared bounds. The reference table
explicitly does not claim material membership, source-history comparison or
external alpha qualification. It operated on copies; the source script also
checks unchanged original input volumes.

No CPU/P100 timing comparison or modeling speedup follows from this single
baseline. Independent qualification and artifact writing have separate cost
rows. The complete outer caller wall includes construction return/publication
and both actual consumers; inner construction timing is a nested diagnostic.
