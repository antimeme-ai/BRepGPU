# Direct CDT public consumer and scaling workloads

Root delegated the public-constructor tests and independent performance workload
unit after completing the independent oracle. Read strategy and current wrapper,
proposed `docs/DIRECT_CDT_CONSTRUCTOR_TEST_PLAN.md`, and received root ACCEPT
before implementation. Native product and binding remain separately owned.

## Genuine public capability RED

Authored `tests/test_direct_cdt.py` against the agreed public `DirectCDT` and
`constrained_delaunay` entries, then ran on the new isolated fleet checkout:

```
PYTHONPATH=python:tests python3 -B -m unittest -v test_direct_cdt
```

Actual exit 1: `ImportError: cannot import name 'DirectCDT' from
'brepgpu.face_domain'`. This preceded the binding implementation. It establishes
missing-public-capability RED, not numerical fault sensitivity.

Nine focused methods cover the seven independent authored families, both
metrics/three seeds, five original external domains/three seeds, original-ID and
loop/node-order derivatives, metric reversal, exact cocircularity, protected
samples, cached-repeat/close, invalid source/scalar/metric and resource failures.
Cached-repeat comparison excludes cumulative actual `copy_ns`, per the owning
ABI contract. No expectation of unique diagonals across zero cases/seeds is
introduced.

The ninth method first measures deterministic create/preparation work, then
creates a second owner with exactly that work allowance. Construction must
exhaust its remaining zero work; a second attempt must fail terminally without
resetting the work counter. It is a direct test of the stated resource/lifetime
contract, not a guessed magic work-limit constant.

## Large semantic reader and workload implementation

Extracted `qualify_chain` from the existing shared geometry reader; bounded
`qualify_external` still validates arbitrary original domains and enumerates
the same direct embedding checks. Large authored inputs use actual-coordinate
family admissibility checks, shared positive exact chain/ID incidence/count,
and every-edge independent full Fraction 4x4 determinant. The design gives the
winding multiplicity and hanging-edge argument; no producer validation flag is
trusted and no skipped check is presented as qualified PASS.

Added exact scalable convex parabola, protected nonuniform strip, monotone
concave comb and separated square-hole families. Their short independent
admissibility readers inspect actual coordinates/order/cell separation. These
are oracle preconditions, not product dispatch or watch-derived admission.

Ran the revised oracle/affected external tests on the new fleet checkout. The
first attempt found no default old ear shared library in that fresh checkout;
this was corrected by explicitly selecting the existing release library through
`BREPGPU_FACE_DOMAIN_LIBRARY`, without modifying its checkout. Final command:

```
PYTHONPATH=python:tests python3 -B -m unittest -v test_direct_cdt_oracle test_face_domain_external
```

Result: **15 methods passed in 6.553s**. The four scaling-family cases at target
64 each passed both the bounded direct embedding reader and large chain reader
on actual native-ear→existing-Lawson output, with identical semantic statistics.
Changed actual strip coordinates fail its family precondition. This is bounded
semantic evidence; no direct native constructor or large-scale timing yet.

Authored `scripts/benchmark_direct_cdt.py` for fresh single-core direct versus
native-ear→existing-Lawson complete calls. It records exposed native stage/work
counts, normalization/encoding, separate qualification, product/whole-process
peak RSS, and phase-specific censored outcomes. Size loops are breadth first;
baseline seed-independent repeats are not multiplied by irrelevant seeds.
It never computes a speed ratio from a timeout. Parent process wall includes
interpreter/module startup, while product timing starts from actual given source
values and includes cold library load, native lifetime and output. Full direct
runs await core availability and implementation review.

## Initial breadth baseline: actual qualified output

Root authorized uploading its unchanged binding and running the existing
native-ear→Python-Lawson baseline in the isolated checkout. Verified the requested
core was in allowed affinity before pinning each fresh child. Selected the
existing native release library explicitly, with no change to its checkout:

```
python3 -B scripts/benchmark_direct_cdt.py --implementations ear-lawson --sizes 64,256,1024 --seeds 0 --cpu 2 --timeout 30 --output context/direct-cdt-ear-lawson-breadth.tsv
```

All **12 rows PASS**, including independent input admission, complete exact
positive-chain/source-ID incidence and every-edge exact CDT qualification.
Qualification was 0.011–0.274s and is separate from product timing.

| Family | Actual sites at target 1024 | Complete product seconds | Qualification seconds |
| --- | ---: | ---: | ---: |
| Convex parabola | 1024 | 0.098062 | 0.221537 |
| Protected nonuniform strip | 1024 | 8.200067 | 0.214722 |
| Concave comb | 1024 | 0.058407 | 0.183949 |
| Separated square holes | 904 | 0.882018 | 0.274128 |

These are one fresh observation each, not median/variance or a direct-CDT win.
The strip's strong growth is visible independently of watch geometry; other
families have different costs and remain equally required. Local private copies
of the actual TSV/log are retained under `context/direct-cdt-baseline/`.

Independent root boundary review found aggregate native `max_limbs` omitted the
validation phase while other native statistics aggregate both lifetimes. Root
was asked to preserve validation's maximum and report the maximum of both.
Cumulative work subtraction, sequential validation cleanup, PMR lifetime and
immutable native source copying were otherwise sound in this scoped review.

## Direct constructor breadth and rare signed-cap path

Root reviewed the workload validators, shared-chain path and benchmark driver
ACCEPT. Constructor owner fixed an initial merge tangent progression bug exposed
by the authored cases, then reported the full nine public methods GREEN. The
Fraction metric `(2/3,1/3)` is included in the existing metric-reversal method.

For actual timing, copied the selected release binary and runtime sources to a
private immutable benchmark runner; 148 source/runtime/binary hashes are retained
in its `pins.tsv`. This avoids mixing live code edits into fresh subprocesses.
Direct library SHA-256:
`b9f4ba9adb1d1268f6c3f121486afcfa654ae82e5c508a6bea8d9c5e6c13db17`.

Initial direct matrix (four families, target 64/256/1024, seed 0, one fresh run)
completed with all **12 rows qualified PASS**. At target 1024: convex 0.055245s,
strip 0.053517s, comb 0.053494s; actual 904-site holes 0.365972s. Hole validation
was 0.316808s while construction was 0.010279s: source-domain admission is already
a visible general-family cost. Other target-1024 native construction was
5.743–10.888ms. These are single observations; cold preparation, validation and
copying remain in complete product time. No whole-watch comparison follows.

Root authorized paired target 4096/8192, then independent seeds at selected
sizes with the same 30s phase deadline. Outcomes are retained as they complete;
timeouts are censored outcomes, never a ratio or a qualified result.

Targeted secondary-path discovery used radial-star inputs, then exact integer
diamond-star outers with separate rectangular holes and selected scaling cases.
Both bounded explorations completed in under four seconds total without a
secondary-path hit. They were branch discovery, not thousands of independently
qualified tests. Stopped further random discovery. One constructor-owner literal
hole recipe crossed itself; two explicitly requested small vertex variants
were valid but also produced no secondary path across its selected seeds.

Root instead selected a direct fixture of the actual shared signed-cap helper,
using two independently specified old-DT vertex stars. Implemented
`tests/native_direct_cdt_cap_oracle.py`: before reading cap output it checks each
original boundary's exact simplicity, all original apex-fan orientations and a
generic full 4x4 empty-circle determinant against every original boundary site.
Both literal original-star preconditions passed on the fleet. The helper's
actual triangle output then uses the existing complete `qualify_cdt`; declared
clockwise/vertical seed counters must both be observed. Release/debug/ASan native
processes run externally, so Python never loads an instrumented shared library.
Native fixture execution is pending the constructor owner's shared-helper build.

### Recovery workload gap and reviewed amendment

Actual counters showed all four original scalable families had zero crossed
edges and zero cavity occurrences, even at target 8192. Reported this immediately
to root; their evidence supports initial-DT construction and validation, not
recovery scaling. Proposed an independent exact two-chain long-hole-base family
whose initial DT has many edges crossing a forced protected source segment.
Root reviewed and approved the family/checker/cohort before code. The amendment
is written in `docs/DIRECT_CDT_CONSTRUCTOR_TEST_PLAN.md` before implementation:
small target 32/64 use both exact readers, then selected five-size/three-seed
matrix and baseline, with actual positive recovery counters required.

This is filling a missing algorithm fault/work class, not tuning a favorable
geometry or replacing the earlier cases. Existing four-family results remain
reported with their zero recovery counters and all unfavorable outcomes.

The actual shared signed-cap release fixture now passed its external Python
reader: **32 outputs qualified**, including original-star preconditions and
positive complete final CDT geometry, with **1 clockwise and 4 vertical seed
branches** observed. These are direct helper calls under established old-star
preconditions, not claimed public-constructor secondary-path observations.

### Completed selected workload unit

The long-recovery family at target 32/64 and seeds 0/1/2 passed the complete
bounded reader and scalable reader, with real positive crossed-edge/cavity
counters. Its five-size/three-seed matrix then produced 15 qualified direct
outputs; four baseline outputs qualified and the target-8192 baseline censored
in construction at the 30s worker deadline. At 8192 sites seed 0, direct took
0.337065s plus separate 1.736078s qualification, with 8169 crossed edges, 8174
cavity occurrences and a largest cavity of 4087. Native peak was 9,400,279 bytes.
This supplies actual recovery work rather than a producer success flag.

Root confirmed final release binary
`605e0b21c58093fe5270a30099b9338d18c1b2880de7465d8188553db2a7f968`.
Copied its binary/runtime/workload sources to another immutable private runner
and ran one fresh final-build breadth pass over all five families at target
64/256/1024, seed 0, CPU2. All 15 actual outputs independently qualified. Final
concurrent compilation was restricted to other CPUs; earlier exploratory
observations shared the host with unaffined development builds and can reflect
contention. No robust speedup-statistics claim is made.

Saved owned results under `results/direct-cdt-2026-10-16/`: REPORT.md,
observations.tsv and pins.tsv. The selected unit totals **107 fresh-process
observations: 103 qualified outputs and 4 censored baseline constructions**.
All 78 direct outputs qualified. Every unfavorable/censored outcome remains.
The four original scalable families' zero recovery counters and the subsequent
reviewed recovery amendment remain explicit; historical v2/v3 data are not
relabeled as final-v4 measurements.

Root separately reported the same 32 signed-cap outputs qualified in debug and
ASan native processes as in release, with all original-star preconditions and
one clockwise/four vertical seed branches. No duplicate runs were added here.
Root owns final native lint/instrumentation and actual imported-part consumer
qualification; this workload lane stops after its selected matrix and report.
