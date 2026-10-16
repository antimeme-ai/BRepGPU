# Direct CDT predicate research

Read the current owned `native/face_domain/exact.hpp`, native compiler profile,
Python weighted Lawson predicate, earlier connectivity proposal, and retained
Shewchuk/Attene papers. Acquired and read the original versioned Bartels,
Fisikopoulos and Weiser 2022 paper on fast underflow-aware filters; acquisition,
hash and restoration are in the new research report.

Recommended filtered exact predicates as part of the selected architecture
before implementation: face-scoped scalar bound, local outward interval
filter, exact integer fallback. Derived positive power-of-two scaling that
compensates anisotropic metric weights, exact leading-limb input enclosures,
and the requirements on underflow, FMA, compiler transformations and runtime
floating state. Reused the proven anchored exact-lift column identity and
added a checked fixed-width specialization as a contender. These are general
input-driven methods; none branches on the watch or its dimensions.

Saved `papers/2026-10-16-direct-cdt-predicate-design.md`. Sent root the feasible
cascade and the key warning: exact-on-binary64 expansion predicates applied to
rounded rational UVs prove the wrong geometry. No product code, geometry run,
benchmark or test was executed. The study does not assert a timing target or
performance gain. Root owns design synthesis and implementation selection.
