# Generic periodic reference preparation

Selected operation: on a guarded copied OCCT BRep, split analytic periodic charts by
ANGLE, preserving original geometry/images and complete source history, then serialize
a named derivative for the unchanged NVIDIA converter. This replaces the failed closure-
detection proposal; it does not patch the SDK, increase its angular threshold, omit
source topology or select source names/IDs. Root owns converter invocation/orchestration.

## Source and derivation

Pinned OCCT b8f597c shape-healing guide§4.3.2 explicitly documents
`ShapeUpgrade_ShapeDivideAngle`: every resulting revolution/cylinder/cone/sphere/torus
segment covers at most the chosen angle. `ShapeUpgrade_SplitSurfaceAngle::Compute`
partitions the ACTUAL finite U span using its documented angular precision. A2π+δ span produces three charts whenδ exceeds Precision::Angular(); smaller excess can produce two charts slightly wider thanπ, still far below one turn.
`ShapeUpgrade_ClosedFaceDivide` instead first detects closed/seam configurations and
left the actual cylinder untouched. Both use ShapeFix_ComposeShell; no success is
assumed before execution. Existing RAW `prepare_source_uv(query=false)` supplies finite
source-linked pcurves without its later query/pole transformations. NVIDIA c9c5979
DriveFace's unchanged angular guard remains the downstream admission authority.

Correct curve-image guard: preserve identical underlying3D basis coefficients and
source curve orientation; copied source vertices/locations/tolerances remain unchanged.
Nonperiodic original edge [a,b] requires descendant intervals to partition [a,b]. For a
periodic curve use its public `Geom_Curve::Period()` P and treat parameter intervals in
R/PZ. Shift each descendant relative to a, reduce modulo P, split wrap intervals at0/P,
then exact-sort/check their atomic coverage with multiplicity against the original
interval. Preserve total traversal length/multiplicity, not merely point-set equality.
Initial finite profile admits0<b−a<=P; wider winding is typed unsupported. A full period
may start at a different phase: the prior actual circle's two descendant arcs cover
exactly one P despite failing linear endpoint equality. This proves identity in the
OCCT nominal periodic-parameter contract, not a claim that binary64 P equals mathematical
2π. Use owned exact dyadic arithmetic over finite API parameter words, with no epsilon
or sampled-only periodic identity. Endpoint direction/incidence remains source-derived.

## Literal ownership/API

Own `native/reference_periodic/prepare.hpp/.cpp`, its small `main.cpp`, an authored
`tests/reference_periodic_fixture.cpp` and focused external reader, plus this plan and
journal. Root owns CMake registration and separate official-converter/whole-watch driver.
`PreparedPeriodic prepare_periodic(const TopoDS_Shape&, const SourceUVOptions&)` retains
original/copy/result, source→descendant face/edge/vertex rows, stage/counter/tolerance
metadata. A public tool accepts INPUT.brep NEW_OUTPUT_DIR, optional explicit existing
RAW projection/source/positional limits (defaults1e-7/.001/.005mm), writes prepared.brep,
source-history.tsv, stages.tsv, tolerances.tsv and status.tsv. Failure returns nonzero,
retains diagnostics and publishes no prepared.brep. Geometry-only admission requires one
identity-located solid and at least one supported periodic analytic chart; no error-string
dispatch. RAW helper and exact arithmetic are already owned/adopted dependencies.

Snapshot source basis, parameters, original vertex XYZ and all source tolerances BEFORE
mutation. RAW preparation retains its reviewed physical guards. Divide the copy with
ShapeUpgrade_ShapeDivideAngle(π); precision/max tolerance come from source topology,
not requested chord error. Preserve every original face's descendants, identical basis
values and whole original edge image/order by the guard above. Every output face must
have source ancestry. New cut entities have explicit generated roles. Report new cut
construction tolerances separately (earlier tool emitted1e-5mm); do not claim unchanged
metadata or alter original tolerances to obtain success. Reject unsupported/mismatched
history, invalid/empty descendants, lost solid or changed original geometry.

Existing exact Integer/Rational handles nominal finite parameter values; no new numerical
evaluator/library. Bound byte/work use and stop on exhaustion. The small helper's exact
owner remains local; no Rational crosses worker/API ownership. Serialize BinToolsV4 with
no triangles/normals to a temporary file and rename only after complete guards succeed.

## RED, finite GREEN, review

1. Direct actual1716 operation RED is already captured: original and closed-split inputs
fail unchanged conversion. Add new public-operation missing-symbol RED before body.
2. Authored cylinder/cone solids with shifted/reversed frames, phase crossing0/2π and
literal within-declared-tolerance seam endpoint offset. SAME native tool + official
converter at unchanged.005/heal0. Include ordinary partial chart and genuine multi-turn
negative, and mutation guards for changed circle radius/vertex/lost interval. Independent
external exact dyadic interval model checks emitted nonperiodic/cyclic coverage/history;
source analytic frames and complete ancestor inventory are checked from original fixture.
3. Both actual inputs once through new operation and unchanged official converter;
report ALL prep costs, history/tolerance changes and genuine resulting faces/triangles.
No full-watch call in this lane; root does that only after source/plan/code review.
4. Strict affected build, scoped source review, existing RAW physical guards remain.
No broader mesher/GPU campaign or silent automatic healing fallback.

Root pipeline: attempt official original conversion first. Only a conversion failure
may invoke this generic geometry-admitted tool and reconvert its derivative, with failed
attempt and preparation wall included. Ordinary successful definitions are not RAW-
preprocessed. Subsequent SDK meshing/healing/plane-envelope policy remains explicit and
separate. This plan requires root review before implementation.

Review closures: curve coverage is paired with a directed descendant-edge chain from
the mapped original start vertex to mapped original end vertex; a closed original edge
allows a cyclic rotation but requires one connected cycle through that source vertex.
Exact parameter atoms alone cannot establish topology/orientation. Snapshot source
locations as well as basis values. RAW source endpoints must retain the existing source
residual allowance; moved-vertex test attacks that guard. Source-history output uses
explicit OCCT storage indices and serializer indices, never assumes SDK face ordinals
are original source IDs. Root and numerical peer accept these clarified contracts.

### Source endpoint range restoration amendment

The actual1811 angle result retains one nonperiodic line basis and its original
vertices/tolerance but shortens its last stored parameter by one binary64 step:
`.035355339059327397` becomes`.03535533905932739`. The exact image guard
correctly refuses this derivative. Before that guard, restore an original endpoint's
literal3D parameter only on a same-basis nonperiodic descendant whose corresponding
FORWARD endpoint is exactly the mapped original vertex (unchanged XYZ/tolerance).
Interior cut parameters remain the splitter's parameters. Require positive resulting
range and the same whole exact partition/directed-chain guards afterwards. Record
old/new endpoints; this restores source support, it does not accept approximate coverage.

Pinned `BRep_Builder::Range(E,first,last,true)` updates only `IsCurve3D()` GCurve
ranges; it does NOT update pcurve coefficients/ranges or SameRange/SameParameter flags.
Therefore explicitly set both flags false on each restored edge rather than retain
false equality assertions. `OcctStagingDriver`/`OcctFileSource` contain no invocation
of OCCT's SameRange/SameParameter repair; genuine conversion remains the downstream
consumer check. No 2D range/coefficient or original topology metadata is edited.
This amendment requires root/source-peer review before body.

Source binding closure: `ShapeExtend_WireData::Init(wire,true)` takes TopoDS storage
order; its documented `false` mode invokes BRepTools_WireExplorer when needed to
obtain connected order. Use false mode for the RAW binding word, requiring the full
(edge identity, composed sense) MULTISET equal storage mode, including duplicate seam
occurrences. Missing/disconnected/incomplete exploration refuses, and RAW's existing
exact directed-vertex-cycle guard still decides admission. No BRep edge/wire is reordered
or repaired. Authored partial cylinder exposed this actual adapter assumption.

Source orientation closure: same-basis descendants of a normalized FORWARD original
edge must retain FORWARD parameter traversal, including closed cycles; cyclic atom
coverage and endpoint closure alone would allow an incorrect reversed full cycle.
The nominal helper accepts explicit direction rows and rejects a reversed-cycle test.
Face descendants must retain captured face sense. Distinct source faces/edges/vertices
cannot silently merge into one descendant. History emits source face sense and ordered
edge incidence in the same source-history rows.
The same traversal mode is used at the RAW guard's existing physical-word read
(`require_source_uv_uses`, matches[0]); only that argument changes true→false.
Its physical count, connector inventory, unique oriented ownership, cyclic successor
and exact source endpoint-cycle checks remain. Three existing reversed-seam/pole/
failure-path methods directly cover this shared seam, alongside authored partial input.

Finite result scope: all seven authored periodic/partial shapes pass preparation,
complete source history, exact Fraction image/order and original-tolerance checks.
Three fully export through SDK; four shifted/reversed controls pass genuine import
and tessellation then hit the explicitly named existing unsupported nontriangle raw
export. The optional test flag reports that capability limit separately and never
creates a mesh or calls it a complete SDK result. Both original failing inputs fully
export genuine SDK triangles. Root owns the complete107-definition pipeline run.

## Follow-up: one-input SDK topology control

Actual new107-definition SDK exports complete but1808 fails the existing closed-shell
checker. Old omitted-angle output is closed; explicit25/25 output has exactly three
physical edge pairs with four uses, including opposite duplicate triangles on a top
Plane and its curved extruded wall. SDK reports12spine halfedges/outputManifoldfalse
BEFORE raw extraction. Genuine6rectangle splits retain each parent boundary and cannot
explain this pre-extraction report. Both heal policies show the same fault.

Falsified controls:1808 original and Plane-widened USD at identical.005/25/25 produce
byte-identical output. Every original/corrected face:trimType is general. Disabling just
one angular constraint still reports12spines. No Plane/rectangle/transport repair selected.
Global full-natural-curve Plane hulls are oversized but are not this observed cause.

Next single bounded diagnostic: use the SAME original USD, source/quality.005/25/25,
heal0, all1112faces, unchanged SDK and raw exporter. Invoke the documented public
`SmBrep::ConvertToPolyBrep` with advancingFrontTRUE, retaining every other parameter
from `SmApiTessellate`. Pinned source c9c5979 `SmApiBrep.cpp::SmApiTessellate` hardcodes
FALSE. `SmBrep.cpp::ConvertToPolyBrep` documents TRUE as EquallySpacedPoints +
TriangulateFaceAF; `SmTess.cpp::AddQuadBoundaries` selects AF vs ordinary triangulation.
This is an algorithm-selection control with unchanged enabled quality constraints,
not an angle relaxation, CAD edit, healing or downstream topology repair.

An ignored private copy of the current producer changes ONLY that API call and checks
rbFailedFacesfalse +zero face failures before extraction. It uses the same compiled
raw library/SDK and separate output path; no production/CMake changes. Run once on
physicalcore1 with180s cap. Read the native report and SAME physical-solid checker,
actual source-face coverage and exported triangle inventory. Any unsupported polygon
or topology refusal remains a failure; no welding/dropping/skipping/replacement triangles.
Report failed/default and alternative costs/outcomes separately. Stop on failure; no
parameter search. If it passes, propose a separate generic production policy/finite
independent authored control for root review before integration. Root owns producermain
and full-watch driver; this lane initially owns diagnosis only.

Literal AF call, verified against SmBrep.h453–483 and SmApiBrep.cpp679–766:
`ConvertToPolyBrep(pointer,failedFaces,lamina,chord,curveAngle,surfaceAngle,
maxEdge,maxAspect,TRUE,FALSE,TRUE,FALSE,&failures,&report)`.
Only advancing-front differs from the public wrapper: smoothingfalse, attribute
propagationtrue, same input contextfalse remain. On status failure, failedFaces,
nonempty failures or null output, SetOKBackPtrs(false) then dispose any partial poly.
Own successful output before checking input/output manifold, zero lamina/spine, then
use the unchanged exporter, complete face set and same physical/source-Euler checker.
The input Brep remains alive through extraction. Parent approved this exact control.

Root separately selects a full legacy chord-only0/0 baseline at the same.005mm and
same explicit preparation/closed-shell gates. This is a separately named constraint
policy, not a successful repair or qualification of the failed25/25 setting. No retry
implementation or additional algorithm search is authorized by this diagnostic unit.

AF control outcome: the single180s-limited call produced no completed SDK definition
report or mesh before external censor. No AF production retry is selected. The timer
was inside the timeout's terminated process group, so exact CPU/wall measurements and
timeout exit status were not retained; the shell's subsequent successful cat commands
must not be called a native success. Record the configured180s censor/no completed
result, with ignored source/executable/logs retained. No additional algorithm search.
