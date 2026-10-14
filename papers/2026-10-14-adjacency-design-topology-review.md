# Adjacency design: topology and identity review

2026-10-14. Adversarial review of `docs/ADJACENCY_DESIGN.md`, with the owned
body/packed semantics and the adjacency topology source study as basis. No
product edit or execution. Core relationship and reader are accepted subject
to the two small corrections below before plan development.

## T1 — instance ownership must be a function

The semantic input says an instance is a batch occurrence and each coedge must
reference edge/face sources with the same instance/body/revision. The latter
check alone admits two disconnected bodies with the same instance ordinal,
provided their individual coedges reference their respective body/revision.
Such keys remain distinct because they also include body/revision, but the
instance ordinal ceases to name the promised one source-snapshot occurrence.

Require one consistent `(body_id, revision)` per instance across every source
table. Validate even unused edge/face rows, not just coedge references. Declare
body ID and revision ranges explicitly as unsigned 64-bit values, with positive
values for qualified factory sources. The general input constructor can choose
the same explicit positive contract rather than silently admit ambiguous zero
or oversized lineage values. Different instances may intentionally name the
same source body/revision. No new body namespace or source mutation is needed.

## T2 — generic incidence labels must not imply geometry

The input supports combinatorial fixtures without geometry. Therefore opposite
uses on one face demonstrate a same-face pair, while only qualified cylinder/
plate source occurrences demonstrate the periodic geometric seam. Degree greater
than two supplies multiple uses without any angular/radial ordering. Label the
kind table accordingly, or explicitly document the short names as incidence
diagnostics with these limits. Keep mate outputs and the numeric codes unchanged:
this is naming/claim precision, not a request to add geometric qualification.

## Attacks that the design already defeats

- A seam is not deduplicated by face identity: members retain distinct coedge
  occurrences; the reader selects the other use and returns the same face.
- Closed-ring endpoint coincidence cannot collapse edge uses: vertices are not
  the incidence key, and this unit does not construct vertex adjacency.
- Nondeterministic atomic fill does not change mate identity: CSR rows are sets;
  degree-two membership selects exactly the other occurrence.
- Degree zero/one/high degree cannot cause a two-slot overflow or arbitrary mate:
  complete CSR remains available and explicit sentinels cover unmatched uses.
- Equal signs on degree two are not accepted as a paired edge.
- Repeated source snapshots remain separate batch occurrences with source
  geometry/IDs retained, rather than being falsely reconstructed new solids.
- Permuting slots rewrites typed edge/face references while semantic keys remain
  unchanged. Original loop order remains outside CSR and is not overwritten.
- Source identity and result ownership survive native cleanup; device pointers
  and geometry tolerance do not define entities.

The design appropriately stops short of editing, Boolean assembly, manifold
vertex certification, radial ordering and watertightness. Large permuted batches
provide an actual downstream reader with qualified primitive semantics. The
generic high-degree fixtures attack storage contract separately. No topology
blocker remains after T1/T2 are incorporated.

## Disposition

Re-read the corrected source/reader clauses. T1 is resolved by positive uint64
body/revision/entity ranges and one instance→body/revision mapping across all
typed source tables. T2 is resolved by same-face-pair and multiple-use names,
with qualified-body seam meaning and absent radial-order scope explicit.
Design accepted for plan development; no remaining topology finding.
