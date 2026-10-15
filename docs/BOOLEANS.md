# Owned through-bore Boolean and source history

`brepgpu.boolean.difference(box,cutter)` constructs a regularized closed
box-minus-cylinder solid on CPU using the actual input face sections, planar
arrangements and opposite-operand material classifications. An existing native
CPU/CUDA evaluator samples its retained carriers; the existing ABI5 library
builds and reads its edge-use adjacency. CUDA does not construct this Boolean.

The supported operands are exact owned factory `BodySnapshot` values: a box and
a cylinder with identical XYZ frame origin, and cylinder frame scale exactly
twice the box scale. Box scales .25,.5,1,2 are admitted, with existing bounded
common translations. In box-local coordinates, the box is
[-.75,.75]×[-.75,.75]×[-.5,.5]; the cutter radius is .5 and its ends are at z±1.
Other shapes, offsets, scale ratios, partial cuts, contact cases and operations
are explicitly unsupported. They do not silently produce an empty/unchanged solid.

```python
from fractions import Fraction as F
from brepgpu.body import Frame, make_body
from brepgpu.boolean import difference, EntityKey
from brepgpu.boolean_sampling import prepare_grid, sample
from brepgpu import Engine

box = make_body('box', Frame((0,0,0),1))
cutter = make_body('cylinder', Frame((0,0,0),2))
cut = difference(box,cutter)
assert cut.solid.classify_point((F(5,8),0,0)).label == 'interior'
assert cut.solid.classify_point((F(1,2),0,F(1,2))).boundary_faces == (1,7)
source = EntityKey(box.body_id,box.revision,'face',1)
record = cut.history.from_input(source)
origin = cut.history.origin(record.images[0])
request = prepare_grid(cut,17)
with Engine(backend='cpu',threads=1,
            library_path='build/pair-release/libbrepgpu.so') as engine:
    samples = sample(engine,request)
```

`CutResult` retains the original operands, both actual lower/upper intersections,
both original cap arrangements, thirteen classified region/band/face decisions,
an immutable `CutSolid`, complete history and phase timings. The candidate graph
and history are validated before publication. Failure preserves inputs and any
prior result; sampling failure publishes no partial batch and preserves the
already completed solid.

The graph has ten vertices, fifteen edges, thirty coedges, nine loops, seven
faces and ten carrier charts. Every edge has two opposite uses, including the
same-face cylindrical seam at distinct atlas U0/U4. The two annular cap holes
contribute to Euler characteristic0, giving a single genus-one shell. This is
assembled from original input entities and exact section events, with no XYZ
welding. It is not the existing factory plate: that plate's hole is smaller.

The result keeps all original box carrier bits and the original cutter wall
carrier bits and frames. Only the wall's trim shrinks to original V[.25,.75],
and its material orientation reverses. Wall carrier scale therefore differs
from the solid's topological frame; consumers use each Patch's actual frame.
No carrier refitting or hidden UV renormalization occurs.

History covers every original vertex/edge/coedge/loop/face/chart with a typed
`EntityKey(body_id,revision,kind,entity_id)` and status: 58 retained, 13 modified,
14 deleted. All 81 output entities have reverse origins, including 10 generated
entities. New rims jointly name the actual box cap and cutter wall, section
pieces/events and cap arrangement uses. New rim anchors also name the original
cutter seam. The cropped seam has original t[.25,.75]; orientation is a separate
map. `from_input` and `origin` raise KeyError for unknown keys. Identity is scoped
to the retained source revision; cross-session persistent naming is later work.

`solid.classify_point` uses exact submitted int/Fraction/finite-binary64 values
and returns interior/boundary/exterior with every incident face. Rim points stay
on the material boundary; points inside the bore are exterior even on a cap
plane. `solid.classify(slot,u,v)` returns existing trim constants and preserves
exact Fraction parameters. `edge_point(edge_id,t)` returns exact world XYZ and
`coedge_uv(coedge_id,t)` exact source atlas UV for intrinsic edge traversal;
IDs are 1-based and patch slots 0-based.

Sampling copies candidates to actual binary64 before exact trim classification.
For example cap UV(5/6,1/2) as exact Fractions is a boundary point, while its
nearest binary64 is slightly outside the disk and is material interior. No
snapping reconciles these distinct requests. Rejected/invalid candidates retain
rows, trim labels, native status−1 and NaN vectors; they are not evaluated. The
consumer checks native counts, query bits, source descriptors/lineage, backend
and statuses, then converts first jets and orients normals. Children close per
call; the caller owns the borrowed Engine. Independent accepted sample limits
are per-component world position 2^-36 and oriented unit normal 2^-30, relative to
the original carrier at actual submitted UV, not CPU parity.

`solid.topology_input()` emits existing `TopologyInput` with typed sources and
`snapshots=()`. Retain the CutResult alongside it. This separate result type is
not a canonical BodySnapshot: existing canonical meshing, classification adapters
and `pack_bodies` do not accept it. There is no general result mesher or imported
solid support in this operation.

Fleet reproduction:

```sh
PYTHONPATH=python python3 examples/cut_bore.py --recipe unit --backend cpu \
  --output-directory results/local-through-bore
PYTHONPATH=python python3 benchmarks/boolean/run.py \
  --output-directory results/local-through-bore-costs
```

Use `--backend cuda --device 0` or 1 for the P100 consumers. Source preparation,
CPU construction, exact graph/history reading, request setup, Engine and native
sampling/adjacency costs, cleanup, independent checking and artifact writes have
explicit timing scopes. The optional `scripts/reference_boolean.py` runs with
the separately installed experiment interpreter, and compares volume, bounds and
boundary closest points on like-sized operands. It does not qualify owned
history or replace mandatory external alpha suites.

[Design](BOOLEAN_DESIGN.md) · [Plan](BOOLEAN_PLAN.md) ·
[Research](../papers/README.md) · [Return-pass notes](RETURN_PASS.md).
