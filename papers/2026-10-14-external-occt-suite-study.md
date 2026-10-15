# External qualification: OCCT DRAW and public regression corpus

2026-10-14. **Recommendation: adopt the pinned OCCT public modeling regression
corpus as an external alpha qualification requirement, with an owned execution
adapter that exercises BRepGPU.** Community suites need not be green at every
breadth-first stage; the operator requires them green before a BRepGPU alpha.
That requirement must not be reduced to running stock OCCT, a few selected
examples, or accepting unsupported operations as passes. No production changes,
test execution or dependency adoption occurred in this study.

## What exists and what was actually read

The existing quarantined OCCT 8.0.1 snapshot is commit
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`. Its intact archive/restoration recipe
is already recorded in the quarantine manifest and acceleration source catalog.
Read its automated-testing guide, DRAW group/grid lists, inherited begin/end
scripts, runner/checker implementation, official CI invocation/data acquisition,
and concrete geometry, classifier, SSI, Boolean, face-building, meshing and STEP
cases. See the accompanying readings and group inventory TSVs for exact files
and source hashes. This is not a claim to have read or executed every case.

Artifacts: `external-occt-sources.tsv`, `external-occt-readings.tsv`,
`external-occt-groups.tsv` and `external-occt.bib`.

Current official 8.0.1 documentation confirms DRAW Tcl regression scripts and
the complementary `OpenCascadeGTest` C++ runner. Thus “the OCCT suite” should
identify both its selected DRAW corpus and relevant migrated C++ tests. The
deprecated GitHub wiki is not the primary current guide. [Official test guide](https://occt3d.com/dev/doc/overview/html/occt_contribution__tests.html),
[pinned guide source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/dox/contribution/tests/tests.md).

Static enumeration of registered groups and grids, resolving `cases.list`
shared script directories, finds **40 groups and 21,006 case registrations**.
These span 348 existing registered grids and 18,514 distinct script paths;
shared scripts can execute under different grid parameters. Registrations are
not successful executions.
There are also 480 `src/**/GTests/*.cxx` source files; that is not a test count.
Actual DRAW enumeration must settle executable cases, platform conditions and
data/plugin availability. A case-only data-reference grep is insufficient:
mesh and exchange cases often set a filename while an inherited end script
loads it. Neither a small grep count nor a missing inline reference establishes
self-contained input.

## Public data and unavailable tests

The pinned official CI downloads this archive even for its 8.0.1 source:
[opencascade-dataset-7.9.0.tar.xz](https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_0_beta1/opencascade-dataset-7.9.0.tar.xz).
A source-only HTTP HEAD check returned 200 and Content-Length 98,739,184 bytes.
The archive was not acquired or inventoried by this study; a successful HEAD
does not prove fixture completeness, licensing for redistribution, or content
hash. The source tree also contains a small `data/` tree, and individual tests
can include fixture files. Preserve acquired archives, hash them, extract into
ignored quarantine, and record each fixture's exact source and availability.
[Pinned CI data acquisition](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/.github/actions/run-tests/action.yml).

The official guide explicitly distinguishes public tests from confidential
fixtures only available to Open Cascade. Missing data is normally SKIPPED.
There is no honest route to claim full internal Open Cascade certification
using a public archive. Record unavailable cases individually and seek public
reproducers or authorized acquisition. Missing public required fixtures block
our alpha gate; confidential unavailable fixtures remain visible exclusions,
not green tests. The public scope must be declared before results are known.

## Relevant corpus, beyond handpicked successes

| Theme | Pinned DRAW groups/grids | Registrations in whole named group |
| --- | --- | ---: |
| Curve/surface geometry | geometry: lines, circles, conics, Bezier/B-spline, projection, revolution | 232 |
| Low-level algorithms | lowalgos: intss, 2dinter, extcs/extcc, classifier, bounds/proximity | 348 |
| Boolean/topology | boolean: section/common/cut/fuse, 2D, cells, splitter, history, periodicity, open solids | 4,093 |
| Regression history | bugs: modalg_1…8, moddata_1…3, mesh, heal, splitshape, STEP/IGES and others | 4,244 |
| Face reconstruction | mkface: edges_to_faces, plane and trimmed/offset variants | 165 |
| Healing/closure | heal; sewing | 2,739; 700 |
| Meshing | mesh; bugs/mesh; de_mesh | 699; included in bugs; 117 |
| Exchange | de STEP/IGES; de_wrapper; bugs/step/iges; xcaf | 1,084; 49; included in bugs; 521 |
| Feature construction | blend/chamfer/offset/pipe/prism/feat/thrusection | separate inventory rows |

The whole public **modeling** set must include adversarial regressions and
general geometry beyond today's three owned primitive bodies. Current inability
to import an arbitrary BREP, build a sphere or execute a Boolean is visible
work remaining before alpha, rather than a reason to silently remove that
case. Rendering, UI, OCAF document transactions and platform font checks need
explicit applicability decisions for a kernel alpha. Exchange attributes such
as colors/PMI/assembly labels may similarly exceed a narrowly declared exchange
API, but geometric import/export checks remain required. The scope decision
belongs in the alpha design and must not be made by filtering failures after
the run. The Linux upstream CI itself has an explicit exclusion list, including
SSI, Boolean and display cases; blindly inheriting that list would hide scope.

## Actual expected answers have different strength

Read examples illustrate why a famous corpus is valuable without being a
universal oracle:

* `geometry/circle/A1` constructs a unit circle, converts it and checks length
  against 6.2831853071795853. It supplies external construction/measurement truth.
* `lowalgos/classifier/buc60609` asserts two face-UV points IN;
  `bug22494` requires ON for a specific historical boundary point. These use
  imported fixtures and tolerance semantics, unlike our exact material test.
* `lowalgos/intss/bug139` requires a closed intersection with zero alone vertices.
  `bug106` largely requires intersection curves to exist: useful regression
  history, weak geometric completeness truth.
* `boolean/bopsection/A1` is self-contained cylinder/prism construction and
  requires section length 14.2832, section consistency and inherited shape
  validity. It exceeds our current plane/wall-only section family.
* `boolean/bopcommon_simple/A1` intersects coincident unit boxes and requires
  area 6 plus inherited validity. This is a direct external Boolean recipe.
* `mkface/edges_to_faces/A1` reconstructs six box faces from edges, requires six
  wires and six faces, valid shape and no self-interference.
* `mesh/end` checks actual triangulation, unmeshed faces, free links, crossings,
  asynchronous edges and free nodes, with mesh/geometric area information and
  declared tolerances. Triangle count or a picture alone is insufficient.
* `de/step_1/A1` declares a file and generated expected records; `de/end` performs
  import/export/reimport, checking tolerances, validity, topology, transfer
  diagnostics and document metadata. The case contains TODO known faults.
* Migrated `BRepAlgoAPI_SectionTest.OCCN2_CylinderSphereSectionIsDone` asserts
  completion and non-null output only. Classifier C++ tests independently
  specify box IN/OUT/ON and sphere/infinite-point behavior.

Therefore eight deliberately chosen mutants establish sensitivity to eight
named fault classes; they do not characterize mutation adequacy, the complete
input domain, or alpha readiness. Our independently derived exact oracles have
value, but share the team's scope choices and assumptions. External regressions
add independently chosen cases, historical failures, imported geometry and
different implementations. Keep analytic/model truth, external expected
answers and differential checks together; inspect weak external assertions
instead of assuming community provenance makes every test a strong oracle.

## Concrete qualification adapter

First run the **unchanged pinned upstream** runner and public data on the fleet.
Use `CSF_TestScriptsPath`, `CSF_TestDataPath` and a fresh result directory; pin
DRAW/plugins, toolchain, data hashes and platform. The native harness handles
group/grid begin and end, `cases.list`, Tcl procedures, time limits and result
parsing. Use its `testgrid` masks and HTML/XML reports, not a homemade regex
execution of script fragments. `DRAWEXE -b` supports batch work, but many
inherited rendering commands require virtual display/software GL or a reviewed
separation of display-only checks. Root owns fleet availability inspection.
This study does not establish that DRAW is installed.

The owned unit then needs a **BRepGPU operation adapter**, not an OCCT fallback:
translate source primitives, curves, transforms, imported geometry and requested
operations into owned inputs; run BRepGPU; export/query its actual output; apply
the inherited semantic expectations. Initially explicit per-command recipe
translation is preferable to pretending a complete Tcl/OCCT API exists. A
DRAW plugin can be a convenient external test host, but successful unknown
commands must never silently execute the operation in OCCT and count as an
owned pass. GTests tied to OCCT C++ classes similarly require translated recipes
or a separately identified upstream-baseline run, not a counterfeit direct run
against BRepGPU. No production OCCT dependency is selected by this proposal.

Preserve case provenance: original group/grid/case, inherited scripts, fixture
hashes, operation/parameters, expected checks, normalized checks, actual owned
output and explicit adaptation limitations. Classify unavailable/unsupported
cases now, implement their missing capabilities on the roadmap, and require
the broad declared public alpha set green before alpha. A small initial adapter
slice can make progress while the complete inventory remains visible.

Normalize **semantic differences explicitly**. OCCT shape IDs, edge splits,
seams, periodic parameter origin, triangulation order and BRepGPU dense slots
are not interchangeable. Compare geometric locus, endpoint incidence,
orientation, connected components, closed regions/holes and membership using
declared tolerances or exact truth as appropriate. Whole length/area/volume
and shape counts supplement these checks. Do not erase multiplicity, seams or
non-manifold uses through coordinate welding. Do not globally discard count
checks: audit any legitimate representation refinement and retain the original
check and reason alongside the normalized replacement. Closed material versus
regularized Boolean behavior and tolerance-based ON versus exact equality need
case-specific contracts. Unknown ambiguity is a disagreement to investigate,
not permission to widen tolerances until green.

Exporting BRepGPU-produced geometry for OCCT validation supplies useful external
validity/differential checks, but exporting inputs and letting OCCT reconstruct
the requested result would test OCCT. Likewise converting via a healing path
can mask owned failures; expose conversion/healing changes and validate the
actual owned graph before repair.

## Upstream statuses and our alpha gate

DRAW uses inherited parse rules, platform-qualified TODO and REQUIRED patterns,
and TEST COMPLETED detection. BAD means a matched known problem; IMPROVEMENT
means an expected TODO failure disappeared; SKIPPED can mean a missing fixture.
REQUIRED can legitimately require an error message for a negative test. These
are meaningful upstream regression statuses, not interchangeable passes.

Store upstream baseline status separately from owned semantic result. A known
upstream BAD must not bless an incorrect BRepGPU output; investigate intended
behavior and use the independent contract. A legitimate negative-input test
can pass by returning its required rejection. A missing, unsupported, crashed
or unadapted alpha-required case cannot pass. The alpha cut requires all
declared applicable public cases to execute their owned operations and meet
their reviewed semantic expectations. Report the full denominator, exclusions,
unavailable fixtures and remaining work. Passing this pinned corpus is a strong
release requirement, not universal correctness or certification by Open Cascade.
