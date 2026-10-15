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

The operator has now supplied `om10-00001_20220701_va_01_3.stp`, the cataloged
MAN VA01.3 construction. Original bytes are retained privately with a recorded
hash; see the [actual STEP source inventory](../papers/2026-10-15-om10-step-source-study.md).
It contains named assembly and B-rep records. Source acquisition is resolved for
this STEP revision; complete manufacturing drawings, original feature history,
fits and tolerances are not established by the exported assembly. The earlier
membership redirect remains recorded as a historical acquisition attempt.

The [frumentarius report](../papers/2026-10-15-watch-drawing-source-frumentarius.md)
compares OM10 with actual ETA6497-1, SellitaSW210-1, MIYOTA9015 and TMI NH35
manufacturer documents. The latter are useful service/casing/part references;
none of the inspected sheets supplies a complete movement manufacturing package.
Original PDFs are retained under ignored papers/pdfs, with URLs and hashes in
the [acquisition manifest](../papers/2026-10-15-watch-drawing-acquisitions.tsv).

## Next watch milestone: one dimensioned real component

The supplied release now gives a concrete first candidate: **OM00-00142,
Bride de fixation**. Its seven-face source describes a thin D-shaped fixing
clamp with three straight outer sides, an outer semicircle and one complete
circular bore. Nominal source dimensions are outer radius0.9mm, straight end
X=2.2mm, bore radius0.6mm and thickness0.3mm; the full outer arc extends to
X=-0.9mm, making the total nominal width3.1mm. Vertex-only coordinate extrema
would miss that leftmost point. Three assembly occurrences reuse this part.

This source selects a clearer first geometry milestone than an unspecified
bridge: thin extrusion of a line-and-arc outline with a real through-bore,
retaining original source entities/assembly placement and bounded mesh output.
General nonconvex sketches and multiple cuts are not required by this component.
A reviewed design is still needed; no implementation is selected by inventory.
Treat the source's declared0.01mm geometric uncertainty separately from mesh
error and manufacturing tolerances. Retain exact original decimal expressions
and the actual file's perturbations; the nominal dimensions above are a source
summary, not a replacement for its full records.

An immediate obstacle is already visible in our code: extrusion coordinates
are on1/256 grid inside±3/4 and height is at least1/4. Therefore thickness divided
by either nonzero XY axis span is at least1/6. Uniform Frame scaling and a global
change of units cannot remove this restriction. Thin wheel plates/bridges can
fall outside the family before nonconvex outlines or multiple bores are considered.
This explains the intentionally sculptural proportions of study01.

Study a source-dimensioned thin-part contract first, including units, exact
decimal/rational inputs, retained controls and actual binary64 output error.
The fixing clamp needs the thin line-and-arc outline and one bore. Later
components may require nonconvex outlines, multiple bores or pockets; select
those operations from their actual blocking features and review before coding. Changing admission checks alone cannot supply
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
letting the new watch consume the whole next tranche. Alongside this concrete
source target, my recommended next engineering lane is the already requested
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
