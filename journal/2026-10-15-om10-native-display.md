# OM10 native geometry/display lane — 2026-10-15

Read the shared Blackbird doctrine and active repository instructions. The task
is the complete source-derived assembled/exploded watch with useful native CPU
and P100 work, not an extension of the old Python triangle loop baseline.

Studied existing evaluator ABI/source, CUDA lifetime handling, CMake targets and
watch display. Read and acquired intact Pineda1988 and Laine/Karras2011 raster
papers, plus the pinned CUDA12.8 Pascal guide. Sources/hashes/restoration and
concrete design input are in papers/2026-10-15-om10-native-display-study.md.

Coordinated original-UV analytic frames with import owner: keep all independent
axes; cone V is slant distance. Proposed native scene/vertex/carrier records,
original UV span maps, shared boundary XYZ, physical keys, stable occurrence IDs,
source-normal consumption, native tile display and explicit cold/reused timing.
Request size was checked and corrected to192bytes with two reserved f64 fields
before design acceptance. Alpha0 must branch before explosion arithmetic.

Sent the layout to root/import/oracle owners for review. Called out CPU/CUDA
transcendental last-bit differences at display snap thresholds: exact integer
coverage does not imply universal whole-image identity when source positions
are independently evaluated. Tiny exact-answer fixtures and whole-source
bounded position checks attack the actual faults; do not invent a promise of
byte-identical source-derived images.

No product code, geometry, test, render, native build or Git mutation occurred
in this research stage. Await integrated reviewed design and plan.

Integrated design and implementation plan accepted after native review. Added
explicit finite-result errors, original assembled alpha0 branch, and deterministic
supersample ID/depth policy. Oracle owner caught cameraY convention ambiguity;
we resolved it as screenY=centerY-scale*cameraUp before implementation green.
Initial two native fleet tests genuinely failed because the executable did not
exist; source/assembly reds were separate. Later expanded cases are not claimed
as initial red.

Implemented the isolated native/om10 C++17 executable and optional CUDA12.8/sm60
plugin. Own checked binary parsing, original-UV analytic/Bezier first jets,
limiting normals, copied source values, retained CPU workers/device geometry,
rigid instances/explosion, fixed-subpixel top-left coverage, complete CSR tile
bins, two-level512-entry GPU scans, visibility, source-normal shading, linear
supersampling/sRGB and explicit geometry/projection/identity diagnostics. CPU
path never loads CUDA. No existing ABI/build changed. Root source review caught
valid cone reference radius0 and proper rotation consistency; fixed both.
Static self-review fixed partial-upload cleanup pointer ownership and CUDA
arithmetic source-row errors before the first GPU qualification.

Fleet Release CPU+CUDA build succeeded, with no host warnings after cleanup;
nvcc reports the expected pre-sm75 architecture deprecation. Six direct methods
passed on CPU1, CPU24, P1000 and P1001: exact tiny pixel/depth/source IDs,
original plane/reversed normal jets, shared edge/opposite winding/offscreen,
front/coplanar tie/instances/explosion, projected-only rotated normals/mapping,
and supersample resolve identity. Expanded analytic/failure/multiblock cases and
independent source review remain. No actual OM10 rasterization has run.
