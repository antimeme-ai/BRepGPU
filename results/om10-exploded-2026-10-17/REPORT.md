# OM10 exploded assembly diagram

The complete movement is separated into visible functional modules: mainplate,
train, bridges, barrel and mainspring, winding/setting work, escapement,
balance/hairspring, motion work and calendar. **All107 definitions,9,162 original
faces and166 occurrences remain.** Original geometry, orientations, placement
matrices, normals and materials are preserved. Only authored presentation
translations and cameras change. This is an explanatory illustration, not a
validated disassembly or maintenance sequence.

[![P100 display of the complete exploded assembly](../../assets/om10/p1001-exploded-assembly-4k.png)](../../assets/om10/p1001-exploded-assembly-4k.png)

Download full size: [P1001 display](../../assets/om10/p1001-exploded-assembly-4k.png)
· [CPU12 display](../../assets/om10/cpu12-exploded-assembly-4k.png).
Each poster is3840×5871. The underlying native render is3840×4000 at2×
supersampling (7680×8000 internal); headers and the full166-name legend sit
outside those pixels. The actual ID buffers show all20 selected major parts and
150of166 occurrences. Sixteen small attached jewels/posts remain occluded;
they are retained, not omitted. Numbered markers use actual visible pixels.
Dashed guides show authored alignment with source positions, not removal paths.

The CPU and P100 replay **the same completed CPU12 mesh**. Their actual assembled
and exploded raw PNGs, scene, view bytes, translation manifest and visibility
tables are byte-identical. Poster headers and timing captions differ by backend.
Original cold meshing is not repeated or included in the new display figures.

| Actual measured scope | CPU12 | P1001 display / CPU12 host |
|---|---:|---:|
| Assembled raster + result download |1.821968s|0.231227s|
| Exploded raster + result download |0.421097s|0.198829s|
| Both-view native replay process |5.14s|4.02s|
| Source/index admission, layout and scene copy |11.269678s|11.032618s|
| Both PNG encodes |1.314155s|1.252280s|
| Actual-ID mask, visibility and guides |13.576433s|13.667918s|
| SVG composition |0.008983s|0.008756s|
| SVG rasterization |2.363776s|2.381637s|
| Complete new replay/export |33.882756s|32.599048s|
| Later caption-only export from the same pixels |5.022339s|4.793285s|

These are single observations, run serially on twelve physical CPU cores and
P100 device1. The raster sums include only disjoint projection, triangle setup,
binning, shading/resolve and download stages. Process and complete scopes contain
nested stages; do not sum the entire table. The later caption export explicitly
identifies CPU12 as the mesh source under both backend headers; it did not rerun
the native display. All stages, including that export, are in
[stages.tsv](stages.tsv). Actual process exit/wall/user/system/peak RSS are in
[processes.tsv](processes.tsv), with [metadata](metadata.tsv),
[visibility counts](visibility.tsv) and [input/tool pins](pins.tsv).

The original completed CPU12 cold source→mesh→1920both-view process took875.16s;
its meshing scope took847.712234s. Those earlier costs and the separate original
P100 run are retained in the [whole-watch report](../om10-watch-2026-10-17/REPORT.md).
This replay establishes no new meshing speedup or whole-watch quality proof.
Independent whole-watch distance/alpha qualification remains separate. A
NVIDIA external-mesh adapter now accepts the exact layout and cameras, but no
NVIDIA diagram is published here while its full physical pipeline is unresolved.

## Reproduction and source

The layout is in [om10_explosion.py](../../examples/om10_explosion.py); the
[replay/export entry](../../examples/render_om10_exploded.py) consumes an existing
complete native model, its successful process report, pinned source-profile1
native display/API2 module and SVG exporter. It refuses to overwrite outputs.
Use `--help` for the actual arguments; geometry/display execution belongs on the
fleet. The separate [NVIDIA replay entry](../../examples/render_om10_exploded_reference.py)
consumes genuine completed external mesh bytes, not owned replacement triangles.

The scene-copy writer checks every byte except each instance's three explosion
doubles. Direct authored transport fixtures cover repeated prototypes, rotated
placements, exact geometry/material/normal preservation and missing/wrong
identities. Source graph and actual pixel inventories are admitted directly;
these are presentation checks, not another numerical mesh qualifier. The
[design and data interface](../../docs/om10-exploded-assembly-unit.md) records the
functional grouping and translation policy. Input source SHA256:
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.

Original CAD: **Association openmovement**, OM10 MAN VA01.3,
[OM10 project](https://openmovement.org/project/om10/). Images are
[CC BY-SA3.0](../../assets/om10/LICENSE.md). Changes: BRepGPU owned mesh/display,
authored functional-module translations, oblique cameras, background-masked
alignment guides, labels, typography and timing panel. Original CAD bytes and
operational binary snapshots are not redistributed here.
