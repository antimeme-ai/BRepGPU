# Meshing design runtime review

2026-10-15. Reviewed `docs/MESHING_DESIGN.md` against actual BodySnapshot,
Curve/UVMap, BodySampler and Engine ownership contracts and pinned OCCT meshing
source study. Source-only; no implementation or execution.

Accepted CPU placement, exact world construction, typed original identity and
bounded immutable output. The max-quarter-segment power-of-two cap1..1024
bounds vertices/triangles/corners before geometry allocation. The tol/2 exact
curve budget plus measured L1 vertex-rounding displacement supplies an honest
total exported geometric bound; failures remain complete-request failures.
No native ABI, GPU lifetime or library change is needed for this operation.

Physical edge samples, closed endpoint identity, original vertex reuse and
per-coedge lifted atlas occurrences are distinct from triangle corners' local
chart UVs. The seam retains both0/4 occurrences; chart joins do not invent
source features. Reading original exact records avoids the existing float-only
curve/UV evaluation paths. The minimal plate fan/side template closes the
complete annular cap without extra edge knots or a generic triangulator.

The independent qualifier's positive directed-edge chains and geometric strip
coordinates directly address coverage and orientation; its actual arc inequality
uses a different calculation from the product refinement bound. This avoids
using Euler/area alone or native/CPU parity as geometry truth. Full face/corner/
physical-edge mappings and published rounded positions belong to qualification.

One clarification sent to root: actual rounded triangle signs and shared
indices alone do not establish global embedding. For these canonical extrusions,
independently check the actual rounded ring's convex/order and nesting/separation
inside the actual outer rectangle, then apply the complete positive-chain/strip
coverage argument to those rounded domains. The check is bounded and geometric;
no general self-intersection engine is necessary. The design already calls for
separate rounded-shell qualification; this makes that obligation concrete.

Whole CPU construction/traversal/rounded consumption and separate preparation,
qualification/export clocks are appropriate actual caller costs. The stated
absence of transfers/CUDA/speedup is correct for the selected operation. Exact
surface approximation, retained hole topology and chordal-domain coverage are
carefully distinguished from every triangle point satisfying the analytic trim.
External alpha qualification remains incomplete, explicitly.

Final disposition: inspected the integrated rounded-domain requirement. The
design now explicitly qualifies actual rounded ring convexity/order/nesting,
actual outer-rectangle separation and complete rounded-domain positive-chain/
strip coverage. The selected axis orientation for bottom/inner faces is also
explicit. Runtime/data design is accepted with no remaining finding; the plan
and implementation must carry these concrete obligations forward.
