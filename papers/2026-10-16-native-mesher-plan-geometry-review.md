# Native mesher implementation plan: independent geometry/source review

Scope: root `docs/NATIVE_MESHER_PLAN.md`, topology
`docs/native-mesher-topology-plan.md`, runtime
`docs/native-mesher-runtime-plan.md`. Static review only, no product/test edits,
builds, modeling or benchmarks. This is **not** approval of the geometry unit
I authored; its independent reviewers own that verdict.

## Finding P1: construct the transport box before consuming its allowance

The main plan derives eta and the definition-local allowance from a box B
containing original and selected-domain geometry, expanded by user epsilon.
The transport inequality itself is correct:

    T(S)-T_hat(L)=A*(S-L)+(A-A_hat)*L+(b-b_hat),
    world_error <= kappa*tau_definition + eta(B).

However B must be constructed before that allowance controls chart selection,
chosen sites and refinement; a description requiring the as-yet-unconstructed
selected domain leaves a circular interface. A locator/lift can leave an adapter
face-envelope hint, and the same carrier can be reused by another face.

Concrete resolution proposed to root: prepare a finite original geometry box
from all true NURBS natural roots, original spatial curves/vertices and admitted
analytic chart envelopes. Require every chosen site/cell to stay within those
envelopes. A needed expansion triggers recomputation of eta/local allowance and
invalidates affected definition generations before further acceptance. An
equivalent single larger source-derived envelope is fine. Never bound the true
NURBS roots by the first face's adapter uv_bounds.

Reread the actual root resolution: B0 now includes all true natural NURBS roots,
original spatial curves/vertices, finite per-face analytic hints plus full
pcurve/lift and supported inverse-coordinate ranges, and declared ideal pole
caps. Original homogeneous hulls/enclosed formulas produce the finite box.
Chosen sites/domains/original-UV supports are required inside the prepared
envelope. Expansion recomputes B0/eta/allowances and invalidates affected
definition generations before acceptance; NURBS natural exits remain unsupported.
Auxiliary plane-chart XY supports need no inverse outside Q. B=B0 expanded by
epsilon contains accepted actual facets because local error is capped by epsilon.
This removes the circularity without treating face hints as original truth.
**P1 resolved; ACCEPT.**

## Accepted mechanisms within this review scope

- Original source rows/words, dimensions, parameter maps and derived-span status
  survive the strict native reader. Explicit metadata is required for NURBS
  periodicity. Positive weights alone do not assert regular charts.
- Physical key/edge ownership is definition-local, while immutable source
  geometry can be shared. Instances of one definition reuse its geometry.
  Descending edge endpoints map once to vertex_ids, independently of coedge
  traversal and curve_same_sense. Higher/open incident multiplicities remain
  explicit, and actual contacts still apply.
- A complete request horizon prevents a quick worker's first boundary request
  from canceling a slower participant before its deterministic request. Ordered
  pending-cell priorities, complete contact epochs and stable optimizer commits
  address backend/completion-order dependence directly. Resource failure is not
  promised to choose the same first error across schedules.
- Exact dyadic midpoint topology sites remain distinct from rational clipping
  pieces. RationalTriangleTask uses the original actual affine facet on clipped
  subcells, with no newly rounded virtual XYZ. Source extraction and interval
  filters enclose the same coefficient expression; GPU success does not require
  a redundant exact pass. Task generations/lifetimes are explicit.
- The actual-facet contact classifier uses emitted binary64 words as exact
  values. A shared node/edge only permits its declared common simplex; extra
  intersection is still forbidden. Closed incidence is not mistaken for physical
  embedding or detection of every hidden source intersection.
- Source-to-world transport uses exact composed source matrices and the actual
  once-rounded output matrix, with a near-one Gram/Gershgorin norm bound. The
  source-only discrepancy path does not acquire matrix-output rounding twice.
- Immutable full mesh includes source bytes, physical generators, exact sites,
  separate UV recovery descriptors, source maps, quotients and actual facets.
  Pole bilinear roles are matched by physical keys rather than facet ordering.
  Optional display uses stored XYZ for every vertex. Source normal evaluation
  cannot silently replace the geometric mesh positions.
- Parent/child exact budgets are separate; root-import references are immutable
  and lease-owned; cancellation drains completions before releasing inputs.
  Reservation formulas include retained arenas and pending numerical scratch.
  Atomic directory publication preserves an existing complete result on failure.
- Independent original-coefficient/Fraction/domain/contact readers are research
  oracles off the product path. Direct source/runtime/error/refinement cases,
  generic performance families and actual complete CPU/P100 calls address their
  own claims. NVIDIA/OCCT are differential references with different quality
  semantics, not automatic whole-cell oracles. No second certification layer
  is proposed.

## Withdrawn candidate finding

I initially suggested that the axis-aligned rectangle center of convex plane
chart Q could lie outside Q. That is false in2D: a separating-line argument using
the extremal coordinate points places the rectangle center in Q. It can lie on
its boundary; source one-sided jets are still defined under the admitted chart.
Withdrew this before requesting a plan change. A corner-average interior seed
is an optional choice, not a blocking finding.

## Verdict

**ACCEPT for independently reviewed root/runtime/topology numerical/source/
physical-truth/oracle/publication scope**, following the P1 resolution above.
The mechanisms are constructive within their stated finite admission and error
semantics. Overall acceptance also requires the other reviewers' geometry-unit
verdict; this author does not self-approve that unit.

## Authoritative map wire seam rereview

Root selected storage of the actual B/T maps. The actual runtime DOMMAPS_ and
SUPPORTS rows retain the finite nodal source point/P/Q and graph-strip frame/
interval/taper semantics. Mapped pcurve s in the source-point descriptor is
distinct from GraphStrip original edge t, which passes through the retained
map exactly once. The source projection and referenced pre-B list define the
transformed graph; invertible exact coordinates plus its monotone inverse
define the fiber map without an algebraic topology site. Full face order B then
T and reference ownership are explicit. **ACCEPT for these field meanings.**

Raised a separate integration question to root: reconstructed authority is per
coedge, so mixed faces need a closed named reference at an authoritative-A /
chosen-Q joint before the global domain map applies. A finite pulled-back
reconstructed reference under B, with explicit joint-star displacement, or a
typed finite-admission refusal can resolve it. The wire rows already represent
the required maps; this is a source-policy definition, not a new row/Boolean.
Root selected HybridPullback and runtime now stores the finite reference-policy
tag. At any authoritative joint original A is retained, while reconstructed
reference arcs are B^-1 of their selected polygons; the explicit inverse sector
formula closes the reference at A. Applying B returns every reconstructed arc
to its selected polygon. T maps only authoritative arcs and excludes the
reconstructed selected polygons except common Q. Ordered B/T displacement and
the source-reference inverse-B movement are charged to their named local
allowances; neither is free. Per-use authority and all-reconstructed identity
remain explicit, with decoder policy/authority mismatch refusal. These actual
runtime fields resolve the mixed source-policy seam. The independent numerical
peer also accepted the root-selected construction, requesting the now-added
plane-Q inverse-reference admission check. **Mixed wire/source seam ACCEPT.**

## Request quality and pole orientation rereview

Read the actual legacy node materialization/recovery checks and revised runtime
QUALITY_/COMPONTS paragraph. `source_residual_mm` is the node cap, comparing
original V/exactCe to original S at the stored chosen recovery UV, plus the
original V/Ce endpoint check. It is source-only kappa transport, not output Q
rounding or an unrepresented exact inverse. Whole original Ce/trace and Ce/chord
comparisons use positional epsilon_def, as does domain+actual-cell. Runtime's
distinct component tags preserve those meanings. **ACCEPT**; the geometry unit
now matches and has a hand-derived rational-circle counterfamily.

Also derived the actual pole facet order independently: det(DPsi) is
delta_u*(g-v_pole), and Phi_xi cross Phi_rho is minus rho times the role triangle
cross product. Runtime's keep-order condition `-face.orientation*sJ=+1`, with
stored role permutation, is correct. Neither a blind face sign nor emitted
facet ordering may redefine the source interpolant. **ACCEPT.**
