# Whole native mesher plan: independent algorithm/runtime review

2026-10-16. Read the actual `docs/NATIVE_MESHER_PLAN.md` and
`docs/native-mesher-runtime-plan.md`, including the completed schema/variants.
Scope: cross-owner source/exact/task values, reservations, deterministic
CPU/P100 execution, real consumers and direct oracles. This does **not**
self-approve the separately owned topology implementation plan. No product
code, builds, tests or geometry execution.

## P1. State the tolerance scope through approximately rigid instances

The accepted design calls the requested positional tolerance world-space.
Runtime source admission accepts usage matrices under the existing approximate
rigidity tolerance, then preserves/composes their actual binary64 matrices in
`INSTS___` and the real native consumer. Such a matrix is not necessarily an
exact isometry. A definition-local error epsilon can become larger than epsilon
after an admitted transform; many-level path composition makes the missing
factor explicit. Example: an admitted diagonal factor `1+eta` on one axis maps
a local error epsilon in that direction to `(1+eta)*epsilon`.

Choose one meaning before implementation. If the claim includes actual
transformed instances, compile an outward operator-norm upper bound for each
actual composed instance linear map and use a definition allowance at most
`epsilon/max_instance norm(A)`. Include transform rounding in the separately
declared display consumer scope; do not double-count original source matrix
uncertainty as meshing error. Alternatively explicitly promise definition-local
millimetre error and state how instance transformations affect it. This does not
require rejecting near-rigid source matrices or introducing a new input gate.

## P2. Cell interpolants must not assume physical facet row order

Runtime schema's `CELLFAC_` `BarycentricTriangle` maps canonical cell barycentric
coordinates to a referenced facet, but its payload currently leaves ordering
implicit. A positive chart cell can require a reversed physical facet order for
a negatively oriented source face. Pairing the three cell roles with the three
`FACETS__` nodes by row position then defines the wrong actual affine L, so the
bound/output reader can disagree about the geometry being bounded.

Specify the role mapping normatively: ordinary cell sites define L through
their PhysicalNode keys, and the associated facet must contain exactly that
node set; derive the permutation by key, or store an explicit permutation.
Likewise pole/interpolant roles P_pole/P_a/P_b must match the facet by explicit
physical identity independently of outward ordering. Check every corner's
site/node association during construction/readback. No new geometric oracle
or certificate is needed.

## Resolved during this review

The owner revised the actual runtime plan to close these concrete seams:

- `ExactRef` names sealed table identity plus index, with explicit table registry
  lifetime and canonical final reindex. Parallel builders cannot produce
  ambiguous unqualified references or mutate a shared global builder.
- BoundBatch and ExactFallback children own separate JobContext/Budget/leases;
  they import immutable values and reserve configured exact scratch before
  admission. No fallback waits for unreserved bytes held by its parent/staging.
- Per-face pending frontiers, complete frozen contact epochs and stable global
  optimizer commits prevent completion order from selecting a mesh mutation.
- NativeMesh quality contains canonical host exact allowances/profile values;
  backend-dependent filter endpoints/maxima stay in TSV observations. No exact
  host recomputation is added solely to certify successful GPU decisions.

The remaining resource implementation must materialize its stated size formulas
and configured child limits; failure must return resource exhaustion rather
than waiting while parent storage occupies the required bytes. The current
selected reservation/continuation policy is sufficient to plan this without
another resource framework.

The independent original-value readers, authored exact arithmetic/contact
answers, real stored-XYZ consumer fixture and predetermined anisotropy/planar/
many-hole families bear directly on the claims. This review requests no added
oracle tier, generic mutation ceremony or benchmark SLA.

Reread the final runtime revision: P2 is resolved by explicit physical-key role
matching/permutation for ordinary and pole interpolants. Child admission now
includes the concrete checked parent/batch/answer/exact-scratch formula and
hard fallback limits. Runtime source/exact/task/resource/determinism scope is
ACCEPT; integrated tolerance meaning P1 still requires the root clarification.

Reread root's actual `Carry positional tolerance through actual assembly
transforms` section and matching design amendment. P1 is resolved: exact source
path matrix T, once-rounded stored T_hat, outward Gram/Gershgorin norm kappa and
storage bound eta on the expanded definition box give
`A*(S-L)+(T-T_hat)*L`, hence `kappa*tau_local+eta`. Per-definition safe local
allowances cover every instance; source-only discrepancy is scaled without
charging output matrix rounding twice. Nonpositive remaining allowance is an
honest unresolved/precision result. **ACCEPT for the independently reviewed
root/runtime source/exact/task/resource/determinism/consumer scope.** Geometry
construction and topology unit acceptance belong to their other reviewers.

## Independent geometry-plan seam review

Read actual `docs/native-mesher-geometry-plan.md`, especially Sections5--8,
and the reconciled runtime map/support records. Scope is the construction at
the geometry/topology seam, not self-approval of topology code or its plan.

The finite HybridPullback construction closes mixed per-use authority:
authoritative arcs retain their original meaning; reconstructed references
are explicitly B^-1 of their selected polygons at shared original A/chosen Q
joints. Positive nodal fan orientations give an ambient homeomorphism with
an explicit affine inverse. Ordered B then pairwise-disjoint tapered T maps
send the complete reference boundary to the selected polygon. T excludes
reconstructed polygons except fixed common endpoints. Both directions of the
domain correspondence and the charged reconstructed inverse movement are
defined; mixed stars do not receive an unexplained E_domain=0 rule.

One concrete finding was fixed during review: requiring every original-UV
auxiliary support P to stay inside the natural carrier rectangle rejects
ordinary boundary nodes. On S(u,v)=(u,v,v^2), pcurve (0,t/(2-t)), the source node
at t=1/2 is A=(0,1/3). A nonidentity nodal fan needs P to contain A strictly,
so P necessarily extends outside u>=0. The actual geometry revision now permits
ambient P/strip supports outside R while retaining original/reference/final
domains and recovery parameters inside R. It bounds ordered component UV
movement and evaluates source derivatives only on the convex endpoint hull
inside R. Continuous C0 span bounds give the required piecewise Lipschitz rule;
no intermediate map point requires source extrapolation. The literal boundary
case is retained as a direct authored oracle family.

Pole construction has a finite compatible interface: master-edge side samples,
one dyadic theta-interpolated cut shared with ordinary connectivity, complete
xi/rho quotient coverage and actual nonzero facet roles. Angular refinement
shares the changed cut; radial refinement requests closer owned meridian
samples and establishes parameter-gap contraction before an inner cap plus
ordinary annulus. It does not pretend rho=1/2 yields dyadic original UV or
that a finite source latitude equals the ideal pole. The bilinear rectangle
bound accounts for mixed source terms through F_xixi and actual corner departures.
Fixed source floors and unsupported general polar charts remain explicit.

**ACCEPT for the independently reviewed geometry-plan construction/seam scope.**
The implementation must implement these actual finite maps and limits, not
substitute sampled closure, local inverse convergence or a legacy mesher.
Authored mixed-authority, nondyadic boundary-node, north/south/cone orientation,
cut adjacency and child-coverage cases already target those claims directly.
No additional literature, product run or certification layer is requested.
