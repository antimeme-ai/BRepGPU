# OM10 mesher diagnosis: what the NVIDIA comparison establishes

Diagnosis only, at the operator's explicit stop-before-proceeding instruction.
This study reads retained outputs and source; it runs no mesher, benchmark,
distance queries or renderer and changes no implementation or quality setting.

The owned mesher has a serious absolute-time problem. Existing observations
place substantial work in global sample preparation and a quadratic sampled-UV
boundary scan, before the GPU consumer can run. NVIDIA's real CPU path handles
large admitted parts quickly. Both facts matter. A whole-watch speedup ratio
would still be false: the owned run is incomplete, the reference excludes four
definitions, and the exact current owned hard part is one of those exclusions.

## Actual comparable facts, with scopes kept separate

The fresh reference attempt uses the original STEP, corrected no-processing
source export, 12 independent workers on 12 physical cores of the second socket,
one inner thread each and CUDA hidden. Owned construction remains one Python
thread on the first socket. Both reference policies attempt all 107 definitions;
103 produce complete meshes and pass the existing physical/original-Euler checks.
Those outputs cover 9,023 original faces, rather than all 9,162. The four missing
definitions contain 139 original faces: 1716, 1756, 1797 and 1811.

| Existing observation | heal=0 | heal=1 |
| --- | ---: | ---: |
| Shared fresh STEP export wall | 4.244974 s | Same shared export |
| Through all 12 worker exits | 7.232926 s | 75.420416 s |
| Separate physical-check wall | 17.731499 s | 17.508186 s |
| Complete physical meshes | 103 | 103 |
| Physical vertices | 106,226 | 106,226 |
| Triangles | 212,832 | 212,832 |
| Serialized definition mesh bytes | 40,877,752 | 40,877,752 |

These are distinct phase observations, not an observed STEP-to-images wall.
Source decode, orchestration, physical checks and later finite source-distance
admission have different scopes. No reference image or complete watch admission
is implied. Equal counts across policies do not mean equal meshes: 68 retained
definition hashes match exactly and 35 differ; the hash comparison does not
isolate coordinate, normal or ordering differences.

The native stage rows allow further attribution without rerunning geometry.
The following sums are sums of worker-local wall intervals, not CPU-time
counters or elapsed parallel batch times. Nested rows are not additive.

| Stage over the 103 admitted definitions | heal=0 summed worker wall | heal=1 summed worker wall |
| --- | ---: | ---: |
| Official OCCT-neutral→USD conversion | 1.548237 s | 1.935539 s |
| USD BRep import/healing | 1.255746 s | 14.843363 s |
| `SmApiTessellate` | 21.477637 s | 20.376390 s |
| Extraction/physical-identity/parent-boundary checks | 0.854010 s | 0.894788 s |
| Binary mesh write | 0.461194 s | 0.464320 s |
| Complete definition through object teardown | 25.839612 s | 38.812821 s |

Across all 105 definitions reaching tessellation, its summed stage wall is
27.531694/20.822404 s. Median per-definition tessellation observations are
0.080596/0.082029 s. Native-default healing's large batch wall is dominated by
the manual barrel spring, definition 1756: import/healing alone takes
71.685816 s, and that definition still fails export. Its complete job takes
72.033524 s. Heal=0's longest job is the spiral, definition 1797: 5.614461 s
tessellation and 5.628511 s through teardown, also an export failure.

The two official bridge failures occur before SMLib import/healing. The two
spring/spiral failures occur because actual SDK polygon walks are not admitted
by the reference exporter; retained direct inspection finds proper projected
crossings. No successful native status, manifold flag or timing supplies the
missing accepted geometry.

## Where the owned run actually is

The saved live-frame observation identifies source face 167661, zero-based face
index 2688, in definition 1756, the manual barrel spring. Its carrier is a cubic
tensor surface with 102 spans; one outer loop contains four original coedges,
including two 102-span cubic long edges and two lines. It has 8,300 sampled
boundary nodes, not 8,300 original topological edges. The preceding source face
167660 is a plane bounded by 102/103-span cubic spring curves and two lines.

The current reference's 103 accepted meshes specifically exclude this spring.
Its small, unsuccessful native polygon result cannot serve as the performance
denominator for the owned production task that is preserving that boundary.
The saved frame's old `indices`/`next_tri` locals belong to a previous face;
their roughly 907k accumulated-index hint is neither an exact triangle count
nor a matched output-density measurement. Do not divide it by 212,832 and call
that a tessellation-density ratio.

The retained owned clocks give `sampled-begin = 5232.434638 s`, or 87 min
12.435 s, before the face phase. That stage includes source validation, shared
edge schedules and global circular-joint boundary checks. It is not a measured
cost for one part or for spatial-curve evaluation alone. The current per-face
scan is another construction-time check, before `qualify_mesh`, native CPU/P100
evaluation and any rendering. The P100 cannot accelerate a stage it has not
been asked to execute.
The source translation/decode/original geometry/assembly checks preceding
meshing total about 56.6 s; they do not explain the hours spent inside construction.

## Algorithm and quality differences that can explain the gap

The owned boundary checker enumerates pairs of sampled segments. For an
8,300-edge simple ring, there are up to 34,432,550 nonadjacent candidate pairs.
It already has an AABB rejection inside `_segments_touch`; proposing that
existing test again would not change the algorithm. Pair enumeration itself
remains quadratic, and exact `Fraction` coordinates can make even bounding-box
comparisons expensive. The global circle-schedule pass uses the same mechanism.
This is a concrete algorithmic cost, alongside interpreter/object overhead;
neither should be collapsed into the statement "Python is slow."
The strict ear loop would also make 68,865,100 successful candidate-slot visits
for an 8,300-node input under the derived successful-clip model. Both work
counts are arithmetic consequences of the current loops, not measured counters;
the current face has not yet reached that ear phase in the saved observation.

Owned interior construction additionally bridges holes, performs occurrence
ear clipping, may improve connectivity with Lawson flips, and refines conforming
triangles against conservative carrier bounds. Exact predicate fallbacks and
source/occurrence maps are part of that production construction. They are not
the later independent mesh qualifier being accidentally timed repeatedly.

The quality knobs also differ materially. At requested 0.005 mm, the owned code
allocates at most 0.000625 mm to its edge/trim bound and at most 0.00375 mm to
surface interpolation plus canonical-vertex displacement, separately bounds
native UV rounding, and rejects an excessive combined budget. Its edge process
retains original knots and shares one schedule between all incident uses. Its
surface process uses derivative/control-net bounds. Those conservative choices
can increase boundary and interior density; current data do not apportion the
cost among density, bounds, exact topology and object/predicate execution.

NVIDIA receives 0.005 mm chord height directly, with both angle controls,
maximum edge length and aspect ratio disabled. Its native implementation uses
specialized cached geometry, curvature/Hermite step estimates, control-net
surface subdivision and spatial vertex trees. This is a different algorithm,
not a C++ transcription of the owned Python loops. Official conversion and
healing can also change representation before tessellation; equal requested
tolerance is not equal geometry or an equal accepted error contract.

The existing radius-10 sphere witnesses already show this runtime returning
successful tessellations that exceed requested chord height on those fixtures.
That finding prevents treating status=SUCCESS as the watch's quality oracle;
it does not prove that any of the 103 admitted watch meshes violates 0.005 mm.
Their finite original-boundary comparison remains outstanding.

## Smallest useful matched comparison, after operator authorization

Use definition 1713, the original main plate: 438 source faces, accepted under
both native policies, with 11,961 physical vertices and 24,078 triangles in
the retained reference outputs. It is a substantial common-success case and
precedes the failed spring in the owned traversal. Definition 1753, the barrel,
is a useful later cross-check with 557 faces and 11,776 reference triangles;
do not open that second case before the first resolves the comparison.

The first new experiment should be one isolated definition on one physical CPU
core per implementation, all inner threads one, CUDA hidden, both declared
native policies retained. Use unchanged original geometry/IDs and all 438 faces,
unchanged 0.005/0.001 mm owned settings and unchanged strict native controls.
No fastest-policy selection, omitted hard faces or triangle-count matching.

Time from the prepared, complete definition to an accepted complete boundary
mesh. Declare input representation/decode and official import/healing separately.
Also report a common display-ready output scope containing XYZ, oriented corner
normals and triangles: the native SDK generates normals during tessellation,
whereas the owned path evaluates carrier normals in its later CPU/GPU consumer.
That owned CPU work must be included in that scope. Source maps and other
additional output remain explicit rather than being silently charged to an
ostensibly identical primitive.

Before a ratio, require complete original inventory, physical closure/Euler,
and the existing independent original-boundary acceptance at the declared
grade for both actual outputs. Report vertex/triangle counts, residual outcomes,
and construction versus independent-check walls separately. A same-core diagnostic
can isolate execution/density issues; any claim about the 12-worker whole
pipeline needs a separately matched complete workload. A failure is a capability
outcome, not a denominator. A finite check also remains a finite check, not a
new universal geometric guarantee.

There is currently no public single-definition selector in the owned source/
meshing caller. Do not pretend that its whole-source preparation cost can be
allocated by face count or reused as a measured isolated-part timer. A future
isolated-input experiment must preserve the full original definition closure
and make that scope explicit. This note authorizes neither that implementation
nor another run. It stops at diagnosis and the proposed discriminating test.

Sources read: retained `context/om10-parallel-nvidia-geometry-v2` reports and
pins; retained owned V4 stack/locals; [owned mesher](../python/brepgpu/imported_meshing.py),
[whole qualifier](../scripts/qualify_om10.py),
[reference design](2026-10-15-om10-nvidia-cpu-reference-study.md),
[polygon study](2026-10-15-om10-nvidia-polygon-export-study.md),
[sphere witnesses](../docs/TESSELLATION_FINDING.md), and NVIDIA revision
[`c9c5979f04cea8c38213004d4f04df0dcfb40d05`](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05)
`SmApiBrep.cpp`, `SmTess.cpp` and `SmApiTessellationParams.h`.
