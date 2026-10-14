# Body mutation and timing tooling review

2026-10-14. Read-only source review of
[run_body_mutations.py](../scripts/run_body_mutations.py) and the latest
[body timing driver](../benchmarks/body/run.py). No runtime, test, benchmark or
fleet execution was performed by this reviewer.

Final disposition: accepted after re-reading both corrections. Reviewed SHA-256:
mutation runner `01297821d4e81007beafe464306f28d4cb1351a8368a65387196920772d1db5a`;
timing driver `dabdb9a24bfd14652b91a1c3250cff0f1893877cff2a57f0e3a469201bbbc3fa`.

## BT-R1: mutation native/source provenance — resolved

The mutation runner records the changed module's original/mutant SHA-256 and
uses the same library path, but did not record or check the actual native
library bytes. Its assertion that all runs use the same core therefore lacked
the existing experiment's concrete binary identity. Record the core hash before
the suite and verify it after all mutants, along with relative source identities
for the body, sampler, independent oracle and direct tests. Keep native paths
and operational logs private. This is a bounded experiment-record correction,
not another test framework.

The corrected runner hashes the core and body/sampler/oracle/tests/driver
before execution, checks all remain unchanged after the trials, and emits a
public source-manifest with relative labels plus the actual core hash. Paths
and operational logs remain private. Accepted.

## BT-R2: complete timing phases and iteration IDs — resolved

The driver correctly rejects negative/NaN/infinite durations and checks one or
five values for each phase that appears. It did not require the full planned
phase set, and did not check that reused iteration IDs are exactly 0 through 4
or that each single observation uses -1. Add those direct completeness checks
before constructing summaries. Otherwise a missing phase or duplicate iteration
could still enter a partial public summary.

The corrected driver requires all seven single-observation and nine reused
phases, exact iteration sets {-1} or {0,1,2,3,4}, and the corresponding one/five
row counts. Finite nonnegative checks remain. Set equality plus row count
rejects duplicate/missing reused iterations. Accepted.

## Accepted source behavior

Four private production-source mutants attack hole membership, normal sign,
internal chart-boundary labels and admitted-row scatter. Each change has one
required source anchor and an AST check. The same independent direct suite must
first pass unmutated; kills require a failing assertion and reject explicit
import/syntax failures. Their changed expressions reach direct exact-label,
analytic-normal and original-request/position checks. Copies and stdout/stderr
stay under ignored context; authoritative source and native libraries are not
written by the runner.

The timing driver runs the fixed 24-case shape/resolution/backend matrix using
blocking subprocess calls in one serial loop. Each case is a fresh process,
with one cold/preparation observation, exact initial qualification outside the
reused timer, one excluded warmup and five retained-input calls. External
actual_reused_caller_ms includes returned-batch publication; internal span and
component phases are labeled separately. CUDA transfer figures distinguish
device payload, retained host provenance and CPU body/normal work. Existing
source/core/plugin identities are checked before/after the matrix. The runner
does not reserve the shared machine or itself prevent unrelated jobs; the
coordinator must uphold the documented no-owned-job-overlap condition.

Public artifacts use numerical case records, relative source labels and hashes.
Native paths, raw worker stdout/stderr and failure tracebacks remain private;
public failure messages name the case and ignored log location only. No further
product-code or numerical issue was found in this narrow tooling review.
