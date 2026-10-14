# Adjacency diagram branch correction

2026-10-14. Root's static caller review found that horizontal paths to two
same-row use boxes crossed the first box, making distinct occurrences appear
like a chain. Changed only SVG placement: each edge now branches at a common
junction into two vertically separated boxes. Both labels still read actual
emitted coedge/mate/neighbor identities, and cyan still marks same-face pairs.
Binary/TSV export values and measured timing phases are unchanged. No existing
artifact was regenerated and no fleet/product execution was run by this lane.
Static Python AST parsing passed after the correction.
