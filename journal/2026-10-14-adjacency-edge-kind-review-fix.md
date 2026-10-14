# Public edge-kind reader review finding resolved

Topology review identified a real public-interface coverage gap: independent
raw-byte qualification checks every kind, but the direct consumer tests did
not assert `AdjacencyResult.edge_kind` or its malformed-slot admission.
Extended the existing all-classes test with the literal public reader sequence
0,1,2,3,4,5,0, and included edge_kind in existing invalid-slot assertions.
This checks the actual consumer accessor against semantic truth; it adds no
tests-for-tests mechanism. No product implementation changed. Root runs the
fleet greens; this lane only edited its test unit and static checks.
