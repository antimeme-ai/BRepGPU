# Through-bore Boolean and history delivery plan

Read BOOLEAN_DESIGN.md and all three accepted design reviews. Implement the
complete same-origin/scale-ratio-two operation, history and actual consumers.
No canonical API widening, library adoption, native rebuild or tuning lane.

## 1. CPU construction, exact queries and history: one owner

Own python/brepgpu/boolean.py and tests/test_boolean_construction.py. Write the
bounded subplan and direct tests first, sync only owned files/required fixtures,
and record actual red on the fleet before implementation. Preserve existing
canonical bodies and make_body behavior byte-for-byte.

Public API: `difference(box,cutter) -> CutResult`, named UnsupportedBoolean for
unsupported geometry/family. Exact factory BodySnapshot and supported shapes
required; malformed argument types raise TypeError. Other operations are absent,
not automatic fallbacks. Same exact XYZ origin and Sc=2Sb, Sb in{.25,.5,1,2}.

Immutable public records/interfaces (all tuples/Fractions owned):

- EntityKey(body_id,revision,kind,entity_id): positive uint64 numeric fields,
  kind in vertex/edge/coedge/loop/face/chart, bool excluded.
- Origin(target,relation,parents,source_range,source_uv,orientation,section,
  piece_ids,event_ids,arrangement): relation retained/modified/generated;
  optional range is exact intrinsic source t endpoints; source_uv is retained
  source chart UV rectangle (chart-local normalized; wall face atlas0..4);
  orientation is+1/-1 separately from intrinsic geometry. section is lower/upper
  section index0/1 orNone. arrangement is a tuple of source arrangement index,
  region/loop/use IDs where relevant; empty when absent. Parents are actual typed
  input EntityKeys. Preserve original use IDs with the raw-UV→face-sense relation.
- HistoryRecord(source,status,images,generated): every input entity exactly once;
  status retained/modified/deleted; images are same-kind descendants, generated
  contains supported births. History(records,origins) is factory-owned, with
  `from_input(EntityKey)` and `origin(EntityKey)`; unknown keys raise KeyError.
- Selection(side,face_id,kind,region,domain,witness,label,keep,orientation): actual
  opposite-operand classification of exact whole-region witnesses; wall domains
  are original source-V intervals. Side is box/cutter, region/domain may beNone.
- Measure(rational,pi): exact rational+pi coefficients for volume.
- CutSolid factory-only: body_id,revision,frame,vertices,curves,edges,coedges,
  loops,faces,charts,patches,radius,wall_band,volume. frame is box/topology frame;
  each Patch keeps its actual source frame. radius is exact box-local1/2,
  wall_band exact(1/4,3/4). No canonical shape or packed-body tag.
- CutResult factory-only: left,right,solid,sections,arrangements,selections,
  history,timings. New output identity from shared body._namespace.

CutSolid methods: `classify(patch_slot,u,v)` exact trim label with existing numeric
constants; `classify_point(point)` returns a result-owned PointClassification
(solid,point,label,boundary_faces); `edge_point(edge_id,t)` exact world Fraction
XYZ; `coedge_uv(coedge_id,t)` exact source-chart/atlas Fraction UV for intrinsic
edge traversal; `topology_input()` emits existing typed TopologyInput with
snapshots=(), caller retaining CutResult. IDs are1-based, patch slots0-based.
Do not call Curve/UVMap.evaluate for exact queries, since those existing helpers
coerce t to float. Use their retained data with exact rational circle/line maps.

Build actual lower/upper cap-wall sections, actual cap arrangements and selected
outside-cell regions. Retain actual classified witnesses: four cap regions,
four side faces, three wall bands and two cutter caps. Use existing exact source
material classifier on each opposite operand. Unknown/boundary decisions reject.
Read exact rational values from Scalar.a only after asserting b=0; no rounding.
Reuse full input carrier bits; crop only wall trim and seam topology. Generate
rim/anchor IDs from actual section/arrangement events, never XYZ welding.

Assemble/validate the complete oriented graph and all six-kind history in this
same unit. Output face order is original box six thenwall7; vertices1..8/edges
1..12 retain box order; lower anchor/rim precede upper; seam last. Other dense
IDs follow actual assembly; oracles resolve roles/sources rather than guessing
shifted loop/coedge IDs. Bottom cap arrangement hole orientation is reversed by
its original face sense. Every output has a full origin and each input a status.
Store origin source t range[.25,.75] for cropped seam/uses, original wall source
UV band and reversal; generated rims/uses/loops name both source faces and actual
section pieces/events. Anchors additionally name original source seam support.

Validate ranges/ownership, all loop words, two opposed uses per edge, complete
connected shell and cyclic vertex links with closed-edge endpoint occurrences.
Publish only after geometry/selection/topology/history checks. Expose a private
assembly/validation boundary usable for a late failure injection test; never add
production fault flags. Construction failure preserves inputs and prior results;
namespace gaps are acceptable. No new CutResult escapes on failure.

## 2. Independent geometry/topology/history oracle

Own python/brepgpu/boolean_oracles.py and tests/test_boolean.py. Tests first with
a healthy existing body prerequisite and missing-new-operation red on fleet.
Public `qualify_result(result) -> dict` and `qualify_samples(batch) -> dict`;
`qualify_topology(result, adjacency_result) -> dict` checks the actual returned
existing ABI5 reader against exact source/result incidence, including permutations.

Do not import construction/private validation/selection helpers into expected
geometry/history. Read actual inputs and derive the original exact frames,
independent material inequalities, full seven domains, original control/frame
bits and affine UV identities. Check unchanged source patches with fresh output
Lineage, cap radius1/3 in raw UV, full original wall angular charts×[.25,.75],
reversed normals and complete edge/both-UV polynomial identities for every
quarter/line. Exact region/section retained source objects and events must match
actual input namespaces; geometry-equal replacement operands do not count.

Reconstruct all expected histories from original input roles/IDs, not from
returned status counts. Check all85 input dispositions, all81 output origins,
forward/reverse equality, actual source ranges/UV/reversal, generated joint-face
parents and anchor seam event. Check actual loops, vertex-link cycles, connected
shell and χ=V−E+F−2=0; whole-domain geometry rules establish no extra intersections.
Exact material probes cover interiors, hole/exterior, every outer face, rim,
seam and corner, plus rational/nextafter witnesses. Measure/volume and counts are
additional checks, not the complete answer.

For samples, bind the actual CutResult/request, candidate bits, row order,
face/chart IDs and native statuses; check exact actual-parameter trim labels,
positions and oriented normals against original source carrier formulas with
per-Patch frame. Cap/hole and outside-band rows remain explicit NaNs/status-1.
Native target label must be backed by actual result.info backend and binding
checks in consumer. Require direct per-component source-world position error <=2^-36 and oriented
unit-normal error <=2^-30, evaluated at actual copied binary64 UV using each
original source Patch frame. These are observed output limits, not a deduction
from native partial-error bounds. Do not infer correctness from CPU parity. Native invalid statuses are operation failures
for admitted rows; rejected/invalid trim candidates are not submitted.

Test all four scales, translations, independently allocated equal-geometry input
namespaces, wrong/unsupported inputs and aliases, source/candidate immutability,
late construction error, existing-result preservation, complete source coverage,
cap/wall sense, same-face seam uses, native CPU1/CPU24/P1000/P1001 consumers and
permuted topology reader rows. Meaningful malformed/falsified outputs may test
public qualifier rejection; do not claim mutation coverage from selected faults.

## 3. Narrow native sampling/normal consumer

Own python/brepgpu/boolean_sampling.py and tests/test_boolean_sampling.py.
Reuse existing immutable body_sampling.Candidate and SampleRow value records
and owned child-cleanup helper where suitable, without canonical snapshot checks.
Public `CutRequest(result,candidates)`, `prepare_grid(result,resolution=17)` and
`sample(engine,request) -> CutSamples`. CutRequest retains result,candidates,labels,
admitted_indices. CutSamples factory retains result,request,
rows,timings,backend ('cpu'/'cuda'),device (integer) with count/evaluated_count properties. Row shape is
existing SampleRow, but snapshot owner is CutResult, never BodySnapshot.

Candidates are copied as actual float UV BEFORE exact trim classification. Cap
and side grid covers[0,1]^2; wall grid V=.25+.5*t. Resolution integer2..257,
bool excluded; max10*257² candidates, bounded iteration cap+1. Reject invalid
patch slots, preserve invalid/nonfinite UV as INVALID rows. Classify the actual
copied bits once; accepted rows are exactly INTERIOR/BOUNDARY. Original exact
Fraction classify queries are a separate API and are never passed to native
under an implicit different-parameter promise.

Borrow an existing Engine, own/close patch/query/result children per call, gather
admitted slots/actual UV and evaluate/download first jets. Check complete actual
counts/order/statuses/slot+UV bits/descriptor+Lineage/backend against the submitted
patches/request. Convert using native per-Patch metadata, never solid.frame for
wall data. Orient each normalized native du×dv by actual output face sense.
Scatter rows in original order; rejected/invalid rows have native_status-1 and
NaN positions/normals. No partial batch on native/arithmetic/allocation/cleanup
failure; already valid CutResult remains unchanged. Healthy retry test uses the
same retained result/borrowed Engine after a late host exception, with child
resources closed and primary error preserved by existing cleanup semantics.

Separate gather/query/evaluate/download/convert/normal-scatter/cleanup costs plus
complete sample wall. Caller owns Engine creation/closure around its complete
operation timer. Existing native binaries remain unchanged. No implicit fallback.

## 4. Actual operation caller, reference experiment and complete costs

Root owns examples/cut_bore.py, optional scripts/reference_boolean.py and
benchmarks/boolean/run.py. Use explicit source preparation outside construction
wall, then actual difference, material/edge/history graph consumption, sampling
and existing CPU/P100 adjacency with its actual mates/neighbors read. Validate
native result ownership and complete statuses inside useful operation timers.
Independent source/material/topology/history checks, visualization and artifact
writes have separate timers. A simple actual resulting geometry visualization
may be added after qualification; no full result mesher is implied.

Export exact operand frames/IDs/source hashes, sections/arrangements/selection
and all output entities/UVs/carriers, forward/reverse history, raw candidate/
consumed rows, actual adjacency and phase costs using TSV/native binary. No JSON
persistence or private host/absolute path metadata. Record source/native/controller
hashes and actual GPU/backend. No canonical packed-body tag for CutSolid.

Run one excluded warmup plus three measured fresh processes on native CPU1,
CPU24 and each P100 for three admitted recipes (scale/translation); same supported
outputs and query grid. CPU constructs every result; GPU placement is only the
sampling/adjacency consumer. Preserve construction versus complete-consumer costs,
startup/transfer/cleanup and independent-check overhead. These are exploratory
small complete tasks; no resident/large-model or whole-CAD speedup assumed.
Do not optimize cold setup before the remaining breadth theme.

Optional already installed reference runtime: construct the identical box and
cylinder from recorded original frames, perform difference on copies, and compare
actual qualified owned output measures/geometry against analytic/reference volume,
bounds and boundary closest-point witnesses. Use only public available APIs;
if no material/history API exists, state the missing comparison rather than
inventing it. Reference success/counts do not certify owned history. Keep actual
copy/consumption, export warnings, errors/timeouts and reference costs separate.
No new reference dependency/build; no required alpha corpus waived.

## 5. Review, compatibility and publication

Review complete CPU construction/history as one invariant; independent oracle
and sampler source reviews follow actual green. Fix all valid findings, retain
rejected-finding rationale. No production mutants required if exact geometry/
identity/negative/late-failure assertions already cover the named fault classes;
selected actual production faults may be used on fleet if an assertion needs a
direct sensitivity check. No additional broad mutation/readiness claim.

Run affected existing body, intersection, arrangement, classification, adjacency
and runtime compatibility at appropriate placements after native consumers pass;
no new native sanitizer rebuild for Python-only ownership/construction. Preserve
all native hashes. Review actual artifacts and costs against claims after real
callers. Update README, user guide, roadmap/AGENTS/issue index, papers bibliography
index and return-pass notes; preserve external alpha gaps and deferred work.
Publish coherent feature commits throughout, merge PR without squash, retain
branch, sync clean main to fleet. After delivery, feature construction remains the
last uncovered breadth theme, with its distinct extrusion/revolution/fillet/
offset units. Do not silently broaden this Boolean into those operations.
