# Actual watch consumer and display subplan

2026-10-15. Read shared/repository doctrine, primary horology sources, actual
extrusion/mesh/consumer/display code. Saved source study and no-blocker design
review in papers. Reviewed WATCH_CHAIN_PLAN from actual consumer/display
perspective: accepted; no unresolved finding. Implementation waits for root go.

Own `examples/watch_scene.py`, `examples/watch_display.py`, and
`examples/render_watch.py`. Geometry sources remain owned convex profiles and
their actual feature results; pierced parts consume actual BoreResult meshes.
Separate radial lugs/spokes remain explicit separate assembly parts, not a
fused analytic gear or physical mechanism. Real thickness/scale is preserved.

Construct a distinctive asymmetric, deliberately layered openwork movement:
dark structural rim, brass barrel and train, silver/blue pierced bridge parts,
ruby-colored collars, and a large balance-like region. Camera framing must
make geometry occupy the image rather than hiding it behind documentation.
Render actual indexed triangles with depth tests, facet normals and material
lighting; SVG only supplies presentation background/type/labels. Produce hero
and near-top view. No dependency or image generation.

Actual caller will independently qualify every constructed mesh using the
appropriate existing/new direct oracle, retain profile/feature/cutter/history
objects, export per-part source-rich records and PLY, and aggregate actual
indices/materials in an explicit manifest. If templates are reused, give each
instance a separate display ID/translation and preserve source identity.
Prefer direct geometry placement in supported source Frames to avoid an extra
instance-transform correctness burden.

Check display indices, finite positions, strict nonzero triangle area and
complete part/triangle inventory. This is display input validation, not a
geometry certificate. Actual source/history useful consumption, construction,
meshing, independent checks, export and CPU rendering get separate timings.
Read the saved images and records; fix actual output/code defects, then rerun
affected work only. Root reviews actual output and source before publication.
Stop at these direct obligations; no broad benchmark/certification campaign.

## Implementation checkpoint

Root accepted all design/plan reviews and reported genuine missing-API red
before implementation. Added the three agreed files. Every part uses its own
actual original source Frame and namespace; this avoids a second instance
transformation layer. Radial lugs remain individually named closed solids.
The scene includes an off-center bore in each long convex bridge, a large
central opening in the blue chassis, a barrel/train and open balance assembly.

Root source review caught short-cutter equality in several initial recipes:
scale.25 equals height.25×feature scale1 and would not cut fully through.
Changed those actual cutter recipes to scale.5, still strictly inside their
convex profiles. No admission widening or tolerance repair. API owner and
oracle owner confirmed result/history/qualifier fields directly.

Caller uses world tolerance1/256 and max quarter segments64, checks actual
bindings, reads ordered immediate/extrusion path objects to original records,
then independently qualifies each actual produced mesh once. Exports per-part
original profile, cutter packed/source tables, actual topology/controls,
immediate/composed histories, mesh leaves/UVs, PLY and inventory. Aggregate PLY
retains part and source-face IDs. Source hashes span actual Python inputs.

Display reads actual rounded indexed positions and geometric facet normals,
performs opaque CPU depth tests, writes transparent geometry PNG and SVG
presentation, and records camera basis/material/light parameters. Oblique hero
and near-top views use unchanged source coordinates and thickness. All three
sources AST-parse locally; no product/test/render execution has been performed
by this owner. Fleet execution remains serialized under root coordination.
