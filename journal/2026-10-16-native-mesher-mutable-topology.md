# Native mutable face topology prerequisite

Implemented the first topology-owner dependency of the accepted whole mesher,
not a separately delivered mesher. The written whole/unit plans govern the
engine. New `native/mesher/topology.hpp/.cpp` owns append-only exact dyadic sites,
packed triangle/edge rows and generation handles, flat owned site/edge indexes,
reciprocal incidence, persistent triangle lineage, fixed exact factor lengths,
terminal-priority LEPP and local three/four-child transactions. Geometric vector
growth avoids reserve(size+k) copies. No initial-CDT wrapper, global Lawson pass,
source-specific dispatch or new dependency.

Authored `tests/native_mesher_topology.cpp` first and observed genuine RED on
the isolated compute checkout: the requested native topology header/operation
was absent. The first actual strict Linux Clang18 build then passed seven direct
semantic cases: a midpoint between adjacent binary64 endpoints, both-incident
patch updates, reciprocal neighbors, incoming-longest tie termination, rotated
anisotropic edge choice, sticky ancestry, real neighbor propagation and a grid
whose two-bisection descendants satisfy the diameter contraction inequality.

Expanded to nine focused cases for source-constraint preservation, recycled
handle refusal, duplicate-site refusal and bounded work-exhaustion injection.
Failures before a site/edit is published preserve parent connectivity; failures
after publication poison the generation so no partial mutable result is readable
or publishable. ASan/UBSan/LSan and strict warning execution passed the first
nine-case revision; independent review then identified the rejected-proposal
orphan API issue below.

Root's valid finding: accepting an already-appended proposal SiteSlot could leave
an unused point that later blocked an exact midpoint. Replaced that API with an
owned coordinate-taking candidate operation. It consumes the attempt, checks
target/neighbor/protection and exact duplicate coordinates before publishing,
and returns no site on routine rejection. Public append is initial-only and
sealing requires every initial site incident. A direct regression rejects (1,0)
at a spent neighbor, verifies unchanged site count, then bisects that very edge
and successfully creates the midpoint once. Actual revised strict fleet run:
nine semantic cases PASS. No connected site can be reused as a proposal.

Actual-source clang-tidy found a symmetric endpoint signature and optional
alias-access warning; replaced the former with a two-site value and retained
the checked optional as a local value. Actual-source engine/fixture lint and
revised nine-case sanitizer execution passed.

Root then identified a missing integration contract: returning only the target
state forces a caller to scan all triangles to discover terminal splits away
from the requested cell. Added an owner-local ordered edit log, reserved before
mutation. Each transaction records removed handles with their old generations,
inserted handles and its new site; `take_changes()` moves that log to the same
owner. The tenth direct case replays the deltas after a terminal split away from
the target followed by an original-boundary request, checks the exact live
handle set and verifies the consumed log is empty. Rejected connected-site
proposals produce no edit or extra node. Latest strict fleet execution: ten
semantic cases PASS; final root CMake instrumentation/review follows this edit.

Source boundaries, geometry tasks, contact epochs, optimizer and full watch/result
integration remain this owner's next dependencies, not completed capabilities
inferred from the primitive tests.

Circumcenter integration review found one additional sticky-attempt edge case:
the double overload imported a nonfinite candidate before spending its lineage.
The existing sticky test gained a literal infinite-coordinate rejection and
observed actual RED (`nonfinite rational binary64 input`). The overload now
rejects/spends without publishing a site; a second finite attempt is refused.
Fresh consistent-header strict execution passes all ten methods. An earlier
compile straddled the runtime owner's Budget layout extension and returned a
spurious cancellation; the clean closure rebuild resolves that transient ABI
mix. The cached metric-coordinate getter now feeds the owned domain bridge
without copying or independently reconstructing the fixed metric.
