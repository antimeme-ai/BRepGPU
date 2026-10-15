# NCSX goal-object rendering, first pass

The operator explicitly requested a first NCSX rendering now, a README goal
object, and publication. Use actual acquired NCSX Fourier coil centerlines and
a separately sourced NCSX plasma boundary, without installing their reference
library. This is a reproducible visual scene over existing native patch
operations, not a new general import/sweep/Boolean kernel or as-built CAD model.

## Design

Retain the pinned source datasets and their license/attribution. Decode three
base modular-coil centerlines (all31rows/order30), then apply the inspected
three-period and stellarator-symmetry transforms to obtain18coils. Coil pack
width/depth are declared cosmetic choices; no winding-form or conductor-detail
accuracy is implied. Sample closed offset rails and represent each pack by
four degree(3,1) Catmull/Hermite surface strips per source segment, n64segments.
The section frame uses the source toroidal vector(-y,x,0), projected off the
normalized Fourier tangent, then normalized; its cross product with the tangent
supplies the other section direction. Reject zero tangent, radial-axis and
near-zero projected-frame vectors rather than inventing a frame. This analytic
periodic frame avoids parallel-transport closure machinery. Reuse rail
samples/derivatives at boundaries; normalize through a common
power-of-two scale4 identity-origin frame and check every actual control fits
the native local envelope. All weights are1. Preserve original controls in hex.

Decode the separate136-mode/nfp3 FOCUS plasma boundary at phase mθ−3nφ. It is
not the vacuum vessel and the data pairing is not an equilibrium validation.
A periodic bicubic interpolating surface from a36×24 parameter grid represents
it for display. It is retained as a separately labeled component. Do not invent
ports, support castings, cryostat or a vessel from these plasma coefficients.

Evaluate native first jets on P100 using the existing Engine. Verify actual
query/count/order/descriptor/parameter binding, success/finite output and exact
binary64-control polynomial position/first-derivative grades2^-40/2^-30 in the
normalized local control frame. Scientific/source-fit residuals are reported
separately in original coordinate units after applying scale4. The raw datasets
do not declare calibrated length units; do not add a meter legend. Native returned
first jets supply normals. Check periodic patch seams in positions. Adjacent rectangular strips intentionally
have sharp normal discontinuities. Source
Fourier-to-interpolant residuals are a separate sampled fitting diagnostic,
never a global approximation bound or an as-built measurement.

Reuse the owned orthographic triangle depth-buffer approach with a generic
per-patch grid layout, native-normal lighting and copper/teal materials. No new
render library. Render a wide1800×1200 first comparison view; record camera,
projection, dimensions, light, palette, coil width/depth, fitting grid, sample
grid and source hashes in native TSV records. The output PNG/SVG is a scene
baseline: later geometry/render changes are named instead of hidden in comparisons.

## Plan and review

1. Read/inspect primary data readers, symmetry and PPPL visual references; save
   reading scope/source inventory and intact acquisitions. Review this complete
   bounded design/plan before implementation.
2. Own examples/render_ncsx.py plus a small data directory with originals,
   attribution and upstream license. Keep unneeded reference code quarantined.
   This is a reversible rendering caller; no new kernel behavior/test suite.
3. Execute all geometry construction/evaluation/qualification/rendering on the
   fleet using the unchanged native core/plugin. Local static asset conversion
   and inspection are allowed. Reuse already-qualified saved native jets for
   display-only iterations; retain evaluation versus renderer source hashes.
4. Inspect full actual image for recognizable coil geometry, occlusion, framing
   and clear goal/current-state captions. Source/artifact review must check
   18coils, data transforms, periodic seams and honest provenance/limitations.
5. Replace README hero with NCSX as the continuing goal object. Link exact
   source/scene/reproduction records and explain what the current render uses.
   Preserve previous synthetic studies as history. Push substantive commits,
   merge via PR without squash, retain branch and sync owned fleet checkout.

Native patch arithmetic qualification establishes that evaluated surface data,
not agreement with as-built engineering geometry or community alpha conformance.
