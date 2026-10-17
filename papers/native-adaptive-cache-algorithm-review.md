# Adaptive cache coordinator: scoped algorithm source review

Read actual `adaptive_cache.cpp/.hpp` and `adaptive_types.hpp`, including the
current normalized-point, physical-area filter and scalar-ref interning paths.
Scoped source ACCEPT. No additional execution was requested: the owning runtime
lane runs its direct CPU/GPU, concurrency and sanitizer checks.

Root-relative dyadics remain reduced, checked through axis-depth 64; immutable
node paths preserve split axis/child order. Endpoint keys normalize signed zero
and omit span-local identity only at exact endpoints. Interior sample identity
retains original source and prepared span/interval coordinates. Cross-span
shared points select the original root owner rather than a rounded leaf winner.
Canonical final ordering follows semantic keys/path, not allocation or worker
completion. ExactTable and source leases outlive frozen packets.

CPU jobs write disjoint pre-sized result/destination ranges and finish complete
horizons before coordinator changes state. Original base bound tiers stay keyed
by demanded point-stage signature; query-local precise bounds do not overwrite
them. A new query resets demanded point stages, while wider cached results are
reused only on demand. Sorted complete point promotions invalidate affected
accepted nodes before rebuilding the frontier. No initial four-corner result is
published without bounds on the actual zipper triangles.

Current actual-facet preparation normalizes same-root dyadics directly, retaining
exact rational fallback for cross-span metadata. Interval cross-products prove
physical noncollapse in the clear case; exact actual-XYZ cross-products resolve
ambiguity. Scalar refs intern by source/axis/semantic coordinate, separate from
canonical sorted point identities. Actual-facet failure splits the owning cache
leaf and regenerates connectivity; it does not repeatedly certify the same
unchanged facet.

One backend-admission seam was sent to the CUDA owner: a positive exact Rational
allowance can have downward binary64 bound zero. CPU already keeps its positive
exact value. The CUDA entry must accept finite downward zero rather than impose
an unselected public allowance floor; native precise comparison settles the
remaining threshold decisions. Runtime owner agreed, and root owns that device
guard fix. This review makes no trim/boundary-band or whole-watch claim.

Final narrow reread also ACCEPT: both node-failure and actual-facet-failure paths
now collect a complete unique split horizon, deduplicate exact missing PointKeys,
and preflight node/control/point/frontier capacities before modifying the selected
cover. The same all-affine inactive-axis refusal applies there. Typed KernelJob
Range slices, explicit query-optional guard and typed internal external-lease
release with a C ABI bridge preserve the prior ownership semantics. No additional
execution was introduced by this reread.
