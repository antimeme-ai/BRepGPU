# Source-first facet work plan review

Scoped ACCEPT of docs/adaptive-trimmed-lazy-work-plan.md, read against
adaptive_trimmed_geometry.cpp::source_input, compare_original_triangle_precise,
compare_trim_piece_precise and trim_point_floor_requests, plus the shared input
and work records. This is a scheduling/data-transport change using an existing
source theorem and kernel; no new numerical theorem or device ABI.

A positive triangle whose three exact UV corners lie in one original closed
rectangular span is wholly supported by that span. The centroid is only an
indexed root locator; its membership alone never establishes containment.
Canonical source point ownership at a C0 knot must not replace the actual
triangle-side root. Existing original normalization and actual parent XYZ feed
the same S-minus-L source operation. It needs no numerical-leaf partition when
that whole-parent bound passes.

The packet whole_parent tag is sound with the captured known leaf and exact
three-corner containment. Its B corners and e_leaf are still the SAME retained
ones. Failed or cross-original-span Bands retain the old positive SAME-L pieces
before point-floor/refinement routing. Accepted Bands with no pieces require
an explicit direct acceptance; an empty piece range never proves acceptance.
Fresh table namespaces, complete keys/leases, drained asynchronous work and
cumulative work/precise/piece limits cover both phases. No source evaluator,
source map, cap profile or topology algorithm changes are selected.

Actual source reread also ACCEPT: prepare_work_data and the materializer keep
all parent rows, materialize only failed Band payloads and remap piece parents
under a fresh table key. Packet whole_parent reads the original captured actual
UV/XYZ instead of copying cross-table scalar references. Original Band centroid
lookup is followed by all three exact closed-span checks; original source and
precise packs retain SAME actual XYZ and original normalization. No execution
was added by this review; the owner completed the affected real-source checks.
