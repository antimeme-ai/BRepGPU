# Interchange converter source review

Root reviewed the complete converter against the accepted source/runtime/oracle
design and actual parser types. Accepted, no blocker. This review is separate
from the implementing owner and does not stand for unexecuted reference/caller
results.

Complete typed face and packed NURBS extents are checked before conversion.
Offsets advance across unsupported multispans, source faceuses survive unchanged,
and document-wide native face/chart IDs use the canonical shared body namespace.
No geometric equality merges source identities; the original file bytes/hash,
prim paths, face ordinals, complete source arrays and uninterpreted trim fields
remain owned. Later file changes cannot affect the immutable snapshot.

The chosen weight factor is the smallest power of two that lifts the minimum
positive weight to at least one; if it makes any weight exceed256, no larger
power can work. Exact Fraction scaling handles original subnormal or very large
weights without losing their ratios. Spatial power-of-two normalization and
one-round homogeneous products preserve the admitted native envelope. The exact
maximum Cartesian control displacement and 4pRδ/L partial bounds use original
source coordinates and exact endpoint spans. Unsupported domains/degrees/spans
or envelopes produce explicit records, never fitted replacements. Malformed
packing aborts the whole result. The existing native ABI is unchanged.

Direct fleet converter tests pass. Native/source first-jet qualification and
caller/reference result review are separate mechanisms attacking different
fault classes and remain reported with their own execution scope.
