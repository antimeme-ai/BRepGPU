# Native mesher whole-design review: runtime and numerical interfaces

2026-10-16. Independent review of the proposed uncommitted
`docs/NATIVE_MESHER_DESIGN.md`, following research commit `0d20314`. Read the
whole design and checked the existing predicate API, source/model contracts,
display session lifetime and native rigor profile. Review only; no product/test
changes, builds, CAD or benchmark execution.

Verdict: **changes requested before plan**. The selected native ownership,
source semantics, geometry-aware refinement, shared feature authority and
CPU/P100 placement are coherent directions. Three interfaces still hide
implementation choices that can change correctness or deadlock the operation.
The resolutions below are bounded additions to the whole design, not separate
ports or a new certification system.

## R1 — implicit trace points cannot enter the current rational predicates

Blocking, numerical/source-interface seam. Proposed source realization lines
86–99 defines a continuous selected inverse trace by projected carrier
equations. Its values are generally algebraic/implicit (analytic charts may be
transcendental). Lines173–179 similarly obtain physical-plane corner UVs through
an inverse branch. Lines195 and228 then speak of exact schedules/chart domains,
but `bgcdt::Predicates` accepts exact integer coordinates after rational
normalization, not implicit inverse roots. Root isolation alone does not produce
an exact rational root, nor an exact sign on the real-valued trace sites.

Resolution: explicitly distinguish source parameter identity, continuous trace
and isolating enclosure, chosen representable chart site, and the bounded map
between them. Select rational/dyadic chart coordinates deterministically inside
the admitted branch, prove/retain their displacement and include it in the same
domain/cell bound. Exact topology then concerns that declared polygonal domain;
it does not claim its rounded vertices equal implicit source roots. Preserve
trace parameter correspondence and per-use branch/quotient identity in native
values/results. Alternatively choose indirect predicates on implicit sites and
account for that much larger implementation unit; do not leave the choice until
an inverse-map port discovers it. The chosen schedule parameters can remain
exact dyadics/rationals while collar exits are isolated and deterministically
bracketed rather than called exact roots.

Degenerate original edges with a=b also need a declared constant/quotient branch
before entering Cbar_e's regular `(t-a)/(b-a)` formula. Sent this adjacent issue
to the geometry/oracle reviewer to avoid conflicting ownership.

## R2 — define the shared numerical decision, not just interval capability

Blocking, CPU/P100 contract lines305–314 and residual lines114–125. The design
correctly forbids libm guesses and calls for a common residual inequality. It
must define what exact quantity the filter encloses and how unresolved values
are evaluated. Otherwise one backend's looser interval bound can select extra
splits and mutate topology while both paths claim the same decision contract.

Resolution for the rational residual unit: define acceptance on the original-
coefficient rational residual net, e.g. every coefficient satisfies
`sum_axis R_i_axis^2 <= epsilon_local^2 * (min_j W_j)^2`, with positive weight
lower bound and explicit source-to-cell/restriction semantics. Directed interval
evaluation filters that comparison; overlap triggers the owned host exact
rational/integer comparison, with positive denominators cleared. Equality has
a named accept/tie policy. Do not use an approximate cached residual net as exact
input. Derived original coefficient enclosures must be propagated, or exact
coefficients recomputed for fallback. Other charts must name their own defined
bound and unresolved outcome; no claim that this polynomial comparison handles
an arbitrary implicit/trigonometric source map.

State the host/device arithmetic profile in the design: IEEE binary64,
rounding/FTZ/DAZ/excess-precision and contraction assumptions, explicit directed
CUDA intrinsics or proved outward host operations, and behavior for overflow,
NaN, subnormal-sensitive denominators and invalid lower bounds. No global host
FP environment changes behind other workers. Existing predicate filters have
runtime guards but do not automatically guard a new geometry interval unit.
Typed unresolved/numerical failure is legitimate; an uncertain acceptance is not.

This is a numerical interface specification, not a requirement to implement all
endpoint formulas inside the architecture document. The subsequent unit design
can derive those formulas against this selected contract.

## R3 — nested task/resource waits and shared restriction ownership

Blocking, concurrency/lifetime seam lines160–165 and289–312. Face owners may
submit child bound/evaluation tasks while retaining large scratch arenas.
If every worker blocks awaiting queued children, the bounded pool deadlocks.
If parent arenas reserve the entire run budget and child buffers need additional
reservations, help-while-waiting alone does not solve the memory deadlock.
GeometryStore's owned mutable restriction trees also need a single writer or
immutable publication policy when multiple faces reference a carrier.

Resolution: immutable shared original/span roots; face/edge-owner local mutable
restriction nodes or versioned immutable child publication with specified
ownership. The scheduler uses continuations or helps ready child work while
waiting; no geometry worker blocks all execution slots behind its own queued
children. Reserve bounded child/device staging capacity before admitting parent
work, or yield/release parent scratch to meet a documented reservation policy.
Task buffers outlive their completions. Cancellation stops submissions, marks
dependent generations unusable, drains in-flight device work and only then
resets arenas/releases source spans. Capture worker exceptions into task results
and complete dependency barriers on every error path. Preserve the previous
immutable published result atomically.

These rules do not require adopting a task library. The existing display pool
is a useful bounded batch pattern but not an implementation of nested throwing
geometry tasks. Per-owner exact budgets plus aggregate reserved bytes are
otherwise correctly selected in the current design.

## Nonblocking clarifications for plan ownership

- Keep raw source trim authority/reconstruction policy, selected trace/map,
  source mismatch and curved-strip associations explicit in the semantic result
  schema. Do not grow a receipt protocol. Generalized cell-domain/map→physical-
  facet/quotient mapping is already correctly required at poles.
- Initial binary source parser must preserve the existing surface<=3 and
  curve<=32 admission. GPU degree buckets need an explicit admitted host route
  for unsupported device buckets; source admission must not shrink silently.
- Choose hard-error versus declared CPU fallback when a requested P100 session
  cannot initialize or runs out of device storage. Report any fallback. An
  accelerated result must not silently become a pure CPU timing.
- Source/full-mesh decoding and deterministic little-endian output need checked
  sizes, exact-value representations and semantic ownership; BGMESH01 is
  correctly retained as a lossy derived display view.
- Deterministic IDs on successful runs do not imply deterministic first error
  when resource cancellation races. State which error ordering is promised.

The proposal correctly retains independent Python/Fraction actual-output
qualification off the product path, with native direct constructor invariants
and bound checks as production mechanisms. No new native certification layer is
requested by this review.

Root accepted the three interfaces as valid design gaps and requested a concrete
resolution proposal. Saved `papers/2026-10-16-whole-mesher-runtime-resolution.md`:
explicit source/trace enclosure/chosen rational chart-site values; two bounded
rational residual task kinds with exact squared-coefficient acceptance and host
fallback; narrow optional host-compiled analytic certificate routing; precise
task identities/environment/error behavior; immutable roots and owner-local
mutable trees; continuation-based child work with reserved completion capacity;
and drain-before-reset cancellation. No production/test edits or execution.

Root subsequently selected a simpler explicit reconstructed-trace policy in the
main design. Updated resolution R1 accordingly: reconstructed gamma_h is the
stored finite dyadic UV polygon; inverse/Newton work is candidate location only;
uniform original C_e-S_f(gamma_h) bounds and branch/domain checks supply admission.
No mandatory implicit root tube or Cbar construction remains. Authoritative
original pcurves retain their curved-domain strip/map obligation. Optional
locator isolating boxes are not confused with the selected geometry.

At root's request edited only `Native execution and P100 placement` in the main
design to select the R2/R3 mechanisms: two bounded rational residual tasks and
exact squared-coefficient acceptance, common exact allowances/equality policy,
certified CPU/P100 filters with host exact fallback, narrow analytic routing,
FP profile/error behavior, canonical sites/XYZ/priority decisions, immutable
roots/owner-local trees, yielding continuations, parent+child reservations and
drain-before-reset cancellation. No product/test changes or execution.

Rereview verdict for owned R1–R3: **ACCEPT for implementation planning**. The
reconstructed trace is finitely represented and compatible with rational exact
topology; the bound task has a defined common decision and actual bounded kernel
shape; runtime ownership/reservations permit progress without nested-pool or
memory-holding deadlock. Canonical host proposal/priority/XYZ words prevent
backend-dependent interval tightness or completion order changing NativeMesh
geometry. Detailed endpoint formulas, exact extraction representation and
allocator/task code remain implementation-unit work under these selected
contracts, not unselected algorithm boxes. Other reviewers own whole geometry/
refinement findings; this scoped acceptance does not override them.
