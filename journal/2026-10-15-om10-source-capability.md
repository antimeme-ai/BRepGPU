# Whole OM10 source capability checkpoint

2026-10-15. After reviewed design/plan and actual fleet missing-API red, the
isolated pinned OCCT8.0.1 adapter and owned immutable BGSRC001 reader are
implemented. This checkpoint delivers source extraction, not a completed mesh
or watch render.

Actual reviewed extraction retained all107 original solid definitions,
138 products,196 defined usages,9,162 faces,9,768 loops,51,742 coedges,
25,871 edges,17,129 original vertices,9,162 carrier records,77,613 spatial/UV
curve records and219 style/material records. The original source SHA-256
matches the independently retained input. Original-to-translated vertex
displacement is zero. Actual active representation units are millimetres;
the adapter derives that from the file and rejects non-millimetre/mixed contexts
until their original parameter maps are implemented, rather than making a
false generic original-UV conversion claim.

All original edge bindings are SameParameter=true; none is flagged degenerate.
Recovered pcurves need degree5. The actual source's635 spline carriers are C1
or better across every original interior knot. The isolated source adapter does
not tessellate. It disables optional shape-processing operations; intrinsic
STEP translation still reconstructs pcurves and finite singular charts.

Review closed these concrete findings:

- Missing native forward-declaration includes and transitive isolated OCCT
  runtime lookup were corrected after actual fleet compiler/loader failures.
- Original Cartesian vertex values are retained. File units are read explicitly;
  translated tolerances remain separate from source uncertainty and chosen
  geometry quality.
- Original EDGE_CURVE SameSense is composed with source occurrence/bound sense
  before choosing a seam UV branch. Silent nearest-endpoint parameter swapping
  was removed. Source parameter contradictions fail explicitly.
- Every translated face's actual wire/edge occurrence inventory is compared
  with original topology. Exact topology copies require identical carrier,
  location, range, endpoint identities and parameter/degeneracy flags; imported
  tolerance changes are named. Pcurves come from actual matched face edges.
- Five translated extra UV connectors are independently identified at original
  analytic pole vertices. They retain source-linked diagnostics and are not
  invented original EDGE_CURVE entities. Owned chart-closure meshing is a
  separately reviewed continuation, not part of this source checkpoint.
- Affine seeds do not establish a unique non-SameParameter projection branch;
  the owned mesher rejects that unproved family. Nothing in the actual supplied
  movement is omitted by that boundary.
- Frozen records reject caller-supplied mutable nested containers and foreign
  row types. The binary reader verifies lengths, versions, flags, finite values,
  references, weights, knots and ownership; no OCCT handles or JSON escape.

Independent original STEP comparison checked all vertices, analytic frames and
parameters, all original spline controls/knots/degrees, all25,871 spatial curves,
196 local usage frames and166 expanded world placements. These are direct
source checks, not CPU/GPU parity or render appearance. Reader/binary direct
tests passed on the fleet; exact current counts are retained by the parent.

Source bytes, binary snapshots and operational logs remain ignored/private.
Research, exact binary/API schema, author PDFs and restoration details remain
in the continuing import/mesh study. No foreign CAD or mesh is published as
owned product geometry. Full owned meshing and its direct qualification proceed
before any actual source-derived trophy render.

Final source API review pins full Bézier curve carriers to natural[0,1], with
active source edge/coedge intervals retained separately. An ambiguous carrier
claiming its active interval as its full domain is rejected. The actual OM10
contains no direct Bézier curve records; its extracted spline spans already
retain explicit original source domains. This correction closes an API ambiguity
without changing the supplied movement geometry.
