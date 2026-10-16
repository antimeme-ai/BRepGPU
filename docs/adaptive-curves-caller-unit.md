# Complete original-source shared-curve caller

Status: implemented after scoped plan/source review and direct RED. The real
original-output caller passes CPU1/CPU4/both-P100 geometry, lifecycle and
publication checks; see results/adaptive-curves-2026-10-16/REPORT.md. This caller emits bounded shared
chains and occurrence traces; it does not fabricate a complete NativeMesh.

Grounding: pinned SmTess::TessellateEdge and OCCT EdgeDiscret::Tessellate2d
establish the physical-chain/occurrence dataflow being compared, while the
source/error contracts and independent checks are selected in
adaptive-shared-curves-unit.md. Reuse the owned patch caller's no-replace
publication, native resource ownership and explicit cold/warm elapsed scopes.
Source-derived IDs and original exact parameters survive output; pointers,
allocation offsets and profiler counters do not substitute for semantic keys.

Build native/mesher/adaptive_curves_main.cpp as brepgpu-adaptive-curves. Required
options are --source FILE, --definition ID, --output NEW_DIRECTORY,
--positional-mm VALUE, --node-source-mm VALUE and --threads N. Repeated
--positional-mm values request fresh sequential constructions in one retained
session; node-source allowance remains a separate explicit input. CPU is default;
P100 requires --backend p100, --cuda-module LIB and --device N. Explicit
--memory-mib, --max-nodes and --max-depth cap work. Validate finite positive
allowances, checked indices/count products and the session's actual depth limit.
CPU-only builds never load CUDA.

Decode SourceStore once. Select the requested original definition and compile
only its original spatial curves and every incident pcurve. SourceCache roots
for occurrence recovery/trace bounds are requested lazily from the retained
original source; the old eager geometry pipeline is not a prerequisite. All
definition edges and incident uses must be represented or the operation fails
with a named unsupported/exhausted/source outcome. Do not silently restrict to
successful faces or one coedge per edge. Coordinates and allowances are already
mm; units_mm is original metadata and does not rescale them.

The caller supplies default definition allowances and no narrower face overrides.
The native session owns their exact table through each query. A warm query starts
from the same base numerical policy and selects new chains/traces from source
roots. It may reuse demanded numerical records; it may not return a previous
finished result or inherit finer physical leaves. Original vertices remain fixed;
constructed point revisions and occurrence generation identify current values.

## Actual result artifact

Write one owned little-endian BGACRV01 artifact per query. Wire revision and
geometry policy, source digest and original definition ID qualify every record.
Serialize actual source-edge chains in literal edge direction, physical keys,
constructed revisions, actual binary64 XYZ, exact original t, sample paths and
original endpoint associations. Serialize every use's original IDs, source
authority, sense/map/lift metadata, loop/junction identity, chosen UV/chart
coordinates, mapped original s, physical-node references, interval keys,
implemented correspondence maps and separate source/trace/domain bounds.
Wire field order is finalized against the reviewed native header before RED.

Canonical rationals use sign, numerator/denominator limb counts and little-endian
uint32 limbs, as in BGAPCH01. Physical keys are definition/source derived;
original vertex rows may appear in several edge chains with different endpoint
parameters while retaining the SAME key/XYZ. Source IDs remain distinct even if
coordinates coincide. Bounds represented by native ExactRefs are emitted as
exact rational values, not rounded acceptance claims. Raw analytic source
coordinates remain original curve/use/mapped-s descriptors, never invented exact
UV from a rounded source evaluation. Every count/index/optional tag is checked;
no trailing or partial successful artifact is published.

Write queries.tsv with source/profile/backend/threads, requested allowances,
inventory and actual output counts, point/node/restriction/cache work and actual
promotion revisions,
trace/request rounds, actual host-device bytes and separately scoped memory.
Write timings.tsv for source decode, root compilation, forest/session/device
setup, query construction, actual output and session close. The external runner
measures process completion; external geometry checking is separate. Worker CPU
time and overlapping stage time cannot be summed into wall time. Output is a
fresh directory published only after every required artifact closes. Failure
removes only that caller's temporary directory; existing output is untouched.

## Single independent actual-output reader

Write independently authored external SourceStore fixtures and one Python
integer/Fraction reader of the REAL caller output. No production evaluator or
trace helper supplies expected geometry. Check complete source edge/use
inventory, literal endpoints, exact parameter/map coverage, ordered paths,
shared key/XYZ identity, constructed revisions and separate seam UV occurrences.

For polynomial/rational families, form exact original residual Bernstein
coefficients against the SAME actual emitted chord and verify a whole-interval
bound. Use exact original scalar image formulas for nonlinear straight curves;
do not mistake equal-parameter deviation for geometric chord error. Circle
families use an independent analytic interpolation/sagitta model with original
frames and endpoint departures. Test original node guards against ORIGINAL
V/Ce and carrier values; actual Q is not an oracle for that source residual.
Authoritative domain bounds remain separate from trace/physical chord bounds.

Select shared-plane, nonlinear stationary straight-line, positive weighted and
degree32, C0/nonclamped knot, descending-parameter/map-break, wire, cylinder-seam
and plane/cylinder circle families before performance observation. Include a
source discrepancy invisible at endpoints, a fixed original mismatch, metadata
units-only changes, malformed/missing inputs, resource refusal and no-replace
publication. Finite unsupported inverse/pole outcomes remain explicit.

Run CPU1, actual parallel CPU and both P100s on the fleet. Compare cold and
fine/coarse/fine/repeat output, plus a separate fresh coarse construction.
Require useful GPU evaluations/restrictions/bounds and retained numerical reuse
from actual counters. Backend parity tests deterministic placement; the
independent original-output reader supplies geometric truth. Scoped source
review and caller sanitizers address memory/publication faults. Stop once direct
affected checks pass; no additional certification or unchanged watch retry.

## Wire revision 1

Header is `<8sII8Q32s`: BGACRV01, wire1, numeric profile1, original definition
ID, boundary node-row count, edge count, alias count, physical-interval count,
use count, site count, use-interval count, original source digest. All following
integers are little-endian; there is no implicit struct padding. A rational is
`<iII>` sign/numerator-count/denominator-count followed by reduced uint32 limbs.
An optional value has a uint32 present flag followed by its payload when present.

Rows follow in header order. A boundary node writes generator(uint32), original
source/generation/ordinal/constructed-revision(uint64 each), XYZ(3 doubles), exact
t, path-bit count(uint32) and that many byte0/1 directions. An edge writes eight
uint64 values: original edge/curve/endpointVertex0/endpointVertex1, node first/
count and interval first/count; then original t0/t1(double) and four uint32
curve-sense/same-parameter/degenerate/reserved0. Aliases write original and
representative VertexIDs (two uint64).

A physical interval writes BoundaryKey(owner:uint64,interval:uint64,kind:uint32),
two boundary-node uint32 indices, exact t0/t1, exact parameter-chord bound,
optional exact image bound, correspondence(uint32), optional line map. Physical
interval order matches the boundary interval pool used by each edge range.
A line map writes exact covector3/curve-scalar2/trace-scalar2/original-t2, then
traversal(int32). Map payloads address retained ORIGINAL curve parameters.

A use writes original use/face/edge/carrier/loop IDs (five uint64), sense(int32),
reconstructed/map-kind/shifts2/authority (five uint32 words, signed shifts by
int32 bit pattern), sites first/count, intervals first/count and face/boundary
generations (six uint64). A site writes boundary node(uint32), original LoopID
(uint64), junction(uint32), original shifts2/selected branch2(int32 each), then
exact t/s/chosenUV2/chartXY2/node-source-upper.

A use interval writes BoundaryKey, sites2/authority/correspondence(uint32 each),
exact carrier-trace-to-chord/source-to-selected-trace/UV-component2 bounds,
optional exact authoritative-domain bound, optional line map and optional
cylinder identity. A cylinder identity writes original spatial-curve/carrier IDs
(uint64), orientation(int32), exact t2/chosenUV4/constant-v. No query table keys,
cache pointers or transient numerical pool offsets are written as source IDs.

A closed point request on a declared quotient still requires an original curve
evaluator. Select one REAL positive original span: analytic saved source domain,
Bezier[0,1], or the lower adjacent compact-knot span inside the true expanded
natural B-spline domain (first positive span at natural start). This reuses the
original/prepared separation and canonical source endpoint ownership described
in the numerical report; it is not a fabricated t±epsilon curve interval. The
physical forest still publishes two original endpoint occurrences and zero
positive intervals. No finite positive source profile means named unsupported.
Source endpoint/alias guards precede positional refinement, including wires;
adaptive splitting cannot cure a fixed original endpoint discrepancy.


Keys in BGACRV01 are qualified by the retained artifact/query. Cross-quality
logical sample comparisons use original edge plus exact t, as in the native
runtime contract; a transient per-forest ordinal alone is not a global key.
