# Adjacency plan: topology, identity and reader review

2026-10-14. Reviewed `docs/ADJACENCY_PLAN.md` against the accepted design and
topology study. Accepted: no topology blocker or scope correction required.
No implementation or execution during this review.

The cylinder first red is concrete and correctly uses coedge IDs 4 and 6 for
the two wall seam occurrences. The current factory emits top/bottom cap uses
as IDs 1/2, then wall bottom rim/seam/top rim/seam as IDs 3/4/5/6. The expected
same-wall pairing is therefore literal source truth rather than slot-count
parity. It must fail at the missing new operation after healthy body creation.

The prepared interface enforces one body/revision per instance, typed source
uniqueness, valid lineage references and signed uses before native work. Reusing
one source snapshot in separate instances remains explicit. Edge/face/coedge
permutations retain source keys and leave authoritative source snapshots/loops
unchanged, while rewritten flattened references address the new typed storage.

The planned qualification directly covers absent uses, duplicate/lost members,
degree zero/one/high degree, same-face pairs, conflicting signs and independent
storage permutations. Generic incidence models remain separate from literal
geometric body qualification. The 65,537-edge case crosses a 256×256 scan level
and prevents treating one block-total scan as a complete recursive scan.

The actual caller consumes native rows and emitted mate/neighbor arrays, then
resolves their typed identities. Four real callers include CPU and both GPUs,
seam and hole-plate roles. The serial benchmark is correctly 3 sizes × 2 storage
orders × 4 placements = 24 cases. The largest plate batch has 491,520 edges and
983,040 coedges, within the declared 2^22 limits. These are disjoint qualified
primitive instances rather than a claim about one complex solid. Globally
permuted edge/face/coedge storage attacks locality assumptions and slot/ID
confusion while preserving the same relationship.

Implementation advice for the later caller unit: make the small benchmark batch
explicitly box+cylinder+plate; retain complete source/instance mapping in exported
rows; consume actual emitted mate/neighbor values instead of reconstructing
them from original coedges; and give caller/benchmark work its own written
conceptual subplan before code. These details realize the accepted plan and do
not require a new design or broadened scope. Independent qualification outside
the measured wall must still be reported as a caller cost, not erased.

No request is made to sort CSR rows, construct radial order, add vertex
adjacency, infer geometry or integrate session residency in this beachhead.
