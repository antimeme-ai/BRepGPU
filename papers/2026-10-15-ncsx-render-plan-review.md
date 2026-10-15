# NCSX first-render design/plan: source and visual review

Accepted `docs/NCSX_RENDER_PLAN.md`; no blocking finding. Reviewed against the
actual PPPL visual pack and acquired pinned Simsopt data/readers. No implementation,
reference-library import, fitting or product execution occurred in this review.

A stdlib CSV inventory confirms NCSX.dat has31 rows and18 columns: three base
curves with all modes0..30. The inspected original CurveXYZFourier loader defines
per-coil columns as sinX/cosX, sinY/cosY, sinZ/cosZ, with the constant from row0's
cosine entries. The configuration sets nfp3; its symmetry helper enumerates three
rotations and both stellarator-symmetry states, producing18 curves. Preserve
original file/reader revision and base/rotation/flip identity; do not invent a
TypeA/B/C column mapping from the documentary picture alone.

The stated136-mode FOCUS plasma input is explicitly separate from the coil
centerlines and is not a vacuum vessel or a validated paired equilibrium. The
plan correctly prevents invented vessel ports, castings and manufactured winding
forms. Cosmetic width/depth and interpolation resolution are declared display
choices. Exact native grades certify the interpolating binary64 patch controls,
not the source Fourier series everywhere, CAD fidelity or engineering clearances.
Sampled fitting residuals are correctly separated from a global bound.

Implementation cautions retained for source review:

- Use the original transform's actual matrix convention/order. The reference
  RotatedCurve evaluates row vectors against a transposed Z rotation and then
  the stellarator flip diag(1,-1,-1); rotation and flip do not commute per labeled
  instance. Record the applied matrix rather than describing it only as reflection.
- Parse/retain all original Fourier coefficients. Check FOCUS header/mode count,
  nfp and phase/units against its actual reader; distinguish full-torus versus
  per-period grid spacing. The36×24 display interpolant may not resolve every
  source harmonic. Actual held-out sampled residuals must remain visible rather
  than silently dropping modes or claiming Fourier-exact geometry.
- Closed coil rail frames and periodic interpolation must close at source and
  surface seams; native position seam checks concern the actual fitted controls.
  Flat pack sides can have legitimate normal creases. Do not smooth them into
  nonexistent round winding forms or treat artificial fitted chart joins as CAD
  topology. Native first-jet normals may shade the actual display surfaces.

The1800×1200 orthographic cage/core view is suitable if the complete nested
nonplanar coil geometry and central opening remain legible. The actual depth
buffer resolves occlusion. Fixed camera/crop/light/palette/sections and sampling
metadata allow later comparisons. Image/README labels should name source-based
NCSX coil envelopes and **plasma boundary**, then the actual P100 first-jet
operation, while keeping vessel/forms/ports/assembly topology as the CAD goal.
A source-derived scene is an appropriate first trophy; no as-built/native-import/
complete GPU B-rep or community-alpha claim follows.

Pinned source licenses/attribution, unchanged native binary IDs, submitted query/
descriptor binding, actual output qualification and separate evaluation/render
source pins are appropriately included in the plan. Saved-artifact and final
visual review remain necessary before publication. Ready for implementation.
