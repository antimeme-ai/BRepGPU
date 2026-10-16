# Definition-local original-edge forest and complete horizons

Implemented the shared schedule/epoch prerequisite of the accepted whole native
mesher in `native/mesher/boundary.hpp/.cpp`. One owner compiles the selected
definition's original edge/face incidence, retains endpoint vertex XYZ and the
literal source endpoint/parameter association, and seeds original spatial-curve
knots plus incident source-t map guide breakpoints. Descending ranges stay in
their original endpoint direction. Constant/declared-degenerate edges retain
their two endpoint occurrences without fabricating a positive interval. This
does not implement a selected parameter-map profile or rational knot preimages.

Samples retain exact parameters and persistent creation ordinals. A split
creates the exact midpoint and two indexed child intervals, never a private
per-face source-edge node. Original vertex and edge identities include the
definition; immutable source geometry can be shared by different definitions
without sharing their physical samples or mesh allowances.

Protected keys distinguish original-source-edge and face-cut owners. A pole cap
cut cannot alias a same-numbered original EdgeID; every forest lookup rejects
the cut tag. The existing fixture exercises direct lookup and horizon-arrival
refusal. Face-cut requests also require their face-generation envelope and route
to the geometry cut recipe.

The owner requires the full current epoch's face horizon before preparing any
split. An explicit epoch participates in arrivals and typed evaluation keys,
in addition to face generations. This matters after a no-split round: unchanged
face generations do not make a late previous-epoch result current. Requests
are validated for current leaf and source incidence, then sorted/deduplicated.
Evaluation batches retain RunResource-backed immutable parameter tables; child
jobs import those exact refs into their own budgets and use the common original
curve evaluator. Results must match the complete ordered task set. All new
samples/children and local generation changes are prepared before mutation;
mid-commit failure makes the mutable forest unusable. Every incident face in
this definition is invalidated once, with local deltas returned to scheduling.

The source-backed authored test was written first; fleet compilation observed
the genuinely absent `boundary.hpp` RED. Actual strict Clang18 execution then
passed, followed by the strengthened ASan/UBSan/LSan execution. The fixture has
two faces in one definition and another definition sharing their original
edges/vertices. It checks source endpoint mapping despite a false sense flag,
an original map breakpoint, a fast face held for a slower face whose distinct
request arrives last, duplicate request merging, deterministic exact descending
midpoints/creation ordinals, complete result-set rejection before publication,
both incident generations changing once, no-split/stale-epoch handling and
cross-definition isolation. Numerical sample answers are literal straight-line
answers in this coordinator test; the real run producer must invoke the shared
canonical geometry API, which has its own original-source numerical oracle.

Source/epoch code and fixture actual-source lint pass; independent source review
is pending. Only the forest can construct sealed evaluation-batch handles;
callers retain/copy them and cannot replace the immutable task table. Face joints/traces,
domain admission and the whole runtime producer remain the next integration
steps; this forest alone is not a watertight mesh claim.
