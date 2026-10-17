# Definition contact owner source review

Actual-source review of contact_epoch.hpp/.cpp and the fixed-site flip addition
in topology.hpp/.cpp. The contact owner waits for the complete original-face
horizon, retires old facet revisions, inserts source-keyed current facets, and
queries changed facets against the live definition BVH. It compares each new
pair once, retains forbidden pairs through an intrusive incidence list, and
invalidates those pairs when either facet retires. The completed epoch marks
both current cell keys. Frozen child batches retain actual facet preparations;
neither a mutable Budget nor a topology owner crosses to children.

Shared physical node slots require identical source-derived keys and exact
binary64 XYZ words. Permission for an edge additionally requires the same
declared edge identity; three common nodes imply no permission. Original source
edge declarations must belong to the definition. Face-local declarations retain
face generation. There is no coordinate welding or same-face blanket exemption.

The optimizer approval retains the exact two old cells, proposed physical keys,
XYZ, declarations, owner identity, epoch and scene revision. Proposal checks
cover its new pair and both new facets against the remaining live BVH. The root
must reserve the sole optimizer transaction through face commit and scene
replacement; this requirement remains explicit in the API. The topology flip
stages without mutation, requires two positive new triangles and an absent new
diagonal, preserves the surrounding constraints, and records both retired
lineage parents. Allocation failure poisons the affected owner rather than
publishing a partial transaction.

ACCEPT for these owner/transaction responsibilities. Focused direct semantic
tests and a consistently instrumented ASan/UBSan/leak closure pass for contact,
topology and shared-boundary export. This does not claim that the full mesher
coordinator has executed or that all geometric chart families are implemented.
