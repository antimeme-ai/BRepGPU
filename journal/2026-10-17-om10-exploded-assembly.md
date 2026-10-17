# OM10 exploded assembly illustration

Parent requested a proper exploded movement diagram rather than a pin-lifted
assembly. Began read-only study on the NVIDIA repair branch, with no code/render
or visual commit selected before a separate feature branch/worktree.

Viewed the actual CPU3840×4200 poster and the already acquired OM10 technical
sheet/ETA assembly illustration. Read native presentation, current model caller,
scene replay and legacy Python layout. Found the actual native path never calls
the old functional-tiered Python layout; it only scales centroid displacements.
This concretely explains the close broad plates and visibly floating pins.

Read the original source graph on fleet without geometry evaluation:31 groups,
20 named top-level non-leaf modules,166 occurrences. Saved source group inventory
only in ignored context/adaptive-watch/explosion-design. Acquired/read the
primary Agrawala2003 assembly-instruction paper, retaining PDF/hash and grounded
notes. Proposed source-functional stack/lanes, meaningful axial guides and a
complete external legend, with translation-only scene replay and shared matched
CPU/P100/NVIDIA offsets. The illustration does not claim physical disassembly.

Focused design/plan and literature/source study are saved for parent review.
No mesher/source/native caller/CMake/reference pipeline file changed. No native
build, geometry operation or image render ran for this design checkpoint.

## Implementation and actual preview

Implemented the source-linked layout/replay in `examples/om10_explosion.py` and
`examples/render_om10_exploded.py`. The native scene codec copies every section
and instance byte except the three presentation-translation doubles. The direct
fleet fixture first failed on the absent module, then passed its two transport
properties: repeated prototype instances with distinct IDs/rotations retain
unchanged bytes, and missing/extra/nonfinite translations or duplicate IDs
refuse. Logs are ignored `context/explosion/transport-{red,green}.log`.

One real complete166-occurrence CPU12 preview rendered at1920×2000, using the
same immutable completed model and source-profile1 display executable. All20
named major parts had actual visible ID-buffer pixels. The native both-view
process was1.11s. SVG postprocessing initially used the wrong original timing
scope (`q0` rather than `query0`); corrected that lookup and exported the SAME
saved pixels without rerendering. This was a caption failure, not geometry or
display failure.

Parent viewed the actual annotated preview and accepted its module separation,
source colors and framing. Applied only the requested central train/bridge
clearance increase and moved the train caption left/up; no further cosmetic
search selected. Complete original names and all166 source paths are retained
in the manifest/legend; small attached jewels/posts may remain occluded. Final
matched CPU12/P1001 replays will reuse one exact translation word and camera.

## Final matched replay

Implementation checkpoint `5a330e7` preceded final execution. Ran finalCPU12,
thenP1001/CPU12-host serially on the same pinned completed CPU12 mesh, same
source-profile1 display executable/API2 module, exact translation manifest and
BGVIEW bytes. Affinity was the12 physical cores0,2,…,22. No CAD import, remeshing,
source alteration, native build or numerical qualification ran.

Both finals completed. Each retains107 definitions,9,162 source faces and166
original occurrences/rotations. All20 selected major parts have actual visible
ID-buffer pixels;150/166 occurrences are visible. The16 occluded occurrences
remain source-attached garniture and remain in the complete manifest/legend.
Viewed both actual final posters: central wheels/bridges have clear stepped
separation; barrel, winding, balance/spiral, anchor and calendar modules are
recognizable. Panels remain outside native geometry pixels. Original colors and
light background retained; no further aesthetic iteration.

Native raster3840×4000 with2× supersampling; poster3840×5871. CPU/P100 raw
assembled PNGs, raw exploded PNGs, visibility tables, scene bytes, manifest and
view bytes are byte-identical. Backend headers/timing captions differ honestly.

| Additional replay/export scope | CPU12 seconds | P1001 display / CPU12 host seconds |
|---|---:|---:|
| Assembled raster + download (disjoint native stages) |1.821967835|0.231227486|
| Exploded raster + download (disjoint native stages) |0.421096643|0.198828770|
| Native both-view process (`time` wall) |5.14|4.02|
| Source/index admission, layout and scene copy |11.269678|11.032618|
| Both actual PNG encodes |1.314155|1.252280|
| ID visibility, background mask and guides |13.576433|13.667918|
| SVG composition |0.008983|0.008756|
| SVG rasterization |2.363776|2.381637|
| Complete new replay/export |33.882756|32.599048|

Original CPU12 cold source→mesh→1920bothviews875.16s and meshing847.712233632s
remain separate historical scopes. Both new displays deliberately replay that
SAME CPU12 completed mesh; P100 display does not imply a new P100 mesh run.
All22CPU/23CUDA native stage rows, original native tables, process wall/user/
system/peak-RSS and actual PNG/SVG/export timings remain in ignored bundles
`context/explosion/final-{cpu12,p1001}` locally and on fleet.

Root actual-source review accepted byte preservation and visibility semantics,
then requested three small reporting corrections: make CPU12 mesh provenance
explicit even under P100 header, validate successful finite original process
wall before rendering, and name visibility rows by occurrence ID rather than
assuming label row order. Closed all three. Re-exported only captions/annotations
from SAME saved native pixels and IDs; no native rerun. Separate caption export
walls5.022339sCPU /4.793285sP100 (SVG rasterization2.145439s/2.169905s) remain
in `caption-reexport-timings.tsv`, not folded into original display costs.

Final shared geometry scene SHA256:
`41870485e3c40f9bdbfbb35db02f9e9dfae94dc4bf24aa0d050f5047fa87a781`.
Views SHA256:
`20c4271d44c9f78b207460dbd8feef3b913e26c02d2624c1ee4ab0bbe49e5ce1`.
Manifest SHA256:
`b5a573d570effc15412c670ba88b30db71e7d7430ef4659ba3cf91b42c0f25cf`.
Original source SHA256 remains
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.
All pinned source scene, labels, display executable/module and SVG exporter
hashes were unchanged before/after each complete replay/export.

NVIDIA handoff: use `explosion.tsv` original occurrence IDs/full source usage
paths and its world-offset XYZ, and exact `views.bgview` frames. Display labels
are separate166-row original label ordinals. External producer must retain its
own genuine geometry/source inventory; this scene is not a substitute NVIDIA
mesh. Illustration offsets are authored alignment, not validated removal paths.

## Genuine NVIDIA external-mesh replay adapter

Parent selected the same visual unit's external replay continuation. Read actual
`native/om10_reference/raw.hpp`, `main.cpp`, existing NVIDIA assembly writer and
current benchmark transport outcome. Reference display is CPU-only, takes raw
BGREFM01 and existing BGVIEW, emits the same little-endian u64 instance buffers.
Its disjoint frame stages differ from the native-source display: raw projection,
triangle setup, bins, shading/resolve and diagnostic host copy. Never label this
as CUDA display or as new NVIDIA tessellation.

Plan amendment accepted by parent. Implemented separate
`examples/render_om10_exploded_reference.py` and a BGREF translation helper.
Admission requires complete original source/hash/107 definitions/9,162 faces/
166 occurrences, all existing physical/source-Euler reports PASS, full original
face inventory, original usage paths and bit-identical12-double poses. Copies
final manifest/views/labels exactly; rewrites only instance explosion3d. Every
geometry/normal/material/triangle/face byte remains unchanged. Actual independent
numerical qualification remains separate; accepts/report labels the selected
benchmark `COMPLETE_UNQUALIFIED` nominal .005mm request without a matched-quality
claim. Original source→both-image outcome and new replay/export costs are separate.

Existing direct fixture extended with a two-definition/three-occurrence raw
reference scene, repeated prototype and rotated placement. Actual missing-helper
RED then four methods GREEN in.001s on fleet; geometry/normal/face/material bytes,
wrong ID/pose/definition and invalid offsets are checked. Logs are ignored
`context/explosion/reference-transport-{red,green}.log`. Reference display's
actual `--help` and new Python entry import/help succeeded. No native build,
meshing, reference repair or complete-watch replay was performed. Waiting for
parent's completed NVIDIA dataset and selected outcome before one replay.

## External ordinal correction and public diagrams

Root actual-source review found an invalid API assumption in the reference
adapter: exported face values are SDK ordinals0…input_faces−1, not original STEP
face IDs. Prepared periodic faces can have multiple descendants. Replaced the
false join with complete SDK ordinal coverage from each actual definitions.tsv
`input_faces` count. Original coverage remains separate in the preserved source
manifest and parent pipeline preparation/history/physical gates; the adapter
does not invent an ordinal-to-original mapping. The direct authored regression
uses different original IDs and two SDK descendants of one original face. All
five direct methods pass on fleet (no full-watch replay or native rerun).

Parent accepted final owned visual/source closure and requested publication.
Published the accepted CPU/P1003840×5871 diagrams under new assembly filenames,
preserving earlier images/results. Added a small result report, all new native/
replay/caption-export stages, actual process costs, aggregate visibility, metadata
and tool/source pins. README OM10 hero now uses the proper functional assembly
diagram; source credit/CC BY-SA3.0 and authored-translation limitations remain.
The NVIDIA adapter is ready, but reference physical completion remains pending;
no NVIDIA diagram or qualified-quality claim is published.

## Complete genuine NVIDIA diagrams

Parent supplied both complete chord-only reference outcomes and authorized two
serial replays after all reference measurements ended. Both have all107 physical
shell/source-Euler checks PASS, complete107/9162/166 original inventory, and
COMPLETE_UNQUALIFIED status. Actual policy is0.005mm chord, curve/surface angles
0°/0°, heal0/1, explicit periodic-plane preparation. Added those actual metadata
fields visibly to the caption and retained the original metadata/outcome. No
new meshing, geometry repair or independent distance qualification ran.

Ran both proper-layout replays serially with CPU12 affinity0,2,…,22. Both final
PNGs were inspected. Every20 selected major part has real visible pixels;
150/166 occurrences are visible and all166 remain. The exact manifest, view and
label byte hashes match both owned diagrams. Supplied reference materials remain
unchanged, explaining the reference's gray/gold palette. Only BGREF instance
explosion triples changed; exact pose, geometry, normals, keys, faces and materials
were preserved by the slice comparison. All input/tool hashes remained unchanged.
Each scene has694,248 raw normal corners and281,156 placed triangles. The two
healing policies have equal assembled raw PNG bytes but different exploded raw
PNG bytes; no forced equality or replacement.

| Scope | NVIDIA heal0 seconds | NVIDIA heal1 seconds |
|---|---:|---:|
| Parent's original fresh source→both old views |43.673683950|45.001244677|
| New assembled raster + host copy |2.174333243|2.423829944|
| New exploded raster + host copy |0.478745018|0.526560666|
| New both-view native process (`time`) |5.57|6.11|
| Source/pose/layout admission + copy |12.662044913|11.914417957|
| Both PNG encodes |1.257947359|1.261015085|
| Actual-ID mask/visibility/guides |13.325605402|13.251167394|
| SVG composition |0.008750942|0.008363309|
| SVG rasterization |2.669154608|2.706944669|
| Complete new replay/export |36.140975761|35.871445750|

Original source pipeline scopes and every new native/replay stage are retained
in the public result tables, with nesting explicitly nonadditive. New process
RSS/user/system are actual `/usr/bin/time` results, not fabricated legacy fields.
No matched-quality ratio follows: the nominal NVIDIA request is unqualified.

Published both large NVIDIA diagrams under new filenames, left owned CPU/P100
images unchanged, and updated the result report/license/README. Preserved policy
history:25°/25° exported107 but one shell failed physical checks under both heals;
a private advancing-front control was censored180s without a result and was not
used in production. No further runs are selected. Ignored actual bundles are
`context/explosion/final-nvidia-chord-only-heal{0,1}` locally and on fleet.

Root integrated the completed reference branch with a merge commit, bringing
the periodic tool, full direct results and current AGENTS notes into this
reviewable diagram branch. The README now gives the actual43.67/45.00s reference
outcomes and links the full report, explicitly separating fresh STEP timing
from the owned source-snapshot timing. Root accepted the actual heal0 poster
and policy-caption/source diff. No geometry or display implementation changed
after the completed replays; no repeated qualification campaign followed.

## Operator-authorized merge

The operator requested merging the completed delivery. PR70 merged into main
with merge commit06234a60c52f8986f3a674c0ead4771f0105e35e, preserving the
reference implementation/history. PR71 was then retargeted from the reference
branch to main; GitHub reports it clean and mergeable. The diagram branch
already contains the completed reference head032632b and the accepted visual
head0b97a3f. Both working trees were clean before this journal-only entry.
No GitHub CI checks are configured on either PR; validation remains the actual
fleet tests, complete reference operations and all four inspected diagrams
recorded above. No implementation changed at merge time and no test or render
was repeated. Merge PR71 with a merge commit as well, then verify main contains
both accepted heads and the published README, reports and large images. Keep
feature branches and ignored operational/source artifacts intact.
