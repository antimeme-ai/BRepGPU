# Independent source reader/runtime ownership review

Scope: actual `native/mesher/source.hpp`, `source.cpp`, `resources.hpp` and the
source-value row definitions. Static independent review during the next bound
unit's genuine RED; no additional execution campaign or product edits by reviewer.

## Finding

`load_source(path, resource, limits)` constructs a PMR byte vector from
`resource.get()` before reaching `read_source`'s explicit nonnull guard. Passing
a null resource to this public overload therefore installs an invalid allocator
and can dereference it for a nonempty file, instead of throwing the same typed
internal_error. Guard at load_source entry, before any allocation, and exercise
that public path in the existing source fixture. This is a concrete boundary
failure, independent of source CAD content.

## Reviewed mechanisms

The source reader bounds every take by remaining bytes, nested rows must be
fully consumed, section order/count/size and EOF are checked, and variable counts
are checked against remaining payload before allocation. Scalar/ID reads and
UTF8 bytes consume work. UTF8 rejects overlong, surrogate and out-of-range
sequences. Finite words and signed zero are retained without rescaling the
source's already-mm spatial values by original-unit metadata.

RunResource's atomic reservation precedes actual allocation and rolls back on
upstream failure; deallocation releases reserved capacity. SourceData's resource
is destroyed after its PMR fields, and allocate_shared's retained RunAllocator
keeps the resource alive through control-block deallocation. load_source retains
its resource while a borrowed-input read_source call can throw. The extra input
copy on this convenience path is charged and is not a hidden budget bypass.

ID indices reject zero/duplicates and typed references resolve through them.
Definition/face/loop/use ownership, original ordered loop endpoint occurrences,
edge endpoints within definition vertex inventories and face edges within edge
inventories are checked. Seam uses survive. Generation-stamped inventory checks
avoid per-definition whole-table resets; CSR construction starts from resolved
in-range pairs and preserves occurrence multiplicity where required. Amortized
pool push growth avoids the already-fixed exact-reserve quadratic copy pattern.

The usage graph is processed without recursion and detects cycles even in
unreachable components. Root-reachable path and placement counts are checked
before addition; limits.records>=1 follows earlier mandatory metadata-row
admission, so subtraction underflow is excluded by the maintained count bounds.
Per-source original placement admission retains the existing approximate rigid
tolerance; exact composed transforms and world-error transport belong to the
later construction stage, not an invented exact-orthogonal parser promise.

Saved span shape/weight checks do not make saved span approximations original
truth. The selected geometry unit recomputes extraction from original controls/
knots. Active-map/geometric consistency and singular/profile admission remain
the geometry procedures; this parser does not silently replace source maps.

Reread the actual fix: load_source rejects a null resource before stat or PMR
construction. The existing source fixture calls the public overload with a
nonexistent filename and a null resource, requiring internal_error before any
filesystem/allocator access. The function also deliberately retains its shared
resource owner through staging-byte destruction on read_source failure.

**ACCEPT** in the inspected source ownership/length/allocation/graph/CSR scope.
Parent owns the already scheduled affected source Release/ASan reruns; this
review starts no duplicate campaign. It does not claim completion of native
meshing or independent geometric qualification.
