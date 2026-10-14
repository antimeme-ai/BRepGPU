# Adjacency plan: runtime adversarial review

Reviewed docs/ADJACENCY_PLAN.md against the revised design and earlier runtime
findings on 2026-10-14. ADR1–ADR3 are reflected in the design: E=0/C=0 distinction,
structural admission before diagnostic writes, and cleanup attempts with error
reporting. Accept the plan for implementation. No additional blocking finding.

The native operation remains one complete owner unit, including ABI admission,
CPU/GPU count/scan/fill, actual reader, resource cleanup and direct fault tests.
No split ownership of the prefix/fill invariant or extension of Engine is needed.
The red cylinder test precedes both native and wrapper implementation and runs
against a healthy existing fleet environment. Independent oracle work follows
the concrete interface, not a guessed binary layout.

The native subplan/header should fix the following implementation details before
the wrapper consumes them:

- Descriptor/info/timing exact sizes, payload element-count arguments, reserved
  field rules and supported backend/device/thread combinations.
- The structural admission order and diagnostic destination rule. Null/short or
  misaligned descriptors cannot be dereferenced; pointer-range wrapping cannot
  be accepted; rejected output/input aliases cannot receive early writes.
- All array allocations and scan scratch extents computed with checked integers;
  uint32 values remain exact because C is bounded. For 256-lane scan, 65,537
  edges produces 257, then two, then one block total and exercises recursion.
- Each padded block lane reaches all barriers; fill and reader run only after
  their predecessor on the same stream. The E+1 sentinel is initialized for
  every successful case, including E=0 and C=0/E>0.
- CPU partial worker launch failure joins launched workers. CUDA phase/resource
  failure attempts outstanding-work completion, frees, stream/event destruction
  and device restoration without replacing the primary diagnostic.

The proposed guarded tooling build is suitable for recoverable allocation and
phase faults; it must not contaminate the ordinary production binary or require
an environmental failure to simulate a production branch. Native preflight tests
must check destination bytes directly, while Python never publishes any result
after runtime failure. Tests should not assert unchanged runtime destinations
when the design explicitly permits partial writes after execution starts.

The measured campaign has 24 cases: three sizes, two storage orders and four
backends. Repeated calls retain prepared host input and initialized runtime only;
they still allocate, upload, build, read, download and clean up. Synchronizing
host phase clocks honestly measures an instrumented execution schedule and must
not be labeled kernel-only. Full native and caller wall remain the value claims.
Independent qualification occurs outside each timed interval, and actual reader
consumption occurs inside full caller wall. Source and binary snapshots, serial
fleet scheduling, separate topology build tree and named compatibility scope are
adequate; no additional certification layer is warranted.

The native unit can now proceed after root records accepted reviews and the
direct fleet red. No implementation or native execution occurred in this review.
