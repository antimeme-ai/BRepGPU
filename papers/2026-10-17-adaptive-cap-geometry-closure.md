# Original source cap numerical unit

The first quotient geometry unit implements the accepted
[plan](../docs/adaptive-seam-wedge-geometry-plan.md). It prepares a retained
`quotient_trim` source bank without changing its original envelope, proves
ordinary cone/sphere regularity on each actual node box, and bounds the finite
source cap map against the same three stored physical points on CPU/P100.

`PreparedCapSource` has a private producer constructor. Its auxiliary pole and
ideal-point intervals cannot be replaced through a public aggregate. The source
map admission helper determines finite-side chart roles before sampling cut Q.
The separate cut-progress helper compares both symbolic source gaps, returning
proved, not-contracted or unresolved. Neither helper invents a point or edge.
The existing regular/trace factories retain their refusal profile; ordinary
Patch/Surface and boundary admission of the new capability remain a subsequent
typed-domain integration dependency.

The independent reader extends `native_adaptive_geometry_oracle.py` with
`--cap-fixture`. It constructs ORIGINAL finite `Fsrc(xi,rho)-L(xi,rho)` using
exact bivariate power arithmetic, independent original-argument Taylor bounds
and tensor Bernstein restriction. Production instead uses the reviewed
directional derivative interpolation inequality. The reader subtracts the
original frame origin and actual L constant algebraically before enclosures,
so a large translation does not create false error. The finite subdivision
limit bounds oracle execution; it is not a claim that every valid input will
be decided at this profile.

Six authored sources cover north/south sphere and cone, each with orthogonal
and large nonorthogonal frames. Each retains three original uses, two original
nondegenerate edges and two original vertices, including a variable-U original
reconstructed guide. Four actual-Q/map cases per source cover ordinary and
slanted cuts, displaced cut Q, and opposite finite/ideal side signs. Two cases
also invoke the owned precise comparison. The large-Z sphere case supplies a
direct interior witness against using only a near-pole-strip derivative for
the whole finite-to-ideal connector.

Scoped results: six sources on CPU and both P100s, all108 whole-map bound rows
PASS; complete emitted outputs are byte-identical across backends. Independent
local Jacobian tests check two ordinary boxes per source. Source-only BOTH-side
progress, one-sided nonprogress, finite precision uncertainty, profile/identity
refusal, physical collapse and caller work-cap checks pass. A cone/large-frame
path passes ASan, UBSan and leak detection. The same north/large-frame operation
on P1000 passes memcheck with0 errors and0 leaked allocations. Strict Release
and actual-source clang-tidy pass. No further numerical campaign is selected.

One fixture assumption was corrected: the midpoint of a coarse RN pole interval
need not remain uncertain at a more precise stage. The uncertainty-path input
now uses the middle of the requested source enclosure; this changed the test
input, not the numerical comparison. The first source-backed missing-header
RED and the missing CUDA operation RED were recorded before implementation.

Root's source review accepted factory/local regularity, whole-rho cap math,
private source ownership and the source-only amendment. The construction/runtime
interface review is saved in
[the scoped review](2026-10-17-adaptive-cap-interface-geometry-review.md).
The next work adapter must retain domain/view/construction provenance and reuse
these exact numerical operations. Complete singular-face construction and
publication are not claims of this low-level unit.
