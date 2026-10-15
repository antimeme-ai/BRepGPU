# External qualification is required for alpha

Operator direction, 2026-10-14: broad community/reference suites may be red,
unported or unsupported during development. **BRepGPU alpha cannot be cut until
its required external qualification is green.** Local contract tests, selected
mutants, reference-build success and attractive artifacts cannot replace it.
Current external qualification is incomplete; this document claims no passes.

## Development and release have different gates

Continue the breadth-first sweep with direct geometry/model/metamorphic tests
for each declared operation. Run external cases as capabilities and adapters
arrive; record failures and unsupported input honestly. Broad external failures
do not automatically block unrelated development commits.

Before alpha, define the advertised operations/input families and map the
complete relevant upstream suite inventory to them. Select coverage from
external operation families, including adversarial historical cases, before
observing which examples pass. An unimplemented case required by the advertised
alpha remains blocking; it cannot automatically become out of scope. Changes
to the promised alpha scope or required corpus need explicit operator review.
Unrelated graphics/UI/internal implementation tests can be excluded with a
specific reason; such exclusions are not BRepGPU passes.

Required cases must exercise **BRepGPU's actual output** through owned adapters.
Running an unmodified reference suite checks that reference build. Preserve
original case identity, inputs, expected checks and source revision while
translating operations. Compare semantic geometry, material/region membership,
incidence, coverage and error behavior. Different valid chart/edge splitting
must not fail solely because entity counts or parameterizations differ.

Missing data, unported adapters, required unsupported geometry, unresolved
oracle disagreements, crashes and failures are all non-green for alpha.
Skipped/not-run/xfail/known-BAD outcomes cannot count as passes. Preserve upstream
known-problem annotations separately; determine the actual geometric contract
instead of inheriting a known defect as the expected BRepGPU answer. No empty
suite or small handpicked passing subset may satisfy the release requirement.

## External candidates and what they can establish

| Candidate | Required role when applicable to alpha | Limits to preserve |
| --- | --- | --- |
| Pinned OCCT DRAW regression groups and complementary GTests | Broad analytic B-rep geometry/topology, sections, splitting/Booleans, meshing and exchange recipes; retain historical failures and upstream checks | DRAW scripts require adapters; some fixtures are separate/confidential. Tolerance and known-BAD semantics need explicit disposition. Some assertions are only successful status/counts |
| Pinned CGAL exact circular geometry and arrangement tests plus an independent reference executable | External exact answers for circle/segment predicates, arrangements and point location; compare our actual current-family outputs | CGAL is not an entire curved B-rep kernel. Upstream tests validate CGAL; differential input/output comparisons validate BRepGPU. Monotone decomposition differs from our physical edge representation |
| Public NIST/CAx-IF exchange/conformance fixtures | Applicable advertised STEP/interchange fidelity and validation properties | Models/properties do not supply general SSI/Boolean answers; acquire only data with suitable access/redistribution rights |
| Appropriate community meshing/mesh-CSG corpora | Applicable tessellation, mesh embedding/manifold/Boolean interoperability claims | Polygonal tests do not certify analytic trimmed-surface construction; model collections without expected operations are input corpora, not answer oracles |

The strongest broad public B-rep starting point found is OCCT's corpus; there
is no established single vendor-neutral complete B-rep certification suite
identified by this research. Use independent sources for their actual claims,
not an invented universal certification. Pin reference versions, case/data
hashes, tolerance/exactness conventions and backend placement when selecting
the release matrix. Exercise every advertised execution backend where the
operation is promised; fail visibly rather than silently falling back.

## Current gap and next implementation unit

The repository has studied/retained OCCT source and uses owned analytic/model
oracles. It does not yet replay community B-rep cases against our operations
or compare current arrangements with a separately executed exact kernel.
The initial fleet inventory found no DRAWEXE or OCCT/CGAL development setup in
the queried system locations. Since then the OM10 work has built an isolated
pinned OCCT source/query bridge and acquired the actual NVIDIA runtime; these
are usable for their declared adapters. They do not establish an installed
DRAW regression runner, a CGAL exact reference or community-case replay.
Runtime GMP/MPFR libraries alone are not that exact geometry reference build.

Next, produce the pinned upstream case/data inventory, classify coverage against
the alpha capability matrix, and implement separately built reference runners
and owned operation/result adapters through researched/reviewed design and plan.
Start actual comparisons on currently supported geometry while keeping the
remaining community coverage visible as release debt. Do not wait until release
day to discover data/adapter gaps. Test-only reference library adoption must be
discussed with the operator before installation/adoption; this policy selects
no production dependency or shipping-language architecture.

The release run must identify the exact candidate source/build, complete
required case inventory, actual BRepGPU/reference outcomes and all non-pass
dispositions. Every required case must pass at that candidate before tagging
alpha. No separate ceremonial score or certificate substitutes for these runs.

## Mutation testing's bounded role

The current manually selected mutants check that particular assertions detect
named mistakes in existing operations. They are useful against vacuous tests
and missing assertions, even in a young suite. They do not discover missing
requirements, justify narrow input coverage, establish oracle independence or
qualify general B-rep behavior. A selected eight-of-eight kill result describes
eight injected faults; it is not a mutation coverage score or alpha readiness.
Reports should lead with geometry coverage, independent truth and external
qualification status; targeted mutant outcomes are supporting detail.

## Narrow carrier exchange evidence

The interchange beachhead now reads a pinned upstream mouse fixture and an
actually exported foreign sheet, checks owned source/control/native jets and
compares scalar reference outputs on the actual same sources. This is real
limited exchange evidence; it does not replay the complete required OCCT/NIST/
CAx-IF exchange corpus or establish applicable alpha coverage. Two multispan
mouse faces and every source trim remain unsupported/uninterpreted. Required
missing adapters/data/cases continue to be non-green; no alpha scope is changed.
See docs/INTERCHANGE.md and results/interchange-2026-10-15/REPORT.md.
