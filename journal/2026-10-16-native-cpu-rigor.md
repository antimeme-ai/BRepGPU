# Native CPU rigor implementation

Implemented the independently reviewed scoped tooling plan for the first native
sampled-face unit. Its host-only C++20 target requires Linux Clang18.1.3 and
libstdc++13; existing CUDA/C++17 targets keep their separate settings. The fleet
doctor verified actual compiler, formatter, analyzer, CMake and Ninja versions
and required named analysis checks. Initial missing CLI entry was RED before
implementation; it establishes only the absent tooling seam.

Actual temporary native CMake targets using the owned target helper compiled and
ran a clean C++20 span consumer, then caught a real heap overrun with ASan and a
real signed overflow with UBSan. Both faulty processes failed with the expected
instrumentation diagnostics. Unknown profile and instrumentation configuration
were rejected. Raw sources/output remain private. These probes establish initial
instrumentation wiring, not geometric coverage or a recurring test campaign.

Independent review found the doctor checked the default standard-library release
while a CMake include/flag override could change the actual unit. Accepted and
fixed with an owned forced-include profile header in every affected target and
its real dependency analysis. A separate actual compiled-unit probe deliberately
changed `_GLIBCXX_RELEASE` to12 after including `<version>`; the target refused to
compile with the required libstdc++13 diagnostic. Default actual module builds
select release13. Target-scoped flags, required warning rejection, actual
compilation-database units and owned-header reachability were reviewed.

The CLI runs actual native CTest fixtures and explicitly selected Python tests.
Instrumented shared libraries require native fixtures or an explicitly selected
matching host; it refuses to present an uninstrumented Python host as an ASan
run. Thread/fuzz profile settings exist, but no TSan/fuzz claim is made. Actual
module lint and focused arithmetic/geometry/malformed/lifetime sanitizer results
are recorded with the module delivery. Optimized performance is measured
separately, including the Python preparation cost.
