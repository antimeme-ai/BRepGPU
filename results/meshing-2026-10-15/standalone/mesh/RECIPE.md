# Complete canonical CPU triangle mesh

plate: 136 shared vertices, 272 oriented triangles, 16 subdivisions per original circle quarter.

Requested world tolerance 1/1024; curve bound 1/2048, actual rounded-vertex L1 bound 121/6070852297695428608, total 2964283348484217/6070852297695428608. Exact vertex coordinates and corner UVs remain rational. One indexed rounded coordinate per global vertex supplies PLY and both SVG views. PLY normals are geometric facet normals, not smoothed source normals. Top view draws actual top-face triangles; the second view draws every face as a wireframe without hidden-line removal.

Circle chords approximate the analytic trim: disk caps omit a narrow rim and plate caps include a narrow strip inside the analytic hole. The error bound covers these domains; not every triangle point is on the exact analytic trim. A plate retains its through-hole. No GPU/native mesher, Boolean, imported-body or general-triangulation claim applies.

body-tables/ preserves original packed factory records; original-*.tsv names original vertices/edges/coedges/loops/faces/charts/maps/control bits. corners.tsv uses chart-local UV; boundary-uses.tsv uses original maps, including distinct seam atlas0/4 occurrences. Source IDs are typed within the retained process-local body/revision namespace.
