# Discovery mode comparison runner

Subplan before implementation: own one standard-library orchestration script
at benchmarks/discovery/compare_modes.py. It consumes the preserved staged
dynamic and specialized benchmark/core/plugin artifacts, admits only identical
benchmark/core bytes, records differing plugin identities as provenance, and
runs exactly the reviewed28 conditions per card by default. Cards execute
sequentially; variant order alternates by case. Each variant/case invokes the
unchanged run.py with one native condition and seven repetitions, retaining its
original samples, summaries and work files in a fresh subdirectory.

The script will record order and basename/SHA provenance, aggregate matched
full-condition phase summaries with all quality/work extrema, and compare work
histograms rather than discard them. Optional exact case IDs and reverse order
allow a compact same-case/both-card confirmation. No new numerical oracle,
native/CPU/benchmark change, JSON format or dependency is needed. Failed cases
stop the sequence with their existing artifacts and captured process logs.

Source review and AST/local CSV inspection are allowed here; all native builds,
execution and timing remain with the coordinator after the scalar queue. The
script itself must not infer a performance acceptance decision from one run or
add phase medians into an artificial wall result.

Implemented compare_modes.py. Actual CMake/runtime source names the executable
brepgpu_discovery_benchmark, core libbrepgpu.so and sibling plugin
libbrepgpu_cuda.so. The script resolves staged directories, refuses executable
symlinks and inherited core/preload overrides, compares core/benchmark SHA256,
and records only artifact basenames/hashes/sizes. It rechecks staged bytes around
each execution. Plugin hashes are provenance and need not match.

Default case IDs0..27 implement the reviewed table exactly. Stable case parity
chooses initial order, so --reverse-order inverts the original order even for
a selected confirmation subset. Cards execute sequentially. Each call to the
unchanged run.py requests one CUDA card/case, no CPU workers and seven samples.
Original samples/summary/work files remain under cardN/caseNN/variant. Order,
full28-case recipe and provenance are TSV. Comparison preserves both phase
median/min/max and every summary quality/work column; matching raw quality rows
and actual histogram bins are required before proceeding to the next case.
Changed quality/work stops execution with current comparison and original
artifacts retained. First-use/setup samples stay separate from resident/event/
cached-host phases. The script does not make an acceptance decision or add
phase medians.

AST inspection passed. Artifact-only inspection against an actual preserved
CUDA0 cylinder4096/AABB result retained all13 phases and quality/cache fields,
gave zero improvement for identical artifacts, and rejected a changed quality
population. No native subprocess, local/fleet benchmark or build was invoked.
The first artifact inspection's altered gap count matched the original zero;
corrected that inspection to increment the actual value. This was inspection
setup, not a product behavior failure.

Coordinator commands from the checkout root, after preserved sibling staging
and qualification (the enclosing staged directories must be distinct):

```sh
python3 benchmarks/discovery/compare_modes.py \
  --dynamic-binary context/staged-dynamic/brepgpu_discovery_benchmark \
  --specialized-binary context/staged-specialized/brepgpu_discovery_benchmark \
  --output-directory results/local-mode-comparison
python3 benchmarks/discovery/compare_modes.py \
  --dynamic-binary context/staged-dynamic/brepgpu_discovery_benchmark \
  --specialized-binary context/staged-specialized/brepgpu_discovery_benchmark \
  --cases 6,7,8 --reverse-order \
  --output-directory results/local-mode-confirmation
```

IDs6/7/8 are cylinder4096 depth8 AABB/DOP/Bernstein. Default devices0,1 apply
the same selection to both cards. Optional --cuda-devices selects0 or1 for a
bounded diagnostic run; any performance retention confirmation still needs
both cards under the reviewed plan. The operator/coordinator must prevent other
owned target jobs; this runner sequences its own work rather than claiming
exclusive control over external processes. No benchmark/run.py or native
source changed.
