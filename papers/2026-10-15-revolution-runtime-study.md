# Revolution: exact source-preserving carriers and current CPU/P100 consumers

2026-10-15. Research only; no owned geometry execution, implementation, new build,
or dependency adoption. Read current profile/extrusion, generic sampling helpers,
actual caller/reference/cost sources and acquired sweep implementations. A
read-only fleet inspection confirmed the installed usd-brep1.0.0 rotary API and
its live docstring. Private acquisition paths and machine names are omitted.

## Selected useful first family

Use the actual existing XY PlanarProfile and its Frame, not a reconstructed or
reinterpreted meridian. Restrict its already strict-convex CCW3..32-vertex1/256
lattice polygon to radius x=r in[1/4,3/4], axial y in[-3/4,3/4]. Full positive
revolution about profile-local +Y through Frame.origin is
S=(r cos(theta),y,-r sin(theta)). Thus theta0 is literally the original source
embedding(r,y,0); the axis and handedness also match the proposed foreign call.
Positive radius excludes poles, collapsed circular edges and axis crossings.
Partial angles, negative-angle variants, arbitrary axes, holes and nonconvex or
curved profiles are separate extensions. The finite polygon family already makes
closed genus-one solids with cylindrical, planar-annular and conical faces.

For source edge i, let r(t),y(t) be its exact endpoint interpolation. Use four
rational angular quarters and one source-edge parameter t. In quarter0 the
homogeneous circle controls(A,B,W) are(1,0,1),(1,1,1),(0,2,2); other quarters are
integer signed permutations of(A,B). Their rational point is
C(s)=(1-s*s)/(1+s*s), S(s)=2*s/(1+s*s), then rotated by the quadrant.
For every angular control and both source endpoints construct
(r*A,y*W,-r*B,W). These are exact dyadic degree(2,1) tensor controls. No trigonometric
control rounding, square-root weight, fitting or new native ABI is needed. With
W in{1,2} and abs(r),abs(y)<=3/4, every homogeneous numerator satisfies the existing
abs(numerator)<=W admission bound. Retain actual control bits, source Frame,
source-edge parameter and quarter/chart ID in the output. Native du is the
rational quarter derivative, not radians or constant angular velocity.

The source CCW edge delta(dr,dy) has outward rotated normal proportional to
(dy*C,-dr,-dy*S). Direct differentiation shows the angular-u/source-edge-v cross
product has this direction, so faces can use positive orientation consistently.
The radius lower bound and nonzero lattice edge prevent a singular cross product.
Normal normalization still rounds; independent exact/interval truth must grade
actual downloaded output rather than promise exact floating normals.

## Physical topology and original identity

One physical face per source edge, four charts per face; quarter joins are atlas
joins and do not invent physical edges. One anchor vertex per source vertex at
its original theta0 position. One full closed circular edge per source vertex
(start=end at that anchor), and one meridian seam per source edge preserving the
original spatial line and its direction. A standard positively oriented face
rectangle traverses lower ring forward, meridian at atlasU4 forward, upper ring
backward and the same meridian at atlasU0 backward. Adjacent faces share rings
with opposed uses; each seam has two opposed uses on the same face. Counts are
N vertices,2N edges,4N uses,N loops,N faces,4N charts:13N output entities and
Euler characteristic0/genus1. Closed-edge endpoint occurrences and vertex-link
cycles require qualification beyond degree-two edge incidence.

Existing Curve('circle',...) encodes an XY circle at fixed Z. It must not silently
mean an XZ circle at fixed Y here. Use an explicit circle_y record or an owned
operation curve with an equally explicit axis contract. Maintain fresh operation
namespace and complete six-kind typed provenance. Geometric retention at theta0,
generated rings/faces, same-face seam occurrences and chart ancestry are distinct
relations; copying extrusion's two-cap history would be false. Original source
face/chart/loop remain bound records even when they do not survive as output
boundary entities. The semantic design should specify every history relation.

Material membership reduces local world position(X,Y,Z) to the original polygon
at(sqrt(X*X+Z*Z),Y). For each exact source halfspace, compare a*sqrt(D)+b to zero
with sign-aware exact rational squaring, or an already owned exact quadratic
scalar. Do not round hypot before classification. D=0 is exterior by the radius
constraint. Return all active supporting source-edge/output-face IDs. Native
sample admission first copies actual binary64 UV, then classifies those bits;
direct exact material queries retain their original values.

## Current integration, failure and resource boundaries

Recommend separate factory RevolutionResult/RevolutionSolid, not widened
BodySnapshot/ExtrusionSolid guards. The existing Boolean `_check` and `_rows`
helpers operate on the actual shared result.solid patches/charts/faces contract:
complete descriptors, slots/UV bits, frame/Lineage, native statuses, world jets,
face sense and row order. Reuse is feasible through a separate guarded
RevolutionRequest and factory samples, without changing those public old types.
Current ABI1 evaluator and ABI5 explicit TopologyInput with snapshots() empty can
consume retained operation records; no native build is required. The ABI5 reader
must consume actual mates/neighbor faces, including a same-face seam pair.

At most128 charts; reject chart_count*resolution**2 above1,048,576 before building
a grid. Bound arbitrary candidate iterables at cap+1 before publishing. At grid17,
3/6/16-edge profiles yield3468/6936/18496 candidate rows; every ordinary chart is a
complete regular square, so these are admitted, unlike extrusion's rejected cap
samples. Quarter joins are interior face points, while the physical seam has
both atlas0/4 occurrences and source-edge endpoints map to rings. Retain actual
input copies, IDs and output rows, not an independently regenerated ideal grid.

Construct geometry/topology/history privately, validate all contracts, then
publish one immutable result. A failure publishes no partial solid/history and
leaves the source healthy. Sampling borrows Engine and owns patch/query/result
children through existing child scopes; status/provenance/normal failure returns
no samples while preserving an already valid result. Close every child after
late host failure, preserve the original exception, and require a successful
retry with that Engine/result. Optional kernel timing absent flags means NaN,
not an invented finite measurement. The complete result consumer still owns
context creation, packing, allocation, transfers, frame/normal/scatter and cleanup.

## Reference implementations and available foreign experiment

The installed public function is rotational_sweep(curves,base_point,axis,
angle_deg,cap_ends=True). Its docstring says360 gives the closed full turn and
cap_ends is ignored for that case; profiles must stay on one side of the axis.
Use actual source world line segments, Frame.origin, axis(0,1,0), angle360;
retain original curve handles and before/after parameter/point probes. The
standalone create_surface_revolution returns a carrier, not a closed solid.
Pinned NVIDIA SmApiCreateRotationalSweep sets output null, copies curves before
the consuming primitive and guards the newly allocated Brep until success. Its
sweep geometry uses degrees and tolerance-based full-turn/axis checks; over360
is rejected. These contracts motivate separate immutable input/result ownership,
not adopting its tolerance model. [NVIDIA API](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html).

OCCT MakeRevol distinguishes vertex→edge, edge→face, wire→shell and face→solid,
uses radians with default2pi, and exposes first/last/generated/deleted and
axis-degenerate edges. Copy is configurable. The implementation builds missing
source3D curves and updates result tolerances; a const-looking generating shape
is not a guarantee of immutable reference data. Its separate degenerate-edge
handling explains why axis contacts are a distinct first-family extension.
[OCCT MakeRevol](https://occt3d.com/dev/doc/refman/html/class_b_rep_prim_a_p_i___make_revol.html).

Truck rsweep uses normalized axes and radians, dispatches abs(angle)>=2pi to a
closed sweep, and requires at least two divisions for that path. Its owned
closed sweep explicitly connects the final section to the original object,
including face→solid boundary construction. Its geometric subdivisions are a
representation choice, not required owned edge/face-count truth. Signed angles
flip the axis there, unlike the selected single positive turn.
[Truck rsweep](https://docs.rs/truck-modeling/latest/truck_modeling/builder/fn.rsweep.html).

The future foreign experiment should compare manifold status, volume
2*pi*integral_polygon(r dA)*Frame.scale**3, complete finite bounds
origin+scale*((-rmax,ymin,-rmax),(rmax,ymax,rmax)), all owned face-interior native
witnesses and exact source/ring/seam witnesses. Record the pi coefficient and
actual foreign floating volume/tolerance separately. Confirm all world endpoint
bits are exactly representable or explicitly bound their construction rounding.
Do not compare foreign parameterization, face counts or history IDs as equality.
No foreign material/history adapter or external alpha corpus is established by
this proposed one-operation comparison. No sweep was executed in this study.

## Actual caller and meaningful costs

Use three genuinely different positive-radius meridians: a sloped triangle,
asymmetric chamfered polygon and a16-edge lattice polygon. Include one planar
annular and one cylindrical generating edge across the fixtures. Verify strict
convexity and radius admission before selecting points; changing only world scale
does not increase workload. Preserve the source, full output records, controls,
chart/UV maps, six-kind history, native rows and CSR arrays in artifacts.

Measure source preparation separately, then prepared-profile→CPUconstruction→
material/edge/UV/history reads→request→Engine/native jets→oriented normals→ABI5
build/actual topology reader→cleanup as one useful wall. Also publish points→
complete wall and fresh-process full child wall. Independent qualification,
foreign execution and SVG/export remain separate. Native phase medians nest in
larger walls and are not additive; absent wrapper kernel profiling is no kernel
speed claim. Use CPU1/CPU24/both P100 placements with serialized GPU runs and
record source/native hashes, interpreter and observed timing availability.
No GPU construction or whole-operation win is assumed, and this is not permission
to start the deferred residency/optimization lane. An actual ring/seam wireframe
with admitted native geometry/normals is useful visualization; it is not a new
result mesher. Independent full topology/history/material and actual downloaded
first-jet checks are prerequisites for any performance interpretation.

Pinned file hashes: `revolution-runtime-sources.tsv`; bibliography:
`revolution-runtime.bib`. These are study references, not runtime dependencies.

Selected campaign sizing clarification: root chooses grid9 by default, giving
972/1944/5184 rows for3/6/16 source edges, one excluded fresh-process warmup plus
three measured runs at each of four placements(48 total). Grid17 counts above
are capacity illustrations, not the selected campaign. A concrete admissible
chamfer is(1/4,-1/4),(3/8,-1/2),(5/8,-1/2),(3/4,-1/4),(3/4,1/2),(1/4,1/2),
which exercises horizontal, vertical and sloped meridian edges. One useful
triangle is(1/4,-1/2),(3/4,-1/4),(1/2,1/2). A16-edge source can use the already
qualified lattice polygon with exact radial translation/scaling into[1/4,3/4],
but must remain a newly recorded actual profile, not a canonical-shape stand-in.
