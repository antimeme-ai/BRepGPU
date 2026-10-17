# Original canonical point cost

One bounded native client measured 1024 dyadic parameters per curve on neuroses,
CPU 2, with one reused GeometryContext per family. Source load and exact
preparation took 1.517 ms separately. The 60 second deadline did not censor a
family. Actual original-source points use the existing nearest-original
binary64 policy; output checksums and private exact-arithmetic work are in
[points.tsv](points.tsv).

| Original curve | 1024 points | Exact work | Budget peak |
| --- | ---: | ---: | ---: |
| Line | 6.773 ms | 193,155 | 552 B |
| Unit circle | 4307.041 ms | 216,541,769 | 1820 B |
| Positive rational | 33.560 ms | 1,007,524 | 2492 B |

The circle costs about 4.206 ms per canonical point. This is a material whole
mesher risk. The owned candidate is a fixed precision or double-double interval
filter that isolates the binary64 rounding cell, retaining exact Rational
fallback for ambiguous cases. Cached range-reduction constants and source data
belong to that same geometry implementation. Ordinary libsin results alone
cannot establish the current rounding policy. No numerical policy or point
implementation changed in this observation.

The client includes context construction, dyadic parameter construction and
actual point evaluation. Line and rational use typed GeometryStore dispatch;
the circle directly evaluates its original analytic row, without row lookup.
This is one exploratory shared-host observation, not robust timing statistics,
a complete mesher benchmark or a comparison with NVIDIA. The authored source
fixture and client are in tests/native_mesher_geometry_fixture.py and
benchmarks/native_mesher/points.cpp. The build used Clang 18, -O3,
-fno-fast-math and -ffp-contract=off.
