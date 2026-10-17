# Region CDT bridge lifetime: finite design and implementation plan

The selected change releases completed bridge phases under the SAME Budget before
CDT and adjacency allocations. It changes neither the core topology, source sites,
metric, seed, predicates, constraints, work cap nor the 64 MiB production child cap.
No library or allocator is adopted.

## Actual failure and storage grounding

The selected original-component v6 failure requested 3,670,016 bytes at alignment4
with live64,301,215, peak65,202,847, limit67,108,864; max integer width28 limbs.
The allocation is DivideConquer::edge's vector growth to131,072 Quad records
(28 bytes each); the old65,536-record buffer remains live during replacement.
Its final replacement footprint would fit the recorded limit, whereas the
allocation overlap does not. This is not proof of the complete job fitting later.
The failure log does not record actual site count or failed-facet/refinement cause;
these remain unobserved rather than inferred from the vector capacity.

`native/direct_cdt/core.cpp:103–175` already recycles erased Quad slots through
free_; rotation handles remain slot*4. A simple planar partial graph on N>=3
vertices has at most3N−6 edges, also with disconnected/collinear components;
that bounds live edge records, not a geometric vector's transient allocations.
`Constructor` constructs Mesh before the temporary DivideConquer stage. Mesh also
holds a bit_ceil(max(16,24N))-entry directed hash table, reserved2N faces, and
per-site rows. These layouts and their caps remain unchanged in this unit.

References read: pinned Triangle1.6 `triangle.c` poolalloc4026–4088,
pooldealloc4089–4110, pooldeinit4005–4025 (dead-item reuse and explicit pool lifetime),
and [Shewchuk, section2](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html)
(record-locality/edge-versus-triangle comparison). CGALv6.1
[face base](https://github.com/CGAL/cgal/blob/v6.1/TDS_2/include/CGAL/Triangulation_ds_face_base_2.h#L34)
stores three vertex and three neighbor handles; its
[TDS](https://github.com/CGAL/cgal/blob/v6.1/TDS_2/include/CGAL/Triangulation_data_structure_2.h)
uses Compact_container. Neither reference selects a core rewrite: our immediate
waste is retaining dead bridge representations alongside an already recycling core.

## Ownership proof and selected phases

`Predicates::Predicates` in `native/direct_cdt/predicates.cpp:136–179` decodes and
copies both weights and all coordinates into owned Integer Point rows, creates
owned exact lifts and numeric enclosures, and retains no bgfd_input, node or limb
pointer (`predicates.hpp:25–41`). Integer::decode owns its copied magnitude.
Every allocation remains on the SAME caller Budget; returned result vectors do too.

Implement lexical/helper phases inside `construct_region_cdt`, with optional
nonmovable Predicates as the only handoff between encoding and solving:

1. Admission scope owns boundary flags, borrowed point refs, segments/index,
   compact mapping and duplicate-sort rows. Retain only directed slots/loops,
   positive/negative inventory, and original-site `active` mapping. Remap slots
   through compact before that mapping dies; destroy index before its borrows.
2. Encoding scope owns transformed Rational pairs, denominator, nodes and limb
   wire arrays. Construct optional<Predicates> by emplace while those arrays live.
   Exit scope immediately after its constructor; no shrink_to_fit/clear-only
   pseudo-release and no duplicated Budget owner.
3. Solve with retained predicates/slots/loops. Destroy predicates immediately
   after solve; destroy slots/loops and active after mapping returned triangle
   slots back to original sites. Keep result stats/denominator_bits by value.
4. Build the existing HalfEdge/adjacency/component result only after all preceding
   phase scratch has died. Existing ordering, loop left incidence, inventory and
   exception/stop behavior remain intact.

For caller-live bytes L, admission A(N,B), transformed/encoding E(N,W), owned
predicates P(N,W), topology C(N,B,W), and adjacency H(T), the intended peak becomes
L+max(A, E+P, P+C, H), with the small retained slot/active/result rows included in
these phase terms. Previously A+E persisted into P+C and H. W denotes actual
coordinate/lift limb capacities, not a universal constant; exact arithmetic scratch
and vector reallocation remain charged. This is lifetime reduction, not a new
universal memory-admission bound or a claim that every262144-site graph fits64MiB.

## Direct RED, checks, ownership and order

Own only adaptive_region_cdt.cpp, the existing native_adaptive_regions.cpp and
SAME Python Fraction chain reader if necessary, this plan and journal. No public
header/API, production limit, core algorithm, numerical source or CMake change.

First add a fixed128×128 interior integer grid inside a four-site protected square,
N=16388, exact factor(2,1;0,1), seed0,16MiB Budget and bounded work to the existing
region fixture, with a focused argument that avoids the expensive real-face phase.
Observe old resource-exhaust RED honestly as an owned working-set regression; if
this preselected budget does not separate lifetimes, report that fact before
changing the test. Print existing point/loop/triangle records, so the SAME exact
positive-triangle/atomic-boundary-chain reader verifies the complete result.
Record N, B, triangles, work and Budget peak; after result destruction require
live==caller baseline, including on resource exhaustion. One deliberately tiny cap
must still refuse and release scratch. The existing nested/disconnected/rational
cases retain their current independent oracle.

After genuine RED implement these scopes, run the single affected region oracle
and strict target build, then request root/numerical scoped source review. No new
geometry theorem, campaign, point search or blind ten-minute replay. Root owns
one selected actual-component integration after stable group handoff; its outcome,
not this small oracle, determines whether original1755 now completes.
