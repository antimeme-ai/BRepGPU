# Deliver one extrusion-result mesh connection, then stop

Issue54. Both independent design reviews accepted EXTRUSION_MESH_DESIGN. All
geometry/tests/callers run on the fleet; local source/AST/static-image work only.

1. Oracle owner writes independent extrusion_meshing_oracles.qualify_mesh and
   tests/test_extrusion_meshing.py, then records genuine missing-API fleet red.
   Expected data uses original source/profile/height and independently checked
   face/coedge words, never production triangulation or inverse helpers. Cover
   complete exact/rounded domains, source/UV, shared incidence/vertex links,
   rounded convex-prism embedding, actual error and failed-call preservation.
   A bounded set of real-output corruptions attacks distinct obligations.
2. Parent implements complete extrusion_meshing.tessellate, immutable mesh and
   lazy ExtrusionResult adapter. Allocate actual result vertices once; directed
   face-loop fans preserve winding once. Full exact affine UV inversion; actual
   float ratios and global profile support guard rounded shell. No source
   approximation/refinement or widening other API guards. Run direct green.
3. Semantic colleague reviews actual implementation and independent checks in
   the existing continuing review. Fix real findings and rerun affected cases.
   Run existing test_extrusion/test_extrusion_construction/test_extrusion_sampling
   if present and canonical/cut meshing compatibility once, with qualified native
   paths where those existing tests require them. No broad GPU/numerical reruns.
4. Parent implements an actual examples/mesh_extrusion.py caller/cost mode, reusing
   existing indexed mesh/export helpers. Export original profile/result/control/
   history, mesh PLY and SVG. The generic SVG gets only a truthful optional footer
   for a feature with no periodic seam; preserve canonical/cut defaults. Consume
   actual typed source/history alongside indexed buffers inside the useful wall.
   Use triangle/chamfer/16-vertex recipes with original heights/Frames. One first
   output plus three serial repeated fresh meshes each; qualify first output,
   compare subsequent complete output/reader values outside timed work. Measure
   source preparation/feature construction, mesh, indexed/history reading, direct
   useful wall, independent checking and export separately. No speedup claim.
5. Review saved actual outputs and view the real SVG/PNG. Record scoped results
   and relevant passing/failing tests, update README/API/roadmap/issue/journal.
   A second broad testing/profiling/mutation campaign is not a completion step.
   Stop once direct obligations, compatibility, callers and finding-driven
   reviews pass. Publish coherent commits/PR, merge preserving commits/branch,
   sync clean fleet/main and record handoff. External alpha debt stays open.
