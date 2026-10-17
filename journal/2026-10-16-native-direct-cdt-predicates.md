# Native direct CDT metric predicates

Root accepted the concrete predicate design/plan and readonly integer limb view
before code. Authored the native fixture first. Its isolated fleet command
`clang++-18 -std=c++20 -fsyntax-only tests/native_direct_cdt_predicates.cpp`
failed on the genuinely absent `native/direct_cdt/predicates.hpp` capability.
No old live-watch checkout was modified.

Implemented immutable centered integer points, cached exact metric lifts,
positive power-of-two normalization, bit-built outward binary64 enclosures,
conservative scalar signs, outward local interval signs and exact fallback.
Added axis order and exact supporting-line distance comparison for the actual
constructor consumer. All owned vectors/integers use the caller's existing
memory/work budget. Added only a read-only `Integer::magnitude()` span to the
existing arithmetic header. No numeric library or CUDA/SDK dependency adopted.

Root independently reviewed normalization, subnormal endpoint construction,
sign reversal, significand carry, exponent bounds, scalar error derivations,
interval arithmetic and the cached determinant; ACCEPT after adding an explicit
`FLT_EVAL_METHOD==0` filter guard. Runtime changed rounding or FTZ/DAZ uses
the exact path. Fixtures cover construction and queries under those states,
and preserve/restore the fixture's floating environment.

The independent vector producer uses the existing generic original-rational
4x4 Fraction determinant oracle, not native weights/lift code, for expected
circle signs. Its 195 cases include unrelated rational denominators, metric
scales through 2^-500/2^500, exact cocircular zero and perturbations through
2^-2200, huge exact translations, signed permutations, and explicit inside/
outside/metric-reversal examples. Both filtered and forced-exact actual C++
queries match. Separate fixture checks cover exact axis/line-distance ties,
bad slots, collapsed support lines, nonpositive weights, memory exhaustion and
work exhaustion.

Actual isolated Linux Clang 18.1.3/libstdc++13 fleet results:

- Debug `-O0 -g`: PASS, 45,251,410 ns whole vector batch.
- Release `-O3 -DNDEBUG`: PASS, 5,236,039 ns whole vector batch.
- ASan/UBSan `-O1 -g`, no sanitizer recovery, leak detection enabled: PASS,
  36,811,588 ns whole vector batch.
- Every profile: 708 scalar signs, 6 interval signs, 846 exact signs and
  21,360 bytes maximum tracked native memory for one vector's contexts.
- Pinned clang-format check PASS. Focused clang-tidy with the owned-header
  filter PASS; its reported suppressed warnings were in non-user code.
- Full native warning profile PASS. No geometry execution occurred locally.

These are bounded fixture observations including binary input, construction,
filtered/forced-exact queries and reporting. They are not a robust performance
comparison, per-predicate latency claim, CDT qualification or watch speedup.
The next meaningful performance consumer is the complete constructor over the
predetermined independent geometry families; root requested no further predicate
campaign after these checks and numerical review.

The binary input is owned generated test data, not CAD source. Restore it with
`PYTHONPATH=python:tests python3 -B tests/native_direct_cdt_predicate_vectors.py context/native-predicate-vectors.bin`
after creating ignored `context/`. SHA256:
`f36a1529108e694198545476fd526fbbaca77a1d594087735f427243d7d5f305`.

Reviewed source SHA256 values:

| File | SHA256 |
| --- | --- |
| native/direct_cdt/predicates.hpp | eeab75c371d7f5bd832e8447387cef93f10ed51115631d228f4048ce0687ec3c |
| native/direct_cdt/predicates.cpp | e3544cf7f2c6c1329a2975a917668778e62247fe180af0b6a64e4ed934908a84 |
| tests/native_direct_cdt_predicates.cpp | cfd4012d4cee3f026cdb9a4470460696d3840acb39ed171648fbcd6e6acf41d8 |
| tests/native_direct_cdt_predicate_vectors.py | 6d3b1e1946a8a612b5d709cfd577bc34c1e8260feb99755f82cd2023399d5002 |
| native/face_domain/exact.hpp | 8a82c22993f2428e9828605cdb38fa19bc4b54ce04846b78a2745b43929e6175 |

In parallel, reviewed the constructor owner's signed secondary-Chew cap issue.
Confirmed a valid nonconvex DT deletion fan can produce a negative XY seed;
renormalizing it would corrupt directed adjacency. Supported a separately named
globally consistent infinitesimal lift tie policy by original-site rank while
preserving geometric zero in the raw predicate API. All-four-collinear projected
facets remain a real constructor bookkeeping obligation; a final-positive check
is necessary but alone does not prove all admitted collinear inputs complete.
No generic polygon fallback or unproved bound was introduced.
