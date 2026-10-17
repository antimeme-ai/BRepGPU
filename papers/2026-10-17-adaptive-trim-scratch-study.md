# Trim preparation scratch: coordinate facts and one diagnostic

Read-only study for the reported original definition 1755 v5 failure. Parent
reported 45 analytic faces (17 planes, 15 cylinders, 13 cones), an exact 64 MiB
scratch failure after the last 817-second observation, and roughly 3 GiB process
RSS under a separate 32 GiB run cap. These observations do not identify the
failed allocator request or the size of the exact operands. No product edit,
build, geometry execution or new qualification campaign belongs to this study.

## Retained coordinates already avoid rational refinement history

Read adaptive_regions.cpp::dyadic_round, centroid, grid_target,
plan_construction_refinement, RegionBuilder::{source_sites,grid_site,anchor_sites,
triangulate}; adaptive_region_cdt.cpp::construct_region_cdt; and the accepted
construction design's central-anchor/packet-target sections.

New BAND sites already round the ACTUAL parent centroid to a global dyadic grid.
The exact tests require all three barycentric numerators to be at least one
sixth of positive parent double area. RefinementInput defaults anchor_bits to
1024; failure to find a point under that cap is unresolved. The rational centroid
(/3) is temporary, not the new retained site. Packet targets already use
prepared-root-relative dyadics with depth at most 64, and the cache constructs
their common-schedule corners. Region CDT triangulates the supplied coordinates:
it does not emit circumcenters, recovered-edge intersection vertices or new
Steiner coordinates. Packet rims reuse existing schedule vertices.

Important correction during this review: FaceLocator::plane returns
chosen(dot(...)) for BOTH coordinates. chosen converts to binary64 and back to
an exact dyadic. The unrounded exact Gram foot is not retained. Therefore the
current plane candidate's to_double bank ordering compares the same represented
numbers, and an initially proposed sub-ulp exact-foot refusal finding was
withdrawn. Reconstructed analytic locators and guide choices likewise store
chosen dyadic proposals. Exact source knots or mapped-cap panel interpolation
can introduce other rational coordinates in their admitted profiles, but their
presence and sizes in this failure were not measured.

SurfaceCacheSession::absolute_point may use a general rational normalized
coordinate while searching for a compact root-relative point. Its absolute
fallback retains the supplied original UV; it does not replace that UV by the
normalization quotient. The dyadic chosen metric in face_metric and
the integer CDT bridge also do not change semantic UV. When all supplied sites
and that factor are dyadic, construct_region_cdt uses a common power-of-two
denominator, rather than its general LCM branch.

A bounded dyadic interior policy is compatible with the current construction
contract and is already implemented. It does not imply a universal 53-bit or
64-bit anchor exists in an arbitrarily thin valid triangle. For positive double
area A and maximum edge L1 length M, rounding the centroid on spacing h changes
any edge orientation by at most Mh/2. Thus h <= A/(3M) suffices for the existing
A/6 central margin. This is an owned elementary bound, not a new insertion or
termination theorem. Lowering the current finite cap could cause additional
unresolved outcomes; no such policy change is selected here.

## Temporary exact arithmetic and index storage are different candidates

Read adaptive_trimmed_geometry.cpp::LeafIndex, clip_polygon, prepare_work_data;
adaptive_surface_cache.cpp::step_facets; native/face_domain/exact.hpp::Budget;
and exact.cpp::Rational arithmetic. The facet work child JobContext uses
CacheLimits::child_bytes, whose default is 64 MiB, independently of the global
RunResource cap. Budget failure tests requested bytes against its own remaining
live capacity; low process RSS cannot rule out this local failure.

LeafIndex reserves N LeafBox records and 2N BoxNode records on that Budget.
Under the current ordinary 64-bit field layout those arrays are approximately
136N bytes (40N plus 48*2N), or 34 MiB at N=262144, before other scratch. Actual
N and the failing reserve were not reported. The index is built lazily once in
a materialization work call; persistent output rows use the run resource.

Exact virtual clipping can have odd denominators even when all retained mesh
UVs are dyadic. Its t=(c-a_axis)/(b_axis-a_axis) is a ratio of dyadics; barycentric
and later SAME-L XYZ combinations carry those rational values. Each leaf clip
starts again from the actual parent triangle and applies four rectangle planes.
Virtual intersections are not fed into the mesh site pool or the next CDT.
Geometrically, its vertices are original triangle vertices, original edge/box
edge intersections or box corners. Consequently repeated mesh horizons do not
by themselves nest these virtual intersection coordinates into new source
sites. Intermediate exact operands, point count, retained piece count and the
actual source exponents can still make scratch expensive. No huge-operand
diagnosis follows without those measurements. The separately owned lazy-XYZ
change should be evaluated on its actual next operation before selecting more
work.

Reference grounding: NVIDIA USD/BRep revision
c9c5979f04cea8c38213004d4f04df0dcfb40d05, SmTess.cpp::ProcessTree (4860–4915),
SmTessSrfCache::BuildTree (13215–13290), ImplantUVSegment (14293–14365), and
TriangulateSingleFace (8540–8655), reread in this unit. They separate shared
cache corners, indexed trim embedding and residual polygon triangulation;
their floating tolerance/clamping policies provide no exact coordinate-bit or
scratch bound for us. Previously read Balazs/Guthe/Klein 2004 §§5.2, 6–7 and
Ma/Hewitt 2002 §§4.1–4.4 motivate cached partition/rim locality; no rational bit
complexity theorem is inherited. Source revision/hashes and article acquisition
records remain in papers/whole-mesher/source-readings.tsv and the existing
bibliographies.

## One direct diagnostic if the next operation fails

Do not trap every allocation. Read-only GDB disassembly of the retained v5
binary established Budget::do_allocate's failure-only branch at function+64,
before it overwrites the original allocation arguments to construct Failure.
At that instruction rdi is Budget*, rsi requested bytes and rdx alignment.
The actual v5 offsets are limit/live/peak/work_limit/work/max_limbs at
8/16/24/32/40/48 bytes after the vtable. The disassembly's upstream pointer at
88 bytes agrees with that layout. A catch at __cxa_throw is too late to assume
those caller-saved argument registers still hold Budget/size.

The ignored private GDB command file captures the failure-only allocation size,
Budget counters, arithmetic counters and twenty stack frames, then CONTINUES.
Parent must verify the actual v6 disassembly before its one selected-component
operation; no inferior was executed in preparing the file. Caller frames plus
requested size distinguish LeafIndex reserve/candidate scratch from Integer
limb growth or virtual XYZ arithmetic. max_limbs is useful but can underreport
a failed growth request because observation occurs after successful allocation;
the requested byte count and stack are therefore essential. There is no extra
exception catch, successful-allocation breakpoint, abort or cap change. GDB
timing is diagnostic, not an uncontended benchmark. No second run is proposed
unless the actual result leaves a concrete unanswered question.
