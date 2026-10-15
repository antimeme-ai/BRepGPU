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
