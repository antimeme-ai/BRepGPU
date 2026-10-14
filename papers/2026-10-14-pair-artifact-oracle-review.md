# Independent retained pair caller artifact review

Reviewed `results/pairs-2026-10-14/caller/{cpu-24,p100-0,p100-1}` with standalone
standard-library Fraction, struct, CSV and XML analysis. No product module was
imported/executed; no native or fleet job. **Zero violations found.**

| Artifact | Bodies per side | Chart dimensions | Examined pairs | AABB survivors | Excluded | Mask bytes |
|---|---:|---:|---:|---:|---:|---:|
| cpu-24 | 4 | 28 × 28 | 784 | 43 | 741 | 100 |
| p100-0 | 4 | 28 × 28 | 784 | 43 | 741 | 100 |
| p100-1 | 16 | 116 × 116 | 13,456 | 228 | 13,228 | 1,684 |

Aggregate: **15,024 examined pairs, 314 survivors, 14,710 exclusions**. These
are carrier-box survivors, not counts of surface intersections or contacts.

## Original source and whole-carrier enclosure

I reconstructed expected body order, shape cycle, dyadic scene origins/scales
and complete chart lists from the declared deterministic caller recipe, without
calling its factory. Every `sources.tsv` row has the expected side/dense index,
body ID, revision 1, typed face/chart IDs and original patch slot. Every body is
complete, including all four wall quarters under one face; no omitted face can
be hidden by declaring a smaller full-domain result. IDs are process-local typed
namespaces, not pointers or cross-process persistent identities.

The small scenes contain 22/19 left/right faces and 128/136 original control
records. The larger scene contains 86/83 faces and 544/552 control records.
Every patch's degree, control count, source chart rectangle and exact homogeneous
control bits agree with the independently derived canonical box/cylinder/plate
maps: planes degree (1,1), quarters degree (2,1), positive weights, and dyadic
weighted controls yielding denominator 1+s² and quarter-circle numerators.
No duplicated/missing control slot or extra patch/control record was found.

For every original control I computed Cartesian A/W as a Fraction. Every local
box contains the exact control extrema. Every world box contains both exact
origin+scale transforms of those extrema and exact transforms of the actual
retained local endpoints. Positivity of the rational Bernstein weights extends
this enclosure to the entire original carrier, rather than just sampled UVs.
Different body frames are explicitly applied; no local-coordinate overlap is
substituted for common-world overlap.

This artifact analysis verifies the retained bounds and mask against the exported
inputs. The caller separately downloaded resident indexes and checked exact
agreement with these scene boxes during qualification; this review does not
claim a new device-memory observation from local file analysis.

## Complete bit/page and shortlist truth

Each retained page starts at zero, examines its complete Cartesian domain,
ends at total_pairs, reports correct dimensions/word count, and has flags 3
(REACHED_END plus FULL_DOMAIN). Backend metadata matches the requested CPU/CUDA
operation. Event profiling is off and kernel_ms is NaN. There is no unexamined
prefix/suffix represented as an exclusion.

I decoded every little-endian u32 word and recomputed every bit from exact
closed comparisons of the retained world endpoints. Equality remains overlap.
All bits agree. Every final word has **16 zero unused high bits**; padding is
neither examined work nor excluded geometry. Every `survivors.tsv` row appears
once in increasing flat=i*right_count+j order and matches its left/right dense
indices and complete body/face/chart source keys. The retained quality counts
and byte payloads agree: GPU index uploads 2,688/11,136 bytes and mask downloads
100/1,684 bytes, with zero topology upload.

The small SVGs show all 43 candidate cells; the large SVG correctly shows 190
of the 228 candidates within its explicitly labeled first-100-by-100 window.
SVG titles map to actual set bits, rather than invented intersection curves.
These caller outputs do not establish trim overlap, collision/contact, SSI,
watertight meshes or changed Boolean behavior. They are single demonstrations;
matched repeated performance comparisons belong to the separate benchmark.
