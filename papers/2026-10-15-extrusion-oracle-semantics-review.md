# Extrusion independent oracle and test review

2026-10-15. **ACCEPTED after O1 and O2 corrections.** Source-only review of the
whole extrusion_oracles.py and eleven-method test_extrusion.py unit against the
design, actual CPU construction and native sampling/ABI5 contracts. Root and the
independent owner run qualification on the fleet; this review runs no product.

O1: the falsified-origin test initially used dataclasses.replace with a bool
orientation. The correct public Origin constructor rejected that value before
the independent qualifier could see it, aborting the test rather than testing
oracle sensitivity. Owner changed that one injection to the existing altered()
mechanism, preserving the valid constructor guard. Verified the corrected source.

O2: minimum-edge/minimum-height geometry was initially qualified without actually
sampling its native normals. Added actual sampled 1/256-edge triangle, height1/4,
at Frame scales1/4 and4 with large translated origin to the native-normal method.
Verified both calls are present; their executed outcome belongs to the owner's
serialized fleet greens. This covers minimum nonzero cross magnitude and a
nonsquare diagonal normal as well as axis and larger-profile cases.

Expected geometry derives from original lattice vertices, actual retained Frame
and height, without production construction/trim/validation/history helpers.
Literal complete role enumeration checks source and output vertices/curves,
intrinsic UV records, every carrier control/frame/UV bit and typed Lineage.
It checks the entire prescribed face domain and boundary word; paired incidence,
connected shell, cyclic vertex links and Euler supplement full coverage rather
than stand in for it. Along each expected side boundary one raw coordinate is
constant, so the asserted endpoint equality establishes the complete affine
carrier-composition identity, rather than sampling an unbounded curve model.

The history model independently enumerates all exact target/source roles, the
lower reversed coedge mapping, both cap translations/parities, all8N generated
outputs, affine source/axial maps and exact reciprocal multiplicities. Original
source namespace/type/grid validity and actual history lookup identity are read
directly. Typed local Fraction geometry, all dense integer slots/IDs, bool-vs-int
aliases and native binary64 bit correspondence are checked explicitly.

Public exact readers are independently queried at whole support strata,
non-dyadic edge/UV parameters, cap offsets down to2^-1074, nextafter values and
every supporting vertex/edge/face. Native samples bind copied query bits, order,
gather labels/counts, source identity and rejected NaN rows; world position comes
from the original source formula. Oriented normals use rational intervals from
integer-square-root bounds, including a perfect-square branch; requiring the
component error bound at both interval endpoints safely bounds error against the
true unit normal. Neither production derivative normalization nor CPU/GPU parity
supplies expected normal truth.

Actual native CSR is independently qualified from binary arrays and checked
against the solid incidence plus typed edge-use/mate/neighbor readers under full
slot permutations. Tests additionally exercise verified32-vertex input, cyclic
source order, geometry-equal different namespaces, hostile positive-turn winding
stars, bounded/failing iterators and late publication failure/retry. Falsified
controls/domains/ownership/history and actual output samples address named faults;
they are not a community-suite or alpha-readiness claim. No further blocker found.
