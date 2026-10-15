# Boolean-result mesh caller and cost study

The selected delivery must export and consume the actual `CutSolid` retained by
`CutResult`, not a larger-hole canonical plate and not reconstructed packed
factory tables. The existing through-bore has seven faces, fifteen edges, thirty
coedges and ten carrier charts; its original cutter wall retains its own frame
and V[1/4,3/4]. Its graph/history and this parameter ownership must survive export.

Read directly: `examples/mesh_body.py`, `examples/cut_bore.py`,
`benchmarks/meshing/run.py`, `python/brepgpu/boolean.py`,
`python/brepgpu/meshing.py`, `docs/MESHING.md`, `docs/BOOLEANS.md`, and the
canonical meshing result report. No geometry or benchmark was executed locally.

## Actual indexed consumer and exported display

Reuse `mesh_body.read_mesh` unchanged if the new mesh has `.body` equal to the
actual CutSolid and retains the existing mesh-record fields. It reads every exact
vertex/source, triangle/corner, physical edge parameter and directed coedge/UV
occurrence; constructs shared XYZ/index/face-ID buffers and geometric facet
normals; and computes signed volume, triangle area and boundary-length metrics.
That is real indexed rendering/export consumption, not merely record counts.
Its rounded floating-point area/volume measurements are diagnostics, not an
independent geometric oracle. Exact qualification remains a separate operation.

`mesh_body.mesh_svg` needs only a truthful label hook: CutSolid has no `.shape`,
but has the actual frame/faces and a named top face. The SVG must draw every
actual indexed top triangle with an open bore, then all actual triangles in the
wire view. Highlight one physical cropped wall seam, while retaining its two
original atlas occurrences. No hidden-line/occlusion claim; PLY facet normals
are geometric, without smoothing. A small optional title parameter is preferable
to copying renderer code or inventing a canonical shape name.

Split reusable mesh export from canonical source export. `write_source` assumes
factory `.shape` and `.packed_tables`; calling it on a CutSolid is invalid.
The new cut caller should export actual graph records directly, using the
existing recursive TSV record traversal pattern from `cut_bore`. Keep the
canonical caller's existing defaults and statements unchanged.

## Original/result/history pins

Assert `mesh.cut_result is cut`, `mesh.body is cut.solid`, `cut.left is box` and
`cut.right is cutter`; retain exact tolerance and segment cap. Freeze all full
mesh records and the original cut reference between repeat calls. Export original
box/cutter packed tables only under their explicit operand names, with immutable
byte hashes and sizes. Export result frame, vertices, curves, edges, coedges,
loops, faces, charts, and original patch control/frame bits as actual CutSolid
records. Export sections, arrangements, selections, forward history and reverse
origins from the retained CutResult. No packed CutSolid representation exists.

Use recipe.tsv for exact binary64 frame bits, exact rational tolerance, original
operand/result body IDs/revisions, cutter scale ratio2, radius, wall band,
counts, CPU placement and explicit native-call/transfer-zero scope. Include
sources.tsv with the new mesher, independent qualifier, caller, benchmark,
entry/geometry tests, and all directly consumed owned geometry modules. Pin all
sources before and after the campaign; avoid hostnames and absolute paths.

Mesh exports retain rational and rounded vertex positions, source keys, triangle
face IDs, chart-local corners, intrinsic physical-edge parameters, directed
coedge parameters and original UV, exact error bounds, qualification and actual
reader buffers/metrics. Preserve cropped seam original t[1/4,3/4] history separately
from its intrinsic output edge t[0,1]; boundary wall V stays [1/4,3/4], never
renormalized. Validate these claims against the independently reviewed API.

## Proposed bounded fleet campaign

Reuse canonical meshing measurement shape: serial CPU-only, original operands
and CutResult prepared once per case, first-after-imports/preparation, one excluded
warmup and five repeats. Fresh complete mesh plus actual indexed reader is the
complete wall. First output receives independent qualification outside the wall;
six later full immutable mesh records and reader buffers are compared directly
to that first qualified output outside the wall. Export that first output after
measurements. Record first/warmup/repeats, qualification and correspondence costs
separately; phase medians need not sum. Native/CUDA calls and host/device bytes
are zero. No GPU meshing or full CAD speedup claim follows.

Suggested cases span admitted frame sizes and world tolerances:

| Case | Origin | Box scale | World tolerance |
| --- | --- | ---: | ---: |
| coarse unit | (0,0,0) | 1 | 1/2 |
| fine unit | (0,0,0) | 1 | 1/1024 |
| translated/scaled | (-4,2,-1) | 2 | 1/4096 |
| translated subnormal | (smallest positive subnormal,0,0) | 1/4 | 1/4096 |

Coarse case exercises minimal topology; finer cases exercise larger indexed
results and different absolute world error budgets. Subnormal translation checks
actual source and rounded output handling without requiring a new frame scale.
Actual subdivision/counts/bounds must be observed, not predicted into a report.
Box scale2/cutter4 is admitted by difference; box scale4 is not. Boundary count/resource limits and extreme
rounding rejection belong to direct tests, not unnecessary benchmark expansion.

Standalone caller times operand preparation, Boolean construction, mesh
construction, actual indexed reader, independent result/mesh qualification and
filesystem export separately, with a directly enclosing preparation-through-export
wall. Benchmark reports preparation/Boolean/export as separate single costs;
complete mesh wall excludes all three, qualification, imports, final source and
timing-file writes and process shutdown. Do not sum unrelated medians or label
CPU Fraction construction as native/GPU time.

## Awaiting implementation

Tentative API is `cut_meshing.tessellate(CutResult,...)` with `.cut_result`, actual
`.body=CutSolid` and the established immutable mesh records. This study selects
no implementation before reviewed design and plan. No dependency is needed.
