# Literature still to acquire

These sources need fuller inspection before their assumptions can enter the
implementation design. Abstracts, metadata and slides are labeled in the reports.

| Source | Gap | Primary link |
| --- | --- | --- |
| Valasek/Horváth 2026, register-efficient Bernstein evaluation | Publisher full text read; standalone PDF not acquired | [DOI](https://doi.org/10.1111/cgf.70403) |
| Krishnamurthy 2010 GPU NURBS dissertation | PDF acquisition blocked | [University repository](https://escholarship.org/uc/item/59n1g12w) |
| Hu et al. 1997, robust interval surface intersections | Publisher abstract read; full text needed | [DOI](https://doi.org/10.1016/S0010-4485(96)00099-1) |
| Huang/Hu/Martin 2005, degree elevation and knot insertion | Free PDF inspected via browser; direct acquisition timed out | [Author archive PDF](https://archive.ymsc.tsinghua.edu.cn/pacm_download/12/444-cagd_2005_degree.pdf) |
| Piegl/Tiller, The NURBS Book | Book access needed | [Publisher](https://doi.org/10.1007/978-3-642-59223-2) |
| Fogel/Halperin/Wein 2012, CGAL Arrangements and Their Applications | Full book not acquired/read; publisher metadata only. Official arrangement manual and actual retained source suffice for the bounded first unit | [Publisher](https://doi.org/10.1007/978-3-642-17283-0) |
| Ma/Hewitt 2003, control-polygon point inversion and projection | Institution record read; full article needed | [DOI](https://doi.org/10.1016/S0167-8396(03)00021-9) |
| Li et al. 2019, geometric strategy for orthogonal projection | Publisher abstract read; full text and theorem assumptions needed | [Publisher](https://jcst.ict.ac.cn/article/doi/10.1007/s11390-019-1967-z) |

The [topology acquisition list](gpu-topology-shopping-list.md) adds seven
surface-intersection, Boolean, overlap and watertightness articles. The
[numerical manifest](gpu-numerics-acquisitions.tsv) records acquired PDFs and
attempts separately. No access requests have been sent.


## Conservative bounds follow-up, 2026-10-14

The [numerical acquisition list](bounds-numerical-shopping-list.md) records
remaining homogeneous-box and interval-arithmetic full-text/PDF gaps, with
their design relevance. They do not block the positive-weight hierarchy study.

- Selimovic2006, *Improved algorithms for the projection of points on NURBS curves and surfaces*, CAGD23(5):439–445, [DOI](https://doi.org/10.1016/j.cagd.2006.01.007). Author publication page and Chen2008 references were inspected; full article not acquired. Needed before any derivative/normal-based exclusion is considered. This is a follow-up gap, not a blocker for positive-weight control-hull/AABB bounds.
- Ma/Hewitt2003 above remains a fulltext gap. The journal curve/surface article is distinct from the curve-only TPCG2003 paper (DOI10.1109/TPCG.2003.1206938). The acquired [Chen et al.2007 counterexample](https://inria.hal.science/inria-00518368/document) is now available in the bounds PDF catalog; no numeric controls are supplied by its diagram.


## Real mechanical watch sources, 2026-10-15

- **OM10 MAN STEP acquired from operator:** `om10-00001_20220701_va_01_3.stp`
  received2026-10-15; original bytes and provenance/hash retained privately.
  [Actual source inventory](2026-10-15-om10-step-source-study.md) records named
  parts, units, occurrences and a concrete thin fixing-clamp candidate. The
  earlier free-membership download barrier no longer blocks this STEP revision.
  Still missing: a complete part-level PDF manufacturing drawing set and explicit
  fits/tolerances/feature history. STEP's geometric uncertainty is not a machining
  tolerance. Preserve and inspect exact source notices before source redistribution.
- Public OM10 technical/release documents and official ETA6497-1, SellitaSW210-1,
  MIYOTA9015 and TMI NH35 PDFs are acquired. Their useful scope and remaining
  dimensional/reuse gaps are in the [source report](2026-10-15-watch-drawing-source-frumentarius.md).
  No broader proprietary drawing package was acquired. NAWCC's public
  [company-records catalog](https://www.nawcc.org/research/company-records/) lists
  membership-gated Hamilton watch service records; its separate engineering
  drawing index belongs to clock records and is not evidence of watch-part CAD.

## Direct constrained triangulation full texts, 2026-10-16

- **Chin–Wang1998**, *Finding the Constrained Delaunay Triangulation and
  Constrained Voronoi Diagram of a Simple Polygon in Linear Time*, SIAM Journal
  on Computing28(2):471–486. Publisher abstract/bibliography inspected; full
  algorithm not acquired. Needed to evaluate the ordered simple-polygon
  specialization, its degeneracies and relation to the general holes contract.
- **Seidel1988**, *Constrained Delaunay Triangulations and Voronoi Diagrams with
  Obstacles*, in *1978–1988 Ten Years IIG*,178–191. Bibliographic/theorem leads
  inspected; full original not acquired. Needed for a serious optimal sweep
  comparison. Do not substitute Chew's different constrained divide-and-conquer
  method or infer a cavity algorithm from the title.

These gaps do not block reading the acquired Chew, Shewchuk–Brown, BRIO,
predicate and2026 GPU CDT originals. They remain explicit acquisition work,
not algorithms claimed read or implemented.

## Whole-mesher geometry-based methods, 2026-10-16

- **Aubry, Dey, Mestreau, Karamete, Gayman2015**, *A robust conforming NURBS
  tessellation for industrial applications based on a mesh generation approach*,
  CAD63:26–38, DOI10.1016/j.cad.2014.12.009. Publisher abstract/introduction
  inspected; full text remains unavailable after publisher, title/author and
  open-access-location searches. Needed for its complete three-dimensional
  anisotropic/tiered algorithm and singularity treatment; no detailed theorem
  or implementation is attributed to the preview.
- **Piegl–Tiller1998**, *Geometry-based triangulation of trimmed NURBS surfaces*,
  CAD30(1):11–18, DOI10.1016/S0010-4485(97)00047-X. Bibliography/preview only.
  Needed to compare its geometry-based subdivision with parameter-bound
  refinement. Piegl–Richard1995 is acquired in full and is a different paper.

These remain comparison gaps; the acquired full primary subdivision,
anisotropic refinement, feature-protection and trimming papers support the
current whole-mesher design study. See `whole-mesher/RESTORATION.md` and the
algorithm/geometry-contract bibliographies for exact acquisition/read scopes.

## Adaptive subdivision follow-up, 2026-10-16

- **Kumar–Manocha1995**, *Efficient Rendering of Trimmed NURBS Surfaces*,
  CAD27(7):509–521, DOI10.1016/0010-4485(94)00003-V. The primary author abstract
  at `https://www.cse.iitd.ernet.in/~subodh/research/abs.trim.html` was read.
  Its linked `https://www.cs.jhu.edu/~subodh/papers/trim.cad.ps.gz` returns404;
  the analogous current author-host path also returns404. Full source remains
  requested for its cell-tracing/coherence/intersection details. No theorem or
  detailed algorithm is claimed from the abstract. Ma/Hewitt2002's separate
  full publisher paper was acquired/read and is not a substitute for this one.

## Lutterkort–Peters shared-curve follow-up, 2026-10-16

- David Lutterkort and Jörg Peters, *Tight Linear Envelopes for Splines*,
  Purdue CSD-TR99-004 (1999). Institutional metadata at
  https://docs.lib.purdue.edu/cstech/1435/ was read. The primary download
  https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=2434&context=cstech
  returned HTTP403 both through browser and a bounded fleet curl request with
  browser User-Agent. No PDF was acquired and no full-text theorem is claimed.
  Needed for a precise control-polygon envelope comparison, not a blocker for
  the selected original-coefficient actual-chord and checked-image algorithms.
