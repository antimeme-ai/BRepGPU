# Classification design: arithmetic/runtime adversarial review

Reviewed docs/CLASSIFICATION_DESIGN.md after the bounded arithmetic/reference
study. Accept the exact CPU material-membership and rectangular-face-contact
design. No blocking runtime or arithmetic finding remains. The public contract
is a small coherent operation; no native/filter/GPU implementation or language
architecture investigation is needed to deliver it.

Exact conversion precedes world/local frame subtraction/division. This correctly
handles the subnormal-origin witness: a cylinder translated by 2^-1074 in x
contains world x=1/4 strictly, even though ordinary subtraction rounds the local
coordinate to the radius. Fraction inputs also represent exact shifted boundary
points unavailable as binary64. Rational radial signs avoid squared-underflow
misclassification. int excludes bool; nonfinite floats and unsupported coordinate
types raise rather than returning exterior. No arithmetic tolerance is permitted.

Feasibility precedes equality-face collection. Thus equality on an infinite
carrier outside finite material does not emit a boundary face. Full original
face IDs, including cap/wall at rims and multiple slabs at corners, are retained;
periodic chart seams create no extra material boundary. Hole-wall material sense
is reversed without inventing a new solid or relabeling its coedge identity.

The rectangular contact construction uses actual affine control geometry and
frame, not sampled extents or a hole-cap bounding rectangle. Exact interval
intersection gives the whole convex locus and its dimension. Midpoint-relative-
interior testing suffices for a positive-length transverse intersection of these
axis-aligned convex rectangles; it would not justify general trimmed-face contact.
Both UV representations must reproduce original controls exactly. Ordered source
vertices are construction values, not newly assigned body topology IDs.

Retained immutable source snapshots/FaceRefs and exact rational triples provide
complete result ownership without handles or borrowed storage. Invalid rows or
iterator failures cannot publish a partial batch. Arbitrary int/Fraction bit size
can cause expensive arithmetic or MemoryError; propagate failures and make no
fixed-width or query-count-only runtime guarantee.

One public-value clarification was sent to root: same_domain is false for
parallel-disjoint or noncoplanar carriers, and normal_relation is independent of
whether finite contact is empty. Root reports this clarification incorporated.
No additional behavior was inferred from a general face bounding box or from
normals alone.

Qualification appropriately uses original boundary/curve records, separate
half-space/edge-intersection contact truth, literal degeneracies and actual
world/bothUV outputs. Benchmark complete exact conversion/construction,
publication and reader costs; preparation and independent qualification remain
separate. Pure CPU placement establishes no CPU24/P100 speedup claim. The future
filtered arithmetic notes stay inactive in the return pass.

No product execution, production edit or implementation occurred in this review.
Proceed to the whole-unit plan and its adversarial review, then direct healthy-
environment red before code.
