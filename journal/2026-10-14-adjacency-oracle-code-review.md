# Adjacency oracle review and caller timing wording

2026-10-14. Independently reviewed new oracle/tests for literal loop/UV source
truth, complete multiplicity, actual mate/neighbor semantics, permuted repeated
instances and nontrivial multi-level scan boundaries. Found one direct public
reader coverage gap: `edge_kind` was not asserted although raw kind bytes were.
Requested literal all-six-class reader assertions and invalid-slot checks;
saved the detailed review under `papers/`. No oracle/test edit or execution.

Applied root's caller metadata wording finding in the owned benchmark:
`profile=0` is now `event_profile=0`. Synchronous host phase measurements remain
enabled and separately named; the metadata only states that CUDA event profiling
is absent. This changes no product computation or timing scope.
