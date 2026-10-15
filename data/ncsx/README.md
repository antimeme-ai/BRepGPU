# NCSX scientific geometry inputs

Unchanged data files from [SIMSOPT](https://github.com/hiddenSymmetries/simsopt)
commit `81436b4f7c655a4099eadadb168b7cc27923e555`. Upstream MIT license and
copyright are retained in LICENSE.simsopt. SIMSOPT is not a product dependency;
the small owned rendering caller reads these coefficient tables directly.

| File | Upstream path | SHA256 |
| --- | --- | --- |
| NCSX.dat | src/simsopt/configs/NCSX.dat | 2b857ca137184d0f4b5e7fa271c6cf946cf7fdfa127a19ec302d4f9731db57a7 |
| input.NCSX_c09r00_halfTeslaTF | tests/test_files/input.NCSX_c09r00_halfTeslaTF | 32b5609d02643d05b30c353ba5ab0a9b3ef6c486a8122f141a3c14e01748c956 |

NCSX.dat has three base modular-coil Fourier centerlines, modes0..30, six columns
per coil in sinX,cosX,sinY,cosY,sinZ,cosZ order. The source configuration uses
three field periods and stellarator symmetry to obtain18coils. The caller uses
all30nonconstant modes, rather than the upstream loader's default25.

The second file is a FOCUS-format **plasma boundary**, not a vacuum vessel or
VMEC namelist:136modes,nfp3, rows n,m,Rbc,Rbs,Zbc,Zbs, phase mθ−3nφ. No
paired-equilibrium, as-built revision or calibrated unit identity is asserted.
The files contain no embedded length-unit declaration; original coordinates
are retained and the scene has no dimension legend.

Coil-pack section dimensions, surface fitting, material and rendering are owned
illustrative choices. Actual winding forms, supports, ports and vessel are not
supplied here. See [source study](../../papers/2026-10-15-ncsx-source-geometry-study.md)
for original reader/symmetry URLs, observed scope and acquisition records.
