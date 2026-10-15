# Interchange plan: runtime and strict-boundary review

Reviewed the complete `docs/INTERCHANGE_PLAN.md` against the accepted design,
actual exported samples and live reference-query feasibility. **Accepted; no
blocker.** Add `tests/test_usda_subset.py` explicitly to strict-boundary ownership
beside `python/brepgpu/usda_subset.py`; the owner returns the complete lexer,
typed document and direct literal/rejection tests as one conceptual unit.

The fixed 16 MiB, one-million-token, 500,000-component, 4,096-prim and depth-64
limits bound parsing and allocation; load caps acquisition at MAX_BYTES+1. Hash,
parse and retained export operate on one immutable byte snapshot. Complete
consumption, declaration/identity duplicate rejection, metadata whitelist and
explicit composition rejection match the actual identity-wrapper/BrepArray files.
Parsed float-width semantics matter: float/float vectors round to binary32 and
reject overflow/nonfinite results, double controls remain binary64, and integer
types reject noninteger syntax and out-of-range values. Tests should assert
literal typed values, not reconstruct expected values with production helpers.

SourceError/UnsupportedSource/SourceBudgetError separate invalid input, unsupported
scene syntax and resource rejection; no partial Document is published. Semantic
face rejection remains conversion inventory, distinct from parser rejection.
Native resources remain existing Engine children; complete walls include cleanup,
required statuses and reader work, with reference acquisition, exports and exact
independent qualification separately reported. Three measured fresh-process
repetitions are exploratory, without warm-context or whole-CAD gain claims.

Independent foreign parsing of the unchanged mouse bytes and actual generated
sheet attacks parser/control correspondence. Exact source-basis and emitted-
control oracles separately attack conversion and native arithmetic; their query,
descriptor and lineage binding is explicit. No SDK/dependency/native ABI work is
needed. Actual red and reviewed-plan approval still precede implementation. This
review performed source inspection only.
