# GPU edge-use adjacency: independent qualification study

Scope: research for the first GPU topology beachhead. No new dependency, GPU
speed claim, general solid validation or topology edit is selected here. The
operation builds edge-to-coedge adjacency and an actual downstream mate reader
from immutable packed use records. Product implementation follows integrated
design and plan review.

## Sources and the seam failure

Read the owned `body.py` record definitions, packing and three canonical body
factories; `body_oracles.py` complete geometric/topological checks; body tests;
the intersection qualification and direct-production-mutation controllers;
and `docs/TOPOLOGY.md`. Existing geometry truth is a different invariant from
the new adjacency operation and should remain its own mechanism.

Read pinned OCCT 8.0.1 revision
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42` in the existing reference archive,
with restoration in `QUARANTINE.md` and `papers/reference-catalog.tsv`:

- `src/ModelingData/TKBRep/TopExp/TopExp.cxx`,
  `MapShapesAndAncestors` and `MapShapesAndUniqueAncestors`: an occurrence walk
  appends an ancestor on each use; the separate unique operation compares
  identity, optionally including orientation.
- `src/ModelingData/TKBRep/BRep/BRep_Tool.cxx`, `IsClosed(edge,face)` and
  its surface overload: seam recognition checks the closed-surface curve
  representation.
- `src/ModelingData/TKBRep/BRepTools/BRepTools.cxx`, `IsReallyClosed`: requires
  the closed representation and then checks exactly two occurrences on the face.

The primary [OCCT issue 26682](https://tracker.dev.opencascade.org/view.php?id=26682)
records regressions when ancestor duplicates were removed. The cylinder seam
has two uses by one face; collapsing them can make the seam appear to be a free
boundary. This is direct motivation for retaining coedges rather than unique
face membership. Current official
[TopExp documentation](https://occt3d.com/dev/doc/refman/html/class_top_exp.html)
distinguishes ancestor occurrences from unique ancestors, and
[BRep_Tool documentation](https://occt3d.com/dev/doc/refman/html/class_b_rep___tool.html)
describes two parametric curves on a closing edge. These sources teach the
identity distinction; they do not qualify the owned GPU implementation.

## Independent relation model

The specification is a relation between every input coedge occurrence and its
referenced edge. Each occurrence has its own typed source identity, face,
orientation and retained loop/UV provenance. Adjacency contains every occurrence
exactly once, in the row belonging to its referenced edge; GPU row order is
explicitly unspecified. Identity is `(body, revision, kind, entity_id)`, never
slot position, coordinates or a host/device address.

Use a simple dictionary/list model over the original source records: initialize
one list per logical edge, walk each original coedge once, resolve its edge and
face through the original source mapping, and append a typed use tuple. This
does not reproduce native histogram/prefix-sum/fill kernels. Compare each
actual downloaded CSR row, interpreted through its returned coedge indices,
with the model's complete multiset. Reject unknown, repeated or missing coedge
indices, incorrect edge membership, bad row bounds and incorrect final extent.
A set alone is insufficient: it hides duplicate output occurrences.

The expected downstream result follows directly from each dictionary list:

- Zero uses: unused edge, no mate.
- One use: boundary use, no mate.
- Two opposed uses: each coedge has the other coedge as its unique mate and
  the other use's face as its mate face. The mate face may equal its own face.
- Two uses with equal orientation: orientation conflict, no mate.
- More than two uses: nonmanifold incidence, no unique mate.

The exact edge-kind numbers, invalid slot sentinel and native output layouts
must come from the integrated design. Do not silently infer a mate from a
unique set of face IDs, endpoint equality, or first two entries of a high-degree
row. A closed ring with equal start/end vertices is unrelated to a periodic
seam; their use records are the reader's input, not geometry heuristics.

If a native reader emits per-coedge mates and per-edge kinds, qualification
compares both to this model independently. CPU/GPU parity is additional
regression evidence, not the source of expected results.

## Literal canonical body truth

Factory bodies use one-based typed IDs and zero-based slots. Their current
counts are box: 12 edges/24 coedges/6 faces; cylinder: 3/6/3; one-hole plate:
15/30/7. All edges have two opposed uses. The tables below are derived from
declared oriented boundary loops, not obtained by executing the new adjacency
builder. A plus/minus sign is the stored coedge orientation relative to its
canonical spatial edge, not face-normal orientation.

For the box, the complete logical edge-to-face relation is:

| Edge ID | First use | Second use |
| --- | --- | --- |
| 1 | bottom - | front + |
| 2 | bottom - | right + |
| 3 | bottom - | back + |
| 4 | bottom - | left + |
| 5 | top + | front - |
| 6 | top + | right - |
| 7 | top + | back - |
| 8 | top + | left - |
| 9 | front - | left + |
| 10 | front + | right - |
| 11 | back - | right + |
| 12 | back + | left - |

The box loop sequences, represented as `(edge ID, sign)`, are:
top `5+,6+,7+,8+`; bottom `4-,3-,2-,1-`;
front `1+,10+,5-,9-`; back `12+,7-,11-,3+`;
right `2+,11+,6-,10-`; left `9+,8-,12-,4+`.
Coedge IDs follow these six sequences consecutively, four each. This gives
literal expected coedge identity as well as face membership; selecting the
wrong coedge from a correct face pair cannot pass.

For the cylinder:

| Edge ID | Coedge ID, face, sign, UV role | Opposed use |
| --- | --- | --- |
| 1 (bottom ring) | 2, bottom, -, cap circle | 3, wall, +, v=0 |
| 2 (top ring) | 1, top, +, cap circle | 5, wall, -, v=1 |
| 3 (seam) | 4, wall, +, u=4 | 6, wall, -, u=0 |

Thus seam mates are coedge4↔coedge6 with wall↔wall. Ring mates cross cap and
wall even though each ring has the same start and end vertex. Four wall charts
remain one logical face; they must not invent four faces or extra edge uses.

For the plate, edge1..12 retain the box face-pair/sign table. Top has outer
coedge1..4 then hole coedge5, `(edge14,-)`; bottom has outer coedge6..9 then
hole coedge10, `(edge13,+)`. Front, back, right and left loops occupy
coedge11..14,15..18,19..22,23..26 respectively with the box sequences above.
The inward wall loop is:

| Coedge ID | Edge ID | Face | Sign | UV role |
| --- | --- | --- | --- | --- |
| 27 | 15 | wall | + | seam u=0 |
| 28 | 14 | wall | + | top ring v=1 |
| 29 | 15 | wall | - | seam u=4 |
| 30 | 13 | wall | - | bottom ring v=0 |

Additional mate pairs are 5↔28, 10↔30 and 27↔29. Hole loop designation remains
retained source provenance; adjacency neither changes it nor proves a region
classification. The face-normal sign of this wall is inward, but the edge
reader must use the explicit coedge signs rather than multiply by face signs.

## Storage and schedule metamorphisms

Permute edge, face and coedge storage independently, rewrite every dependent
slot, and retain logical entity IDs plus body/revision. Expected mate relations
and signed membership in logical identity space remain identical. Changing
coedge order can change arbitrary CSR row order, which is permitted. Changing
logical loop order or UV occurrence identity is not a storage permutation.

Existing `qualify_snapshot` deliberately insists IDs equal slot+1 and revision1
for its canonical fixtures. Do not use it as an oracle for a reordered record
view; that would reject the very identity contract being exercised. Qualify the
original canonical snapshot once and model the separate flattened/permuted
adjacency inputs. Where bodies are concatenated, edge/face/coedge IDs overlap
between snapshots: retain body namespaces and reject accidental welding.

Use several deterministic random incidence graphs with variable degree0..many,
sign imbalance, repeated face identity across distinct occurrences, edge and
face slot0, sparse/high logical IDs, disconnected components, empty input and
nonempty edge table with zero coedges. These graphs qualify incidence semantics,
not complete B-rep geometry. Include a many-use hot edge plus trailing empty
rows and uneven boundary-sized tables to attack atomic fill and scan tails.

Repeat one crowded GPU input enough to vary launch scheduling; compare typed
membership and reader results each time, without requiring nondeterministic
row order to vary. Test CPU1/CPU24 and both P100s against the same independent
truth. A transformed body frame should not change topology outputs.

## Malformed inputs and failure scope

Qualification should reach the actual public native entry point, not stop only
at Python validation. Native flat-input checks include null required pointers,
out-of-range edge and face references, signs other than±1, reserved fields,
unsupported counts, insufficient output capacities, size multiplication and
`edge_count+1` overflow. Check that rejection precedes input dereference for
unsupported counts: use tiny storage with oversized declared counts only where
the ABI explicitly promises pre-access validation. Null input/output for true
zero extents follows the chosen empty-call contract; it is not inherently an
error. Output capacities and rejected-call publication behavior need explicit
design wording.

Do not claim actual near-2^32 prefix totals were executed because an oversized
count was rejected. Allocation-scale prefix overflow is a distinct unexecuted
case unless the chosen resource cap makes it mathematically unreachable. Native
and Python byte-size checks and the public count cap should underwrite that
limit directly.

Run actual GPU paths under Compute Sanitizer memcheck/leak-check on both cards.
If scan uses shared-memory synchronization, a focused racecheck/synccheck run
on uneven/crowded inputs targets different faults; document unsupported tools
or device limitations rather than silently converting a skip into evidence.
Native host sanitizer tests target CPU indexing and size arithmetic. Exercise
failures with allocator/plugin fault injection only if the native unit provides
a bounded owned injection seam: actual allocation/copy/launch failure and retry
must not leak or publish a partial topology result. No new general lifetime
manager is required for this beachhead.

## Direct production mutants

Keep each mutated build/source tree isolated on the fleet. A healthy unmutated
test baseline must pass and the mutated actual operation must fail its direct
semantic assertion; build/import failures are not kills. Select actual source
sites after implementation, rather than force this list into unsuitable code:

| Fault class | Actual production mutation | Direct oracle witness |
| --- | --- | --- |
| Lost incidence | Omit final coedge or one fill store | Complete typed membership with trailing row |
| Wrong row | Fill referenced edge+1 instead | Unequal degrees and edge/slot0 |
| Wrong prefix | Inclusive scan or offset tail off by one | Empty/interior/trailing rows plus complete extent |
| Seam collapse | Reader refuses same-face mate | Cylinder seam's literal two opposed use IDs |
| Sign blindness | Two-use reader ignores orientation | Same-sign degree2 gets conflict/no mate |
| Nonmanifold truncation | Reader chooses first two of degree>2 | Crowded edge has no unique mate |
| Provenance error | Mate-face output uses own or wrong face | Distinct-face ring and box boundary uses |

CPU mutants alone do not characterize CUDA count/scan/fill faults. If the code
has separate CUDA sites, include actual CUDA builder and reader mutations and
run their direct oracle tests on the device. Shared host validation mutations
should exercise native rejection assertions, not merely Python wrappers. The
campaign size should cover implemented distinct fault classes without a tests-
for-tests certification layer.

## Complete caller and cost controls

Export actual input arrays, CSR offsets/indices, edge kinds and mate slots, plus
the original typed source map and retained loop/UV references. The useful demo
prints or draws edge uses and mate-face links, including a wall→wall seam,
from downloaded native output. Summaries without individual reader outputs
cannot demonstrate that the reader consumed GPU-built adjacency.

The measured operation must include the actual native reader. Record separate
input preparation, allocation/upload, builder, reader, synchronization/download,
typed host decoding and cleanup costs where the implementation exposes real
timing boundaries. Otherwise label combined spans honestly. Kernel-only event
timings, if collected, are a separate scope. Device transfer bytes describe
host↔device arrays, not network transfer. Retained CPU geometry/UV tables are
not extra DMA bytes. Allocation/teardown can be necessary current costs without
starting the explicitly deferred residency/session optimization lane.

Use matched tiny single bodies and a larger disconnected assembly of all three
owned bodies, not only synthetic uniform degree2 vectors. Separately label hot-
edge and general incidence stress results. Benchmark CPU1/CPU24/bothP100s with
the same exact count caps, reader semantics and exported result scope; excluded
warmup and repeated trials, fresh-process cold wall and any reuse wall must be
named. Independent full qualification belongs outside measured native spans,
but its wall cost must be reported if part of a complete caller. Pin actual
sources and all used separate topology binaries; do not rebuild/overwrite old
numerical libraries. No qualification, rebuild, copy or mutation job may overlap
owned measured trials. Preserve spread and state absence of a caller gain when
host preparation/decoding overwhelms accelerated topology work.
