# Complete OM10 source-to-image pipeline

The target is the supplied **Association openmovement OM10 MAN VA01.3**:
107 solid definitions,9,162 faces and166 placed leaf occurrences. Assemble and
explode the same complete geometry, preserving original names, usage paths,
rigid transforms, edge/coedge identities and per-face source mapping.

The source-to-display implementation is under qualification. Fresh explicit UV
preparation now exports all107 definitions and their unmeshed query copies with
actual STEP shape processing disabled. Independent checks pass every original
carrier, spatial curve and assembled placement. Thirteen focused methods and
the bounded new ASan/UBSan fixture pass; source-chart and numerical recovery
retain the original geometry, parameters and selected tolerances. The whole
source mesh and CPU/P100 integration still must pass before actual watch
rasterization. Stopped and partial runs are not passes. See the
[source UV design and current limits](../papers/2026-10-15-om10-explicit-uv-source-preparation.md).
The [reviewed plan](OM10_IMPLEMENTATION_PLAN.md),
[source/recovery study](../papers/2026-10-15-om10-import-mesh-study.md) and
[independent-oracle subplan](OM10_ORACLE_SUBPLAN.md) describe those obligations.

The operator also requires **NVIDIA USD/BRep on CPU** on this same complete
movement and both views. The owned CPU/P100 matrix isolates our acceleration;
it does not supply that external baseline. The genuine NVIDIA producer and raw CPU display are implemented and tested on
authored geometry under both declared healing policies; complete watch
admission and measurements remain pending. External results
must identify their actual source translation and renderer; no OCCT or owned
triangles may be reported as NVIDIA output. The final comparison remains
incomplete until that reference run and its outcomes are reported.

| Stage | Ownership and execution |
| --- | --- |
| STEP/XDE extraction | Isolated pinned OCCT8.0.1 CPU adapter; original IDs, units, source geometry, reconstructed pcurves and diagnostics |
| Boundary recovery and mesh | Owned Python CPU machinery; shared original-edge samples, exact predicates, complete multi-hole domains, conforming refinement and provenance |
| Assembly and explosion | Owned CPU occurrence graph and rigid composition; authored world offsets, exact original assembled endpoint |
| First jets, placement and display | Owned C++17 CPU implementation or explicit CUDA12/P100 plugin; original-UV positions/normals, tiled visibility/shading and image resolve |
| Presentation | Owned standard-library PNG/SVG composition; source-linked callouts and actual timings |

The source bridge supplies geometry and reconstructed UV curves. Product
triangles come from the owned mesher. CPU mode runs independently of CUDA.
The GPU path accelerates the named native geometry/display stages; common CPU
import and meshing remain in the complete wall. This is an imported-assembly
consumer, separate from the existing narrow Boolean and feature constructors.

## Build on the fleet

Restore the pinned OCCT archive using
[the retained source recipe](../papers/acceleration-roadmap-reference-restoration.md).
Keep source archives and the private STEP intact in ignored quarantine.

```sh
bash scripts/build_om10_reference.sh \
  quarantine/new-iteration/occt \
  quarantine/om10/occt-build \
  "$PWD/quarantine/om10/occt-install"

cmake -S native/step_source -B build/om10-step-source \
  -DCMAKE_BUILD_TYPE=Release \
  -DOpenCASCADE_DIR="$PWD/quarantine/om10/occt-install/lib/cmake/opencascade"
cmake --build build/om10-step-source --parallel 8

cmake -S native/om10 -B build/om10-release \
  -DCMAKE_BUILD_TYPE=Release -DBREPGPU_OM10_ENABLE_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="${CUDACXX:-nvcc}"
cmake --build build/om10-release --parallel 8
```

Use the qualified CUDA12.8 compiler for the Pascal build. The native executable
does not link CUDA; the explicitly selected plugin contains sm_60 device code.
A CPU-only build can omit BREPGPU_OM10_ENABLE_CUDA. Existing runtime ABIs and
canonical body admission remain separate.

## Qualify before rendering

All modeling, native execution, tests and benchmarks run on the Linux fleet.
The supplied original file must have SHA-256
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.
Choose unused output directories; callers reject existing destinations.

```sh
python3 -u scripts/qualify_om10.py \
  --step om10-00001_20220701_va_01_3.stp \
  --output context/om10-qualification
```

This extracts and meshes once, checks original STEP geometry/assembly and
complete mesh domains/physical shells, then retains that model while qualifying
actual CPU24/P1000/P1001 source jets and both projected poses. Two PROJECT_ONLY
view requests produce no raster image. Existing independent source, Cox/analytic,
UV-chain, physical-incidence and native-output checks provide the answers.
Source and binary pins must remain unchanged throughout the run.

## Render and measure matched operations

After complete qualification passes, use the actual complete caller:

```sh
PYTHONPATH=python python3 examples/render_om10.py \
  --step om10-00001_20220701_va_01_3.stp --mode render \
  --backend cpu --threads 24 --view both --output context/om10-cpu

PYTHONPATH=python python3 examples/render_om10.py \
  --step om10-00001_20220701_va_01_3.stp --mode render \
  --backend cuda --device 0 --view both --output context/om10-p100

PYTHONPATH=python python3 scripts/benchmark_om10.py \
  --step om10-00001_20220701_va_01_3.stp \
  --output context/om10-matched-benchmark

PYTHONPATH=python python3 scripts/compose_om10_posters.py \
  --benchmark context/om10-matched-benchmark \
  --reference-outcomes context/om10-nvidia-reference/reference-outcomes.tsv \
  --output context/om10-posters
```

Defaults are2400×2000 pixels,2× supersampling per dimension,5μm mesh target
and1μm selected source residual. Input model uncertainty remains a separate
source value. GPU0/GPU1 are measured separately against native CPU1/CPU24.
The driver executes two fresh complete callers in fixed CPU24 then P1000 order.
Each extracts and meshes the actual STEP once, then produces assembled followed
by exploded PNG/labeled-SVG views. Its complete wall is one observation (n=1)
for **both labeled views**, including interpreter exit. Filesystem caches are not
flushed; these two observations do not establish a statistical workflow speedup.
The driver compares original-source metadata, source-snapshot, scene, complete
two-view request and product-source hashes across the callers. It copies each
exact192-byte request row under a single-view header for three alternating fresh
native processes per pose/backend. Native medians and ranges describe prepared-
scene display, including raw output and shutdown; they exclude STEP, meshing,
PNG and SVG. No geometry or floating-point request field is reconstructed.

Resident native runs retain actual scene/source jets symmetrically on CPU and GPU.
Their per-frame walls include placement, bins, display, transfers and raw native
writes. They exclude Python PNG/SVG composition and common preparation; they
are view-update results. Native process walls include native shutdown. Complete
STEP-to-both-views process walls include both PNG/labeled-SVG writes and
interpreter exit. Each poster shows the same actual full-workflow CPU24/P1000
totals, labeled **FRESH STEP → BOTH LABELED VIEWS · n=1 · SHARED TOTAL**, in a
separate panel from that pose's four repeated native-process medians. A shared
total is not a measured fresh-to-single-pose wall; do not divide it or duplicate
it as per-pose observations. The fixed complete P1000 caller supplies
000-assembled and001-exploded artwork. CPU1/P1001 have native measurements;
no complete workflow wall is inferred for them. Individual frame/encoding stage
rows remain narrower diagnostics without apportioned common preparation costs.
The [reviewed two-view measurement plan](OM10_TWO_VIEW_BENCHMARK_PLAN.md)
records the scope and its tradeoff against separate fresh per-pose callers.
The full-workflow panel additionally retains both predeclared NVIDIA CPU
policies,heal0 andheal1, from their actual reference-outcomes.tsv rows. Successful
rows require the same original source/two-view hashes and completed admitted
fresh two-view wall. Failed/unsupported rows remain explicit without a comparable
headline total; actual elapsed and diagnostic times remain in the retained table.
The reference baseline includes NVIDIA CPU import/tessellation and shared owned
CPU display. Actual poster pixels remain the fixed owned P1000 images; no
reference failure is replaced by owned triangles or hidden from the comparison.
The [reference-poster plan](OM10_REFERENCE_POSTER_PLAN.md) records the direct
interface. Final composition requires both actual outcomes; synthetic test
values are confined to temporary fixture directories.
Final SVG-to-PNG rasterization uses the existing static compositor separately
and is named/timed separately. Stage rows overlap and cannot be summed as a
complete operation. Native upload/download means host↔GPU transfer.

## Geometry, provenance and output

Original STEP contains no UV trim curves. Reconstructed foreign pcurves remain
available as provenance and branch guides. Owned source-driven plane/cylinder/
cone recovery preserves original XYZ and parameters; exact rational plane UV
preserves collinearity. Singular source poles retain explicit chart closures.
Clamped linear spline-boundary columns recover original LINE parameters and
their incident original-vertex joints by exact source coefficients. This keeps
the generic128ULP outside-domain rejection unchanged. Foreign UV that cannot
be evaluated inside the original domain has an explicit count; its raw values
remain retained and corrected source residuals must still pass.
Nominal internal-near-tangent circular joints on planes can require coordinated
chord samples. The owned sampler finalizes a shared schedule before emitting
edge keys, preserving original curves, endpoints and full parameter intervals.
Every incident carrier must admit every interval under the existing budget;
exact final boundary simplicity is mandatory. Admission uses a stated nominal
frame grade and the retained translated joint envelope capped by the selected
source residual. It makes no exact source-isotopy claim. Protected finite collars,
sample/work limits and unresolved multiple crossings reject explicitly. The
[source study](../papers/2026-10-15-om10-tangent-boundary-study.md) records the
observed cusp, actual coupled faces and independent review.

Clamped spline joints near one natural V boundary can retain a source tolerance
gap even when each foreign UV endpoint correctly follows its original curve.
The owned recovery selects between those two existing endpoints at the same
directed original vertex, within one original U-knot cell. It changes no curve,
vertex, active interval or knot. The selected endpoint correction participates in
every incident trim chord bound before shared sampling, with explicit joint
provenance. Original spatial-curve chord bounds also include the measured
departure of canonical endpoint vertices from the original curve. Both terms
must fit the existing allocation; source tolerance supplies no extra allowance.
The [source diagnosis and design](../papers/2026-10-15-om10-spline-joint-recovery.md)
record the finite candidate scope and direct counterexample.

All physical triangles must have nonzero area; only proven pole quotients omit
collapsed physical cells while retaining their UV coverage/error accounting.

Exact owned UV coverage and separately rounded native UV are distinct claims.
Native rounding has a displacement allocation, and corrected boundary source
residuals remain explicit. Numerical bounds are evaluated in binary64 with
stated limitations; finite probes do not establish universal certification.
Per-cell acceptance includes the ideal surface interpolation bound plus the
largest canonical-corner displacement from that surface. The final bound adds
twice the shared-edge chord bound and native UV rounding. Source-consistency
residual is a separate gate; it is not added to the requested mesh tolerance.
Unsupported parameter correspondence, topology, branch, quality, resolution or
work-cap conditions fail the whole caller. No face is silently omitted.

Outputs include the complete BOM and original transforms, source products and
styles, authored explosion/palette, source-face and primitive-instance mapping,
source/binary hashes, error metrics and measured stage walls. Native rendering
writes RGBA, depth and primitive/instance IDs. The caller writes PNG pixels and
labeled SVG compositions using visible instance anchors; final poster PNGs use
the separately timed static SVG compositor. Original CAD and operational binary
snapshots remain private. Source-derived artwork carries its
[own attribution and license](../assets/om10/LICENSE.md).

The [return-pass notes](RETURN_PASS.md#whole-om10-implementation-observations)
retain preparation, refinement, mesh reuse and Python/native ownership questions.
External community qualification still gates alpha separately.
