# Native sampled-face engine delivered

Implemented the reviewed bounded unit after design/plan acceptance and an actual
fleet RED: authored specification tests could not import the absent native
capability. That initial missing-capability RED was not described as numeric
fault sensitivity. Production now owns exact integer coordinates/predicates,
rank-space segment/point BVHs, original-order first crossings, exact bridges,
strict source-occurrence ears and a no-hole strict-convex fast path in C++20.
Python supplies one lossless packed snapshot and consumes original node IDs.
The coarse C ABI owns buffers/results and reports typed resource failures.

No product dependency was adopted. The new host target uses the separately
reviewed Clang18/libstdc++13/C++20 rigor profile; existing CPU/CUDA modules retain
their previous build. Actual source import, edge sampling, refinement and full
source/physical/quality checking remain separate components. The original long
watch job and its pinned checkout were not changed by this unit.

Review found and fixed a genuine aggregate-input resource bug: a large common
denominator could expand many coordinates before cumulative encoded-byte checks.
An authored exact-input case failed on the previous fleet binding because its
expected ResourceError was not raised; cumulative projected limb bytes now reject
before expansion. Snapshot encoding checks before allocation/append, and reads
are bounded. Review also caught predecessor uint32 addition overflow at extreme
counts, bare-filename DSO loader ambiguity, missing strong node-slot distinction,
and a performance counter that only measured candidates after box rejection.
These were fixed. Boundary search now reports BVH node visits plus every leaf
box test, before overlap filtering. Source-node arguments use a strong NodeSlot;
other dense ordinals remain checked raw integers as documented.

The actual loader resolves the selected path, loads its symbols and checks all
record layouts before expensive source preparation. Context output/error ABI
sizes are checked. Native payload uses a context-owned pmr resource including
arithmetic temporaries and retained capacities; work accounting includes limb
operations. Named memory/work limits resolved one real static-analysis warning.
The only source suppression documents that edge AABB construction is symmetric
in its endpoints, so their interchangeable order is intentional.

Final focused evidence:11 authored Python tests, including252 signed arithmetic
vectors checked against independent Python Fraction results; exact domain/source
ID checks; malformed snapshots/contexts; explicit input/work/memory exhaustion;
context reuse and invalid DSO preflight. Native CTest exercises repeated hole
bridging/tree reconstruction, result cleanup, short outputs and malformed words.
Selected original NVIDIA/Manifold domain cases also passed their independent
actual-output checks. Final release compiler, formatting, clang-tidy and Static
Analyzer passes covered the actual three C++ units and all reached owned headers,
including the forced CPU profile guard. Final native ASan/UBSan CTest passed in
0.03s; the252 vectors passed through the sanitized native executable in0.025s.
No Python process loaded an ASan shared library. Independent algorithm/numeric/
resource implementation review accepted the fixed code.

Performance uses fresh optimized fleet processes, explicit single-core affinity,
CUDA hidden and independent qualification timed separately.54 planned scaling
trials passed (sparse strip, strict convex, dense concave and separated holes).
The actual8300-node source face was reproduced from unchanged source/schedules,
then generated8298 source-keyed triangles in three initial18-second trials.
Those initial trials predate final counter/layout refinements and are retained
privately, not substituted for final-binary measurements. The legacy exact
construction was censored at300s with no completed construction observed.
Final-binary trials and all public rows are reported in the result report.

The measurements explicitly expose remaining quadratic ear-point work; no
universal subquadratic triangulation claim is made. Active-subtree pruning,
small-inline integers, allocation profiling, broader native sampling/refinement
and parallel scheduling remain future units. This delivery does not establish a
whole-watch render, whole-mesher/GPU speedup, universal external conformance or
alpha qualification. Root owns consumer integration and complete-part/watch
qualification before claiming those outcomes.
