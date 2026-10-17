# OCCT reference visualization source review

Scoped ACCEPT of docs/occt-watch-reference-unit.md and the actual independent
native/occt_reference producer, render_om10_occt_reference.py caller and focused
test_occt_reference.py. No build, meshing or display execution by this reviewer.

Read the adopted OCCT source BRepLib_ToolTriangulatedShape::ComputeNormals:
source UV NormEstim is tried when available, with triangulation fallback. It
does not itself apply TopoDS face reversal. The producer applies triangulation
location once, corrects the transformed normal pseudovector for a negative
transform, and applies face reversal to normals and triangle roles once.

Using Read and ReadSubs on the SAME BinTools_ShapeSet preserves source TShape
identity. Reverse serialized storage indices must name unique actual root faces
with the original orientation. Every requested original face must have a finite
nonempty actual triangulation. The complete exact face/definition word and PASS
status are checked before display; a failed definition cannot yield a complete
image marker. Source bytes and executable pins are checked, original placements
are retained by the existing assembly/raw transport, and face-local transport
keys are not called sewn source topology.

The existing box/cylinder source equations, oriented normals, original face
word and three placements including rotation provide a direct finite source
check of this adapter. Missing-source and existing-output refusals preserve
attribution. This is an external OCCT CPU reference with independently authored
views; requested 0.005 mm absolute deflection and 0.5 rad angle do not establish
a proved 5 micrometre original-surface bound, owned mesher success, matched-camera
comparison or NVIDIA result. That distinction is explicit in the caller's image
and metadata. No new production dependency or owned numerical evaluator is added.
