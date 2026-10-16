# Native predicate implementation plan

Status: root reviewed and accepted before code; steps 1–5 complete. Constructor
integration remains the subsequent owning lane's work.

1. Review `DIRECT_CDT_PREDICATES_DESIGN.md` interface/ownership, enclosure and
   scalar-error derivations with root and the constructor owner. Agree the
   small read-only existing Integer limb accessor and separate metric weights.
2. Author native direct predicate fixture with known independent exact answers;
   run it on the isolated fleet checkout before product source exists to record
   genuine missing-capability RED. No execution touches the old live watch.
3. Implement `native/direct_cdt/predicates.hpp/.cpp`, shared-budget immutable
   centered point/lift decode, exact query methods and counters. Implement
   exact scaled enclosures, conservative scalar filter, outward interval
   filter and runtime-safe exact fallback. Root owns CMake/product ABI files.
4. Run focused fixtures debug/release and actual ASan/UBSan builds on the
   fleet, with pinned compiler and source lint. A bounded Fraction-generated
   batch exercises signs, wide arithmetic, metric compensation and cancellation
   through actual compiled predicates; fixture expectations do not call the
   product's formula. Include non-default floating state and exhausted budgets.
5. Independent source/code review attacks enclosure endpoints, integer bit
   shifts, sign conventions, cache/allocator lifetime and filter soundness.
   Fix valid findings and rerun affected checks. Publish root's meaningful
   checkpoint without claiming full CDT qualification.
6. Constructor owner integrates the shared header and actual native predicate
   calls; full-domain and local-CDT semantic readers qualify its actual output.
   Predicate counters feed later complete-call performance reports. Do not
   launch a whole-watch rerun from predicate microbenchmarks.
