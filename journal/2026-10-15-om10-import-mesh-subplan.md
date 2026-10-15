# OM10 source and trimmed mesh implementation subplan

2026-10-15. Geometry lane owns one complete invariant: translate the actual
supplied STEP to source-linked immutable values, construct every trimmed solid's
own mesh, preserve physical/source/corner identity, and export an honest native
consumer scene. This follows the research in
[the continuing study](../papers/2026-10-15-om10-import-mesh-study.md) and the
reviewed whole-source design. It does not begin production before root accepts
the overall implementation plan.

## Design review

Accept the complete design. Clarified source B-spline versus display Bézier tags,
root assembly accessors, actual per-corner limiting normals at cone apices, and
the independent source/mesh binary interfaces. The code must reject nonfinite
intermediates, not merely finite inputs. Actual complete geometry qualification
precedes final source-derived trophy renders.

## Files and boundaries

- `native/step_source/CMakeLists.txt` and `native/step_source/main.cpp`: isolated
  OCCT STEP/XDE extraction and BGSRC001 writer; no product tessellation.
- `python/brepgpu/imported_source.py`: frozen native values, bounded binary
  reader, structural validation and original-ID lookups.
- `python/brepgpu/imported_meshing.py`: owned curve/surface evaluation needed by
  CPU meshing, shared source edges, coedge UV occurrence interpretation, hole
  triangulation, conforming refinement, source mesh and BGMESH01 writer.
- `tests/test_imported_source.py`, `tests/test_imported_meshing.py`: direct
  analytic/metamorphic/error-raising tests. Independent lane owns separate
  geometric qualifiers; these tests use the qualifier where useful rather than
  copying it into a second coverage layer.
- Continuing study and this journal: actual findings, resolved review issues,
  source translation differences and measured costs. Public paths contain no
  private source headers or host identifiers.

No adoption of a triangulation or rendering library. No changes to existing
canonical family guards, patch ABIs or feature result contracts. CPU geometry
meshing can initially be owned Python; strong native CPU display is separately
owned, and actual Python/import/mesh wall remains in the cold measurements.

## Sequenced work

1. **Relevant red** on the fleet: missing imported-source and mesh APIs fail
   initial independent expectations. Record missing-API failures accurately.
   Add binary safety cases, a thin rounded plate with bore, nonconvex many-hole
   planar domain, periodic seam, multispan rational surface and degree-five edge
   as their exact agreed constructors become available. Later added cases are
   not retroactively described as the initial red run.
2. **Reader and extraction**: implement explicit length-counted BGSRC001 and
   immutable values. Traverse original STEP entities with transfer bindings;
   preserve original complete product/usages and source geometry. Read XDE
   colors/material where available. Original file SHA is supplied/checked by
   the owned caller. Headless source conversion runs on fleet only.
3. **Early actual source inventory**: emit every source/translated count,
   recovered pcurve type/degree, SameParameter state, seam/degenerate edge,
   correspondence and tolerance change. Inspect all diagnostics. Resolve source
   conversion gaps explicitly; no partial success renders.
4. **Owned curves and domains**: evaluate degree-preserving original curves;
   share one physical edge sample chain, preserve original endpoint identities,
   map each coedge to its proper UV lift, bound chord error and measure recovered
   UV residual. Analytic/indirect/periodic fixtures establish formula conventions.
5. **Triangulation and refinement**: robust orientation filters with exact
   fallback, rightmost-first visible hole bridges, occurrence-aware ears,
   retained collinear constraints and conforming interior refinement. Source
   chart singularities retain UV occurrence identity while removing physical
   zero-area triangles. Fail exhausted budget or ambiguous mapping explicitly.
6. **Independent geometry qualification**: full source inventories, every face's
   positive directed boundary chain, physical shared-edge incidence/vertex links,
   source values/normals/residual and actual complete solid comparisons. Native
   CPU/P100 evaluate-only outputs supply consumer compatibility independently of
   image appearance. Fix actual findings in this lane, rerun affected checks.
7. **Native scene export/integration**: preserve physical keys while duplicating
   display corners for normals and seams; export analytic or span carriers in
   definition-local coordinates. Root supplies full source placements, explosion
   offsets/materials and labels. Reader/writer range failures invalidate output.
8. **Adversarial source review**, fix valid findings, record rejected findings
   with actual source reasoning. Only then return complete geometry to root for
   assembled/exploded renders and CPU/P100 measured callers.

## Direct test obligations

- Binary unknown/truncated/oversized records, invalid references/ownership,
  nonfinite controls or frames, duplicate source identities, invalid knots and
  mismatched curve/surface degrees raise informative errors.
- Exact analytic plate extents/area/hole, nonconvex many-hole material coverage,
  unchanged topology under profile translation/scaling and reversed face order.
- Both periodic seam occurrences retain distinct UV but identical physical edge
  sample keys; opposite incident uses consume reverse shared chains.
- Degree-five curve values and multispan surface values agree with independent
  original polynomial/rational specifications, including derivatives/span widths.
- Coarse requested tolerance does not delete small holes or source edges.
- Thin parts retain source thickness; no previous primitive minimum-height limit
  is imported into this family.
- Singular chart corner occurrences preserve the physical quotient and positive
  rendered triangles; ambiguous/broken domains fail rather than fill or drop.
- Actual OM10 no-skip check: all 107 solids, 9,162 faces and 166 expanded source
  paths reach geometry/assembly output before root's final source-derived render.

This is enough direct work for the requested claim. No mutation campaign,
second certificate layer, unrelated historic GPU rerun or alpha certification
is introduced. Actual failures can justify additional focused cases.

Actual-source correction study: original sphere face165503 has a pole vertex
but no declared degenerate source edge. Its three-edge original loop needs an
explicit generated UV chart connector between the two source seam endpoint
occurrences. Review the continuing study's ChartClosure amendment with root and
the independent owner before implementing it. Retain original identities; no
generic UV gap or nearby vertex becomes a pole by tolerance. Actual mesh
refinement bookkeeping also must count dyadic construction depth rather than
propagated neighbor updates. Unproved non-SameParameter continuation is rejected;
the actual supplied source's original edges are all SameParameter=true.

Reviewed actual-source amendment: preserve exact rational UV construction only
when binary64 cannot produce a positive conforming fan; round explicitly at
native evaluation and account for its first-partial displacement. Recover only
foreign UV overhang at original global spline endpoints within 128 scale ULPs,
retaining raw UV, count, displacement and existing source residual limit. Root
and independent oracle reviewed the actual adjacent-double parents first. Add
direct faults/greens, focused actual face, then complete actual source run.

Root requested a diagnostic all-face scan if repeated first failures hide
remaining families. Extend the existing original-face diagnostic utility with
explicit --scan and bounded worker count; each face reports pass/failure, no
product mesh is assembled or qualified and no geometry is rendered. This is a
fault inventory only; the final product remains complete and fail-closed.

Checkpoint status: root and independent oracle accepted the source-derived pole
connector, sharp /8 Hessian bound, C1-cross-knot Hessian maximum, exact rational
UV constructions and bounded global-end foreign UV recovery. All13 focused
meshing tests pass on the fleet in1.041s. Previously failing original face165122
now meshes into22 positive UV cells in0.0108s; four foreign endpoint recoveries
and maximum native UV rounding estimate1.39e-16mm are explicit. Source checkpoint
7e7de7d already preserves all107 definitions/9162 faces. Full actual mesh and
independent qualification are still running; this checkpoint does not claim
that the whole OM10 target is green and contains no watch render. A separate
four-worker diagnostic scan reports every face failure without assembling a
product mesh. Early actual faces produce80k–330k cells; preserve complete
coverage and learn the actual final scene size before performance work.

Root's diagnostic utility review found that scan workers ignored --tolerance
and the utility lacked the explicit Linux execution guard. Workers now receive
the selected immutable MeshQuality through fork inheritance and main rejects
non-Linux CAD execution, matching the fleet caller. No geometry algorithm changed.


Actual cylinder face165361 demonstrated that maximum arithmetic construction
ancestry still does not measure geometric resolution under propagated conforming
splits: levels18/20/20 retained a freely splittable0.1736rad width and0.0270mm
bound. Root and oracle reviewed the budget correction. Carrier max_refinement
now controls a metric resolution floor G*2**(-max_refinement), where G is the
initial sampled face-domain diagonal in declared global first-partial-norm UV
metric. Selected free-edge metric length (or centroid triangle diameter) must
exceed that finite floor. This is a parameter/Lipschitz metric, not a claimed
physical edge length. Source edge adaptive recursion retains its original
dyadic-depth limit. Carrier max_refinement also bounds created interior nodes
per face by2**max_refinement (initial boundary nodes excluded), terminating
nonconvergent fixed-boundary floors even when their free edges stay long.
Default20 therefore admits20 metric halvings and1,048,576 interior constructions;
no approximation tolerance, source face, or quality allocation is relaxed.
Budget failure rejects the complete mesh. Direct metric-floor and work-cap
faults plus original165361 are the immediate tests before the complete rerun.

Cylinder-boundary amendment accepted by root and independent oracle: original
source C(t)/vertex-driven analytic inverse, explicit branch/provenance and
JointRecovery records, original-t control/analytic derivative bounds for owned
UV chords, positive radial interval proof and endpoint-incidence perturbation.
First implement direct inverse/axis/branch/degree-five derivative bound fixtures;
then original165500/165590, then retained whole-source/native qualification.
Raw foreign UV may exceed0.001mm; corrected original source residual may not.

Cylinder recovery direct16 methods pass; original165500 now7834 UV cells with
two original-vertex joint recoveries and corrected residual3.44e-14mm; original
165590 now330 cells with raw foreign residual0.00661018mm and corrected source
residual0.0001409593mm. Root found the analytic frame projection assumed
orthogonality beyond reader admission; use the actual cross-product dual basis
instead, retaining original axes and indirect frames. The diagnostic all-face
scan now accepts a rotation index: it still visits every face exactly once but
can prioritize the unexplored tail before repeating expensive early source
faces under new owned recovery. This changes no deliverable source coverage.

A later cylinder diagnostic exposed a retained foreign-knot constraint: source
spatial curve2436 has ordinary C1 knots, but the foreign pcurve has slightly
offset knot values. After owned cylinder inversion these foreign guide knots
are not product trim geometry. Exclude them from that cylinder occurrence's
mandatory edge cuts, retaining every original spatial knot and every knot still
needed by another incident raw-pcurve carrier. No original source topology or
curve representation is omitted.

Cylinder checkpoint:17 focused methods pass in0.990s, including actual dual
frame inversion and adjacent-double source interval ownership. Original166811
now meshes to14218 positive UV cells in2.914s after excluding foreign-only guide
knots; corrected source residual3.5747e-5mm. No physical collapse exemption was
introduced. Replaced old four-worker diagnostic scan with eight workers on the
current code, visiting source indices3000..end then0..2999, every face once.
Complete-source and native qualification remain pending, with known plane/cone
recovery work under study. No target rendering has occurred.

Next written extension: source-owned exact rational plane inverse and cone
axial/slant inverse, original-LINE generator proof at verified actual apexes,
source-owned cone V metric envelope. Root reviewed the family/metric design;
independent oracle has actual plane collinearity/cone axial-height witnesses.
Focused tests attack exact oblique-plane collinearity, source analytic/control
axial extrema, cone slant/sign/axis branch, whole LINE endpoint residual proof
and endpoint perturbation; real166230/165591/166142/166156/apex faces follow.
No collapse exemption, source omission, or quality widening. Native limiting
apex normals remain an actual evaluate-stage obligation before graphics.

Native owner independently evaluated48 actual apex queries CPU24/P1000/P1001:
all jets identical; canonical analytic apex V selects correct limiting normals,
while tiny imported/local-axial overshoot flips the nappe. Root directed verified
original source-apex endpoint V=-R/sin(angle); originalXYZ/rawmetadata remain,
endpoint perturbation and cone metric envelope include this explicit correction.

Plane/cone checkpoint:21 focused methods pass in1.067s; independent closed-plane
UV/physical/native integration5 methods pass in0.549s. Actual source cases:
166230 plane6cells/0.013s;165591 plane375cells/1.63s;166142 cone240cells/0.098s;
166156 cone99384cells/20.27s; canonical apex166149171239cells/34.79s and
17114867669cells/13.47s, each with one source-linked ChartClosure. Old foreign-UV
slivers no longer cause physical-zero triangles in these cases. The two apex
endpoints now use exact canonical host -R/sin V; originalXYZ remains unchanged.
Restarted complete eight-worker diagnostic scan with current source-owned
recovery, rotating source index6000 then wrapping all prior faces. Complete
whole-source/native qualification remains pending; no trophy image yet.

Read-only root review found a narrow admission bug while the full run is pinned:
collapsed cells currently require any corner to be a supported source pole,
rather than requiring EVERY REPEATED physical key to be that supported pole.
Independent whole-mesh qualification already enforces the stronger condition.
Queue the direct guard tightening after the pinned run and before rendering;
existing authored hemisphere at deliberately coarse quality supplies a repeated
nonpole-pair-plus-pole fault. No new collapse allowance or source omission.

New complete-scan geometry subplan (implementation awaits parent/oracle review):
1. Keep all-face diagnostic scan running, record every failure. Parent stopped
   the pinned full run before product edits; no target images exist.
2. Add direct source LINE angular-bound fixture proving tiny projected velocity
   despite full axial speed, and a coarse hemisphere repeated-NONpole negative.
   Run genuine fleet reds, then the narrow projection/quotient fixes.
3. Add an owned fixed-vertex constrained metric-Lawson stage after ears only
   when initial surface bounds fail. Exact incircle, strict convexity/positivity,
   immutable source boundaries, deterministic cocircular no-op and8*N*N flip
   rejection guard. No new dependency, points, tolerance or source omission.
4. Fleet reds: convex quad wrong diagonal expected corrected; constrained wrong
   diagonal unchanged; nonconvex pair unchanged; cocircular no-op; thin strip
   expected local connectivity and exact UV area/boundary-chain preservation.
5. Implement, focused greens, then actual five thin rings, spiral171044 and
   LINE failures171110/171115 with default .005/.001/max20. Independent existing
   complete UV/physical/native fixtures attack real output after code review.
6. Parent restarts fresh full retained qualification only after stable reviewed
   files and broad-scan remaining-family inventory. Report cap/timing/counts;
   no minimum-angle, universal termination or alpha claim.

Root and independent oracle accepted the written connectivity/LINE/quotient
subplan. Four new fleet reds observed: axial LINE trim bound2mm instead of0;
unsupported repeated nonpole pair accepted; two missing Lawson entry points.
Implemented exact projected LINE direction, stronger repeated-key guard and
owned constrained metric flips. Both reviewers found the same strict-convexity
precondition gap (new triangles positive alone does not establish the old
opposite-side relation for malformed overlapping input); added exact old-side
check and a direct overlapping-positive-pair no-op case. Added original face ID
to connectivity exceptions. Final25 direct methods pass in0.865s. Independent
existing5 complete-domain/physical/native integration methods pass in0.528s.
Actual original171110/171115 now each130 vertices/128 cells in0.759/0.716s;
corrected original source residual below5e-16mm. No zero-area exemption.
The current read-only old-module scan continues; later171042 is the same cubic
spiral cap family as171044. All five ring caps and both spiral caps remain
explicit default-quality focused obligations, then complete-source qualification.

First exact-Fraction ring170999 now passes2048 cells in156.1s; Lawson128.6s.
Parent proposed exact common-denominator integer arithmetic for this fixed
point set. Written derivation preserves the exact determinant sign, so selected
geometry is identical. Stop remaining Fraction-only focused queue after this
baseline; review arithmetic amendment, implement, reuse25 direct methods with
anisotropic determinant assertion, then all seven cap sources at unchanged
quality/default20. Complete read-only old scan continues independently.

Next source-boundary subplan (no production yet): exact linear natural spline
columns, verified original LINE affine coefficient identity and incident original
vertex inverse close four actual foreign-overhang cases. Written study specifies
admission, raw retention, new recovery-kind strings in existing occurrence/joint
records, original-domain output and chord endpoint perturbation. Independent
source inspection confirms all eight actual LINE occurrences satisfy the exact
coefficient conditions. Keep128ULP generic rule unchanged.
Meaningful fleet reds: authored clamped tensor column+original LINE source maps
recover exact UV; off-column LINE, unequal weights and wrong branch reject;
adjacent original curve endpoint shares column inverse only through original
vertex/incident LINE. Existing generic out-of-grade negative remains. Then all
four actual faces, independent schema/source/domain checks and code review.
Current connectivity checkpoint remains separate and production files stable
until root stages it and both reviewers accept this extension design/plan.

Both design/plan reviews accepted natural-boundary recovery with explicit V
clamping and fixed coedge branch. Two genuine fleet reds observed (missing
recovery; wrong-branch rejection absent). Implemented exact saved-column inverse,
whole original LINE coefficient identity, original-loop directed adjacency,
exact Fraction V and mandatory original joint records. Non-LINE neighbor scope
remains narrow. Complete raw branch guide U hull fixes one column per coedge;
no pointwise natural-column switch. Raw outside admitted recovery is never
source-evaluated; `foreign_uv_unevaluated_count` names the excluded metric samples.
Root and independent code reviews accepted; added source coedge/carrier and
face/loop diagnostic IDs per root review.27 focused methods pass0.823s.
Actual cases172494/172983/173009/173010 now21/17/19/17 cells in0.054/0.023/0.026/
0.023s. Corrected source residual maximum5.3441e-14mm; four original joint records
each. Independent original-column/endpoint/joint and exact domain checks pass
all four; five existing closed-solid/native integration methods pass0.533s.
No generic128ULP change, source entity omission, collapse exemption or image.

Before pinning the next full run, owner found canonical boundary displacement
missing from the actual triangle interpolation acceptance. Parent held the run;
root and oracle accepted the direct barycentric perturbation derivation. Written
amendment completes existing intended error accounting, retaining .005/.001.
Direct fleet red: independent planar prism source with one carrier displaced
normally0.0005mm, source residual limit .001 but requested mesh .0004; ideal
surface bound0 cannot excuse canonical displacement above .0003 allocation.
Also check admitted0.0001 displacement appears in actual combined metrics. Then
cache face-local node displacements, reject impossible fixed corners early,
accept ideal+maxcorner and finalsum maxcombined+2chord+round. Existing27 methods,
actual165590 and source-boundary cases verify the affected path before review.

Joint-cell accounting final28 direct methods pass1.000s; independent existing
five closed-solid/native integration methods pass0.564s with actual physical
probe gate tightened to requested tolerance plus existing numerical comparison
grade. Both code reviews accept. Explicit nonfinite displacement rejection and
same joint-bound initial Lawson trigger close review findings. Actual165590 now
53 cells/.077s: ideal maximum0.0001452878mm, canonical displacement maximum
0.0001409593mm, maximum actual per-cell sum0.0002525484mm. Four source-boundary
cases remain green. Default cap cases all five rings and both spirals pass;
additional R13.6 cylinder173378 passes4096 cells in184.31s. Remaining old cap
IDs172236/173126 are same thin full-period cone/cylinder source family; complete
current mesh qualification still covers each. Old full scan has only the three
studied failure families; no source omissions or target image.
