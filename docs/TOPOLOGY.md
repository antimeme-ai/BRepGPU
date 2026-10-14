# Topology identities and packed body snapshots

Architecture, 2026-10-13, status updated 2026-10-14. The first numerical
iteration carries lineage. The [body sampling beachhead](BODY_SAMPLING.md)
now implements a narrow read-only subset: CPU-built canonical box/cylinder/
one-hole-plate snapshots with shared boundaries, loops, charts and packed tables.
General construction/edit history and device topology remain proposed here.

## What the references teach

SMLib's live topology uses pointers, linked edge uses and mate/radial relationships.
Its `SmBrepData` export already packs face-loop and loop-edge-use ranges, but some
records still contain host pointers or borrowed geometric objects. It is an
adapter source, not a structure we can copy directly onto a GPU.

Truck separates geometry and topology cleanly, but its live IDs derive from raw
pointer addresses and geometry uses shared ownership/mutexes. Its compression
format maps those objects to dense oriented references for storage. OCCT
separates an underlying shape, placement and orientation; its several equality
relations matter when deciding whether two uses share geometry or topology.

We learn their relationships and algorithms while giving our data explicit
identity and ownership. Source details are recorded in the acceleration/topology
study under `papers/` and the [reference map](../papers/REFERENCE_IMPLEMENTATIONS.md).

## Three kinds of identifier

| Identifier | Meaning | Lifetime |
| --- | --- | --- |
| Entity ID | Logical vertex, edge, coedge, loop, face, shell or body identity | Preserved for an unchanged entity through supported edits; never derived from its address or coordinate |
| Snapshot slot | Dense index into one typed array | Valid only for the named immutable snapshot; may change after an edit or compaction |
| Runtime handle | A checked reference to an engine, buffer or result allocation | Process-local lifetime; has no geometric identity and is never a device pointer exposed to Python |

Entity handles name a body and a 64-bit ID, with an explicit entity kind.
Snapshots have a 64-bit revision. Device references use checked 32-bit slots;
the initial format rejects counts requiring the reserved invalid slot. CPU
adapter maps resolve entity IDs to slots at snapshot creation. Kernels receive
slots in contiguous buffers and do not probe a host hash table per query.

Snapshot lineage is the complete tuple `(body_id, revision, entity_kind,
entity_id)`, plus chart identity where geometry is parameterized. A revision
or face ID alone is not globally unique across bodies. A Boolean output owns
a new body namespace and emits explicit input-to-output history relations for
unchanged/transferred, split, merged, generated and removed entities. Within
single-body edits, unchanged entities keep their IDs. Cross-body history is
not a silent reuse of one operand's namespace.

The ID allocator does not recycle IDs within the process/session. Serialization
needs an explicit namespace and import mapping before cross-session identity is
promised. Semantic topology equivalence does not require independently rebuilt
bodies to have identical IDs.

## Canonical records and adjacency

| Packed records | Canonical relationships |
| --- | --- |
| Vertex | Stable ID and geometric position/tolerance record |
| Edge | Endpoint vertex slots, one spatial curve with its explicit parameter range, tolerance and stable ID |
| Coedge | Edge slot, owning face/loop, UV curve/chart with its parameter range, edge-to-UV correspondence, and orientation |
| Loop | Ordered range of coedge uses; distinguishes outer boundary and holes |
| Face | Carrier surface/chart and a range of loop slots |
| Shell/body | Ordered face uses, orientation and declared manifold/solid scope |

Store variable adjacency as offset/count ranges and packed index arrays: edge
to coedges, vertex to incident edges, face to loops and loop to ordered coedges.
This is the compressed sparse row pattern. Canonical loop order defines
traversal; a kernel-specific next/previous cache may be derived from it, but it
is not a second independently edited authority.

Geometry occupies separate homogeneous-control and curve arrays. A packed
record contains typed indices and ranges, never a C++ object pointer, virtual
dispatch target, allocator address or Python object. Use structure-of-arrays
where a kernel reads a few fields; layout choice does not change identity.

A manifold regular edge has two opposed oriented uses. These need not belong
to two different faces: a periodic seam can use the same edge twice on one face,
with distinct UV curves/chart offsets. Poles and degenerate boundaries are
explicitly represented rather than approximated with fabricated tiny edges.

Generated intersection edges use a canonical normalized parameter `t` in
`[0,1]`. Their spatial curve and each incident UV curve have declared maps from
that `t` into their stored curve ranges. Coupled fitting uses the shared
correspondence, and opposite traversal reverses its order. Orientation alone
does not establish same-parameter or same-range correspondence. Imported curves
whose parameter relationship is unknown remain unresolved until the adapter
establishes a map or the operation reconstructs a qualified common-parameter
boundary. Publication checks geometric agreement along that correspondence,
using the operation's bounded contract rather than endpoint agreement alone.

## Edits, intersection identities and compaction

An operation builds a candidate snapshot in separate storage. It preserves IDs
for unchanged entities, assigns IDs to new/split entities, and records generated,
modified and removed relationships for callers. A split face does not pretend
that every resulting piece retained the original face's identity.

An intersection event creates one shared spatial edge identity and the distinct
oriented face uses/UV curves that refer to it. Canonical spatial event/edge
identity uses operand-order-normalized source entities and branch/segment event
provenance. Per-coedge occurrence identity additionally carries its owning
loop/face use, chart/period offset and orientation. These are separate keys:
opposed uses or different seam charts must not create separate spatial edges.
Coordinate proximity
alone cannot merge edges or vertices: nearby points can belong to different
branches, and tolerance proximity is not a transitive identity relation.

Count/scan/fill and sort/group operations can construct dense arrays and
adjacency on GPU once their semantics are established. CPU construction can
produce the same format first. Compaction emits an old-slot-to-new-slot map and
rewrites every dependent reference; stable entity IDs remain unchanged. Results
carrying old slots also carry their snapshot revision and cannot silently be
applied to a newer snapshot.

Before publication, check referential ranges, oriented loop closure, shared edge
incidence, the declared vertex/shell topology, orientation, material selection
and geometric compatibility of spatial edges and incident UV curves. These
checks are operation-specific: topology incidence alone does not certify
watertight geometry or exclude self-intersection.

Failed classification, unresolved intersection boxes, memory/queue exhaustion
or failed output checks leave the previous body intact and return diagnostics.
Readers holding that snapshot remain valid. A CPU fallback must rebuild the
whole operation or participate in a defined shared-event protocol; partial
results are not welded together by their coordinates.

Start with body-sized rebuilds and immutable shared geometry. Profile edit size,
allocation, map construction and transfers before adding segmented arrays or
incremental adjacency repair. Flattening improves access patterns; it does not
remove algorithmic irregularity or make full-body copies free.

## What iteration 1 actually carries

Each patch records parent face/entity ID, source snapshot revision,
parameterization identity and its source UV rectangle. Query/results name an
immutable patch set and dense patch slots. These are lineage records, not an
implemented body graph or a claim that a sample lies inside a trimmed face.

Reference-kernel pointer maps exist only inside the optional adapter while it
extracts a live reference model. Exported identities come from the adapter's
explicit model/snapshot mapping; no pointer address is persisted as a face ID.
Changing or rematerializing the carrier invalidates that mapping unless the
adapter explicitly establishes correspondence.
