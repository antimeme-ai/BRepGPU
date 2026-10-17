# Cap continuation and final contact integration review

Scoped actual-source ACCEPT of native/mesher/adaptive_component.cpp after reading
the query coordinator and final contact translation. No new execution was
requested by this review.

The immutable continuation vector is local to one original ComponentQuery and
is indexed by its complete original face inventory. Both complete and
needs_boundary paths capture history before pooled retirement. A completed face
later invalidated by another incident face's boundary refinement keeps its
control/metric and starts a new assignment against the current physical
snapshot. Fresh qualities create a new continuation vector. The exact query
allowances pass through unchanged. A retained unaffected face keeps its own
paired boundary snapshot; global boundary epoch is not confused with per-face
boundary generation.

Final contact frontier identity uses the independent face_generation while each
cell retains its construction revision. Protected physical edges keep their
original source BoundaryKeys. Generated face_cut keys require the original
face owner and are qualified by the same face generation. Mutable owners are
retired only after each task's completion is folded, and final contacts run on
the complete current set of accepted face results. No concrete blocker remains
in the reviewed continuation/full-generation/face-cut seams.
