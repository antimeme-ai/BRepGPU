# Deliver the whole native mesher

Status: accepted after independent algorithm, geometry and runtime plan review.
Implementation follows the concrete unit procedures and interface reconciliation
below. The selected
[design](NATIVE_MESHER_DESIGN.md) governs every stage; this is one source-to-mesh
delivery, not three independent ports or a fallback to the Python mesher.

## Operation, ownership and native interfaces

The complete operation is

    original BGSRC001 bytes
      -> typed SourceStore and incident CSR
      -> original-coefficient GeometryStore
      -> settled shared BoundarySnapshot / chosen working charts
      -> exact domain + metric connectivity + bounded refinement
      -> settled actual physical contacts + accepted fixed-site optimization
      -> immutable NativeMesh
      -> full semantic binary and derived native display view.

CPU1, parallel CPU and CPU+P100 execute this operation with the same geometry
policy. No Python geometry/predicate/refinement/recovery/scheduling callback
belongs in it. STEP extraction stays the existing isolated foreign adapter.
Independent research readers and authored fixture generators may use Python.

The implementation directory is `native/mesher/`. Use the existing strict
Linux Clang18.1.3/C++20 profile. CUDA is an optional separately compiled sm60
module with a small owned POD boundary; a CPU build must not require CUDA.
No new foreign production dependency or copied reference implementation is
selected. All product/native execution, instrumentation and performance work
take place on the fleet, in a new isolated checkout.

The shared native values have the following roles. Unit plans supply concrete
member layouts; reconcile them before code so two owners do not independently
invent what a site, trace or cell means.

| Value | Owner and contents |
| --- | --- |
| `SourceStore` | Immutable original bytes and all13 original typed sections; ID-to-dense-slot indexes, definition ownership and face/edge/use incident CSR. Preserve source digest, original coefficient words, source units/tolerances, styles, diagnostics, parameter maps and every occurrence. |
| `ExactWords` / local arithmetic values | Immutable canonical integer/rational words may cross owners. Arithmetic imports them into a single owner's budget; mutable budget-owning integers never become shared roots. Chosen topology sites and generated midpoint parameters are exact dyadics; clipping/solves may use local rationals. |
| `GeometryStore` | Immutable original-extracted roots, their original knot/parameter domains, anchored homogeneous control values/enclosures, analytic frames and compiled profiles. Source coefficients remain inspectable. Local restrictions have a face/edge owner. |
| `WorkingChart` | Original UV, admitted physical-plane inverse descriptor, or an implemented analytic quotient map. Stores source projection/parameter map and branch/lift identity. It is not a boolean saying an approximate UV is exact. |
| `BoundarySnapshot` | Ordered shared physical source-edge parameters and canonical actual XYZ, common face-vertex joints, per-use chosen-chart sites/trace maps, quotient occurrences and generation. Reconstructed polygons and authoritative curved domains have different explicit trace kinds. |
| `FaceResult` | Chosen-chart sites, source-linked physical generators, positive coverage cells/maps, actual facets, ordered original boundary occurrences, settled local bounds and boundary generation. No unowned private physical boundary node. |
| `NativeMesh` | Complete source/definition inventory, canonical nodes, all edge/use/site/cell/facet/quotient/ancestry relations and quality. No live task/scratch pointers, opaque foreign handles or pending decisions. |
| `MeshingRun` | Source/geometry lifetime, owner jobs and continuations, aggregate reservations, numerical batches/device session, request rounds, complete publication and diagnostics. |

Stable source IDs and dense slots are distinct types/fields. All index narrowing,
count arithmetic, section allocation and ranges are checked. Approximate
coordinates may propose a site but never silently replace its exact descriptor.
Canonical actual XYZ and proposal priorities come from the common host policy;
device completion order cannot choose geometry. The result format contains
geometry policy/quality, while timing/backend observations go in TSV reports.
Output exact tables contain only values reachable from semantic mesh records,
reindexed in canonical record/field order. Temporary extraction/filter/fallback
caches and their pool allocation order never become extra geometry rows.

The concrete procedures form one plan with these subordinate unit documents:
[values, numerical execution and runtime](native-mesher-runtime-plan.md),
[original geometry and correspondence](native-mesher-geometry-plan.md), and
[shared features, topology and contacts](native-mesher-topology-plan.md).
Their interfaces use the same SourceStore/ExactWords/chart/site/cell semantics;
root integrates their valid review changes rather than treating them as
independent implementation choices.

## Dependency order and integrated milestones

1. **Native values, source and arithmetic.** Establish the shared header
   contracts and strict source admission; implement owned local exact arithmetic,
   immutable cross-owner words and interval primitives. RED cases come first.
   This milestone is preparation, not a mesher or watch result.
2. **Original geometry and real numerical tasks.** Compile original rational
   curves/surfaces and analytic formulas, execute actual residual tasks on CPU,
   then P100, and implement the selected correspondence/plane/singularity
   profiles. Use literal original-coefficient answers and independent families.
3. **The one meshing authority.** Construct shared feature schedules and chosen
   domains, exact admission/connectivity, mutable refinement and actual contacts.
   It uses the real geometry interface. No legacy Python fallback or production
   test adapter may fill an unimplemented invariant.
4. **Native caller and complete result.** Integrate bounded scheduling, request
   rounds, semantic output/readback and display views. Exercise complete small
   source assemblies under CPU1/parallel CPU/P100 before large consumers.
5. **General workload and actual consumer delivery.** Resolve rigorous tests,
   adversarial code reviews and the predetermined performance families; only
   then run the whole watch and produce its actual assembled/exploded graphics.

Implementation can overlap independent conceptual owners once the preceding
interfaces exist. The numerical owner owns both CPU/P100 evaluation of the
same residual predicate. The geometry owner owns source extraction, charts and
correspondence. The feature/face owner owns the full shared-boundary/connectivity
invariant. Root owns native source/result/application integration and the
independent actual-output experiments. Within an invariant, sequence its work;
do not split its failure paths into uncoordinated patches.

Each meaningful design, RED, implementation, review-fix and qualified result
milestone gets a coherent commit on the feature branch and is pushed. Reviews
inspect actual code; valid findings are fixed and the affected direct checks
rerun. Preserve substantive history when merging. An unfinished unit may be
committed honestly but cannot be presented as complete meshing.

## Source and immutable geometry preparation

Port the strict BGSRC001 schema into native values directly, including all row
lengths, reserved words, knot/control relationships and typed ownership. Keep
the existing surface-degree<=3 / curve-degree<=32 limits. Preserve finite
original analytic charts and their axes; source spans are derived hints until
original extraction is established. No NURBS periodic metadata is inferred.
The endpoint association is already encoded by `edge.parameter_range` following
`vertex_ids`; `curve_same_sense` must not reverse it a second time.

Build source indexes/incidence once. Do not rescan all definitions, uses or
carrier spans for each edge/cell. Reject duplicate/foreign IDs, malformed source
ownership, invalid knots/weights and source positional discontinuities under
the selected policy. An unavailable chart/map/singularity is typed unsupported,
not a dropped source entity. Byte/count/arithmetic limits fail before unchecked
allocation. The source codec admits retained existing fixtures and the watch
source without source-specific exceptions.

Original homogeneous BSpline extraction, interval/exact arithmetic, triangular
restriction, analytic bounds, correspondence profiles and finite plane/pole
admission are specified constructively in the geometry and numerical unit
plans. A compiler root records the original source expression, not just a
rounded extracted net. Rational residuals use the actual emitted facet map,
including source canonical node departures and XYZ rounding exactly once.

For cells crossing knots, use a UV span index, exact rational clipping against
the intersecting original rectangles, and triangular subdivision of each convex
intersection. Restrict the same original emitted affine map on every subpiece;
never substitute newly evaluated surface corners. C0 is covered without an
imaginary exact output crease graph or a C2 theorem across the knot.

### Carry positional tolerance through actual assembly transforms

The source admits approximately rigid matrices; their actual values and composed
paths need not have norm exactly one. Keep the world-space epsilon claim by
deriving definition-local allowances, rather than silently treating them as
exact isometries. For each source usage path, compose its affine matrix T=(A,b)
as exact rationals from the retained binary64 words. Round the12 stored instance
entries once, nearest-even, giving T_hat. Preserve the original path in INSTS.

Prepare finite source chart envelopes before computing these allowances. Include
all true natural NURBS roots, all original spatial curves and vertices, and
per-face analytic envelopes containing the supplied finite hints, whole original
pcurve ranges/lifts and supported spatial-inverse coordinate ranges. Source
sphere/torus images have a global finite frame/radius box; plane/cylinder/cone
images use those explicit finite coordinate envelopes. Add declared ideal pole/
apex cap ranges before evaluating the box. Do not truncate reused NURBS carriers
to the first face's adapter uv_bounds. A rational positive homogeneous hull or
the enclosed original analytic formula then yields a finite definition box B0.

Every actual reference/final domain, recovery site and carrier-evaluated
endpoint/restriction hull must stay within its prepared source envelope.
Auxiliary PL-map supports may extend outside a natural parameter rectangle;
no carrier evaluation occurs there. Domain displacement uses the total ordered
UV movement and carrier derivatives on the convex endpoint hull inside the
natural rectangle, including one-sided derivatives across continuous knots.
A candidate requiring carrier evaluation outside requests explicit envelope
expansion; recompute B0, transform errors and all local allowances, invalidate
the affected definition generations, then restart before accepting geometry.
This is a source-preparation transaction, not an unchecked assumption about
tolerance-dependent sites. A natural-domain NURBS excursion remains unsupported,
not an expansion beyond its original geometry. Physical-plane auxiliary XY
supports may lie outside Q without evaluating an inverse there; source/final
domain points must remain in Q as the geometry plan requires.

Set B=B0 expanded by requested epsilon. With the enforced envelopes and local
allowance capped by epsilon this contains actual emitted nodes and their affine
facets. From exact G=A^T*A obtain

    kappa >= max(1, sqrt(max_i sum_j abs(G_ij))).

Use an outward dyadic square-root enclosure. This Gershgorin norm bound stays
near one for nearly rigid matrices, including general rotations. Bound transform
storage error over B: for each axis take

    d_i = abs(b_i-b_hat_i) + sum_j abs(A_ij-A_hat_ij)*max_abs(B_j),
    eta >= sqrt(sum_i d_i^2).

Then the exact instantiated source and stored-matrix mesh differ by at most
`kappa*tau_local + eta`. Set each definition's local positional allowance to
a canonical dyadic lower bound on the minimum `(epsilon-eta)/kappa` over all
its instances, capped by epsilon. Its nodal source-residual allowance similarly
scales by kappa under its declared source-only comparison; no matrix-output
rounding is charged twice. Unplaced retained definitions use kappa1/eta0.
Nonpositive remaining allowance is an explicit precision/unresolved outcome,
not a reason to discard that instance or enlarge epsilon. Store canonical
definition allowances as semantic components; keep backend filter endpoints out.
The independent reader can recover each source composition and actual output
matrix from the retained path and instance row. Rendering arithmetic/projection
has its separate existing consumer error contract.

## Shared features and face construction

Settle original source-edge interval forests and canonical endpoint/generator
identity before face construction. Choose the face working chart first. Common
face-vertex occurrence joints close selected reconstructed polygons exactly;
seam lifts/quotients remain separate. Locator convergence proposes chart sites;
whole-original-geometry bounds and explicit maps decide admission.

Preserve the old recovery setting's actual meaning: `node_source_residual` caps
the original vertex/curve target versus the original carrier evaluated at its
stored chosen original-UV recovery parameter. It is a point condition, not a
whole-curve closest-distance promise. Source evaluation is exact/enclosed; the
chosen dyadic parameter is not called an exact inverse. Actual emitted XYZ
rounding remains in the actual-cell/chord bound. The whole original curve versus
selected polygonal carrier-trace approximation uses the positional allowance,
not this smaller nodal cap. Otherwise a circular trim on a plane would be
overrefined by treating chord sagitta as source recovery mismatch.

Default trace/edge correspondence is parameter-matched. Checked monotone line/
angle profiles compare images through their stated maps, preserving original
parameter associations. They must not overrefine geometrically straight curves
solely because their parameter speed is nonlinear. Authoritative curved trims
retain their own graph-strip and jointly owned vertex-neighborhood obligations;
unresolved tangent sectors fail explicitly rather than redefine the domain.

Per-use authority can be mixed on a face. Select `HybridPullback`: retain each
authoritative original arc and define each reconstructed reference arc as B^-1
of its selected polygon, using the nodal fans at its authoritative joints.
After B, reconstructed uses are exactly selected polygons; apply tapered T
strips to authoritative uses only, excluding reconstructed polygons except
fixed shared endpoints. Store the finite B/T descriptors and reference policy
in DOMMAPS_/SUPPORTS. Charge ordered source-domain displacement and inverse
movement in source/reference-trace comparisons; no mixed star gets a free
E_domain0 rule. The geometry unit gives exact fan/inverse equations, analytic
source-point descriptors, Q containment and direct mixed-domain oracles.

Admit complete simple polygon domains with a closed-segment index for exact
crossings/adjacent overlap and indexed half-open ray winding for hole ownership.
Avoid carrying the old quadratic hole-pair loop into mandatory native admission.
One winding query per hole over all other loops can reject outside/nested holes
after boundary nonintersection is established. Record candidate work honestly;
adversarial overlapping boxes do not inherit a universal NlogN claim. The initial
ordinary profile has one declared outer; multiple outers return typed unsupported
with their complete inventory until a coherent component construction exists.

Initial connectivity calls the existing typed direct CDT core with exact T0*q
sites and unit diagonal metric. No per-face binary pack/decode. Build compact
mutable triangle/edge records with incident handles, generations, free slots
and local update stencils. Append-site predicates must support newly generated
exact dyadics; the old immutable initial predicate table is not misused as a
dynamic API. No global mesh-site denominator LCM is introduced.

Refinement follows the reviewed fixed-metric circumcenter/LEPP algorithm.
Circumcenter candidates stay in their failed target, or its unprotected edge
with both incident lineages unspent. A spent flag never resets within the
generation. Exact midpoint transactions replace all terminal incidents; no
ordinary refinement flip changes the forest. Requeue only changed geometry and
affected contact neighborhoods, invalidating old generation handles.

Original-boundary requests go to the shared edge owner and bisect a master
interval. Apply deduplicated requests in deterministic rounds and restart
incident face generations from the new snapshot: the new source node generally
does not lie on the old UV chord. Retain immutable geometry roots and unaffected
completed face results. Boundary error/precision floors require explicit
unresolved/infeasible outcomes, not increasingly desperate private samples.
Each request round has a complete canonical horizon: every active face in the
definition/dependency component reaches its first request yield or settled
bounds horizon before requests merge. A quick worker cannot cancel a slower
face before its deterministic request arrives. Contact epochs likewise consume
the complete stable delta frontier; optimizer commits have one stable order.

Physical contact uses an actual-facet BVH and exact binary64 triangle/contact
classification, including coplanar overlap, T junctions and incident pairs that
overlap away from their declared common simplex. Sharing a vertex does not
exempt the pair. Work is within definitions, not across assembly instances.
Contact failures request local refinement with finite resource bounds. The
operation promises an embedded bounded realization, not detection of every
sub-epsilon input self-intersection.

Only after bounds/contacts settle may fixed-site metric-shape flips run. Exact
convexity, source constraints, accepted replacement bounds/contacts and strict
once-rounded potential decrease govern each transaction. No refinement is
interleaved. Publish only current face/boundary/contact generations.

## Numerical execution, resources and output

The shared exact rational coefficient inequality determines CPU/P100 bound
decisions. Filters return proved acceptance, proved bound failure or unresolved;
unresolved rational comparisons use the owned host exact computation. Source
weight positivity and equality policy are explicit. The first real device tasks
are curve and triangle residual restriction/comparison, not merely displaying
a CPU-produced mesh. Analytic decisions stay host-authoritative unless their
named conservative formula has canonical certified rational constants.

Keep immutable root/control tables resident and batch bounded kind/degree tasks.
Include task/cell/source/boundary generation in results. Per-owner continuations
yield numerical work rather than occupy every worker awaiting their children.
Reserve parent scratch plus permitted child/staging capacity before admission;
aggregate accounting includes retained arenas, pending buffers and device memory.
No shared mutable exact Budget, mutex held across yield, or reclaimed input view.
Cancellation stops submissions, completes dependency bookkeeping, drains device/
CPU work and only then releases buffers. Poisoned devices fail unless the caller
explicitly selected a reported host fallback policy.

Write the full semantic result with explicit little-endian fields and counted
sections, retaining original bytes, full physical/source/occurrence/cell/map/
quotient relations. Reject malformed references on readback. Write to a run-owned
temporary path, close successfully, then rename atomically. A failed run leaves
the previous published result intact and never emits a successful partial mesh.

Build BGMESH01 as a derived native display view. All vertices set the stored-XYZ
flag so renderer evaluation does not silently move the actual meshed geometry.
Source normal/first-jet evaluation remains a separate numerical display consumer.
Retain per-corner UV/orientation/side data and one definition's geometry across
instances. Source/style/material and exploded label layout are native values;
Python must not return as an obligatory source-to-image preprocessing worker.

The CLI reports stage wall times, work, output density, actual CPU/GPU route and
peak reserved resources as TSV. Separate wall from summed overlapping worker
time. Setup, upload/download, synchronization, fallback and destruction belong
to complete-call costs. No short shell timeout stands in for a mesher diagnosis.

## Direct RED/GREEN and qualification sequence

Each unit first writes tests that raise on a spec/model answer mismatch. Run
them on the fleet to observe the missing/wrong operation before implementation.
Use one direct mechanism for each claim, not a second certification system.
Native semantic construction checks remain on the product path; the existing
independent original-value reader is extended off that path for actual outputs.

| Family / fault | Direct answer and required observations |
| --- | --- |
| Exact arithmetic/filter errors | Literal integer/rational results and independently generated Fraction vectors; signed extremes, quotient/remainder identity, normalization, owner transfer, rounding ties, subnormals and explicit work limits. CPU/P100 filters must decide the same defined comparison. |
| Source/extraction loss | Original-knot independent rational evaluation and literal polynomial/Bezier answers; nonuniform knots, C0/C1, endpoint cells, positive weight variation, degree limits and invalid discontinuities. Derived net status is not the oracle. |
| Nonlinear planar map | S(u,v)=(u+u(1-u)/2,v,0) on the square, including nonlinear straight edge parameterization. Geometry is the literal unit square; the admitted plane profile should need two physical triangles for that face. Height-bubble perturbations must invalidate an inadequate plane bound. |
| Rotated anisotropy | S(u,v)=(u,v,kappa(u-v)^2), predetermined kappa/epsilon sweep, rotations and dyadic UV reparameterizations. Literal residuals check geometry; density/work against isotropic ordering expose whether the selected metric actually helps. No optimality theorem is assumed. |
| Twist/weights | Bilinear saddle, nonplanar rational patches and highly varying positive weights. Analytic extrema/independent rational enclosures on actual facets expose missed twist, denominators and incorrect actual L. |
| Domain topology | Concave polygons, thin gaps, many holes, near-tangent valid joins, collinear chains, actual crossing/touching failures. Exact area/directed boundary chains/winding check the emitted cells; all original loops must remain. |
| Shared topology/quotients | Reversed traversal, multiple incidents, periodic seam copies, sphere poles/cone apex, deliberately unresolved singular charts. Original keys/parameters, exact physical incidence/links and full source-domain coverage maps expose cracks, aliasing and deleted pole cells. |
| Source discrepancy | Shifted vertices/carriers and reconstructed pcurve disagreement, with both feasible and witnessed infeasible requests. Check actual original generators, selected chart policy and whole intervals. A large conservative bound alone is not an infeasibility proof. |
| Physical embedding | Separate/touching/crossing and coplanar triangle pairs, including shared-vertex overlap away from that vertex; thin separated surfaces. Independent exact actual-facet intersections and declared incidence distinguish valid contacts from hidden crossings. |
| Runtime/publication | Worker-count/batch permutations, delayed/stale generations, cancellation/device/resource failures and repeated runs. Semantic output must be identical on successful CPU/P100 policies; failures must preserve old output and release actual ownership safely. |

Use applicable pinned NVIDIA trim/cone/glue inputs, Manifold polygons and acquired
OCCT mesh cases under their actual semantics. Keep missing/unsupported upstream
cases and source-policy conflicts explicit. They remain alpha blockers where
the advertised alpha contract requires them; development does not rename them
green. No new alpha is selected by this implementation.

Run relevant release/debug/address+undefined profiles. Thread instrumentation
targets actual scheduler/shared-result races; source fuzzing targets malformed
external bytes. Formatting, clang-tidy and static analysis cover the new native
sources under the established rigor stack. Broaden checks after failures/new
changes, not after already-resolved claims merely to certify again.

Before the watch, measure sparse/adverse general families: local versus global
curvature, anisotropy, many holes, varied weights/span counts, shared occurrences,
large coordinates/precision floors and skewed face-size distributions. Retain
complete calls, work, memory, density, unfavorable outcomes and explicit limits.
Do not tune capacities, dispatch, source admission or tolerances to watch IDs,
names, dimensions or one observed bottleneck.

Then process all107 watch definitions and9162 source faces through the actual
native CPU1/parallel CPU/P100 operation at the declared quality. Check the full
actual result independently, report geometry/consumer costs separately and
compare actual compatible NVIDIA outcomes. A missing definition is a failure,
not a successful partial assembly. After geometry passes, produce assembled and
exploded labeled graphics with their real timing scopes. The pending README
NVIDIA reference-baseline backfill follows that watch delivery as requested.

## Plan acceptance and completion

Independent reviews accepted the reconciled procedures:
[algorithm](../papers/2026-10-16-native-mesher-plan-algorithm-review.md),
[geometry](../papers/2026-10-16-native-mesher-plan-geometry-review.md), and
[runtime](../papers/2026-10-16-native-mesher-plan-runtime-review.md).
Their counterexamples and constructive resolutions are part of the implementation
contract and direct checks, including ambient map supports, mixed trim authority,
pole orientation and the distinction between nodal recovery and positional error.

The unit procedures, schemas and tests must be reconciled into this plan and
reviewed adversarially before implementation. No pending API or singularity
construction may be replaced by an old Python callback. Resolve valid findings
in the owning unit and then review its actual integrated seam.

Completion means reviewed native code, meaningful GREEN direct checks,
qualified general CPU/P100 experiments and the complete actual consumer result.
An accepted design, a faster primitive, partial source inventory, pretty image
or a passing sampled-only probe is not that completion.
