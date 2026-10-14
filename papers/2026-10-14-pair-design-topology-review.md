# Pair discovery design: topology and scene review

2026-10-14. **Accepted; no remaining topology or scene-construction blocker.**
Reviewed [PAIR_DISCOVERY_DESIGN.md](../docs/PAIR_DISCOVERY_DESIGN.md) against
the delivered [body records](../python/brepgpu/body.py) and the retained
[pair topology study](2026-10-14-pair-topology-study.md). This is a source/design
review; no product implementation or native execution was performed.

The selected-face contract now uses `(body_id, face_id)` keys. Bare face IDs
would collide across canonical snapshots because entity IDs are local to a
body. Each source retains the exact immutable snapshot, so its revision,
frame, typed face and chart identity remain available. Independent limits on
body entries, selected keys and admitted charts also bound empty-selection
inputs before control/hierarchy allocation. Both earlier concerns are resolved.

The enclosure path is sound within the canonical frame contract: construct
depth-zero bounds from original homogeneous controls, download and check the
actual local roots, then transform each endpoint using exact binary64-value
Fractions and outward rounding. Positive uniform scale preserves endpoint
order. Comparing local roots directly would be wrong for differently framed
bodies; the design instead compares their common-world boxes. Full rectangular
carrier bounds contain the trimmed subsets, including holes. No sampled trim
boundary is used to shrink a box.

Chart slots remain occurrences in their exact scenes. Four wall charts remain
one cylindrical face; chart joins and periodic seam occurrences are retained.
A clear examined bit excludes one carrier pair. Excluding a face pair requires
all of its selected chart pairs to be covered and clear. Native arbitrary-box
inputs establish only box overlap; the body adapter adds geometry enclosure.
Self queries preserve diagonal and both directed occurrences without silently
changing the Cartesian domain.

Native results retain their source indexes; downloaded scene pages retain
their exact scenes and copied values after child cleanup. Contiguous page
coverage starts at zero and advances by examined count, including zero-survivor
pages. Tail padding and an omitted prefix are not geometric exclusions. These
rules give a usable immutable shortlist without introducing editable topology
or pointer identity at the GPU boundary.

The multi-body caller, full page walk, source/bounds exports and visible
survivor relationships make this a complete narrow consumer. Separate setup,
upload, native predicate, download, decoding, cleanup and wall timings prevent
an isolated comparison kernel from standing in for delivered task value.
The flat all-pairs method is an appropriate first baseline; acceleration and
large-scene scalability remain measurements and later work, respectively.

The plan should preserve the stated direct checks: differently framed
coincidence/separation, selected-face namespace rejection, exact original-control
world enclosure, hole/concentric false positives, all bits and page metadata,
closed contact, seam occurrences, and retained-source lifetime. No additional
research or qualification framework is needed before writing that plan.
