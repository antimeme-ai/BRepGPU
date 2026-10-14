# Independent body caller artifact review

Reviewed the three retained outputs under
`results/body-2026-10-14/caller/` with standalone standard-library CSV, Fraction,
struct and XML analysis. No product module was imported/executed, and no native
or fleet job was run. No artifact defect found.

| Artifact | Grid | Candidates | Evaluated | Rejected | Exact boundary |
|---|---:|---:|---:|---:|---:|
| cpu-box | 17 | 1,734 | 1,734 | 0 | 384 |
| cuda-1-cylinder | 17 | 1,734 | 1,550 | 184 | 174 |
| cuda-0-plate | 33 | 10,890 | 10,712 | 178 | 1,094 |

Total: **14,358 candidates, 13,996 evaluated, 362 rejected, zero invalid**.
Boundary counts include repeated face/chart representations, not unique spatial
points. All exact submitted-UV labels, accepted positions/normals and exported
source identities passed independent checks, with zero violations.

For every row I read the exact `u_hex`/`v_hex` binary64 values as Fractions;
checked u-major/v-fast grid coordinates and chart order against input resolution;
computed disk/hole membership and true wall seam/height boundaries exactly;
and evaluated the declared analytic plane or rational-quarter-cylinder geometry.
Rejected rows had native status -1 and six NaNs; admitted rows had status 0.
Reported qualification counts and maxima matched the independent calculation:

- CPU box maximum position/normal component error: **0 / 0**.
- Each GPU artifact maximum position/normal component error:
  **1.3848935858110062e-17 / 1.322255008107945e-16**.

These satisfy the finite-corpus targets 2^-36 / 2^-30. They establish these
retained samples, rather than a universal floating-point error bound.

Every row's candidate, patch slot, face/chart IDs, entity kind 5, body ID and
revision matched the input/patch metadata. Body IDs are process-local; each
separate caller run has ID 1, without implying a shared cross-process body.
Frames in these three artifacts are identity. Patch descriptors and exact
homogeneous control TSVs match independently derived canonical carrier nets;
packed controls agree with their TSV values. All table byte counts and SHA256s
match, including total body-table sizes 4,624 / 2,480 / 6,536 bytes for
box/cylinder/plate. Decoded headers, chart ordering, typed record IDs, references,
adjacency ranges and two opposed uses per edge are consistent. Hole-sensitive
Euler counts are 2 / 2 / 0. Actual upload/download payload metadata correctly
uses 20 / 76 bytes per evaluated CUDA row; body-table GPU upload is zero.

The PLY files contain exactly all evaluated rows with the same XYZ, normals and
face/chart IDs; there is no face element. The SVGs display actual row projections:
1,734 box points, 1,550 cylinder points and 3,571 plate points (stride-three
thinning). They also contain respectively 12 / 3 / 15 gray CPU-owned spatial
boundary curves and 87 / 87 / 90 normal arrows. The common SVG group transform
only changes display placement/scale. Captions and recipes correctly state
point output, repeated chart samples and no mesh/chord guarantee. Gray curves
are reference geometry drawn on CPU, not evidence of a GPU edge operation.

`timings.tsv` names cold native setup/cleanup, internal reused-operation phases,
independent qualification and export-inclusive wall separately. These are three
single caller demonstrations with different bodies and grid sizes, not a matched
CPU/GPU speedup experiment. Normal/scatter host phases take about 29.9 / 27.3 /
260.8 ms, compared with evaluation phases 1.85 / 0.652 / 0.649 ms, making host
row/normal consumption a concrete next measurement target. Those observations
do not claim pure device timing or a new performance conclusion from this review.
The repeated matched body matrix is separate evidence. No imported part,
Boolean operation, watertight mesh or general trimmed-B-rep claim follows.
