# Conservative plane ranges for the NVIDIA reference

The two watch definitions that reached our exporter with crossed polygon rings
now produce genuine NVIDIA triangles after correcting their finite plane ranges.
The original 3D curves survive conversion, but their converted finite plane
patches exclude boundary points by about2.4mm. SDK lazy UV generation on those
patches consequently disagrees with the spatial curves. Widening the finite
support ranges removes that discrepancy and the exporter refusals, without
changing the exporter or supplying replacement triangles.

This is an explicit reference interchange policy. It changes only Plane entries
in `face:range` in a new USD layer. Original plane origins/axes, spatial curves,
knots, topology, labels and all other typed layer values remain equal. The old
input, binary and failed outcomes remain available. Both healing policies pass
the selected two-definition controls and existing closed-shell/source-Euler
checks; these are not whole-watch distance qualification or complete-render times.

The [native helper](../native/om10_nvidia/plane_envelope.cpp) captures every
imported positive-weight rational B-spline boundary control and original vertex,
plus the SDK's final canonical plane frames. The [preparation script](../scripts/prepare_nvidia_plane_envelope.py)
uses exact rational arithmetic to form a definition-wide control box and project
its eight corners into each represented plane frame. It unions that enclosure
with the original finite ranges and rounds endpoints outward. Positive rational
basis weights justify the control hull throughout the admitted natural curve
intervals. The definition-wide box avoids guessing face/edge correspondence;
its bounds can be loose. Unsupported curves, nonpositive weights, invalid frames,
incomplete inventories and composed/animated input refuse.

The script requires the existing reference package's `usdpy` bindings. It is an
external USD preparation tool; the owned C++/CUDA mesher has no new dependency
or Python geometry callback. Build the helper with the existing isolated
`native/om10_nvidia` CMake project. For a self-contained USD already emitted by
the official converter:

```sh
export PYTHONPATH="$NVIDIA_PACKAGE/usdpy:$PWD/python"
export LD_LIBRARY_PATH="$NVIDIA_PACKAGE/lib:$NVIDIA_PACKAGE/extraLibs"
export OMNISOLID_PLUGIN_PATH="$NVIDIA_PACKAGE/omniSolid/resources"
export PXR_WORK_THREAD_LIMIT=1 OMP_NUM_THREADS=1 CUDA_VISIBLE_DEVICES=''

python3 -B scripts/prepare_nvidia_plane_envelope.py \
  --input-usd converted.usdc --output-directory new-preparation \
  --helper build/om10-nvidia/brepgpu-nvidia-plane-envelope

build/om10-nvidia/brepgpu-om10-nvidia \
  --heal 0 --chord-height .005 --curve-angle 25 --surface-angle 25 \
  --usd-definition ORIGINAL_ID new-preparation/corrected.usdc \
  --output-directory new-reference-mesh
```

Use a new output directory for each operation. Preparation reports include the
input/output hashes, actual policy, changed ranges, source inventories and stage
times. Record full process wall separately, including Python/package startup.
The producer records the prepared input kind, copying, import, tessellation and
export times. It imports at the input layer's original address to preserve USD
relative asset resolution; its retained byte copy is not a dependency archive.
Original `--definition ID INPUT.brep` still invokes the official converter.
Omitted angle options retain the historical0/0 policy, so pass25/25 explicitly
when selecting the documented SDK angular defaults.

An independent translated radius2/height3 cylinder fixture narrows its two cap
domains deliberately. Its regression checks restored support against the
literal circle extrema, unchanged source attributes, genuine NVIDIA triangles,
closed-shellEuler2 and the original cylinder equations. A separate cylinder
reimport test checks complete mesh/normal/ancestry byte identity through the
prepared-input entry. Actual code review and outcomes are in the
[polygon unit](nvidia-polygon-export-completion-unit.md) and its
[journal](../journal/2026-10-17-nvidia-polygon-export-completion.md).

The two earlier periodic conversion refusals have a different cause: absent
pcurves and tolerant full-period seams exceed the converter's fixed angular
span cutoff before SDK healing. A documented OCCT representation split gets one
derivative input through; the other remains unresolved. This plane-range tool
does not claim to solve them. See the [periodic diagnosis](../papers/2026-10-17-nvidia-periodic-input-usage-diagnosis.md).

## Running the preparation tests

`tests/test_nvidia_prepared_pipeline.py` runs the official converter, the SDK
producer and the usdpy bindings, so it needs the package environment above plus
two paths of its own, `numactl` on `PATH` (the caller binds itself and its
workers to a lane), the built delivered-clock probe for context rows and the
built minimal NVIDIA ingress (`build/nvidia-ingress`, see below). It
checks all of them before any test and fails with one message naming each
missing item:

```sh
scripts/build_measurement_probe.sh   # build/measurement/brepgpu-delivered-clock
export BREPGPU_DELIVERED_CLOCK="$PWD/build/measurement/brepgpu-delivered-clock"
export BREPGPU_NVIDIA_PACKAGE="$NVIDIA_PACKAGE"
export BREPGPU_REFERENCE_DISPLAY="$PWD/build/om10-reference/brepgpu-om10-reference-display"
export OMNISOLID_PLUGIN_PATH="$NVIDIA_PACKAGE/omniSolid/resources"
export PYTHONPATH="$PWD/python:$PWD:$NVIDIA_PACKAGE/usdpy"
export LD_LIBRARY_PATH="$NVIDIA_PACKAGE/lib:$NVIDIA_PACKAGE/extraLibs"
python3 -B -m unittest tests.test_nvidia_prepared_pipeline
```

Without `OMNISOLID_PLUGIN_PATH` the converter refuses every input ("reference
plugin unavailable"), which the worker would otherwise report as a periodic
preparation failure. The executables default to `build/` and can be overridden
with `BREPGPU_NVIDIA_CONVERTER`, `BREPGPU_REFERENCE_SOURCE`, `BREPGPU_OM10_NVIDIA`,
`BREPGPU_REFERENCE_PERIODIC` and `BREPGPU_NVIDIA_PLANE_ENVELOPE`.

## What counts as NVIDIA's time

`examples/render_om10_nvidia.py` writes `scopes.tsv` for every run; the
headline is `nvidia_headline`, and `scripts/benchmark_om10_nvidia.py` reports
it as `complete_seconds`.

- **Ingress rule.** Both producers' headlines include ingress from the same
  STEP bytes, timed the same way: one process, fork to exit, with no hashing
  and no checks of ours inside it. NVIDIA reads only `.brep`, so its ingress is
  `native/nvidia_ingress` (`brepgpu-nvidia-ingress`): `STEPControl_Reader`
  read and transfer, then one `.brep` per solid definition, with no XDE names,
  colours, materials or properties. Those files are the input NVIDIA consumes;
  each run records how many are byte-identical to our XDE export. Our XDE
  read (`brepgpu-reference-source`) supplies the placements and face map our
  display and admission use, and is our cost. The owned headline must count
  its ingress by the same rule, and both `..._excluding_ingress` variants are
  reported.
- **NVIDIA's own work** per worker: process start (launch to `main`: the
  launcher's fork time and the producer's `main` start, both on
  `CLOCK_MONOTONIC`; the `/proc/self/stat` start time is a tick-resolution
  cross-check), context setup, conversion, import and heal, tessellation,
  its rectangle-triangulation call, object teardown (each definition's
  completion time minus its measured stages) and process exit (end of `main`
  to reap, same clock). The batch scope is the makespan over workers; the
  measured batch wall is reported beside it.
- **Conversion in prepared mode.** An NVIDIA user converts in-process, as
  the producer does, paying context setup and the first conversion's one-time
  cost (about 10 ms on the authored fixtures, against 2.7 ms per later
  conversion) once per worker. The preparation worker therefore keeps one
  long-lived converter per worker (`brepgpu-nvidia-convert --serve FD`),
  whose replies arrive on a dedicated descriptor, never on stdout. Each
  `ConvertOcctFileToUsd` span is NVIDIA's; request round trips beyond it,
  failed requests and each server's start, setup and exit are ours
  (`ours_converter_process_overhead_sum`); time a server waits while our own
  preparation runs is listed as `converter_service_idle_sum` and charged to no
  one. A server that dies, breaks its pipe or times out is replaced and the job
  retried once; a second failure is ours (worker and caller exit 5), never an
  NVIDIA refusal. Served output is byte-identical to one-shot output (authored
  fixtures; all 107 OM10 definitions on the fleet).
- **Ingress judgement.** The caller refuses NVIDIA input as ingress (exit 4)
  unless the minimal ingress reports the arm's shape-processing (no
  operations for translation-only, some for the default read) and exactly the
  source's definitions as identity-located forward solids. Both readers' OCCT
  libraries are pinned. On a byte difference
  `nvidia-input/nvidia-input-diff.tsv` lists face, edge and vertex counts,
  pcurve presence and the largest vertex move before the refusal.
  Our XDE exporter runs in the arm's mode (`brepgpu-reference-source
  --shape-processing translation-only|default`).
  - **Translation-only arm.** The byte check is per definition. A definition
    whose `.brep` our export corrected (face sides `S1`, `S2`, each with a
    `reference_face_orientation_raw` record) reaches NVIDIA as our corrected
    file, declared in `metadata.tsv` (`nvidia_input_ours_corrected`), once our
    uncorrected translation of it (`source/uncorrected/`) is byte-identical to
    the minimal ingress's file, so the declared correction is the only
    difference from NVIDIA's own translation; otherwise the harness refuses
    (exit 3). Every
    other definition's minimal-ingress file must be byte-identical to our
    export of it, or the arm refuses (exit 4). Placement corrections (`P1`,
    `P2`) change usages, not `.brep` bytes, and exempt no definition; NVIDIA
    meshes each definition alone and never reads a placement, and our display
    places them from the reference or native source, whose usages are read
    raw (`native/step_placements.hpp`). OCCT's located value is written only
    where the adapter's one predicate (`step_placements::agree`) accepts it:
    2^-32 relative, translation within the placing contexts' uncertainty, and
    no point of the child's extent moved by more than it. The extent bounds
    each solid by `native/shape_extent.hpp` (faces by their carriers and 3D
    edges; OCCT's box spans an unbounded carrier where pcurves are missing and
    misses a cone's apex). The same bound's diagonal decides whether a
    declared uncertainty is usable (G6 UR-7), as in the adapter. OM10 writes
    no raw placement in either mode.
  - **Default-read arm.** In default mode the exporter supplies no flags or fix
    parameters, so OCCT's own default processing runs, and it declares
    `shape-processing=default` with the operations OCCT ran
    (`shape-process-operations=`). Its face map binds each original face to
    the faces that processing made of it. NVIDIA's default-read input must be
    byte-identical to our default export for every definition, and both reads
    must report the same operations; otherwise the harness refuses (exit 3),
    because the face map would not describe NVIDIA's input.
  - **Face sides, both modes.** A face whose orientation flag the default read
    changed is decided from geometry over all of OCCT's pieces of it: ray
    parity from probes on both sides of each piece, confirmed by OCCT's solid
    classifier. A face whose raw use points into the material is written with
    its outward side: `S1` when its outer bound runs against it, `S2` when its
    whole shell is inside out. Each seam use of a corrected face stays on its
    own pcurve: the seam's pairing is the one under which the face's wires
    close in the parameter plane, as OCCT's default read pairs the repaired
    face. Every such definition is verified on its published bytes, read
    back: BRepCheck valid and every shell's signed volume of the right sign,
    on a copy that only adds missing pcurves (projected, an added one moved by
    whole periods to meet its neighbour, an added seam paired, and placed in
    the period, so its wire closes); a pcurve that is there is never moved or
    re-paired. On the copy only, an edge that carries an added pcurve takes
    that pcurve's measured deviation as its tolerance where it is larger;
    every other edge keeps OCCT's tolerance. The deviation is capped at the
    definition's usable declared uncertainty (else the adapter's source
    residual grade); above it verification fails by name, so an edge off its
    surface refuses rather than being covered. The cap covers the
    translation-only arm by design: the default read's processed solid carries
    OCCT's own pcurves and tolerances (its FixShape raised them), so the copy
    adds nothing there and that arm's claim is OCCT's default read. `source/verification.tsv`
    lists each verified definition with the largest tolerance raised, the
    largest added-pcurve deviation and the cap. The
    translation-only export writes the correction into its own `.brep` and
    declares the letter. A face whose side is not decidable, including one
    whose evaluation OCCT cannot complete, refuses the export in both modes,
    which the caller reports as ingress (exit 4, ours). A face the default
    read turned against its shell refuses default mode; translation-only
    writes its raw use, which is decided outward.
  The prepared worker expects each input's translated face count.
- **Physical admission.** NVIDIA's mesh of each definition, and ours, is split
  into its edge-connected shells, each closed and edge-consistent; the outer
  shell (largest by magnitude) must enclose a positive volume, every other one
  is a void, negative; shells are pairwise disjoint (exact triangle-triangle
  tests on candidates from a grid), every void lies inside the outer shell
  and none inside another void (one vertex each, by ray parity). Euler is
  taken over all shells and compared with the source's, each face counted
  from its own topology (chi = 2 - 2g - b - p: a natural torus 0, a band 0, a
  natural sphere 2). A fault in the mesh NVIDIA produced (as our wrapper
  copied it) is NVIDIA's FAIL; a fault in our wrapper's `.bgrefmesh`
  container is ours (exit 5). An inside-out mesh fails, on both arms
  alike. Only such a verdict on NVIDIA's mesh is an NVIDIA FAIL; an error in
  our admission code is ours (exit 5). NVIDIA's own mesh of the correct
  `void-block.step` fails: six directed edges are used twice the same way
  (three void faces wound against the material) and its volume is 840
  against 936, in both ingress arms, with both shells forward, and both heal
  settings; NVIDIA's own report says input manifold 1, output manifold 0,
  12 lamina edges. The legacy matched mode compares
  every face's carrier, side and style with the STEP source adapter's source
  of the same file (`source.bgsource` in the matched directory), as the
  native match does; without one it refuses the matched inputs (exit 3). In
  both modes a definition the adapter leaves incomplete (a face refused)
  refuses before NVIDIA's ingress as ours (exit 4; every physical row not
  evaluated), so no NVIDIA outcome on it is scored.
- **Page cache.** One policy for NVIDIA and owned ingress
  (`measurement_lane.prepare_ingress_cache`; `measurement_lane.py run
  --ingress-file`): immediately before each ingress the STEP file's pages are
  dropped with `posix_fadvise(POSIX_FADV_DONTNEED)` and the resident fraction
  of the STEP file, the tool and its OCCT libraries is recorded
  (`page-cache.tsv`). Library pages are recorded, not dropped.
- **Ours, listed and excluded:** input preparation (periodic split and Plane
  ranges; charged as required only when it changed the input bytes, and the
  headline is flagged "requires our preparation"), prepared-USD copies, f64
  extraction, mesh writes, report bookkeeping, physical admission, decode and
  scene packing (the emit stage), display and presentation, context capture
  and pin hashing. An NVIDIA conversion refused before our repair is listed
  as `nvidia_failed_attempt_sum`, not in the headline.
- **Quiet gate.** Owned and NVIDIA runs use the same policy
  (`measurement_lane.measure_quietly`): settle checks on foreign CPU only,
  run immediately before the timed region (for NVIDIA, inside the caller after
  our XDE read, before NVIDIA's ingress), the run's own foreign CPU included
  (for NVIDIA, its `nvidia_ingress` and `nvidia_batch` context rows), up to
  three attempts, and the first quiet attempt (else the first attempt) as the
  one n=1 estimate. Choosing an arm by the minimum over many single runs
  favours NVIDIA, as the steel-man intends; a headline needs the median of
  at least 10 fresh processes (G8 §7.5). Interrupt,
  softirq and writeback time that no process owns counts as foreign.
- **Validity.** NVIDIA's arm is valid when its batch and our physical
  admission pass, and in finite mode when its geometry quality passes. A failure of our ingress tooling exits 4 (`ingress_refused`)
  and of our post-batch work exits 5 (`ours_failed`); neither is an NVIDIA
  failure. A harness or settings problem exits 3 (`harness_refused`).

Build the minimal ingress against the pinned OCCT:

```sh
cmake -S native/nvidia_ingress -B build/nvidia-ingress -DOpenCASCADE_DIR="$OCCT_PREFIX/lib/cmake/opencascade"
cmake --build build/nvidia-ingress
```
