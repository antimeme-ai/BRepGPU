# Intersection design: two-sided consumer review

2026-10-14. Accepted after IR-R1/IR-R2 corrections. Reviewed
[INTERSECTION_DESIGN.md](../docs/INTERSECTION_DESIGN.md), the actual existing
Engine/resource/JetBatch source, body sampler cleanup/provenance pattern and
current immutable body inputs. No product code or native execution.

The subsequent span clarification is accepted: `Piece.span_id` groups chart
occurrences of the same spatial span. Sampler rows/export retain both span ID
and piece ID; consuming both occurrences preserves provenance without implying
additional geometry.

## IR-R1 — exact-parameter and resource bounds: resolved

The first consumer paragraph left candidate/precision caps and the admitted
parameter fields unspecified. Arbitrary algebraic interior interpolation can
require mixed radical arithmetic even when each endpoint separately has degree
two. The corrected contract uses factory-only immutable requests, exact
endpoints and rational interiors, resolution 2..4097, at most 2^19 candidates,
and at most 4096 inward-window binary subdivisions. Precision failure occurs
before native work and does not invalidate complete exact geometry. One point
occurrence is one output candidate/two native rows. This is concrete and fits
the existing order-zero batch boundary.

## IR-R2 — success grade and arithmetic failure: resolved

An empirical local evaluator target does not alone certify every returned
intersection sample. The corrected sampler checks finite actual outputs,
componentwise distance to the exact curve, and exact pairwise source-position
differences before publication. A failed 2^-36 grade raises named
SamplingArithmeticError for the entire batch. World/scatter and grade checking
are distinct timing scopes; rounded UVs do not become exact trim labels.
The independent oracle still binds actual original controls/frames and verifies
whole-map identities, so the product's own model check is not the sole geometric
truth mechanism.

The error arithmetic is reasonable for this factory family: circle XY first
derivatives are <=2R<=2, axial span<=4, coordinate-plane spans<=6; nearest UV
rounding on [0,1] is <=2^-54. Maximum frame scale four turns a local 2^-40 budget
into 2^-38 per side, and 2^-48 affine slack plus UV terms leave ample room under
paired 2^-36. Forward position sensitivity stays bounded near tangency; no inverse
Jacobian or normalization by a vanishing normal cross product is used here.
The added exact per-row grade check enforces success independently of that
qualification-based budget argument.

## Source/lifetime/interface disposition

Result left/right FaceRefs bind exact source snapshots/faces. Pieces retain
original snapshot patch slots and inspectable coupled spatial/UV maps; native
PatchSet slots are a separate dense map keyed by side and original slot, not
an assumption that original slot numbers are globally unique. Compare actual
returned slot/UV bytes and complete source descriptors, including frames, source
rectangles, lineage, degrees and compact control offsets/counts. Native row order
and scatter must be specified consistently in the implementation plan.

An endpoint's algebraic value remains in the request; its nearest native UV is
only an approximation. Exact endpoint/interior membership comes from the complete
result and request constructor. The sampler must not reclassify it through the
body classifier. Point pieces have lo==hi and no invented self-loop; chart/seam
occurrences remain distinct source mappings even at the same XYZ.

The sampler borrows Engine, owns one PatchSet and scopes fresh query/result
children. Constructor failure must consume unpublished/adopted ownership;
nested BaseException guards attempt every close and preserve the exact primary
object with guarded cleanup attachment. Close failure on success prevents batch
publication. Exact result identity is checked before native work; closed sampler
or Engine fails operations while immutable geometry remains usable. Empty
intersections carry zero native rows and no fabricated successful geometry.

Caps, shape/status/source checks and all arithmetic/cleanup failures prevent
partial successful batches. Existing ABI1 order-zero positions suffice; no
native source/library changes or repeated native arithmetic campaign are needed.
Report CPU construction/request work, setup, gather, actual query/evaluate/
download/conversion/grade/cleanup and external wall separately. Device payload
is 40 bytes input/56 bytes position-status output per two-sided candidate,
excluding metadata and geometry setup. No GPU construction, contact or
end-to-end CAD acceleration claim follows.
