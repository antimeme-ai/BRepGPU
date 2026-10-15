# Strict ASCII boundary implementation subplan

Own `python/brepgpu/usda_subset.py` plus `tests/test_usda_subset.py` as the complete
byte-to-typed-document unit. First write direct literal and rejection tests;
confirm an existing Patch prerequisite works and the absent parser fails on the
fleet. Then implement a bounded lexer and recursive typed parser with immutable
Attribute/Prim/Document results, named failures, exact integer and declared
float widths, identity wrappers, narrow metadata and full-input consumption.
Count bytes/tokens/components/prims/depth before exceeding budgets. Reject unknown
syntax/composition, duplicates and malformed literals without publishing a partial
document. Preserve all accepted typed attributes rather than selecting geometry.

Sync only owned files to the existing fleet checkout. Execute direct tests there,
review/fix findings and report results to root; no local product execution, native
builds, dependency changes or Git operations. Actual reference-array semantics and
carrier conversion belong to the separate converter owner. Source inspection/AST
checks locally remain allowed.
