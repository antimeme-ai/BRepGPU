# M3 brief: corpus acquisition

**Purpose.** Acquire the B-rep inputs that every speed and fidelity claim rests
on. Without them, the arena has nothing honest to run.

**Serves the goal by.** It supplies the arena corpus of the design (§9.4): broad,
real, not watch-shaped, with licences and provenance recorded so results can be
published and reproduced.

**Read first.**
1. `BLACKBIRD.md`, especially "Engineering source material: CAD, CAM, and
   technical data packages".
2. `AGENTS.md`.
3. `QUARANTINE.md` and `papers/reference-catalog.tsv`, which set the acquisition
   and restoration conventions.
4. `journal/2026-10-17-fast-mesher-direction.md`.
5. `docs/FAST_MESHER_DESIGN.md` §9.4 and §9.6 (corpus and tolerance plan).
6. `papers/2026-10-17-mesher-test-plans-cad.md` §(2) and §(3), and
   `papers/2026-10-17-mesher-test-plans-meshgen.md` Synthesis §2, which name
   the cases worth adopting.

**Where data lives.** The laptop is 99% full (about 23 GiB free), so corpus data
lives on the fleet host. Use the existing fleet SSH alias given to you by the
dispatcher, with `-o BatchMode=yes`. Store data under the fleet BRepGPU root in
a new `corpus/` directory that is outside any Git checkout. On the laptop, keep
only the tracked manifest and small metadata. Never put the host's name or
absolute paths in tracked files: refer to "the fleet host" and to paths relative
to the BRepGPU root.

**Sources, in priority order.**
1. **ABC dataset** (Koch et al. 2019).
   - Record its licence and terms, and those of the underlying Onshape public
     models.
   - Acquire a reproducible sample: specific chunk files by name and hash,
     with STEP and the metadata the dataset ships per model.
   - The selection rule is fixed and written down before anything is looked
     at.
   - Note that ABC's own pipeline filtered out models that needed healing.
2. **Fusion 360 Gallery** B-rep data: record licence and terms. Prefer
   subsets that carry STEP or native B-rep with topology.
3. **NIST**: CAx-IF test models and the NIST MBE/PMI CAD test models (CTC/FTC
   and STC families, AP203/AP242 variants). Record their public-domain status
   or other terms.
4. **Public technical data packages** carrying native CAD plus STEP: open
   hardware projects (e.g. the openmovement OM10 source family, printer and
   machine projects). For each, record which TDP components are present and
   which are missing (drawings, tolerances, BOM, process specifications), as
   BLACKBIRD requires.
5. **Already held** (just list them in the manifest): the OCCT 7.9.0 dataset,
   step-conformance, resources4truck, and the Gmsh and FreeCAD test STEP/BREP
   files in quarantine.

**Outputs.**
- **Data** on the fleet host under the BRepGPU root's `corpus/`. Intact
  original archives are kept; extracted working copies have detritus and nested
  Git metadata stripped.
- **`corpus/MANIFEST.md`**, tracked in the repository. For each source: what it
  is, URL, version or revision, acquisition date, licence and terms, file
  counts, archive SHA-256, selection rule for samples, restoration commands,
  and missing TDP components.
- **`corpus/manifest.tsv`**, tracked. One row per acquired archive or file:
  name, source, revision, archive name, SHA-256, size, licence, destination
  relative to the BRepGPU root.
- **SHOPPING_LIST.md entries** for anything that needs an account, payment or
  an operator decision. Write them as a section titled "For SHOPPING_LIST.md"
  in MANIFEST.md; the dispatcher merges them.

**Done when.** Each priority source is acquired, or recorded as unobtainable
with the reason, and the manifest lets someone else restore the corpus on
their own host.

**Must not.**
- Run any geometry, mesher, OCCT or NVIDIA code on the corpus. Acquisition
  only; integrity checks such as hashes and `file` are fine.
- Fill the laptop disk.
- Publish hostnames or absolute paths.
- Commit.
- Accept terms or create accounts on the operator's behalf.

**Return.** A table of sources with status, size, licence and model count,
plus anything needing an operator decision.
