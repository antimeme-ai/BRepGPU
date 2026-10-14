# Focused classification mutation controller review

Source-reviewed scripts/run_classification_mutations.py and the seven supplied
target rows. No product or mutant execution occurred in this lane.

Seven unique source replacements target actual emitted behavior: hole material
sense, radial float underflow, rounded frame normalization, missing incident
faces, finite boundary segment crossing, incomplete contact vertices and wrong
right source UV. The selected direct methods contain independent geometry and
literal assertions for those behaviors. Sites must resolve exactly once; no
unused helper or syntax-only mutant earns a claim.

Each mutant uses an isolated copied Python/test tree with inherited caches
excluded. PYTHONPATH names that tree and PYTHONDONTWRITEBYTECODE prevents caches
being written, so restoring source for the healthy retry cannot reuse a stale
mutant pyc. Baseline and retry must exit zero. The five protected production
libraries are hashed before/after; product/native sources are never rewritten by
the controller. Operational logs stay in ignored context.

**CM1 — reject every crash/status anomaly explicitly.** Initial kill admission
requires a nonzero exit plus FAIL and AssertionError text and no ERROR text, but
does not constrain that exit to unittest's normal failure status. A negative
signal exit after a printed failure could receive credit. Sent root the required
fix before editing: require returncode exactly 1 and the completed unittest
FAILED-with-failures summary. A crash, status 2, infrastructure error or incomplete
runner output must fail the campaign. This repairs the direct mechanism rather
than adding a second test of its output.

Accept the scoped mutation workflow after CM1 disposition. Actual retained runs
must show normal assertion exits and healthy baseline/retry; successful process
statuses or appealing rendered output alone do not qualify an assertion kill.

Disposition: root updated kill admission to require returncode exactly 1 and a
completed `FAILED (failures=N)` summary with positive N and no ERROR output.
Source read confirms the fix; CM1 is resolved and the controller is accepted.
Root reruns the corrected fleet campaign after the serial performance workers.
This lane does not claim outcomes before those actual runs complete.

Final disposition: corrected mutations-final.tsv now records all seven mutants
with exit status 1 and healthy baseline/retry status 0. Root retained the actual
completed assertion-failure logs under ignored context. All five protected
library before/after hashes agree. Accept these scoped actual outcomes; they
do not establish a universal mutation score or require a second certificate.
