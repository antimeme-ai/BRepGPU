# NCSX study00: source-based P100 surface rendering

![NCSX goal object](../../assets/ncsx/goal-object.png)

A first rendering of all eighteen NCSX modular-coil envelopes and a separately
sourced NCSX plasma boundary. Existing native C++/CUDA patch evaluation supplies
the actual displayed positions and derivative normals; an owned CPU depth-buffer
renderer shades the sampled triangles. This establishes a repeatable visual goal
object, not a complete engineering B-rep or a new modeling operation.

## Actual run

Linux x86_64, Python 3.12.3, Tesla P100-PCIE-16GB device 0, driver 580.178.04.
The existing pair-release core and CUDA plugin were consumed unchanged;
[native-libraries.tsv](study-00/native-libraries.tsv) records both actual hashes.
The native result backend tag was 1 (CUDA), matching the requested placement.
No timing or speedup was measured for this visualization.

| Component | Fitted patches | Native first-jet samples |
| --- | ---: | ---: |
| 18 coil envelopes, 64 segments × 4 sides each, degree (3,1) | 4,608 | 69,120 |
| Plasma boundary, periodic 36×24 grid, degree (3,3) | 864 | 21,600 |
| Total | 5,472 | 90,720 |

All native statuses were BG_OK and every component was finite. Every returned
position and both first partials were compared against an independent exact
Bernstein polynomial evaluation at the actual submitted binary64 controls and
parameters: 816,480 scalar components. Errors and limits below are maximum
absolute scalar errors in the normalized local control frame; source/world
positions restore the common scale of 4 once.

| Native quantity | Observed maximum | Limit |
| --- | ---: | ---: |
| Position | 1.5092094240998222e-16 | 2^-40 |
| First partial u | 2.6188271411183974e-17 | 2^-30 |
| First partial v | 5.360295540768334e-16 | 2^-30 |
| All neighboring patch boundary sample positions, including periodic wraps | 0 | 2^-39 |

Sharp rectangular-section side normals are intentional. The seam check concerns
sampled positions, not smoothness or a manifold-solid topology claim.
[qualification.tsv](study-00/qualification.tsv) retains the direct checks.

## Source fidelity and engineering scope

Unchanged scientific inputs come from SIMSOPT commit
`81436b4f7c655a4099eadadb168b7cc27923e555`; the original MIT attribution/license
is retained with the [data](../../data/ncsx/README.md). No SIMSOPT dependency was
adopted. All 30 nonconstant Fourier modes of three coil centerlines are used.
The source's three-period and stellarator-symmetry transforms produce 18 coils;
family numbers do not assert a mapping to the engineering A/B/C coil labels.

The centerlines receive illustrative rectangular packs, half-width 0.04 and
half-depth 0.06 in original coordinate units, with projected toroidal section
frames and periodic cubic fitted rails. The separate 136-mode FOCUS dataset
supplies the fitted plasma boundary. It does not supply the vacuum vessel, and
the two datasets are not asserted to be a validated paired equilibrium or one
as-built revision. The raw files do not declare length units; no calibrated
meter scale is inferred.

Held-out samples compare the fitted output against the source Fourier functions:

| Sampled fitting diagnostic | Maximum Euclidean error, original coordinate units | Samples |
| --- | ---: | ---: |
| Coil centerline, recovered from opposed fitted pack faces | 0.003887301573422884 | 3,456 |
| Plasma boundary | 0.006217897392490993 | 7,776 |

These are sampled diagnostics, not global approximation or triangle chord bounds.
The display splits native sample quads into 101,376 triangles; this is not the
canonical trim-aware mesher, general triangulation or a watertight solid export.
Actual vessel, winding forms, ports and supports remain engineering-source and
modeling work in the [goal-object roadmap](../../docs/NCSX_GOAL_OBJECT.md).
External community qualification for alpha remains incomplete.

## Saved scene and reproduction

[recipe.tsv](study-00/recipe.tsv) records fitting, transforms, native layout and
placement. [patches.tsv](study-00/patches.tsv) binds every component/period/flip,
patch and native query range. [controls.tsv](study-00/controls.tsv) preserves
all 50,688 homogeneous control points as hexadecimal binary64 values.
[jets.f64](study-00/jets.f64) is the actual little-endian component-major native
Sxyz, Suxyz, Svxyz output in the normalized frame.

[scene.tsv](study-00/scene.tsv) defines the fixed orthographic camera (35° azimuth,
25° elevation, Z-up), 1800×1200 output, scale 240 pixels per source coordinate
unit, center (900,650), two-sided native-normal lighting, material palette and
triangle/raster rules. Final projected sample bounds are approximately
x=389.19..1501.01, y=240.24..1049.33; the complete object clears the captions.

The native run used source commit `92117fb`.
[sources.tsv](study-00/sources.tsv) preserves that original caller/display,
Python binding, data and license snapshot. First image inspection found footer
crowding at 270 pixels per coordinate unit. Before establishing study00, the
saved qualified jets were redrawn on the fleet at 240 with unchanged geometry,
camera angles, lighting and materials. [render-sources.tsv](study-00/render-sources.tsv)
pins the final display source separately; it deliberately differs from the
initial display hash in sources.tsv. No native evaluation was repeated for this
framing correction. Subsequent comparison images must name scene/data changes.

[Owned caller](../../examples/render_ncsx.py) and
[owned display](../../examples/ncsx_display.py) reproduce the final scene using
the command in the [goal-object guide](../../docs/NCSX_GOAL_OBJECT.md#reproduce-the-first-source-based-scene).
The actual [labeled SVG](study-00/ncsx.svg) embeds the
[transparent native geometry raster](study-00/surface.png). The shareable
[PNG](../../assets/ncsx/goal-object.png) was rasterized from that SVG with the
existing standalone rsvg-convert 2.62.1, without a new runtime dependency.

Source/design reviews precede this run; the final
[artifact review](../../papers/2026-10-15-ncsx-artifact-review.md) checks the saved
records and published scope. Full-image inspection confirmed coil/plasma
separation, depth crossings, complete framing and readable provenance captions.


## Preserved baseline and later rerun

A [post-breadth matched CPU/P100 rerun](../ncsx-rerun-2026-10-15/REPORT.md)
reproduces all nine original P100 artifacts byte-for-byte and adds first current
phase timings. Original study00 remains unchanged and has no historical timing
baseline; the later report separates actual scene equivalence from timing scope.
