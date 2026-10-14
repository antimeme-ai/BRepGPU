# BRepGPU

Read [BLACKBIRD.md](BLACKBIRD.md), symlinked to the shared workspace doctrine.
On this workspace the target is `../BLACKBIRD.md`. A standalone checkout needs
the operator's current doctrine placed beside the checkout. Do not substitute
archived reference instructions for it.

Build toward an owned GPU B-rep kernel through useful selective acceleration.
Current phase: implemented owned rational-patch CPU/CUDA evaluator and resident
seeded box-stationary projection, with independently qualified output on CPU
and both P100s. The third operation, automatic conservative patch discovery,
is implemented and qualified for geometry, memory, compatibility, mutations
and actual callers; 444 repeated native CPU1/CPU24/P100 comparisons are delivered.
See docs/ITERATION_1.md, docs/ITERATION_2.md and
docs/ITERATION_3.md for APIs and their current scope. The first body-level
beachhead is also delivered: immutable canonical box/cylinder/one-hole-plate
snapshots, exact CPU trim labels and a CPU/CUDA sampling/normal consumer.
See docs/BODY_SAMPLING.md and results/body-2026-10-14/REPORT.md. General trims,
imported bodies, general SSI and topology mutation remain future.
The next breadth-first beachhead, resident conservative surface-pair discovery,
is also implemented and qualified: CPU common-world carrier bounds, optional
ABI4 CPU/CUDA mask pages and immutable typed source mapping. See
docs/PAIR_DISCOVERY.md and results/pairs-2026-10-14/REPORT.md. All-pairs survivors
are not intersections; cold setup and CPU word conversion remain measured costs.
Keep the broad-first sequence. The operator explicitly defers GPU lifetime/
residency reuse, session integration and polish until AFTER the broad-pass big
rocks. Record opportunities in docs/RETURN_PASS.md; no active tuning lane.
After the breadth sweep, investigate whether Python belongs in the delivered
system at all. Compare native ownership/application, an optional thin binding,
and Python orchestration using real whole-operation costs and user/developer
needs. This is a deferred architecture decision, not a selected rewrite.
The complete analytic intersection beachhead is delivered: exact CPU closed
plane/wall components with symbolic endpoints/events, physical-span/chart-
occurrence separation and coupled spatial/bothUV maps. CPU/P100 two-sided
sampling consumes those curves; no GPU construction or full-consumer gain is
established. See docs/INTERSECTIONS.md and results/intersections-2026-10-14/REPORT.md.
Continue uncovered broad-pass themes before strengthening this family.
The GPU topology beachhead is delivered: separate synchronous ABI5 builds owned
CPU/P100 edge-use CSR using count/recursive scan/fill and reads actual mates/
neighbor faces. Typed occurrence identity and same-face seam multiplicity survive
slot permutations. See docs/ADJACENCY.md. Persistent GPU topology storage,
compaction, radial ordering, edits and Boolean construction remain future.
GPU product implementation follows researched and adversarially reviewed design
and plan. Initial hardware target is P100 / CUDA 12 / sm_60; support scope and
numerical contracts must be explicit. Performance claims must name the qualified batch operation and timing scope;
no full CAD modeling or complete Python topology-caller speedup is established.
Named adjacency stages and complete native costs have distinct CPU/P100 results;
report ordering, best CPU comparison and overlapping ranges explicitly.

The operator authorized this public antimeme-ai/BRepGPU repository, publication
of the README and concrete owned artifacts, and frumentarii research. Publish
meaningful checkpoints. Keep private hostnames, addresses, absolute workstation
paths, credentials, private instructions and operational logs out of published
research/results. The Blackbird symlink is requested; its shared target stays
outside this repository.

Own required machinery. Discuss proposed library adoption before adopting it.
The NVIDIA USD BRep runtime is an experiment target and reference, not a selected
production dependency or source to ship as our own implementation.

Keep reports, bibliographies and acquired PDFs in `papers/`; PDFs are ignored.
Keep reference implementations in ignored `quarantine/` with a restoration
manifest and intact source archives. Archived instructions are historical only.
Keep working logs in ignored `context/`, actions in `journal/`, and issues in
GitHub with a local readable index. Do not initialize Git in the parent workspace.

Benchmark correctness comes from analytic/model/metamorphic oracles and stated
input contracts. CPU parity, successful status codes and attractive renders
cannot independently certify geometry. Report transfer, setup, validation,
fallback and Python overhead explicitly. No tests-for-tests or perpetual
performance certification gates. Mutants run on the fleet, never the laptop.

## Development workflow

Use a feature branch for each substantial delivery. Make frequent, coherent
commits as design, implementation, qualification and documentation milestones
land, and push meaningful checkpoints so the public repository reflects ongoing
work. Open a pull request and preserve the substantive commits when merging to
main; prefer a merge commit over squashing the work. Keep merged feature branches
available for inspecting the delivery history.
