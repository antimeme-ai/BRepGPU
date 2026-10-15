# Independent direct tests for the complete OM10 delivery

2026-10-15. Implementation subplan for review before code. Purpose: expose wrong
source geometry, missing domains, wrong assembly placement or native visibility
and timing scope with direct answers. This is one bounded qualification lane,
not an alpha certificate, coverage/mutation campaign or second copy of the
product algorithms.

## Reviewed boundaries and lane ownership

The source lane owns `brepgpu.imported_source.load_source(path)` and
`brepgpu.imported_meshing.mesh_imported(source,quality)`, their immutable source/
mesh records and exact source snapshot schema. It owns parser/reader input
validation and source adapter implementation tests. This lane supplies the
independent geometry answers consumed by direct tests and actual callers.

The native lane owns `brepgpu-om10-native`, BGMESH01/BGVIEW01 input and actual
RGBA/depth/primitiveID/instanceID/source-jet output. Root owns assembly/BOM,
explosion and caller, including world placement and callout tests. This lane
reviews root's source-independent placement expectations; it does not duplicate
the assembly implementation under another API.

Proposed files: `python/brepgpu/imported_oracles.py` for independent source/mesh
geometry; `tests/test_imported_geometry.py` for the direct family;
`tests/test_om10_native.py` for scalar renderer/evaluator fixtures and native
ownership failures. Tiny scalar expected-pixel helpers stay with these tests,
unless actual caller reuse warrants an owned `om10_display_oracles` module.
No separate report-schema or qualification-framework package is needed.

Final source/mesh field names follow the source owner's single published schema.
Tests must bind those exact reviewed records before initial fleet red; do not
create a parallel opaque dictionary model. All internals are typed owned values
or explicit binary, never JSON persistence.

## Direct source and trimmed-domain family

Author a small set of explicit geometry records with independently known answers,
not a copied extrusion constructor or OCCT mesher output:

1. An oriented plane with a nonconvex outer boundary, two holes and collinear
   boundary samples. Expected directed boundary words and material domains are
   literal source values. Every physical boundary sample occurs in the mesh;
   positive triangle chains cover all of the domain and none of the holes.
2. A cylindrical periodic face whose seam has two distinct UV occurrences and
   shared physical keys. Opposite face orientation changes the normal, not the
   original carrier or physical edge identity. Include source edge reversal.
3. A sphere-pole or cone-apex face with a known physical vertex quotient. Accept
   valid distinct UV corners at the pole; reject zero-area physical triangles
   and unsupported singular normals.
4. A multispan rational surface with nonunit domains/interior repeated knot;
   original Cox–de Boor scalar values and first derivatives establish span
   extraction at endpoints, either side of the knot, and an interior point.
   A degree-five spatial curve has literal source controls and exact rational
   parameter answers; it must not be silently reduced to degree three.

For source conversion and recovered trims, inspect actual source→output
correspondence. Independently evaluate original curves and original surface at
the corner's UV, including the canonical shared-edge XYZ discrepancy. Compare
the actual emitted quantities with the selected quality components. Whole-domain
bounds use the chosen analytic/control-hull bound where supported; finite
probe maxima remain measured residuals and retain that name.

Geometry/mesh qualifier reads the original ordered loops and saved actual
corners. Exact rational signs of saved binary64 UV coordinates plus integer
boundary-chain equality check polygonal coverage. Physical incidence, vertex
links, source IDs and source evaluation check different fault classes. It does
not invoke the production triangulator, bridge selection or native CSR builder.

Include direct malformed actual-output cases which must raise an error:
missing/duplicated/reversed face triangle, omitted hole, changed original edge
parameter, seam UV collapse and unmatched shared physical edge. Each is attached
to a corresponding production contract test rather than a separate checker
test suite or general mutation harness.

## Native source evaluator and scalar pixel family

Construct explicit BGMESH01 scenes and192-byte requests in the tests using the
single accepted schema. Independent formulas use original analytic axes/UV and
original-knot Cox–de Boor, not the production evaluator or extracted patch helper.
Check finite position/first-jet/normal values for plane/cylinder/cone/sphere/torus
and a nontrivial rational span, including indirect axes and limiting normals.
The same actual source-evaluation path must support a zero-frame/evaluate-only
request before whole-source OM10 rendering.

For tiny images, independently compute projection and sample predicates with
exact rational values after the declared snap. A scalar exhaustive triangle
loop is suitable here precisely because it is small and structurally different
from native CSR/tile/pixel execution. Compare actual primitive/instance IDs and
depth, then linear/sRGB output under the selected arithmetic tolerance:

- One triangle: hand-known inside/outside and exact edge-center samples.
- Two-triangle rectangle: unique shared-edge ownership, opposite winding and
  unbiased barycentric depth/normal interpolation.
- Overlap: front/back, reordered input and exact coplanar lowest-ID tie.
- Instancing: one definition in two noncommuting rigid placements; an exploded
  offset changes the expected winner while alpha0 retains assembled positions.
- Offscreen/negative coordinates, edge-on/subpixel triangles, empty background
  and supersample2 resolve, including the specified final-pixel ID policy.
- Known rotated constant normal and a varying three-normal fixture: normalize
  after barycentric interpolation; retain sharp normals across source faces.
- Invalid indices/counts/nonfinite values, unsafe projected coordinate range,
  degenerate invalid transforms, and explicit CUDA unavailable/error behavior.

CPU1, useful multicore and each promised P100 consume the same valid fixtures.
No CPU process may load/initialize CUDA. New native ownership gets appropriate
host sanitizer/device-memory checking; do not rerun unrelated historic native
families merely because the fleet is available. Failure means nonzero process
exit/no successful output, not an all-background image or CPU fallback.

## Actual source, ordering and stopping condition

1. After overall design and plan review, write direct tests and run the initial
   fleet red against the missing reviewed APIs. State which cases actually ran;
   later additions are not retroactively described as initial red.
2. Source/geometry and native owners implement their coherent units. Run affected
   greens serially where actual fleet resources or source writes conflict.
3. Independently review code and fix real findings. Unsupported or ambiguous
   source cases remain explicit failures; preserve the whole-model requirement.
4. Before any actual OM10 render, qualify every107 definitions/9,162 source faces,
   all emitted mesh domains, physical shells,166 original occurrence paths and
   actual CPU/P100 source jets. Evaluate-only output supplies these native jets.
   Root separately checks independently composed source placements and labels.
5. Render assembled and exploded only after those obligations pass. Compare
   actual saved outputs and attribute any permitted backend edge/depth-band
   differences using the agreed projected-data diagnostic. Unknown differences
   fail; synthetic exact expected winners remain exact.
6. Run the bounded matched timing comparison specified by the parent plan.
   Inspect actual images/callouts, finish source attribution/docs and stop.

Timing checks read actual native/caller rows and source/view pins. They establish
that a displayed named scope includes its work; they do not create a second
metrics certification framework. CPU1/multicore/P100 comparison uses identical
complete source, quality and output. Cold source-to-image, native stages and
reused updates remain distinct; qualification is included only where the wall
explicitly claims qualified work. A slowdown remains a valid outcome.

## Actual thin-UV construction amendment

Face165122 has adjacent binary64 Y coordinates and a shared diagonal whose
rounded midpoint cannot yield four positive UV children. The reviewed rare
fallback retains the exact dyadic midpoint in the owned UV partition and forces
exact orientation whenever a coordinate is Fraction. Native export explicitly
rounds each UV coordinate to binary64; that rounded partition is not claimed to
be exact. Original-surface endpoint displacement from this rounding is checked
separately against tolerance/16 and included in the product total error budget.

The independent reader compares native UV to `float(ownedUV)`. For the rare
changed spline coordinates it evaluates both endpoints using original-knot
rational Cox data; analytic carriers use conservative global first-derivative
norm bounds. It reports the maximum independent native UV rounding displacement
bound. Exact UV chain coverage and physical nonzero-area/incidence obligations
continue unchanged. Recovered foreign pcurve overhang may be corrected only at
original carrier global endpoints with an explicit numeric grade and world
residual, preserving raw recovered UV; original interior knots are never snapped.

## Original cylindrical trim recovery

The actual foreign UV line on face165590 misses original C(t) by up to a
measured .00661mm, although original C(t) is within a measured .000141mm of the
original cylinder. Scalar reparameterization cannot repair the foreign line.
The reviewed owned recovery therefore inverts original cylinder coordinates
from original spatial C(t) samples (and original vertices at joint endpoints),
retaining raw foreign UV/parameters with explicit recovery_kind.

Independent checks evaluate raw pcurves, then separately derive original-axis
atan2/height from retained original physical generators, preserve the nearest
same period branch, and require corrected world residual within unchanged
source_residual_mm. Only cylinder_source_inverse bypasses the former numeric
UV-close grade. Explicit JointRecovery metadata must retain both original
incident endpoint occurrences in original loop order, their physical identity
and parameters, original endpoint incidence, and canonical original-vertex UV.
Non-small raw joints require that metadata. All exact domain, physical solid
and native input/jet/placement checks continue unchanged. Foreign residuals
remain diagnostics; they are not confused with corrected source residuals.

The accepted original-source recovery now includes plane_source_inverse,
cone_source_inverse and cone_source_generator. Independent saved-frame inverse
uses exact rational Gauss-Jordan elimination, separately from the mesher's dual
construction. Plane UV must equal the exact affine source-XYZ inverse, preserving
exact collinearity. Cone inversion checks signed-radius sheet and axial/slant V.
Generator occurrences require original LINE, original active-midpoint branch,
and both endpoint residuals of the affine C(t)-S(U,V(t)) conversion; true apex
endpoints must be the original incident vertex at the original analytic apex.
Plane/cone JointRecovery kinds retain the same incidence/provenance obligations.
Only existing declared pole ChartClosure can replace ordinary joint metadata.
No source points or UV cells are omitted and physical-zero rejection remains.

Verified original cone-pole endpoints canonicalize V to the host analytic
`-R/sin(angle)`, preserving original XYZ and counting the endpoint perturbation.
An equally close raw/local inverse can lie on the wrong signed-radius side and
give a reversed normal. Independent boundary checks therefore require this V
exactly. Actual native declared pole vertices retain the same V, must emit the
limiting-normal flag, and compare against the original positive-interior normal
limit; numeric position closeness alone is insufficient.
