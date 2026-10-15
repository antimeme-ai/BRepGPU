# Actual extrusion result → complete planar mesh

The next result-meshing connection consumes an actual convex-profile
ExtrusionResult, retaining its original profile, solid, controls, source maps
and complete typed history. Its planar trimmed faces are triangulated exactly;
only the saved binary64 vertex rounding can add geometric error. No canonical
body reconstruction, new dependency, native build or GPU mesher is involved.

![Actual six-sided extrusion mesh](../../assets/extrusion-meshing/chamfer.png)

The actual exported top-face triangles cover the original profile polygon,
rather than its square carrier. The second view draws all mesh faces without
hidden-line removal. Both views and the PLY use one shared rounded vertex array.

## What was checked

The independent source-first qualifier checks original profile/extrusion/history
truth, then every actual mesh vertex, physical edge, directed coedge/UV and
triangle corner against those sources. Each face's positive triangles must have
exactly the complete original directed boundary chain. This establishes full
face coverage without overlaps or gaps, rather than checking only vertices or
aggregate area. It repeats this check on actual rounded domains, including
non-axis side planes. Every mesh edge has two opposite uses, every vertex link
one cycle, and the shell is connected with genus zero.

Exact planar source approximation error is zero. Actual maximum L1 vertex
rounding bounds corresponding whole triangles in both directions. Rounded layers
must retain identical XY, common ordered Z levels and globally strict convex
profile support. Original side planes may shift slightly under rounding; the
rounded prism remains embedded and inside the submitted error budget.

Ten direct test methods passed in4.661s after genuine missing-API red. They cover
3/6/16/32-vertex families, a thin valid lattice profile, non-axis sides, all admitted
frame scales, extreme/subnormal origins, height endpoints, exact rounding-budget
equality and rejection below it, original identity/UV/history, immutable output
and preservation/retry after invalid calls. Selected altered actual meshes expose
missing/duplicate/reversed triangles, bad source/chart/UV/shared indices and false
rounded coordinates/error. These are direct fault cases, not a mutation score.

One affected compatibility run passed all48 methods in32.507s, covering existing
extrusion/construction/sampling and canonical/cut meshes. Independent actual-code
review found no open defect. All geometry/test/caller execution was on the fleet.
No broader numerical/GPU campaign or second coverage mechanism was added.

## Actual outputs and scoped CPU costs

Each recipe prepares one actual profile and feature, produces a first mesh and
three repeated fresh meshes. First output receives independent qualification;
subsequent full mesh and indexed/history outputs are compared with that first
output outside useful timing. All three cases have exactly representable output
vertices and actual error0. Direct subnormal-origin cases separately exercise
nonzero rounding and exact budget rejection.

| Case | Original profile vertices | Shared mesh vertices | Triangles | Complete median [min,max] ms |
| --- | ---: | ---: | ---: | ---: |
| Triangle |3|6|8|35.363 [18.424,37.379]|
| Chamfer |6|12|20|28.525 [28.287,80.948]|
| Many16 |16|32|60|107.844 [101.346,111.581]|

The complete wall directly wraps mesh construction, actual indexed-buffer and
record reading, and typed forward/reverse history traversal to original profile
entities. For Many16, phase medians were41.470ms mesh,1.579ms indexed reader and
61.942ms history reader. Phase medians need not add to the complete median.
These are three repeated observations per case on a shared uncontrolled CPU;
the ranges supply no causal scaling/clock/load result or performance speedup.

Profile preparation, original extrusion construction, independent qualification,
correspondence and filesystem export are separate. First independent checking
measured37.732/130.559/344.819ms for triangle/chamfer/Many16. Raw input geometry,
Frames/heights and histories remain in each case's source/result records.
See [raw.tsv](raw.tsv), [summary.tsv](summary.tsv), [single.tsv](single.tsv),
[counts.tsv](counts.tsv) and [timing scope](SCOPE.md). Imports, final summary/source
writes and interpreter shutdown are outside useful walls. Native/CUDA calls
and device transfers are zero.

## Use and stopping point

```sh
PYTHONPATH=python:tests python3 -m unittest -v test_extrusion_meshing
PYTHONPATH=python python3 examples/mesh_extrusion.py \
  --output-directory results/local-extrusion-mesh
```

Each case saves original profile/control/Frame data, actual result records,
complete source histories, exact/rounded/source vertices, triangles/corner UVs,
physical edge/coedge maps, bounds, qualification and actual PLY/SVG.
[sources.tsv](sources.tsv) matches the measured sources before/after the run.
The PNG is a static conversion of the actual chamfer SVG, viewed after export;
conversion is outside caller timings.

[API](../../docs/EXTRUSION_MESHING.md),
[design](../../docs/EXTRUSION_MESH_DESIGN.md) and
[bounded plan](../../docs/EXTRUSION_MESH_PLAN.md) define the narrow input family.
The [continuing independent review](../../papers/2026-10-15-extrusion-mesh-review.md)
and [oracle study/review](../../papers/2026-10-15-extrusion-mesh-oracles.md) record
actual findings. Direct obligations, affected compatibility and actual callers
passed; this unit stops here. There is no general triangulation/quality or
release-certification claim. Required external alpha qualification and existing
reference disagreements remain open; other feature families, operation composition
and runtime/language work remain separate roadmap units.
