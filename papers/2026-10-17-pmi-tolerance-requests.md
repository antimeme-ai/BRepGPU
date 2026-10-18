# Tolerance requests against stated tolerances (PMI)

Research lane M4-G15 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G15.md](../docs/fast-mesher-campaign/briefs/M4-G15.md),
read with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Bead
`brepgpu-12s.16`. Stub: lifecycle map §4.1 G15 (stages S2 and S1,
contradiction X9). Binding operator decision: **D-PMI**. The mesher reads PMI to
warn when ε is loose against the tightest stated tolerance, or tighter than the
source's own uncertainty. It warns and complies. It never refuses.

What ran: one text-level census of PMI, units and uncertainty over every unique
STEP file in the NIST corpus, on the fleet host in the lane directory. No mesher,
OCCT, Boolean or NVIDIA code ran, and nothing ran on the laptop apart from
reading PDFs. Sources and read depth are in
[2026-10-17-pmi-tolerance-requests-sources.tsv](2026-10-17-pmi-tolerance-requests-sources.tsv).

Tags follow the lifecycle map. **[src]** means a source establishes the claim.
**[meas]** means this lane's census measured it. **[own]** means this lane
derived it. **[call]** is a designer's choice, open to override.

## 0. The answer

**The request-checking rule (R-PMI).** It is computed per semantic tolerance
*t* that the mesher can bind to faces (§5).

1. **Margin.** Compute the one-sided margin *m_t*: the distance, in mm along the
   surface normal, from the nominal surface to the edge of *t*'s zone on the side
   where the chord bias falls. §4 derives the side. §5 gives the table per
   tolerance kind. The chord bias cannot consume some tolerances: planar
   features, flatness, straightness and tangent-plane orientation. Those get no
   margin. *m_t* = 0 means the source nominal already sits on the limit.
2. **Consumption.** *c_t* = ε / *m_t*. After meshing it is recomputed as
   δ_f / *m_t*, where δ_f is each face's achieved chord bound (δ_f ≤ ε, and
   δ_f = 0 inside planes).
3. **Warnings.** All warnings carry *m_t*, *c_t*, the tolerance's source id and
   the ε that would bring *c_t* down to *c_max*.
   - **PMI-OUT (fact)**: *m_t* = 0 or *c_t* ≥ 1. A part made exactly to the mesh
     would leave *t*'s zone.
   - **PMI-LOOSE (policy)**: *c_t* > *c_max*. The default is **0.1** [call];
     §5.3 gives the basis. It lives in the named BRepGPU data-quality profile
     (D-profile), so a user may tighten it, and loosening it is recorded.
   - **UNC-FINE**: ε < *u*, where *u* is the uncertainty declared by the
     representation context that holds the meshed B-rep (not the maximum over
     all contexts).
   - **UNC-EXCEEDED** (after meshing): a measured source edge discrepancy is
     larger than *u*. The PDES practice already calls such a gap an error
     [src Pdes1998 §2.7.1].
4. ε is never changed, and no input is refused.

In the brief's "ratio k" terms, with *c_max* = 0.1:
- symmetric profile or position zones of width T on curved features: warn when
  ε > T/20;
- symmetric diametral size bands T: warn when ε > T/40;
- form and runout zones on curved features: warn when ε > T/10;
- nominal on a size limit: always warn.

These map to lane G11's record codes: B-RQ-PM for PMI-OUT and PMI-LOOSE as two
severities, and B-RQ-UN for UNC-FINE and UNC-EXCEEDED.

**Reported quantities** (§6). Per face *f*:
- δ_f;
- r_f, the largest displacement of the reconciled target from *f*'s own source
  surface;
- the combined bound to the source surface, b_f = δ_f + r_f, as a linear sum,
  signed where the side is known;
- u_ctx, the sender's declared coincidence, reported beside b_f and never added
  to it.

Per PMI feature of size:
- the mesh's mating size, two-point size range and least-squares size, from the
  correct inscribed-chord geometry (§4);
- the deviation of each from nominal;
- *m_t* and *c_t*.

Per run:
- the tightest margin;
- the nominal-model flag (tri-state);
- the dimensioning standard, where it can be read.

**The PMI subset read** (§3): only semantic representation on exact geometry:
- geometric tolerances with magnitude, modifiers and unequal disposition;
- dimensional sizes and locations with plus/minus bounds, value ranges and
  ISO 286 class strings;
- the toleranced-feature binding (single, multiple, composite, all-over);
- each context's units and uncertainty;
- the nominal-model flag and the dimensioning standard;
- the PMI validation properties, used as a reader self-check;
- general-tolerance text notes, verbatim.

Presentation, datums and saved views are not read. Neither is PMI on
tessellated geometry.

**Findings that change the design.**

1. **The brief's question 3 premise is wrong, and so is "bosses larger".** With
   vertices on the surface, every feature of size gets smaller. A hole gains
   material: its mating size falls by 2δ. A boss loses material inward: its
   mating size is unchanged and its two-point sizes fall by up to 2δ. Lane G17
   found the same and the dispatcher relayed the correction (§4).
2. **Nominal on the limit is common.** 11 size tolerances in 5 of the 17 NIST
   PMI models put the nominal on the lower limit: Ø6.35 +0.0762/−0 holes, and a
   Ø35 +0.2/0 hole in CTC-01 [meas]. A vertices-on-surface mesh of those holes
   is undersize for any ε > 0. The same happens to every ISO 286 H hole, because
   H puts the lower deviation at zero; that is the ISO 286 convention, and the
   standard is paywalled. The warning that matters most therefore has nothing to
   do with how small ε is.
3. **The relative arena requests sit above the parts' own tolerances.**
   - At 1e-3 of the bounding-box diagonal, all 17 PMI models have tolerances a
     mesh-made part would violate. The worst consumption is 84× the margin
     (FTC-09).
   - At 1e-4 of the diagonal, 11 of 17 still do [meas].
   - This quantifies contradiction X9: relative requests are meaningless on
     toleranced parts, and unit-trustworthy PMI inputs should run only at
     absolute requests.
4. **The declared uncertainty describes the exporter, not the part.**
   - The same NIST part declares 1e-6 mm in one export and 0.0338 mm in another
     (FTC-09). CTC-02 declares 0.119, 0.0508 and 0.01 mm across its three
     exports [meas].
   - The values span 1e-6 to 0.127 mm, under four different names.
   - UNC-FINE is therefore a statement about the file. Only the measured
     per-edge discrepancy says anything about the geometry.
5. **The draft's gross-edge threshold can refuse what the sender declares
   legal.** Design §5.2 refuses an edge whose discrepancy exceeds
   G = min(4ε, ¼ lfs). Take FTC-07 in AP242: u = 0.127 mm and ε = 0.005 mm, so
   G = 0.02 mm. Any real gap between 0.02 and 0.127 mm is legal by the file's own
   declaration and refused by R6. M5 must reconcile G with u. The healing
   thresholds are not this lane's to set (§8).
6. **No NIST model carries the nominal-model flag** (0 of 49 files) [meas].
   - The CAx-IF practice reads an empty name as "not a design intended to be
     manufactured" [src CaxIfPmiRp2024 §4]. Read literally, every NIST
     fully-toleranced model declares itself non-nominal.
   - The flag can only be read as tri-state: declared nominal, or not declared.
     It gates nothing.

## 1. What the sources establish

- **Five quantities that get conflated** [src lifecycle map S2]:
  1. design tolerance;
  2. model coincidence uncertainty;
  3. measurement uncertainty and the decision rule;
  4. specification uncertainty;
  5. mesh chord deviation.

  No source equates the chord deviation with any of the others, and none gives
  a chord-to-GD&T rule. This lane found none either.
- **Semantic PMI is for exact geometry only.** "PMI Representation (semantic)
  data shall only be used when exchanging exact geometry" [src CaxIfPmiRp2024
  §3.4]. It attaches to `advanced_face` through `shape_aspect` and
  `geometric_item_specific_usage`. Multi-face features use
  `composite_group_shape_aspect` ("multiple elements", "pattern of features").
  Part-level "all-over" tolerances point at `product_definition_shape`
  [src §5.1, §6.1, §6.3, §6.4].
- **Global uncertainty is a modelling tolerance**: "the maximum value for when 2
  points are considered equal" [src CaxIfPmiRp2024 §4.2]. In AP203 practice it
  is "the largest gap anticipated between elements that should be deemed
  coincident". A post-processor treats any larger gap as an error [src
  Pdes1998 §2.7.1].
- **Decision rules.** JCGM 106 defines Cm = T/(4u_m). It warns that "10-to-1"
  and "TUR 4:1" rules "are sometimes ambiguously or incompletely defined" [src
  Jcgm106 §7.6.2-7.6.4]. Simple acceptance can be wrong 50% of the time at a
  limit [src §8.3.1.2]. ISO 14253-1's default is guarded acceptance with guard
  bands w = U = 2u [src Jcgm106 §8.3.2, example]. Morse et al. state the same
  rule as "the uncertainty always 'plays against' who is performing the test"
  [src Morse2018 §4]. ISO 14253-1 itself is paywalled and was not read; both
  statements of it here are secondary.
- **General tolerances.** ISO 22081:2021 replaced ISO 2768-2. Its general
  geometrical specification is a profile against the CAD model's theoretically
  exact dimensions, and it applies only to integral features [src Petr2024 §1,
  §2.2]. In AP242, "applies to all untoleranced surfaces" should bind to a
  composite of those surfaces, and "applies to all surfaces" to the part
  [src CaxIfPmiRp2024 §6.3]. Neither rule is followed uniformly in the NIST
  files (§2.4).
- **Tolerance budgets add.** Fusion CAM documents its machining tolerance as
  "the sum of the tolerances used for toolpath generation and geometry
  triangulation" [src lifecycle map S14]. Morse et al. name CAD tessellation as
  a deviation source in additive manufacturing [src Morse2018 §7.1].
- **PMI does not survive exchange reliably.** In CTC-05, STEP kept 76% of PMI as
  representation and 39% as presentation [src Cheney2015]. A reader must
  therefore say what it found, not assume completeness. The validation
  properties provide the check (§7).

## 2. Census of the NIST corpus [meas]

### 2.1 Method

- **Probe.** `pmi_census.py` (about 670 lines) and `pmi_margins.py` (about 160
  lines), written for this lane. Both live in the lane directory on the fleet
  host and run under the host's Python 3.12.3 with `python3 -I`, in the
  2026-10-18 UTC run. No library outside the standard library is used.
- **Lexing.** The probe tokenises Part 21 instances, simple and complex, after
  stripping comments.
- **What it resolves.**
  - each context's length unit, through SI prefixes and conversion-based units,
    and each uncertainty's own unit;
  - which representation types reference each context;
  - `product_definition_shape.name`;
  - every `geometric_tolerance` subtype: magnitude in mm, modifiers,
    `unequally_disposed` displacement, and the target, resolved through the
    usages, composite groups and part shape to faces with surface kind and
    `same_sense`;
  - `plus_minus_tolerance` → `tolerance_value` or `limits_and_fits`, with the
    dimension name and nominal from the dimensional characteristic
    representation;
  - value ranges named "upper limit"/"lower limit";
  - the PMI validation properties;
  - the B-rep bounding box from the vertex points of the solids;
  - text notes that name standards, units or general tolerances.
- **Margins.** `pmi_margins.py` applies the §5 margin table and evaluates
  consumption at the four arena requests: 0.005 mm, 0.025 mm, 1e-4·diag and
  1e-3·diag.
- **Scope.** 71 STEP files across the NIST archives reduce to 49 unique files by
  SHA-256. 17 carry semantic PMI: 16 NIST CTC, FTC and STC AP242 files and one
  D2MI AP242 file. The CAx-IF library and the TDPs are outside the brief's
  census.
- **Not a timing.** The census is a file read, so no load or frequency context
  applies.
- **Limits.** The probe is text-level.
  - Face side comes from `same_sense` on cylinders, cones and spheres, assuming
    outward-oriented closed shells. Tori and B-splines are "unknown" and get the
    smaller margin.
  - Thickness and curve-length sizes touching a curved face are treated as
    diametral sizes, which is conservative.
  - Angular tolerances are not evaluated.

### 2.2 What the files contain

- **Records.** 618 tolerance records:
  - 375 geometric: position 164, surface profile 117, perpendicularity 38,
    flatness 23, parallelism 12, and fewer than 5 each of the other nine types;
  - 167 plus/minus sizes;
  - 41 locations;
  - 35 value-range sizes.
- **Size features are mostly holes.** Diametral sizes bind to 129 concave
  (hole) face sets and 5 convex (boss) sets.
- **Modifiers.** MMC 59, free state 14, LMC 7, tangent plane 6, statistical 2,
  separate requirement 2, and unequal disposition 3 (CTC-02).
- **Size-definition modifiers.** Two-point and least-squares modifiers never
  appear in these ASME parts. "Independency" appears 4 times (STC-07).
- **Not evaluable as written.**
  - **ISO 286 classes.** 6 sizes give only a class: G6 holes in FTC-10 and
    STC-10.
  - **Zero at MMC/LMC.** 4 position tolerances are 0 at MMC or LMC; their margin
    lives in the size band.
  - **Swapped limits.** 7 D2MI sizes write upper < lower. CAx-IF requires the
    upper bound to exceed the lower [src §5.2.3]. The census swaps them and
    records a finding.
- **Validation-property check.** In the 10 files that declare it, the geometric
  tolerance count agrees with the census in 9. STC-10 declares 34 and the
  census reads 37. STC-10 has 3 composite relationships, and the practice counts
  every constituent [src CaxIfPmiRp2024 Table 22 and following text]. The
  exporter therefore counted each composite once. Dimension counts differ
  wherever untoleranced dimensions exist, because the census counts only
  toleranced ones.

### 2.3 Units, uncertainty and the nominal flag

- **Nominal flag.** `product_definition_shape.name` is `''`, `' '` or `'NONE'`
  in all 49 files. None says `'nominal model'`.
- **Units.** 6 of the 17 PMI files encode lengths in inches; the others encode
  in mm. Many mm files carry a "UNITS: INCHES" drawing note.
- **Dimensioning standard.**
  - Three files declare it in the practice's structured form
    (`application_context` 'geometrical dimensioning and tolerancing …' plus a
    document reference): FTC-07, FTC-10 and STC-07.
  - CTC-05 references ISO 16792, a digital-product-definition practice, not a
    GD&T standard.
  - 12 of 17 state "ASME Y14.5M-1994 APPLIES …" in a text note, 9 of them
    only there. CTC-01, CTC-03, CTC-04, CTC-05 and the D2MI model state no
    standard that the probe could find.
- **Declared uncertainty of the B-rep context**, same part across exports
  (mm, with the name as written):

  | Part | AP203 geometry-only | AP203 with polyline PMI | AP242 semantic |
  |---|---|---|---|
  | CTC-02 | 0.119 ('closure') | 0.0508 | 0.01 ('point coincidence tolerance') |
  | CTC-04 | 0.005 | 2e-5 | 0.005 |
  | CTC-05 | 0.0932 ('closure') | 0.0508 | 0.0166 |
  | FTC-07 | 0.01 (inch context) and 1e-6 (mm presentation context) | — | 0.127 |
  | FTC-09 | 0.0338 ('closure') | — | 1e-6 |
  | FTC-10 | 0.01 and 1e-6 (presentation) | — | 0.005 |

  Across the 55 contexts, values run from 1e-6 to 0.127 mm, under four names:
  'closure', 'DISTANCE_ACCURACY_VALUE', 'point coincidence tolerance' and empty.

### 2.4 Binding of general tolerances

Eight PMI files say "APPLIES TO ALL UNTOLERANCED SURFACES" in a note.

- **Bound to the part.** FTC-07 and FTC-10 point the general profile at the
  `product_definition_shape`. The practice reserves that for "all surfaces"
  [src §6.3].
- **Bound to nothing.** In FTC-06 and STC-08 it points at a bare `shape_aspect`
  with no geometric usage, only a draughting callout. In all, 9 records resolve
  to no geometry.
- **Rule** [call]: an unbound or part-level geometric tolerance applies to every
  face for the warning check, and the binding is recorded as a finding.

### 2.5 Margins and consumption

At the two absolute requests, of 427 tolerances with an evaluable margin (11
of them zero):

| ε | c > 0.1 | c > 0.25 | c ≥ 1 or m = 0 | files with any c ≥ 1 |
|---|---:|---:|---:|---:|
| 0.005 mm | 75 | 55 | 20 | 5 of 17 |
| 0.025 mm | 271 | 125 | 65 | 9 of 17 |

The evaluable margins have median 0.127 mm, 10th percentile 0.015 mm and
minimum 0.004 mm. The minimum is FTC-09's 0.008 mm profile on a curved face.

| File | Tolerances | Tightest margin m (mm, kind) | u, B-rep (mm) | Warn / out at 0.005 | Warn / out at 0.025 | ε for c = 0.1 |
|---|---:|---|---:|---|---|---:|
| D2MI 827-9999-904 | 22 | 0.0635 position | 0.01 | 0 / 0 | 15 / 0 | 0.0064 |
| CTC-01 | 14 | 0.025 size Ø (+1 at m = 0) | 0.01 | 2 / 1 | 7 / 2 | 0.0025 |
| CTC-02 | 29 | 0.025 position | 0.01 | 4 / 0 | 12 / 4 | 0.0025 |
| CTC-03 | 21 | 0.0381 size Ø | 0.01 | 1 / 0 | 9 / 0 | 0.0038 |
| CTC-04 | 14 | 0.05 size Ø | 0.005 | 0 / 0 | 6 / 0 | 0.005 |
| CTC-05 | 12 | 0.0127 size Ø | 0.0166 (single context) | 1 / 0 | 4 / 1 | 0.0013 |
| FTC-06 | 47 | 0.1016 size Ø | 0.01 | 0 / 0 | 13 / 0 | 0.0102 |
| FTC-07 | 44 | 0.0127 size Ø (+3 at m = 0) | 0.127 | 8 / 3 | 14 / 6 | 0.0013 |
| FTC-08 | 42 | 0.0127 size Ø | 0.005 | 9 / 0 | 14 / 9 | 0.0013 |
| FTC-09 | 52 | 0.004 profile (+2 at m = 0) | 1e-6 | 30 / 11 | 41 / 28 | 0.0004 |
| FTC-10 | 63 | 0.04 size Ø | 0.005 | 2 / 0 | 20 / 0 | 0.004 |
| FTC-11 | 8 | 0.04 size R | 0.005 | 1 / 0 | 2 / 0 | 0.004 |
| STC-06 | 41 | 0.1016 size Ø | 0.005 (single context) | 0 / 0 | 11 / 0 | 0.0102 |
| STC-07 | 67 | 0.0127 size Ø (+3 at m = 0) | 0.005 | 7 / 3 | 43 / 6 | 0.0013 |
| STC-08 | 37 | 0.0127 size Ø | 0.01 | 8 / 0 | 13 / 7 | 0.0013 |
| STC-09 | 47 | 0.1016 profile (+2 at m = 0) | 2.54e-5 | 2 / 2 | 26 / 2 | 0.0102 |
| STC-10 | 58 | 0.06 perpendicularity | 0.005 | 0 / 0 | 21 / 0 | 0.006 |

"Warn" counts tolerances with c > 0.1, including the m = 0 cases. "Out" counts
c ≥ 1 or m = 0.

- **Relative requests.** At 1e-3·diag (0.06 to 1.17 mm here), all 17 files have
  out-of-zone tolerances, with worst c from 1.5 to 84.5. At 1e-4·diag, 11 of 17
  do.
- **UNC-FINE.** At 0.005 mm it fires on 8 of 17 files: six declare 0.01 mm,
  CTC-05 declares 0.0166 mm and FTC-07 declares 0.127 mm. At 0.025 mm it fires
  only on FTC-07. CTC-05 and STC-06 have one context, which the probe did not
  see referenced by a B-rep representation type; its value is used.

## 3. Question 1: the PMI subset to read

| Read | Entities | Used for | Basis |
|---|---|---|---|
| Units and uncertainty per representation context, each measure in its own unit | `global_unit_assigned_context`, `global_uncertainty_assigned_context`, `uncertainty_measure_with_unit` | u for UNC-FINE and UNC-EXCEEDED; mm conversion of every tolerance | RP §4.2; PDES §2.7.1; census §2.3 (two contexts four orders apart in one file; inch and mm contexts in one file) |
| Nominal-model flag | `product_definition_shape.name` | Reported tri-state; gates nothing | RP §4; census: 0/49 declare it |
| Dimensioning standard | `application_context` + `applied_document_reference`; else a text note matching "Y14.5 … APPLIES" or "ISO …" | Reported only. The bias rule does not depend on envelope versus independency (§4.4) | RP §4, Table 5; census: 3 structured, 12 notes |
| Geometric tolerances | the 14 types in RP Table 12, as simple or complex instances; magnitude; `geometric_tolerance_with_modifiers`; `unequally_disposed_geometric_tolerance.displacement`; `geometric_tolerance_relationship` 'composite' | Margin per §5 | RP §6.8-6.9.9 |
| Dimensional tolerances | `dimensional_size` (name per Table 4), `dimensional_location` and subtypes; `plus_minus_tolerance` → `tolerance_value`; value ranges; `limits_and_fits` strings; type-2 size modifiers (two point, least squares, inscribed or circumscribed) | Margin per §5; the modifier picks the size definition in §4 | RP §5.1-5.3 |
| Feature binding | `shape_aspect` → `geometric_item_specific_usage` / `item_identified_representation_usage` → `advanced_face`; `composite_group_shape_aspect`, `all_around_shape_aspect`, `between_shape_aspect` via `shape_aspect_relationship`; `product_definition_shape` (all-over) | Which faces' side and δ_f apply; unbound is recorded and applied to all faces | RP §5.1, §6.1-6.4; census §2.4 |
| PMI validation properties | `'number of …'` integer items | Reader completeness self-check, with the composite counting rule | RP §10.1 Table 22 |
| General-tolerance text | `descriptive_representation_item` text containing "UNLESS OTHERWISE SPECIFIED", "UNTOLERANCED", "ISO 2768", "ISO 22081" | Reported verbatim. ISO 2768 classes and ISO 286 classes are not evaluated without owned tables | RP §7.4.4; Petr2024 |

Not read:
- **Datums and datum targets.** The margins in §5 are bounded without the datum
  reference frame.
- **Graphic presentation.** Polyline and tessellated presentation, saved views
  and styling carry no values a reader can trust. The AP203 "with PMI" NIST
  files contain only polylines and no tolerance entity [meas].
- **PMI on tessellated geometry.** It is out of scope by the RP. The NIST
  `ftc_08 …-tg` file is tessellated-only and has no semantic PMI [meas].

Ownership: this is an owned reader over the Part 21 entities that the OCCT
bridge already holds. No library is proposed. The census probe shows the scope
is small: about 670 lines of standard-library Python for the census.

## 4. Question 3: feature-of-size bias from the correct geometry

**Correction.** The brief says "an inscribed chord makes holes smaller and
bosses larger". The second half is wrong. With every vertex on the exact
surface, a chord of a circle of radius R lies inside the circle, at distance
R cos α from the centre in the middle of the chord, where α = Δθ/2. The sag is
δ = R(1 − cos α) [src RepoFastTessellation §1.1].

- **Holes.** The material is outside the circle. The chord moves the surface
  into the void, so the mesh *adds* material and the hole gets smaller.
- **Bosses.** The material is inside the circle. The chord moves the surface
  into the material, so the mesh *removes* material.
- **Both.** Every local size gets smaller.

Lane G17 derived the same result for a pin in a hole [src
RepoContactInterfaces §1.3], and the dispatcher relayed the correction. The
lifecycle map's statement (hole mating diameter 2(R − δ)) is correct.

### 4.1 The polygon a mesh makes of a circular section [own]

For a full circular section with n uniform chords, δ = R(1 − cos(π/n)). Every
mesh point lies in the annulus R − δ ≤ r ≤ R about the true axis. That holds for
uneven chords too, with δ = the largest sag, which is ≤ δ_f ≤ ε.

| Size definition (ISO 14405-1 / ASME names) | Hole (internal) | Boss (external) |
|---|---|---|
| Mating envelope: largest inscribed for a hole, smallest circumscribed for a boss (ASME actual mating; ISO GX for holes, GN for bosses) | 2(R − δ): bias −2δ | 2R: bias 0, because the vertices still touch the exact circle |
| The other envelope: smallest circumscribed for a hole, largest inscribed for a boss | 2R: bias 0 | 2(R − δ): bias −2δ |
| Two-point (local) size | between 2R − 2δ and 2R (n even); between 2R − δ and 2R (n odd) | same |
| Least squares, boundary sampled uniformly by arc length | ≈ 2R − (4/3)δ | ≈ 2R − (4/3)δ |
| Least squares, sampled only at mesh vertices | 2R | 2R |
| Material change | added (toward MMC) | removed (toward LMC) |

The least-squares row comes from the mean radius over one chord:
R/2 · [1 + cos²α · ln(sec α + tan α) / sin α] = R − (2/3)δ + O(α⁴).
A numerical check gave ratios of 0.661, 0.665 and 0.666 for n = 8, 16 and 32.

Spheres and cones behave the same way, because their triangles lie inside the
convex hull of on-surface vertices [src RepoFastTessellation §1.1]. On a torus
the inner half is a saddle, so the bias there is two-sided (±δ). The same holds
for B-spline regions of mixed curvature sign.

### 4.2 What fails, per tolerance principle [own]

The size limits are [D + ei, D + es] with ei < es, as diametral deviations from
the nominal D.

- **Envelope requirement (ASME Rule #1 default).**
  - A hole's MMC is its lower limit. Its mesh mating size D − 2δ stays at or
    above it iff 2δ ≤ −ei.
  - A boss's MMC is its upper limit. Its mesh mating size is D, which is
    unchanged.
  - At LMC, both need a smallest local size D − 2δ ≥ D + ei, the same
    condition.
- **Independency (ISO 8015 default).** Two-point sizes must lie in the band.
  The smallest is D − 2δ, so again 2δ ≤ −ei.
- **Result.** For every feature of size and both principles, the inscribed
  chord fails the size tolerance exactly when **2δ > −ei**. Envelope versus
  independency changes which size is measured, not this condition. That is why
  the dimensioning standard is reported but decides nothing.
- **MMC/LMC-modified geometric tolerances** (virtual condition).
  - Hole at MMC: the boundary is D + ei − t. Mating D − 2δ clears it iff
    2δ ≤ −ei + t.
  - Boss at MMC: the boundary is D + es + t, and the unchanged mating size
    always clears it while es + t ≥ 0.
  - Boss at LMC: the inner boundary D + ei − t needs 2δ ≤ −ei + t.
  - Hole at LMC: the outer boundary is not approached.
  - A zero-at-MMC position on a hole (FTC-10, STC-10) is therefore checked
    against −ei/2. The census flags these four tolerances; it does not link them
    to their size rows (§2.1 limits).
- **Nominal on a limit.** If ei ≥ 0, the mesh fails at any δ > 0. The census
  found ei = 0 on 11 sizes in 5 of the 17 PMI models (§0 item 2). ISO 286 H holes
  are the same case. Fits whose band lies wholly above the nominal (a G hole)
  have a CAD nominal already outside its own band. The mesh moves the hole
  further from the band, by 2δ.
- **Location of a derived axis.** The mating circle of the mesh lies between
  radii R − δ and R about the true centre. Its centre therefore moves at most δ.
  Position, coaxiality, symmetry and axis orientation consume at most δ against
  a radial half-zone t/2.

### 4.3 How the bias is reported

Report it signed, per feature of size. The interval is [−2δ_f, 0] on diameter
for two-point sizes, −2δ_f for the hole's mating size and 0 for the boss's,
and −(4/3)δ_f for a uniformly sampled least-squares size. Give the size
definition named in the PMI, or the principle's default where none is named.
Never report it as ±δ, because the sign is known.

## 5. Question 2: the warning rule and its basis

### 5.1 Margins per tolerance kind [own]

m is the radial distance from the nominal to the zone boundary on the bias
side. The zone width is t (the magnitude), and the size deviations are
ei < es (diametral).

| Tolerance | Feature | m | Note |
|---|---|---|---|
| Flatness, straightness | any | none | Planes and straight lines mesh exactly |
| Any, with tangent-plane modifier | any | none | The tangent plane of a plane is exact |
| Orientation, location, profile | all faces planar | none | δ = 0 inside planes; boundary chords belong to the adjacent curved feature |
| Roundness, cylindricity, circular/total runout | curved | t | Floating zone; the polygon's form error is δ |
| Surface/line profile, bilateral | curved | t/2 | Unbound or part-level: applied to all curved faces |
| Profile, unequally disposed (displacement d on the outside of material) | convex: t − d; concave: d; unknown side: min | — | The sign convention of AP242 `displacement` is assumed to be ASME's "U"; AP242 text needed (shopping) |
| Position, coaxiality, concentricity, symmetry, orientation of a curved feature, regardless of feature size | curved | t/2 | The derived element moves ≤ δ (§4.2) |
| The same at MMC on a hole, or at LMC on a boss | curved | (−ei + t)/2 | Virtual condition (§4.2); with t = 0 it reduces to the size margin |
| Diametral size | curved | max(0, −ei/2) | §4.2; ei ≥ 0 gives m = 0 |
| Radius size | curved | max(0, −ei) | |
| Size between planes | planar | none | |
| ISO 286 class | — | not evaluated | Needs ISO 286 tables; recorded as such |

### 5.2 Why the margin, not the band

A mesh's chord error is not a measurement uncertainty. It is a bounded,
deterministic, one-sided offset of the representation.

- A part made to the mesh, or inspected against it, carries that offset into
  every conformance decision. Morse's statement of ISO 14253-1 ("uncertainty
  always plays against" the prover) and the guard band w = U of JCGM 106 §8.3.2
  both remove the error *from the side it can push across* [src].
- A one-sided offset removes all of δ from one side and nothing from the other.
- So the comparable quantity is δ against the one-sided margin. The full band
  is the wrong denominator.
- For a symmetric zone, a centred process loses 2δ of usable width from T. The
  consumption c = δ/(T/2) = 2δ/T is exactly the fraction of the zone the mesh
  takes from a symmetric process.
- The lifecycle map's analogue "δ ≤ T/20 for features of size" compared the
  diametral bias 2δ against T/10. The margin form instead gives δ ≤ T/40 for a
  symmetric size band at c_max = 0.1. The diametral bias spends both sides' worth
  of 2δ on one limit.

### 5.3 Choosing c_max [call, with basis]

- **No source fixes the number for meshes.** JCGM 106 names the practice ratios
  10-to-1 (gauge maker's rule) and 4:1 (TUR) and calls both ambiguous unless
  stated precisely [src §7.6.4]. Stated precisely here, they become
  c_max = 0.1 (the mesh takes at most 10% of the usable zone) and c_max = 0.25.
- **The default is 0.1.**
  - At the watch default ε = 0.005 mm, 0.1 fires on 12 of 17 NIST PMI models
    (75 of 427 tolerances). 0.25 fires on 8 (55 tolerances).
  - These are fully toleranced test parts. FTC-09 carries an 8 µm profile, and
    a warning that names ε_needed (0.0004 mm for FTC-09) is information, not
    noise.
  - PMI-OUT fires independently of c_max on 5 models.
  - D-profile lets a user loosen c_max, and the loosening is recorded.
- **Not settled by literature.** Whether a mesh at c ≤ 0.1 changes conformance
  decisions is measurable. §9 H4 states the measurement.

### 5.4 The uncertainty side

- **UNC-FINE fires when ε < u_ctx.** ε below u means the sender itself only
  claims coincidence to u. The mesh is still within ε of the reconciled solid
  (design §4.2). It is not within ε of "the source", because the source's own
  faces may disagree by up to u at an edge.
- **The comparison is k = 1.** No source supports a factor, and u is a
  declaration, not an accuracy (§2.3).
- **UNC-EXCEEDED** is the a-posteriori check that PDES prescribes [src
  Pdes1998 §2.7.1]. It fires when a measured edge discrepancy d_e is larger
  than u. This is the "measured above stored" fault that the lifecycle map
  notes is the harder one (S15).

## 6. Question 4: the combined bound and how it is stated

Per face f [own, from the design's quantities]:

- **δ_f**: the achieved chord bound against the reconciled target. It is ≤ ε and
  is 0 inside planes. Its side is known for revolved faces and given as an
  interval.
- **r_f**: the largest distance between the reconciled target and f's own source
  surface. It is 0 when all f's edges resolved R1 (exact). Otherwise it is the
  recorded edge displacement that the design's boundary-band field decays from
  (design §4.2), and it is two-sided.
- **b_f = δ_f + r_f**: the bound from the mesh to f's source surface. It is a
  linear sum, because both terms are deterministic worst-case bounds. A
  root-sum-square combination is for standard uncertainties of independent
  random effects, and neither term is one [own]. Practice adds such budgets too
  (Fusion CAM, §1).
- **u_ctx**: the sender's declared coincidence, reported beside b_f and not
  added to it. No source says u bounds the distance from the B-rep to design
  intent. PDES defines it only as the largest gap between entities deemed
  coincident.
- **d_e**: the measured per-edge source discrepancy, already in the design as a
  starting-condition metric, never charged against ε.

How it is stated to a person. Each statement is generated from the same
records as G11's machine records. Lengths are shown in mm and also in the PMI's
display unit when its notes say inches.

> Part FTC-07 (nominal-model flag not declared; ASME Y14.5M-1994 from a note).
> (Illustrative: the census gives the limits; δ_f = 0.0041 mm is an assumed
> achieved bound.)
> Request ε = 0.005 mm. The sender declares coincidence to 0.127 mm: the request
> is finer than the file claims to be (UNC-FINE).
>
> Face #… (hole, Ø6.350 +0.076/−0 mm, .2500 +.0030/−.0000 in): mesh within
> 0.0041 mm of the reconciled solid and of its source surface. The mesh hole's
> mating diameter is 6.3418 mm and its two-point sizes run 6.3418-6.3500 mm. A
> part made to this mesh is 0.0082 mm under the lower limit (PMI-OUT), at any ε,
> because the nominal is on the limit.
>
> Tightest evaluable margin: 0.0127 mm (size). ε for c = 0.1: 0.0013 mm.

Run level: the counts per warning class, the tightest margin, and the
validation-property agreement.

## 7. Question 5: the NIST FTC and CTC fixtures

The corpus holds the fixtures (corpus/MANIFEST.md §3a). Each fixture's oracle
is independent of our reader.

| Fixture | Files | Oracle | Tier |
|---|---|---|---|
| Reader completeness | the 10 AP242 files with validation properties | Our tolerance counts equal the sender's `'number of …'` counts under the RP Table 22 rule. STC-10's 34-versus-37 is a known exporter deviation (composites counted once), kept as an expected finding | Specification (sender declaration) |
| Values | FTC-06…11 and CTC-01…05, against the NIST definition drawings and PMI definition PDFs in the corpus | A hand-built expected table of each tolerance's value, modifiers and target from the drawings, written once in M8 | Specification |
| Units, metamorphic | any AP242 PMI file rewritten from mm to inch (values ÷ 25.4, unit entity changed) | All margins in mm unchanged. Also: inch-encoded and mm-encoded parts give the same bounding box (CTC-01 930.1 mm in both) | Metamorphic |
| ε, metamorphic | all | c scales linearly with ε, warnings are monotone in ε, and ε_needed reproduces c = c_max exactly | Metamorphic |
| Nominal on a limit | FTC-07, FTC-09, STC-07, STC-09, CTC-01 (the 11 ei = 0 sizes) | PMI-OUT at every ε > 0; after meshing, the measured mesh mating diameter is D − 2δ_f (§4.1) | Specification / model |
| Negatives | AP203 geometry-only and AP203 polyline-only files; `ftc_08 …-tg` (tessellated) | No semantic PMI read and none claimed; the reader says "presentation only" or "no semantic PMI" | Specification |
| Binding anomalies | FTC-06 and STC-08 (unbound general profile); FTC-07 and FTC-10 (part-level "untoleranced"); D2MI 904 (upper < lower) | The finding is emitted, and the tolerance still enters the check | Specification |
| Uncertainty per context | FTC-07, FTC-10 and FTC-11 AP203 (two contexts) | The u used is the B-rep context's, never the presentation context's 1e-6 mm | Specification |

These fixtures cover the reader and the warning rule. The mesh-side check
(mating size equals D − 2δ_f) needs meshed output and runs in the offline
oracle stack (design §10).

## 8. Contradictions and cross-lane findings

1. **X9 confirmed and quantified** (§2.5). Relative requests run above the
   tolerances of every NIST PMI part. Recommendation for M5 §9.6: inputs whose
   units are trustworthy and that carry PMI run only at absolute requests. The
   arena still records the PMI warnings it would have raised.
2. **G versus u** (§0 item 5). R6 refuses edges with discrepancy above
   G = min(4ε, ¼ lfs). A sender may declare a larger u, and does: FTC-07 AP242
   declares 0.127 mm. The census does not show whether real gaps exceed G; that
   needs the edge measurement (§9 H5). For M5 and the healing default, not
   decided here.
3. **Uncertainty unit and context** (design §5.1, lane G6). The reader must
   convert each uncertainty measure through its own `unit_component`. FTC-07's
   AP203 B-rep context is in inches with its uncertainty in inches. Its second
   context is in mm with 1e-6 mm. The reader must also use the context of the
   B-rep, not the maximum over all contexts. The code does neither (§10).
4. **Never beautify, still warn.** D-PMI says comply. The mesher therefore
   meshes the nominal even when PMI shows the nominal on a limit. A mid-limit or
   two-sided (vertices-off-surface) placement would change geometry, which S1
   forbids. It is recorded here as a possible future option, not proposed.

## 9. Hypotheses and falsifiers

- **H1 (bias geometry).** For any cylindrical hole or boss meshed with vertices
  on the surface, every mesh point lies within [R − δ_f, R] of the true axis.
  The hole's mating diameter is 2(R − δ_max), and the boss's is 2R within
  floating-point noise.
  - *Falsifier*: a mesh point outside the annulus, or a mating size off by more
    than 1e-9·R, on the authored cylinder families and the NIST holes.
- **H2 (reader units).** The mm-to-inch rewrite of every PMI file leaves every
  margin unchanged to 1e-12 relative.
  - *Falsifier*: any change.
- **H3 (reader completeness).** Geometric-tolerance counts equal the declared
  validation properties in every file except where the exporter's composite
  counting differs (STC-10).
  - *Falsifier*: any other mismatch.
- **H4 (the threshold means something).** Sample the exact B-rep and the mesh,
  evaluate each NIST tolerance's size, profile and position on both with the
  same evaluator, and add simulated symmetric manufacturing scatter at
  0.5·margin. The fraction of conformance decisions that differ between exact
  and mesh is below 1% for every tolerance with c ≤ 0.1, and above 1% for some
  with c > 0.25.
  - *Falsifier*: flips at c ≤ 0.1 (c_max too loose), or no flips until c ≥ 1
    (c_max needlessly strict).
  - This is an offline arena measurement.
- **H5 (u is not accuracy).** Over the 49 NIST files, the measured maximum edge
  discrepancy (design §5.2 step 2) does not track the declared u across exports
  of the same part.
  - *Falsifier*: measured gaps scale with u across the CTC-02, CTC-05 and
    FTC-09 export triples.
  - This also answers §8 item 2.

## 10. Defects found by reading

- `native/step_source/main.cpp:277` takes the maximum of
  `UncertaintyMeasureWithUnit::ValueComponent()` over every uncertainty entity
  in the file. It ignores each measure's own `unit_component` and the context
  the measure belongs to. `main.cpp:427` then scales that raw number by the
  single context factor.
  - Consequences: a presentation context's value can be taken for the B-rep's,
    and an uncertainty written in a unit other than the context's is scaled
    wrongly. Today the mm-only refusal at `main.cpp:246` hides the unit case.
    It becomes live when G6 lifts that refusal.
  - The lifecycle map recorded the "one maximum" half (S2). The unit-component
    half is new.
- None in the PMI path, because no PMI reader exists (`native/step_source`
  reads none; lifecycle map S3 "PMI I").

## 11. Operator questions

None block this lane. One call is open to override: **c_max = 0.1** (§5.3), with
0.25 as the stated alternative. The operator may set it or leave it to the H4
measurement.

## 12. For SHOPPING_LIST.md

Items already listed by M1 are not repeated (ASME Y14.5-2018 and Y14.41,
ISO 1101, ISO 8015, ISO 14405-1, ISO 2768-1, ISO 22081, ISO 286-1,
ISO 14253-1, ASME B89.7.3.1). New:

- **ASME Y14.5.1-2019**, mathematical definition of dimensioning and tolerancing
  principles. It is the precise source for the mating-envelope and
  virtual-condition definitions used in §4.2. Paywalled.
- **ISO 10303-242 normative text** for `unequally_disposed_geometric_tolerance.displacement`
  and `tolerance_zone` semantics. It fixes the sign convention assumed in §5.1.
  The CAx-IF practice shows only a figure (§6.9.4). Paywalled.
- **ISO 286-1 tables in an owned, licensed form.** They are needed to evaluate
  `limits_and_fits` classes (6 occurrences in the census). Listed by M1 as a
  standard. This entry notes that the data tables are what the reader needs.
- **NIST MTC box-assembly TDP** (`mtc.zip`, HTTP 524 at M3). Its QIF inspection
  data would give a measured conformance oracle for H4. Already on the list as an
  M3 retry. Noted here for its new use.
