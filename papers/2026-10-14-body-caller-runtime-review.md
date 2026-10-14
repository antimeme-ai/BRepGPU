# Body caller and benchmark runtime review

2026-10-14. Source-only review of [sample_body.py](../examples/sample_body.py)
and [body benchmark runner](../benchmarks/body/run.py) against the accepted plan
and implemented sampler. No local product execution, tests/builds or fleet work.

Disposition: accepted after the actual returned-call timing correction below.
No unresolved lifetime, phase-scope or public-log privacy blocker found.

Accepted reviewed source identities: example SHA-256
`c6bb3ad53811fb3b33070c6dfa7ca2b2633814d7dfd96d4f51580825d4078f49`;
benchmark SHA-256
`7a80280043d39a8b0925999306b2a1e0eb9b697680af12fc83c57f4fde00bb48`.
The separately reported sampler child-cleanup finding is outside these root-owned
files and must be resolved before full integration; it does not change this caller
timing/privacy disposition.

## Lifetime and publication

The example creates the immutable body/request before native setup, scopes a
borrowed Engine and owning BodySampler, samples/downloads/converts/consumes before
closing resources, then independently qualifies the owned result after closure.
Cold caller cleanup includes sampler PatchSet and Engine close. A sampling failure
unwinds existing context managers and cannot fall through to a successful export.
Qualification runs before export-directory creation; failed output writes can
leave partial files but CLI completion/recipe/timing success is not reported.

The benchmark uses one fresh subprocess per case. Its cold operation creates and
closes the first Engine/PatchSet, then qualifies the owned rows. A separate retained
Engine/PatchSet supports one excluded warm-up and five prepared-input reused calls.
Every call retains sampler status/provenance/finite-normal checks; only the initial
direct exact/analytic qualification is outside repeated timers. First-use scope is
honestly a fresh case process/context, not a cold driver/filesystem guarantee.

## Timing correction and final scopes

Initial benchmark source rendered internal reused_wall_ms as complete returned-call
time. The sampler's internal span ends after successful child cleanup and excludes
timing-tuple/frozen SampleBatch construction. Reported this concrete scope gap to
the coordinator, who added an outer timer immediately around `sampler.sample`.

Re-read source confirms repeated `actual_reused_caller_ms` is recorded alongside
the eight internal phases, the sample-count contract includes all nine phases,
and the primary figure selects actual_reused_caller_ms. METHODS explicitly
distinguishes internal through-cleanup scope from external publication/returned-call
wall. The correction is resolved; no frozen output mutation or synthetic median
sum is needed to make the inner clock include publication.

The example's actual cold wall includes body/request setup, Engine/PatchSet,
sampler call and final resource close. Independent qualification and export are
timed separately; actual export-inclusive wall covers both, through the recipe/
SVG/artifacts, explicitly excluding final timings.tsv write. It begins after
imports/argument parsing/fresh-path admission. Phase scopes include host wrappers
and provenance, not pure GPU/kernel/PCIe measurements. CPU24 affects native
evaluation workers, not the serial Python stages.

## Bytes, identity and log privacy

CUDA query numeric upload is 20 bytes/admitted row; order-1 jets/status download
is 76 bytes/admitted row. Source metadata is 136 bytes/patch descriptor plus 20
bytes/query read from retained host provenance. DevicePatch is 16 bytes plus
homogeneous controls per patch. Body tables remain CPU data with zero upload;
those quantities are not total host memory traffic. The code records these
separately and describes conversion/output allocation as additional CPU copies.

Controller records current relative source identities and actual core/plugin
hashes before the matrix and requires them unchanged afterward. Worker commands
receive explicit selected binary paths. Successful public records contain shape,
backend/device/worker counts, numerical IDs/frames, inputs/results and hashes,
not private paths. Captured stdout/stderr and command-failure detail remain in
ignored context/body; the public failure message points there generically.

Exports preserve every candidate, submitted hex UV, label/native applicability,
face/chart/body/revision and XYZ/normals. Binary table dumps have owned safe names
and a byte/hash manifest; controls/frame metadata are retained separately.
Process-local ID scope is stated. PLY contains admitted face/chart samples; SVG
uses actual admitted samples/normals plus independently labeled CPU boundaries,
without connecting through rejected holes. Duplicate chart/boundary samples,
point-output thinning and absence of mesh/chord/GPU-topology claims are explicit.

Fixed serial cases, five samples, raw phases and min/median/max are appropriate
for the scoped caller. Exact initial qualification and status/provenance/normal
checks are direct mechanisms; CPU/GPU parity and attractive figures do not supply
truth. Publication must retain actual candidate/evaluated counts and first-use/
prepared-input reuse scopes as the source does.
