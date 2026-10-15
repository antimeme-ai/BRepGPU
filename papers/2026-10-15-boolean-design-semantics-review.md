# Boolean design: geometry, topology and history review

Disposition: **accepted after B1 correction**. Source-only adversarial review of
the complete `docs/BOOLEAN_DESIGN.md` against the actual body, intersection,
arrangement and classification records plus the source/history study.

B1: raw arrangement loop orientation is independent of the target Face sense.
Both original box cap parameterizations have U×V in +Z, so their raw arrangement
circle-hole use is -1 on both caps. Copying this use to the outward bottom cap
would give the same traversal as its adjacent wall rim, violating closed oriented
assembly. The revised design explicitly reverses the raw word/sign for face
sense -1, retaining original arrangement loop/use IDs and the orientation
relation. Upper/lower cap rim signs are -1/+1, opposing wall signs +1/-1. Verified
the correction in the actual design before acceptance.

The admitted same-origin inputs with cutter scale twice box scale establish
strict cap penetration and strict XY separation for all four allowed box scales.
The constructed hole radius is box scale/2, distinct from the canonical plate's
box scale/4. The distinct CutSolid type, actual cap sections/arrangements and
retained region decisions prevent a mislabeled recipe substitute. The closed
regularized material predicate retains the cylindrical cut boundary and returns
all incident output faces at rims/corners.

The wall keeps original carrier control, UV and **cutter-frame** bits while the
topological geometry uses the box frame. Its actual face trim is source V in
[1/4,3/4], with no hidden parameter normalization. Existing line/circle/UVMap
records can express these exact curves and lifted seam occurrences. Opposite
same-face seam uses share one physical edge; atlas joins remain geometry events.

The proposed ownership and history are complete: box inputs contribute 62
subshapes and cutter inputs 23. Output counts total 81. The declared 58 retained,
13 modified and 14 deleted inputs total 85; 10 generated outputs complete 81
images. Modified cap/chart domains, cropped/reversed wall and seam, generated
intersection rims/anchors and deleted original cutter caps/rims are consistent.
Full source namespaces and orientation/range maps are retained separately from
fresh output IDs. This is an owned operation history, not external-kernel history
compatibility or general persistent naming.

Graph checks are complemented by full analytic carrier/domain/curve identities;
Euler includes the two cap holes. Exact rational queries and the native sampler's
actual binary64 query classification have separate semantics. Construction
failure publishes no candidate; a later consumer failure preserves a previously
constructed immutable result and returns no partial batch. Existing canonical
consumers and alpha external qualification are not silently widened.

No production change or product/reference execution was performed.
