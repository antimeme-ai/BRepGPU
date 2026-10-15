# Arrangement design: semantic review

2026-10-14. Reviewed `docs/ARRANGEMENT_DESIGN.md` against the bounded source
study and existing complete circle sections. Accepted with no semantic blocker.
Sent the verdict before writing this disposition. No implementation/execution.

The admitted family is complete and narrow: exact axial grid, full circles
strictly inside one cell, pairwise separated disks. Nested/tangent/coincident/
clipped and multi-component sections reject explicitly. Bounded iterable intake
and atomic publication keep unsupported data from silently truncating output.

One closed circle edge/anchor preserves all four original section maps/events
and opposite disk/hole uses. Chart breakpoints are not feature edges. Circle
source indices and grid keys are local to the retained immutable arrangement;
source snapshot/face/section context distinguishes namespaces. Source operand
reversal selects the appropriate original target UV without reversing the
declared positive UV loop topology. Parent face orientation remains metadata.

Original boundary records name both original coedge and underlying edge with
increasing canonical edge-parameter intervals. The arrangement's positive U/V
traversal can oppose that original direction; qualification must derive the
endpoint correspondence from actual source curves/pcurves, not assume the
coedge's slot or traversal sign already equals arrangement direction. The
design explicitly makes that separation.

Exterior twins form a full closed DCEL, including the reversed target boundary
with every cut endpoint split. Cells have CCW outer cycles and CW disk holes;
each disk has its opposite CCW cycle. Self-loop circle anchors legitimately
count twice in edge incidence. With m disconnected circles, graph Euler is
`V−E+F=m+2`, and the hole-aware bounded contribution remains one for the target.
The proposed counts and loop ownership agree with these facts.

Point location returns all CLOSED incident regions: a target corner includes
exterior plus its cell; a boundary cut endpoint includes exterior plus two cells;
an internal cut crossing includes four cells; a circle boundary includes disk
plus containing cell. Strict interior witnesses do not use a possibly holed
cell midpoint. Choosing U before all contained disks' left footprints is exact
and leaves V free at its interior midpoint. Exterior witness(-1,-1) is outside
the normalized target. These values provide an actual region reader, separate
from future Boolean material selection.

Area coefficients encode normalized-UV area exactly as rational plus rational
times pi. Owned horizontal rectangular source charts are isotropic squares, so
world circles remain UV circles. If implementation accepts more general affine
rectangles, it must check equal axis scales before using one UV radius; ellipses
are outside this design. For the declared factory family, strict containment
also proves positive cell remainder without a floating pi comparison.

Independent complete region bijection, exact boundary/cut interval coverage,
circle map coverage, twin/loop checks, source order/frame cases, exact witnesses
and area cancellation attack the relevant distinct faults. No broader overlay,
native/GPU placement or deferred Python-role investigation is required. Ready
for plan review.
