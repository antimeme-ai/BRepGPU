# Operator decisions raised by M4 and B-units

These are the questions only the operator can answer. They came out of the M4
gap lanes, the B1/B2 bug-fix units and gate G1's leftovers. M5 (the design
rewrite) needs them settled, or explicitly left at the default given here.

Each row has a bead (`bd show <id>`), the question, and the dispatcher's
recommended default. Where a lane applied a default to keep working, the row
says so. Answer by bead ID; "default" is a valid answer.

## A. Steel-man and measurement (highest leverage)

| Bead | Question | Recommended default |
|---|---|---|
| `brepgpu-12s.48` | Which NVIDIA angle arm is the steel-man? NVIDIA's SDK default is chord off with a 25° angle. Every recorded NVIDIA time so far was chord-only (0/0). | Run both arms, reported separately. For each, the owned mesher carries the matching term. The headline is the arm where both sides meet the same ε plus 25° angle (matched fidelity), with the chord-only arm beside it. |
| `brepgpu-12s.70` | May NVIDIA's arms include our own projected pcurves as a third arm? | Yes, labelled "our projected pcurves". It is never the headline. |
| `brepgpu-12s.56` | Do non-uniform or sheared placements that we refuse count as owned losses? | Exclude them from the speed ratio. List them as coverage gaps in their own table. |
| `brepgpu-12s.53` | Must a mixed-backend run give the same bytes every time? | Yes, by default: placement is a fixed function of input, configuration and devices. Load-reactive placement becomes an arena flag. |
| `brepgpu-12s.27` | CPU governor during arena runs | Leave schedutil. Every row records the delivered clock, measured by probe. Revisit only if variance hides effects. |
| `brepgpu-12s.67` | Turn AutoNUMA off host-wide for arena runs? | No. `numactl` memory binding already stops migration; record the state. |
| `brepgpu-12s.68` | GPU0's PCIe link runs at Gen3 x8, not x16 | Inspect or reseat it when convenient. Until then, record it per lane and never pool the two GPUs' transfer numbers. |

## B. Corpus and acquisition

| Bead | Question | Recommended default |
|---|---|---|
| `brepgpu-12s.28` | ABC download term ("no warranties") | Accept, and run `corpus/abc/fetch-selected.sh` on the fleet. It is priority 1. G5 also needs it to decide how common offset and extrusion surfaces are. |
| `brepgpu-12s.29` | Fusion 360 Gallery licence (non-commercial, plus indemnity of Autodesk) | Decline for now, because of the indemnity. Revisit for a non-commercial study only. |
| `brepgpu-12s.30` | BRL-CAD from an official source | Acquire when the official repository is reachable; skip the unverified mirror. |
| `brepgpu-12s.31` | Epic CADKernel under your account | Skip. |
| `brepgpu-12s.32` | Tracker reproducer attachments with unstated licences | Use as a private corpus only; never publish the files. |
| `brepgpu-12s.33` | Non-public OCCT Mantis fixtures | Re-author the failure families ourselves; request the originals from OCCT as well. |
| `brepgpu-12s.34` | CAx-IF membership | Defer. |
| `brepgpu-12s.35` | Publishing results on CAx-IF public files | Publish timings and verdicts only, never the files. |
| `brepgpu-12s.36` | openmovement member account (free) for other OM10 revisions | Yes, if you are willing to create it (we will not). |

## C. Geometry and fidelity

| Bead | Question | Recommended default |
|---|---|---|
| `brepgpu-12s.46` | Does collapse displacement count against ε? | Yes. It is inside the fidelity contract. |
| `brepgpu-12s.47` | May a collapse below ε change topology (fill a pinhole, close a gap into contact, absorb an island)? | No by default (the Euler check refuses). A flag allows it, with a record. |
| `brepgpu-12s.59` | A freeform clearance bound runs out of budget: refuse, or run a local triangle-pair test? | Tighten local ε first. If that fails, refuse the faces with a record. The local test becomes an arena flag. |
| `brepgpu-12s.60` | A gap narrower than the source's stated uncertainty | Mesh it faithfully with ε_loc < w/2 (D-features). |
| `brepgpu-12s.61` | A part whose only representation is tessellated | Pass its own triangles through, labelled "deviation unknown". Exclude it from fidelity claims and speed ratios. |
| `brepgpu-12s.69` | A torus with negative major radius (invalid per schema) | Refuse the face with a record. |
| `brepgpu-12s.66` | Missing plane-angle unit | Refuse only for solids with angle-bearing geometry. **Applied by B1 as the default.** |
| `brepgpu-12s.62` | IGES empty unit flag (inches by the standard's default) | Treat as declared, since the format defines it, and record it. |
| `brepgpu-12s.63` | Parasolid XT's metre convention | Treat as declared, since the format defines it, and record it. |
| `brepgpu-12s.64` | The caller states a unit that differs from the file's | Refuse, unless the caller passes an explicit override, which is recorded. |
| `brepgpu-12s.65` | Snapping declared factors to exact constants (for example 0.0174532925 → π/180) | Yes, within 1e-9, recording both values. |
| `brepgpu-12s.71` | PMI or supplemental geometry with no reachable unit context | Use the main representation's units, with a record. PMI is warn-only. |

## D. Output, formats and display

| Bead | Question | Recommended default |
|---|---|---|
| `brepgpu-12s.38` | Do triangles of indeterminate faces go into STL and 3MF? | No. They go to the internal schema and glTF only, and the part counts as incomplete for STL and 3MF. |
| `brepgpu-12s.39` | Sender validation-property mismatch | Warn and comply. |
| `brepgpu-12s.40` | Exit codes for partial runs | Confirm: 0 valid, 2 indeterminate, 3 incomplete, 4 nothing produced, 1 tool failure. |
| `brepgpu-12s.41` | LOD alternatives written into files? | No for alpha; in-process display only. |
| `brepgpu-12s.42` | Do we own renderer decimation below the topology floor? | Not for alpha; leave it to the renderer. |
| `brepgpu-12s.49` | Angle term in the default output, or only the render profile? | Render profile only. The default output promises ε. |
| `brepgpu-12s.50` | Which glTF renderer defines render acceptance? | A rasterizer (three.js or Filament class) for acceptance; a path-traced showcase later. |
| `brepgpu-12s.51` | Emit the 3MF Production extension in alpha? | Yes, with deterministic v5 UUIDs. |
| `brepgpu-12s.52` | Emit the draft glTF extensions (edge visibility, mesh features)? | No for alpha. |
| `brepgpu-12s.55` | Ever hide defect-band tangent steps in display? | Never. |
| `brepgpu-12s.57` | May nearby scales share a mesh made at the tightest ε? | No: one mesh per distinct scale. Grouping becomes an arena flag. |
| `brepgpu-12s.58` | Hidden occurrences in STL and 3MF | Write them and report them. |
| `brepgpu-12s.72` | 3MF separate items: layout-independent parts, or the assembly in place? | Assembly in place, as separate items. The writer never turns FP64 contact into overlap (G3's C2 budget). Contacts are reported. |
| `brepgpu-12s.73` | Cura's 30 µm vertex meld | Accept it and report it as Cura's limit; never coarsen to suit Cura. |
| `brepgpu-12s.74` | STL coordinate frame | Source frame. Recentre, with a record, only when δ exceeds the budget. |
| `brepgpu-12s.54` | PMI warning threshold c_max | 0.1 (10:1) in the named profile. A user may loosen it, and that is recorded. |

## E. Physics roadmap

| Bead | Question | Recommended default |
|---|---|---|
| `brepgpu-12s.43` | Restate D-imprint as per-face density tied to clearance, plus exact normals | Yes. |
| `brepgpu-12s.44` | Watch physics: joint-level or full contact-level? | Joint-level first, with contact only at the escapement. Full contact later. |
| `brepgpu-12s.45` | Where clearances come from when the source has none | A declared contact record supplied with the model. A horological default table later. |

## F. Repository

| Bead | Question | Recommended default |
|---|---|---|
| `brepgpu-8k0` | Rewrite public history to remove the fleet alias from the 2026-10-16 commits? | No. It is a host alias, not a credential, and a rewrite breaks every clone and PR link. |

## Operator answers (2026-10-19)

**Every row not listed here takes its recommended default.**

| Bead | Answer |
|---|---|
| `brepgpu-12s.53` | **Functional equivalence, not identical bytes.** A mixed-backend run must give functionally equivalent output every time. This matches the standing rule that bit-identical CPU/GPU output is not a goal. |
| `brepgpu-12s.68` | **GPU0's x8 link is the slot itself, not a fault.** Every test runs on both GPUs. Comparing transfer numbers between the x8 and x16 GPUs is forbidden. |
| `brepgpu-12s.28` | **Open.** The operator asked where the term is accepted. There is no click-through. The ABC page (deep-geometry.github.io/abc-dataset) states that downloading means agreeing that "the authors give no warranties regarding the dataset". The chunks come from NYU's archive, not Hugging Face. Waiting for a yes before `corpus/abc/fetch-selected.sh` runs. |
| `brepgpu-12s.29` | **Declined, permanently.** No Autodesk software or data enters this machine, except to study its features. The Fusion 360 Gallery is out. |
| `brepgpu-12s.30` | **Use a mirror.** BRL-CAD goes into quarantine as reference material, so a third-party mirror is acceptable. Record its provenance in the quarantine manifest. |
| `brepgpu-12s.36` | **Deferred.** The operator has an account. One other current OM10 revision exists. It waits until the first revision works properly. |
| `brepgpu-8k0` | **Rewrite done.** The public history was rewritten, and hooks now reject the alias in commits, messages and pushes. |
