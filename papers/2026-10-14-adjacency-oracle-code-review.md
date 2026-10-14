# Adjacency oracle and test code review

2026-10-14. Independently read `python/brepgpu/adjacency_oracles.py` and
`tests/test_adjacency.py` against the accepted topology design, current body
factory and caller interface. No oracle/test edits or execution in this lane.

## O1 — public edge-kind reader lacks a direct oracle

The independent oracle correctly checks actual raw `result.kinds`, while direct
tests check public `edge_uses`, `mate` and `neighbor`. None checks the added
public `edge_kind` reader. A constant result or incorrect slot lookup in that
method would survive the test family. The caller now consumes that method and
reports read-kind counts, but repeated counts compared with their own first
result cannot certify those counts.

Correct this within `test_all_degree_sign_and_same_face_classes`: assert each
`result.edge_kind(slot)` equals the already literal `(0,1,2,3,4,5,0)` expected
value. Include `edge_kind` in the invalid public-reader slot cases. These are
direct product consumer assertions, not tests of the oracle implementation or
a new qualification layer. Finding sent to root; acceptance awaits correction.

## Sound independent mechanisms

The relation model derives typed edge keys and submitted coedge references from
immutable input bytes. It neither calls product row/mate helpers nor repeats
native scan/fill. Counter comparison checks row membership AND multiplicity:
lost or duplicated occurrences cannot hide behind set equality. Offsets must
cover the entire C records in nondecreasing bounded ranges. Every coedge is
owned by one expected row; paired uses require the exact other coedge and its
actual original owning face, while all unmatched classes require both invalid
sentinels. Equal signs and degree greater than two cannot acquire arbitrary
mates. Same-face pairs compare complete typed face identity, preserving seams.

The literal primitive relation supplies fixed edge/face/sign/loop/hole/UV
occurrences, independent of the product packer. It checks retained coedge IDs,
edge and face references, original loop ownership, hole role, exact UV-map kind
and tuple, complete original loop order, and the entire flattened typed source
set. The box plane uses and plate reversed cavity wall agree with manual
analysis of the existing factory. Factory geometry, closed-ring endpoints,
manifold/loop material roles are independently checked by the existing direct
body oracle in the primitive test; the new adjacency oracle does not pretend
incidence alone supplies those geometric claims.

The mixed/repeated snapshot test independently shuffles edge, face and coedge
arrays, then compares actual public mate/neighbor answers by complete typed
occurrence identity. Repeated cylinder entity4 remains instances1 and3.
Non-dense high 64-bit IDs in general graph tests attack slot/ID confusion.
Frames change geometry while the native incidence payload and reader truth stay
the same. Source arrays copy mutable list/bytearray/memoryview submissions.

The recursive scan case has 65,539 edges, nonzero counts on both sides of edges
255/256 and 65,535/65,536, and an empty final tail. Literal offsets include the
upper-level add boundary and final full prefix. This is a nontrivial multi-level
scan test, not a mostly-zero fixture with only an overridden final count.
Random sizes straddle scan blocks; three skewed 4,097-use calls check complete
high-degree membership without requiring schedule order.

## Limits and disposition

No test-for-tests, oracle mutant or geometric claim is needed to strengthen this
unit. Direct production mutation and native memory/admission/fault campaigns
remain root's serialized fleet work. The oracle is sound for its declared
relationship; only O1 remains before full public-reader acceptance.
