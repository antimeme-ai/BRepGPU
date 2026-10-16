# Begin moving production modeling into C++

Proposal in response to the operator's request to inspect the sibling
Blackbird repository's actual C++ rigor stack and advise on phasing Python
out. The earlier diagnosis-only hold remains: this study selects no library,
changes no product or build configuration, and starts no replacement watch run.

## Recommendation

Begin a deliberate transition toward an owned C++ production modeling core,
with the existing optional CUDA backend. Keep Python useful for independent
exact geometry oracles, research, data preparation and an optional scripting
front end. The product should eventually perform its complete admitted modeling
workflow without a Python interpreter or per-entity Python object traversal.
This is a recommendation, not an already approved shipping-language decision.

Much of the lower stack is already C++/CUDA: numerical evaluation, projection,
discovery, adjacency, native geometry/display and isolated source/reference
bridges. The migration expands native ownership through the expensive geometry
construction between those components. It does not require replacing the
qualified evaluators or changing their ABI to make an initial advance.

The watch demonstrates both an algorithm problem and a placement/representation
problem: large quadratic searches in one Python thread, exact object arithmetic,
and no mesh-stage CPU worker scheduling. A direct translation would retain
avoidable searches. C++ also does not automatically create parallel execution;
that needs explicit ownership, dependencies and bounded scheduling.

## What transfers from the inspected rigor stack

Inspected the live sibling's CMake, presets, rigor/Linux launchers, actual native
Result/Id types, probes, profiling procedure and recent C++ bug-class/tooling
studies. The checkout has existing uncommitted work; exact inspected file hashes
and source state are retained privately rather than attributing all bytes to its
HEAD. No sibling code, configuration or working changes were edited or executed.
The following is a transfer assessment, not adoption of its unrelated product
rules, Lua/TUI tooling or full check launcher.

| Observed mechanism | Proposed CAD use |
| --- | --- |
| Explicit C++20 compiler/library profiles; CMake/Ninja and actual compilation databases | Reproducible native CPU builds, with separate qualified CUDA configuration |
| Errors for conversion/sign, shadow, fallthrough, format and other focused compiler warnings | Check slot/count arithmetic, narrowing to GPU formats and accidental domain conversions |
| clang-format, focused clang-tidy and Clang Static Analyzer over actual compiled units and reached owned headers | Detect moved-from use, unchecked optionals, suspicious copies and ordinary lifetime/control defects without unanalyzed owned headers |
| Separate debug, optimized, ASan+UBSan, TSan, fuzz and symbolized profiling builds | Memory/UB checks; concurrency checks for actual parallel code; generated malformed input/domain cases; meaningful release performance |
| Deliberate known-bad diagnostic probes in tool qualification | Confirm the selected host/instrumentation can detect the promised fault class, once at setup/change rather than on every geometry edit |
| Native tagged IDs and nodiscard results, explicit ownership and errors | Separate original entities, occurrences, packed slots and runtime handles; distinguish geometric answers from unsupported/unresolved/resource failures |
| Direct behavioral and failure checks despite clean tools | Preserve actual geometry, boundary coverage, ancestry and old-input validity under failure |

The sibling's newer thread-capability, typestate and lifetime-warning experiments
are paired research probes, not blanket production rules. Some newer flags work
on its recent Mac compiler and fail as unknown on its Linux compiler. Evaluate
such a rule on the actual owning CAD API before making it mandatory. An annotated
span remains a borrow, and a clean sanitizer run does not establish correct hole
winding, complete intersections or source correspondence.

## Proposed first native unit

An **owned sampled-face domain engine**: accept immutable boundary occurrences
and their exact UVs; build conservative spatial indexes; detect unsupported
boundary crossings; bridge admitted holes; emit positive source-keyed triangles.
It serves both circle candidate-domain checks and final face connectivity.
This directly attacks the observed pair and ear searches while giving their
arrays, indexes, active occurrences and results one native owner. Boundary
pruning and connectivity remain separately reviewable stages within that design.

Keep calls at operation/batch granularity. Avoid Python callbacks per predicate
or allocation per vertex; these would keep execution ownership fragmented.
Preserve current crossing/ear ordering where that is the declared migration
contract. Retain exact collinearity, bridge duplicates, seam uses, poles,
physical keys and the original spatial/UV correspondence. A native display mesh
is not a lossless input here: it has already rounded UVs and omitted boundary
occurrence/recovery records.

Before implementing that engine, select the exact-number representation.
Constructed planar UVs are genuine rational values, not always dyadic numbers.
Neither double, long double nor fixed128-bit integers implement the current
unbounded Fraction contract. Per-face integer/homogeneous coordinates and
filtered exact predicates are promising representations, but their range and
fallback must be explicit. Any proposed external numeric dependency needs the
operator's concrete benefit/cost/tradeoff discussion before adoption. No numeric
library is selected by this proposal.

See the [numerical and ownership seam analysis](2026-10-15-cpp-migration-numerical-seams.md).

## Expand ownership in complete slices

1. Adapt the relevant rigor mechanisms to a new owned native CPU target and its
   actual headers/tests. Use relevant existing geometry oracles and scoped
   instrumentation; add no blanket requalification campaign to the watch.
2. Deliver the sampled-face engine with indexed candidates and unchanged exact
   geometry/source semantics. Check an isolated difficult face and meaningful
   literal degeneracies before any whole-target retry; measure actual candidate
   work, output and wall without claiming a speedup in advance.
3. Move shared edge sampling/scheduling, carrier-bound refinement and mesh output
   into the native owner as separate complete units, prioritizing measured cost.
   Bind caches to exact source/preparation bytes and edge-schedule generations.
4. Add explicit CPU concurrency once shared schedules are fixed. Use immutable
   inputs, worker-owned scratch, bounded concurrent memory, safe join/failure
   handling and canonical output/identity allocation. Measure one-core and full
   CPU execution separately. Shared edge schedules cannot race across faces.
5. Connect native source/assembly/history and retained CPU/GPU consumers into a
   whole workflow with explicit load/update/execute/shutdown. Retire production
   Python ownership slice by slice; keep optional bindings thin and independent
   oracles independent. GPU migration then acts on stable native data/lifetimes.

This order is a proposed migration path, not authorization to implement all five
units. The first design must make exact arithmetic, native input/output ownership
and the chosen geometry contract concrete before any source change.

## Compiler and numerical boundaries

A C++20 CPU core is a reasonable baseline. The execution host already has
Clang18.1.3, clang-tidy18, clang-format18, CMake3.28.3 and Ninja1.11.1 installed;
version/path inspection is not qualification of a new BRepGPU target. The sibling
uses that Linux compiler with libstdc++13 and a separate newer Mac profile.
Preserve the current qualified CUDA12.8.1/sm60 plugin initially. Our C function
tables and value buffers allow separate compilation without exposing STL objects
or exceptions across the boundary.

NVIDIA's [CUDA12.8.1 host-compiler table](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-installation-guide-linux/index.html#host-compiler-support-policy)
supports Clang7–19 and GCC6–14 on x86_64, with C++20 supported under the stated
compiler requirements. Copying a Clang23 Mac pin into NVCC would exceed that
host-compiler range. No CUDA upgrade or unsupported-compiler bypass is needed
for this CPU migration. A later device dialect change gets its own affected
checks rather than being bundled into the language decision.

Keep the existing strict floating-point settings, explicit rounding/error
budgets, exact saved-value predicate fallbacks and CPU/P100 numerical oracles.
Sanitizers and compiler warnings complement those contracts. They do not replace
them or make a new valid topology a consequence of successful compilation.

## Stop point

Study complete. The recommended direction is native C++ production geometry,
Python independent verification and optional scripting, with the first complete
native geometry slice addressing the measured searches. Implementation remains
held for the operator's next direction. Existing watch compute stays untouched.
