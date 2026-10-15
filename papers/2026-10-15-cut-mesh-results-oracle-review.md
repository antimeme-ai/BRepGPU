# Actual CutResult mesh artifacts: independent source/oracle review

2026-10-15. Bounded review of saved results/cut-mesh-2026-10-15 output. Read
actual artifact bytes, TSV records and source hashes only; no product/test rerun,
reference construction or extra qualification code. Parent separately reports
fleet direct green12/20.765s and affected compatibility46/15.729s. Those runs are
not recharacterized as independent artifact executions by this review.

**Accepted; no blocking artifact finding.** Both root and standalone sources.tsv
have18 entries, all matching the current source/test/caller bytes. Saved mesh
row counts agree with each counts/qualification record:

| Case | Quarter segments | Vertices | Triangles | Corners | Mesh edges |
| --- | ---: | ---: | ---: | ---: | ---: |
| coarse-unit | 1 | 16 | 32 | 96 | 48 |
| fine-unit | 32 | 264 | 528 | 1584 | 792 |
| translated-scale2 | 64 | 520 | 1040 | 3120 | 1560 |
| subnormal-quarter | 32 | 264 | 528 | 1584 | 792 |
| standalone unit | 32 | 264 | 528 | 1584 | 792 |

All saved outputs report7 original faces,30 original coedge uses and genus1.
Raw triangle face IDs cover all seven. Raw wall corners use source charts7..10
and precisely V1/4 or3/4; saved graph patch scales preserve unit cutter2,
translated cutter4 and subnormal-quarter cutter1/2 separately from the respective
result/plane scales1,2,1/4. Original wall seam occurrences27 and29 retain distinct
atlas U0 and4, opposite endpoint order and the same original raw V endpoints.
This directly distinguishes actual retained cutter charts from canonical plate
reparameterization.

Each output retains15 binary packed-table components for each original operand,
original controls/UV/graph tables, both complete sections and actual arrangements,
selections, and all-kind history. Forward history records85 source dispositions:
58 retained,13 modified,14 deleted. Reverse history covers81 output origins with
10 generated. Original source kinds cover vertex10,edge15,coedge30,loop9,face9,
chart12; output kinds cover vertex10,edge15,coedge30,loop9,face7,chart10. Namespaces
in saved source section references, recipes and history target records remain
actual left/right/result identities rather than a manufactured plate identity.

The saved coarse exact bound is1/4 with zero rounding displacement. Refined unit,
translated and subnormal results retain nonzero actual rounding fractions and
separate curve/total bounds; the subnormal exact fractions are necessarily large.
Recipes retain exact tolerance, origin/scale hex, radius1/2, raw wall band and
zero native calls/host-device bytes. The saved qualifier outputs describe the
source-first exact/rounded coverage, original source correspondence and manifold
checks reviewed in the implementation oracle report; raw row/count inspection
here does not replace or repeat that geometry mechanism.

The four cases vary resolution/frame, not Boolean complexity. CPU-only mesh,
indexed reading and actual history reading are named stages. Qualified results
and original source retention support this narrow connection; no GPU meshing,
general feature mesher, whole CAD speedup or external-alpha pass follows.
