# NCSX first trophy: source facts and a comparable engineering view

Bounded source/visual study only. Inspected the acquired original PDF text,
rendered drawing pages and assembly photograph; no image generation, new library,
product implementation or geometry execution. The reference pack contains no
native CAD, machine-readable coil trajectories or vessel control nets.

## Facts the acquired primary material establishes

- PPPL-4742 PDFpage5 Figure4 explicitly identifies **18 modular coils and three
  distinct coil geometries**. Chrzanowski's manufacturing presentation page2
  names the shapes TypeA, TypeB and TypeC. They are strongly nonplanar,
  asymmetric kidney/bean-shaped loops with changing out-of-plane depth.
  [PPPL-4742](https://bp-pub.pppl.gov/pub_report/2012/PPPL-4742.pdf),
  [modular-coil manufacturing](https://ncsx.pppl.gov/Closeout/Chrzano_Mod_Coil.pdf)
- The vessel preliminary design states **three field periods, six modular coils
  per period, 120° repetition**, with stellarator symmetry relating the two60°
  halves through toroidal reversal and an up/down flip. It separately lists
  an18-coil TF system: do not confuse those coils with the18 modular coils.
  Its table's major radius1.4m/minor radius0.33m are device/plasma parameters,
  not an independently defined vessel wall surface.
  [vessel preliminary design, printed pp.3–5](https://ncsx.pppl.gov/Project_Control/PDR_PC/restricted/SSD_WBS_11_12_final.pdf)
- The actual vessel is a highly shaped Inconel625 shell. Fabrication used three
  identical120° assemblies, each comprising two60° sections. The complicated
  diagnostic/heating ports and their orientations are integral engineering
  features; ports were temporarily removed/rejoined for coil assembly.
  [PPPL-4114, introduction and fabrication figures](https://bp-pub.pppl.gov/pub_report/2006/PPPL-4114.pdf)
- Manufacturing pages3–5 show copper winding packs on a machined tee-shaped
  winding form/shell, with insulation, chill plates, clamps and leads. These
  are not round copper plumbing tubes or unsupported armillary rings. A simple
  swept section can depict a centerline, but is an illustrative envelope rather
  than the manufactured winding pack/form.
  [Chrzanowski, pages3–5](https://ncsx.pppl.gov/Closeout/Chrzano_Mod_Coil.pdf)
- NCSX was cancelled in2008 before completing the machine. Render an intended
  assembly/study reconstruction, not a photographically asserted completed or
  operating device. [PPPL-4742, introduction](https://bp-pub.pppl.gov/pub_report/2012/PPPL-4742.pdf)

## Views that should drive the first composition

PPPL-4742 page4 Figure1 is the complete-core hierarchy reference: shaped vessel/
plasma inside the modular-coil set; TF/PF/trim/solenoid, cryostat and supports are
separate surrounding systems. Its Figure2 exposes the vessel's three-period
shape and many non-radial ports. Page5 Figure4 is the best coil-cage composition
reference: an oblique view reveals the central opening, coil nonplanarity and
closely packed inner legs. Do not replace this with evenly spaced circular hoops.

The manufacturing page2 orthographic trio reveals the A/B/C differences and the
large depth of each form. Page4 shows why a copper band on a graphite/silver
form reads as an engineering component. The actual half-period photograph
confirms tight nesting, irregular inner openings and substantial structures.
Vessel-design PDFpage12/printedpage7 Figure4 gives dimensioned orthographic
views; page13 gives the port map. Those drawings are preliminary design, while
the manufacturing/closeout material is later: do not mix revisions into an
as-built dimensional claim.

## Proposed first image

Use a single un-exploded oblique coil cage around a three-period core: dark
neutral background, warm copper winding envelopes, restrained cool silver/blue
inner surface. Preserve the toroidal opening and visible gaps between distinct
coils. Highlight one complete field period subtly, with the remaining two in a
slightly quieter material, so three-period repetition is legible without making
the machine look like three disconnected rings. No invented ports, bolt patterns,
support rods or photorealistic finished winding forms from absent source data.

If the acquired numerical inner surface is a plasma/VMEC boundary, label it
**plasma boundary**. It is not the vacuum vessel. If only a drawing-based shell
is available, label it a **schematic vessel envelope** and preserve the fact
that its coefficients/clearances are reconstructed. A core omitted because no
source geometry exists is more honest than naming a generic torus the vessel.
An optional small source-reference inset can show the intended ported vessel,
explicitly credited as a PPPL engineering concept image, distinct from our output.

Suggested comparable rendering baseline (display choices, not source facts):
1800×1400 canvas; Z up; orthographic camera at azimuth35°, elevation25°, looking
at the source coordinate origin; fixed vertical span1.12×the complete source
assembly's frozen bounding-box height in that camera frame. Record the resulting
world-to-camera matrix and crop explicitly, rather than refitting it every run.
Use a fixed key-light vector(-0.4,-0.6,1), ambient0.22, copper/neutral palette and
fixed component visibility. Resolve depth per pixel/fragment so the narrow nested
coil crossings do not acquire impossible painter-order connections.

Proposed caption hierarchy:

- **NCSX / reconstruction01** — main identity.
- **18 modular coils · 3 field periods** — verified assembly facts.
- **Illustrative swept coil sections / [actual inner-surface identity]** — geometry
  source/reconstruction scope, stated near the graphic.
- **[Actually executed operation and placement]** — e.g. P100 rational-patch
  evaluation only if actual qualified output produced these positions/normals.
  Avoid implying a complete GPU B-rep assembly, solid Boolean or native CAD import.
- A concise README goal line: **Target: sourced vessel, winding forms, ports and
  assembly topology.** Keep the current reconstruction and CAD goal distinct.

## Coordinate and geometry unknowns to preserve

The documentary views alone do not establish a complete right-handed Cartesian
frame, toroidal phase, exact coil ordering/mirror/current convention, winding
centerline, cross section, clearances or final vessel revision. Do not assert
six identical copies of each A/B/C trajectory until the selected numerical source
identifies its replication rule. Stellarator symmetry is not a Z reflection
alone: with a chosen symmetry-plane phase it reverses toroidal angle and Z;
that phase must come from the geometry source, not a convenient screen pose.
Treat proposed Z-up/CCW toroidal conventions as rendering conventions until tied
to source coordinates. Preserve original units and handedness, then record one
explicit source-to-render transform. This makes later iteration comparisons
about actual kernel/geometry progress instead of changed camera or source model.
