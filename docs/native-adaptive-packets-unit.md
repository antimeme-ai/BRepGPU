# Shared adaptive packet sides and triangulation

Status: finite implementation unit under accepted ADAPTIVE_MESHER_DESIGN/PLAN;
root, runtime and geometry peer scoped plan reviews ACCEPT. This is the
construction owner's complete side/packet invariant. It does not implement a
second source evaluator, adaptive executor or boundary-region mesher.

## Files and exact seam

Own `native/mesher/adaptive_packets.hpp/.cpp`,
`tests/native_adaptive_packets.cpp`,
`tests/native_adaptive_packets_oracle.py` and this unit's journal.
Root owns CMake. Runtime owns shared C++20 `adaptive_types.hpp` in namespace
`bgmesh::adaptive`; geometry owns CUDA17 POD `adaptive_types.h`.

Use the agreed entry point:

```cpp
PacketMesh build_packet_mesh(
    bgfd::Budget& scratch,
    const std::shared_ptr<RunResource>& result_resource,
    std::shared_ptr<const PatchSelection> selection,
    std::span<const uint32_t> enabled_leaf_slots);
```

`PacketMesh` is `shared_ptr<const PacketMeshData>`. It retains selection and
result_resource, not Budget-affine Rational values. PatchSelection is the
runtime's immutable selected disjoint leaf partition of one coherent
construction chart/source generation. Several original knot-span roots can
participate. Separate construction charts are separate invocations. Cache
recyclable indices never become point or leaf semantic IDs.

Borrow selection's ExactTable, canonical PointRecords and SelectedLeaves.
PointRecord has PointKey, exact `chart_xy` refs, separate original-UV locator,
actual XYZ and source departure. PointSlot is its query-local immutable index.
SelectedLeaf has LeafKey, exact box refs and four PointSlots in tensor order
`00,10,01,11`; CCW boundary order is `0,1,3,2`. Shared original knot ownership
already requires original C0 admission by the source/cache owner. The packet
builder does not infer continuity by equal XYZ. All ref/slot ranges and corner
coordinates are checked before use.

Output packed records:

* `PacketLine`: axis and exact supporting-coordinate ref, range of sorted
  PointSlot events. All incident sides refer to this SAME event stream.
  Axis denotes the varying coordinate; the supporting constant is the
  perpendicular coordinate.
* `PacketSide`: line index, first/count subrange and reversal flag; four sides
  per packet in CCW order. Ranges include both endpoint samples.
* `PacketRecord`: original selected-leaf slot, side range and triangle range.
  Its LeafKey is read through the retained selection.
* `PacketTriangle`: three PointSlots in positive chart order and leaf slot.
* `PacketRim`: positive atomic side interval, endpoint PointSlots and its packet
  owner, oriented with packet interior on the left.
* sorted `demoted_leaf_slots`, so the SAME later region constructor receives
  those leaf areas. Demotion does not discard source material.

The shared header's precise container/Range spelling is reconciled with runtime
before implementation. These are not alternate PointKeys or geometry records.
Results expose source slots/refs, no evaluator callback and no center-point
request. Output capacity/counts are checked before fill/publication.

## Side schedules

1. Check enabled slots are unique/current and sort by retained canonical LeafKey.
   An empty enabled set returns an empty valid packet result.
2. Emit four side records for each enabled rectangle. Record axis, exact line
   constant, ordered interval endpoints, original side direction and leaf slot.
   Sort by axis/constant/interval/key using exact chart-coordinate comparisons.
   Temporary exact imports belong only to scratch and are cached per input ref.
3. For each collinear line group, sweep endpoint events in exact coordinate
   order, grouping equality BEFORE incidence updates. The open atomic interval
   between consecutive distinct events carries at most one side on either
   orientation. Two opposite sides are internal; one is a rim. Same-direction
   overlapping coverage is an invariant error, never tolerance stitching.
4. A coordinate event has one canonical PointSlot. Equal construction chart
   coordinates supplied as incompatible PointKeys/XYZ are an input/selection
   invariant error, not permission to weld. The finer incident leaf's corner is
   already in PointPool; hanging events need no extra source evaluation.
5. Build each side's event subrange including every neighboring packet endpoint.
   Collinear disjoint intervals on one supporting line do not create spurious
   coverage through gaps. Root/span sides use the same sweep as child sides.

Arbitrary hanging depth is supported; mandatory2:1 propagation is absent.
Sorting costs O(NlogN); event/output work is O(N+H), where H includes side-sample
uses. Every loop and comparison charges the native work budget. No all-leaf
neighbor scan or heap topology object per edge is introduced.

## Rim regularization in the same builder

At an endpoint, derive the cyclic open quadrants occupied by enabled packet
rectangles from side/corner incidence. A packet occupying two adjacent
quadrants at a T-junction is one sector, not two disconnected islands.
If occupied and empty sectors alternate, mark ALL incident enabled packets
for demotion, using stable leaf order. Apply the complete sorted marked set,
update affected sector vertices and repeat until no such vertex remains.
Each packet demotes at most once; no source sample or cache leaf geometry moves.
For the first unit it is acceptable to rebuild the sorted side streams after a
demotion round; work is charged and round/removal counts are reported. An
incremental side/rim update is later selected only if measured repeated rebuilds
justify it. Do not claim a linear total bound for repeated full rounds.

After this finite process, rim halfedges have unambiguous degree2 cycle
continuation. Expose ordered rim intervals; later region construction combines
them with settled source loops. This unit does not run that CDT or classify
trims. Untrimmed selected partitions need no demotion except a deliberately
disconnected/point-touch selection. A demotion is unmeshed band material, never
successful empty output. Until region construction is available, phase1's
complete-rectangle PatchQuery refuses any remaining demotion rather than
publishing partial material.

## No-center positive zipper

For each rectangle, use BL–TR as the fixed first diagonal. Obtain its four
ordered side chains from the common event schedule.

In half BL–BR–TR, maintain A on the bottom chain starting at BL and B on the
right chain starting at TR. Never consume their common corner BR early. If
both chains have another non-BR sample, compare the new A–B diagonal candidates
using the common explicit RN64 squared-distance score from actual XYZ. Overflow
saturates to positive infinity. Equal scores use canonical PointKey order
(equivalently PointSlot only when the shared table explicitly guarantees that
order). This is a heuristic; no exact score escalation is necessary.

Consuming next A emits `(A,nextA,B)` and advances A. Consuming next B emits
`(A,nextB,B)` and advances B. Finish with `(A,BR,B)`. Apply the corresponding
oriented construction in half TR–TL–BL, retaining TL last. Each ear is inside
a convex triangle with samples on its two sides. All triangles are positive,
all supplied collinear side samples survive and k side vertices emit k−2
triangles. Two triangles are emitted for an ordinary four-corner rectangle.
No new sample, midpoint, center, connectivity refinement or source orientation
flip occurs here. Final B-rep face orientation is a separate publisher action.

## Direct RED, implementation and closure

Root targets: library `brepgpu_mesher_adaptive_packets`; native fixture/test
`brepgpu_native_adaptive_packets`; CTest Fraction reader invocation:

```sh
python3 -B tests/native_adaptive_packets_oracle.py \
  --fixture <build>/brepgpu_native_adaptive_packets \
  --source <build>/actual-cell.bgsource
```

The native fixture consumes literal typed PatchSelections and calls this real
entry point, retaining a REAL SourceCache compiled from the existing authored
cell fixture's analytic carrier10. Literal packet coordinates/XYZ exercise
connectivity only; the integrated PatchQuery owns source-bound truth. Before
bodies exist it must fail for the missing operation, not
for an unrelated CMake/source registration mismatch. It emits original case
data plus ACTUAL point IDs, triangles, side/rim intervals and demoted leaves.
The separate Python Fraction reader owns the geometry oracle: positive areas,
exact rectangle-chain equality, every side sample incidence, paired internal
intervals, rim coverage and k−2 count. It does not call production orientation
or triangulation predicates or require a specific legal diagonal sequence.

Authored cases: one rectangle; unbalanced T-junctions through several depths;
multiple nonuniform original span boxes and protected collinear samples;
negative coordinates/large translations; exact non-dyadic coordinate refs;
ring-shaped packet selection with a hole; diagonal-only vertex-touch packets
requiring demotion; canonical point/key tie and overflowing heuristic scores.
Use original actual XYZ literals independent of UV orientation. Error cases
exercise stale/duplicate/out-of-range enabled slots, incompatible common-point
identity, native work/byte exhaustion and cancellation, without adding a
second geometry certifier. Shared selection-seal ownership handles invalid
root partitions; this unit does not independently reconstruct a source tree.

After genuine RED, implement the whole side/zipper unit. Run the direct reader,
strict build/lint and bounded native sanitizer process on the fleet. Instrumented
native stdout can be read by the SAME Fraction reader, with no ASan DSO loaded
into Python. Peer review actual code; integrate findings and stop this unit's
campaign once relevant checks pass. Integrate immediately with actual cache
PatchQuery; a literal unit fixture is not source-to-patch delivery.

All product execution uses the isolated cached-adaptive-mesher checkout and
owned build directories. No old-control checkout changes, product library
adoption, standalone commit or unrelated watch run belongs to this unit.
