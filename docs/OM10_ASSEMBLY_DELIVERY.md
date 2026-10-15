# Whole OM10 assembly, exploded labels and CPU/P100 comparison

Operator goal,2026-10-15: model the complete supplied OM10 assembly, then produce
an exploded/labeled version. Both tasks need CPU-only and GPU-accelerated paths,
with actual timings in graphics suitable for a key public X post. This replaces
the fixing clamp as the delivery target; the clamp can be a development fixture.
This document is a grounded requirements/gap brief, not an accepted implementation
design, a dependency adoption, a performance result or a complete kernel claim.

## What is already available

The supplied STEP describes107 unique solids,31 assembly definitions and166
expanded leaf occurrences. Original product names, occurrence relations,
placements, colors and material properties are present. Source bytes and hash
are retained privately. We have two P100s, CUDA12, owned CPU/CUDA rational-patch
evaluation and an indexed-triangle CPU display from study01.

The [actual whole-source study](../papers/2026-10-15-om10-whole-assembly-geometry-gaps.md)
finds9,162 faces,635 multispan spline surfaces and2,839 spline span rectangles.
Every spline surface's degree fits the current evaluator after correct span
extraction, but the current importer rejects all multispan surfaces. Analytic
planes/cylinders/cones/tori/spheres require their own carrier/domain conversion.
Boundary curves include seven degree-five splines; face loops reach55 per face
and965 edge uses. No explicit pcurve records supply their UV trim boundaries.
Existing meshers cannot consume this complete source family.

## Missing machinery

| Unit | Required work |
| --- | --- |
| Complete source import | Read actual STEP geometry/topology and original assembly transforms, units, source entities and per-occurrence styles; inventory every failed/changed record |
| Owned assembly model | Separate107 reusable definitions from166 full occurrence paths; compose nested rigid placements and preserve per-face/source identity |
| Carrier/curve conversion | Extract spline spans, construct analytic rational charts and preserve original parameter maps; support degree-five boundaries or declare and check a bounded conversion |
| Trimmed solid meshing | Recover UV boundaries from spatial edges, preserve same-face/seam occurrences, share physical edge samples, cover curved/nonconvex/multi-hole faces and report actual error |
| Exploded state | Authored subassembly offsets and directions on top of original placements, with a continuous explode parameter and an exactly recoverable assembled endpoint |
| Labels and presentation | Stable part/instance callouts, readable original names/curated aliases, leader lines, quantity/BOM legend and detail views for dense regions |
| CPU/P100 consumer | Native matched computation and explicit resource lifetime; GPU geometry work on the actual assembly, plus measured placement/normal/display stages |
| Benchmark graphics | Common source/quality/camera, repeated cold and reused runs, actual phase costs, complete walls, variability and a readable timing inset |

Explosion repositions the same parts; it should not build a second independently
modeled set of shapes. Mesh unique geometry once where reuse is part of the
named task, and instance it for the166 placements. Preserve original and exploded
transforms separately. A display explosion is not a validated disassembly path:
the source hierarchy does not establish mates, removal constraints or collision-free
motion. Those are unnecessary for an honest labeled exploded illustration.

## Proposed source/reference dependency

I recommend discussing an isolated **OCCT STEP/XDE reader and reference mesher**
for source extraction, comparison and the first complete reference view. Official
[XDE documentation](https://occt3d.com/dev/doc/overview/html/occt_user_guides__xde.html)
describes assembly definitions/locations, names, colors and STEP exchange.
[Meshing documentation](https://occt3d.com/dev/doc/overview/html/occt_user_guides__mesh.html)
describes edge/face triangulation and deflection controls. This is an established
CAD project with public source, tests and ongoing upstream development.

Benefit: obtain a faithful whole-source reference without making a new full STEP
translator the first dependency of the public image. Fit: separate fleet C++ tool
emitting source-linked data for our owned assembly/CPU/CUDA consumers. Costs:
build/toolkit footprint, version maintenance, source correspondence and tolerance/
healing behavior to inspect, plus its LGPL2.1/exception notices. Its mesher settings
are not independently established error bounds. The importer/reference mesher
must be named as foreign work; no OCCT operation becomes a BRepGPU CUDA operation.
The [official repository](https://github.com/Open-Cascade-SAS/OCCT) supplies source
and license notices. Existing pinned source was studied; no reference runtime
was installed or adopted by this assessment.

Alternative: own a strict reader for the observed STEP family, boundary recovery
and full trimmed mesher first. This fits complete machinery ownership but adds
substantial geometry/interchange work before a complete model is available.
There is no need to recreate an Inventor construction history to assemble and
explode its exported B-rep; reconstructing every original modeling operation is
a different requirement and the file does not supply that history.

## Matched CPU/GPU benchmark requirement

The complete source model must be present in both placements. A fast subset of
faces cannot stand in for the whole assembly. CPU-only execution must avoid CUDA;
the GPU-assisted path must perform named useful geometry work. GPU surface
evaluation does not imply GPU STEP reading, UV recovery, topology construction,
meshing or rendering. If CPU display dominates, matched native CPU/CUDA indexed
rendering is an additional unit; no GPU renderer currently exists here.

For each assembled/exploded task, record:

- **Cold STEP-to-image wall:** input/import, conversion, trimming/mesh, setup,
  uploads, useful geometry/normal/placement work, downloads, display/labels,
  image writes and final shutdown. Include shared CPU stages in both paths.
- **Reused-scene update wall:** explicitly retained geometry/device state,
  requested pose/camera changes, image production and synchronization. This is
  an update benchmark, not another full model-construction time.
- Native geometry and display stages separately, with setup/transfers/cleanup
  visible. Independent qualification is separate from useful work and retained
  in the qualified full-wall report; overlapping timing rows are identified.

Compare a strong native CPU baseline, including useful multicore execution,
against each promised P100 separately. Do not manufacture a GPU win by comparing
native CUDA with an avoidable Python triangle loop. Use identical source revision,
quality/error budgets, view, explosion and labeling policy; repeat and report
median/range under stated conditions. Reuse policy must be symmetric within each
comparison. Source uncertainty0.01mm does not select a manufacturing or mesh budget.

The finished graphic should have assembled and exploded panels, component/count
labels, actual CPU/P100 seconds and a concise scope legend. A compact phase
breakdown distinguishes geometry acceleration from total image production.
Measure before choosing the headline; cold/reused or stage/whole times cannot
be interchanged to imply a larger speedup. A slowdown is still a result.

## Suggested delivery order

1. Review source/reference boundary and a complete import/assembly contract;
   establish all107 definitions/166 placements and visible source issues.
2. Produce a faithful assembled CPU reference with explicit source correspondence.
3. Connect all supported source carriers/trimmed geometry to owned CPU/P100
   consumers, close every whole-model coverage gap and check actual output.
4. Add authored hierarchical explosion, instance labels and readable detail/BOM
   views from the same geometry, with both execution paths.
5. Run the matched cold/reused comparison and compose the two public graphics
   from actual measured output. Review source attribution/derivative terms before
   public source-derived artwork. Stop after the direct obligations pass.

No whole-assembly import, render, GPU speedup or alpha readiness has yet been
demonstrated. The required external community alpha corpus remains separate.
