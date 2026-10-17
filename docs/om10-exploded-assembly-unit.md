# OM10 source-linked exploded assembly illustration

Design/plan only. Parent review and a separate feature branch/worktree precede
product edits or renders. The NVIDIA recovery branch remains devoted to its
reference repair. This unit changes presentation translations/cameras/labels;
it never remeshes, changes a part's orientation, alters CAD or validates a
physical disassembly sequence.

## Actual failure and source basis

I viewed the completed CPU4K exploded poster. Large overlapping plates still
hide most of the train, barrel and regulator; the obvious separation consists
mainly of screws/pins. Many callout leaders converge on the same dense mass.
This is not an image-resolution problem.

At source revision852215550f3e7db634533226c795991aff4fe913,
native/mesher/adaptive_model_main.cpp calls presentation.cpp::make_presentation.
That function chooses the narrow world-AABB axis, then offsets each part by
strength×its axial center displacement and .2×strength×its other center
displacements. It has no hierarchy, functional grouping or projected clearance
between broad parts. Parts with similar axial centers receive similar offsets.
examples/render_adaptive_gallery.py replays these saved values unchanged.

The older examples/om10_scene.py::authored_layout does contain name-based tiers,
but the native caller never calls it. Merely adjusting its constants cannot fix
the actual output. It is useful historical presentation code, not selected logic.

The original source graph contains107 distinct definitions,166 leaf occurrences
and31 assembly groups. Twenty top-level non-leaf groups explicitly name the
mainplate, winding mechanism, barrel, train mobiles, bridges, escapement and
balance. Original labels identify the remaining direct parts and fasteners.
Source inventory was read on fleet without evaluating/remeshing CAD. The
existing OM10 technical sheet distinguishes the train, setting work and
regulator; the acquired ETA6497-1 service drawing p12 visibly separates the
mainplate, wheels, bridge and screws along their assembly axes. Neither is a
license to invent an OM10 disassembly order.

## Intended visual result

One large technical illustration shows the actual movement's supporting plates,
train, power source, winding/setting work, escapement and regulator as legible
modules. The mainplate is the visual datum. Train wheels/pinions align to their
original pivot positions; bridges float well clear above them. Barrel internals
and balance/spiral appear in separate source-linked lanes, not behind broad
plates. Calendar-side plates and wheels occupy an opposite-side layer/lane.
Original proper rotations remain unchanged; each166 occurrence receives exactly
one authored world translation, including zero for the chosen datum.

Use a clear hierarchy: about8 prominent module labels, selected significant-part
labels adjacent to visible pixels, and a complete166-row original-name legend
outside the raw image. Tiny attached jewels/posts can remain with their source
parent group. Do not pretend a projected hidden center is a visible anchor;
record such rows explicitly and omit misleading leaders. The complete source
inventory remains displayed and listed even when an attachment is obscured by
its parent. All major named mechanisms must actually be visible.

Proposed module composition, subject to the source table and preview:

| Presentation module | Actual source content | Separation policy |
|---|---|---|
| Datum/support | Platine garnie, its nested shock/jewel/post groups | Mainplate fixed; keep pressed-in source attachments together |
| Time train | Grande moyenne, moyenne, seconde, échappement mobiles | Keep each original wheel/pinion mobile together; give mobiles distinct axial lifts and pivot guide lines |
| Bridges | Pont de rouage, pont de barillet, their jewels/posts | Lift each broad bridge clear of its supported train/barrel; retain coherent nested garniture |
| Power/winding ratchet | Tambour, spring, arbor, cover, rochet, cliquet | One side lane; separate cover/spring/drum/arbor axially within it |
| Winding/setting | Bloc de mécanisme de tige manuel, cover, stem, sliding/crown pinions and levers | Opposite lower side lane; plate, mechanism and cover at distinct levels; stem follows its own source axis |
| Escapement | Ancre garnie and pont d'ancre | Separate fork and bridge locally; retain source palette positions |
| Regulator | Balancier spiral réglé, coq, shock/regulating groups | Side lane; wheel/staff and spiral visibly distinct; cock lifted above them |
| Dial/calendar side | Chaussée/minuterie/hour wheel, date driver/idlers/jumpers/disc/plate | Opposite mainplate side; broad date plate/disc separated rather than covering the center |

This table is an authored explanatory grouping, not an inferred contact graph.
The explicit occurrence table resolves each166 source usage path once. Root-level
fasteners without an encoded parent require an authored association recorded in
that table; do not infer mechanical fastening from nearest centers. Keep repeated
usages distinct. Never use a generic default tier for an unassigned row.

## Layout rule and camera

Use the movement's source mainplate/thin-axis frame as the common axial frame,
recorded explicitly. Module translations can include lateral components to avoid
one enormous stack shrinking every wheel. Within each module, source in-plane
pivot positions stay aligned; only the listed internal leaf translations differ.
All guide lines derive from source positions and authored displacement endpoints.
Straight axial lines indicate common-axis alignment; doglegs mark a module's
lateral presentation displacement. They do not claim an interference-free path.

The initial camera is a moderately low oblique view, approximately30–40° above
the plate, with the axial direction rising in screen space. Fit once to the
complete translated source geometry, reserving label lanes. Do not preserve the
old camera if it hides the intended layers. Choose the actual framing from a
single fleet preview, not an unbounded aesthetic parameter search.

Pack module projected bounds with visible gaps before the preview. This is a
presentation clearance rule over actual stored mesh bounds, not a collision or
removability solver. Primary objective: show the mainplate, both broad bridges,
each major train mobile, barrel internals, winding plate/mechanism/cover, fork,
balance wheel and spiral. If these still overlap, adjust explicit module/layer
translations; do not hide parts, thin plates, rotate source geometry or invent
replacement meshes.

Target raw image3840×4000 with2× supersampling (7680×8000 internal, within the
existing8192 limit). Add typography/complete legend outside those pixels in an
SVG poster, around3840×5200 if needed. Keep geometry clear of panels. Use modern
legible type, restrained axial dashed guides, distinct section headings and
subtle consistent module colors in annotation. Preserve the original part
materials unless a separately disclosed illustrative palette is selected.

## Concrete data and existing execution seam

No native mesher/caller/CMake change is needed for the first delivery. Extend the
existing standalone scene-replay presentation tools on the selected feature
branch. The source snapshot and completed scene are immutable inputs.

- explosion-layout.tsv: every original occurrence, complete usage path, original
  product/definition/name, source group, authored module, parent role, translation
  xyz, explanation and stable annotation anchor role. No JSON persistence.
- diagram.bgscene: a new copy of the admitted source-profile1 scene. Only the
  three explosion doubles per instance differ. Definition IDs, instance IDs,
  original placement matrices, materials, geometry, normals, controls, triangles
  and all other bytes remain identical. Native CPU/P100 replay uses the existing
  source-profile1/API2 display binary/module.
- views.bgview: explicit assembled and new exploded cameras. Alpha0 retains the
  original assembled positions; alpha1 applies exactly the authored translations.
- labels.tsv remains the complete original166-row name/usage word; layout and
  guide tables supplement it. Keep original occurrence IDs separate from display
  label ordinals.
- Existing external NVIDIA assembly writer consumes the SAME source occurrence
  translation word and view bytes when its complete reference pipeline is ready.
  It still uses genuine SDK geometry; incomplete NVIDIA inputs cannot yield a
  complete comparison image. Root owns that pipeline integration/schedule.

The scene-copy writer checks complete166-row unique source usage identity before
writing and compares bytes excluding the precise explosion fields afterward.
This directly establishes the presentation-only claim; it is not a new geometry
qualifier. The source graph used for names/hierarchy must match the admitted scene
and original digest. No fresh CAD import, geometry cache or private coordinate
publishing service is introduced.

## Finite execution and direct acceptance

After parent selects the feature branch and approves this plan:

1. Write the explicit source occurrence/module table and replay adapter; retain
   source and input-scene hashes. A direct small authored nested assembly checks
   exact complete mapping, repeated occurrence identity and translation-only
   bytes. Missing/duplicate/unknown usage paths refuse before output.
2. Run one fleet preview of the complete actual scene on CPU, using the existing
   renderer. Inspect geometry and actual instance-ID buffers. Record which major
   parts/modules have visible pixels and whether leader anchors use those pixels.
   Fix a concrete layout fault if found; no meshing or qualification campaign.
3. Run final matched CPU12/P1001 display replays serially from identical scene,
   translation manifest and camera bytes. Preserve all166 instances and original
   source counts. Match the NVIDIA view later through the same manifest, without
   changing this layout to favor a different mesh.
4. Deliver assembled datum inset if useful, large exploded PNG/SVG, full legend,
   layout/guide tables, native stage tables and actual additional display/export
   process walls. Original875/872s cold whole-model times remain distinct from
   these new replay costs. Record SVG/PNG/raster/download/export scopes honestly.
5. Actual image review must show separated major plates, bridges, wheels, winding,
   barrel, escapement and balance/spiral. An inventory-only PASS is insufficient
   for the user's visual request. Label the layout as an authored explanatory
   explosion, not validated disassembly, maintenance or manufacturing instructions.

Ownership: this unit owns the eventual explicit layout/replay script and focused
presentation test/docs only. Root retains native model caller, meshing, CMake,
reference pipeline and public report. No implementation/render/commit is selected
on the current reference-repair branch.

## Delivered interface

Implemented `examples/om10_explosion.py` and
`examples/render_om10_exploded.py`; direct transport properties are in
`tests/test_om10_explosion.py`. Final manifests are named `explosion.tsv` and
contain166 canonical occurrence rows with columns:
`occurrence_id`, `display_label`, `source_usage_path`, `product_id`,
`definition_id`, `source_group`, `module`, `role`, `world_offset_x_mm`,
`world_offset_y_mm`, `world_offset_z_mm`, `reason`.

`views.bgview` contains the exact assembled/exploded frames. NVIDIA replay must
consume that same source-usage translation word and view bytes with its genuine
geometry. `diagram.bgscene` preserves every non-explosion input byte. All20
major parts are actually visible in the final matched CPU/P100 ID buffers;
150of166 occurrences have visible pixels. All166 remain in source geometry,
manifest and full original-name legend. The final poster is3840×5871 around an
unchanged3840×4000 native raster,2× supersampled. Actual scoped timings and
limitations are recorded in the unit journal; no new meshing time is claimed.

## NVIDIA external-mesh replay continuation

Add a separate reference replay entry, without altering the final owned images.
It reads the completed NVIDIA `scene.bgrefmesh`/metadata/BOM/definition reports,
the original native model and the final CPU diagram bundle. Require the same
original source hash and complete107/9162/166 inventory, unique complete source
usage/definition word, exact original12-double placement matrices and copied
166 labels. Reference qualification status remains a separate reported scope;
this adapter cannot turn incomplete definitions into a full diagram.

`BGREFM01` uses header64, definition40, instance136, vertex56, triangle24 and
material32. Write a new reference scene changing only instance explosion3d at
offset112, preserve all other bytes, and compare every disjoint unchanged slice.
An authored two-definition/three-occurrence fixture checks exact preservation,
rotated/repeated instance identity, and wrong placement/ID refusal. No native
build or alternate NVIDIA triangulation is introduced.

Copy the final `explosion.tsv`, `views.bgview` and labels exactly. Replay through
the existing CPU12 raw-reference display, whose ID files are little-endian u64
like the native-source display. Its real stages are `raw_instance_transform_project`,
`triangle_setup`, `tile_count_scan_allocate_fill`, `visibility_shading_resolve`,
`diagnostic_host_copy`; sum only these disjoint frame stages. Do not fabricate
CUDA/RSS/stage values. Retain actual process wall and all native stages.

Reuse actual-ID annotations/full legend and fixed diagram framing. Generalize
poster provenance to explicitly say NVIDIA CPU reference mesh / BRepGPU CPU12
display. Original source→both-image timing is read from the selected completed
row of `reference-outcomes.tsv`, with scope/status shown; new replay/PNG/SVG costs
remain separate. No full-watch replay before parent provides a completed dataset
and selected outcome. This remains genuine NVIDIA geometry; byte preservation
and source inventory admission are not a new whole-source geometry qualifier.
