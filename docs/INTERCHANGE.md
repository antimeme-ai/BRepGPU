# Reference-to-owned surface carriers

The interchange breadth beachhead reads a declared **static ASCII USD BRep
export subset**, copies supported NURBS carriers into owned Patch records, and
consumes their positions and first derivatives on native CPU or P100. It retains
every original face as converted or explicitly unsupported. It does not interpret
trims, import a solid, compose a USD stage or support general STEP/IGES/USD.

The [design](INTERCHANGE_DESIGN.md), [plan](INTERCHANGE_PLAN.md) and
[actual results](../results/interchange-2026-10-15/REPORT.md) define the scope.
No new dependency or native ABI is introduced. The existing isolated NVIDIA
USD BRep runtime supplies a separate reference experiment, not a product import.

## Read and evaluate

From a qualified fleet checkout:

```sh
PYTHONPATH=python python3 examples/import_carriers.py \
  --input data/interchange/mouse.usda --backend cuda --device 0 \
  --library-path build/pair-release/libbrepgpu.so \
  --grid 17 --output-directory results/local-interchange/mouse
```

Use `--backend cpu --threads 1` or `--threads 24` for the identical CPU caller.
Grid width is a power-of-two number of intervals plus one, at most129; the
complete request is capped at1,048,576 samples. The normal caller imports only
owned modules and the standard library. It fails if there are no converted faces.
The inventory API can represent a valid source containing only unsupported faces:

```python
from brepgpu.interchange import load
conversion = load("data/interchange/mouse.usda")
for face in conversion.faces:
    print(face.key, face.status, face.reason, face.patch_slot)
patches = conversion.patches
```

The foreign mouse has four NURBS faces. The curved quadratic×linear and bilinear
carriers at original face indices1 and2 convert; faces0 and3 contain multiple
spans and remain unsupported. All four have general trims, retained as
**uninterpreted**. The full carrier rectangle can extend outside the trimmed face.
`face.face_range` is source evidence, not a membership test. Source faceuse
orientations are retained individually; no unique outward face normal is inferred.

## Supported boundary and numerical meaning

The reader tokenizes and type-checks complete self-contained UTF-8 `#usda 1.0`
files with identity Xform/Scope containers, BrepArray primitives, uniform typed
arrays and the explicitly documented small metadata grammar. References,
payloads, layers, variants, authored transforms, animation, connections, unknown
syntax/types, duplicate identities and binary USD are rejected. This is an owned
bounded subset reader, not an OpenUSD replacement. Input is capped at16MiB,
1million tokens,500,000 scalar array components,4,096 primitives and nesting64.
`SourceError`, `UnsupportedSource` and `SourceBudgetError` name invalid input,
unsupported file semantics and resource admission failures. No partially parsed
or converted snapshot is published after such a failure.

A converted carrier is one clamped tensor span, degree0..3 in either direction,
with positive weights that admit exact common power-of-two normalization into
[1,256]. Coordinates admit a power-of-two spatial scale and source spans in
[2^-20,2^20]. Multispan/unclamped/higher-degree or out-of-envelope carriers have
explicit unsupported reasons. Unknown analytic surface tokens remain inventoried;
no cylinder angle map is guessed from a bounding rectangle.

Source XYZ is Euclidean, stored U-major/V-fast; owned controls are homogeneous.
Each weighted coordinate is rounded once. Exact control displacement bounds the
whole-carrier position conversion error, with separate first-partial conversion
bounds. These do not replace the independent native arithmetic checks. Original
knot domains and affine parameter maps are retained. Native output is checked
against the actual supplied controls; restored source derivatives also account
for rounded domain subtraction, scale division and final multiplication. Source
metadata retains its unit/up-axis declarations; no units or scene transforms are
invented. Geometry remains in the source local coordinates.

`Conversion` owns original bytes, their SHA256, the immutable typed document,
all source-face records and native patches. Full source keys are
(content SHA256,prim path,zero-based face index). Native numeric body/face/chart
identity uses the shared owned namespace and document-wide face ordinals. Equal
geometry does not merge identities. These are snapshot identities, not a promise
of persistent naming across changed files/sessions.

## Inspect and measure

The caller exports original source bytes and controls/knots, every face/use/status,
actual native control bits and patch domains/lineage, query bits and exact-affine
source UVs, local/source-world raw jets, consumed rows, numerical checks, native/
source hashes and phase costs. TSV and binary records are directly inspectable.

```sh
PYTHONPATH=python python3 benchmarks/interchange/run.py \
  --input data/interchange/mouse.usda \
  --output-directory results/local-interchange/costs
```

The driver runs CPU1,CPU24 and each P100 in fresh subprocesses, one excluded
warmup and three measured repetitions. The useful wall includes source read,
parse/validation/conversion, query preparation, context creation, uploads,
evaluation, download, required validation/frame conversion, checksum consumption
and cleanup. Exact independent qualification and artifact writes are separate;
child-process time includes those plus import/launch overhead. Excluding a warmup
does not establish retained GPU-context reuse. These small cold tasks measure the
whole operation, not a modeling speedup or a resident-kernel throughput contest.

The optional `scripts/reference_interchange.py` uses the already isolated
usd-brep1.0.0 environment to export a copied polynomial sheet or read the original
source and record actual scalar jets. `scripts/compare_interchange_reference.py`
checks those records against exact source geometry at actual submitted float UVs,
and compares owned output accounting for source-query rounding. The intended
polynomial sheet is checked against its independently specified analytic map.
Neither successful foreign import nor reference parity alone qualifies our output.

General NURBS extraction, additional formats, trims/topology, scene composition
and cross-session naming are return-pass work. Applicable community exchange
qualification is still missing for alpha; these examples do not satisfy that
release requirement. Continue the uncovered Boolean/history and feature themes.
