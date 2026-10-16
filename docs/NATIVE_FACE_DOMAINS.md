# Native exact sampled-face domains

The first C++ transition moves sampled-domain crossing detection and initial
hole/ear triangulation into an owned C++20 CPU engine. Imported meshing selects
it by default. Source import, edge sampling, refinement, independent checking and
the existing C++17/CUDA display path keep their current owners.

Input is a complete immutable set of source node IDs, exact two-dimensional
coordinates and ordered outer/hole boundary words. The Python binding normalizes
each axis to a shared positive denominator, preserving every coordinate exactly.
The native context copies packed values on creation and retains them for reuse;
the high-level functions create a context per call. Exact rank boxes
prune candidates; owned signed integers evaluate every surviving geometric
predicate. Rank values never replace coordinates in orientation or distance.
There are no Python callbacks inside native geometry operations.

The context owns packed trees, occurrence links, exact arithmetic and output.
Source IDs remain distinct from dense node slots and duplicated bridge
occurrences. Strictly convex simple rings use an exact cursor-preserving fast
path. Other domains use indexed blockers and exact hole visibility. Hole bridge
distance ordering is exact, so a valid hole triangulation can differ from the
legacy rounded-distance result. All original boundary samples remain required.

The design and implementation plan describe the complete current contract:
[domain design](NATIVE_FACE_DOMAIN_DESIGN.md),
[implementation plan](NATIVE_FACE_DOMAIN_PLAN.md),
[production integration](NATIVE_FACE_DOMAIN_INTEGRATION.md).

## Build and use

On the qualified Linux CPU host, use Clang18.1.3, libstdc++13, CMake3.28.3 and
Ninja1.11.1. These settings apply only to the new target. The tooling refuses a
different actual compiler, standard-library release or language standard.

```sh
python3 -B scripts/check_native_face_domain.py --profile release --tests tests.test_face_domain
python3 -B scripts/check_native_face_domain.py --profile debug --lint
python3 -B scripts/check_native_face_domain.py --profile asan
```

The release library defaults to
`build/face-domain-release/libbrepgpu_face_domain.so`. Select another built
library explicitly with `BREPGPU_FACE_DOMAIN_LIBRARY`. The binding resolves its
absolute path, loads it and checks the ABI before expensive imported-source
preparation. Watch consumers pin its actual bytes alongside source and display
binary hashes. Missing/invalid libraries fail visibly.

```python
from brepgpu.face_domain import FaceDomain

points = {0: (0, 0), 1: (2, 0), 2: (2, 1), 3: (0, 1)}
with FaceDomain(((0, 1, 2, 3),), points, token=17) as domain:
    triangles = domain.triangulate()
    stats = domain.stats
```

Default input/native payload/work budgets are256 MiB/1 GiB/2^34 work units.
Caller-selected limits and geometric invalidity have distinct failures; an
exhausted search never means disjoint or successfully triangulated. Independent
contexts can operate concurrently. Sharing one mutable context concurrently is
outside this API's contract. A new source, sample schedule or preparation choice
requires a new context/token; this unit is not a topology-edit cache.

`mesh_imported(source, quality, domain_backend='python')` explicitly selects the
legacy reference. The watch render, qualification and benchmark CLIs accept
`--domain-backend native|python` and record the choice. This option concerns
CPU domain construction; the separate `--backend cpu|cuda` selects display.

## Correctness and performance evidence

The focused suite checks exact arithmetic against independent Fraction vectors,
source boundary/ID preservation, holes, crossings, collinear samples, malformed
input, bounded resources and context reuse. Native ASan/UBSan fixtures exercise
the actual owned module. Selected external inputs retain original Manifold
corpus bytes/license and NVIDIA's complete planar trim recipe; our own exact
domain/incidence/embedding readers check the resulting triangles. Their selected
scope is explicit and does not replace full external alpha qualification.

The release benchmark uses fresh processes and complete normalized-input-to-
triangle cost, with independent qualification timed separately:

```sh
BREPGPU_FACE_DOMAIN_LIBRARY="$PWD/build/face-domain-release/libbrepgpu_face_domain.so" \
python3 -B scripts/benchmark_face_domain.py --family sparse-strip \
  --sizes 256,1024,4096,8192 --implementations native,legacy --repeats 3 \
  --timeout 300 --output context/face-domain-scaling.tsv
```

The runner also supports strict-convex, dense-concave and holes families and an
owned lossless BGFAC001 input snapshot. Timeout records identify the unfinished
phase and preserve completed construction timings. Crossing-only node visits and
leaf box tests count work before overlap filtering; low exact-predicate counts
alone cannot prove elimination of exhaustive pair enumeration. Ear candidate and
dead-occurrence counts expose remaining worst cases. No universal subquadratic
triangulation claim is made. Actual watch-face and complete-part observations
must keep their scopes distinct from whole-watch or GPU performance.
