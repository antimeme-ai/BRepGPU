# Finite original-boundary / NVIDIA triangle queries

Implementation note for the [accepted concrete appendix](../docs/OM10_NVIDIA_REFERENCE_PLAN.md#finite-geometry-helper-concrete-reviewed-boundary),2026-10-15.
This is one reference-only numerical admission unit. It never replaces source
or triangles, never renders, and supplies no universal Hausdorff certificate.

## Read sources and direct derivation

Read pinned OCCT8 BRepExtrema_DistShapeShape.hxx/.cxx: the default deflection is
Precision::Confusion; SetDeflection sets myEps, and Perform explicitly invokes
SolidTreatment for SOLID/COMPSOLID roots. Build a FACE compound of the resident
original solid instead. Load that compound into shape2 once; replace only the
point vertex in shape1 per query. Set deflection to2^-40mm explicitly. Do not
substitute untrimmed surfaces or filled solids on solver failure. A face trim
failure remains a failed numerical query, relevant to the separately reviewed
source/pcurve policy.

Acquired and read David Eberly, [Distance Between Point and Triangle in3D](https://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf),
revised2020-09-11, mathematical formulation/algorithm sections. It formulates
the squared distance as a convex quadratic on barycentric coordinates: its
minimum is the stationary interior point or a boundary point. The three boundary
intervals reduce to clamped segment projections. This supports a short owned
enumeration rather than importing a region-case implementation. No dependency
or reference source code is adopted.

Own derivation: for triangle a,b,c, let e=b-a,f=c-a,w=p-a,n=e×f.
The unconstrained projection's coefficients are
u=((w×f)·n)/(n·n), v=((e×w)·n)/(n·n). If u,v≥0 and u+v≤1,
the point with barycentrics(1-u-v,u,v) is an interior candidate. For each of
the three edges x→y, clamp t=((p-x)·(y-x))/|y-x|² to[0,1], giving its nearest
segment point. Compare actual squared Euclidean distances of these feasible
candidates; the smallest is the nearest triangle point. Across triangles, ties
retain the lowest emitted triangle index by ascending scan. This covers edge
and vertex minima explicitly, including a point projected outside the triangle.

Use owned scalar long-double arithmetic for these calculations, reporting its
precision in metadata. Do not compute a cancellation-prone Gram determinant
as the difference of two nearly equal products; use n·n. Reject nonfinite
intermediates/results and nonpositive denominators, rather than fabricate a
nearest point. Binary64 input and numerical output remain approximate; fixed
2^-32mm grade accompanies the two prescribed distance gates. No arbitrary
epsilon expands the barycentric domain. Literal interior/edge/vertex cases and
the reconstructed nearest point directly check the routine.

The intact241,821-byte acquired PDF is papers/pdfs/eberly-distance-point-triangle.pdf,
SHA-2563d1d9eb86abd73a109ad29587d6f10ca6bd215a6d5dba5bc2ecd13546ef58ae6.
It is ignored literature under its CC BY4.0 notice. Restore with
`curl -fL https://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf -o papers/pdfs/eberly-distance-point-triangle.pdf`.
The existing quarantine manifest restores the pinned OCCT source and prefix.

## Concrete owned boundary

New native/om10_reference_geometry links only existing authorized OCCT8 and the
unchanged raw mesh decoder. CLI is exactly the accepted appendix. Require one
definition-local raw definition, no instances and at least one triangle; require
positive finite tolerance with finite derived limits. Read the original single
identity-located FORWARD solid once, reject existing face triangulations/edge
polygons, and build its full trimmed boundary compound. No source meshing.

BGRPROB1 header16 is magic8/u64count; positive bounded count times40 must exactly
match the file. Each row carries source face ID,qualified-owned triangle index
and finite XYZ3. These IDs are witness provenance labels; the helper does not
invent an unsupported mapping back from OCCT traversal to original STEP IDs.

Forward selection/probes are exactly the accepted per-NVface middle union64
global schedule. Every failed solver result is retained with blank distance and
explicit failure; threshold failures retain actual measured distance. Reverse
brute search consumes every provided actual source witness. Finite failure
exits nonzero; no successful summary is written on exception/partial execution.
No BVH, adaptive sampling, hidden tolerances or second quality mechanism.

## Reports and timing

summary.tsv uses field/value rows: status,original_faces,selected_nv_triangles,
forward_queries,forward_max_mm,forward_failures,reverse_queries,reverse_max_mm,
reverse_failures,tolerance_mm,numerical_grade_mm,solver_deflection_mm,
forward_limit_mm,reverse_limit_mm,source_read_seconds,forward_seconds,
reverse_seconds. metadata.tsv additionally names OCCT boundary/numerical scope
and actual reverse long-double precision. forward.tsv records NVface/triangle,
probe name,XYZ,distance and status. reverse.tsv records original face/owned
triangle,XYZ,closest NVtriangle,barycentrics,closest XYZ,distance and status.
These are direct result records; independent shell/source coverage remains the
other owner's existing mechanism. Query walls are qualification costs outside
the benchmark and must not be added to fresh complete image totals.

## Bounded red/green/review sequence

Authored unmeshed sphere and planar triangle fixtures first fail against absent
entry. Sphere-center forward distance must be its radius, exposing a filled-
solid zero shortcut. Literal triangle cases exercise known interior,edge,vertex
closest points/barycentrics, including a shifted/underresolved failed mesh.
An authored annular solid checks a point over its hole stays a positive distance
from the trimmed boundary. Check positive finite case and malformed witness/raw
input/tolerance/output failures. Build only this new target and authored fixture
on the fleet; use focused sanitizer checks if linkage permits. Root/reference
owner code review precedes one root-authorized complex-definition cost pilot;
actual watch queries and rendering stay prohibited until that instruction.
