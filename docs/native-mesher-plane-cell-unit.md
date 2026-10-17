# Physical-plane actual-cell consumer

This is the finite PlaneXY specialization of the accepted actual-cell bound,
inside the existing persistent numerical service. It adds no source evaluator,
projection proof or alternate geometry policy. Original-UV recovery coordinates
remain display/correspondence values; the bound uses distinct exact chart XY.

The existing admitted immutable FaceBoundarySnapshot retains the original
source face/carrier, generations and FrozenPlaneChart. One FaceCellWork retains
that snapshot, and each cell adds chart_xy[6] refs into work.values. All cells in
that packet belong to that one plane face/generation. The service checks the
same GeometryStore, run, source/face/boundary generations and source carrier.

Proposed low-level API in plane_cell_bounds.hpp:

```cpp
struct PlaneCellProfileView {
  std::span<const Rational,3> origin;
  std::span<const Rational,6> basis; // X/Y interleaved per XYZ axis
  std::span<const Rational,8> quadrilateral; // CCW00,10,11,01
  const Rational& height_upper;
};
struct PlaneTriangleView {
  BoundTaskHeader header;
  CellMapKind map_kind;
  std::span<const Rational,6> chart_xy;
  std::span<const Rational,9> actual_xyz; // same positive XY role order
  const Rational& allowance;
};
PlaneCellProfile profile(budget, profile_view); // validates/precomputes once
CellBoundResult bound_plane_cell(const PlaneCellProfile&, PlaneTriangleView);
```

Validate one owner, fixed bound-policy revision1, parent ordinal0, plane map
kind, nonnegative height/allowance, strictly positive convex Q and positive
triangle orientation. Closed exact halfplane tests establish that all three XY
corners lie in Q, so convexity contains the whole triangle. Preparation already
proved the original source projection homeomorphism and global height bound;
the numerical consumer does not repeat that proof.

Let P(q)=O+X*q.x+Y*q.y and h be the retained global height upper bound. At every
q in Q, the source map satisfies ||S(Psi(q))-P(q)||<=h. Both P and the actual
triangle map L are affine on this XY triangle. Thus P-L is a convex combination
of its corner residuals and

    ||S(Psi(q))-L(q)|| <= h + max_i ||P(q_i)-actualXYZ_i||.

If allowance<h, return bound_fail. Otherwise compare each exact squared corner
residual with (allowance-h)^2. Accept equality. This avoids sqrt and transcendentals
and preserves the actual stored physical coordinates. A failed sufficient bound
requests refinement; it is not a claim that the actual geometric error exceeds
the request. No XY value is interpreted as original UV.

An all-plane packet runs in one bounded CPU scheduler step: import the profile
once, validate Q once, evaluate its bounded cell array in the same JobContext
Budget and publish all full CellBoundResults atomically. No Rational survives
the step. The existing cell-capacity and scratch/work limits bound this work.
Both CPU and P100 channels take this same exact host branch; no fabricated
device tasks or acceleration claim is attached to a zero-height plane.

Before code, author a missing-API RED using the actual warped-plane original
source: S(u,v)=(3u/2-u^2/2,v,0). Its admitted physical XY chart meshes the full
quad with two physical plane triangles. Their exact physical interpolation
error is zero even though the old same-UV interpolation residual is nonzero.
Use that independent polynomial identity, plus literal nonorthogonal affine
frames, h+corner-error equality/just-fail, closed-Q/outside/degenerate roles,
metadata and typed resource/cancellation checks. Then strict Release, scoped
ASan/UBSan/leak and format/tidy suffice; full caller integration belongs to the
coordinator and face-engine owners.
