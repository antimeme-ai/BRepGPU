# Reference source export subplan

2026-10-15, design/research only. Root requires the NVIDIA CPU baseline on the
complete supplied watch. Geometry lane owns faithful fresh source export;
independent lane owns the official bridge/tessellation, and native lane owns
the common raw-mesh display. V3 is running and all its pinned files/binaries
remain unchanged.

Read the existing source adapter and assembly reader, pinned OCCT BinTools and
ASCII serialization implementations, and the pinned official NVIDIA bridge
parser/source interfaces. The bridge has no direct STEP input. Selected binary
BinTools V4 with triangles/normals disabled: official reader supports V4, while
OCCT's ASCII topology/location writer forces 15 digits despite 17-digit curve
and surface tables. Binary preserves the translated double payloads. Oracle
owner confirmed this format choice; native owner confirmed stable definition/
occurrence IDs, actual source matrices, shared authored explosion offsets and
opaque NVIDIA face ordinals when exact source-face correspondence is absent.

Written design:
`papers/2026-10-15-om10-reference-source-export-study.md`.

Proposed files after parent acceptance and meaningful red:

- `native/reference_source/main.cpp` and `CMakeLists.txt`: standalone
  reference-only fresh read/transfer/graph/export path, using the same pinned
  public OCCT settings. Existing product source/reader/binary pins stay unchanged.
- `python/brepgpu/reference_source.py`: strict immutable binary manifest reader;
  reuse existing typed product/usage/style and assembly machinery.
- `tests/test_reference_source.py`: authored source geometry/identity/placement,
  actual V4/no-mesh output, selected direct rejection paths.

Sequence: parent design/plan review → actual absent-mode fleet red → minimal
implementation → authored export and official one-definition bridge probe →
one full source export/identity/placement
check with the existing independent raw-source oracle → adversarial review →
root baseline integration. No new product dependency, source archive change,
geometry repair, cached-as-fresh benchmark or product mesh substitution.

Timing: the new reference-only executable avoids unused owned carrier/span
conversion work and parses STEP once. It is the only STEP exporter invoked in
each fresh external baseline. Existing reviewed source and the independent raw
STEP mechanism are out-of-timing comparators, never cached-as-fresh imports.
Complete external wall includes fresh source import,
official conversion/import/tessellation/export, common CPU24 display, both
assembled/exploded presentations and exit. Phase timings attribute the actual
cost rather than inserting a second STEP parse.

Independent owner reviewed the full design/subplan: ACCEPT. Header arithmetic,
single-solid V4/no-mesh path, graph/source-face fields and separate target fit.
Recorded original `ADVANCED_FACE.SameSense` versus actual translated orientation
checks separately, and limited full-manifest qualification to original ID/graph
mechanisms because it intentionally carries no full surface tables. Parent
review remains pending. No implementation yet.

Root identified post-PASS source pin provenance as a design requirement. The
current product adapter is a monolithic main with no reusable session API;
selected a proper new target rather than changing its pins, including private
cpp files or inventing macros. The small reference import/graph path uses the
same authorized OCCT calls but is checked independently through original entity
and serialized geometry/placement output. This keeps V3, its unchanged source
adapter and later pilot checks valid without a gratuitous owned full-mesh rerun.

Root design/plan review ACCEPT after reading the complete source/serialization,
manifest and single-import design. Implementation may proceed after meaningful
fleet red. Distinguish source identity from embedded run timings and keep all
old source/helper/binary pins unchanged.
