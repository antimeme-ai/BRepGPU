# Actual preserved-binary compatibility

All eight fresh-process combinations passed on Linux x86-64 / CPython 3.12.3: two CPU cases and six CUDA cases across P100 device ordinals 0 and 1. [Raw outcomes and artifact identities](compatibility.tsv) accompany the [reusable checker](../../scripts/check_projection_compatibility.py).

| Actual combination | Backends checked | Evaluator behavior | Projection behavior |
| --- | --- | --- | --- |
| Current Python adapter + preserved iteration-one core/plugin | CPU, CUDA 0, CUDA 1 | Exact/analytic jets pass before and after refusal | `BG_UNSUPPORTED=2`, flags zero; repeated refusal leaves evaluation available |
| Current core/Python + preserved iteration-one CUDA plugin | CUDA 0, CUDA 1 | Exact/analytic jets pass before and after refusal | `BG_UNSUPPORTED=2`, flags zero; repeated refusal leaves evaluation available |
| Committed old ABI-1 Python adapter + current core/plugin | CPU, CUDA 0, CUDA 1 | Exact/analytic jets pass through actual old caller declarations | Not attempted: the old adapter has no projection API |

The preserved core and CUDA plugin were the actual earlier production binaries, not recompiled substitutes. Their SHA-256 identities were checked before and after the run and remained unchanged. The old adapter was extracted with `git show` from `1bc2d22f0cd3f977993906ad105e24aadc4bb56a` into ignored context. The current adapter and production build came from the reviewed source checkpoint `af91141a9a8a7bac626559192ee2b5263c077be9`, which includes numerical implementation commit `2533fac`; the integrator supplied the completed build and matching current adapter before execution. Exact adapter/core/plugin file identities are retained per row.

Each process evaluated an affine plane and the exact rational quarter-cylinder on 56 shuffled patch/UV requests for orders 0, 1 and 2, including endpoints, interior non-dyadic parameters and inward `nextafter` values. The actual supplied binary64 controls/parameters define the independent Fraction oracle; analytic polynomial/rational maps give the same exact jets. No CPU/GPU parity check supplies the geometry truth.

Across all cases, 2,184 evaluated request statuses were `BG_QUERY_OK=0`, with 21,840 component checks. Metadata reported the requested CPU/CUDA backend in every result. Five combinations requested projection twice each; all ten attempts returned nonfatal unsupported status, followed by another independently checked evaluator run in the same context. Maximum cumulative absolute errors across the compatibility matrix were:

| Grade | Maximum absolute error |
| --- | ---: |
| E0, position | `1.1102230246251564e-16` |
| E1, position and first derivatives | `1.6094907629922961e-16` |
| E2, all six vectors | `3.3571083358093477e-16` |

These results establish the specified actual-binary/caller combinations and this direct compatibility corpus. Broader geometry, fault, memory and performance qualification remain their separate reports. The compatibility checker records no performance timing and starts no benchmark jobs.

To rerun with preserved artifacts available in the checker's documented defaults:

```sh
python3 scripts/check_projection_compatibility.py --current-revision af91141a9a8a7bac626559192ee2b5263c077be9
```

The utility only reads preserved binaries, creates a checked old-adapter extraction and worker logs under ignored context, runs one isolated process per combination, and writes sanitized TSV outcomes. It does not overwrite the baseline or change repository Git state.
