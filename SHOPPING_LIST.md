# Shopping list

Items the research lanes could not acquire, for operator acquisition or
decision. Compiled 2026-10-17 from the fast-mesher study papers; each entry
names the paper that wants it. Mark entries acquired or declined in place.

## Operator decisions needed

| Item | Why we want it | Decision needed |
|---|---|---|
| BRL-CAD source (`libbrep` CDT tessellator, `ON_Brep_CDT_*`) | An owned public B-rep CDT tessellator with a test suite (test-plans-cad, Appendix C) | github.com/BRL-CAD/brlcad returned 404; the only reachable copy is an unverified third-party mirror. Acquire from an official source, or skip? |
| Epic CADKernel (Unreal Datasmith) | An owned B-rep tessellator (test-plans-cad) | Source requires an Epic account. Acquire under the operator's account, or skip? |
| Tracker reproducer attachments with unstated licences | Field-failure fixtures for the corpus (test-plans-cad, "Reproducers worth acquiring") | OCCT GitHub #507, #688, #572, #1444, #682; FreeCAD #12744, #25311, #20456, #25514, #17175, #33251, #29811; truck #41, #123; Gmsh #623, #415, #1107, #3167, #541, #2237, #3147, #3278, #3260. Licence status is unstated. Use them as private corpus, or skip? |
| ABC dataset download (3 chunks: STEP, meta, stat, Parasolid; 8.95 GB) | Priority-1 arena corpus (M3; corpus/MANIFEST.md) | Accept the site's download term ("The authors give no warranties regarding the dataset") and run `corpus/abc/fetch-selected.sh` on the fleet host? Per-model rights follow Onshape ToU 1.g.ii; models before 2018-08-07 with a LICENSE tab keep reserved rights |
| Fusion 360 Gallery (s2.0.1 extended STEP first) | Second large real B-rep source with native `.smt` and instanced assemblies (M3) | Licence binds the user and a for-profit employer to non-commercial research use with an indemnity of Autodesk. Accept, or skip? |
| CAx-IF test-round models | Current multi-vendor AP242 exports with known issues (M3) | Member areas only. Seek membership or access via PDES, Inc. or ProSTEP iViP, or skip? |
| Publishing results on CAx-IF public library files | No licence covers republishing the files (M3) | Default: publish timings and verdicts only, never the files. Confirm, or ask PDES/ProSTEP? |
| openmovement OM10 other revisions (MAN VB, AUT beta 01.3, earlier betas) | More real horological geometry (M3) | Needs an openmovement member account (free tier). Create one? |
| OCCT Mantis attachments and fixtures absent from the public dataset | Hang, OOM and thin-face fixtures (test-plans-cad) | Mantis 0033082, 0026965, 0033476, 0033681, 0033314, 0033906, 0025589, 0023631, 0026074; files `bug25281_tess_infloop_extract.step`, `bug28118_18547.stp`, `OCC396_f2903.brep`, `bug31301.stp`, `bug25287_bad_tess_face_plus.step`, `bug23105_f372.brep`, `bug25551_m-a-i-B6.brep`, `bug25061_a.brep`. Not public; request from OCCT, or re-author? |

## Standards

- SASIG PDQ V3.0 Attachment C ("recommended values") — healing thresholds.
- ISO 10303-59:2022 (STEP product data quality) — healing and data quality.
- ISO/PAS 26183 — data quality.

## Kernel and vendor documentation

- Parasolid PK reference pages: `PK_EDGE_repair`, `PK_EDGE_optimise`, sew
  options (tolerant-geometry-healing).
- ACIS R10 healing function reference; "Healing Tolerances" and "Tightening
  Gaps" articles (doc.spatial.com now requires login).
- CADfix user guide; Elysium CADdoctor and Q-Checker defaults; Pointwise
  Assemble Models documentation (Cloudflare 403); Simmetrix tolerance white
  papers.

## Papers (paywalled, bot-walled or unread)

Highest value first:

- Xiao, Alliez, Busé, Rineau 2021, "Delaunay meshing and repairing of NURBS
  models" (HAL bot wall) — meshing through trim defects.
- Steinbrenner et al. 2001, "Procedural CAD model edge tolerance negotiation"
  (Engineering with Computers).
- Kahlesz, Balázs, Klein 2002 — sewing for tessellation.
- Li et al. 2026 (JCDE) — self-intersection avoidance; publisher returned 403.
- TetWild (SIGGRAPH 2018) and fTetWild (TOG 2020) papers — Thingi10K run
  criteria (yixin-hu.github.io).
- Sheng–Hirsch 1992; Cheng 1992; Nairn et al. 1999; Reif 2000;
  Lutterkort–Peters 2001; Zhang–Ma 2006; Concheiro 2014; Eisenacher 2009;
  Rockwood et al. 1989; Cripps–Parwana 2011; Tristano 1998; Borouchaki 2000;
  D'Azevedo–Simpson; Rippa 1992; ETER 2023 full text; Jeannerod et al. 2013
  full text; Krishnamurthy 2007/2009 CAD papers (fast-tessellation-literature).
- Galtier–George 1996; Löhner–Cebral 1999; Löhner 2013/2014; Laug–Borouchaki
  2011; Laug–Guibault–Borouchaki 2017; Said et al. 1999; Tremel et al. 2007;
  Tu–O'Hallaron–Ghattas SC05; Zhou et al. 2010/2011 GPU octrees;
  Aurenhammer–Hoffmann–Aronov 1998; Loseille–Alauzet 2011 part I;
  Dyken–Reimers–Seland 2009; Guéziec et al. 2001; Barequet–Kumar 1997
  (parallel-growth-clusters).
- Gu, Chase et al. 2001 (JCISE); Chong, Kumar, Lee 2007; Busaryev, Dey, Levine
  2009; Fang, Brüderlin, Zhu 1993; Attene, Campen, Kobbelt 2013 (CSUR)
  (tolerant-geometry-healing).
- Hamann–Tsai 1996 is held locally as a scan with no text layer; needs OCR or
  a better copy.

## Corpora and fixtures

- Thingi10K meshes and metadata CSVs (Hugging Face `Thingi10K/Thingi10K`
  v1.5.0) — triangle corpus, flag-table methodology.
- mmg3d CI corpus 0.0.46 (`static.bordeaux.inria.fr/mmg/mmg3d.tgz`, 333 MB).
- geogram test data (`BrunoLevy/geogram.data`).
- SALOME `DATA_DIR` sample shapes (`flight_solid.brep`, `slots.brep`).
- gCDT paper datasets.

## Reference code

- fast-envelope (`wangbolun300/fast-envelope` @ 520ee04b) — exact versus
  sampled envelope.
- Blender at the pinned commit in full (test registration, `BLI_math_mpq.hh`,
  buildbot CI); only ten files are held.
- CGAL testsuite Docker images and nightly results.
- artem-ogre/CDT external fuzz harness (Som1Lse); not public in tree.
- Nanevski et al. 2001 predicate test set, original location.

## Retry later (no decision needed)

- NIST/MTC box-assembly TDP `mtc.zip` (DOI 10.18434/T4/1503154) and NIST
  `mds2-2146` `qif-ontology-dataset-V2.zip`: data.nist.gov returned HTTP 524.
- CAx-IF `rec_prac_gvp_v32.pdf` (Geometric Validation Properties practice):
  404 on mbx-if.org; try a Wayback capture.

## From M2 (neighbour reconciliation)

- Weihe & Willhalm 1998 (WAE'98, MPI-I-98-1-019) and the Algorithmica 2000
  version; only the Willhalm Diplomarbeit was read.
- Urick et al. 2019, "Watertight Boolean operations" (CAD 115).
- Patel, Marcum, Remotigue 2005 (IMR): zip versus stitch by gap width.
- Langbein 2003 PhD thesis (Cardiff); Langbein et al. GMP 2002.
- Werghi et al. 1999, CAD 31(6), published version.
- Meyer & Sanders 2003, J. Algorithms 49(1).
- Müller et al. 2007, JVCIR journal version of position-based dynamics.
- Gao et al. 2018, GPU optimization of material point methods.
- Kaufmann & Klass 1988, reflection lines (CAD 20(6)); Hagen et al. 1992,
  surface interrogation (IEEE CG&A); Gouraud 1971 thesis; Phong 1975;
  Lavoué & Corsini 2010 (IEEE TMM).
- Parasolid internals for `PK_FACE_grow_auto_c` and automatic loop healing.
- Already listed above: Barequet & Kumar 1997; Xiao et al. 2021 full PDF;
  ACIS "Tightening Gaps".
