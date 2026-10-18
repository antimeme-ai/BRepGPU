# Complete NVIDIA OM10 reference

> **Marked 2026-10-18, bead brepgpu-12s.37.17 (NVIDIA figures only; the owned 875.16/872.50 s quoted below are marked in their own report).** Pre-fix harness: the NVIDIA complete walls, stage rows and 25°/25° failed-attempt times here used 12 workers assigned by source face count, the caller confined to one socket's 12 physical cores but no worker pinned to its own core, memory unbound under AutoNUMA, one worker count, no per-timing context rows, a complete wall that includes our own physical-shell admission, decode and scene packing, and the OCCT no-shape-processing ingress arm rather than the candidate strongest ingress (OCCT's default STEP read); whether the fixed harness or the default-read arm is faster is not yet measured.

The genuine NVIDIA USD/BRep CPU reference now meshes and renders the entire
original movement under both declared healing policies: **107 definitions,
9,162 original faces and166 placed instances**. Each run produces231,416 unique
triangles and115,522 definition-local physical vertices. All107 decoded shells
pass the existing opposite-edge-use, orientation and original-source Euler checks.
The SDK also reports manifold input/output and zero lamina/spine edges throughout.

These are **COMPLETE_UNQUALIFIED** development results. They use an explicit
0.005mm chord request with curve/surface angular criteria disabled (0°/0°),
the original reference producer's legacy angular policy. Independent whole-watch
distance qualification remains open. Neither matching nominal chord values nor
closed topology establishes equal approximation quality or a speedup ratio.
The bounded existing source-query pilot on definition1808 refuses before
numerical output: `face-edge has no usable trimmed pcurve` (0.054907s,
1,173 finite witness points supplied). This is unavailable source-query
preparation, not a measured NVIDIA distance failure. No qualification campaign
or query-source repair is part of this delivery.

| Fresh original STEP → both labeled1920 views | Healing off | Healing on |
|---|---:|---:|
| Actual complete child-process wall |43.673684s|45.001245s|
| Fresh original STEP transfer/export |4.037132s|4.162916s|
| All12 preparation/SDK workers through exit |6.129702s|7.635501s|
| Decode, physical admission, scene packing/write |19.508225s|18.837788s|
| Physical-shell/source-Euler admission, nested above |16.736427s|16.354855s|
| Shared owned CPU12 display process, both views |1.265442s|1.524523s|
| Assembled PNG/SVG/labels |0.465881s|0.764544s|
| Exploded PNG/SVG/labels |0.485211s|0.481491s|

One observation per declared policy, run serially on twelve physical cores,
one SDK inner thread, with CUDA disabled. Preparation includes failed original
conversion attempts and their selected periodic derivatives. The complete wall
also includes source binding, pinning, orchestration, metadata writes and child
exit. Rows are nested and must not all be added. These first complete operations
use the historical matched assembled/exploded views; the new large technical
explosion is a separately timed replay of these same completed meshes.

[Actual outcomes](reference-outcomes.tsv), per-definition
[heal0](heal0/definitions.tsv)/[heal1](heal1/definitions.tsv), physical checks
[heal0](heal0/physical.tsv)/[heal1](heal1/physical.tsv), and full caller stages
[heal0](heal0/timings.tsv)/[heal1](heal1/timings.tsv) retain the direct results.
Input/tool hashes, exact parameters, complete BOM/placements and preparation
inventories are beside them. Source CAD and operational binary snapshots remain
private. All records refer to source SHA256
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.

## What changed at the adapters

Original conversion is always attempted first. Only actual conversion refusal
selects the guarded periodic preparation: OCCT's public angle splitter on an
isolated source copy, with exact nominal-period interval coverage, directed
edge chains, unchanged original basis/vertices/tolerances, face orientation and
complete ancestry. The two refused inputs change6→11 and117→122 faces, bringing
the imported face inventory to9,172 while retaining all9,162 original parents.
Focused preparation takes approximately0.011/0.115s. New cut tolerances and48
literal original-endpoint parameter restorations are recorded explicitly.

The existing conservative Plane range preparation changes only named face:range
attributes; a no-Plane layer passes through byte-for-byte. Genuine SDK healing,
tessellation, normals, physical vertex identities and original oriented face
ancestry remain in use. No owned replacement triangles, dropped components,
welding or post-tessellation topology repair is used to make the reference finish. Source-to-
descendant history and SDK face ordinals are separate namespaces.

Implementation/design: [periodic unit](../../docs/nvidia-periodic-preparation-unit.md),
[plane preparation](../../docs/NVIDIA_REFERENCE_PREPARATION.md),
[complete operation plan](../../docs/nvidia-complete-watch-run-plan.md).
The pinned official source revision is
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`; native release1.0.0 is unchanged.

## The stronger angular request still fails

Do not interpret the chord-only baseline as repairing the separate25°/25°
constraint policy. That policy exports all107 definitions under both heals,
but one definition fails the independent physical-shell gate before images:
three four-use edges include opposite duplicate triangles between a Plane and
its curved wall. NVIDIA itself reports nonmanifold output/12spine halfedges
before our raw extraction. All other106 definitions pass the physical reader.
The failed complete attempts take41.147823/43.241815s; these are not render times.
Their [outcomes remain published](angular25-failure/reference-outcomes.tsv),
with native reports and physical results under both policy directories.

Original and Plane-widened USD produce byte-identical failing output at these
same angular settings; every face has general trim type. Disabling either
angular criterion alone retains the failure. These controls rule out the
observed Plane-range preparation and raw export as its cause, without claiming
an upstream defect for every possible source/import representation.

A single isolated call to public `SmBrep::ConvertToPolyBrep` with advancing-front
enabled, unchanged0.005mm/25°/25°, produced no completed SDK result within its
configured180-second limit. Exact native CPU/wall and timeout exit were not
retained because the timer was inside the terminated group. This is a censored
experiment, not a successful result or measured180-second completion. No AF
production fallback or parameter search followed.

## Comparison and next work

The owned CPU12 and P1001/CPU12 cold source-snapshot→both-image operations remain
875.16/872.50s, with273,978 unique triangles. Their original STEP transfer is
outside those walls; the NVIDIA walls above include it. Their
[original report](../om10-watch-2026-10-17/REPORT.md)
and output remain unchanged. The new reference supplies the missing real
complete external baseline. It does not establish matched geometric quality;
the owned mesher's performance work must address the algorithm and complete
operation rather than hide Python/setup or compare isolated kernels.

The new exploded diagram separates train, bridges, barrel, winding work,
oscillator and calendar, retains all166 instances, and uses the same authored
translation/camera manifest across the three channels. It is delivered in the
[exploded-diagram branch](https://github.com/antimeme-ai/BRepGPU/pull/71), with
large images, actual visible-pixel labels and separately recorded replay costs.
It is an explanatory assembly illustration, not a validated service procedure.

Relevant checks: seven authored periodic/history controls, two malformed-input
refusals, three affected source-UV methods, and nine direct converter/batch/
angular/cancellation methods pass on the fleet. Four additional authored
periodic cases retain an explicitly reported existing raw-export limitation
after successful SDK import/tessellation. No alpha certification is claimed.

CAD attribution: Association openmovement, OM10 MAN VA01.3,
[OM10 project](https://openmovement.org/project/om10/), CC BY-SA3.0.
