# Finite original-boundary / reference triangle queries

This reference-only executable reads one unmeshed original definition, actual
definition-local BGREFM01 triangles and already qualified source witnesses.
It performs numerical finite admission queries, without rendering, meshing,
NVIDIA linkage, fallback triangles or a universal distance certificate.

```sh
cmake -S native/om10_reference_geometry -B build/om10-reference-geometry \
  -DCMAKE_BUILD_TYPE=Release -DOpenCASCADE_DIR=EXISTING_OCCT8_PREFIX/lib/cmake/opencascade
cmake --build build/om10-reference-geometry --parallel 4
build/om10-reference-geometry/brepgpu-om10-reference-geometry \
  --source original-definition.brep --mesh genuine-definition.bgrefmesh \
  --witnesses source-witnesses.bgprobe --tolerance .005 --output-directory NEWDIR
```

Run only on the fleet. Actual source-query pilots require root's instruction;
whole-source qualification and watch rendering retain their separate gates.

The raw mesh must have one definition,zero occurrences and positive triangle
count. BGRPROB1 is header16(magic8/u64count), then40-byte records containing
original source face ID,u64 qualified-owned triangle index and XYZ3f64. Counts,
lengths and finite values are checked before queries; original witness provenance
and complete source/physical-shell coverage belong to the caller's existing
checks. Witnesses never supply display triangles to the NVIDIA baseline.

Read the original identity-located forward solid once. Require no embedded face
triangulations/edge polygons, and a usable CurveOnSurface for every face-edge
use, allowing OCCT's plane-on-demand curves. Missing/nonfinite trim data fails
with SOURCE_QUERY_PREPARATION before any result summary exists. That is a
reference-query capability gap, not a NVIDIA geometry failure. The helper does
not create pcurves, call a broad shape fixer, use untrimmed surfaces, or query
the filled solid. It retains a compound of all original trimmed faces in one
OCCT distance tool with explicit2^-40mm deflection.

Forward probes use the middle emitted triangle per NVface plus the predetermined
64 global schedule, with centroid/three edge midpoints per selected triangle.
The gate is tolerance+2^-32mm. Reverse probes search every actual triangle using
owned scalar long-double interior/three-segment closest points and retain the
lowest triangle ID on equal distance. Their gate is2*tolerance+2^-32mm. See the
[math/source note](../../papers/2026-10-15-om10-reference-geometry-study.md) and
[accepted appendix](../../docs/OM10_NVIDIA_REFERENCE_PLAN.md#finite-geometry-helper-concrete-reviewed-boundary).

Outputs are forward.tsv,reverse.tsv,summary.tsv,metadata.tsv,timings.tsv.
Threshold failures retain actual distances and return nonzero. Failed OCCT
queries have blank distance and SOLVER_FAIL rows; forward_solver_failures counts
them separately. A caller must treat missing summary or solver failures as an
incomplete query capability, not as completed numerical NVIDIA quality failure.
The final summary is published atomically after all other reports close. Existing
destinations are rejected; any partial/exceptional run cannot publish success.

Timing rows enclose actual input/query preparation, forward queries and rows,
reverse searches and rows, and an overlapping complete-through-query-close wall.
Final reports/host destruction/process exit are excluded from that last row.
These are qualification costs outside the fresh image benchmark. Metadata records
actual solver deflection, numerical grade and long-double precision; none is an
exact-distance or global approximation guarantee.

Seven direct authored cases cover sphere interior versus boundary, annular trim,
literal triangle nearest regions/barycentrics, N65 fixed selection/ties, missing
PCurves, malformed inputs and final-report failure. No private watch data is used.
A separate build with BREPGPU_REFERENCE_GEOMETRY_SANITIZERS=ON instruments the
owned helper; the already authorized OCCT reference libraries remain unchanged.
