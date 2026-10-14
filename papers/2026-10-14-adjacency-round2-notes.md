# Adjacency round-two notes

Schedule: AFTER the broad-pass big rocks. The working adjacency operation moves
the pass to another uncovered theme. These are source-grounded future options,
not an active tuning lane or selected dependency.

[Measured results](../results/adjacency-2026-10-14/REPORT.md): stages around0.32ms
ordered/1.4ms shuffled on P100; full native15–22ms and typed Python traversal
4–5s. Ordered CPU1 is8.2ms; shuffled CPU24 is19.4ms. Packing costs8–9s and
permutation adds8s. These named scopes guide later work; no full gain is established.

## Placement and consumption

The first operation uploads compact slots/signs, constructs CSR, reads unique
mates/neighbor faces on GPU, and downloads full CSR/reader arrays. Python then
resolves every actual use/mate/neighbor to a source occurrence for the comparison.
Later geometry/topology consumers should operate directly on these packed rows
rather than round-trip and box every source. Expose only the host results a real
caller needs, while preserving exact lineage, complete incidence and failure.
Do not omit qualification or replace source identity with coordinate proximity.

Prepared inputs expand source records for repeated batch instances. A compact
body template plus explicit instance mapping could reduce host preparation and
storage without inventing body IDs; the full typed key remains instance/body/
revision/kind/entity. Slot permutations and edited snapshots must invalidate or
rewrite dependent device storage. Input size alone cannot identify a cache.

## Lifetime, CPU scheduling and profiling

The synchronous ABI deliberately owns fresh stream/buffers per call. Future
sessions could retain prepared input, counts/cursors, hierarchy scratch and
outputs under explicit device/context ownership and budgets. Readers cannot be
overwritten while live; cleanup, primary failures, fatal device diagnostics and
previous-device restoration remain part of the contract. The first beachhead's
recoverable failure tooling and distinct-device probe are useful prerequisites,
not guarantees about unknown driver failures.

CPU24 creates worker groups per count/fill/reader operation and scans prefixes
serially. Retaining workers or changing placement thresholds deserves complete
caller measurements; CPU1's owned direct counters are the current fair baseline.
The P100 path synchronizes phase clocks for instrumentation. Optional event
profiling or fewer synchronization boundaries is a later whole-operation study,
with explicit timing scope and unchanged completion/cleanup behavior.

## Literature options after measurement

The owned scan follows work-efficient hierarchical tree scans. Merrill/Garland
2016 decoupled look-back reduces traffic, but descriptor publication and forward
progress/deadlock constraints need a fresh reviewed implementation and actual
P100 timings. Do not select it from asymptotic traffic alone. Hot-row atomics
may benefit from bounded local aggregation; every coedge occurrence still must
be emitted. Stable ordering is a separate contract and cost. Neither coedge-ID
sorting nor atomic fill order establishes a geometric radial order.

Read sources and acquired papers: [runtime study](2026-10-14-adjacency-runtime-study.md),
[BibTeX](adjacency-runtime.bib), [catalog](adjacency-runtime-sources.tsv).

## Subsequent topology scope

Generated intersection events are still local exact geometry, not newly edited
body edges/coedges. Planar arrangements must establish shared events, region
classification and directed loops before Boolean publication. Compaction needs
old-to-new slot maps and unchanged entity IDs with revision-aware readers.
General vertex/shell manifoldness, poles, degenerate boundaries, radial order
and persistent cross-session names require separate operation-specific truth.
The current degree-two reader certifies only its declared incidence relation.
