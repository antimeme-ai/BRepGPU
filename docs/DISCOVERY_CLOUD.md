# Point-cloud discovery caller

`examples/discover_cloud.py` submits 1,024 deterministic XYZ targets to one
named untrimmed patch, without UV seeds. It builds and uploads one complete
hierarchy, downloads the actual cache for exact qualification, runs the resident
batch operation, and writes every valid answer, including unresolved brackets.
The three supplied shapes are a rational quarter-cylinder, a polynomial
parabolic cylinder and a diagonal plane. Lineage identifies a synthetic fixture;
this caller does not import a CAD face or apply trims.

Use Python 3.10 or newer and the qualified Linux x86-64 ABI3 CPU core. CUDA
requires the corresponding optional CUDA backend with discovery ABI3 and a
supported device. The CPU interval hierarchy builder runs for both backends,
so its round-to-nearest, gradual-underflow and masked-exception requirements
also apply to CUDA calls. A missing requested backend fails the run. Build
locations are the adapter defaults; `BREPGPU_LIBRARY`, `--library` and
`--plugin` select explicit artifacts when needed.

Run from the checkout root, with a new output directory for each invocation:

```sh
PYTHONPATH=python python3 examples/discover_cloud.py --backend cpu --threads 1 \
  --shape cylinder --mode aabb --output-directory results/local-cloud-cpu
PYTHONPATH=python python3 examples/discover_cloud.py --backend cuda --device 0 \
  --shape cylinder --mode bernstein --output-directory results/local-cloud-cuda0
PYTHONPATH=python python3 examples/discover_cloud.py --backend cuda --device 1 \
  --shape cylinder --mode bernstein --output-directory results/local-cloud-cuda1
PYTHONPATH=python python3 examples/discover_cloud.py --backend cpu --threads 24 \
  --shape polynomial --mode bernstein --output-directory results/local-cloud-polynomial
PYTHONPATH=python python3 examples/discover_cloud.py --backend cpu --threads 1 \
  --shape diagonal --mode dop26 --output-directory results/local-cloud-diagonal
PYTHONPATH=python python3 examples/discover_cloud.py --backend cuda --device 0 \
  --shape cylinder --mode aabb --max-visits 1 --max-refinements 0 \
  --output-directory results/local-cloud-capped
```

Defaults are `--count 1024`, `--depth 8`, `--max-visits 8191`,
`--max-refinements 8` and `--tolerance-squared 2**-24`. The command line accepts
a numeric tolerance such as `5.960464477539063e-8`. Depth and budgets control
work and memory; they do not guarantee gap precision. The last command makes
budget-limited answers visible. Precision and coverage flags are independent.

Every downloaded node is checked by the existing independent exact power-basis
restriction oracle, including bounds, supports, Bernstein base coefficients,
padding and dyadic cell coverage. Each returned binary64 UV is evaluated with
Fraction on the original supplied homogeneous controls; its exact distance
must be no greater than the reported upper endpoint. Each bracket must contain
the cloud's exact analytic global distance, and every gap-success row must
meet the exact squared-width and objective-gap tolerance. These checks have no
certificate epsilon. Descriptive XYZ and approximate distance use their
separate evaluator error allowances. Known analytic UVs are used for
qualification and export only; they are never supplied to discovery.

For these analytically known clouds, `qualify_row(check_flat_lower=False)`
checks the returned lower against exact global truth rather than repeating
the expensive all-leaf method-specific lower calculation for every target.
This option requires an analytic global answer. Standalone conformance retains
the default `True` and checks the independent flat/root lower recipe in addition
to witness certificates. Both paths qualify actual downloaded caches.

The caller writes `records.tsv` with target, answer and provenance bits, actual
work/status flags and exact rational witness/global distances; `controls.tsv`,
`hierarchy.tsv` and `bound-cache.tsv` preserve original input and actual caches.
`summary.tsv` records outcome populations and descriptive errors.
`timings.tsv` separates first use, preparation, CPU hierarchy build/upload,
cache download/exact qualification, query ingestion, resident discovery,
allocating download/provenance, cleanup, result qualification and export.
These are single caller samples rather than benchmark medians.
`caller_total_through_svg` directly measures the wall interval starting just
before input preparation and ending after SVG export. It includes setup,
native calls, exact cache/result qualification and records/SVG export; it
excludes process imports, argument parsing and final `timings.tsv`/`RECIPE.md`
writes. Named phases should not be summed to infer this wall interval.

The owned SVG shows up to 180 evenly distributed requests, retaining both
gap-success and unresolved representatives when present. Gray boxes are the
root and up to 16 cached leaf AABBs, even in DOP/Bernstein mode. They are not a
visited/pruned query trace. All request rows remain in the TSV regardless of
what is drawn. Gap precision bounds squared distance; it does not establish
unique nearest UV, trim validity or accuracy of UV near degenerate minima.

The caller rejects an existing output path before native work, then creates
the directory exclusively at export to avoid overwriting earlier receipts.
Native resources are all closed on success or failure. Cleanup failures retain
an active primary exception; a failed qualification emits no success graphic.
