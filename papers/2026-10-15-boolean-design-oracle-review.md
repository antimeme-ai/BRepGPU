# Through-bore Boolean design: oracle review

2026-10-15. Reviewed complete `docs/BOOLEAN_DESIGN.md` against actual source
body topology and the independent material/history study. **Accepted; no
blocking finding.** No product implementation or execution.

## Material and complete embedding

The same-origin scale ratio two gives strict cap crossing and strict side
clearance at all admitted scales. The independent closed answer is box
inequalities plus radial squared distance at least cutter radius squared.
This correctly retains hole walls/rims and removes cap-disk interiors. Every
supporting face is required at rims and outer corners; complementing the closed
cutter alone would give the wrong answer on its wall.

The construction uses actual section/arrangement objects and classified complete
regions, not a renamed plate. Its completeness argument is sufficient for this
narrow family: four entire box sides, two complete cap annuli and the complete
reversed four-chart cylinder band. Original carrier bits and exact domain/map
identities establish each whole face. The cylinder lies strictly inside the XY
box sides; its only outer-boundary intersections are the two original cap
sections. The analytic domains and exact joins therefore establish the embedding,
beyond a graph/count or sampled-volume argument.

The opposed edge uses, closed oriented loop words, complete ownership, one shell
and cyclic vertex links establish the manifold sewing. Closed circle edges need
separate endpoint occurrences in link checks even though start=end. Same-face
seam uses must remain distinct. Correcting Euler for the two annular cap holes
gives 10−15+7−2=0, consistent with the genus-one complete boundary. No manufactured
plate constructor or generic self-intersection engine is required as an oracle.

## Independent all-kind history count

Source box entity counts in vertex/edge/coedge/loop/face/chart order are
(8,12,24,6,6,6), totaling62. Source cutter counts are (2,3,6,3,3,6), totaling23.
All85 inputs therefore have dispositions.

The box retains58 entities: everything except the two cap faces and two cap
charts. Those four are modified because the retained trim domain changed,
although the carrier controls did not. The cutter contributes nine modified
entities: wall face1, wall loop1, wall charts4, seam edge1 and seam uses2. Thus
there are13 modified images, and the remaining14 cutter entities are deleted:
end vertices2, original cap rings2, ring uses4, cap loops2, cap faces2 and cap
charts2. A cropped/reversed wall is not deleted merely because its original
outside bands are absent.

The output has71 retained/modified images plus ten generated entities: anchors2,
rim edges2, rim uses4 and cap inner loops2. Total81 matches the complete output
(10,15,30,9,7,10). The generated rim entities must name both participating source
faces and actual component/piece/event identities; anchors additionally name the
source seam support. Forward/reverse relation equality and explicit source/output
inventories check genuine mappings, not these counts alone. Unchanged carrier
controls cannot justify marking an entire changed-trim cap or wall chart retained.

## Native consumer and failure boundaries

Keeping original source carriers and original UVs is sound. Planar charts retain
the box frame; wall charts retain the **cutter** frame and V trim [.25,.75].
Native position and partial conversion must use each patch's descriptor. A
sampler that instead uses CutSolid.frame globally would place/scale the wall
incorrectly and fail original-source map/normal checks. No source parameter
reparameterization is implied by the new spatial seam/rim curves.

The transactional construction and immutable source ownership support direct
late-exception/input-table/healthy-retry checks. Consumer failures must publish
no partial consumer output while leaving already completed immutable solids
usable; construction failures publish no CutResult. Namespace allocation need
not be rewound. Existing unsupported cases, canonical API exclusions and external
community alpha debt remain explicit. Reference difference is complementary
geometry evidence and cannot supply the owned all-kind history semantics.
