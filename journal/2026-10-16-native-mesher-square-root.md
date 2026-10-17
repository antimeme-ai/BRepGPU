# Exact dyadic square-root bounds for actual analytic consumers

Geometry's jet/chord procedures need a scalar norm upper beyond binary64 range
without L1 pessimism or a second bigint. This is an actual finite consumer need
after residual-task implementation, not a speculative arithmetic expansion.
Agreed API: Rational::square_root_bounds(fractional_bits), returning SqrtBounds
with same-owner exact dyadic lower/upper. The grid is absolute2^-fractional_bits;
negative input fails and all precision/work/storage is charged to its Budget.

Authored132 independent Python integer-isqrt/Fraction vectors, including huge/
tiny4096-bit scales,8192-bit near-squares, exact dyadic/nondyadic roots and
precision0..2048. The native fixture checks exact oracle bounds, actual squared
enclosure, owner identity, negative refusal and extreme-precision exhaustion.

Genuine isolated fleet RED before implementation: producer emitted50,372 bytes;
the actual native build failed exit1 for the three missing square_root_bounds
calls. No exact implementation/header edits preceded this RED. Raw output stays
ignored in the square-root unit context.

Selected implementation: floor(n*2^(2b)/d), then owned integer Newton divisions
from a power-of-two upper estimate. Monotone iteration stops at floor sqrt;
exact-square/remainder identity determines an equal versus next-grid upper
endpoint. No floating locator, copied bigint engine, or extra dependency.

Actual implementation GREEN: all132 independent vectors,2,868,727 charged work,
maximum257 limbs and9,260B peak. Strict Linux Clang18 C++20 Release and
ASan+UBSan/leak builds both pass. The original277 Rational+173 Integer/IEEE
vectors also pass both profiles unchanged. Actual-source clang-tidy and named
100-column format checks pass. Builds use isolated sqrt-unit directories and
the existing native_cpu_rigor.cmake profile; no legacy Integer changes or new
dependency. Root owns permanent CMake integration and independent code review.

Producer SHA256:
17509d4dfef3881db23d295d8d2f479823e5979121fe4623673910cd0c2d18d0.
Generated payload SHA256:
96e0c5a385d08865d2644a89f041edbf55faf3c7f0a5bf574135192941bf799a.
Raw RED/GREEN/build/tidy logs remain ignored under context/sqrt-unit on fleet.
