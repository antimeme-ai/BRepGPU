# NVIDIA polygon continuation: independent review

Review basis: the polygon export study/subplan and official SMLib `SmPoly.h`,
`SmPolyFace::TriangulatePlanar`, `MakeManifoldEdge`, `SetOriginalFace` and
`SmPolyEdge::IsConvexCorner` from the acquired USD/BRep source. No production
edits or native execution in this review lane.

The narrow capability is appropriate: admit existing triangles and exact
axis-extruded four-corner rectangles; invoke only the official SDK planar split;
preserve SDK physical vertices, positions, parent source-face labels and actual
parent corner normals. Crossed spring/spiral rings remain failures. This adds no
owned triangulator or vertex repair and cannot establish a complete watch.

Primary code confirms that `IsConvexCorner` uses OriginalFace surface derivatives
without consulting GetOKBackPtrs, so temporarily clearing that label is needed
for raw XYZ. `SetOriginalFace(..., FALSE)` changes only the label. The split copies
both public user ULONG fields and the parent OriginalFace label into children;
new diagonals need parent+vertex normal recovery because outgoing edge AuxData
is not populated by this split. Re-enumerating the entire PolyBrep is necessary
because the original polygon survives as one of the triangles.

Design/plan refinements sent to owner and root before final acceptance:

* Exact rectangle admission must include finite XYZ, distinct physical vertices,
  nonzero extrusion and cross-section, and cyclic alternating extrusion/end
  edges. Coordinate multiset matching alone cannot establish a valid ring.
* Any SDK failure or exception after partial topology mutation rejects the whole
  definition and discards that PolyBrep without publication or reuse. On a valid
  re-enumeration restore original parent labels/user fields on all children;
  unknown or missing ancestry is a hard error, never inferred from XYZ/labels.
* The small authored extraction fixture should include an untouched sibling
  triangle sharing a physical vertex with a different parent corner normal and
  different original user fields. This directly catches a mistaken global-vertex
  normal map, parent contamination and incomplete metadata restoration. Literal
  distinct corner normals must be preserved exactly rather than recomputed or
  normalized. Rotated/reversed rings and unsupported crossed/nonrectangular
  negatives remain part of this same bounded fixture.

The parent selects the extraction fixture seam. Revised design/subplan review,
then genuine RED, focused GREEN and bounded code review precede actual 1808
attempts under both unchanged policies. Existing whole owned qualification stays
pinned and unrelated; no test campaign or target rendering is requested here.

## Revised design and subplan acceptance

Final independent DESIGN and SUBPLAN ACCEPT after the owner amended both files.
The exact cyclic any-axis equalities now establish finite, nonzero perpendicular
sides with four distinct physical vertices, and reject a bow-tie coordinate
multiset. The parent selected `raw_mesh.hpp/.cpp` as the one production/fixture
seam. A retained final polygon-to-parent map survives temporary user-field
restoration. RAII restores valid tagged labels/user data; unknown ancestry,
failed or throwing official mutation rejects the complete definition and the
caller destroys the PolyBrep without reuse or publication. The literal sibling
triangle with shared vertex and different original normal/user bytes closes the
per-parent fidelity gap in the fixture plan. All review findings are addressed.

This accepts the bounded design and implementation plan, not implementation or
actual date-disc output. Root authorization, genuine fleet RED, focused GREEN
and independent code review remain the next steps. No new dependency, tolerance,
healing policy, ring repair or whole-watch PASS follows from this acceptance.
