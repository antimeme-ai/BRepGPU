# Region CDT storage: captured allocation and bridge lifetime review

Read-only source review after the actual selected-component v6 failure. Parent
captured 621.74 s, requested allocation 3670016 bytes/alignment 4, Budget limit
67108864, live 64301215, peak 65202847, work 221274596 and maximum integer
28 limbs. Stack identified DivideConquer::edge/build through
construct_region_cdt and RegionBuilder::triangulate. No new geometry execution,
build, implementation or memory-limit change belongs to this review.

## What the failed request means

In native/direct_cdt/core.cpp, Quad has four uint32 next handles, two uint32
endpoints and a bool: 28 bytes in this ABI. The request is exactly 131072 such
records. The current vector's single-element growth is consistent with moving
from capacity 65536 to 131072 while the old 1835008-byte buffer remains live.
Remaining Budget capacity was 2807649 bytes, so the new buffer exceeded it by
862367. After the hypothetical old-buffer release, live would be 66136223,
972641 below the unchanged cap. The capture therefore identifies transient
vector growth overlap under substantial other live scratch, not a gigantic
integer request. It does not show that the whole operation would subsequently
fit, nor identify the active site count.

Deleted quad slots are already reused. edge() appends only when free_ is empty;
erase() unsplices both directions, marks the slot dead and pushes it into free_.
At an append, slot high-water equals the live-slot count. There is no source
evidence here of allocation proportional to the total history of deleted edges.
One undirected edge is one Quad; its four rotational handles are not four Quads.

A simple planar partial graph on global n>=3 sites has E<=3n-6 even when the
divide/conquer recursion currently contains disconnected or collinear pieces.
The completed triangulation identity E=3n-3-h cannot instead be imposed on every
partial merge. Applying the planar bound requires the algorithm's no-loop,
no-parallel-live-edge, noncrossing embedding invariant, including newly connected
merge edges. Requiring edge 65537 implies n>=21848 under that invariant, not an
exact point count. A bounded pool must check overflow and rotational handle
limits separately; exceeding a proven topology bound is not permission to
silently truncate output.

## Selected smaller conceptual change: end dead bridge phases

Read native/mesher/adaptive_region_cdt.cpp in full, plus
Predicates::{Predicates,orient,incircle}, native/face_domain/exact.hpp::Integer::
decode and DomainSegmentIndex construction/destruction. The root's proposed
bridge lifetime unit is sound under the following ownership boundaries:

* Admission uses point_refs, segments, DomainSegmentIndex, boundary bitmap,
  compact map and unique_order. The index borrows point references and segment
  storage, so it must be destroyed before those arrays. Keep compact until
  original boundary slots have been remapped to active site indices. Preserve
  active's original-slot word, directed slots/loops and signed cycle counts.
* Encoding uses transformed Rational2 rows, common denominator, nodes and limbs.
  Integer::decode COPIES the limb range into owned PMR storage. Predicates then
  owns wx/wy and translated Point x/y/lift plus interval metadata. Its only
  external owner pointer is the SAME Budget. It retains no bgfd_input, node,
  limb, original Rational or temporary array references. The encoding scratch
  may be destroyed after its constructor finishes.
* Predicates is noncopyable and has no implicit move. An outer
  optional<Predicates> emplaced within the encoding scope is sufficient; do not
  require a move-return or let a reference escape an inner optional. Solve with
  its live value and preserve the same Budget/work counter.
* After solve_directed_regions returns, no bridge postprocessing needs
  Predicates, encoded source arrays, directed slots or loops. Destroy these
  before halfedge/adjacency arrays. Keep active only through the returned
  triangle's active-to-original remap; then release it too. Original caller
  points/cycles remain caller-owned and cannot be released or reparented by
  this helper.

Scope destruction must really release capacity; clear() alone retains vector
buffers. DomainSegmentIndex's destructor releases its implementation storage.
Use lexical scopes/helpers or destruction of owning optionals. Exception paths
must retain normal RAII. No new Budget, reset_work, cap increase, global-resource
reparenting or destruction of required source validation is selected.

The core already overlaps a live Mesh with initial DivideConquer, then converts
initial triangles into that Mesh before the initial pool dies. That is separate
algorithm storage, not dead bridge input, and is outside this narrow lifetime
change. Compact/intrusive or blocked quad pools could avoid growth overlap;
an up-front 3n-6 arena could avoid reallocation but reserve far more than a
collinear/partial input needs. Neither choice removes dead bridge arrays. Moving
quad slots also requires preserving slot*4 rotation identities and every live
handle. These remain alternatives, not selected additional work.

## One direct fault-class oracle

Extend the existing real region fixture with one deterministic dense supplied
site set, a protected directed outer boundary and genuine interior anchors.
Invoke construct_region_cdt under one fixed SAME Budget: the old lifetime body
must reproduce resource exhaustion, and the new body must return the complete
mesh at that unchanged cap. No artificial allocation wrapper, fabricated
occupancy, watch-specific site IDs or raised limit. Feed returned coordinates,
cycles and triangles into the existing exact positive atomic-chain/required-site
oracle; keep the existing hole/disconnected/component cases. Record peak and
post-return live bytes relative to real caller input plus returned payload.
That attacks dead-phase overlap and dangling-copy ownership through the actual
operation. Ordinary memory/UB sanitization of that fixture covers freed-input
borrows; no second geometric certifier is needed.

Reference grounding remains the selected source design and the already acquired
papers/direct-cdt.bib: Shewchuk 1996's triangle-versus-quad-edge storage discussion
and the divide/conquer/deletion mechanisms read for the original constructor.
The planar edge inequality above is an elementary Euler bound, not a runtime or
coordinate-bit guarantee imported from a reference package. No dependency or
new topology algorithm is selected by the captured memory failure.

Actual implementation reread: scoped ACCEPT of the phased
adaptive_region_cdt.cpp and fixed-grid/tiny-cap additions to the existing region
fixture/reader. Compact-to-slot remapping occurs before compact dies; the index
is destroyed before its borrowed tables; predicate emplace completes while
encoding arrays live. Predicates is reset after solve, and active survives until
original-site remapping. All of those buffers are gone before HalfEdge adjacency.
Signed cycle counts and result diagnostics survive by value. The fixture checks
return/exception cleanup and feeds the same exact chain reader. Its Budget peak
describes Budget allocations, not total process RSS or the fixture's ordinary
caller std::vector record storage. No execution belongs to this source review.
