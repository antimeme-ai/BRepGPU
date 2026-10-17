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

## First implementation review

The new `raw_mesh.hpp/.cpp` production seam matches the selected capability:
exact any-axis rectangle preflight, finite physical positions/normals, source
label detachment only for split candidates, SDK-only diagonals, full physical
identity/XYZ comparison, parent-local original corner normals, source coverage,
child counts and exact directed boundary/diagonal cancellation. Metadata tags
are restored after a retained ancestry map has been formed. Parent snapshots
outlive the RAII restoration guard. Caller retains ownership and destroys a
failed PolyBrep; no partial Mesh is returned on exceptions.

One defensive finding was sent to owner/root: `ring()` checks the current edge
before adjacency access but can dereference the next edge through
`GetEndPolyVertex()`/the next entry before that entry's own null check. Validate
all edge pointers, owners and start vertices first, then inspect CW/CCW adjacency
and derived end points. This is a local guard correction, not a new test campaign.
The owner is also strengthening the same fixture with extraction counters and a
natural SDK noncompletion after an earlier successful split, to exercise parent
metadata restoration on partial mutation. Final code acceptance is pending those
bounded changes and the owner's focused fleet checks.

## Final static code acceptance

Re-read the corrected two-pass `ring()` validation: all edge pointers, original
owners and start vertices are established before any next/CW/derived endpoint
access. Finding resolved. Reviewed fixture additions for explicit official split
counts and natural noncompletion after an earlier completed SDK split; requiring
that earlier parent's two children and retained sibling metadata prevents an
early preflight rejection from satisfying the failure-path oracle.

Final static CODE ACCEPT for the shared extraction helper and producer/driver
integration. All admitted diagonals come from official `TriangulatePlanar`;
physical key/XYZ identity, parent-local original normal bytes, directed original
boundary, diagonal cancellation, whole source-face coverage and restored source
labels are checked directly. No source Brep lifetime/ownership transfers occur.
No further blocking findings. Focused fleet GREEN and bounded sanitizer outcomes
remain owner/root execution work; this review performs no native execution and
does not establish complete actual 1808 or whole-watch admission.

## Final fixture linkage and instrumentation scope

Reviewed the final CMake correction: the literal fixture links the shared
extraction helper plus SM_API/SMLib, the APIs it actually calls. The main producer
retains its original bridge/USD dependency ordering. Removing unused fixture
OpenUSD linkage isolates the selected test boundary; no product dependency or
SDK code changed.

Owner reports the three focused methods GREEN in 0.044s and five affected
producer methods GREEN in 1.385s. The natural failure was `official split left a
nontriangle`; direct fixture assertions observe the earlier completed split and
restored original/new children. Final bounded instrumentation reports the same
three methods GREEN in 0.198s with AddressSanitizer and UndefinedBehaviorSanitizer
excluding vptr, LeakSanitizer enabled and no suppressions. The initial full-UBSan
failure and intermediate unused-OpenUSD startup allocation logs remain retained.
The vptr exclusion is specifically required by the official SDK's template
reinterpretation between SmTArray specializations, not a claim of complete
foreign-runtime UB coverage. This lane reviewed source/linkage but did not rerun
those native checks. Static CODE ACCEPT stands with that precise execution scope.
