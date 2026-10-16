# Native adaptive patch caller: scoped source review

Read actual `adaptive_main.cpp`, `output_directory.hpp` and
`tests/native_adaptive_caller.py` after the packet operation's first direct
GREEN. No additional caller run or geometry campaign was introduced by this
review. Scoped caller/publication source ACCEPT; runtime geometry/lifetimes
remain their owning units.

The caller resolves the original carrier, takes a rational surface's natural
expanded-knot domain rather than an adapter hint. This review originally accepted
an erroneous division by units_mm. Runtime review corrected that finding against
the existing source contract: snapshot coordinates are already millimetres;
units_mm is metadata. Root removed the division and added a metadata-only
invariance case. The caller constructs the new SourceCache/PatchSession
directly. Multiple tolerances are fresh queries of that retained session,
followed by fresh actual triangle artifacts; no old mesher supplies geometry.
Result guards refuse empty triangles and unmeshed demotions. Exact parameters
and actual XYZ are written with bounded integer/index conversions. The source
digest, carrier and new patch-format/policy fields distinguish the operation
from whole-B-rep output.

The output owner uses a unique sibling temporary directory and no-replace
rename only after artifacts close and the session closes. Its destructor
removes only that temporary tree. Existing files, directories and dangling
symlinks are refused rather than replaced. This is atomic publication, not a
claim of crash-durable storage. Source decode, compile, session, query, output
and close scopes stay distinct; process exit is measured by the external caller.

The independent reader decodes actual rational words and triangle indices,
checks positive chart cells and physical noncollapse, exact unit-rectangle
coverage/incidence, shared point usage and query/backend history. Root resolved
the initial sampled-only polynomial limitation in the actual reader: for
`S=(u,v,u²)`, every point of a facet is bounded by squared triangle-u width
divided by 4 plus maximum actual nodal departure. The reader compares each
exact squared nodal departure with the square of the remaining allowance.
For the C0 piecewise affine source it first excludes triangles crossing the
original knot, then uses the whole-facet maximum nodal departure bound. These
checks follow directly from convex interpolation and the variance bound, and
were reread in the actual code after the change.

The weighted rational source retains explicitly labelled seven-point
falsification samples; those samples alone are not a whole-cell theorem.
Its production full-cell bound is attacked independently in the owning
numerical unit. This scoped caller review does not infer whole-cell truth
from successful images, sampled values or backend parity.
