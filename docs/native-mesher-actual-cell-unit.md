# Actual ordinary-cell bound consumer

This implements the accepted ordinary-cell mechanisms, consuming the existing
GeometryStore, exact root index, bound runtime and original analytic jets. It
does not introduce chart construction, topology sites, another source evaluator,
plane inversion or pole recipes.

The shared CellMapKind in mesh_values.hpp has the accepted wire values
uv_triangle=1, plane_xy_triangle=2, analytic_pole_fan_quad=3. The public consumer
accepts only uv_triangle and explicitly refuses the other two until their
specialized source maps are connected. Source-domain/branch admission remains
the domain/map owner's responsibility; bounds never invent a period lift.

```cpp
struct ActualTriangleView {
  BoundTaskHeader header;
  Slot<CarrierTag> carrier;
  CellMapKind map_kind;
  std::span<const Rational,6> original_uv;
  std::span<const Rational,9> actual_xyz;
  const Rational& allowance;
};
struct ActualCellLimits {
  uint32_t max_pieces;
  BoundBatchLimits batches;
};
PreparedCellBounds prepare_actual_cell(bgfd::Budget&,
    const std::shared_ptr<RunResource>&, BoundRootBank,
    ActualTriangleView, ExactTableKey output_values, ActualCellLimits);
struct CellBoundResult { BoundTaskHeader header; BoundDecision decision; };
CellBoundResult bound_analytic_cell(GeometryContext&, const GeometryStore&,
    ActualTriangleView, GeometryBoundPrecision);
```

Parent key is (stable face owner, TriangleHandle.slot, triangle slot generation,
ordinal0). Source/boundary/face generations and the per-request retry revision
remain distinct header fields; bound_revision is the fixed predicate policy1.
UV role
order is the positive chart triangle returned by MutableFaceTopology; the three
actual XYZ triples MUST correspond to those same physical keys. Output facet
winding/face orientation never permutes only one side of this pairing.

## Exact clipping and actual affine map

Reuse intersecting_surface_roots, which performs exact ordered-axis searches on
the existing tensor root grid. Do not scan every source span or construct a
second BVH. Extend BoundRootBank with geometry() and find(BoundRootSelection),
the latter backed by a once-built sorted immutable root-slot lookup. Root bank
selection is prepared once and must contain every needed rational root.

The selected enumerator is an occupied-column walk. Two indexed point queries
validate the full natural-domain bbox without materializing its Cartesian root
product. Query U bands with V collapsed at the triangle's minimum V, deduplicate
column IDs, and exact-clip U once. Discard zero-area columns; derive each positive
column polygon's V range, then query rows at a strict interior U midpoint and
clip only V. This retains canonical U-major/V-fast order and visits O(kU+K)
candidate records, plus exact binary-search/log and arithmetic costs. A thin
diagonal need not enumerate every rectangle in its bounding box.

For each indexed candidate rectangle in stable U-major/V-fast order, clip the
positive original triangle by the four closed exact halfplanes Umin,Umax,Vmin,
Vmax. Sutherland-Hodgman intersections use exact signed distances and rational
t=dA/(dA-dB). Each clipped vertex carries its original triangle barycentrics;
all three barycentrics use the same t as UV. Remove consecutive duplicate
vertices and a duplicate final/first vertex. A triangle/rectangle intersection
has at most7 vertices; refuse an internal combinatorial overflow rather than
growing an unexplained unbounded polygon buffer.

Discard empty/zero-area intersections. Rotate each remaining convex polygon to
its lexicographically least UV vertex, fan in its preserved positive order, and
discard only zero-area fan triples. Positive subcells alone are tasks. Closed
neighboring rectangles share zero-area boundaries, never positive-area pieces;
no derivative estimate crosses a C0 root boundary. Exact original-area versus
sum-of-piece-areas is the clipping construction's coverage invariant. No output
is published if an arithmetic/resource/cancellation failure interrupts it.

At a virtual corner with original barycentrics lambda, form
L(lambda)=sum(lambda_i*actual_xyz_i), using exact Rational arithmetic. NEVER
evaluate S at this corner and substitute that value into the output facet.
Virtual vertices remain bound subcells and acquire no PhysicalNode/SiteSlot.
This preserves actual nodal departure and the same affine output interpolant
across every knot root. Freeze original UV, exact L and allowance into the
prepared cell's one sealed ExactTable, then emit SurfaceBoundInput records with
the corresponding original root's bank slot.

Piece keys retain parent owner/cell/generation and use ordinal0..N-1. The parent
key is stored explicitly, not inferred by stripping an arbitrary piece key.
PreparedCellBounds is an immutable RunResource-backed handle, retaining bank,
sealed values, stable piece records and parent header. batch(first,count) creates
a bounded BoundBatch over a contiguous canonical range with the table lease;
it refuses missing/oversized ranges. Empty positive input coverage is an error.
max_pieces and existing byte/work limits bound preparation before publication.

CellBoundFold retains the prepared handle and one bounded seen/decision vector.
add(BoundCompleted) verifies all piece keys and source/boundary/bound revisions,
including separate request_revision and face_generation,
rejects duplicates, and marks each completion. result() is available only when
every piece completed; it returns the explicitly retained parent key and accept
iff all pieces accept, otherwise bound_fail. This prevents a lost piece or a
partial prefix from becoming cell success. Completion order cannot change the
conjunction. Geometry/runtime errors remain errors, not bound failure or success.
Both fold.result and the analytic wrapper return CellBoundResult with the FULL
parent header. Frontier binding resolves its stable owner ordinal through the
retained run OwnerRegistry, checks source/policy identity, and maps face/boundary
generations, checked triangle slot/generation and request revision separately.
The coordinator also retains the actual Source/GeometryStore job association;
an arbitrary external header is not authority to route across runs.

## Analytic ordinary triangles

Use original source jets at explicit request precision. The rectangle is the
exact UV bbox of the positive triangle. Let Muu/Muv/Mvv be shared exact enclosure
Euclidean norm uppers of the whole-rectangle Hessian vector intervals. Let E be
the maximum shared norm upper of original point position minus ACTUAL corresponding
XYZ at the three original corners, evaluated at the same explicit precision.
Compare exactly against the rational allowance:

    E + (Muu*du² + 2*Muv*du*dv + Mvv*dv²)/8 <= allowance.

This is the existing accepted Taylor/barycentric variance bound. The mixed term
uses Cauchy-Schwarz with coordinate variances, each bounded by range²/4; the
Taylor factor1/2 yields1/8. Nodal departure appears once. Source jets and shared
enclosure_norm_upper define numerical operations, not a new trig evaluator or
libm oracle. A failed sufficient bound returns bound_fail for refinement;
precision/resource/cancellation errors remain typed errors. Rational carriers
always use partitioned direct residual tasks instead of this C2 bound.

## Focused RED and evidence

Before implementation, author a real source fixture with degree1 tensor knot
roots and a genuine C0 crease at u=1/2, and an actual triangle crossing it. An
independent Fraction POWER-polynomial calculation supplies each clipped-piece
residual result, including off-source actual XYZ that would be falsely accepted
by resampling virtual corners. Include an exact-knot touch and zero-area candidate
to verify no coverage loss/double counting, and require piece-fold refusal for a
missing or duplicate completion. CPU and both P100s must produce the same final
cell answer through the already qualified runtime. A simple analytic plane with
off-source actual corners and a curved analytic carrier exercise the explicit
jet/nodal bound. Add finite unsupported-map/ownership/resource failures.

Observe missing-API RED, implement, then focused Release/ASan+UBSan/leaks,
source lint/format and the actual device ownership path. Reuse existing low-level
arithmetic/device qualification; no repeated broad certification campaign and no
whole-watch performance claim from this unit.
