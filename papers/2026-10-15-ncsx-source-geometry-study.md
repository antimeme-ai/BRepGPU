# Actual NCSX geometry for the first goal render

2026-10-15. **Acquired authentic machine-readable NCSX modular-coil Fourier
centerlines and a named NCSX plasma Fourier boundary from the scientific authors'
SIMSOPT repository.** These support a recognizable first NCSX rendering without
inventing a generic toroidal coil assembly. They do not supply an as-built
vacuum vessel, winding-pack cross sections, supports, ports or a validated
BRepGPU CAD model. Search was bounded; no library installation, product code,
product execution or engineering reconstruction occurred.

## Original data retained

Pinned repository: `hiddenSymmetries/simsopt`, commit
`81436b4f7c655a4099eadadb168b7cc27923e555`, resolved from the actual Git tree.
Eight original raw files, 253,121 bytes, are retained unchanged in ignored
`quarantine/ncsx-simsopt-81436b4f7c65/`. The acquisition archive contains those
unchanged files, not a claim to contain the whole upstream repository. Its
manifest and restoration instructions are in
`quarantine/NCSX_GEOMETRY_MANIFEST.md`; public exact URLs, byte counts and SHA256
values are in `papers/ncsx-geometry-sources.tsv`.

The key file is
[`src/simsopt/configs/NCSX.dat`](https://github.com/hiddenSymmetries/simsopt/blob/81436b4f7c655a4099eadadb168b7cc27923e555/src/simsopt/configs/NCSX.dat),
11,893 bytes, SHA256
`2b857ca137184d0f4b5e7fa271c6cf946cf7fdfa127a19ec302d4f9731db57a7`.
Actual content is 31 numeric rows and 18 columns: three distinct base coils,
with modes 0 through 30. The official configuration loader identifies this as
the NCSX experiment's modular coil configuration, excluding circular coils.
Its default truncation is order 25; the data contains order 30, so a renderer
using all rows should explicitly say so rather than claiming default-loader
identity. [Pinned configuration definition](https://github.com/hiddenSymmetries/simsopt/blob/81436b4f7c655a4099eadadb168b7cc27923e555/src/simsopt/configs/zoo.py).

The second actual file is
[`tests/test_files/input.NCSX_c09r00_halfTeslaTF`](https://github.com/hiddenSymmetries/simsopt/blob/81436b4f7c655a4099eadadb168b7cc27923e555/tests/test_files/input.NCSX_c09r00_halfTeslaTF),
15,440 bytes, SHA256
`32b5609d02643d05b30c353ba5ab0a9b3ef6c486a8122f141a3c14e01748c956`.
Despite its name, it is a **FOCUS-format plasma boundary**, not a VMEC namelist
or vacuum-vessel model. Its actual header is `136 3 0`; 136 Fourier rows follow,
with toroidal index -6…6 and poloidal index 0…10. Every R-sine and Z-cosine
coefficient is zero, establishing the stored stellarator-symmetric convention.
The surface reader and Fourier definition were acquired to remove phase-sign
guesswork.

## Exact file and symmetry conventions

For each base coil, six consecutive columns are
`sin_x, cos_x, sin_y, cos_y, sin_z, cos_z`. Row m supplies Fourier mode m; row
zero's cosine column supplies each coordinate constant and its sine column is
unused. Thus, with theta=2*pi*t over one closed traversal,

`x(theta)=cos_x[0]+sum_m>=1(sin_x[m]*sin(m*theta)+cos_x[m]*cos(m*theta))`,

and similarly y,z. This differs from the interleaved coefficient-vector layout
used internally by `CurveXYZFourier`; read the file layout directly.
[Actual loader](https://github.com/hiddenSymmetries/simsopt/blob/81436b4f7c655a4099eadadb168b7cc27923e555/src/simsopt/geo/curvexyzfourier.py).

`zoo.py` sets nfp=3. Upstream loops field-period k=0,1,2, flip=False/True, then
the three base curves, producing **18 modular coils**. It transforms row-vector
points as `p @ Rz(2*pi*k/3).T`, then for flip=True multiplies by
`diag(1,-1,-1)`. Consequently stellarator reflection flips both y and z after
rotation; it is not just a y reflection. Swapping transform order can permute
the complete set, but changes the upstream indexing convention. Retain original
base/type, field period and flip as distinct instance identity.
[Actual symmetry generator](https://github.com/hiddenSymmetries/simsopt/blob/81436b4f7c655a4099eadadb168b7cc27923e555/src/simsopt/field/coil.py),
[actual rotation implementation](https://github.com/hiddenSymmetries/simsopt/blob/81436b4f7c655a4099eadadb168b7cc27923e555/src/simsopt/geo/curve.py).

The FOCUS surface rows are `n m Rbc Rbs Zbc Zbs`; its phase is
`m*theta - nfp*n*phi`, with nfp=3. Sum the cosine/sine coefficients separately
for R and Z, then convert standard cylindrical coordinates using
`X=R*cos(phi), Y=R*sin(phi), Z=Z`. Phi spans the full torus for a complete
display; multiplying nfp twice is incorrect. This is confirmed by the acquired
`SurfaceRZFourier.from_focus` reader and its class Fourier definition.
[Actual surface reader](https://github.com/hiddenSymmetries/simsopt/blob/81436b4f7c655a4099eadadb168b7cc27923e555/src/simsopt/geo/surfacerzfourier.py).

The raw coordinate files contain no embedded unit declaration. Their scale is
consistent with the roughly metre-scale NCSX scientific model, but this bounded
inspection did not establish a dataset-specific unit calibration or engineering
revision. Preserve coordinates without rescaling, avoid a calibrated dimension
legend, and do not infer winding-pack dimensions from the centerlines. Three
current magnitudes are present in the upstream loader, but are unnecessary for
the goal render and do not validate an electromagnetic configuration here.

## Honest first render and remaining gaps

Render the actual 18 expanded Fourier centerlines, optionally as narrow display
tubes or ribbons with clearly illustrative thickness. A translucent actual
FOCUS plasma boundary can give the distinctive three-period geometry and depth.
Call it **plasma boundary**, never vacuum vessel. Coil thickness, materials,
lighting, camera, exploded offsets or cutaways are presentation choices, not
recovered engineering geometry. Retain an unmodified non-exploded view and
expose source/order/symmetry attribution. This is a source-derived goal object,
not evidence of current kernel support for arbitrary stellarator CAD.

The official PPPL/NCSX archive search located engineering/physics PDF references
and coil/vessel discussion; the existing operator pack likewise contains PDFs,
previews and a photograph rather than machine-readable source geometry. No
authentic engineering vacuum-vessel mesh/CAD file or winding-pack solid was
verified within this bounded search. SIMSOPT's authored scientific configuration
is the useful actual dataset acquired, not a claim that PPPL released this exact
file as an as-built model. [Official PPPL physics source](https://ncsx.pppl.gov/pvr/Physics_Validation_Report/Chapter_2.pdf).

The acquired repository LICENSE is MIT, copyright 2023 SIMSOPT contributors,
and no separate notice was present in these two raw data files. Retain the
upstream license and attribution with redistributed coefficients. This records
the observed repository licensing, without asserting ownership of underlying
historical engineering geometry. Source study and a small owned Fourier reader
do not select SIMSOPT as a dependency. An exact whole upstream source archive
was not needed or acquired; the retained 62,300-byte acquisition archive keeps
all eight downloaded originals intact with individual hashes.
