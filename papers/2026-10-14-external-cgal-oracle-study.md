# CGAL as an external exact arrangement oracle

2026-10-14. Recommendation: adopt **a separately built, test-only CGAL reference
executable**, subject to the operator's library decision. Use it before alpha
for an independent semantic comparison of our supported planar grids/full
circles. Do not add CGAL to the product or claim that upstream tests alone test
BRepGPU. No library was installed, no reference executable was built, and no
product ran during this study.

The inspected official release is **v6.2.1**, tag object
`2e3640e7bfc3f77acec333ea64a0037760d2c957`, resolving to commit
`28811b671a12b5caa9e3688569dadbc6b3728fe6`. The
[source catalog](external-cgal-oracle-sources.tsv) records 20 actual source/data
files with commit URLs and content hashes, plus eight official manual pages.
Documentation `latest` is mutable; the retrieved pages identified 6.2.1.
No source archive was extracted or vendored. An implementation step must acquire
an intact pinned archive into quarantine and record restoration/hash information.

## Exact candidate and what it adds

Use `Arrangement_2<Arr_circle_segment_traits_2<Cartesian<Gmpq>>>` for the first
reference. Rational input lines and circle centers/radii remain exact; the
traits also represent quadratic algebraic intersection coordinates. Whole
circles and line segments are supported directly. The official example instead
uses EPECK, also an exact option; explicit Gmpq makes exact rational request
parsing straightforward for our bounded family. This is a proposed test build
choice, not a product arithmetic change. [Traits reference](https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arr__circle__segment__traits__2.html),
[actual example types](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Arrangement_on_surface_2/examples/Arrangement_on_surface_2/arr_circular.h).

`Exact_circular_kernel_2` is a second exact primitive oracle for circle/line
incidence and intersections. Its implementation uses a filtered circular kernel
over exact arithmetic, with Gmpq when GMP is enabled. It is useful for source
geometry and admission boundaries, but does not replace the arrangement DCEL
or complete face-domain comparison. [Pinned kernel source](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Circular_kernel_2/include/CGAL/Exact_circular_kernel_2.h).

`Arr_conic_traits_2` handles bounded degree-two curves, including ellipses,
parabolas and hyperbolas. The official guidance prefers circle-segment traits
when only circular arcs/segments are needed. CORE-backed conic tests are a
valuable later reference for richer curve families, not a reason to enlarge
our supported input contract now. [Conic reference](https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arr__conic__traits__2.html).

## Actual upstream suites, including their limits

The pinned `Circular_kernel_2/test/Circular_kernel_2/CMakeLists.txt` declares six
test executables, including `test_Exact_circular_kernel` and
`test_Exact_circular_kernel_basic`. The former calls circle predicates,
constructions, bounding boxes and has-on checks. Its construction helpers check
exact extrema and circle/circle and arc intersections, including tangent
multiplicity. Much of this suite uses C `assert`; build qualification with
assertions enabled, recording flags, rather than accepting a Release binary
whose `NDEBUG` removes assertions. Randomized helpers also need their actual
seed/configuration retained. [Exact test](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Circular_kernel_2/test/Circular_kernel_2/test_Exact_circular_kernel.cpp),
[actual construction assertions](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Circular_kernel_2/test/Circular_kernel_2/include/CGAL/_test_circles_constructions.h).

The arrangement package has real data-driven trait, construction, overlay,
point-location, insertion, removal and other tests. Inspecting the actual
`cgal_test.cmake` matters: construction/overlay registrations are not a turnkey
full-circle comparison for our product. The circle-segment traits registration
checks several constructor/comparison/vertex operations; intersection/split/
merge data files exist but those operations are not all registered by that
circle-segment function. The conic traits registration explicitly includes
intersection, split and merge. Record the concrete selected CTest list and
outcomes, not a generic claim that every package file ran.
[Actual registration script](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Arrangement_on_surface_2/test/Arrangement_on_surface_2/cgal_test.cmake).

The registered circle-segment point-location command uses one whole unit circle
and one boundary query. Its verifier compares enabled CGAL point-location
strategies against each other. That is useful reference health evidence, not
an independent exhaustive circle/grid corpus. The construction harness also
compares expected vertex/curve lists, face counts and boundary-component counts,
and calls arrangement validity checks. We should reuse the mature upstream
tests while retaining our stronger complete region-domain specification.
[Point-location command](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Arrangement_on_surface_2/test/Arrangement_on_surface_2/test_point_location.circle_segments.cmd),
[verifier](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Arrangement_on_surface_2/test/Arrangement_on_surface_2/Point_location_test.h),
[construction truth](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Arrangement_on_surface_2/test/Arrangement_on_surface_2/Construction_test.h).

The public Git tree is needed for these tests: CGAL's repository license notes
that some doc/test files are absent from distributed source tarballs. A release
installation supplying headers alone is insufficient for upstream test replay.
[Repository distribution note](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/LICENSE.md).

## First whole external harness

This is a proposed implementation unit, not code already delivered:

1. Build a standalone developer/reference executable against the pinned CGAL
   source. BRepGPU never imports, links or calls CGAL in a shipped operation.
   A test controller invokes both implementations from original submitted input
   records. An external rational TSV protocol can describe the unit rectangle,
   exact cuts, independently reconstructed original source circles and queries.
   No rounded mesh, product-generated topology or product private circle cache
   supplies the reference input. Check retained body/revision/face/chart identity
   against the original query before transforming source controls to target UV.
2. Insert the four rectangle sides, complete cut segments and whole circles into
   CGAL. Validate the resulting arrangement. Start with all five delivered
   recipes, all rectangular orientations for line-only cases, source operand
   reversal, bottom-face circles, dyadic frames, narrow/subnormal cuts and
   one-ULP separation. Add seeded rational families and replayable reduced
   failure inputs within current limits. Tangency/clipping/intersecting disks
   are reference-capability cases for checking our explicit refusal, not newly
   supported BRepGPU output.
3. Establish a region bijection using independently derived original-input
   witnesses and **all** CGAL faces, including the unbounded face. Match every
   input cell-minus-disks domain and each source disk uniquely; reject missing
   or extra CGAL/product domains. Do not let product witness choices define the
   expected subdivision. Compare supporting curves, directed interval coverage,
   adjacent semantic region pairs and oriented outer/hole components.
4. Normalize representation differences: CGAL splits circles into x-monotone
   arcs, while BRepGPU keeps one physical closed edge and one anchor. Group
   reference arcs by original exact circle support and incident semantic region
   pair, verify complete closed coverage, and compare their boundary domain.
   Coalesce collinear fragments only while original support/source interval and
   region adjacency permit it. Never require raw vertex/edge-ID or edge-count
   equality across kernels.
5. For closed point membership, expand a CGAL located face to that face, an edge
   to its two incident faces, and a vertex to all incident face handles; dedupe
   semantic regions. This handles two/four-way crossings and artificial circle
   extrema correctly. Query original domain witnesses, every crossing and cut
   endpoint, source circle quarter joins/interiors, exterior/target boundary,
   and exact near-boundary points. Compare all incident region sets with the
   actual product reader. CGAL's ordinary locate returns one feature, so merely
   comparing its face result would weaken our closed-region contract.
   [Point-location interface](https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arr__naive__point__location.html).
6. Use exact reference has-on/intersection predicates on emitted world/UV curve
   samples and source boundaries. CGAL geometry alone does not know our custom
   chart/source namespace or rational parameter convention; retain original-map
   polynomial identities and typed-source checks for those distinct claims.
   Preserve inputs, reference version/build flags, selected suite results,
   comparison outcomes and every mismatch. Run the eight named production
   mutants against the external comparator where applicable to show that its
   actual comparisons observe those faults; no second-order testing machinery.

## Build and test-only adoption proposal

The concrete benefit is an independently developed exact geometric arrangement
engine, its mature predicates and degeneracy corpus, and a second topology
construction algorithm. Owning a second complete arrangement kernel would cost
far more while sharing our interpretation errors. Our owned product and direct
oracles remain responsible for public contracts and provenance.

Costs are a separate C++ test adapter/controller, template compile time and
memory, pinned source/test-data acquisition, exact serialization, and semantic
normalization. CGAL has been header-only since version 5, but the adapter and
upstream test executables still need compiling. This must never enter timed
product calls or become an undeclared product fallback.
[Installation manual](https://doc.cgal.org/latest/Manual/installation.html).

Current official build documentation specifies C++17+, CMake3.22+, Boost
headers1.74+, and a multiprecision backend. Choose explicit GMP/Gmpq plus MPFR
development packages for this reference; the manual lists GMP5.0.1+/MPFR3.0.0+.
It currently lists GCC13.3+ among continuously tested Linux compilers; older
compilers may work but are not guaranteed. Fleet compiler availability must be
checked rather than assuming the existing CUDA compiler is the right host
compiler. No Qt viewer, Eigen solver, TBB, GPU or GUI is needed for our adapter.
The upstream arrangement test CMake requests `Core` and links `CGAL::CGAL_Core`;
its conic tests add CORE arithmetic requirements even though the simple adapter
only needs `CGAL::CGAL`. Current docs say Core needs Boost multiprecision.
[Dependency manual](https://doc.cgal.org/latest/Manual/thirdparty.html),
[actual test CMake](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Arrangement_on_surface_2/test/Arrangement_on_surface_2/CMakeLists.txt).

The inspected arrangement traits and exact circular-kernel headers declare
GPL-3.0-or-later OR commercial licensing. CGAL has package/file-specific licenses;
many unmarked repository test files are CC0, but explicit notices override that.
Keep upstream notices and license texts. Treat the CGAL adapter as a separately
licensed verification tool and publish its source/build instructions if it is
distributed under the GPL route. A subprocess is an engineering boundary, not
an automatic exemption from license obligations. Runtime adoption would be a
different, separately discussed dependency/license decision.
[Official license scheme](https://www.cgal.org/license.html),
[distribution license](https://github.com/CGAL/cgal/blob/28811b671a12b5caa9e3688569dadbc6b3728fe6/Installation/LICENSE).

## Alpha meaning and remaining gaps

The eight current hand-written mutants establish that eight named faults are
observed by the direct suite. They do not establish enough general mutation
coverage, complete kernel correctness or alpha readiness. Community suites and
external semantic comparisons add independent implementations and accumulated
degeneracies; they complement direct specification/original-source oracles.
Both the selected upstream reference-health suites and BRepGPU-versus-reference
supported-family corpus must be green **before alpha**, with exclusions explicit.
They need not be per-commit gates.

CGAL is not a universal analytic B-rep conformance suite. Polygon Mesh Processing
works on polygon/triangle meshes; exact mesh predicates do not recover analytic
circle/trim/chart identity or certify our complete B-rep topology. Its suites
belong to the later mesh beachhead. Our current circle adapter cannot certify
general NURBS SSI, imported STEP topology, sewing, solid Boolean assembly or GPU
numerics. Those need their own external suites and declared supported contracts.
[PMP scope](https://doc.cgal.org/latest/Polygon_mesh_processing/index.html).
