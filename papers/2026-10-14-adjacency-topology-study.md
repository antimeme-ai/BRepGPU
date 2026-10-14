# GPU edge-use adjacency: topology semantics

Research, 2026-10-14. This study reads the owned body records/oracles and pinned
OCCT/Truck source. It proposes one GPU adjacency beachhead; no product code,
dependency adoption, native execution or performance result is claimed.

## The relationship being constructed

For an immutable packed snapshot with E edges and C coedges, define

`U(e) = { c in [0,C) : coedge_edge[c] = e }`.

Every coedge occurrence belongs to exactly one row. The result is E+1 offsets
and C coedge slots, with offsets[0]=0, offsets[E]=C and each row containing
exactly U(e), with no duplicates or omissions. Zero-degree rows are legitimate
for the generic incidence primitive; they do not describe an unused edge in the
current qualified closed body fixtures. This relationship is independent of
vertex coordinates, curve controls, face chart count and GPU scheduling.

The existing byte export already supplies the relationship. A coedge row is
`<QIIIIi>`: 64-bit coedge ID, uint32 edge/face/loop/UV-map slots and signed
orientation. IDs are not address values. The UV-map slot distinguishes the two
seam representations, while the face slot alone does not. Read the real packed
bytes or project their fields once; do not create an unrelated synthetic graph
for the product caller. Geometry need not be copied to construct incidence.

The authoritative loop sequence stays in `loop_coedges`; edge-row sequence does
not define loop traversal or radial geometric order. CSR construction is a
derived index, not an independently edited topological authority. Atomic fill
may make the row sequence nondeterministic. The design must either promise a
defined canonical row order and implement its cost, or explicitly define rows
as occurrence sets. Slot permutation changes numeric slot ordering, so semantic
comparisons must resolve typed identity even if rows are numerically sorted.
For this beachhead an unordered CSR with a schedule-independent semantic reader
is sufficient; a sorted row API is an optional contract choice, not a theorem
about count/scan/fill.

## Lessons from the retained references

OCCT distinguishes shared underlying shape, shared shape plus location, and
shared shape plus location and orientation. Its ancestor-map implementation
appends occurrences; its unique-ancestor variant deduplicates using shape
equality selected by an orientation flag. Edge-to-distinct-face incidence thus
cannot stand in for edge-to-coedge occurrence incidence. A second pcurve on a
closed surface is explicit in its curve representation. These source facts
motivate the owned occurrence-level relationship above; OCCT pointer identity
is not selected as our identity mechanism.
[Shape equality](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/TopoDS/TopoDS_Shape.hxx),
[ancestor maps](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/TopExp/TopExp.cxx),
[two pcurves](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRep/BRep_CurveOnClosedSurface.hxx).

Truck's compressed shell separates vertex, edge and face arrays. Each boundary
contains oriented references to shared edge indices. Compression maps live
edge identities to indices and preserves each boundary occurrence. Rebuilding
faces follows those oriented references. Its comments distinguish the exchange
form from the live editing representation. We infer that dense exchange records
are a useful ownership boundary for our immutable GPU input; neither Truck's
serialization dependencies nor its live pointer-derived IDs are adopted.
[Compressed topology](https://github.com/ricosjp/truck/blob/88ed005249e5e3a6b07f62425399435905cd3ab6/truck-topology/src/compress.rs),
[live IDs](https://github.com/ricosjp/truck/blob/88ed005249e5e3a6b07f62425399435905cd3ab6/truck-base/src/id.rs).

## Seams, closed edges and variable degrees

The owned fixture counts are box E=12,C=24,F=6; cylinder E=3,C=6,F=3;
one-hole plate E=15,C=30,F=7. Every edge has two opposed coedge uses. The
cylinder/plate seam has two uses of one edge on one wall face. Their distinct
coedge IDs, opposite traversal signs and distinct UV maps must all survive.
There is no valid requirement that neighboring face slots differ.

Each circular rim has one edge with identical endpoint vertex slots. It is a
closed spatial edge, not two merged edge records, and still has two distinct
incident coedges. Endpoint incidence multiplicity matters to vertex valence,
but vertex adjacency is outside this unit. Chart joins on the wall do not create
additional physical edges or coedges. The four wall patches describe one face.

Variable-degree storage must not hard-code two slots per edge merely because
the canonical fixtures are closed manifold bodies. The generic constructor can
accept any valid finite coedge→edge map, including degree 0, 1, 3 and a highly
skewed row. Such graphs are incidence fixtures, not qualified geometric solids.
Keep that distinction in tests, artifacts and performance wording. Validate
edge/face references and orientation values before use; do not silently drop
an out-of-range coedge into a zero or empty row.

## Downstream reader that demonstrates actual use

For the owned bodies, a useful reader answers: which other oriented use shares
this edge, and which face owns that use? For each coedge c:

1. Locate its row using `coedge_edge[c]` and CSR offsets.
2. For degree two with opposed signs, select the other coedge by occurrence
   identity, then read that coedge's face and orientation.
3. Emit the mate coedge slot and mate face slot, plus an explicit class for
   non-two-use rows or conflicting signs if those are supported inputs.

The cylinder seam returns a different mate coedge on the SAME wall face. Its
ring edges return the cap/wall pair. Box edges return distinct plane faces.
Plate hole-rim edges return the planar hole-loop face and cavity wall; their
hole role remains available through the original loop records. This reader
does not need a face-pair deduplication pass. Distinct-face neighbors can later
be derived deliberately, but that derived view must retain the originating
edge/coedge uses and cannot replace them.

Two-use incidence alone does not prove correct geometry, oriented loop closure,
an embedded manifold or material membership. Continue to use the existing body
oracle for those supported fixture claims. Conversely, do not rerun exact
surface geometry for every cached adjacency timing iteration: fixed qualified
source bodies and an independent incidence/reader oracle address the new unit.

For arbitrary degrees, do not publish an arbitrary first mate. A boundary use
has no mate; a nonmanifold edge may have several other uses and no radial order
is supplied by this input. A fixed-size mate reader should report that scope
explicitly. A general all-other-uses reader would produce sum d(d−1) outputs
and brings a different sizing/performance contract; defer it unless needed.

## Identity, slots and a useful large input family

Body lineage is `(body_id, revision, entity_kind, entity_id)`. Edge, coedge and
face slots are typed dense addresses in one immutable packed input. Device work
may use only slots; the Python input and result must retain the original source
mapping. The result must reject interpretation through another input with the
same counts. Native handles name allocations, not geometric entities.

A large workload can be the disjoint union of replicated owned box/cylinder/
plate records. Label it a batch of body instances, not a newly constructed
connected solid. Each instance needs an occurrence key in addition to its source
snapshot lineage if the same snapshot appears more than once. A practical key
is `(instance_ordinal, source_body_id, revision, kind, entity_id)`. This preserves
source identity without mutating the source snapshot or falsely claiming
cross-session/import/edit identity. If instead the caller constructs distinct
owned bodies, their separate factory body namespaces already distinguish them;
report that CPU construction cost and memory explicitly.

Flattening must translate edge, coedge, face and loop slots by their respective
instance offsets; it must not reuse one numeric offset for all kinds. Instance
ownership checks reject cross-instance references. No coordinate comparisons
or welding are needed. Packed instance records plus source maps can drive the
actual reader at hundreds of thousands of uses while retaining the known seam
and hole roles.

At least one measured large input should globally permute coedge storage and
rewrite every dependent reference. Otherwise blocks of adjacent degree-two
uses can accidentally make a broken fill/reader look correct and may overstate
cache behavior. A slot-permutation qualification also permutes edge/face slots
and resolves result comparisons through their complete typed instance/source
keys. It must preserve original loop order by rewriting its coedge slots,
rather than sorting loops. A geometric frame change cannot affect adjacency;
geometry sampling remains a different operation.

## Recommendations for design review

Specify unsigned count/offset widths, the invalid-slot sentinel, empty E=C=0
behavior, overflow checks and mutation-free publication. Offsets may need one
more representable value than slots; arithmetic checks precede allocation and
pointer arithmetic. A count/scan/fill implementation should retain valid
CPU results and input immutability on allocation or device failure.

Separate immutable host input, uploaded native topology, constructed CSR and
reader output ownership. Downloaded immutable rows must remain usable after
device cleanup. Name whether the cached benchmark rebuilds CSR, only reruns
the reader, or does both. Include cold setup, packing, native validation,
upload, construction, reader, download, source decoding and final cleanup in
the corresponding measured scopes. Topology readback bytes are neither curve
control bytes nor network transfer bytes.

The independent new oracle should derive U(e) from submitted packed fields,
then check all row membership and actual mate identities. Literal cylinder seam
and box/plate role expectations add semantic attack power. Wrong-edge remap,
omitted use, duplicated use, same-face seam collapse, arbitrary mate on degree
three and stale input interpretation are direct fault classes. GPU/CPU equality
alone cannot certify them. Source-level production mutants belong on the fleet.

For round two, retain deterministic sorted-row views, other CSR relationships,
general radial ordering, incremental updates, segmented storage, transaction
history, session-level reuse and consumer bulk decoding. None is required to
demonstrate this edge-use adjacency beachhead.

## Owned files read

- `python/brepgpu/body.py`: canonical constructors, frozen records, packed tables.
- `python/brepgpu/body_oracles.py`: complete fixture correspondence, ownership,
  opposed uses, seam identity and hole-sensitive Euler checks.
- `docs/TOPOLOGY.md`, `papers/2026-10-14-body-topology-study.md` and the pinned
  acceleration roadmap source study.

No production changes or tests were made by this research lane.
