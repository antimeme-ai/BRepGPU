# Direct constrained Delaunay: independent geometry and measured work

The owned CPU constructor builds fixed-site CDT connectivity directly, avoiding
the old sequence of creating an ear triangulation and repairing it in Python.
The selected workloads expose both that gain and a remaining quadratic cost in
many-hole input validation. These are sampled-domain results, not CAD import,
complete-part meshing, whole-watch rendering or GPU timings.

There are **107 fresh-process observations: 103 independently qualified outputs
and four censored baseline constructions**. All 78 direct-constructor outputs
qualified. Every row, including timeouts and unfavorable observations, is in
[observations.tsv](observations.tsv); exact binary/runtime/source snapshot hashes
are in [pins.tsv](pins.tsv).

## Scope and methods

Inputs are exact original UV sites, protected source boundary segments, one
outer polygon and optional disjoint holes, a constant positive diagonal metric,
and a recorded seed. No Steiner sites, snapped coordinates or dropped collinear
samples are allowed. Both implementations return original source-ID triangles.

`product_seconds` includes Python exact normalization/packing, first library
load in that process, native ownership/validation/construction, result copying
and destruction. The baseline includes actual native ear triangulation followed
by the existing Python metric Lawson operation. It requests a CDT on every
case; the existing imported mesher conditionally requests Lawson according to
its surface-error bound. These operation timings do not establish a whole-mesher
speedup. Input generation and independent qualification are separate columns;
`process_seconds` additionally includes interpreter/module startup and all
worker stages.

Execution used Linux, Clang 18.1.3 release builds, libstdc++ 13 and affinity to
one logical CPU on a dual-socket Xeon E5-2697 v2, 2.70GHz system. The host was
shared with active development: earlier unaffined builds could contend with
timed workers. Final-build confirmation restricted concurrent compilation to
other CPUs. Each row is one fresh observation; multiple seeds are algorithm
variants rather than controlled timing repetitions. **Do not interpret these
data as robust speedup statistics or isolated hardware throughput.**

The 30s limit applies to the entire worker. A CONSTRUCTING_TIMEOUT retains the
last observed phase and does not imply an exact 30s product duration. No speed
ratio is computed from a censored row. Qualification completed before every
PASS was written.

## Largest selected domains

Complete product time and separate qualification time for seed 0:

| Family | Actual sites | Direct product, s | Ear + Python Lawson product, s | Direct qualification, s | Direct build |
| --- | ---: | ---: | ---: | ---: | --- |
| Convex parabola | 8192 | 0.246410 | 0.864769 | 1.919821 | v2 |
| Protected nonuniform strip | 8192 | 0.232224 | Worker censored at 30s | 1.976080 | v2 |
| Concave comb | 8192 | 0.249707 | 0.637147 | 1.715145 | v2 |
| Separated square holes | 8104 | 13.183501 | Worker censored at 30s | 2.692569 | v2 |
| Long recovered constraint | 8192 | 0.337065 | Worker censored at 30s | 1.736078 | v3 |

The other censored row is the 4096-site protected-strip baseline. At 4096 sites,
the long-recovery baseline completed in 8.097662s and direct seed 0 in 0.161695s,
with independent qualification reported separately. Three direct seeds at 8192
long-recovery sites took 0.325627–0.337065s. These observations retain their actual
build and shared-host conditions.

Small inputs do not justify a universal improvement claim: the initial 64-site
comb observation was 0.006713s direct versus 0.006217s baseline. Both remain in
the data. Timing noise and setup matter at that size.

## The counters distinguish initial construction from recovery

All four first scalable families had **zero crossed edges and zero cavity
occurrences**: their protected segments were already initial-DT edges. They
exercise construction, exact degeneracies, ownership, validation and eliminating
old repair work, but do not measure constraint recovery. This gap was reported
before tuning; the separately reviewed long-recovery family was then added.

That family places protected sites on two exact parallel chains above and below
an unsplit hole-base segment. The initial DT crosses the required source segment.
The actual source coordinates, station order and strict containment are checked
independently. Every designated direct row requires positive crossed-edge and
cavity-occurrence counters as well as complete geometric qualification.

| Long-recovery sites | Crossed edges | Cavity occurrences | Largest cavity | Total charged native work | Tracked native peak bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| 64 | 53 | 58 | 29 | 37,525 | 73,657 |
| 256 | 245 | 250 | 125 | 198,780 | 293,217 |
| 1024 | 1013 | 1018 | 509 | 960,897 | 1,175,509 |
| 4096 | 4081 | 4086 | 2043 | 4,546,893 | 4,700,459 |
| 8192 | 8169 | 8174 | 4087 | 9,783,391 | 9,400,279 |

These are observed seed-0 counts, not a universal complexity proof. The source
recovery cases also ran seeds 1 and 2. A small bounded diagnostic found actual
recovery in the original Manifold Eberly case (two crossed edges/seven cavity
occurrences); most other bounded inputs already had their source edges in the
initial DT.

At 8104 sites/2025 holes, native validation took **12.951883s** while native
construction took **0.058919s**. Its existing hole-against-hole containment loop
is the dominant cost. Changing that reader is a separate algorithmic unit;
the result is retained rather than hidden by an aggregate average. Native
tracked peak was 8,133,632 bytes, and whole-process peak including qualification
was 52,764 KiB. Native tracking counts owned allocation payload; process RSS is
a separate measurement.

## Final-build confirmation

The main scaling matrix used v2 and the subsequent shared-cap-helper build v3.
After final fixes and analysis, a **fresh final-build breadth run** covered all
five families at target 64,256,1024, seed 0. All 15 outputs independently qualified.
The full older matrix was preserved rather than relabeled as the final binary.

Final SHA-256:
`605e0b21c58093fe5270a30099b9338d18c1b2880de7465d8188553db2a7f968`.

| Family | Actual sites at target 1024 | Final product, s | Native validation, s | Native construction, s | Independent qualification, s |
| --- | ---: | ---: | ---: | ---: | ---: |
| Convex parabola | 1024 | 0.028341 | 0.004080 | 0.002993 | 0.213803 |
| Protected strip | 1024 | 0.027558 | 0.001249 | 0.005959 | 0.223025 |
| Concave comb | 1024 | 0.028011 | 0.001149 | 0.006177 | 0.196514 |
| Separated square holes | 904 | 0.181645 | 0.155594 | 0.005455 | 0.276738 |
| Long recovered constraint | 1024 | 0.038548 | 0.001734 | 0.016356 | 0.212220 |

Native stages are contained in product time; construction's own substage clocks
are nested and must not be summed with their parent. Earlier and final timings
have different observed host conditions; the table does not attribute their
difference to the metric, compiler or helper refactoring.

Additional selected 256/1024 strip/comb observations used metric `(16,0.25)`
under three direct seeds and one baseline. All 16 qualified. Metric changes
can change legal diagonals; the independent reader checks actual metric-space
geometry instead of copying the isotropic triangle list.

## What qualifies the actual output

The independent reader checks original node rows before dictionary/set collapse,
all original uint64 identities and exact UVs, every protected segment, positive
triangle orientation, opposite interior incidence and complete exact boundary
chains. It then evaluates a generic Fraction 4x4 determinant on **every**
unconstrained interior edge in the caller's actual diagonal metric. Exact
cocircular zero accepts either legal diagonal. No native predicate, topology
flag or Python Lawson answer is used as the CDT oracle.

Bounded authored/reference inputs also use direct pairwise embedding checks.
Large authored families use checked actual-coordinate input admissibility plus
the same positive-chain/incidence reader: equality of oriented boundaries gives
coverage multiplicity one inside and zero outside, ruling out overlapping
positive triangles. Complete paired edge incidence and protected source segments
exclude nonconforming contacts. The argument and admission conditions were
reviewed before implementation in the [constructor test plan](../../docs/DIRECT_CDT_CONSTRUCTOR_TEST_PLAN.md).
Arbitrary captured CAD domains cannot select this path merely by claiming they
belong to a family.

Seven independently authored bounded families, all five original pinned
NVIDIA/Manifold inputs, exact wide/cancellation cases, metrics, seeds and source
permutations qualify the public constructor. The external inputs are difficult
geometries, not upstream answer keys or a completed external alpha suite.

No public workload here exercised the secondary signed-cap repair branch.
Instead, the actual shared helper was called on two independently established
old-DT vertex-star inputs. All 32 actual outputs passed complete CDT semantics
in release, debug and ASan native processes, with one clockwise and four vertical
seed branches observed. Python read their external output and did not load an
instrumented DSO. This is focused helper evidence, not a public-path observation.

## Reproduction and remaining work

On the selected Linux compiler profile:

```
python3 scripts/check_native_face_domain.py --unit direct-cdt --profile release
PYTHONPATH=python:tests python3 -B -m unittest -v test_direct_cdt
python3 -B scripts/benchmark_direct_cdt.py --cpu 2 --sizes 64,256,1024,4096,8192 --seeds 0,1,2 --timeout 30 --output context/direct-cdt.tsv
```

Choose an allowed CPU and record it. The driver selects fresh workers, retains
phase-specific timeouts and requires complete qualification for PASS. Rebuilding
the current checkout does not recreate historical v2/v3 bytes automatically;
the three recorded snapshots and binary hashes distinguish those observations.

The [algorithm strategy](../../docs/DIRECT_CDT_STRATEGY.md) and
[primary-source study](../../papers/2026-10-16-direct-cdt-algorithm-selection.md)
explain alternating-cut initial DT, randomized special-cavity recovery,
filtered-exact predicates and compact indexed ownership. Published bounds count
geometric operations under their assumptions; exact bit complexity, allocations,
validation and Python boundary costs remain real measured work.

Next consumer evidence is the explicit direct imported-mesh path at unchanged
source/quality bounds and a fully qualified actual part. Arbitrary captured-input
admission, many-hole validation, Python boundary preparation and later refinement
remain visible work. This report changes no alpha requirement and establishes
no full CAD-kernel or whole-watch speedup.
