# Complete actual mesh caller and serial cost unit

Implemented `examples/mesh_body.py` and `benchmarks/meshing/run.py` under the
accepted subplan. The real consumer packs shared rounded XYZ, indexed triangles,
original face IDs and geometric facet normals, computes actual triangle area/
signed-volume and physical-boundary lengths, and traverses every exact source,
corner UV, edge parameter and directed coedge UV/parameter occurrence. Removed
a contrived hex-character checksum during self-review; no manufactured per-
coordinate work remains. Strict edge/coedge zips prevent silently skipping data.

The standalone caller separates body preparation, construction, indexed reading,
complete construction+reader, independent qualification and actual export.
The serial matrix uses the four requested cases, each first/warmup/five repeats.
First output receives independent qualification; six later complete frozen
outputs and consumer buffers are directly compared outside wall with separate
correspondence cost. Full exports retain the FIRST qualified output. All nine
owned source dependencies are pinned before/after the campaign.

Exports include exact/rounded/source vertices, triangles/local chart corners,
original physical edge and directed boundary occurrences, explicit bounds,
quality/reader metrics, original packed body and readable source records/control
bits, triangle PLY with actual geometric facet normals and SVG drawn from the
actual indexed buffers. Top-face triangle view shows the plate hole; a separate
all-face wire view names source face colors and highlights one physical seam.
No point cloud, inferred visibility solver, native/GPU operation or speedup claim.

Both complete files pass static AST parsing without imports. No product,
benchmark, SVG generation or fleet execution ran here; root retains the execution
hold pending direct greens and reviews. No Git operations were performed.
