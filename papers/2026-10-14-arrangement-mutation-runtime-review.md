# Arrangement focused mutation runtime review

2026-10-14. Accepted source-only review of
`scripts/run_arrangement_mutations.py` and the eight entries in
`results/arrangements-2026-10-14/mutation-targets.tsv`. No mutations or product
execution ran on the workstation.

Each replacement changes an actual production geometry/topology site: omitted
cell holes, broken twin, broken ordered next, reversed original boundary
parameters, wrong original source UV side, incomplete closed cut incidence,
admitted tangent disks, or lost use orientation. Each named target exists in
the ten-method direct suite and reaches the changed operation with an original
domain/source assertion. The tangent admission target directly requires refusal
of closed disks that touch; the wrong-source-side target includes both operand
orders. The remaining graph faults meet the complete independent boundary-word
and domain model, rather than relying on successful product status.

The controller requires a unique literal site, copies Python/tests into isolated
fleet directories without bytecode caches, and points both import paths at that
copy. It runs the healthy source, one changed source and restored healthy source
against the same named test. Bytecode writing is disabled. Mutation credit
requires exit status exactly 1, a FAIL entry and AssertionError, no ERROR entry,
and a completed `FAILED (failures=N)` footer with positive N. An import error,
exception-only failure or crash receives no credit. Healthy baseline/retry
must return success. All five protected native binaries are hashed before and
after the complete campaign; the script does not rebuild or mutate them.

This accepts the execution mechanism and claim-to-target mapping. Actual saved
baseline/mutant/retry rows, process status and assertion diagnostics remain the
fleet campaign's execution evidence. Eight focused sites characterize these
named failure classes; they do not constitute exhaustive mutation coverage.
