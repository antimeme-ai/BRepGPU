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
