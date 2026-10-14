# Body sampling design: independent oracle review

Reviewed [BODY_SAMPLING_DESIGN.md](../docs/BODY_SAMPLING_DESIGN.md) against the
canonical analytic family, existing first-jet/frame API and
[oracle study](2026-10-14-body-oracle-study.md). Source/design review only;
no product edits or execution. The corrected design is accepted for a plan.

**R-B1 — required snapshot admission clarification, resolved.** Prepared grids
cache trim labels, dense slots and gather/scatter mappings. Different bodies
reuse those slots/face IDs, so accepting another body's prepared grid could
silently evaluate different carriers with stale labels or wrong frame/source.
The initial design retained candidate body/revision but did not explicitly
require sampler/request binding before native work. Root added exact retained
snapshot binding, including frame/chart ordering, rejection of a different
snapshot object even with matching IDs/counts, and foreign-body/frame cases.
This private ownership check does not make object addresses public entity IDs.

The exact integer predicate correctly classifies original binary64 plane UVs;
no rounded XYZ or epsilon defines membership. The plate hole cannot have exact
binary UV boundary samples at the chosen width/radius ratio 3. Root was informed;
the study makes that consequence explicit. Disk cardinals, rounded arc UVs,
chart joins and same-face seam coedges are distinct cases. Wall top/bottom
boundary variables mean v=0/1; internal chart endpoints are not body boundaries.

Whole line/circle/UV correspondence must be checked through rational identities
and actual table roles, while floating probes qualify their implementation.
Circle spans remain one edge, and seam occurrences remain distinct. Expected
counts and opposed incidence are coherent for the proposed representation;
plate hole loops must be accounted for when checking Euler characteristic.

Known outward axes/radial normals independently check face orientation, including
negative bore-wall orientation. Positive power-of-two frames preserve normals
and trim coordinates. Frame/first-jet conversion fits the existing envelope;
declared numerical error targets are finite-corpus checks rather than universal
error bounds. Snapshot records remain recursively immutable; existing Patch
controls are tuples, and temporary native arrays/descriptors belong to sampler
calls. Rejected/invalid
rows, compact native stride, scatter/source identity and cleanup need direct
tests, without claiming general imported B-rep or mesh correctness.

No remaining numerical, trim, topology-identity, ownership or minimal-campaign
blocker was found after R-B1's correction. Actual builder/sampler code and fleet
qualification remain future evidence; this acceptance does not certify them.
