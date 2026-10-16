# Conforming packet implementation review

2026-10-16. Root reviewed the actual adaptive_packets.cpp/.hpp against the
accepted finite packet plan, shared selection types and independent reader.
Scoped source ACCEPT after the arithmetic-environment finding below. Runtime
and numerical-source guarantees remain their own units.

The constructor orders selected leaves, validates exact corner rectangles and
canonical common point identity, and builds one sorted supporting-line event
schedule. Every side consumes its exact endpoint subrange, including arbitrarily
unbalanced hanging sites. The opening/closing sweep rejects overlapping
same-direction side uses and emits only singly incident rim intervals. Sector
occupancy catches alternating diagonal packet contacts; monotone demotion
rebuilds the same schedules without dropping the returned band material.

Two chains on each fixed-diagonal half retain their shared final corner. Each
step consumes one nonfinal chain site, so triangle orientation follows the
rectangle invariant despite collinear side samples; no new center site or
distance predicate chooses topology validity. The common RN distance heuristic
only chooses a legal next step, with canonical PointSlot ties, including +inf.
Together the two halves emit k-2 triangles and preserve every scheduled site.

Exact word comparisons borrow retained table values. Equal denominators and
dyadic virtual shifts avoid repeated Rational import/GCD; the general fallback
cross-multiplies existing owned Integers. Scratch vectors stay in the passed
Budget; immutable result vectors retain their resource/selection leases. Failure
cannot publish a partial result. Empty selection is valid for this builder;
the complete runtime separately refuses an empty/incomplete requested patch.
The input selection is a coherent chart partition supplied by its producer;
this builder does not establish global source embedding or source error bounds.

Finding: the public entry checked FE_TONEAREST but omitted the common profile's
FLT_EVAL_METHOD and FTZ/DAZ requirements. The owner replaced that partial check
with admit_numeric_environment(). Ordinary preparation having already checked
its environment was insufficient for a separately callable packet operation.

The independent Fraction reader checks legal geometry rather than a prescribed
diagonal sequence, including actual side incidence/coverage and independently
derived pinch demotions. An all-demoted empty output cannot vacuously pass its
nonempty fixtures. The owner reports its first strict run green on ten cases /
174 triangles. Focused failure-path/sanitizer checks remain with the owner;
this review does not claim whole-patch or P100 integration is already green.
