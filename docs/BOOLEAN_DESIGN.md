# Through-bore Boolean construction and history

## Purpose and admitted operation

Deliver the solid-operations/history breadth beachhead: an actual regularized
closed difference of two owned input solids, assembled on CPU from exact source
sections, planar arrangements and material selection. The result has a closed
oriented boundary, usable exact geometry/trim/material queries, complete source
history and a native CPU/P100 sampling consumer. No library, native ABI or GPU
Boolean-construction kernel is added. Feature construction is the next theme;
general Boolean geometry and integration tuning are return-pass work.

Accept exact factory BodySnapshot inputs: left.shape='box', right.shape='cylinder',
mathematically identical XYZ Frame origins, cutter.scale=2*box.scale, and
box.scale in {.25,.5,1,2}. The cutter caps lie strictly beyond both box caps,
and the radius .5*box.scale lies strictly inside the box's .75*box.scale XY
half-width. All translated/common-origin instances within existing Frame limits
are supported. Other types, frames, offsets, non-through/coincident-cap/contact
cases and other operations raise named unsupported/type errors. No disjoint
input silently returns a canned unchanged shape or empty result.

The existing make_body('plate') has hole radius .25*scale and is geometrically
wrong for this family. Never call that factory, relabel its records, or construct
an unrelated mesh as the Boolean. Preserve the original inputs exactly on every
success/failure. Build candidates privately and publish one immutable result only
after required checks pass; existing readers remain usable after failures.

## Exact section and region construction

Locate actual input faces/charts by their original factory records. Compute two
existing intersect_faces(FaceRef(box,cap),FaceRef(cutter,wall)) sections, ordered
lower then upper. Each is one closed four-piece exact circle with original
both-UV/spatial maps and all four atlas events. Construct existing arrange_face
on each actual box cap with its section. Retain those sections and arrangements;
never replace them with hand-authored equivalent source records.

For each finite cap region, evaluate its exact rational witness in world space
and classify it using the existing cutter material classifier. Retain the cell
outside the cutter, remove the disk inside, and reject unexpected boundary or
ambiguous decisions. No other boundary crosses the admitted region, so the
witness selects a complete region under the proven family/separation contract.
Retain side-face witnesses outside the cutter. On the cutter wall select the
middle source-V band [.25,.75] lying inside the box, reverse its outward sense,
and discard the lower/upper bands. Cutter cap witnesses lie outside the box and
are discarded. Save immutable selection records with actual source face,
region/band, witness, classification and keep/reverse decision.

These are whole-cell selections after complete exact intersections/arrangements,
not heuristic sampled decisions for arbitrary surfaces. Boundary inclusion is
regularized: closed box inequalities AND radial_distance_squared >= radius^2.
The new hole wall/rims remain boundary; `A and not closed_B` would wrongly delete
that wall. Cap points inside the bore remain exterior even on the box cap plane.
Every incident supporting output face ID is returned at corners and rims.

## Distinct owned result and geometry frames

Use a separate factory-only immutable CutSolid and CutResult. Do not subclass
BodySnapshot, advertise shape='plate', or loosen canonical consumer admissions.
Reuse existing immutable Vertex, Curve, Edge, Coedge, Loop, Face and Chart record
meanings where they agree. IDs remain dense1-based; slots0-based. The result has
fresh shared body._namespace identity and revision1. CutResult retains both input
snapshots, actual sections/arrangements, region decisions, result and history.
No native handle or reference-kernel pointer is stored in the construction.

CutSolid.frame is the box frame for vertex/spatial-curve local coordinates.
The result's six planar carriers copy actual box patch control/UV/frame bits;
the four cylindrical carriers copy actual cutter wall control/UV/frame bits.
Only native Lineage changes to the new output body/face/chart. In particular,
wall Patch.scale remains the cutter scale, not CutSolid.frame.scale. There is
no clipping/refitting/rounding of carrier controls and no fake common patch frame.
Wall carriers keep their full original normalized U,V; the retained trimmed face
occupies V in[.25,.75]. Native partials remain derivatives of those original
carrier parameters. Source chart map is identity with explicit retained band,
not a hidden reparameterization into[0,1].

All output vertices and spatial curves use exact representable box-local values.
Outer8 vertices/12 line edges come from actual box records. Lower/upper rim
anchors and circle edges come from actual section/arrangement identity and maps,
transformed exactly into the box frame; their radius is .5 and z=-.5/+.5.
The seam joins the two named atlas0/4 anchors, corresponding to source cutter
seam parameter [.25,.75]. Coordinate equality does not establish sharing.
Public exact world edge evaluation uses rational input ratios and the retained
four-piece circle parameter, not angle/arc length. Input geometry maps and new
spatial/incident UV curves must agree over complete pieces, not only endpoints.

Retain original box outer loops after verifying the selected arrangement's
boundary is their complete unsplit source boundary. Add one oppositely oriented
circle hole use per cap from the selected arrangement region. Arrangement words
are oriented in raw target UV independently of face sense: reverse their order
and multiply each use sign by the original cap face orientation when it is-1.
Thus the upper cap rim use is-1, lower cap rim use+1, while wall upper/lower
rim uses are+1/-1. Preserve original arrangement loop/use IDs with this explicit
orientation relation; do not silently copy the lower cap's raw UV hole sign.
Keep six box face
IDs/order under the new namespace, then inner wall face7, with orientation-1.
The inner wall loop reverses the original cutter wall boundary and crops it:
seam at atlasU0 upward, upper rim CCW, seam at atlasU4 downward, lower rim CW.
Circle wall UV maps use source V=.25/.75; seam maps are existing generic line
maps (atlasU,.25)->(atlasU,.75). Distinct U0/U4 uses name one physical seam edge.

Output roles are8 outer vertices plus2 anchors,12 outer edges plus2 rims plus1
seam,7 faces,9 loops,30 coedges and10 charts. Outer edge/vertex source order is
preserved; lower anchor/rim precedes upper; seam is last. Every physical edge has
exactly two opposite uses, including the same-face seam pair. Faces form one
connected shell. Every vertex link is one cycle, distinguishing both endpoints
of a closed edge. With two annular cap holes, V−E+F−holes=10−15+7−2=0 (genus1).
Plain V−E+F ignores the multiply connected cap domains and is insufficient.

Before publication check typed indices/ownership, dense identities, complete
loop/face/chart ownership, oriented loop closure, opposed incidence, connected
shell and cyclic vertex links. The declared exact domain/geometry construction
also excludes unwanted self-intersection: six box patches form the outer box,
the entire reversed cylindrical band is strictly interior in XY, and the only
cross-face intersections are the two exact shared rims/outer box edges.
A graph check alone cannot certify that geometric claim. The independent oracle
must compare all carrier/trim/edge maps and domains against the analytic family.

## History and persistent meaning

EntityKey=(body_id,revision,kind,id), with kinds vertex,edge,coedge,loop,face,chart.
Keep full typed namespaces; slot position/XYZ cannot substitute for identity.
History records every input subshape once, with status retained/modified/deleted,
its same-kind output images and separately generated outputs it supports. Each
output subshape has exactly one origin record with relation retained/modified/
generated, actual input parents, optional intrinsic source edge parameter range,
retained source UV band/domain, orientation relation and section/piece/event keys.
Forward and reverse lookup agree. This declares owned semantics; it does not
claim compatibility with another kernel's exact history conventions or persistent
naming after arbitrary future edits.

Retained means the complete subshape's geometry and boundary domain/orientation
survive under a fresh output namespace; modified means changed trim/range/sense.
Box caps and their charts are modified; box sides, outer boundary entities and
original outer loops are retained. Cutter wall/its loop and four charts are
modified and wall orientation reversed; its seam edge and two seam uses are
cropped, with source intrinsic t in[.25,.75], and coedge sense reversed. Original
cutter caps, cap charts/loops, two rings, four ring uses and end vertices are
deleted. No original cutter cap may appear as an output material face.

The two rim edges and four rim uses/inner cap loops are generated jointly from
the corresponding actual box cap and cutter wall, retaining complete circle
component/piece/event identity. The two anchor vertices additionally name the
original cutter seam support and named atlas join event. Generated output is not
falsely a modified original cutter cap ring. Edge/coedge parameter maps address
intrinsic geometric traversal; orientation is separate and applied once.
All output81 subshapes have origins, all input85 subshapes have dispositions:
58 retained images,13 modified images,14 deleted inputs,10 generated outputs for
this family. Counts are useful consistency, not a replacement for actual mappings.
The operation result retains left/right operand roles separately from subshape
history; no ambiguous whole-tool deletion semantics is invented.

## Queries and selective acceleration consumer

CutSolid.classify(patch_slot,u,v) uses exact submitted int/Fraction/finite-float
ratios for cap disk exclusion, rectangle limits and wall retained band. Nonfinite
query coordinates return INVALID; wrong scalar types reject. Return the existing
REJECTED/INTERIOR/BOUNDARY/INVALID labels with no numerical tolerance. Internal
quarter chart joins are not physical trim boundaries; atlas0/4 seam occurrences
are retained. An exact rational point on a rim can be boundary even when its
nearest binary64 UV falls on either side; do not snap it.
CutSolid.classify_point returns exact point/material label and every supporting
output face ID. Its world constraints come from actual admitted operands/result.
An exact rational+pi volume can be derived from the selected cap arrangement's
area and actual world height, not used as sole geometry qualification.

A narrow CutResult sampling adapter gathers actual classified candidate rows,
evaluates existing owned Patch records through Engine on CPU/P100, converts with
each patch's own frame/domain, applies output face sense to native du×dv normals
and preserves original row/source identity. Sampler candidates are actual binary64 UV values: copy/coerce them before
classification and classify those same actual bits; exact rational classify()
queries are separate and are never silently rounded across a trim. A wall grid
spans [.25,.75]; explicit outside-band queries remain rejected rows. Check actual native request/count/
status/descriptor/lineage binding before consumption. No GPU construction claim.

Expose an explicit TopologyInput from the assembled edges/faces/coedges using
new result entity identities, with snapshots=() and the CutResult retained by
the caller. Existing ABI5 can build/read mates and neighboring faces on CPU/P100
without widening canonical pack_bodies or adding a new topology ABI. This reader
checks incidence only; the complete result oracle checks geometry/history.
Use these real consumers for complete construction/consumption costs, not an
artificially duplicated large modeling task. No general mesher/classifier/scene
adapter is implied by the new result type; canonical APIs continue rejecting it.

## Qualification, source studies and failure behavior

Independent specification oracles use original exact frame ratios, the complete
analytic box-minus-cylinder material predicate, unchanged source carrier bits,
original source parameter maps and complete output domains/boundary words. Check
all shared curve/UV maps over rational polynomial pieces, oriented normals,
full source-history coverage, manifold embedding and rollback. Test all four
admitted scales, translations, equal geometry/distinct input namespaces, seams,
rims, corners, nextafter/query-edge cases and invalid/unsupported operands. Inject
failure after actual partial candidate work and verify input tables/sampling and
a previously completed result remain unchanged and usable. No partial CutResult is published for construction input, arithmetic, allocation
or output-check failure. A consumer failure publishes no partial sample batch
and preserves an already constructed CutResult unchanged and usable.

A separately executed existing usd-brep reference difference on copies can compare
actual current-family volume, membership/query witnesses and boundary geometry.
Its public API mutates/consumes some operands and does not promise our history;
reference success cannot replace owned geometry/history truth. No new reference
build/dependency is adopted. Community Boolean qualification remains required
and incomplete for alpha; these bounded cases do not waive the missing corpus.

Source semantics/history, geometry oracle and runtime studies under papers/
precede this design. Review the whole design, then the implementation plan, before
product code. Test/build/run only on the fleet. Report complete CPU construction,
native consumer/setup/transfers/cleanup and separate independent checking/export
costs; any CPU/P100 comparison must identify its actual geometry/topology consumer.
No assumed GPU gain or end-to-end CAD speedup follows from a valid CPU Boolean.
