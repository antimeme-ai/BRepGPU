# Native face state and shared-boundary transport: actual code review

Scope: `face_frontier.{hpp,cpp}`, the heuristic staging and `next_terminal`
changes in `topology.{hpp,cpp}`, and `freeze_face_boundary` with its retained
snapshot values in `boundary.{hpp,cpp}`. This review read the implementation,
the direct state/identity assertions and the recorded strict/sanitizer results.

The frontier uses the same topology handles, exact cached fixed-metric diameter
and stable lineage ordering. A higher pending cell blocks lower failures. Retry
revision is distinct from the fixed bound policy revision. Local edits fold all
retirements/installations before accessing surviving generations; intermediate
insertions retired in that transaction are never read as live. The bounded
pending stream avoids a triangle-table scan at each child yield.

Heuristic tickets spend the selected ancestry before admission, preserve that
state on cancellation, and forbid intervening topology mutation. Coordinates
remain inside the topology owner until commit. The nonmutating terminal walk
uses the same longest-edge rule as the existing refinement entry point: an
incoming equal-longest edge terminates the pair, otherwise lengths strictly
increase. The face engine can obtain source XYZ before mutating connectivity.

The frozen boundary retains the original SourceStore and sealed exact parameter
table. Chains follow the original parameter-range direction, including descending
ranges, without another curve-sense reversal. Edge IDs order transport rows;
original occurrence traversal remains in the retained source. Each export gets
a distinct exact table ordinal, preventing a same-key/different-pool alias.
Export is restricted to a settled forest.

ACCEPT for these state and transport units after actual source review. Their
strict, address/undefined-behavior and lint checks pass as recorded by the owning
lane. This does not accept unimplemented whole-trace/profile publication, contact
epochs, optimization, full NativeMesh output or a complete-source mesh.

The first ordinary FaceEngine integration was also read. It dispatches actual
UV/XYZ packets, checks all source/boundary/face/triangle/retry metadata, and stages
source-point children before connectivity commits. Its fixture now uses the real
cell preparation/filter/exact-fold consumer. Complete original-use disposition
admission must accompany geometry's forthcoming finite snapshot/map payload;
loop counts and selected polygon segments alone are insufficient for that claim.
