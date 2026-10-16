# Native CPU rigor for the first face-domain unit

Operator-authorized adaptation of the inspected C++ tooling patterns. This is
one small build/analysis unit supporting the new exact sampled-face engine;
it selects no production dependency or CUDA change. The old live watch checkout
and existing qualified files/binaries remain untouched.

Use a separate fleet checkout, explicit Clang18.1.3/libstdc++13 and C++20 for
`native/face_domain`. CMake3.28.3/Ninja1.11.1 are already installed. Verify actual
tool versions, reject unknown instrumentation/profile values and refuse a
foreign compiler rather than silently selecting one. Existing C++17/CUDA targets
retain their builds. No TUI/Lua tools or blanket sibling-project source gate
are imported by this adaptation.

## Build and analysis contract

An owned `tooling/native_cpu_rigor.cmake` supplies a target function used only
by the new module and its direct native fixtures. Required warnings:
Wall/Wextra/Wpedantic/Werror, conversion/sign-conversion, shadow, non-virtual
destructor, old-style cast, overloaded virtual, format=2, undef and implicit
fallthrough; reject unknown required warning options. Keep no-fast-math and
FP contraction disabled. Set C++20 required, extensions off and emit the actual
compilation database. Test targets use the same build settings as their library.

Profiles: debug; optimized release; address+undefined with no recovery and
frame pointers; separate thread instrumentation when real concurrent owner
behavior is exercised; fuzz instrumentation for a selected input/property target.
A profile's availability is not a claim it has run or proved geometry. No
third-party SDK links or sanitizer exclusions belong to this new exact engine.

A small owned fleet CLI (`scripts/check_native_face_domain.py`) configures/builds
the selected profile, runs the selected actual tests and/or source analysis,
with target/output roots explicit. It must not start unrelated whole-repository,
CUDA, mutation or performance campaigns. Missing tests and nonzero child exits
fail visibly. The eventual native fuzz target requires its own meaningful input/
property oracle; no generic byte demo is presented as a domain fuzzer.

Native lint checks only the owning module/native fixture source set: formatter18,
focused clang-tidy18 checks (use-after-move, unchecked optional, easily-swappable
parameters, unnecessary copies/value parameters and Static Analyzer). Analyze
real compile database units, with owned headers reached by compiler dependency
reports; no unattached owned header silently escapes. Exclude acquired/system
headers and generated build files. Unknown/missing checks or wrong compiler in
the actual database fail. Syntax/style probes do not become geometry oracles.

## Minimal direct tooling evidence

Before module implementation, the first actual direct domain fixture is RED
against the missing capability, as specified by its owning plan. The tooling
unit's narrow direct cases use an actual one-file temporary CMake target to show
clean C++20 compilation and required compiler/warning rejection, then known
heap-overrun and signed-overflow failures under the instrumented profile.
Run these only on the fleet. Reuse installed tool evidence where adequate;
this is initial profile wiring, not a recurring certification layer. Native
format/tidy diagnostics run on the actual changed module once it exists.

Source changes use focused direct behavior, relevant native instrumented tests
and actual owning-source analysis. Documentation uses diff/link checks. Relevant
work only: no broad sanitizer repetition without changed paths/concerns, no
mutation campaign and no extra review of tooling summaries. Root reviews the
small target function/CLI before use and records actual profile results.

## Performance separation

Optimized one-core performance uses matching compiler flags/counter settings,
input/source hashes and quality. Include Python normalization/packing, native
copy/rank/index, solve and output; display isolated stages separately. Record
all repetitions, candidate/predicate/ear work and whole-process RSS. Sanitized
and debug timings cannot substitute for release comparisons. Test coverage and
validity alone cannot satisfy this delivery: the reviewed native plan requires
sparse scaling and the actual difficult-face result before a whole-watch retry.

Independent algorithm/implementation-boundary review ACCEPT. Root accepts this
small tooling plan; proceed with the scoped target wiring and direct fleet
setup checks, without changing the old live checkout.
