# Discovery numerical code: independent memory/admission review

Review, 2026-10-14, of the complete numerical-owned unit:
[discovery_cpu.cpp](../native/discovery_cpu.cpp),
[discovery_cpu.h](../native/discovery_cpu.h),
[discovery_math.hpp](../native/discovery_math.hpp),
[discovery_check.cpp](../native/discovery_check.cpp), with
[canonical ABI3](../include/brepgpu_discovery.h) and the actual core/kernel caller.
Root's source checkpoint is `38d66c2`. This lane reviews physical storage,
indices, admission and failure flow independently of the exact-geometry oracle
lane. No native execution, test additions, numeric edits or commits by this
reviewer. No blocking defect found.

## Vector growth, complete trees and bounded temporary storage

Builder stores physical parent/node identities as u32 indices, not references
into vectors. `box()` constructs a local node and reads its existing parent
before pushing. After recursion and possible `nodes` reallocations, append
writes children through `nodes[id]`. DOP parent support reads use a fresh index
after `bounds.resize`; no parent pointer survives that resize. Roots are resized
once before patches and a reference to roots[slot] is stable throughout recursive
node/bound growth. Node-local Net/left/right/line storage is automatic and bounded
by the accepted split depth, independent of request count.

Root admission calculates complete depth-derived totals and byte products before
calling this builder; the builder also checks node cap, u32 index limit, root
capacity and bound-vector stride capacity. A degree-(0,0) patch contributes one
leaf; varying patches recurse to the requested total depth. Axis selection never
splits an inactive degree unless both degrees are zero, which already terminates.
Every successful internal append has both children. A failed split/enclosure/
append returns false with unpublished partial vectors; runtime refuses and destroys
them rather than publishing missing domains. Allocation exceptions propagate as
allocation failures, preserving core OOM handling. Slot narrowing to u32 is safe
under patch_count<=max_nodes<=1,048,576.

New cache elements are value-initialized to zero as the vector grows. Active
coefficients alone are assigned. Clearing vectors before another private build
and resizing with zero values prevents stale inactive Bernstein coefficients
from a previous degree/mode. Bounds have exactly node_count times 0/20/490 values
when runtime validates final builder shape. Mode enum3/unknown values are refused;
AABB does not allocate an auxiliary array.

## Local net and coefficient indexing

Net capacity is sixteen controls times four intervals. Degree bounds0..3 and
control_count=(p+1)(q+1) precede Net population. Both U and V splitter formulas
stay inside this capacity with the existing u-major, v-inner control indexing.
Left/right triangle endpoints remain in initialized original copies, and each
subsequent endpoint update addresses the degree-bounded line and appropriate
child control index.

Bernstein doubled degrees remain0..6. `choose()` arguments never exceed6;
its integer intermediates and numerator/denominator products fit unsigned.
Denominators are positive, enclosed factors must have positive lower endpoint,
and coefficient loops skip impossible product index pairs before addressing
controls. Canonical cache index is node*490+polynomial*98+2*(r*(2q+1)+s): at most
245 pairs per node. Query loops use the first K=(2p+1)(2q+1) coefficients in each
49-pair block and never divide inactive zero padding. C4 is checked positive
before division; negative C0 is retained. DOP accesses ten support pairs with
its exact20-value stride.

Interval multiply/divide checks all endpoint products/quotients rather than
allowing a NaN comparison to hide one endpoint. Denominator intervals require
finite ordered values and strictly positive lower endpoint. Construction checks
valid/intersected bounds before returning success; query checks finite ordered
boxes/cache intervals and finite nonnegative lower results. Unknown modes fail
at the shared query entrance before mode-dependent access.

## Original controls, source association and SoA ranges

ScalarCPU passes the original patch descriptor, still containing its global
control_offset, alongside the global controls pointer to the evaluator. Its
single-patch slot0 adapter does not reset or apply that offset twice. The
interval witness separately receives controls+4*original_offset exactly once.
The reviewed CUDA caller uses the corresponding retained DevicePatch offset.
Neither uses child interval nets or rounded displayed XYZ as the original carrier.

CPU/kernel callers validate selected slot then root index and root.patch_slot
against that requested slot. Traversal checks every popped node's slot against
the retained root slot before pruning. Logical u/v indices/levels remain data,
not pointer identity. Retained source descriptors associate the slot with the
existing body/entity/revision/chart metadata; no source identity is invented
from the physical node index.

CPU range indexes each output field as field*full_count+row while [begin,end)
only selects rows. All six integer outputs use the same global row. Runtime
checks 8*N output sizing before allocation, partitions disjoint ranges and joins
workers before publication. The range does not allocate/throw and empty ranges
do not dereference payloads. This is not a standalone validator of arbitrary
raw arrays: controls offsets/capacities, options and output spans are the owned
snapshot/admission precondition stated in the private header.

Similarly, non-AABB query cache pointers must name the admitted complete owned
cache. Public callers cannot import a tree or bind an arbitrary cache pointer;
runtime verifies builder shape and backend upload owns all actual arrays before
publishing. Node-count/cache-stride indexing is consequently safe under that
contract. A null/short raw cache passed directly to a private numerical helper
is outside the supported interface, not a promised per-query refusal. The
actual public capability/array/admission path supplies no such missing-cache
entry point.

## Stack, budget and counter invariants

The depth12 DFS stack has13 entries. Descending an internal node leaves at most
one sibling per prior level; both push slots are checked before writing. Child
indices are in range, distinct and greater than the current preorder index,
preventing backward cycles. Short-circuit level checks occur before `1u<<level`;
accepted levels are at most12 and their sum at most12. Center calculations use
bounded integer dyadic indices and exact power-of-two scaling. The helper does
not re-validate every child partition relation during each query; complete
partition is the owned builder/exact-oracle invariant.

Visits starts at1 for root lower evaluation; both child evaluations require at
least two remaining visits, so unsigned budget subtraction does not underflow
under validated max_visits>=1 and visits<=budget. Pruned roots are no more than
computed visits. Refinements never exceed64; each fixed projector call has a
bounded evaluator count, so evaluation accumulation is far below u32 overflow.
Arithmetic failure preserves already-performed counters while replacing all
fields with NaNs and clearing candidate/gap/coverage certification flags.
Invalid slots/targets have zero work counts. Failure outputs cannot masquerade
as valid certificates.

Root-first refinement is counted once. The root leaf's duplicate midpoint
refinement is skipped, including depth-zero varying patches and degree-zero
carriers. Limit reasons count withheld eligible work rather than fabricating
extra evaluations. Visit limitation returns the already-computed root lower,
not the minimum of a partial leaf set. Stack/tree invariant failure produces
ARITHMETIC rather than a supported budget fallback. Strict lower>incumbent
pruning retains equality. Full traversal needs an unpruned retained terminal
region before claiming coverage; a malformed all-pruned path fails.

## Environment and direct-check scope

Environment inspection is allocation-free and checks actual supported Linux
x86-64/FLT_EVAL_METHOD0/IEEE binary64, standard nearest mode, MXCSR nearest/
FTZ/DAZ/masks and x87 nearest/masks. It does not alter caller controls. Core
calls it before nonempty builder arithmetic (including CUDA context builders)
and on every actual CPU worker; failure joins/discards before publication.
Empty runtime execution deliberately skips arithmetic admission.

The owned direct numerical check restores modified fenv/MXCSR through RAII,
checks mode admission, uses dyadic plane truth, visit-cap fallback, constant/root
solve deduplication, a nonzero original-control offset, rounded1/3 upper behavior,
negative C0 and inactive padding. Its1/3 check attacks substitution of ordinary
rounded distance; the independent Fraction caller supplies the stronger exact
upper certificate. These mechanisms are complementary. This review did not
run them and does not upgrade their scope into a hardware or universal geometry
claim.

## Disposition

No required numeric source correction identified. Root/numerical owner were
notified before report completion. Final supported-host memory checks, actual
exports and exact output qualification remain root-owned execution evidence.
