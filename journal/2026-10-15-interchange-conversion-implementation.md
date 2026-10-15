# Source carrier converter implementation

After accepted design/plan and the written subplan, synchronized the eight direct
conversion tests only and ran them on the fleet. Actual red: eight tests in
0.012 seconds, each failing because `brepgpu.interchange` was absent. No product
implementation existed at that point.

Implemented immutable source records and factory-only Conversion, capped load,
same-byte hash/parse retention, complete schema-typed face/NURBS packing checks,
all-face/use inventory and shared numeric namespace allocation. Unsupported
analytic/multispan/envelope cases retain original source data and explicit
reasons. Exact Fraction input ratios form one-round homogeneous controls;
recorded bounds use actual emitted Cartesian control displacement and positive
weight derivative bounds. Added a separate literal mixed-type/multi-prim case
for type-prefix packing and document-wide face identity (now nine direct tests).

Local checks only parsed the source AST and checked whitespace. Product execution
remains on the fleet. Green awaits the independently owned strict parser; parent
and oracle owner notified that the converter is ready for adversarial review.

After the parser owner's direct green, synchronized only the converter, its own
test file and public fixture directory. Actual converter green on the fleet:
nine tests in 0.129 seconds, OK. Parent source review found no blocker. No native
library build or GPU operation was needed for this conversion-only unit.
