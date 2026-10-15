# CutResult mesher design: independent oracle review

2026-10-15. Reviewed docs/CUT_MESH_DESIGN.md against actual Boolean, canonical
meshing and independent qualifier sources and the new source-first oracle study.

**Accepted.** No blocking finding. The design admits the actual factory CutResult
and retains original history and source carrier frames rather than pretending
that the output is a canonical plate. It explicitly preserves actual radius1/2,
raw wall V[1/4,3/4], shared closed rim anchors, original seam lifts and face senses.
Complete positive triangle chains in physical planar/chord coordinates address
whole-domain coverage; edge incidence, cyclic vertex links and rounded embedding
attack separate topology/rounding faults. Exact arc inequalities and actual float
displacement support the stated complete bounded approximation rather than a
centroid/material claim.

Implementation attention, already required by this design: derive world radius
from physical result curves/frame but evaluate wall corners with the original
cutter patch Frame. Assert requested-result object identity directly in factory
invocation tests. A forged same-geometry parent substitution cannot be discovered
from implicit body-scoped local IDs alone and is outside the declared trusted
factory input boundary. No added untrusted-deserialization validation is needed.

Accept the bounded four-case CPU-only measurement and external non-green limits.
No product code or local geometry execution. Plan review is the next boundary.
