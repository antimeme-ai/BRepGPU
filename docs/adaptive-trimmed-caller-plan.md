# Complete selected-face caller and original-output oracle

Status: finite PLAN for peer review before product edits. Root owns this caller,
wire/reader, build integration and complete-operation report. Construction,
surface runtime and actual-facet numerics remain whole companion ownership units.
No new dependency and no production Python geometry callback.

## Purpose and grounding

The operation takes an ORIGINAL SourceStore definition and face, constructs the
complete shared boundary, then emits every selected face region as actual XYZ
triangles. It is checkpoint3, not an assembly NativeMesh publisher. The process
must fail if a required use/domain/source operation is unsupported or exhausted;
it never publishes a successful subset.

Pinned NVIDIA c9c5979f04cea8c38213004d4f04df0dcfb40d05,
SmTess.cpp::Phase2CreateFacePolygons12023–12125 and
AddQuadBoundaries5334–5430/5610–5840 establish the interior-packet/remaining-band
dataflow. OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42,
BRepMesh_Triangulator86–270 establishes protected-wire consumption. Neither
reference supplies our original-output accuracy oracle. That is selected below
from exact oriented-chain algebra and independently authored original functions.
The numerical owner's plan supplies the primary-paper assumptions behind actual
SAME-L bounds. Existing adaptive patch/curve callers establish owned source
decode, explicit CPU/P100 placement and no-replace output publication.

## Product files and native operation

1. Add `native/mesher/adaptive_trimmed_main.cpp` and executable
   `brepgpu-adaptive-trimmed`. Keep source decode and resource ownership native.
   Options: --source, --definition, --face, --output, repeated --positional-mm,
   --node-source-mm, --threads; optional --backend p100 --cuda-module --device;
   --memory-mib --max-nodes --max-depth. Check positive finite allowances,
   checked capacity products, original definition/face relationship and depth64.
2. Extract the existing BGACRV01 writer and original definition curve-request
   gathering from `adaptive_curves_main.cpp` into owned caller-I/O helpers;
   preserve its existing wire/behavior. Both callers use the same writer.
   This is mechanical reuse, not a second shared-boundary implementation.
3. Compile original spatial curves and every incident original pcurve for THIS
   definition, including wires and original endpoint spans. Run the real
   AdaptiveBoundarySession before face construction. Its complete snapshot is
   retained until the face result and both query artifacts have closed.
4. Prepare the selected face's surface cache through prepare_trim_source_cache.
   The envelope is fixed from ORIGINAL source data before quality queries:
   analytic carrier declared finite domain; rational natural source knot domain.
   No fine output chain chooses a narrower envelope for a coarser query. A chosen
   chart point outside that envelope is a named source/profile refusal. The
   runtime/source plan must close this explicit caller premise before RED.
5. Run FaceTrimSession through its real begin/step/result protocol. Boundary
   requests return to the shared-definition authority; never privately replace
   physical Q or omit a requesting facet. Surface numerical caches may survive
   repeated qualities. Every query starts fresh construction/physical forest
   from the same base policy. Release results before the next query, close all
   sessions before publication. CPU-only never loads a CUDA module.
6. Add the owned targets/tests to native/mesher/CMakeLists.txt only when companion
   headers and ownership are reconciled. Root owns these CMake edits to avoid
   concurrent rewrites. CUDA kernels are linked through the existing module;
   numerical code does not create another ticket/allocation subsystem.

## External artifact

Each query writes the existing query-N.bgacrv PLUS an owned little-endian
BGATRM01 face artifact. No pointers or mutable pool offsets stand in for source
identity. A slot may reference another table retained in this SAME artifact.
Complete literal field order is recorded here against accepted headers before
writer/reader RED; the required semantic rows are:

* Header: wire1, numerical profile, original source digest, original
  DefinitionID/FaceID/CarrierID and physical face orientation; source/query/
  boundary/chart/cover/connectivity revisions; domain policy and row counts.
* Site table: generator (boundary/grid/anchor), exact original UV and chart XY,
  actual three binary64 XYZ, constructed revision; boundary sites reference an
  actual BGACRV use/site/node row; cache/anchor sites retain original root and
  exact UV semantic point key, independent of allocation order. Seam occurrences
  remain distinct even when their physical key and XYZ agree.
* Directed protected cycle table: original loop or packet-rim provenance,
  normalization sign and complete ordered atomic site indices. Preserve the
  source use and logical interval associations in source cycle edges.
* Triangle table: three site indices, packet/component semantic generator and
  revision, facet ordinal, actual bound/domain policy and source association.
  UV-positive order stays explicit; physical face orientation is separate.
* Metric: actual fixed binary64 factor and exact common Gram values defining
  the connectivity predicate. The independent coverage oracle uses original
  chart coordinates, not metric coordinates.

Canonical rationals use BGAPCH01/BGACRV01 sign and reduced uint32 limb format.
All counts/index ranges/optional tags are checked. No trailing or partial
artifact is accepted. Bounds are emitted as exact rational uppers when native
acceptance used exact tau; rounded values must not contradict that acceptance.
Wire details cannot require exposing host placeholder controls or downloading
device child nets.

## One direct original-output reader

Add tests/native_adaptive_trimmed_caller.py, reusing the actual BGACRV reader
and independently authored original SourceStore fixtures. It reads the REAL
caller artifacts, never a test-only triangle dump or native bound descriptor.
Use Python integer/Fraction only in this external oracle.

Topology/coverage has ONE direct mechanism: exact oriented atomic edge chains.
Check every actual triangle has positive original-chart area and distinct sites;
every interior directed edge has exactly one reversed mate; surviving directed
edges equal the complete normalized ORIGINAL chosen source boundary. At packet/
band interfaces the common atomic edges must cancel; a long edge against two
short edges fails incidence instead of being silently geometrically combined.
All protected/active anchors must appear, including collinear source/rim sites.
Original source use/site references and seam physical identities must agree with
the complete boundary artifact. Unknown nonincident equal chart sites refuse.

The coverage reasoning is an OWNED derivation: winding of the summed triangle
boundaries equals the boundary-chain winding away from edges. Positive triangle
orientation makes its multiplicity nonnegative, so equality with the selected
domain's winding (one inside, zero outside) rules out omitted holes, overlaps
and omitted regions. Atomic incidence additionally rejects hanging edges.
Do not add a second all-pairs triangle checker or another coverage certifier.
Original source loops supply the expected chain; artifact-provided cycles alone
are not an independent expected value.

Geometry oracle uses ORIGINAL authored functions and SAME actual emitted L:

* Exact affine plane: max original vertex residual, exactly squared comparison.
* Quadratic/coupled saddle: original POWER coefficients expanded on each actual
  triangle; exact triangular Bernstein residuals provide a whole-facet upper.
* Positive rational p,q<=3: independent POWER numerator/denominator expression,
  triangle polynomial substitution, positive denominator lower and exact
  residual coefficient upper. No production restricted net supplies truth.
* C0 source: oracle splits by its authored ORIGINAL break coordinates; each
  piece keeps original triangle barycentrics and SAME L. It never uses selected
  numerical leaf boxes as the original function partition.
* Regular cylinder/cone/sphere/torus: independent source-frame interpolation
  bound with derivative suprema and actual vertex departure. Analytic sampling
  can falsify but cannot prove whole-facet acceptance.

Fixture construction must make original Ce/S/vertex compatibility exact or
explicitly budgeted; source-node delta remains separate from positional bound.
CPU/P100 equality checks placement determinism, not source accuracy.

## Finite RED, implementation and review order

1. Before product implementation, register the real caller test and a small
   authored plane with a tiny interior square hole wholly inside one initial
   leaf. The oracle must demand the complete boundary/face artifact and hole
   chain. Record the actual absent-caller/interface RED on the fleet.
2. Implement writer/extraction + complete caller against accepted shared headers;
   integrate owner RED/GREEN results without substituting test-only mocks.
3. Extend the SAME reader to the preselected families: aligned many holes,
   multiple outer regions/packet islands, hanging C0 spans, reversed loops,
   actual curved facets requiring a band anchor, forced packet grid target,
   long cross-leaf facets and a reconstructed periodic seam. Include nonlinear
   exact line-image authoritative identity and unknown-map refusal.
4. Failure cases: wrong face/definition, original node mismatch, malformed
   input, insufficient memory/depth, cancelled/stale native work in the owning
   service and no-replace output. A rejected process must not publish artifacts.
5. Run affected CPU1/CPU4/both-P100 actual callers; compare fresh fine/coarse/
   fine/repeat queries and an independently fresh coarse process. Scoped ASan/
   UBSan/TSan and device protocol/memory checks target NEW ownership paths.
   Reuse completed curve/surface direct checks only where extraction changes
   their behavior. Stop after the relevant claims are answered.
6. Peers review actual source, not only declarations. Integrate valid findings,
   record rejected findings with their reasons, rerun affected direct checks.
   Publish coherent implementation and result commits on the feature branch.

## Complete-operation performance

Record external process wall and native decode/compile/session/device setup,
complete boundary and face construction, output and close. Separate classification,
packets, CDT, virtual clipping, ordinary/direct source bounds, precision and
construction horizons where the native owner measures them. Report work counts,
candidate visits, active sites/facets, packet/band fraction, transfers and memory.
Overlapping worker/device scopes are not summed into process wall. Oracle time
is separate. Retain unsupported/failed/censored observations. GPU advantage is
a measured hypothesis, not an acceptance requirement. No watch retry or render
is part of this checkpoint; complete component/assembly publication follows it.
