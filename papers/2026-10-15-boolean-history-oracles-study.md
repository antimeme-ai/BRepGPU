# First curved difference: material, shell and history truth

2026-10-15. Bounded study for a CPU box-minus-through-cylinder operation. Read
actual owned bodies, exact classification/intersection/arrangement records,
roadmap and alpha policy; pinned OCCT Boolean/history source and selected recipes;
pinned NVIDIA difference bindings and the existing reference bore benchmark.
No implementation, product execution or dependency adoption.

## Scope grounded in the existing bodies

The existing box has local half-width 3/4 and height 1. The cylinder has radius
1/4 and height 1; Frame applies one uniform power-of-two scale. Therefore a
strict through cutter, with its circle strictly inside the box caps, uses cutter
scale twice box scale. The resulting hole radius is half the box scale. Equal
frames instead give the canonical plate's smaller hole but **coincident cutter
and box caps**. Do not quietly treat those two contact regimes as identical.

Recommend a first coaxial family: equal exact XY origins; cutter scale twice box
scale; cutter bottom strictly below box bottom and top strictly above box top.
Available box scales are 1/4, 1/2, 1, 2 with corresponding cutters 1/2, 1, 2, 4.
Keep arbitrary admitted frame translation and strictly-through axial offsets;
reject off-axis/partial/tangent/coincident-cap cases explicitly unless the design
separately admits them. Current centered `Curve('circle',(radius,z))` records fit
coaxial output; off-axis holes need a representation decision. The topology of a
plate is a valid expected model, but `make_body('plate')` is not a Boolean
implementation or a same-geometry oracle for every admitted scale pairing.

## Independent material and boundary answer

Treat original finite float coordinates as exact rational values. Let the box
have closed bounds xmin..xmax, ymin..ymax, zmin..zmax, and cutter center (cx,cy),
radius r. Define Q=(x−cx)^2+(y−cy)^2−r². For this admitted through family the
regularized difference is exactly

`D = {closed box inequalities AND Q >= 0}`.

Strict interior requires every box inequality strict and Q>0. If any closed
constraint fails, the point is exterior; otherwise it is boundary iff a support
constraint equals zero. Report all output supporting face IDs. In particular:
cap points with Q<0 are exterior even exactly on the cap plane; wall points with
zmin<z<zmax are boundary; each rim has both cap and wall support; unchanged outer
corners have all three original box supporting faces. Cutter caps outside the
box are absent, not dangling output faces.

Regularization means closure of interior, so it restores retained boundary loci
while removing lower-dimensional debris. This definition comes from CGAL's
[regularized-set manual](https://doc.cgal.org/latest/Nef_3/index.html); its Nef
implementation is polyhedral and is not an exact curved-cylinder oracle.

Use literal exact points in every material cell and on every support stratum,
plus one-ULP and exact rational offsets around rims/caps. Include points on the
cutter's original external caps and at the cylinder's seam/cardinal aliases.
A selected point grid is direct reader evidence, not a completeness proof by
itself. Do not compute expected labels as `A.classify(...) AND NOT B.classify(...)`:
that incorrectly removes the closed wall and also shares production classifier
faults. Evaluate the original inequalities independently.

## Complete shell and geometric coverage

The stronger whole-family oracle is a complete bijection between output faces
and these seven expected domains: four unchanged box sides, two original box
caps with precisely one open circular disk removed, and the original cutter wall
restricted to zmin..zmax with reversed material orientation. Establish each
domain and its original carrier map exactly. Exclude all extra/duplicate faces;
no surviving cutter cap, hole-filling face or omitted sector is permitted.

For each cap, reconstruct the rectangle and circle from original input geometry,
not from returned loop coordinates. Its exact arrangement must cover disk,
annulus and exterior; retain the annulus and discard the disk by the material
predicate. Check its outer word and full hole word, original boundary parameter
ranges and the entire circle's ordered four source spans/events. This identifies
the whole retained domain, rather than accepting a cap because a witness or its
area happens to match.

For each of four wall charts, require the complete original angular interval and
one exact positive axial interval. If output normalized t covers box height,
original cutter v is

`v = (zmin − cutter_zmin)/cutter_scale + (box_scale/cutter_scale)*t`.

Use original chart parameter q+s for the angular map, not an angle inferred from
atan2. Establish map equality from original rational controls/maps and affine
substitution (polynomial cross-multiplication), with positive denominator over
the full domain. Verify rim world maps and both original box-cap/cutter-wall UV
maps through all source pieces and seam joins. Sampling can then qualify actual
native first jets without replacing this whole-domain correspondence argument.

The output wall normal points **toward the hole**; original cutter intrinsic
carrier derivatives need not change when its material face orientation reverses.
Exact material witnesses on either side of each oriented face detect a reversed
shell or retained wrong region. Choose rational offsets from actual domain
clearance, rather than a fixed epsilon that disappears near a boundary.

Require every physical edge to have exactly two oppositely directed uses,
including the seam's two distinct uses on the same wall face; one connected
shell; closed oriented loop words; valid cyclic vertex links; and no unreferenced
vertices/edges/charts. Entire expected domains and source geometry establish the
embedding without a generic pairwise surface-intersection checker. Cardinal
chart events do not automatically become four different physical rim vertices.

The existing whole-circle convention suggests V=10, E=15, F=7, 30 uses and 9
loops: eight box vertices plus two rim seam anchors, twelve box edges plus two
closed rims and one wall seam. These are supplementary invariants. Plain
V−E+F gives the wrong genus because the two cap faces have holes: the corrected
cell count is `10−15+7−2=0`, consistent with one genus-one shell. Check actual
loop/domain structure before using this calculation.

## History must describe retained geometry and source identity

Pin a vocabulary before implementation. Recommend typed source keys with operand
occurrence, body namespace, revision, entity kind and ID. Output keys live in a
new namespace. "Reused" describes unchanged oriented source geometry/domain,
not necessarily equal numeric IDs or the same Python object. All promised source
kinds must have an entry, including deleted ones; all output entities must have
complete provenance. If history covers only solid/face/edge/vertex kinds, state
that explicitly; retain coedge/chart occurrences and maps separately.

| Source entity | Required retained-image meaning in the strict through family |
| --- | --- |
| Box solid | Modified to the cut solid; original immutable input survives. |
| Four side faces, 12 outer edges, 8 outer vertices | Reused unchanged geometry/domain with exact correspondence. |
| Box top/bottom faces | Modified to one annular face each. An arrangement had two inside regions, but there is only one retained face image; do not claim two output splits. |
| Cutter wall | One cropped and material-reversed wall image, with four original chart maps and exact axial range. It is modified, not wholly deleted. |
| Cutter seam edge | One retained cropped seam interval; two wall coedge occurrences remain distinct. |
| Cutter cap faces, original cap rims and original seam endpoint vertices | Deleted from the output: no reused or modified images. |
| Each new output rim edge | Generated jointly by the corresponding original box cap and original cutter wall, retaining the actual intersection component/pieces/events and both UV maps. |
| New rim seam-anchor vertex | Generated at the box-cap/cutter-seam intersection; retain that source event and both supporting source faces. |

If "split" is a public history category, define whether it counts all intermediate
partitions or only retained images. Do not infer a split solely from "modified".
Generated links are many-to-many: two rim edges each have two source faces.
An entity with no retained same-dimensional image can still generate other
entities, so deletion must not erase generated provenance.

These distinctions follow the independently read
[pinned OCCT history contract](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRepTools/BRepTools_History.hxx)
and [Boolean builder declarations](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKBO/BRepAlgoAPI/BRepAlgoAPI_BuilderAlgo.hxx):
modified relations concern retained images; removed sources lack retained images;
generated relations can survive removal. We should own precise semantics, not
copy OCCT's IDs or compare representation-specific seam counts blindly.

## Failures, rollback and named fault sensitivity

Validate request shape/frame/topology identity and strict separation before
publication. Build candidate sections, arrangements, clipped wall, shell and
history privately; publish one immutable result only after full invariants pass.
A failure must leave both inputs and every existing reader/packed table unchanged
and publish neither partial body nor partial history. Consuming a monotonically
allocated namespace ID is not a mutation of the inputs and need not be rolled
back.

Use bounded injected exceptions at real construction boundaries (after a section,
after a cap arrangement, after wall clipping and immediately before publication),
then compare original packed bytes/source records and run one healthy retry.
Exercise actual allocation/construction exception paths where feasible without
inventing a new native fault framework. Unsupported requests should raise their
named error with unchanged inputs; they are not green supported operations.

Decisive fault witnesses: retain the disk instead of annulus; flip the Q sign;
drop a hole word or a wall quadrant; keep a cutter cap; reverse only one rim use;
merge the seam's same-face occurrences; skip axial cropping; lose one source
parent/event; label modified wall deleted; omit a deleted source entry; mutate
an input before a late failure. Each is checked by a direct geometric, incidence,
source/history or rollback assertion. Actual selected production-site mutants
run only on the fleet; handpicked kills do not imply general coverage.

## Feasible foreign comparison and external debt

The already installed USD BRep 1.0.0 reference can independently construct the
same box/cylinder dimensions and perform difference, as the existing
`benchmarks/cpu/benchmark.py::bored_setup` already does. The pinned Python
[difference binding](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SmPyLib/src/SmPyBooleans.cpp)
explicitly consumes both arguments. Use an isolated process and fresh source
bodies/copies; do not borrow owned input pointers or rely on reference rollback.
Save actual geometry/probe queries and outputs, source parameters and warnings.
Compare material/closed-boundary geometry under the declared representation map;
volume/bounds alone are weak and source/history IDs are not interchangeable.
No new reference dependency or local reference execution is needed for this study.

Pinned OCCT `tests/boolean/bopcut_simple/W3` is self-contained cylinder-minus-box
and uses surface-property plus view checks; `ZE5` uses rotated cylinder cuts.
They demonstrate available operation recipes, not a direct expected answer for
our reversed operand/order or admitted coaxial family. Broader OCCT regressions,
foreign curved contacts and Boolean history adapters remain alpha debt under
`docs/ALPHA_QUALIFICATION.md`. Keep unsupported/missing adapters visible; do not
select only passing external cases or call reference-build success our
qualification. The first exact curved operation is a breadth beachhead, not a
general Boolean kernel or alpha-release claim.

## Concrete design candidate: unchanged source carriers

Root selected the still narrower same-XYZ-origin pairing, cutter scale=2*box
scale, box scale in [1/4,2]. Off-axis and axial offsets are explicit unsupported
requests. This removes a representation extension without reducing the
independence of the material/shell/history oracle above.

Retain all original source homogeneous controls, frames and original carrier UV
unchanged, assigning fresh output lineage. The wall keeps source v in [1/4,3/4]
as an exact trim band; no clipped/reparameterized control net or quantization is
needed. Its output seam maps use source u=0 and u=4, with v from 1/4 to 3/4;
rims use source wall v=1/4 and 3/4. Output edge curves in the box-local frame have
radius 1/2 and z=−1/2,+1/2. Those are the same world curves because cutter
scale is twice box scale. The cap hole in original box UV is centered at (1/2,
1/2) with radius 1/3. Verify that literal rational identity, not the canonical
plate's UV radius 1/6.

The independent completeness check can therefore be exact and small: original
six box carrier nets and four cutter wall carrier nets must be bit-preserved;
all output cap outer words and radius-1/3 hole words must match the original
source arrangements; all four retained wall chart domains must be exactly the
full angular interval times [1/4,3/4], with reversed material normal. The cap
annuli, four sides and wall band have a complete source-domain bijection.
Their only boundary identifications are the original box edges, two circles and
one same-face seam. Source map identities establish nonintersection away from
those declared joins, while opposite uses, closed loops and cyclic vertex links
establish the connected closed manifold. This proves the intended genus-one
embedding together with the corrected Euler check; it does not rely on a
manufactured plate output or sampled volume.

History must separately preserve source-carrier identity and changed trim:
box caps become annuli, cutter wall becomes reversed band, cutter seam retains
its middle parameter interval, and source cutter caps/rims/endvertices have no
retained images. Joint-parent generated rim links and source section/event IDs
remain required. Same carrier controls are insufficient to mark the entire wall
face or either cap face "reused" when their trims/orientation changed.
