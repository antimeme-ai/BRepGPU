# Corpus manifest

Lane M3 of the fast-mesher campaign
([brief](../docs/fast-mesher-campaign/briefs/M3-corpus.md)). Acquired
2026-10-17. Data lives on the fleet host under `corpus/` in the BRepGPU root,
which is outside any Git checkout. This file and [manifest.tsv](manifest.tsv)
are the tracked record. Every path below is relative to the BRepGPU root.

No geometry, mesher, OCCT or NVIDIA code has run on any of this data. The only
checks were SHA-256, `file`, ZIP member listings and STEP header schema reads.

## Layout on the fleet host

| Path | Contents |
|---|---|
| `corpus/archives/<source>/` | Intact original downloads, byte for byte |
| `corpus/sources/<source>/` | Extracted working copies, with detritus and nested Git metadata stripped |
| `corpus/terms/` | Snapshots of the licence and terms pages that were read |
| `corpus/abc/lists/` | ABC's own chunk index, MD5 and size files |
| `corpus/abc/fetch-selected.sh` | Staged ABC fetch. **Not run** (see ABC below) |
| `corpus/_tools/` | `fetch.sh` (download once, log size and hash), a copy of `scripts/ingest_zip.py`, and `ingest_flat.py` for ZIPs without a root directory |
| `corpus/logs/` | Download logs |

Disk use: archives 1.44 GB, working copies 2.81 GB. Archive bytes in the
status table count only the original downloads.

## Status

| # | Source | Status | Archive bytes | Licence | Models |
|---|---|---|---|---|---|
| 1 | ABC (Koch et al. 2019) | **Blocked on an operator decision** about the download terms. Selection rule fixed; index files acquired; fetch script staged | 0 of 8.95 GB selected | Model copyright stays with the creators, licensed under Onshape ToU 1.g.ii; downloading means agreeing to a no-warranty term | 0 (30,000 slots selected) |
| 2 | Fusion 360 Gallery | **Not acquired.** The licence imposes obligations (operator decision) | 0 | Autodesk non-commercial research licence with an indemnity | 0 |
| 3a | NIST MBE PMI (CTC, FTC, STC, HTC, MTC, D2MI) | Acquired | 224 MB | Public domain in the US; "can be used without any restrictions" | 17 test parts (CTC 5, FTC 6, STC 5, HTC 1), the MTC box assembly and the D2MI models; 71 STEP files; native CATIA, NX, Creo, SolidWorks and Inventor |
| 3b | CAx-IF public STEP library | Acquired (21 files). Test-round models are members-only | 2.9 MB | Not stated | 21 exports of 8 test models; 53 STEP files |
| 3c | NIST/MTC box-assembly TDP (`mtc.zip`) | **Unobtainable today**: the NIST data server returned HTTP 524 | 0 | NIST open licence | — |
| 4 | Public TDPs (Ender-3, CR-10, Voron 2.4r2, Voron Trident, Rat Rig V-Core 3) | Acquired | 1.21 GB | GPL-3.0 ×4; CC BY-NC-SA 4.0 ×1 | 5 machines; 374 STEP files plus 2 full-assembly STEP files (241 MB and 185 MB) |
| 5 | Already held | Listed only | — | per source | see "Already held" |

The TSV has 81 rows, one per acquired archive or file: 75 for files in
`corpus/` (archives, the two nested assembly STEP files, ABC index files and
terms snapshots) and 6 for already-held sources.

## 1. ABC dataset

- **What.** One million Onshape public CAD models as B-rep (STEP converted
  from Parasolid, plus the native Parasolid), with meta, stat, feature, mesh
  and FeatureScript formats. Koch, Matveev, Jiang, Williams, Artemov,
  Burnaev, Alexa, Zorin, Panozzo, CVPR 2019.
- **URL.** <https://deep-geometry.github.io/abc-dataset/>. The chunks are
  hosted at `https://archive.nyu.edu/rest/bitstreams/<id>/retrieve`.
- **Version.** v00, the latest for every format. The site's repository
  `deep-geometry/abc-dataset` is at `392789113158b874bfa630098f5bbab3ed38309e`
  (MIT; this covers the site, not the models).
- **Licence and terms**, from the page snapshot `corpus/terms/abc-index.html`:
  - "The copyright of the CAD models is owned by their creators. For licensing
    details, see Onshape Terms of Use 1.g.ii."
  - "By downloading the dataset you agree to the following terms: The authors
    give no warranties regarding the dataset."
- **Onshape ToU 1.g.ii** (effective 15 July 2020, read 2026-10-17). Public
  documents owned by Free Plan users are licensed to "any End User or third
  party accessing the Public Document", worldwide and royalty-free, "to use,
  copy, modify, merge, publish, distribute, sublicense, and/or sell copies".
  There is one exception. Public documents created before 7 August 2018 that
  carry a LICENSE tab reserving greater rights keep those rights. ABC was
  collected in 2018, so some models may fall under this exception; the
  per-model `meta` file has to be checked once acquired.
- **Why not acquired.** Downloading is stated to mean agreeing to terms. The
  term only disclaims warranty, but the brief forbids accepting terms on the
  operator's behalf. This needs one yes or no from the operator. The selection
  is fixed and the script is staged, so acquisition is then one command.
- **Healing filter.** ABC's own pipeline (Koch et al. 2019) dropped models that
  needed shape healing and meshes that over-refined. The corpus is real-world
  sloppy, not adversarial. ABC parts are arbitrary-scale user sketches, so they
  run only at the relative tolerance levels (design §9.6).
- **Acquired so far.** The index files in `corpus/abc/lists/`: nine
  `<format>_v00.txt` chunk lists (100 chunks per format, 800 in `all_v00.txt`),
  `md5.yml` and `size.yml`. Their hashes are in the TSV.

### ABC selection rule (fixed 2026-10-17T22:24Z, before any ABC chunk was downloaded or any model examined)

Universe: the 100 v00 chunks `0000`..`0099` listed in the dataset's own
`step_v00.txt`. Each chunk holds 10,000 model slots.

1. Chunk choice. For each chunk number `NNNN`, compute the SHA-256 hex digest
   of the ASCII string `brepgpu-corpus-abc-v00-chunk:NNNN`. Sort ascending by
   digest and take the first three: `0048`, `0042`, `0004`.
2. Formats per chosen chunk: `step`, `meta`, `stat` and `para` (the native
   Parasolid original from which ABC's STEP was converted). `feat`, `obj`,
   `stl2` and `ofs` are recorded as available, not acquired.
3. Robustness sweep: every model slot in the three chunks (30,000 slots).
4. Speed sample: within the three chunks, take every model directory that
   contains at least one STEP file; compute the SHA-256 of
   `brepgpu-corpus-abc-v00-model:<8-digit model id>`; take the 1,000 smallest
   digests. A model's unit is its whole directory (all its STEP files).
5. No model is excluded or replaced for size, content, kernel behaviour or any
   result. Absent or empty STEP files are recorded as such.

Reproduce step 1 with:
`python3 -c "import hashlib;print(sorted(range(100),key=lambda i:hashlib.sha256(f'brepgpu-corpus-abc-v00-chunk:{i:04d}'.encode()).hexdigest())[:3])"`,
which prints `[48, 42, 4]`.

### Selected chunk files (from the dataset's `md5.yml` and `size.yml`)

| File | Bytes | MD5 (dataset) | Bitstream |
|---|---|---|---|
| abc_0048_step_v00.7z | 878,754,007 | fb74610a1832697548854ecdfb897d8b | 88820 |
| abc_0048_meta_v00.7z | 826,908 | cbb504bb8bc0f5f5a39532b8f6e01710 | 88818 |
| abc_0048_stat_v00.7z | 1,659,273 | f7ff0edb007c2b4813200abafd7aecd5 | 89234 |
| abc_0048_para_v00.7z | 1,839,058,736 | 6d1f03da375639960a9e7f717bb30f84 | 88819 |
| abc_0042_step_v00.7z | 1,088,842,040 | 153ddfb88d3b9fc0dd7e46afaebddcff | 88795 |
| abc_0042_meta_v00.7z | 776,696 | 4e74f5a867a1bbd5b172f1f41351e740 | 88793 |
| abc_0042_stat_v00.7z | 1,448,476 | 3f8163ad33632a79d84b42fd74558bc4 | 89216 |
| abc_0042_para_v00.7z | 1,879,933,362 | 1dc6446c9fb92114728fda0d22ae1bca | 88794 |
| abc_0004_step_v00.7z | 1,116,692,123 | 533ca5fb659c00e1a8f8b931074561c3 | 88642 |
| abc_0004_meta_v00.7z | 761,594 | ff06f43c70e0f6b2056a8c1cea15acda | 88640 |
| abc_0004_stat_v00.7z | 1,594,379 | ec7370127c7e37099fe5d0ac7329a743 | 89098 |
| abc_0004_para_v00.7z | 2,138,888,258 | 3b660a8943b927eb6b428e16ad1d7a42 | 88641 |

These total 8.95 GB. SHA-256 digests get added to the TSV once the files are
downloaded. The dataset itself publishes only MD5.

### Restoration (after the operator approves)

```sh
mkdir -p corpus/abc/lists corpus/archives/abc
for f in all meta para step stl2 obj feat stat ofs; do
  curl -fsSL -o corpus/abc/lists/${f}_v00.txt https://deep-geometry.github.io/abc-dataset/data/${f}_v00.txt
done
curl -fsSL -o corpus/abc/lists/md5.yml  https://deep-geometry.github.io/abc-dataset/data/md5.yml
curl -fsSL -o corpus/abc/lists/size.yml https://deep-geometry.github.io/abc-dataset/data/size.yml
# compare the list hashes with manifest.tsv, then for c in 0048 0042 0004 and f in step meta stat para:
#   curl -fL -o corpus/archives/abc/abc_${c}_${f}_v00.7z "$(grep " abc_${c}_${f}_v00.7z$" corpus/abc/lists/${f}_v00.txt | cut -d' ' -f1)"
#   and check it against md5.yml with md5sum -c
# extract each into a new directory: 7z x -ocorpus/sources/abc/<chunk>_<format> <archive>
```

## 2. Fusion 360 Gallery

- **What.** Autodesk Research B-rep datasets, repository
  <https://github.com/AutodeskAILab/Fusion360GalleryDataset> at
  `1084b881f3bb710267801d812d6e9286b8667059`. Docs and licence snapshots are in
  `corpus/terms/fusion360/`.
- **Subsets with B-rep and topology** (sizes from S3 `Content-Length`):

  | Subset | Contents | Archive | Bytes |
  |---|---|---|---|
  | Segmentation extended STEP (s2.0.1) | 42,912 STEP plus seg and timeline | `segmentation/s2.0.1/s2.0.1_extended_step.zip` | 506,333,119 |
  | Segmentation s2.0.1 | 35,680 parts: `.smt` (native ASM) plus STEP plus meshes | `segmentation/s2.0.1/s2.0.1.zip` | 3,326,258,457 |
  | Reconstruction r1.0.1 | 8,625 sequences: `.smt`, `.step`, `.obj`, JSON | `reconstruction/r1.0.1/r1.0.1.zip` | 2,103,449,157 |
  | Assembly a1.0.0 | 8,251 assemblies, 154,468 parts: `.smt`, `.step`, `.obj`, JSON with occurrences | `assembly/a1.0.0/a1.0.0_00..10.7z` (11 archives) | about 18.8 GB |
  | Assembly joint j1.0.0 | 32,148 joints, 23,029 parts | `assembly/j1.0.0/j1.0.0.7z` | 2,975,602,672 |

  All are under `https://fusion-360-gallery-dataset.s3.us-west-2.amazonaws.com/`.
  Segmentation STEP is scaled ×10 relative to its `.smt` (documented unit
  quirk).
- **Licence** (`LICENSE.md`, updated 11/2021):
  - "You may only access and/or use the Dataset subject to" its terms.
  - Use is limited to non-commercial research.
  - The dataset may not be redistributed in its entirety. Modified subsets
    carry the same non-commercial terms.
  - Users must not try to re-identify the creators.
  - The user "shall defend and indemnify Autodesk".
  - The licence can be terminated at any time.
  - A for-profit employer "shall also be bound".
- **Why not acquired.** Accessing the data accepts a licence with an
  indemnity, non-commercial limits and employer binding. That is the
  operator's decision. If approved, the preferred order is: s2.0.1 extended
  STEP (breadth, 506 MB), then assembly a1.0.0 (instancing), then
  reconstruction r1.0.1 (native `.smt` beside STEP).

## 3a. NIST MBE PMI CAD test models

- **What.** The NIST MBE PMI Validation and Conformance Testing models, which
  CAx-IF also uses for AP242 testing:
  - Combined Test Cases CTC 01–05 and Fully-Toleranced Test Cases FTC 06–11,
    with ASME Y14.5 GD&T drawings, PMI definitions and verification and
    validation results;
  - Simplified Test Cases STC 06–10 (2023);
  - Hole Test Case HTC (2024);
  - Modified Test Case MTC box assembly (four parts plus fasteners; NX and
    SolidWorks);
  - D2MI project models (NX).
- **URL.** <https://www.nist.gov/ctl/smart-connected-systems-division/smart-connected-manufacturing-systems-group/mbe-pmi-0>
  (page snapshot `corpus/terms/nist/download-free-cad.html`). The exact
  `www.nist.gov/system/files/...` file URLs are in the TSV source column.
- **Version.** Server `Last-Modified` is the revision marker:
  - PMI STEP files: 2024-06-19
  - FTC/CTC definitions: 2022-07-19
  - FTC-CTC CAD models: 2024-05-07
  - FTC and CTC older-CAD archives: 2024-05-07
  - STC v4: 2024-06-18
  - HTC v2: 2025-09-04
  - MTC assembly: 2025-09-04
  - D2MI: 2019-08-22
  - Drawings and results PDFs: 2021-10-21
- **Licence.** The READMEs inside the archives say the material is US
  Government work, "not subject to copyright protection and is in the public
  domain" under 17 USC 105, and "The test cases and CAD models can be used
  without any restrictions." Acknowledgement is requested; "the use of the NIST
  logo is not allowed". There is no click-through.
- **Counts.**

  | Archive | Contents |
  |---|---|
  | NIST-PMI-STEP-Files | 33 STEP: 17 AP242 (editions 1–3), 13 AP203, 3 CCD. `ftc_08_..._ap242-e1-tg` is tessellated, not B-rep |
  | NIST_MBE_PMI_FTC_Definitions | 6 geometry-only STEP, 20 PDF |
  | NIST_MBE_PMI_CTC_Definitions | 5 geometry-only STEP, 12 PDF |
  | NIST-FTC-CTC-PMI-CAD-models | 11 STEP; CATIA 14, NX 11, Creo 22 versioned files, SolidWorks 11, Inventor 11 |
  | NIST-FTC-PMI | 6 STEP; CATIA, NX, Creo and SolidWorks, older CAD versions |
  | NIST-CTC-PMI | 5 STEP; CATIA, NX, Creo and SolidWorks, older CAD versions |
  | NIST-STC-PMI-v4 | CATIA, NX, Creo and Inventor ×5. No STEP inside; STC STEP is in NIST-PMI-STEP-Files |
  | NIST-HTC-PMI-v2 | CATIA, NX and Creo, plus a PDF |
  | NIST-MTC-Assembly | NX 9, SolidWorks 8 parts plus 1 assembly, SAT, JT |
  | NIST-D2MI-Models | NX 6, 5 STEP, SAT |

  Also 11 drawing PDFs and 3 result PDFs. The archives total 71 STEP files.
  Extraction omitted no files.
- **TDP components.**
  - Present: native CAD in five systems, STEP AP203/AP242 with semantic and
    graphical PMI, GD&T drawings, PMI definitions spreadsheets, verification
    and validation results.
  - Missing: BOM (single parts, apart from the MTC), material and process
    specifications, CAM toolpaths and inspection programs. The MTC's CAM, QIF
    and inspection data are in `mtc.zip` (3c).

## 3b. CAx-IF public STEP library

- **What.** Synthetic STEP files checked by the CAx Implementor Forum,
  2008–2010. They cover:
  - as1 assembly with Geometric Validation Properties (volume, area and
    centroid written into the file: a ready exact-quantity oracle);
  - io1 colours and annotations;
  - s1 external references (multi-file assemblies);
  - dm1 material and density;
  - sg1 supplemental geometry.

  Exporters: OpenCascade/DataKit (OC), Pro/E (PE), Theorem/NX (TU), CoCreate
  (CM), CATIA V5 (C5) and I-DEAS (ID). Each ZIP's `.log` files are retained.
- **URL.** Files are live at `https://www.mbx-if.org/library/<name>.zip`. The
  library page itself is gone. Its last capture,
  `https://web.archive.org/web/20220127152634/https://www.cax-if.org/cax/cax_stepLib.php`,
  is kept as `corpus/terms/caxif/cax_stepLib.20220127.wayback.html` and lists
  each file's expected unit, volume, area and centroid.
- **The 2001 library**: nine STEP files (as1-id-203, as1-tc-214, boot, clamp,
  conrod, io1-pe-203, io1-ug-214, s1-id-214, s1-ug-203). They now exist only in
  the Internet Archive and were fetched with the raw `id_` capture URLs in the
  TSV. Every one starts `ISO-10303-21;`. Gmsh's held `io1-pe-203.step.gz` is the
  same model but not byte-identical (51,253 vs 51,254 bytes).
- **Schemas** of the 53 files: 35 AP214 (`AUTOMOTIVE_DESIGN`), 12 AP203
  edition 2, and 6 AP203 `CONFIG_CONTROL_DESIGN` (some with GVP or
  presentation-layer modules).
- **Licence.** None stated. The library page reads: "PDES, Inc. and ProSTEP
  iViP are providing these STEP files to organizations interested in
  implementing the STEP standard … Users can import the files at their own
  risk." Use for testing is clearly intended. Republishing the files is not
  addressed, so publish results only, not the files, until the operator
  decides.
- **Not obtainable.**
  - CAx-IF test-round models live in member areas (CAESAR, Nextcloud, the
    legacy member area); access requires membership.
  - `rec_prac_gvp_v32.pdf` (the GVP recommended practice) returned 404 at
    `www.mbx-if.org/documents/`.
- **TDP components.** STEP only. No native CAD, drawings, BOM or process data.

## 3c. NIST/MTC box-assembly TDP

- **What.** "Design, Manufacturing, and Inspection Data for a Box Assembly",
  DOI 10.18434/T4/1503154. It is the full TDP (design, CAM, QIF inspection)
  for the MTC parts in 3a.
- **URL.** `https://data.nist.gov/od/ds/73AE20D288CF02F1E05324570681502B1955/mtc.zip`
  plus `.sha256`. The landing page `smstestbed.nist.gov/tdp/mtc` does not
  resolve.
- **Status.** On 2026-10-17 the server stalled and then returned HTTP 524
  (origin timeout). The companion knowledge-graph dataset `mds2-2146`
  (`qif-ontology-dataset-V2.zip`, 10,680,120 bytes, SHA-256
  `9ae303335274645f14d8ceacf56f8defd174271fd645308905f0395d82ef1085`) failed
  the same way. Its record is kept in `corpus/terms/nist/mds2-2146.json`.
- **Next step.** Retry later with `corpus/_tools/fetch.sh <url> corpus/archives/nist/mtc-tdp/mtc.zip`.
  Licence: <https://www.nist.gov/open/license> (snapshot kept).

## 4. Public technical data packages

All five were fetched as GitHub codeload ZIPs at pinned commits, then
extracted with `scripts/ingest_zip.py --skip-symlinks`. The only file omitted
was one `.DS_Store` (Rat Rig). None of these repositories uses Git LFS. File
counts are the extracted working copies.

| Package | Commit | Licence | Native CAD | Neutral B-rep | Drawings | BOM | Firmware/config | Other |
|---|---|---|---|---|---|---|---|---|
| Creality Ender-3 (`creality-ender-3`) | 88c7758cea9d0d00a54fdb238bedb3b33425f409 | GPL-3.0 | SolidWorks: 110 SLDPRT, 34 SLDASM, 38 SLDDRW; 87 Creo/Pro-E part and assembly files (`ASM/`); 1 Fusion `.f3d` | 103 STEP, 2 IGES, 1 Parasolid `x_t`, 1 3DXML | 43 PDF and 43 DWG part drawings (54 DWG in total) | `Ender-3 BOM.XLS`, PCBA BOM | Marlin source, PCB files, wiring | User guide, test G-code. Windows driver binaries are untrusted and not to be run. Two nested `.rar` files not extracted |
| Creality CR-10 (`creality-cr-10`) | cc66e1d3cf2f4ee3c0f563175c67518c487bbfca | GPL-3.0 | SolidWorks 2 SLDASM plus 1 SLDPRT, eDrawings EASM | full-assembly STEP, IGES, Parasolid `x_t`, 3DXML | 2D PDF and DWG part drawings (62 PDF, 58 DWG) | `BOM/CR-10 BOM.xls`, board BOMs | Marlin source, schematics, wiring | — |
| Voron 2.4r2 (`voron-2`) | a192410e27ea345644ae5c4b29b4c9c40cbe1a73 | GPL-3.0 | Fusion `Voron_2.4r2_Assembly.f3d` (60.8 MB) | **full-assembly STEP**, 241,522,213 bytes (extracted to `corpus/sources/tdp/voron-2.CAD-step/`), 3 bed STEP | Bed and panel DXF and PDF drawings | not in repo (published on the Voron website) | Klipper configs, wiring PDFs | Assembly manual PDF, 155 STL printed parts |
| Voron Trident (`voron-trident`) | f04f747d4dea06315806f352abf57f767cedf2b4 | GPL-3.0 | Fusion: top assembly plus 16 sub-assembly and 3 experimental `.f3d` | **full-assembly STEP**, 185,183,500 bytes (extracted to `corpus/sources/tdp/voron-trident.CAD-step/`; `__MACOSX` omitted), 6 other STEP | Bed and panel DXF and PDF drawings | not in repo | Klipper configs | Assembly manual PDF, 145 STL |
| Rat Rig V-Core 3 (`ratrig-v-core-3`) | 4f5d91f42c9a9f7ec346c73728fa6329491aad8c | **CC BY-NC-SA 4.0** | none published | 261 STEP parts and panels (inch and mm panel variants) | 176 DXF, 1 PDF template | 16 BOM CSVs | RRF and Klipper configs, slicer profiles | 88 STL; nested ZIPs not extracted (they duplicate `cad/`). Submodule `v_core_1_3` (Rat-Rig/V-Core-Pro-1.3) is not in the GitHub ZIP |

TDP components missing in every package:

- tolerances and GD&T on the 3D models (the drawings carry only some dimensions);
- material and process specifications beyond the print settings in the manuals;
- CAM toolpaths for the machined or sheet parts;
- inspection instructions;
- calibration and test data.

The STLs are meshes and are not counted as B-rep.

Restoration for each package:

```sh
mkdir -p corpus/archives/tdp corpus/sources/tdp
curl -fL https://codeload.github.com/<owner>/<repo>/zip/<commit> -o corpus/archives/tdp/<name>-<commit12>.zip
sha256sum corpus/archives/tdp/<name>-<commit12>.zip   # compare with manifest.tsv
python3 -I scripts/ingest_zip.py corpus/archives/tdp/<name>-<commit12>.zip corpus/sources/tdp/<name> \
  --root <Repo>-<commit> --skip-symlinks
# Voron nested STEP: unzip -d corpus/sources/tdp/voron-2.CAD-step corpus/sources/tdp/voron-2/CAD/Voron_2.4r2_Assembly_STEP.zip
#                    unzip -d corpus/sources/tdp/voron-trident.CAD-step corpus/sources/tdp/voron-trident/CAD/trident_assembly.step.zip -x '__MACOSX/*'
```

ZIP roots are `Ender-3-<commit>`, `CR-10-<commit>`, `Voron-2-<commit>`,
`Voron-Trident-<commit>` and `V-core-3-<commit>` (note the lower-case
`core`).

### openmovement OM10 family

Only MAN VA01.3 is held (see 5). Other revisions on the member-download index
need an openmovement account: MAN VA beta revisions, MAN VB, AUT beta 01.3
(2023-05-22) and the printable holder.

OM10 TDP components:

- Present: STEP assembly, public technical sheet (gear data, casing section),
  release notes.
- Missing: native Inventor files, a complete drawing set, tolerances and fits,
  BOM, material and process specifications, CAM.

## 5. Already held (not re-acquired)

These are on the laptop under ignored `quarantine/`, restorable from
[the reference catalog](../papers/reference-catalog.tsv) and
[QUARANTINE.md](../QUARANTINE.md). Rows for them are in the TSV.

| Source | Revision | B-rep inputs (file counts) | Licence |
|---|---|---|---|
| OCCT test dataset 7.9.0 | release asset V7_9_0_beta1 | 1,494 `.brep`, 336 STEP, 58 IGES | no per-file statement; OCCT test material |
| step-conformance | 96f30bbe0954 | 27 STEP with `.truth.json` | CC0 1.0 |
| resources4truck | 9bf9de1426c5 | 10 STEP (4 ABC parts, OCCT analytic primitives) | Readme calls the ABC files MIT, an upstream claim that conflicts with ABC's own statement |
| Gmsh benchmarks | b4e84a43d02f | 35 `.brep` (1 gzipped), 35 STEP (9 gzipped), 3 IGES | GPL-2.0-or-later with exceptions |
| FreeCAD test files | cc8a5eb0dc81 | 29 `.brep`, 4 STEP | LGPL-2.1 |
| usd-brep TestFiles, Netgen tutorials | c9c5979f04ce, 31e85f811fd6 | 30 and 2 B-rep/STEP | Apache-2.0, LGPL-2.1 |
| OM10 MAN VA01.3 (the watch) | 2022-07-01 | 1 STEP, 18,756,909 bytes, SHA-256 `c60675ab…61e3` | CC BY-SA 3.0 (openmovement) |

## Verification

With a copy of this repository's `corpus/manifest.tsv` in the BRepGPU root:

```sh
awk -F'\t' 'NR>1 && $4 ~ /^corpus\// {print $5"  "$4}' corpus/manifest.tsv | sha256sum -c -
```

Every acquired row passed this check on the fleet host on 2026-10-17. The
`terms/` snapshots are live pages, so re-fetching them may give different
digests.

Restore NIST and CAx-IF files with
`corpus/_tools/fetch.sh <url from the TSV source column> <archive path>`, or
plain `curl -fL -o`. Extract them as follows:

- NIST ZIPs and the three CAx-IF `s1-*` ZIPs:
  `python3 -I scripts/ingest_zip.py <zip> <new dir> --root <zip stem> --skip-symlinks`
  (`s1-pe-214.zip`'s root is `s1_pe_214`).
- The other CAx-IF ZIPs hold a single top-level STEP: `unzip -d <new dir> <zip>`.

## For SHOPPING_LIST.md

### Operator decisions needed

| Item | Why we want it | Decision needed |
|---|---|---|
| ABC dataset download (3 chunks: STEP, meta, stat, Parasolid; 8.95 GB) | Priority-1 arena corpus: the speed sample and robustness sweep (design §9.4) | The site says "By downloading the dataset you agree to the following terms: The authors give no warranties regarding the dataset." Accept that term and run `corpus/abc/fetch-selected.sh` on the fleet host? Model licences then follow Onshape ToU 1.g.ii; models created before 2018-08-07 with a LICENSE tab keep reserved rights |
| Fusion 360 Gallery (s2.0.1 extended STEP first; then assembly a1.0.0, reconstruction r1.0.1) | Second large real B-rep source; native `.smt` beside STEP; real instanced assemblies | Its licence binds the user (and a for-profit employer) to non-commercial research use and an indemnity of Autodesk. Accept, or skip? |
| CAx-IF test-round models | Current multi-vendor AP242 exports with known issues | Member areas only. Seek CAx-IF membership or access via PDES, Inc. or ProSTEP iViP, or skip? |
| Publishing results on the CAx-IF public library files | Republishing the files is not addressed by any stated licence | Publish timings and verdicts only (default), or ask PDES/ProSTEP? |
| openmovement OM10 other revisions (MAN VB, AUT beta 01.3, earlier MAN betas) | More real horological geometry beside the watch | Needs an openmovement member account (free User tier advertised). Create one under the operator's account? |

### Unobtainable today (retry, no decision needed)

- NIST/MTC box-assembly TDP `mtc.zip` (DOI 10.18434/T4/1503154) and NIST
  `mds2-2146` `qif-ontology-dataset-V2.zip`: data.nist.gov returned HTTP 524.
- CAx-IF `rec_prac_gvp_v32.pdf` (Geometric Validation Properties recommended
  practice): 404 on mbx-if.org. Try a Wayback capture.

### Not pursued in this pass

- Voron BOMs and sourcing guides (published on the Voron documentation site,
  not in the repositories).
- Rat-Rig/V-Core-Pro-1.3 (submodule of V-Core 3; a different machine
  generation).
- LulzBot and Prusa production files. Prusa's MK4 and Core One repositories
  are not on GitHub under the names tried.
- The ASD-SSG STEP AP242 Benchmark. Only reports were seen; no files were
  checked.
