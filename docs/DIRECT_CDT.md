# Native direct constrained Delaunay connectivity

The owned C++20 constructor builds fixed-site sampled-face connectivity from
exact source UVs and protected boundary segments. Python packs one immutable
input and copies original source-ID triples back; it performs no predicate
callbacks or connectivity repair inside the native operation.

The selected algorithm is alternating-cut initial Delaunay divide-and-conquer,
followed by randomized Shewchuk–Brown special-cavity recovery and signed-cap
Chew repair. Indexed topology replaces per-predicate Python container
traffic. Certified scalar/interval predicates fall back to owned exact integer
arithmetic and cached metric lifts. Read the [design](DIRECT_CDT_DESIGN.md),
[numerical design](DIRECT_CDT_PREDICATES_DESIGN.md) and
[primary literature](../papers/2026-10-16-direct-cdt-algorithm-selection.md).

## Build and use

On the selected Linux Clang18.1.3/libstdc++13 profile:

```
python3 -B scripts/check_native_face_domain.py --unit direct-cdt --profile release --tests test_direct_cdt
python3 -B scripts/check_native_face_domain.py --unit direct-cdt --profile debug --lint
python3 -B scripts/check_native_face_domain.py --unit direct-cdt --profile asan
```

The instrumented profile runs native executable fixtures. The independent
`tests/native_direct_cdt_cap_oracle.py` reads actual native-process output; it
does not load an instrumented library into Python.

```python
from fractions import Fraction
from brepgpu.face_domain import DirectCDT, constrained_delaunay

points = {0: (0, 0), 1: (2, 0), 2: (2, 1), 3: (Fraction(1, 2), 2)}
loops = ((0, 1, 2, 3),)
triangles = constrained_delaunay(loops, points, metric=(2., 1.), seed=0)

with DirectCDT(loops, points, metric=(2., 1.), seed=0) as domain:
    triangles = domain.triangulate()
    counters = domain.stats
```

`BREPGPU_DIRECT_CDT_LIBRARY` selects an exact explicit library path; otherwise
the loader searches the direct release/debug build directories. Layout and
symbol checks fail before source preparation in the imported consumer.
The direct binary also embeds the unchanged ear/checker ABI. Boundary-library
discovery can use it when no separate face-domain build exists; an explicit
`BREPGPU_FACE_DOMAIN_LIBRARY` selection remains exact.

## Geometry and failures

The input is one simple positive outer loop plus strictly contained disjoint
negative holes. Every distinct exact site must occur exactly once in loop words;
IDs are actual uint64 integers, including zero and max. Protected collinear
samples remain. The positive constant diagonal metric accepts finite exact
scales, including the exact values of supplied binary64 numbers. Output has no
Steiner sites and uses the original source IDs. Exact cocircular ties may select
either legal diagonal; unchanged input and seed are repeatable.

Defaults bound encoded input to 256MiB, tracked native allocation payload to
1GiB and charged work to 2^34. These are explicit arguments, not watch-specific
thresholds. Input/domain errors and resource exhaustion raise different Python
exceptions. A context constructs once: success is cached, failure is terminal,
and neither resets its budget or publishes partial connectivity. Repeated
copies can accumulate output-copy time while construction work stays fixed.

The stats separate validation, preparation, construction and copy time, exact
and filtered predicate decisions, merge/recovery/index work and native memory.
Initial-DT time includes its ordering subset. Process RSS and independent
qualification are separate measurements. Current hole containment validation
still has a pairwise cost; the complete call has no claimed O(n log n) bound.

## Real consumer and evidence

`mesh_imported(source, quality, domain_backend='direct')` selects this constructor
with the actual carrier metric. Circle-schedule checks, source maps, physical
keys and carrier refinement remain. It bypasses Python Lawson and redundant
final domain prechecks. The default remains `native` ears; `python` is the old
explicit reference. The final refined mesh is not claimed to remain a CDT.
OM10 rendering/qualification/benchmark CLIs accept the same direct selection and
pin actual source files, both selected ABI roles and seed zero.

The [performance report](../results/direct-cdt-2026-10-16/REPORT.md) retains 107
fresh observations: 103 qualified outputs and four censored old-path workers.
All 78 direct outputs qualify against original-value boundary/incidence and
independent full Fraction determinants. The largest forced-recovery case has
8,192 sites and more than 8,000 crossed edges. Bounded original NVIDIA/Manifold
inputs supplement authored families; this is not the complete alpha corpus.
The actual signed-cap helper has independent release/debug/ASan output checks,
including clockwise and vertical seeds. A public marked-fan repair hit remains
unobserved in the selected cohort and is not inferred from the helper checks.

The [complete original main plate](../results/main-plate-direct-2026-10-16/REPORT.md)
also passes:438 source faces,95,766 physical vertices and191,688 triangles at the
unchanged quality. One CPU constructs the complete part in208.258s; the existing
independent original-source/domain/physical-shell checks take169.985s separately.
Its input-domain reader now uses independent exact ranked-box candidate pruning.
Whole-watch CPU/P100 qualification and rendering remain separate unfinished work.
