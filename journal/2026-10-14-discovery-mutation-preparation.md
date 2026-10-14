# Discovery production mutation preparation

Prepared [isolated mutation runner](../scripts/run_discovery_mutations.py) for the
qualified Linux x86-64 fleet. No native execution or mutation was performed on
the source workstation. Integration owns execution, compiler selection, device
coordination and interpretation of actual red evidence.

The runner refuses other host platforms and existing run directories. It copies
only active owned headers/native/Python sources into a fresh ignored
`context/discovery/<run-id>`, preserves baseline sources and hashes, changes unique
production source anchors only inside independent copied files, compiles unchanged
runtime/evaluator/projector objects once, and relinks each changed discovery object
into its own core library. Existing independent Fraction restriction/cache/row
oracles check a selected associated actual input. Every unmutated associated case
must pass first. Build, loader, CUDA device and unrelated infrastructure failures
are not counted as kills.

Eight CPU mutants cover inward subdivision endpoint rounding, removal of outward
primitive rounding, reuse of descriptive root distance as upper, wrong DOP normal
squared norm, reversed target support sign, clipping negative C0, querying inactive
Bernstein padding, and returning a visited-only lower after cap. A ninth CUDA
mutant changes strict pruning to equality pruning. Its direct production shared
`discover_local` kernel uses a constant-zero patch: CUDA directed operations give
L=U=0, whereas CPU interval widening makes that particular equality indirect.
The direct GPU probe checks actual compact output/status/coverage/counters; it
does not execute a reference implementation or model traversal.

Artifacts include copied runner, original/mutated source manifest, per-baseline and
per-mutant logs, separate build logs and `mutations.tsv`. The CPU source mutation
coverage does not establish equivalent device rounding mutation coverage; the
optional CUDA predicate case is explicitly separate. Omitting `--nvcc` leaves that
device case unqualified and prints the limitation. A survivor or infrastructure
failure returns nonzero and must be investigated rather than converted into a
claimed kill.

Proposed invocation from the isolated active checkout, after selecting the qualified
compiler/toolchain:

```sh
python3 scripts/run_discovery_mutations.py --run-id production-mutants-2026-10-14 --cxx "$BREPGPU_MUTANT_CXX" --nvcc "$BREPGPU_MUTANT_NVCC" --device 0
```

The two task-specific variables are paths selected by integration, not new
dependencies or toolchain installation. Estimated cost is 2–5 minutes for the
shared CPU build/eight discovery relinks and up to 1–2 minutes for two small sm_60
probe compilations/executions, subject to compiler/load. These are planning
estimates, not measured timings. Authoritative product files, retained libraries,
benchmark/caller/compatibility lanes and archived reference material remain untouched.

## Actual integration outcome

Integration reported successful qualified-fleet execution of the prepared runner:
all nine actual production mutants were killed, all associated unmutated baselines
passed, and there were no build, loader or other infrastructure failures. The actual
per-mutant exits and logs are preserved under ignored
`context/discovery/production-mutants-2026-10-14/`; authoritative product sources
and retained baselines were unchanged. This entry attributes execution to
integration; the numerical owner performed source preparation/review only.

The killed fault classes are reversed outward subdivision endpoints, removal of
CPU outward primitive rounding, rounded descriptive distance substituted for the
root upper witness, DOP normal norm/sign errors, negative Bernstein C0 clipping,
inactive padding evaluation, partial lower after visit cap, and equality pruning
in the actual CUDA shared discovery algorithm. Concrete primary log messages
belong in the published qualification table after integration reads the actual
per-mutant evidence; source-predicted failure reasons are not substituted for logs.
