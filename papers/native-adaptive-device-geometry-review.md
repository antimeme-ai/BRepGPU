# Adaptive device numerical cooperation review

Scoped actual-source review of `native/mesher/cuda/adaptive_cuda.cu` against
`adaptive_numeric.hpp`, `adaptive_geometry_types.h` and the accepted adaptive
geometry unit. Reviewed after the first strict sm_60 build; source findings were
sent to the module owner before final device reader runs.

The cooperative rational node kernel retains one complete active warp per task.
It stages at most 64 homogeneous coefficient intervals, forms the same actual
corner predicate and source anchor as the CPU routine, and assigns the at-most
25 residual coefficients to lanes. Each lane uses the common coefficient
routine and interval norm. Inactive lanes contribute zero; warp MAX cannot
change finite/nonnegative coefficient ordering and the common norm returns
infinity rather than NaN on uncertainty. Lane zero passes the maximum to the
same common bound fold, bypassing only its serial coefficient loop. This
preserves source-to-actual-bilinear bound meaning and operation order within
each coefficient. Restriction similarly stages the parent before writing child
coefficients. Analytic point/node and actual-facet kernels call the same common
finite formula bodies, with no second geometry evaluator.

Three concrete admission/action findings were resolved by the owner: validate
the family and exact rational net shape `(p+1)(q+1)4` before a short net can reach
the sample kernel; require every node box inside its prepared root; and classify
a failed bound with both axes affine-separable as numerical uncertainty instead
of selecting a geometric split with no active axis. The latter preserves the
promotion/failure path when point or interval uncertainty dominates.

Scoped numerical cooperation ACCEPT after these resolutions. The same original
source reader subsequently passes against actual outputs on both P100s. This
review does not independently approve CUDA lifetime/fault ownership; those are
owned by the module and runtime reviews. It makes no whole-mesher performance
or global embedding claim.

Subsequent actual-facet transfer closure preserves both common CPU/P100 formula
bodies: the minimum of the six-coefficient upper and quarter-twist plus maximum
nodal residual is valid for any contained chart triangle. In the exact path,
A=||D||²/16, B=max nodal squared residual, r=tau-e are nonnegative; the alternative
accepts when s=r²-A-B>=0 and s²>=4AB. This is equivalent to
sqrt(A)+sqrt(B)<=r and includes exact equality. The numerical owner independently
reviewed the actual RN and precise bodies and returned scoped ACCEPT. No source,
point or actual-L substitution accompanies the tighter bound.
