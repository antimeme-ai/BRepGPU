# Independent exact assembly/allowance code review

Scope: actual native/mesher/assembly.hpp/.cpp and the literal-composition fixture.
Read-only numerical, graph, ownership and failure review; owner runs the focused
Release/instrumentation checks. **ACCEPT in this scope.** No concrete defect found.
This does not approve the geometry owner's complete original definition B0.

Source usage matrices compose in the correct parent-times-child order, including
the parent's translation exactly once. Original binary64 coefficients decode as
exact dyadics; intermediate matrices remain Rational, and only final retained
instance entries round to binary64. The stack reference is not used after a
potential reallocating push. The explicit path stack unwinds alongside frame
pop, while the parent/usage index avoids a complete usage scan at each path.
Admitted acyclic source graph, path-depth, instance and Budget limits bound
expansion. Definitions with no placed occurrence remain in the allowance table.

For exact composed A, the maximum absolute row sum of A^T A bounds its largest
eigenvalue. Initializing that row bound to1 enforces kappa>=1 before the exact
dyadic upper square root. Each stored-matrix coefficient discrepancy is exact;
absolute coefficient differences times the maximum absolute coordinate in
B0 expanded by requested epsilon bound every component of (T-T_hat)(q).
The exact squared sum and upper square root therefore bound eta for all such q.

The selected decomposition is
T_hat(q)-T(x)=A(q-x)+(T_hat-T)(q), so using the EXACT A norm together with a
matrix discrepancy bound at q is correct. Since every local allowance is capped
by requested epsilon, q lies in that expanded envelope. Flooring the positive
(epsilon-eta)/kappa to the declared dyadic grid is conservative; inability to
retain a positive allowance is a typed refusal. Componentwise minima over all
placed occurrences preserve each occurrence's world-space bound. Node source
correspondence transforms both original operands by the same source placement,
so its separate delta/kappa rule correctly omits matrix-storage eta.

Every supplied envelope names the same immutable SourceStore, one distinct
definition slot, an ordered box and the current arithmetic owner. Equal envelope
count plus these uniqueness/range checks covers the original definition inventory.
All exact output values freeze before publication; partial instance/path buffers
stay unpublished on a failure. Resource/allocator ownership follows the existing
RunAllocator and sealed ExactTable contracts. Source units are not applied twice.

The fixture independently states the literal nested rotation/three-scale matrix
and compares stored world positions at source-box corners against the remaining
allowance. It also covers reused/unplaced definitions, canonical path ordering,
instance exhaustion and precision refusal. No extra execution campaign requested.
