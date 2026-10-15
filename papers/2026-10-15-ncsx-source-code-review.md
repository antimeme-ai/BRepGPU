# NCSX source geometry caller: independent code review

2026-10-15. **Accepted; no blocking source/geometry finding.** Read complete
`examples/render_ncsx.py`, `examples/ncsx_display.py`, public data attribution
and retained license against the actual acquired pinned SIMSOPT coefficient
files, reader and symmetry implementation. No product execution or code edits
occurred in this source-only review. Actual image/artifact review follows fleet
generation.

The caller requires the exact two acquired dataset hashes and validates the
31×18 coil and 136-mode/nfp3 FOCUS schemas. Three source base coils retain all
30 nonconstant Fourier modes, including the correct zero-mode cosine constants
and per-coil sinX/cosX/sinY/cosY/sinZ/cosZ columns. The tangent helper omits the
common positive 2*pi factor when differentiating against t; only its normalized
direction is used, so this does not change section frames. Three field periods,
both symmetry states and three base coils yield 18 actual instances. Rotation
followed by flipping both Y and Z matches the inspected upstream convention.
The FOCUS surface phase m*theta−3*n*phi and cylindrical XYZ conversion match
the original reader; it is labeled plasma rather than vessel.

Projected toroidal direction and source tangent form an explicit periodic
section frame, with degenerate directions rejected. Rail offsets and thickness
are illustrative. Cyclic Catmull controls interpolate the 64 retained samples
per rail. Degree-(3,1) control order is longitudinal cubic first, then each
adjacent rail pair. Clockwise section traversal gives the stated outward
du×dv convention for the untransformed sweep; the symmetry matrix flips two
axes and preserves orientation. All four side joins and longitudinal closure
reuse the corresponding source rail samples.

The 36×24 periodic plasma sample lattice and tensor-product Catmull-to-Bezier
matrix produce degree-(3,3) patches with the intended cyclic indices and
u-major/v-fast controls. End controls reuse identical lattice contributions,
including both periodic closures. This is a fitted display carrier, with no
global fit-error, embedded-solid or exact engineering-topology assertion.

The common scale-four normalization is applied to controls once and undone
once on downloaded positions. Unit weights, native envelope checks and retained
hexadecimal controls make the actual submitted binary64 geometry inspectable.
The exact rational Bernstein qualifier evaluates position and both first
derivatives for every actual returned query. Its quarter/half sampling values
are exactly representable in the submitted binary64 UV arrays. Descriptor,
query/count/order, slot/UV binding, successful statuses and finiteness are
checked. Periodic native positions are compared separately. Source Fourier-to-
carrier centerline/plasma residuals are separately sampled in original source
coordinates, correctly lacking a global error bound.

Display positions and lighting normals come from actual native first jets;
the renderer uses those values directly with one documented orthographic
camera and opaque depth buffer. Two-sided lighting does not claim solid
orientation qualification. The explicit scene metadata, original controls,
recipe and raw jets preserve the image's construction. The source files lack
a unit declaration and the scene has no calibrated dimension legend. Public
attribution retains exact hashes, original repository revision, MIT notice,
the default-order difference and absence of a paired-equilibrium claim.

Suggested small provenance hardening: compare native `jets.info.backend` to
the requested backend and record that observed value, rather than relying
solely on request arguments in scene metadata. The current Engine contract
does not silently fall back, so this is not a discovered execution error or
run blocker. The actual P100 caption still needs the fleet's real hardware/run
record; a device ordinal alone does not identify a GPU model. No as-built,
general B-rep construction or external-alpha qualification follows from the
render, and the captions correctly preserve those boundaries.
