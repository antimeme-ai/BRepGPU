# Pair oracle implementation review

2026-10-14. Independent source review of
[pair_oracles.py](../python/brepgpu/pair_oracles.py) and the direct pair tests.
No native execution, fleet activity or edits to the reviewed unit.

## Mathematical helper disposition

**No mathematical or source-binding defect found in the helper checkpoint.**
The oracle converts submitted homogeneous controls to exact Fractions and
checks positive weights. Cartesian control extrema enclose the full rational
carrier because its Cartesian point is a convex combination of these
projected controls with positive normalized Bernstein weights. Local-root
containment therefore supplies a sufficient enclosure check independent of
surface sampling or native CPU parity.

World-box qualification checks both exact transformed original-control
extrema and exact transformed actual local-root endpoints. The latter catches
inward frame conversion even if a looser local root happens to contain the
carrier. Native input download is compared bit-for-bit with intended binary64
boxes, preserving signed-zero identity while the mathematical predicate
correctly regards both zeros as the same number.

Expected words use exact Fraction comparisons of actual resident box
endpoints, retain equality and enumerate the requested row-major interval.
Word equality checks every bit and zero padding. Metadata qualification checks
the dimensions/product, bounded interval, next cursor, word extent,
REACHED_END/FULL_DOMAIN distinction, unknown flags and profile-duration scope.
Explicit requested start/budget/profile parameters permit direct request
conformance; empty and terminal intervals avoid division by zero.

Scene qualification retains exact source snapshot objects, canonical unique
body/chart order, typed patch lineage, immutable storage and every chart of
each represented face. Page survivor checks match exact retained source
occurrences and reject extra tail/unexamined survivors. Entirely omitted face
selection cannot be inferred from output alone: direct preparation tests must
compare requested selection with the expected source tuple, as the plan says.

## Stable direct-test checkpoint

The owner delivered twelve stable direct tests after the genuine closed-contact
missing-operation red. The suite checks exact extreme/subnormal/touching words,
asymmetric offsets and a complete page walk, empty/profile cases, descriptor
admission, the2^32 last pair and terminal interval, maximum-size all-one output,
resident input copies and retained parent lifetime. Allocated raw buffers
attack alignment, exact extents, aliases, diagnostic overlap, stale/wrong kinds
and descriptor flags. Constructor/post-native interruption tests inspect real
native handles; page cleanup preserves a chosen primary across a cleanup
KeyboardInterrupt. No native execution was performed by this reviewer.

The geometric fixtures are coherent. Differently framed opposite box planes
coincide at x=.75; separated world frames retain equal local geometry. A
horizontal plane crosses every quarter-cylinder carrier. Tangent x=.25 planes
retain chart0/3, translated tangent cylinders retain the stated paired seam
occurrences, and concentric unequal-radius walls deliberately retain box
false positives. The small cap carrier lies entirely inside the plate hole at
z=.5, so its survivor cannot be advertised as trimmed-face overlap.

**PO1 — whole requested selection needs explicit truth.** The initial stable
suite qualified inferred represented faces but did not compare default
selection with the complete expected canonical chart tuple. Dropping an
entire face only for `face_ids=None` could produce a smaller FULL_DOMAIN that
still passes the helper. Requested a direct multi-body all-chart identity
assertion, plus repeated local face IDs and an explicit four-chart selected
wall. Root accepted the gap; owner is integrating the assertions.

Also requested direct expected-backend checks for raw page/index metadata and
bit-identical extreme-box download checks: accepting either legal backend
does not verify the requested one, and float tuple equality ignores zero signs.
These fit the existing direct tests without an additional oracle framework.

Root reports the native unit separately owns inherited rounding/DAZ/FTZ and
worker-partition checks, with actual CUDA callback current-device/host-worker
restoration coverage being added. Those are different fault classes from
Fraction mask truth. They are not established by CPU/GPU parity or an old
callback-family test.

## Corrected stable-suite disposition

**Accepted; PO1 and the metadata/input-copy refinements are resolved.**
Re-read the updated thirteen-test checkpoint. Default selection now compares
all canonical charts of box, cylinder and plate against explicit source
tuples with exact snapshot identity. A separately selected tuple includes the
same local face ID in two body namespaces and all four charts of one wall,
while preserving canonical input/chart order rather than selection-key order.
Extreme input downloads now use bit-identical `same_boxes`, and index/page
backend fields are compared with the requested engine backend.

The added CUDA-only test requires both devices rather than silently skipping
an unavailable requested CUDA backend. It checks the calling thread's other
current device after context creation, both uploads, resident index downloads,
profiled page creation/download and every close. Host workers separately set
their own prior device, exercise upload/download/page/cleanup and restore it.
The CPU invocation skips only this explicitly CUDA-specific behavior. This
is direct new-callback observation, distinct from numerical mask truth.

No remaining oracle/test source blocker was found. Fleet execution and its
reported outcomes remain root-owned and are not inferred from this review.
