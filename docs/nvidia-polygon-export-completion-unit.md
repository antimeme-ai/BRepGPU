# NVIDIA polygon export: conservative plane support ranges

This reference-only unit addresses the two fresh strict exporter refusals. It
changes no owned mesher/CAD and does not reinterpret previous103/107 attempts
as complete. The existing exporter deliberately supports triangles and exact
axis-extruded rectangles; it rejects the SDK's residual crossed large rings.
That gate is correct for those actual inputs, but does not establish that the
reference cannot produce valid triangles under its documented usage.

## Actual reference basis

Pinned NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05:
`source/SM_API/inc/SmApiTessellationParams.h` documents standard curve/surface
angles25°/25° and zero as disabled. `SmApiBrep.cpp::SmApiTessellate` forwards the
requested chord and both angles to `SmBrep::ConvertToPolyBrep` without a hidden
triangle-only option. `SmBrep.cpp::ConvertToPolyBrep` constructs the shared
curve/surface tessellation drivers from those controls, copies input topology,
and retains original-face ancestry. All enabled constraints contribute. Our
main currently explicitly changes both documented angular defaults to zero.

`SmPoly.cpp::TriangulatePlanar` permits complex faces/holes but excludes
self-intersection; `TriangulateNonPlanar` explicitly excludes spiral-like shapes
and skips multiple loops. They are not justified repair tools for the observed
crossed rings. `SmuTessellate.cpp::GetMeshData` emits float positions and arbitrary
outer polygon counts and ignores inner loops, so merely switching to that USD
export does not solve complete triangle/domain preservation. Float transport
is not rejected categorically; its actual role/precision must be justified if
later selected. No float adapter or SDK repair is currently selected.

Official operation notes describe angular control as boundary tangent versus
surface-control-polygon turning; it is not a post-hoc output dihedral cap. Their
known15° workaround concerns inside-out fillet-adjacent cap/cone triangles,
not the present ring cause. Do not search/tune angles from observed watch IDs.
The first control is exactly the documented25°/25° defaults plus unchanged
requested absolute .005mm chord, maximum edge/aspect disabled and strict
allowPartial=false.

## Finite sequence and ownership

1. Parent owns generic finite nonnegative curve/surface-angle CLI controls and
   policy metadata in main.cpp, preserving explicit legacy0/0 selection. Parent
   supplies the isolated reference build; this owner does not edit main/CMake.
2. Retain actual zero-angle failure logs as RED. On the SAME two retained original
   definitions, execute the single predeclared25/25 control under heal0 and heal1
   (four small operations), unchanged official conversion/import, chord, raw
   export and source data. No whole107 run yet. Record actual params, SDK failures,
   face inventory, residual ring word and original geometry pins.
3. If these yield valid existing triangular/rectangle output, the fix is correct
   reference invocation, not wider geometry admission. Require every original
   imported face represented, finite actual XYZ/normals, distinct physical
   triangle keys, unchanged original face labels, and existing physical shell/
   source-Euler reader on each produced definition. Retain genuine failure
   outcomes if the SDK or topology still fails. Do not claim universal .005mm
   distance certification from this finite check.
4. If25/25 still yields nontriangles, inspect their actual loop count/ordering,
   planarity, original-face association, normal mapping and projected crossings
   before choosing any export capability. No raw_mesh.cpp body change is
   authorized merely because the old gate rejects. Any new generic admission
   needs literal geometric preconditions, official SDK triangulation, retained
   parent/corner ancestry, direct authored model oracle and separate plan review.
   No owned mesher triangles, fans, reordered rings, dropped faces or tolerated
   self-crossings.
5. If an exporter change becomes necessary, this owner owns raw_mesh.{cpp,hpp}
   and the existing literal polygon fixture/Python driver. Preserve whole input
   preflight before mutation, discard-on-SDK-failure ownership, exact original
   XYZ/physical identities, per-parent corner normals and user-field restoration.
   The existing directed-boundary/diagonal oracle tests the actual claimed
   transport; extend it only for a selected new profile. A settings-only repair
   does not justify artificial raw fixture changes.
6. Peer/root source review closes the selected body/invocation. Parent coordinates
   import-refusal diagnosis and any later complete reference attempt. No broad
   certification campaign or new dependency is selected.

Build/test/actual operations run only on fleet in an isolated reference tree.
Current branch is fix/nvidia-watch-reference. Operational source IDs, original
CAD and raw outputs stay ignored; tracked notes report aggregate outcomes and
exact reference functions/revision. No product body changes at this checkpoint.

## Observed control and next diagnostic boundary

All four25/25 controls still refuse at the unchanged exporter. A reused read-only
SDK inspector (only its angular parameters changed) finds the SAME residual
ring word as0/0: two46-edge, one55-edge, two41-edge and two7-edge rings across
the four operations. Every ring is a single owned closed CCW-linked loop with
distinct physical vertices and radial-reversed endpoints. The existing exact
binary64 projection predicates still find crossings in every ring. This
falsifies angular defaults alone as the repair; it does not authorize a crossed
polygon triangulation.

Before selecting a different SDK construction algorithm, add only an ignored
source diagnostic that imports each of the two retained heal0 USD inputs and
samples its original SmBrep BEFORE tessellation. Read actual face/loop/edgeuse
order. At17 normalized source-use parameters, call NormalizedEvaluate for3D and
EXISTING UV with bOKToMakeUVTrimCurve=false, then EvaluatePoint on the same face
surface. Record missingUV/status, Ce-versus-S(UV) residual and endpoint/link gaps;
do not silently create/project trims. Read-only finite residual observations
are diagnosis, not certification. Compare the3D directed sample words (both
affine orientations considered) to the SAME original BGsource edges using the
existing OriginalSourceOracle. A failed affine match may be a clock change and
requires inspecting source coefficients; do not declare bad3D geometry from it.
No new production checker, inverse solver or geometry dependency is selected.

If source3D/UV correspondence is sound, the documented public
SmBrep::ConvertToPolyBrep bAdvancingFront=true path is an existing SDK alternative
worth a separately reviewed control: it uses EquallySpacedPoints plus
TriangulateFaceAF, whereas SmApiTessellate hardcodes the bisection/TriangulateFace
path. It retains per-face failures/reports. It is not selected before the source
check. If source representation is suspect, the existing guarded RAW pcurve
preparer can be examined as a separate control, stopping before any chart split
or query edits. Any resulting copy must preserve original3D/source identity and
report its costs; no assumption either route fixes these inputs is made.

## Earlier divergence: finite plane domain, before tessellation

The bounded source trace now establishes a more specific fault. Both retained
inputs contain two planar faces whose imported finite bilinear patches omit
parts of their own imported3D boundaries. The sampled out-of-box distances are
about2.43mm and2.33mm; residuals to the physical support planes are below1e-14mm.
SDK lazy NormalizedEvaluate(parameterSpace=true,bOKToMakeUVTrimCurve=true)
returns success at every sampled use, yet Ce−S(UV) reaches2.44mm/2.33mm on those
faces. Other planes remain at rounding-size residuals. These are finite
observations, not whole-curve certificates, but they directly falsify the
finite-domain premise before polygon export.

Pinned OcctStagingDriver.cpp::ComputePlaneLoopDomainFrom3dBoundary uses24
intervals over each WHOLE3D edge. DriveFace unconditionally replaces the plane
face range with their projected sample box; it deliberately ignores source
pcurve ranges on this branch. The actual imported3D edges still agree with the
original source sample words to rounding. Four analytic circle edges change
clock during rational conversion: comparison to their original circle images
and oriented angular extents, rather than affine clocks, gives residuals below
7e-16mm. Missing pre-tessellation UV caches are expected: pinned
SmuConvert.cpp sets bCreateUVTrimCurves=false and documents ad-hoc generation.

Consequently an advancing-front control is not yet selected: it would consume
a demonstrably deficient carrier/UV representation. Copying missing OCCT
pcurves alone cannot override the explicit plane-domain branch. The smallest
candidate is an external interchange control that supplies a conservative
original boundary envelope for these finite Plane ranges before import,
without modifying their support planes,3D curves, topology, ancestry, quality
or raw triangle exporter. Its exact construction and allowed API must receive
review before execution. No SDK fork, geometry replacement or denser-sampling
claim is selected here.

## Selected private control: conservative staged Plane ranges

Parent selected this control after the source trace. Converter/SDK binaries and
raw_mesh remain unchanged. The private driver consumes the SAME retained USD
files; no original CAD write or SDK fork is involved. Run only the two existing
heal0 inputs with .005mm chord,25°/25°, maximum edge/aspect zero and strict
partial=false. Run their heal1 counterparts only after both heal0 operations
pass the unchanged geometry/source gates.

1. With the package's existing usdpy OpenUSD bindings, read each BrepArray's
   face:surfaceType, face:range and brep:surface:plane:{origin,axis,refDirection}.
   Require a single complete one-definition BrepArray, finite values, exactly
   two ranges per face and one plane frame per Plane token in traversal order.
   Write those literal plane frames to a private TSV. Unsupported/composed or
   animated inputs refuse rather than inventing a different schema.
2. An ignored strict C++ driver imports the unmodified USD with heal=false and
   checks every actual imported edge exactly once. All curve families supported
   here must derive from SmBSplineCurve (SmLine and rationalized analytic arcs
   do); otherwise refuse. Require3D, finite natural/edge intervals with the edge
   interval contained in the natural interval, a valid finite knot word, and
   finite positive homogeneous weights. GetControlPoint(SM_CP_HOMOGENEOUS_RATIONAL)
   exports the actual untouched binary64 homogeneous controls, including weights
   (no rounded Euclidean division). Require full edge/control inventory.
3. The driver builds each frame exactly as the pinned import branch:
   X=refDirection; Y=axis cross X; SmAxis2Placement::SetCanonical(origin,X,Y).
   Emit its represented canonical origin/X/Y words. This uses the SAME SDK
   normalization/orthogonalization semantics rather than assuming exact unit
   axes or repeating a subtly different inverse. No source object is changed.
4. Python Fraction treats every emitted double as exact, divides Hxyz by positive
   W exactly, and builds one enclosing definition-wide3D control AABB. Positive
   rational B-spline basis functions form a convex combination throughout each
   admitted natural interval, so this box contains every supported boundary
   regardless of use direction or rational clock. Project all8 AABB corners into
   each SAME represented plane basis using its exact2x2 Gram dual (det>0), then
   union those bounds with the old face range. This avoids any face/edge matching
   assumption. The projected bounds are exact rational arithmetic; convert each
   endpoint to binary64 outward using comparison plus nextafter, refusing
   nonfinite/collapsed results. This changes the finite support envelope only.
5. Export an unchanged copy of the original layer to a NEW USD path, set ONLY
   face:range for Plane entries, and save. Before/after checks compare complete
   prim/attribute/relationship/metadata inventory and typed values; all attributes
   other than this one must compare equal, and non-Plane range entries must be
   identical. Old range inclusion is checked. Plane origins/axes, original3D
   controls/knots, loops/uses, IDs and labels remain untouched. No JSON manifest,
   source-ID branch, pcurve construction or guessed polygon triangulation.
6. Reimport the corrected USD using the requested healing policy. Reuse the
   official SmApiTessellate and unchanged extract() implementation, retain strict
   import/member statuses, per-face failure vectors and tessellation report,
   and require every source face represented by finite nondegenerate triangles
   with original ancestry. The existing physical/source-Euler reader checks any
   produced meshes. The SAME bounded original3D/lazyUV observations must show
   the previous millimetre-scale carrier discrepancy removed; this is diagnosis
   plus ordinary reference transport admission, not a new5µm certificate.
7. Preserve all earlier zero/default-angle failures and exact control parameters,
   widened ranges, preparation/import/tessellation/export times and outcomes.
   Failure still emits no successful mesh claim. Root reviews the private source
   and actual result before selecting any supported public integration. No AF,
   RAW pcurve-only run, whole-watch attempt or new dependency is in this control.

### Public integration after the private control

Both heal0 controls now pass unchanged raw extraction plus the existing closed
physical-shell/source-Euler reader. Their old millimetre-scale Ce/S discrepancy
is removed by the SAME diagnostic. Parent selected a capture-only native helper
native/om10_nvidia/plane_envelope.cpp, and an explicit external USD preparation
script scripts/prepare_nvidia_plane_envelope.py. The helper reads INPUT.usdc,
OUTPUT_PREFIX and PLANE_FRAMES.tsv; it writes homogeneous controls, inventory,
final represented SmPlane frames and native stage times. The script accepts
--input-usd, --output-directory NEWDIR and --helper; it emits corrected.usdc,
range/policy reports and exact preparation stages. Parent owns producer
--usd-definition and CMake. The original converter route is unchanged.

Capture the final frame after SmPlane::CreateCanonical, which canonicalizes the
placement again, and require UVScale1. Include original imported vertices in
the definition hull as well as curve controls. Exact Gram coordinates describe
the represented affine frame; they do not certify all rounded SDK projection
routines. Reprojection and output gates still decide success. Input must remain
standalone/unanimated with no relative external assets, references or payloads.

One authored SDK cylinder with literal radius2, height3 and translated origin
provides two Plane caps bounded by positive rational circles. Its test
intentionally reduces only their face ranges to±.25 (the literal±2 cardinal
support is then excluded), applies the generic helper, checks restored support
against literal physical circle cardinal points in the SDK's translated cap chart,
   and unchanged source attributes,
then uses the existing public strict producer/physical-shell reader and original
cylinder equations. No watch IDs or sampled hull oracle are used in that test.
