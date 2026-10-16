# Whole native operation and presentation

Complete runtime-plan section7 with one executable, `brepgpu-mesh`. Its required
source, output, positional tolerance and source-node tolerance select exactly the
same RunPreparationJob, scheduler/coordinator, publisher and SceneStore already
used by the connected tests. CPU/P100 selects the numerical service and the
source-profile display API together; missing CUDA modules fail explicitly. A
mesh-only option is a declared operation scope, never a completed render claim.
No Python geometry, subprocess orchestration or new dependency enters this path.

The executable validates options before making a sibling temporary directory.
It refuses an existing final directory, writes all required artifacts and closes
them, then uses a no-replace Linux directory rename to publish. A concurrent
creator must not be overwritten. Failure removes only its own temporary output
and emits a typed stage error. The final directory contains the full semantic
mesh, source scene and views, native timing/count metadata and required images.
Use original product/usage labels in a TSV keyed by occurrence, with stable short
occurrence numbers on the exploded image. Labels describe existing instances;
they never become geometry keys.

Presentation derives definition boxes from stored XYZ, transforms all eight box
corners with actual occurrence matrices, and fixes one camera/basis/framing for
matched CPU/P100 inputs. Generic explosion moves occurrence box centers outward
from the assembly center; it does not inspect source names or IDs to select
geometry. Its scale is recorded. Degenerate assembly extent fails explicitly.
Bounds include both assembled and exploded positions. A separate presentation
array retains explosion translations; original NativeMesh transforms stay exact.

Render assembled/exploded views with the existing CPU v2 API or CUDA v2 factory.
Keep source evaluation, upload, projection, setup, bins, shading, download, image
encoding and close times separate from source preparation and meshing. Whole
operation wall includes the required output through close; output publication
and process teardown are named separately. Worker work sums are never described
as elapsed time. Draw measured stage values on the finished images after those
stages complete; whole-operation time remains in TSV because it includes writing
the graphics themselves.

An owned small PNG writer uses PNG RGB8 with zlib stored DEFLATE blocks, CRC32 and
Adler32. This is deliberate bounded output machinery, not a new compression
library or an image-quality change. Validate against an independent standard
PNG reader in the external test boundary. A finite owned bitmap alphabet supplies
labels/timing text; raster drawing is cosmetic and cannot modify mesh positions.
PNG, camera and label oracles check actual encoded bytes/pixels and known affine
placement, not merely successful exit. CLI tests exercise a real complete source
fixture, publication refusal (including existing bytes), CPU/P100 semantic mesh
agreement, required nonempty images and image dimensions. Scoped host sanitizer,
strict build and actual-source peer review close this unit. Broad geometry and
watch performance belong to the same final operation but not to a second image
certification campaign.

Plan review: topology owner ACCEPT with a timing correction. Artifact TSV records
an explicitly prepublication snapshot; it cannot include the later close of its
own file or directory rename. After no-replace rename, stdout reports actual
publication and required-output-through-close elapsed wall. Only an external
runner measures process exit. Device/session/resource close remains a directly
measured stage. Camera basis is common; assembled and exploded framing may use
their respective stored bounds, fixed identically across backends and retained
in the emitted view records so the assembled hero stays legible.

Whole-operation workload connection uses separately authored polynomial sheets
(12 disjoint faces in one definition), a positive-weight rational arch, and a
piecewise-C0 tent surface. Original source controls and boundary curves are
constructed directly, without the production evaluator. Both reused assembly
occurrences survive. Independent POWER/rational/piecewise-linear formulas check
actual emitted facet interiors at original UV; CPU worker counts and P100 must
also preserve complete semantic bytes. The many-face family exposes simultaneous
mutable-owner scheduling; single-face fixtures cannot establish that behavior.
