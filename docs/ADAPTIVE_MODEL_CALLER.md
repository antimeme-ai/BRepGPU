# Native adaptive assembly caller

`brepgpu-adaptive-model` runs complete placed definitions through the native
cached-adaptive mesher and writes assembled/exploded labeled images from their
actual stored mesh positions. It retains the original source bytes and paired
per-face boundary artifacts. The regular authored assembly and
[three-use analytic cap](ADAPTIVE_CAPS.md) paths are connected; the complete
imported watch remains in progress.

```sh
brepgpu-adaptive-model --source original.bgsource --output new-result \
  --positional-mm .005 --node-source-mm .001 --threads 4 --face-owners 2 \
  --width 1920 --height 1800 --explosion 2
```

CPU mode loads no device module. P100 requires explicit `--backend p100`,
`--cuda-module` for adaptive geometry, `--display-module` for the existing API2
renderer and `--device 0` or `1`. There is no backend fallback. `--definition ID`
selects complete original definitions and their actual placements; repeat it
for a subset. Omit it to require every placed definition. An unplaced selection
refuses rather than inventing a placement. `--mesh-only` omits display files and
images while retaining semantic output, scene admission and labels.

Repeat `--positional-mm` to request fresh meshes at successive qualities. Full
original envelopes are prepared once; later queries reuse them for exact
assembly error transport. Surface source banks and the bounded face pool remain
shared. Immutable curve banks are retained per definition across queries, so
each cold boundary service uploads only its relevant curves. This avoids
reuploading unrelated assembly curves; shared curves across distinct definitions
can still incur duplicate host extraction. Source spatial units are already mm.

Each query writes `definition-ID/` semantic directories, `scene.bgscene`,
`views.bgview`, `assembled.png`, `exploded.png` and `labels.tsv`. Top-level
`faces.tsv` names the complete relative mesh path and the exact captured boundary
snapshot used by that face, which may precede the final forest snapshot.
`queries.tsv` records requested and exact transported local allowances, counts,
contacts and numerical GPU work/transfers. Surface counters are cumulative owner
snapshots, not per-row deltas. Curve counters are per component. Transfers mean
host/device memory traffic, excluding source-file transport or SSH.

`timings.tsv` separates decoding, original envelopes/assembly, bank compilation,
each complete component and its stages, display startup/render stages, semantic
output, annotation/PNG and close. Nested stage times must not be summed twice.
Captions report source preparation, complete meshing and display startup/render
time plus quality. External process wall time and independent source-reader work
remain separate. BOM `label` is the displayed PART number; `occurrence` retains
the original assembly index, including gaps in selected subsets.

All required faces and both required frames complete before no-replace atomic
publication. Failure emits its stage/error and removes the temporary output.
Memory/work limits bound the actual operation; no failing source face is skipped.

The finite authored cohort covers two distinct complete definitions, three
styled placements including a proper quarter-turn, source face style, and
fine/coarse/fine on CPU1/CPU4/both P100s: twelve assembly queries,24 complete
definition outputs and144 original-checked faces. The SAME original component
and scene readers check geometry, shared chains, inventory, actual display
positions, placements and materials. Subset/BOM, mesh-only, no-replace, unplaced
selection and mid-assembly source refusal are checked directly. CPU1/P100
semantic artifacts and scene/view/BOM bytes match; CPU4 routing counters differ
while actual mesh semantics and scene/view/BOM bytes match. This is not a whole-watch
or GPU speed claim. NVIDIA reference and external alpha qualification remain
required. See the [design](adaptive-model-caller-unit.md) and
[reader plan](adaptive-model-oracle-unit.md).
