# Independent oracle audit and the BRepGPU alpha gate

The existing suite establishes useful, narrow claims about implemented machinery.
It does not establish general curved B-rep correctness or readiness for an alpha.
Our handpicked mutant campaigns show that particular assertions reject particular
faults; their perfect kill counts are not a measurement of the unexamined fault
space. The operator's new policy applies: community suites can remain red during
development, but the required applicable community cases must be GREEN before
cutting a BRepGPU alpha. Current results do not satisfy that release gate.

## What the current tests establish, and where they stop

Read the actual `tests/test_body.py`, `test_intersections.py`,
`test_classification.py`, `test_arrangements.py`, `test_adjacency.py`, their oracle
modules and mutation controllers. Existing independent mechanisms are materially
stronger than CPU/GPU parity or a visual check:

- Adjacency compares complete typed coedge memberships with a dictionary/list
  relation, rather than repeating native CSR count/scan/fill. Literal original
  loops, opposite signed uses, same-face seams, permutations, malformed admission,
  direct native checks and memory/fault campaigns exercise different failures.
- Classification compares exact world material constraints and all supporting
  original face IDs. Rectangle contact uses a complete corner/halfspace model
  rather than the product's interval construction. Subnormal and one-ULP traps
  directly distinguish exact from rounded decisions.
- Arrangement qualification enumerates every admitted source cell/disk domain,
  complete directed outer/hole/exterior loop words, source subranges, original
  maps/events and all closed incident regions. It requires completeness for its
  rectangle/grid plus strictly interior disjoint-circle family, rather than
  accepting Euler/area totals alone.
- SSI whole-map identities and retained source constraints check every point on
  returned rational pieces. Importantly, `intersection_oracles.qualify_result`
  explicitly says it is **not arbitrary completeness**. A valid returned subset
  is not automatically the full intersection; literal recipe expectations and
  request-specific checks supply the additional completeness coverage we have.

Those mechanisms remain concentrated on our own box, cylinder and holed plate,
our own naming/orientation/chart conventions, and descendants constructed from
those records. Both product and oracle usually consume the same owned body
factory records; some expectations are independently hardcoded canonical
primitive facts, while others deliberately treat original records as input truth.
A shared error in what that input is supposed to mean can survive both sides.
An incorrect import, unit conversion, periodic parameter interpretation or trim
convention will not be exposed by an all-owned-factory workload.

There is intentional oracle reuse: arrangement qualification calls the existing
SSI qualifier, which calls the body qualifier. This avoids redundant copies of
the same proof, but it is correlated coverage, not three independent votes.
The native CPU/GPU paths also share contracts, representations and some arithmetic
choices; parity cannot eliminate those correlations. Literal examples and exact
models reduce these risks in the admitted family; they do not remove the need for
foreign inputs and independent operation semantics.

The five arrangement caller recipes exercise scale and topology size, including
1,024 cells/64 circles, but they do not exercise arbitrary circle clipping,
tangency, overlap, nesting, free trim curves, arbitrary rotations, nonuniform
surface parametrizations or general NURBS. Repeating a canonical construction
many times is a performance workload, not equivalent geometric diversity.
No external STEP/BREP/community-case execution adapter is exercised by the actual
Python tests reviewed. The current product has no general imported curved B-rep
qualification path. That missing path is an alpha blocker, not an excuse to label
the community corpus inapplicable after it fails to enter the program.

## What the mutant counts mean

`run_arrangement_mutations.py` selects eight exact textual production sites and
then runs one assigned direct test per mutant. The seven classifier mutations
follow the same approach. Both demand healthy baseline/retry and genuine
AssertionError test failures; syntax/import/infrastructure errors do not earn
kills. These are useful fault injections. Examples include missing holes,
incorrect twins, parameter reversal, wrong UV side, omitted boundary incident
cell, wrong hole material sense, missing contact corner and rounded radial sign.

The limitation is selection: we chose the fault types and mutation sites while
knowing the current tests and the contract. We did not enumerate a justified
mutation-operator population, measure survivors, test an independently withheld
fault set, or cover all interacting branches. Several faults are intentionally
large and straightforward, such as making every use orientation positive.
Eight kills show eight fault classes have witnesses, not that the suite is eight
units closer to complete CAD robustness. Older campaign controllers also differ
in their failure filtering; do not retroactively describe all campaigns using the
strictest later controller's guarantees.

For alpha, retain these regressions and add a bounded, independently selected
mutation population across arithmetic signs/comparisons, multiplicity,
classification boundaries, loop ownership, source provenance and error paths.
Publish the complete attempted set and outcomes: assertion killed, survived,
equivalent with concrete rationale, invalid/unreachable, and infrastructure
error. Preserve survivors as unresolved evidence; do not remove difficult mutants
to retain a 100% headline. Include coupled faults and boundary perturbations where
they represent plausible real defects. Mutation sensitivity supplements direct
foreign-case geometry truth; it does not replace it.

## Is there a canonical, universal B-rep oracle?

This bounded primary-source review did not verify one public suite that is an
accepted, exhaustive, vendor-neutral answer oracle for all curved B-rep modeling.
There are several valuable resources with different purposes. The absence of a
single universal oracle is no reason to defer using them.

**OCCT regression suites are the most immediate source of deep modeling cases.**
Current OCCT documentation describes DRAW regression tests, C++ unit tests,
separately located geometry data, confidential cases and upstream statuses that
allow known BAD cases or missing-data SKIPPED cases in a non-regressive run.
Thus “no upstream regressions” does not mean every geometry case was executed
and correct. Do not import those status exceptions into our alpha gate.
[OCCT automated testing system](https://www.occt3d.com/dev/doc/overview/html/occt_contribution__tests.html)

We already have pinned OCCT 8.0.1 reference source at commit
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`. A local text inventory found 4,109 files
under `quarantine/new-iteration/occt/tests/boolean`, of which 2,078 contain
`locate_data_file` and 17 contain `TODO`; these are **file counts, not a qualified
test count**. Self-contained cases already available include
`boolean/bopcommon_simple/A1` (identical boxes), `ZC6` (cylinder/box common), and
`boolean/bopsection/A1` (historical prism/cylinder section regression).
Some assertions are area/length plus shape/view checks; their geometry and
operation intent should be retained and supplemented with our semantic checks,
not mistaken for complete expected topology. We have studied OCCT machinery;
we have not yet adapted this broad case field into our executable gate.

**NIST and CAx-IF provide neutral interoperability/conformance evidence.**
NIST's STEP analyzer checks entity/attribute information, PMI/validation-property
conformance and basic format errors. Its public CAD/PMI cases expose actual
foreign geometry and documented intended annotations. They are useful importer
and preservation tests, not a solver that gives the complete answer to arbitrary
surface intersections or Booleans.
[NIST analyzer](https://www.nist.gov/services-resources/software/step-file-analyzer-and-viewer),
[NIST test cases](https://pages.nist.gov/CAD-PMI-Testing/models.html)

CAx-IF's geometric validation properties distinguish solids, independent surfaces,
curves and points, with measures such as volume, area, centroid or curve length.
These are valuable preservation checks. Equal scalar aggregates do not determine
a unique shape, adjacency, trim or Boolean result, so they must accompany local
geometry/topology qualification.
[CAx-IF geometric/assembly validation properties v4.6, §4.6](https://www.mbx-if.org/home/wp-content/uploads/2024/05/rec_prac_gvp_v46.pdf)

The actual Round 58J suite primarily tests AP242 interoperability, including PMI,
assemblies, composites, kinematics and persistent IDs. Its §1.4 distinguishes
public-domain NIST cases from production/JAMA data with redistribution or usage
restrictions. Acquire only fixtures we are allowed to use and retain each source's
terms; a public test specification does not grant redistribution of every model.
[CAx-IF Round 58J v1.2, §§1 and 1.4](https://www.mbx-if.org/home/wp-content/uploads/2026/09/test_suite_58j_v12.pdf)

**ABC is a rich B-rep corpus, not an operation answer oracle.** The original paper
provides parametrized curves/surfaces for geometric-learning ground truth such as
normals/features, and its processing pipeline uses Open Cascade. The dataset
publishes STEP converted from Parasolid and original Parasolid records; files and
auxiliary formats can be missing from processing errors. Use it for independently
selected foreign topology/trim/geometry diversity, then attach explicit operation
requests and independently justified expectations. Do not call input parametric
surfaces ground truth for the output of an unprovided Boolean operation.
[ABC paper](https://openaccess.thecvf.com/content_CVPR_2019/html/Koch_ABC_A_Big_CAD_Model_Dataset_for_Geometric_Deep_Learning_CVPR_2019_paper.html),
[ABC formats/terms](https://deep-geometry.github.io/abc-dataset/)

**Mesh CSG resources attack an important adjacent domain.** Lévy's ThingiCSG
contains OpenSCAD CSG cases and compares mesh backends; Thingi10K stresses mesh
intersections and repair. CGAL's corefinement/Boolean contract concerns triangulated
surface meshes, with specific self-intersection/manifold/output restrictions.
They can stress polyhedral cases and supply useful differential computations,
but a tessellated cylinder does not preserve the exact curved B-rep, pcurves,
periodic seam and tolerance contract we need to qualify.
[ThingiCSG paper, §3](https://arxiv.org/html/2405.12949v1),
[CGAL corefinement/Boolean semantics](https://doc.cgal.org/6.0/Polygon_mesh_processing/index.html#title6)

## Concrete alpha qualification policy

Development can proceed with published community failures. An alpha cut must
satisfy all of the following for its declared required domain:

1. **Declare coverage before choosing successful fixtures.** Maintain a versioned
   case manifest tied to upstream revisions/IDs, available data hashes, operation
   intent, geometry features, expected result semantics, tolerances and required
   platforms/backends. Include entire applicable upstream groups or an explicit
   feature-stratified, deterministic selection rule independent of our outcomes.
   A diversity subset is acceptable only if its bounds and excluded population
   are visible; it must not be described as the whole community suite.
2. **Make admission independent of product success.** Determine applicability from
   the fixture's geometry and the release's stated requirements, not whether our
   importer recognizes it or an operation returns success. For a required case,
   missing data, license/access blockage, absent adapter, unsupported geometry,
   timeout, crash, unresolved oracle or upstream TODO is blocking non-green.
   An exception expected by a malformed-input test can pass that test's rejection
   contract; it never passes the corresponding valid modeling case.
3. **Adapt foreign geometry honestly.** Preserve original surfaces, trim domains,
   world frames/units, face orientation, seam multiplicity and source identity.
   Do not rebuild every foreign fixture as one of our primitives, heal or drop
   troublesome records silently, or run an independent kernel while crediting
   BRepGPU. Qualify the importer separately from the requested modeling result.
4. **Require more than scalar or structural plausibility.** Attach complete
   literal/model expectations where possible; otherwise use justified geometric
   distance/residual bounds, trim containment, complete branch/region checks,
   material witnesses, shell/component/genus expectations and operation-specific
   invariants. Predeclare tolerance semantics. Check original surface↔3D-curve↔
   both-pcurve correspondence and every resulting adjacency/loop/seam. Equivalent
   valid topology subdivisions should pass geometric/semantic equivalence rather
   than require identical serialization or face counts from another kernel.
5. **Use independent reference implementations with explicit limits.** Compare
   against pinned OCCT and an independently implemented reference where available,
   plus exact analytic cases and metamorphic transforms/operand reversal and
   Boolean identities. OCCT-derived import and OCCT-derived expected answers are
   correlated; majority agreement is not proof. Mesh references qualify their
   precise polyhedral interpretation only. Investigate disagreements rather than
   choose whichever output ours matches. Reference uncertainty is non-green.
6. **Publish the full denominator and gate.** Required cases must all pass direct
   semantic expectations on the exact release artifacts; no automatic upstream
   xfail/skip/TODO mapping. Show failed, unrun, missing, unsupported, timed-out and
   unresolved counts alongside passing counts, with original IDs. Out-of-scope
   capabilities may be identified explicitly before results and remain visibly
   unqualified; changing scope to escape failures requires a reviewed change to
   the alpha claim and manifest, never silent exclusion or a green status for the
   excluded case. A blanket B-rep alpha claim cannot be justified by a tiny scope.

Minimum required coverage should include foreign trimmed analytic and rational
surfaces beyond our canonical factory, multiple knots/weights/degrees, periodic
and singular charts, degenerate/seam edges, inner loops and multiple shells,
independent bodies with coincident geometry, units/frames/large and tiny scales,
near tangency/coincidence/slivers, and operation sequences that consume previous
results. For each feature claimed in alpha, attach applicable community
regressions plus exact/adversarial model cases and complete result readers.
The gate must expose missing families rather than turning an empty family green.

The next concrete work is corpus inventory/admissibility and adapter design,
starting with the already available self-contained OCCT cases; then original
foreign fixtures with clear rights, NIST import-preservation cases and a
predeclared ABC diversity cohort. New modeling capability and adapters can land
while that scoreboard is red. Alpha remains blocked until the required population
and all other release checks are green. No community adapter or new dependency
was adopted in this research task.

## Unverified candidate, not a recommendation

ForgeCertify advertises vendor-neutral kernel certification and published tests.
Its own methodology treats undeclared capabilities as skips and its quick start
says those skips are not failures. Runnable source/assertions were not verified
in this bounded review: the linked releases could not be fetched through the
research tool and a direct page request returned HTTP 403. This is a lead to
investigate, not evidence of a canonical accepted oracle or an alpha gate we
already satisfy. Even if verified, its skipped-capability policy would need our
stricter required-case interpretation.
[Claimed methodology](https://forgecertify.org/about),
[claimed runner/downloads](https://forgecertify.org/downloads)
