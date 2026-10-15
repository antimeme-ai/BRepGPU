# Strict ASCII boundary red, implementation and green

Wrote direct literal/rejection tests before production source. Fleet entry test
passed the existing Patch prerequisite, then failed at absent usda_subset import:
one test, 0.018 s, exit 1. Implemented the complete owned lexer/typed parser in
`python/brepgpu/usda_subset.py`, with no foreign imports or native changes.

Public Attribute.type_name retains the base declared type without [] suffix;
all admitted attributes are immutable tuple arrays. Prim records are flat preorder
with canonical absolute source paths; Document and all emitted metadata/values
are immutable. Enforced byte/token/component/prim/depth budgets, whitelist metadata,
literal widths, duplicates, identity wrappers, full consumption and explicit
unsupported scene rejection. Direct decimal→binary32 midpoint correction avoids
double rounding; literal tests exercise both sides of an exact midpoint.

Only owned module/tests and tiny ignored samples were copied to the fleet. Final
direct green: ten tests, 0.081 s. Separate fleet acquisition-path check parsed the
actual bicubic export (2 prims, 47 attributes), actual mixed analytic export
(2 prims, 42 attributes), and unchanged upstream mouse fixture (2 prims,
42 attributes), retaining all actual surface tokens. This checks syntax acceptance,
not conversion, trims or geometry. Local checks were AST-only; no local product
execution, builds, installs, native library changes or Git operations. Sent root
the exact public interface and reference construction/interpreter information for
coordinated integration and review.
