# NCSX caller/display source review

Reviewed actual `examples/render_ncsx.py`, `examples/ncsx_display.py` and the
consumed native Python result layout against the accepted render plan. Geometry
and display contracts read sound; two bounded provenance findings require
resolution before the recorded fleet render. No code edits or execution here.
Fourier coefficient/symmetry semantics are reviewed independently by the source
owner; this review focuses on actual native qualification and displayed outputs.

## Native qualification and scale

The caller submits a complete patch-major/u-major/v-fast query array. Current
5×3 coil and5×5 plasma grids use exact dyadic UV values; the Fraction oracle
therefore evaluates the actual submitted binary64 parameters exactly. It checks
all native positions and both first partials against the actual binary64 unit-
weight control polynomials with independently evaluated Bernstein weights.
Degree/control offsets/counts/flags, domain, origin/scale, patch count and exact
returned slot/UV arrays are checked. Successful statuses and finite outputs
precede fitting diagnostics and display. This is meaningful arithmetic evidence,
not native CPU/GPU parity or a source-fit sample masquerading as native truth.

The data layout is component-major Sxyz/Suxyz/Svxyz. Downloaded values remain in
the local normalized frame. Multiplying positions by4 is correct for the emitted
scale4, zero-origin patches. Uniform positive scaling leaves unit du×dv normals
unchanged, so normalized native cross products are appropriate source/world
lighting normals. The polynomial grade limits are local-coordinate limits;
the saved recipe/layout and reports should preserve that convention.

All adjacent coil segments/sides and both periodic plasma axes compare actual
native boundary positions, including wraparound. Side creases are legitimate;
the operation does not claim a smooth manufactured solid. Held-out interior
centerline/plasma fitting samples compare against source Fourier evaluations,
and are correctly labeled sampled diagnostics, not global surface/chord bounds.

## Display input and depth

The display validates finite XYZ/unit normals, equal sample extents, exact native
sample count, native patch count, and contiguous complete tile coverage. It uses
source/world positions directly and applies no hidden scale. The camera basis
is orthonormal with right×up toward the viewer and source Z upward. Orthographic
pixel-center barycentrics and largest camera-Z selection correctly resolve
crossing surfaces, independent of triangle painter order. Both triangle signs
are handled; near-zero projected area is a display omission, not a claim of
model topology. Two-sided diffuse lighting is declared explicitly.

Complete camera/projection/crop, palette, light, raster convention, tile splitting,
normal convention and illustrative-section metadata are written. Captions
correctly identify18 modular coils/three periods, fitted plasma distinct from
vacuum vessel, illustrative pack sections and no as-built/global-error claim.
P100 labeling must be backed by the actual placement/binary record below.
Final image inspection still needs to check silhouette, occlusion, header/footer
clearance and cropping; a correct shader does not guarantee a useful composition.

## Findings

**ND1 — actual execution provenance is incomplete.** The saved recipe/display
summary currently repeats requested backend/device arguments, while the caller
does not assert the downloaded ResultInfo backend. Add the actual backend check,
order1 term count3 and data extent9×count, then record actual backend plus the
resolved CPU core and CUDA plugin hashes used in this run. Current SOURCES pins
caller/display/Python/data but omits those binaries. Geometry grades alone do not
distinguish a healthy CPU result from the advertised P100 result. Hardware model
can be recorded by the coordinated fleet environment record; avoid private paths
or hostnames in public metadata.

**ND2 — the final source check excludes rendering completion.** The current
end-source equality check executes immediately before render(). It must also
cover rendering completion, or record separate consumed evaluation/render pins.
A display-source edit after that check could otherwise leave published source
pins inconsistent with the rendered artifact. Reuse of saved qualified jets for
later display-only changes remains appropriate with explicit new render pins.

Both changes are narrow caller provenance corrections; no new geometry engine,
dependency or test suite is requested. After resolution and actual artifact/
visual inspection, this is suitable as the first source-based NCSX goal render.

## Disposition after root corrections

Re-read the actual corrected caller. ND1 is resolved: ResultInfo backend and patch
count are checked, term count3 and exact9×count data extent are required, actual
observed backend is saved, and native-libraries.tsv pins the resolved consumed
core plus its default same-directory CUDA plugin by basename/hash. The renderer
still receives the request label only after that observed-backend equality holds.
No private paths or hostnames are emitted. ND2 is resolved: both source and native
binary hashes are rechecked after render completes. No finding remains before
fleet execution. Accepted for the actual qualified run and subsequent artifact/
image inspection; this source review does not claim those unexecuted outcomes.
