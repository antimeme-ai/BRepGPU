# Through-bore design runtime/integration review

Read the complete `docs/BOOLEAN_DESIGN.md` against current canonical body,
exact sections/arrangements/classification, BodySampler, Patch/native conversion,
TopologyInput and history studies. **Accepted with two bounded API clarifications;
no construction/integration blocker.** No product source edits or execution.

The selected common XYZ origin and cutter.scale=2*box.scale family gives strict
throughness and radial separation for every admitted frame. It yields radius
.5*box.scale, preserving the explicit distinction from a canonical plate. Actual
sections, arrangements and exact witness classification produce the selected cap
regions; side/wall/cap decisions have complete material meaning under this narrow
proven family. Copied canonical fixtures would not satisfy that construction.

Distinct factory-only CutSolid/CutResult avoid accidental canonical admission.
The result frame governs spatial topology, while every Patch retains its actual
original frame/control/domain bits. In particular cutter wall Patch.scale differs
from CutSolid.frame.scale; original V=.25..75 and unchanged native derivatives
make the map explicit without carrier refitting. Mixed source patch frames are
already supported by the existing native ABI and JetBatch.convert(). New output
Lineage must be checked separately from retained input history.

The sampling API should specify actual submitted binary64 UV values before trim
classification. Direct exact CutSolid.classify may accept Fraction, but silently
classifying one rational point then sampling its rounded neighbor could cross
the cap rim. Either samples accept finite binary64 UV, or conversion is explicit
before labeling/binding; a separate exact query remains a different request.
The plan should name this rule alongside complete output request identity checks.

Failure publication should be stated per operation: construction publishes no
partial CutResult/history; sampling publishes no partial SampleBatch and leaves
an already valid CutResult usable. A later native consumer failure cannot
retroactively unpublish that immutable CPU construction. Private candidate arrays,
typed records, shared namespace allocation and bounded native children otherwise
match existing ownership practice; namespace gaps after failure are harmless.

Manual role/count read confirms 85 input subshapes and 81 outputs: 58 retained,
13 modified and14 deleted input dispositions, with10 generated outputs. Complete
typed mappings and original piece/event/edge-range identities, not these counts,
prove source correspondence. Closed-circle endpoints, both seam occurrences,
opposed uses, loop ownership, connected shell and cyclic vertex links are all
explicit; the corrected Euler expression accounts for cap holes. Geometry/domain
checks independently establish embeddedness; graph checks alone cannot do so.

The proposed explicit TopologyInput with snapshots=() uses existing ABI5 without
weakening pack_bodies. The actual CutResult stays retained in its caller. Canonical
mesher/classifier/FaceRef APIs retain their supported types, while result-owned
queries and sampling form a usable narrow operation. Complete CPU construction,
native setup/upload/download/frame/normal/reader/cleanup and outside independent
qualification/export costs remain distinct; no GPU construction or speedup claim
is implied. Updated the runtime study with the selected unchanged-carrier design.

Final disposition: re-read the clarified design. Sampler conversion to actual
binary64 precedes classification/binding, while exact Fraction classification
remains explicit; native consumer failure publishes no partial SampleBatch and
preserves the valid CutResult. Both clarifications are resolved. **Accepted with
no remaining runtime/integration blocker.**
