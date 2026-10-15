# Complete OM10 source-to-image pipeline

The target is the supplied **Association openmovement OM10 MAN VA01.3**:
107 solid definitions,9,162 faces and166 placed leaf occurrences. Assemble and
explode the same complete geometry, preserving original names, usage paths,
rigid transforms, edge/coedge identities and per-face source mapping.

The complete pipeline is implemented. Focused ring/spiral connectivity,
axial-edge and source-owned spline-boundary checks pass; whole-source mesh
and three-backend integration must pass before actual watch rasterization.
The [reviewed plan](OM10_IMPLEMENTATION_PLAN.md),
[source/recovery study](../papers/2026-10-15-om10-import-mesh-study.md) and
[independent-oracle subplan](OM10_ORACLE_SUBPLAN.md) describe those obligations.

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
  --output context/om10-posters
```

Defaults are2400×2000 pixels,2× supersampling per dimension,5μm mesh target
and1μm selected source residual. Input model uncertainty remains a separate
source value. GPU0/GPU1 are measured separately against native CPU1/CPU24.
The driver executes four fresh complete callers: CPU24 and P1000 for each pose.
Their complete STEP-to-image walls are single observations (n=1). Each includes
actual STEP extraction and owned meshing. It then measures three alternating
fresh native processes per pose/backend using those unchanged actual prepared
scene/view inputs. Native medians and ranges describe prepared-scene display,
including raw output and shutdown; they exclude STEP, meshing, PNG and SVG.

Resident native runs retain actual scene/source jets symmetrically on CPU and GPU.
Their per-frame walls include placement, bins, display, transfers and raw native
writes. They exclude Python PNG/SVG composition and common preparation; they
are view-update results. Native process walls include native shutdown. Complete
STEP-to-image process walls include PNG/labeled-SVG writes and interpreter exit.
The poster shows these single-observation full walls in a separate panel from
the four repeated native-process medians. CPU1/P1001 have native measurements;
no complete STEP-to-image wall is inferred for them.
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
