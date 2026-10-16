# Whole mesher: concrete numerical tasks and runtime ownership resolution

Proposed resolution to runtime review R1–R3, 2026-10-16. Design input only;
no production/test code or execution. This is part of the whole native mesher,
not a separately delivered accelerator or a new certification layer.

## R1: represented sites and source traces are distinct values

Use these conceptual native records; the later plan defines concrete C++ names:

| Value | Meaning and representation |
| --- | --- |
| `SourceParameter` | Canonical exact dyadic/rational on the original physical edge, with edge/definition identity. Existing binary64 knot/end words are exact dyadics. Sample subdivision parameters for reconstructed traces are dyadic; exact derived span-intersection parameters may be rational. |
| `TraceBranch` | Original coedge/use, trim authority/reconstruction policy, locator chart and integer period lift. For reconstructed uses the selected realization is the stored finite piecewise-affine UV trace, not an implicit inverse root. Authoritative original pcurves retain their original curve/map meaning. |
| `TraceSample` | Source parameter, branch ID and a deterministically chosen dyadic chart site. A locator may retain an optional candidate/root box, but that box is not required to define the selected trace. Whole-interval original-curve/carrier discrepancy is required. |
| `ChartSite` | Exact rational coordinates used by topology, plus site identity. It may be a chosen dyadic trace sample or an exact rational derived intersection. |
| `PhysicalNode` | Canonical actual binary64 XYZ, physical/source generator identity and source parameter; separate from any occurrence's chart site. |

For RECONSTRUCTED uses, analytic inverses or local NURBS solves only locate
candidate dyadic UV sites near the retained occurrence branch. Fixed dyadic
joints make incoming/outgoing uses close exactly; seam lifts and singular
quotients remain explicit. Define gamma_h(t) by piecewise-affine interpolation
of those stored sites at their stored original edge parameters. Exact topology
concerns that finite rational polygonal trace. It does not claim that sites are
exact roots of a projected inverse equation. There is no required Cbar corrected
spatial curve, exact root tube or continuation solver in this default policy.

Acceptance bounds original C_e(t)-S_f(gamma_h(t)) on every whole interval using
the original coefficients/formulas and separately bounds C_e against the actual
emitted spatial chord. Source branch/lift, loop simplicity, winding, hole/use
inventory and quotient checks still apply. Newton convergence or proximity at
samples cannot supply those acceptance facts. The reconstructed selected
polygonal realization has E_domain=0 relative to itself; its original spatial
edge/carrier discrepancy remains explicit. AUTHORITATIVE original pcurves retain
curved-domain meaning and require the selected polygon/curved-domain strip and
joint correspondence instead. Physical-plane chart certification remains a
separate chart-map obligation; a rounded inverse is not automatically an exact
inverse of the original source.

Every backend uses the same host-selected finite sites/parameters. A locator can
use an isolating box as an optional numerical method, but source acceptance is
the whole-interval discrepancy plus branch/domain checks, not existence of an
unrepresented exact root. Distinct source uses remain distinct even when chosen
coordinates coincide. Degenerate topology uses the declared quotient/feature
rules, not a proximity weld. A degenerate original edge takes a constant/quotient
path before any formula divides by its parameter extent.

## R2: small, explicit numerical task family

Select two rational bound tasks and one optional analytic-certificate task, plus
the existing approximate first-jet consumer. They do not solve source continuation,
feature contact, topology, chart selection or point insertion on the GPU.

`RationalTriangleResidual` takes a source surface/span-root ID, exact rational
triangle coordinates in that span's original chart, actual three emitted XYZ
words, the applicable remaining positional allowance, and source/schedule/cell
generations. For a triangle crossing spans the host enumerates actual covering
subcells and supplies the restriction map into the original triangle's same
affine physical interpolant. A containing rectangular bound can be a separately
named conservative control, not an implicit replacement for actual coverage.

`RationalCurveResidual` takes a source curve/span-root ID, exact original
parameter interval, actual two emitted XYZ words and its positional allowance.
The first implementation's parameter-matched linear chord map gives a rational
residual after degree elevation. This is a safe bound primitive, not a declaration
that parameter-matched bisection is the whole curve algorithm. An independently
proved monotone geometric-line/graph specialization avoids needless refinement
of nonlinear straight curves; its map/proof belongs to the feature geometry unit.

For both rational tasks define, over the task's declared cell parameter domain,

    R = N - W*L
    w_* = min_j W_j > 0
    accept iff for every i: sum_axis R_i_axis^2 <= tau^2 * w_*^2.

The R coefficients are those of the exact original-coefficient restriction,
elevation and multiplication, not arbitrary interval endpoints. The nearby
anchor is subtracted by an exact algebraic identity. Coefficient extraction
from original knots is exact rational arithmetic, or its interval image encloses
that exact expression. `tau` is a common host-defined nonnegative rational
allowance already accounting for the domain/strip rule; an outward rounded
approximation to an irrational subtraction is not silently treated as exact.
For a combined bound with nonrational terms, the host supplies a canonical proven
lower allowance or keeps the combined decision on the host.

The inequality is sufficient, not necessary, for geometric accuracy. Failure
means this bound does not accept the cell, not that the source has an unavoidable
geometric error. Equality accepts. `w_*` must be positive as an exact coefficient
quantity; if an interval cannot prove positivity, use the host exact computation.
With surface degree at most three in each axis, the ordinary triangle residual
degree is at most seven, or36 triangular coefficients per coordinate. The
degree32 curve task has residual degree at most33, or34 curve coefficients per
coordinate. These bounded buckets give an actual GPU kernel/data-layout target.

The device need not own arbitrary integers. It reads outward binary64 enclosures
of immutable original-span coefficient expressions, exact-input descriptors'
enclosures, and actual XYZ/allowance words. It performs enclosed restriction,
residual construction and inequality filtering. Return:

- `PROVED_ACCEPT`: every exact comparison is enclosed on the accepting side;
- `PROVED_BOUND_FAIL`: a coefficient is enclosed strictly on the failing side;
- `UNRESOLVED`: overlap, uncertain positive denominator or arithmetic exceptional
  condition, with task ID/status. This never means accept.

The host CPU policy filters the same task with the same mathematical inputs.
An unresolved rational task computes the exact rational residual and squared
comparison, clearing only known-positive denominators, under its owner budget.
The P100 policy uses the same host fallback. Thus interval tightness changes
cost/fallback counts, not acceptance, splits or output. Exact-source expression
IDs and immutable input words identify tasks; interval endpoints alone do not
identify the geometry. No second CPU recomputation is required for every
already-certified GPU sign; numerical qualification attacks the kernel itself.

`AnalyticCertificate` is narrower than an arbitrary GPU analytic solver. The host
geometry compiler may produce canonical certified dyadic/rational constants for
a named conservative analytic cell-bound formula, retaining the original carrier
and chart maps. CPU and device then evaluate/filter the same defined rational
comparison using those constants. Examples can include radius/derivative norm
upper bounds and exact cell extents, with all subtraction/rounding effects
included by that formula. If no such implemented certificate exists, actual
analytic bound decisions stay on the host. This is explicit admitted routing,
not silently claiming analytic certified GPU construction. Analytic first jets
can still use the existing approximate CPU/CUDA consumer under their evaluation
error contract. Neither route treats ordinary sin/cos output as an enclosure.

The shared task descriptor contains kind/bound-revision, source/root ID, cell/
edge ID, exact-input-table ranges, actual output-point range, allowance ID,
source generation, boundary generation and task ordinal. Result records contain
the same identity, status and optional useful interval/work data. All variable
buffers have explicit element counts and bounded owner lifetimes. Device buckets
are private packing views of those values, not a second source schema.

Successful NativeMesh geometry also requires backend-independent proposal and
priority inputs: host-defined deterministic site locations, canonical physical
XYZ words and error/shape priority values, never whichever GPU interval endpoint
was returned first. First-jet outputs used only for display retain their own
numerical comparison scope. A GPU-evaluated value may define a canonical mesh
word only if its enclosure proves the common rounding decision; otherwise the
host canonical evaluation supplies that word. Worker scheduling or GPU batch
completion order never determines which proposal is inserted next.

### Required arithmetic profile

Use IEEE binary64 operations with explicit host outward endpoint formulas and
CUDA directed double intrinsics. New host interval code requires round-to-nearest,
no excess precision, no FTZ/DAZ and no unproved reassociation/contraction. Verify
the active environment in each participating worker or disable its filters;
never mutate a shared process environment as a hidden geometry operation.
Explicitly modeled FMA is possible, but cannot be substituted into a proof for
separate operations. CUDA build retains strict division/sqrt, no fast math or
FTZ and explicit instruction semantics for directed endpoints.

An exact coefficient/input too large for a finite enclosure produces unresolved,
not an infinity accepted by accident. NaNs, zero-crossing denominator intervals,
indeterminate infinity arithmetic and unmodeled underflow similarly return to
host exact work. Correctly enclosed subnormal intervals are valid; ignoring tiny
terms is not. Canonical exact coefficient decisions and tie policies, stable
host trace/site choices and deterministic request rounds make successful CPU/
P100 results identical regardless filter tightness. Device work/memory failure
is a typed run failure unless an explicitly selected, reported fallback policy
requeues the same task on the host. Never label a silently CPU-only result P100.

## R3: task graph, memory reservations and cancellation

`MeshingRun` owns immutable SourceStore and original/span GeometryStore roots,
one bounded ready queue, the run resource allocator and optional GeometrySession.
Face/edge owners mutate only their own local restriction nodes, adjacency,
scratch and result. Shared compiled roots are constructed before dependent work
or published immutably once; no lazily mutated shared tree is left unassigned.
Later shared child caching would require its own explicit ownership design.

A face needing bulk numerical work packages a bounded batch and yields a
continuation. The continuation records the exact pending generation and owns the
task input/result buffers until completion. It holds no mutex across a yield.
CPU worker threads execute ready tasks; they do not all block waiting for queued
children. GPU completion makes continuations ready through the same coordinator.
Alternatively helping waits must be explicitly implemented; an ordinary blocking
future on every occupied worker is forbidden.

Before admitting a parent job, reserve its bounded active scratch plus the
maximum outstanding child/staging demand that parent is allowed to issue. Limit
outstanding batches per owner. The scheduler admits fewer parents when necessary
and always preserves a runnable child/completion path. Reservation failure before
admission can queue the parent; failure to fit a single admitted unit returns
resource exhaustion. The coordinator must not hold every byte in suspended parent
arenas while children wait for an additional allocation. Where yielding requires
more storage than reserved, release/recompute disposable parent caches before
issuing work; do not wait while holding resources needed for progress.

Each exact arithmetic/arena owner keeps a nonshared budget. A run-level resource
accounts actual reserved host blocks, pinned staging, live immutable results and
device buffers, including reusable arena high-water retention. Temporary and
result allocations transfer accounting on ownership transfer rather than being
counted twice or disappearing. A queued task handle is generation-checked before
using a recycled slot. Cancelled/stale outputs are discarded, never applied to a
new boundary snapshot.

On error/cancellation: stop new submissions; record task/source/stage/error;
complete dependency bookkeeping even for failed children; mark dependent results
unusable; drain outstanding CPU tasks and owned device streams; then release
their buffers and reset owner arenas. Capture all worker exceptions at task
boundaries. Do not unwind through thread entry points or leave a barrier waiting
for a worker that exited. A device session error poisons that session until
reconstructed. Keep the prior immutable published mesh; temporary output never
acquires a successful complete header or replaces it before direct construction
checks and successful close/atomic rename.

Deterministic successful geometry does not guarantee which racing resource/error
is first observed. Report cancellation/resource errors honestly; deterministic
error ordering is promised only if explicitly implemented. Resource limits bound
work and memory, not an arbitrarily short wall timeout. Research qualification
remains the existing independent actual-output mechanism off the production path.
