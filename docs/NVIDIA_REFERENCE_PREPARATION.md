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
