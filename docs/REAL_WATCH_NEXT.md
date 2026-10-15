# Real watch source and the next modeling target

2026-10-15. Operator requested a real mechanical-watch technical data package or
high-resolution drawing search and advice on the next move. This is a source
study and proposal, not an accepted implementation design or new geometry claim.

## Recommended source: OpenMovement OM10

OM10 is the strongest lead for an inspectable movement reconstruction. The
[project timeline](https://openmovement.org/timeline-om10/) reports a first ticking
manual-wind prototype on27May2023. Its public
[technical information](https://openmovement.org/documents/OM10/OM10_Technical_Information_20210314.pdf)
contains dimensions and gear-train data; page4 is a clear casing section and
plan with casing-clamp coordinates. It does not dimension the complete internal
movement. The public PDF states CC BY-SA3.0. Keep source terms with acquired
artifacts and inspect each package's actual notices before selecting derived
geometry for publication.

The [download index](https://openmovement.org/member-downloads/) lists STEP and
other construction material. The visible STEP link redirects to signup; the
[free User membership](https://openmovement.org/membership-levels/) advertises
construction access. The actual CAD/manufacturing package has **not** been
acquired or inspected. No complete TDP is claimed from an index or filename.
That account-mediated acquisition is the remaining source step; see
[acquisition list](../papers/SHOPPING_LIST.md#real-mechanical-watch-sources-2026-10-15).

The [frumentarius report](../papers/2026-10-15-watch-drawing-source-frumentarius.md)
compares OM10 with actual ETA6497-1, SellitaSW210-1, MIYOTA9015 and TMI NH35
manufacturer documents. The latter are useful service/casing/part references;
none of the inspected sheets supplies a complete movement manufacturing package.
Original PDFs are retained under ignored papers/pdfs, with URLs and hashes in
the [acquisition manifest](../papers/2026-10-15-watch-drawing-acquisitions.tsv).

## Next watch milestone: one dimensioned real component

Acquire and inventory one fixed OM10 manual-wind release. Separate what is
explicitly dimensioned, present only in CAD, inferred from a drawing and absent.
Check assembly placement, part identities, tolerances and source notices.
Then choose one fully described bridge or plate component as the next bounded
modeling consumer. The particular part and prerequisite operation must follow
the acquired geometry; a candidate name is not a claim that its dimensions are
already available.

An immediate obstacle is already visible in our code: extrusion coordinates
are on1/256 grid inside±3/4 and height is at least1/4. Therefore thickness divided
by either nonzero XY axis span is at least1/6. Uniform Frame scaling and a global
change of units cannot remove this restriction. Thin wheel plates/bridges can
fall outside the family before nonconvex outlines or multiple bores are considered.
This explains the intentionally sculptural proportions of study01.

Study a source-dimensioned thin-part contract first, including units, exact
decimal/rational inputs, retained controls and actual binary64 output error.
The selected part may then require curved/nonconvex outlines, multiple bores or
pockets; select one complete operation from its actual blocking features and
review that design before coding. Changing admission checks alone cannot supply
its missing geometry, topology, maps or mesh.

Deliver that original component's source dimensions → actual owned solid →
bounded mesh, with inspectable source ancestry and direct checks on thickness,
outline, hole centers/radii and material domains. Use a source CAD comparison
only for properties it actually supplies; it does not establish manufactured
fit or full movement function. Preserve study01 and NCSX as existing baselines.
Whole-caliber reconstruction, motion and escapement qualification remain later
milestones. Tooth geometry must follow the actual specified horological form;
an arbitrary involute replacement would change the source design.

## Keep the second breadth pass broad

The first result meshes and actual composition chain are delivered. Avoid
letting the new watch consume the whole next tranche. While source access is
resolved, my recommended next engineering lane is the already requested
runtime/Python ownership study: the actual watch spends20.168s in useful
construction/mesh/history work, with6.581s of source/history reading versus
4.868s meshing. These are measured walls, not an attributed language profile.
Study exact arithmetic, object traversal and data conversion before selecting
a native bulk consumer or retained runtime prototype. Compare complete fresh,
reused and shutdown costs with ownership/failure behavior preserved; no rewrite
or speedup is selected from these numbers alone.

External community adapters remain a separate unfinished tranche item and an
alpha requirement. A faithful watch component adds a valuable real workload;
it does not replace external operation cases. NCSX source-to-owned geometry is
also still open. This search changes none of those priorities or release claims.
