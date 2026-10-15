# Owned convex-profile extrusion

`make_profile(points,frame)` and `extrude(profile,height)` construct a real
profile-driven solid on CPU. The source is a strict convex CCW polygon with
3..32 distinct vertices, exact1/256-grid XY coordinates in[-.75,.75]² and the
existing bounded Frame. Height is positive, exact1/256-grid in[.25,2], in local
units; the result spans Z=±height/2 about the original source plane.

```python
from fractions import Fraction as F
from brepgpu import Engine
from brepgpu.body import Frame
from brepgpu.extrusion import make_profile, extrude, EntityKey
from brepgpu.extrusion_sampling import prepare_grid, sample

profile = make_profile(((F(-5,8),F(-1,2)),(F(3,4),F(-3,8)),
                        (F(-1,8),F(3,4))), Frame())
feature = extrude(profile,F(3,4))
source = EntityKey(profile.body_id,profile.revision,'edge',1)
images = feature.history.from_input(source).images
origins = tuple(feature.history.origin(key) for key in images)
request = prepare_grid(feature,17)
with Engine(backend='cpu',threads=1,
            library_path='build/pair-release/libbrepgpu.so') as engine:
    samples = sample(engine,request)
```

Both factories publish immutable operation-specific records. They do not call a
canonical box or Boolean factory. All nonincident vertices must be strictly left
of every directed source edge; positive adjacent turns alone would admit winding
stars. Clockwise/concave/collinear/repeated/off-grid/nonfinite/oversized profiles,
holes, curves and unsupported heights fail explicitly without sorting, rounding,
welding or repair. UnsupportedExtrusion subclasses ValueError; malformed types
raise TypeError. Existing canonical/Boolean type guards remain unchanged.

The source owns all six entity kinds: N vertices, N edges, N coedges, one loop,
one face and one chart, with fresh namespace/revision. The fixed source plane
carrier XY=(-1+2u,-1+2v),Z0 is trimmed by the actual polygon. Both output cap
carriers preserve original XY control bits, UV and Frame at Z±h/2. Every source
edge a→b generates the complete bilinear side
S(u,v)=((1-u)a+ub).xy+(v-1/2)h·ez. Controls combine exact original endpoints with
exact height levels; all homogeneous weights1. No fitting/rounding is required.
Local geometry and exact query results use Fraction; native controls use exact
binary64 dyadics. Normal/source-world evaluation still has stated output error.

The result graph has2N vertices,3N edges,6N coedges and N+2 each of loops/faces/
charts, one closed outward genus-zero shell. Source vertex identity shares each
vertical edge between adjacent sides. The lower cap reverses the source word
and face sense; the upper preserves it. Every side has natural outward normal
(dy,-dx,0), with separate intrinsic curve parameter and traversal orientation.
Counts/Euler2 and exact world volume source_area*h*scale³ supplement whole
boundary/domain/map and cyclic-link qualification.

Every one of3N+3 input entities has exactly two same-kind modified cap images.
Every one of14N+6 output entities has an origin:6N+6 modified cap images and8N
side births. Source keys name actual namespace/revision/kind/ID. Origins retain
cap local translations, source edge/UV affine maps, profile-local axial maps and
orientation separately. Swept side uses name their owning source edge/vertex/
coedge occurrences. Reciprocal links alone cannot validate ancestry: publication
also checks precise generating IDs/roles. Geometrically equal profiles in other
namespaces do not share history. Unknown lookups raise KeyError. Persistent
cross-session naming and history composition remain later work.

`feature.solid` retains the exact original profile and supplies:

- `classify(slot,u,v)` exact submitted-value trim constants;
- `classify_point(worldpoint)` interior/boundary/exterior with every incident face;
- `edge_point(edge_id,t)` exact world XYZ and `coedge_uv(coedge_id,t)` exact UV;
- `topology_input()` existing typed ABI5 input with snapshots=(), caller retaining
  the feature result separately.

Entity IDs are1-based; patch slots0-based. Profile.classify(u,v) checks the original
source domain. Exact Fraction boundary UV and its nearest float are distinct
requests. Sampling copies actual binary64 before classification, submits only
interior/boundary candidates, checks native descriptors/Lineage/querybits/backend/
statuses, converts per-Patch frames and scatters oriented normals. Rejected/
invalid rows preserve IDs/order, status−1 and NaN geometry. Resolution2..257,
maximum1,048,576 total candidates;34charts can exceed that cap before257².

The separate extrusion sampler reuses owned type-agnostic provenance/row/cleanup
machinery; it borrows Engine and owns all per-call patch/query/result children.
Late construction failure returns no result and preserves source/prior outputs;
consumer failure returns no partial batch and permits retry with the same retained
feature/Engine. No native build/dependency or result mesher is introduced.

Independent sampled world position component limit is2^-36, oriented unit-normal
limit2^-30 at the actual copied UV. Integer-square-root rational intervals bound
non-axis side unit normals; production normalization and CPU parity are not the
truth. Whole exact source domains/control bits, affine edge/bothUV identities,
source ancestry and actual native CSR/mate/neighbor readers are checked separately.

Fleet reproduction:

```sh
PYTHONPATH=python python3 examples/extrude_profile.py --recipe many16 \
  --backend cuda --device 0 --output-directory results/local-extrusion
PYTHONPATH=python python3 benchmarks/extrusion/run.py \
  --output-directory results/local-extrusion-costs
```

The three caller recipes have genuinely different3/6/16 source vertices. Useful
profile→consumed costs include complete CPU feature construction, exact readers,
request and native contexts/transfers/row/adjacency/cleanup. Source profile
preparation, the additional points→consumed timer, independent checking, rendering/
exports and whole child-process costs are explicit. No GPU feature construction
or speedup is presumed. Optional isolated USD BRep linear_sweep comparisons use
exact same world endpoints and compare measures/boundary witnesses; they do not
supply owned history/material or satisfy mandatory external alpha suites.

[Design](EXTRUSION_DESIGN.md) · [Plan](EXTRUSION_PLAN.md) ·
[Research](../papers/README.md) ·
[Qualification and costs](../results/extrusion-2026-10-15/REPORT.md) ·
[Return pass](RETURN_PASS.md).
Revolution, fillet and offset remain separate subsequent feature units.
