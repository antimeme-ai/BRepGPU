# Actual-cell source partition and completion review

The parent reviewed actual `cell_bounds.cpp`, its immutable root lookup and full
completion envelope. ACCEPT within this ordinary-cell scope after the metadata
findings below were resolved. This is the bound consumer of the existing mesher
design, not a completed face engine or a second geometry evaluator.

Exact clipping carries the original triangle's barycentrics through every cut.
Virtual XYZ therefore evaluate the same original emitted affine facet; they are
not new source-surface samples. Positive fan pieces retain source/root and parent
identity. Exact summed oriented area checks complete source-domain coverage
before publication. Boundary-only intersections contribute no positive-area task.
All original corners must lie inside the true natural rectangle.

The indexed column walk avoids materializing a thin triangle's entire bounding
box grid: it clips U once per intersected column, then queries only its occupied
V range. Existing exact knot-axis indexes remain the only source index. A
16-by-16 authored grid observes 16 initial column entries and 46 rectangle
candidates rather than 256 bounding-box roots. This concerns candidate work;
binary searches, exact arithmetic and full-call timing are separate costs.

Analytic ordinary-cell bounds use original whole-rectangle Hessian norm uppers,
the triangle coordinate variance bound and actual corner departures. The Taylor
factor yields (Muu*du^2+2*Muv*du*dv+Mvv*dv^2)/8. The maximum actual nodal departure
is charged once. Rational cells always use direct restricted residuals, preserving
their cancellation, rather than this generic C2 estimate across knots.

Integration review found that fixed bound-policy revision and per-request retry
revision had conflicting meanings. They are now distinct fields. Review also
required explicit face generation because a rebuilt face can reuse triangle slot
and slot generation while source/boundary generations stay unchanged. Full
CellBoundResult records retain source, face, boundary, triangle and request
identity through the final fold. Stale face and retry completions are rejected;
no scalar answer discards their envelope.

The fold validates whole completed batches before updating its seen set and
returns success only after every prepared piece completes. The owned runtime
already guarantees unique ordered keys inside each completed batch. Plane-XY
and pole-fan maps explicitly require their specialized consumers.

Eight independent Fraction power-polynomial cases inspect actual clipped L and
original C0 geometry. CPU and both P100s, focused host sanitizers, device memory
checks and source lint pass. This does not establish source trim embedding,
chart recovery, specialized pole coverage or whole-watch performance.
