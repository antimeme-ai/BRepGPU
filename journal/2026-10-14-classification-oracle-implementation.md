# Complete independent classifier truth and direct tests

Implemented `classification_oracles.py` as the accepted whole independent unit.
Point truth reads original vertex slabs and rim radii, applies exact original
world frame and collects all real incident face IDs only after feasibility.
Contact truth reads original rational affine controls, builds support/edge
halfplanes, solves rank-three rational systems and retains the complete feasible
corner set. Exact affine rank, normal relation, full-domain equality and
relative-interior midpoint determine labels. Exact original affine inverse maps
qualify bothUVs; raw exact vertex sets and positive-leftUV winding establish
complete ordering with cyclic area rotation allowed. No product geometry helper
supplies expected output.

Wrote eleven direct methods in `tests/test_classification.py`, preserving root's
separate entry red: literal material/all supporting faces; exact rational circle
and subnormal radial trap; all one-ULP slab sides; tiny-origin and extreme frames;
huge/exact scalar batches; all closed coplanar contact dimensions; tiny parallel
planes/opposed normals/full-domain equality; crossing versus finite-edge touch;
all rectangle orientations and source reversals; unsupported faces; malformed/
immutable/atomic failures. Root review added direct original query binding and
an input generator's sentinel failure. Added direct BodySnapshot contact adapter
source/UV correspondence to the existing transverse test. These attack request
and consumer contracts separately from result-only geometry truth.
AST parsing and diff checks passed; no local product execution occurred. Root
serializes actual fleet greens, adversarial code review and production mutants.
