# Native mesher command

`brepgpu-mesh` reads the original BGSRC source, prepares original geometry and
assembly allowances, settles shared boundaries and native face refinement, then
writes the semantic mesh and matched assembled/exploded presentation. Source
preparation, topology and exact fallback run on CPU. The P100 backend uses the
selected bound module for numerical batches and the selected v2 display module.

```
brepgpu-mesh --source input.bgsource --output NEW_DIRECTORY \
  --positional-mm .005 --node-source-mm .001 --threads 24 --backend cpu
```

For P100, add both `--bound-module /path/to/libbrepgpu_mesher_bound_cuda.so`
and `--display-module /path/to/libbrepgpu_om10_cuda.so`, select `--backend p100`,
and optionally choose `--device 0`. The module paths are explicit. `--mesh-only`
selects semantic mesh/labels/timings output; that mode requires only the bound
module on P100.

The output parent must exist, and the final directory must be absent. Files are
written and closed under an owned sibling temporary directory; Linux no-replace
rename publishes them together. A failed invocation removes its temporary
output and preserves an existing final result.

Required inputs are source/output paths, positive positional and source-node
allowances, and `--threads 1..256`. Node allowance retains its source-node
recovery meaning. Whole curve/chord/domain/cell approximation uses the positional
allowance. Defaults: `--memory-mib 65536`, `--owner-mib 64`, `--seed 0`,
`--width 1920`, `--height 1800`, `--explosion 2`. Owner caps apply to face and
boundary-preparation arenas. Run memory includes prepaid reservations; the CLI
reports that peak explicitly. Resource exhaustion fails the operation.

A complete display result contains `mesh.bgnmesh`, `scene.bgscene`,
`views.bgview`, `assembled.png`, `exploded.png`, `labels.tsv` and `timings.tsv`.
The semantic mesh retains original source bytes and source/assembly/cell maps.
The scene preserves stored XYZ and resolves occurrence appearance. Numbered
image labels refer to original product/usage names in the escaped TSV.

Image captions report source preparation, meshing and display startup+frame
wall values separately, with backend/threads/view and requested allowances.
The timing TSV contains directly measured stages and named work/count/memory
rows. Its `whole.prepublication` value is a snapshot before writing/closing
that TSV. Stdout reports actual required-output-through-close, publication,
through-publication and resource-teardown wall values afterwards. An external
runner measures process-exit wall. Work sums are operations, never elapsed time.

The external fixture `tests/native_mesher_cli.py` checks actual complete source
runs on CPU1/CPU4/P100, full semantic/scene/view byte agreement, independently
decoded PNGs, labels/timings, existing-output preservation and failed-run cleanup.
Those small fixtures establish the connected executable; whole-source admission
and large-operation performance are reported separately.
