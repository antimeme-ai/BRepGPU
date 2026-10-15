# Actual pierced-feature chain and watch-inspired assembly

Consume original profile → actual ExtrusionResult → one complete axial circular
bore → bounded complete mesh, with six-kind ancestry back to original profile
and actual cutter. This delivers the missing operation composition and a real
watch-inspired visual assembly. See WATCH_TARGET.md for aesthetic/functional limits.

## Exact admitted operation

`feature_bore.difference(feature, cutter)` accepts exact factory ExtrusionResult
and canonical cylinder BodySnapshot. Preserve both actual objects. Same original
Z frame origin, positive source Frames as currently admitted, and cutter height
strictly exceeds feature height in world units: sc > height*sf. Cutter radius
is sc/4. Its local center in feature coordinates is C=(oc.xy-of.xy)/sf and local
radius r=sc/(4sf). For every source convex CCW edge d=B-A, require s=cross(d,C-A)>0
and s*s>r*r*(d dot d), using exact original values. This admits off-center bores,
rejects exterior/tangent/partial/contact cases and allows no tolerance repair.
Cutter cap faces are outside. V endpoints are1/2 ± height*sf/(2sc), strictly0..1.
No restriction to a canonical box or bounding-box relabeling is permitted.

New factory-only immutable BoreResult retains feature, cutter, solid, immediate
history and composed_history. BoreSolid retains feature frame, topology/patch
records, center/radius/wall_band and enough original sources for exact readers.
Original outer vertex/curve/edge/coedge/loop identities remain one-to-one with
fresh result namespace; source cap faces/charts become modified trimmed domains.
Original feature carrier control/frame/UV bits remain untouched. Side faces retain
source affine maps. Add two closed rim edges/anchors, one cropped seam, cap-hole
occurrences and one inward cutter wall. Preserve four original cutter wall charts,
their actual Frames/coefficients and raw V interval; atlas seam U0/U4 stays two
occurrences on one physical edge. No GPU construction or new ABI/library.

For profile N, counts are vertices2N+2, edges3N+3, coedges6N+6, loopsN+5,
facesN+3, chartsN+6. Original cap loops remain outer loops; generated hole loops
are distinct. Complete closed genus-one shell, with exact material predicates:
source convex supports and Z range plus radial squared>=r². Boundary readers
retain all incident faces. Body-local circles record center explicitly; each
actual coedge maps intrinsic edge t to original cap/cutter UV without reversing
its parameter a second time. Preserve unchanged public canonical APIs/guards.

## Source ancestry and map composition

Immediate history records every boundary entity of actual feature solid and
actual cutter (all six kinds), with retained/modified/deleted dispositions and
all result origins. Use actual intermediate keys, source ranges and UV domains.
Generated rims/uses name the incident feature cap face and cutter wall; anchors
also retain cutter seam ancestry. Deleted input entities may generate descendants.

Composed history traces result targets to original profile and cutter keys.
Each root path retains the actual ordered immediate origin and, where a feature
entity is involved, its actual extrusion origin. Do not replace those maps with
an unlabelled transitive parent set or flatten incompatible parameters. Preserve
translation/axial/source_t/source_uv and domain/owner information in each step.
Provide origin(target) with complete paths and from_input(root) with descendants.
Validate path namespaces, actual step objects/parents and complete root/target
coverage before publication. Original profile correspondence and cutter V/range
are directly checked over the actual maps, not merely reciprocal key counts.
This is a two-operation composition, not a universal feature graph or persistent
cross-session naming. Failures publish nothing and preserve all prior objects.

## Complete mesh of actual domains

`feature_bore_meshing.tessellate(result, *, tolerance,max_quarter_segments=256)`
returns immutable BoreMesh retaining actual result/body/immediate+composed history
and shared mesh leaf records. Positive exact world tolerance and bounded dyadic
quarter segments follow existing circular mesh contracts. Use the first n with
worldR/(2n²)<=tolerance/2; actual shared rounded vertex L1 displacement rho gives
total bound worldR/(2n²)+rho. Reject unmet budget/embedding before publication.

Read actual outer source loops, edges, cap domains and original cutter frames.
Each circular rim has M=4n source samples at original rational quarter parameters,
shared across cap/wall and both seam occurrences. Outer edges are not subdivided.
Source cap UV is original extrusion carrier XY=(-1+2u,-1+2v), not canonical box UV.
Original wall corners keep source V interval and chart-local U. No carrier refit.

Triangulate one convex outer cap polygon minus the inscribed circle polygon.
Choose rightmost ring anchor H=C+(r,0) and maximum-X original outer vertex P.
Strict containment ensures P.x>H.x. The segment H→P stays inside convex outer
polygon and outside the circle except H, so it is a visible bridge. Splice the
clockwise inner word into the CCW outer word, duplicating bridge endpoint
occurrences only, never physical vertex indices. Exact strict-positive ear
clipping ignores duplicate coordinate occurrences of an ear's own endpoints
while blocking other points on/in it. Do not omit degenerate/unresolved regions;
raise if a complete triangulation cannot be constructed. Cap orientation applies
once; wall/outer sides consume exactly the same boundary nodes. Expected mesh
V=2N+2M and T=4N+4M, genus one. General multihole/nonconvex triangulation is not
promised. Degenerate valid-input cases found by direct tests require fixing the
algorithm, not silently shrinking the admitted family to get green.

Independent qualification reconstructs original source expressions, exact
circle-support admission, source/history maps, face-domain boundary words and
actual patch UV reproduction. Require strict positive triangles and FULL integer
boundary-chain equality for cap-hole domains, side rectangles and physical wall
chord strips, separately exact/rounded. Check ring order/convexity/strict nesting,
paired layers, planes, opposite mesh incidence, cyclic vertex links and connected
shell. Rounded geometry must remain the actual extruded polygon-with-polygonal-hole
shell; oblique rounded side planes may move within rho. Source cap triangles may
enter a bounded strip inside the analytic hole; no exact material-IN claim.

## Watch scene and actual consumer

Original asymmetric skeleton movement with separately modeled pierced wheels,
convex bridges/collars, bearings, layered supports and a balance-like assembly.
All displayed solids come from actual supported constructors and complete meshes;
pierced parts execute the new chain and retain their original profile/cutter.
Optional stylized teeth are separately inventoried convex extrusion parts, not
one fused/involute gear. No functional kinematics, working escapement, spring
law, manufacturing tolerances or as-built caliber is established.

Source geometry uses admitted Frames/heights; no anisotropic display compression.
Keep explicit component recipes, body/revision/source IDs, material, placement,
camera/light and actual indexed output. A dedicated caller consumes meshes and
both histories, qualifies actual distinct geometry, exports source-rich records/
PLY/scene and renders actual triangles with owned CPU depth-buffer display.
Reuse established renderer ideas without changing the NCSX baseline. Render and
qualification cost stay separate from useful chain/mesh/indexed-history work.

## Direct tests and stop

Genuine missing-API red, then bounded geometry/history and whole exact/rounded
mesh green. Exercise 3/4/6/16/32-source vertices, nonaxis/offcenter cases, frame/
height extremes, strict tangency/exterior/short-cutter rejection, coarse/fine and
budget failure, duplicated bridge occurrences and nearby alignment cases. Check
all six kinds and original map values, source namespaces, actual rounded/query
values and immutable/failure behavior. Alter a few actual outputs to expose
missing/fill-hole triangles, wrong ancestry/maps/UV/seam/shared indices and bounds.
No mutation-score or broad certification campaign.

Independent design/plan/code and actual output review, one affected compatibility
run, actual watch caller and truthful image complete this bounded delivery.
Optional native sampling is a separate explicitly timed consumer; do not claim
GPU Boolean or GPU rendering. Publish coherent branch commits/PR, merge and sync.
External alpha corpus and prior reference disagreements remain open. Stop once
these direct obligations and finding-driven fixes pass.
