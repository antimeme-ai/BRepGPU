# Physical contacts and shared boundaries: scoped code review

The parent reviewed actual contact classification, its facet BVH and the
definition-local boundary forest. These are components of the accepted mesher,
not a second acceptance layer for a full mesh.

Contact authority is the actual binary64 XYZ interpreted as exact rationals.
Declared shared nodes must have identical XYZ words. Common node IDs permit only
their declared simplex; extra incident overlap still fails. Noncoplanar plane
cuts and coplanar projected clipping classify point, segment and area contacts.
Filtered separation or incident-simplex permission requires strict proved signs;
uncertainty goes to that exact classification. The BVH uses closed boxes and
returns candidates, never geometric permission. Generation-qualified updates,
exact fallback for overflowing SAH comparisons and bounded median fallback keep
index maintenance separate from contact truth.

Review found the projected separation/shared-edge route unnecessarily required
an exact 3D coplanarity test. Disjoint 2D projections imply disjoint 3D facets;
opposite projected third vertices sharing AB imply only AB can intersect. The
owner removed the unnecessary gate and added inclined and one-ulp noncoplanar
cases exercising that route. Exact predicates still handle unresolved signs.
Literal contact dimensions, incident extra overlap, coordinate-key distinction,
transforms and an independent box-candidate model directly check the behavior.

The shared forest retains original edge endpoint association, including
descending parameter ranges; curve_same_sense does not reverse source vertices
again. Physical keys are definition-local and use persistent creation ordinals.
Evaluation batches retain immutable parameter tables. Every face reaches the
same explicit epoch horizon before request merge. Stale epochs cannot credit
an unchanged face generation, and malformed result sets fail before publishing
any split. Incident source-boundary generations change once per transaction.
Face-local cut generations are separate and typed face-cut keys cannot enter
the source-edge forest.

Decision: ACCEPT these contact/index and source-boundary forest units within
their scope. Final affected strict integration and ASan/UBSan topology/contact
checks pass. Whole contact epochs, node/source recovery obligations and the
complete face/refinement coordinator remain integration work.
