# Revolution implementation plan

All three independent design reviews accepted before this plan. Implement one
whole supported revolution as defined in REVOLUTION_DESIGN.md, then stop this
unit at qualified, measured, documented delivery. No tuning, native build, new
library, general result mesher, fillet or offset work in this branch.

## Ownership and reviewed contracts

Three independent owners retain whole conceptual units:

- Geometry owner: `python/brepgpu/revolution.py`,
  `tests/test_revolution_construction.py`, `docs/REVOLUTION.md`; construction,
  exact readers, complete history and atomic publication together.
- Oracle owner: `python/brepgpu/revolution_oracles.py`,
  `tests/test_revolution.py`; independent original-source whole-feature truth,
  native point/normal bounds and actual adjacency qualification together.
- Runtime owner: `python/brepgpu/revolution_sampling.py`,
  `tests/test_revolution_sampling.py`; request/row/provenance/child-lifetime
  contracts together. Root owns actual caller, optional foreign comparison,
  benchmark controller, final reports, documentation and publication.

Root integrates findings, reviews geometry and sampler implementation. Oracle
owner reviews geometry implementation; geometry owner reviews independent oracle
and real saved caller artifacts; runtime owner reviews actual caller/reference
and final performance report. All code owners write subplans and journals.
No owner changes other lanes' files or commits shared work; root commits coherent
checkpoints. Source/static checks may run locally. Product geometry/tests,
reference execution and benchmarks run only on the fleet. GPU jobs are serialized;
coordinate lane use before launches. Preserve existing binaries and toolchain.

The geometry module exports `UnsupportedRevolution`, `Origin`, `HistoryRecord`,
`History`, `PointClassification`, `RevolvedSolid`, `RevolutionResult`, `revolve`.
Reuse exact factory PlanarProfile and original immutable records/Frame; shared
primitive records, namespace, EntityKey and Measure may be reused with existing
semantics. New result/history factories are immutable and operation-specific.
Public result fields: profile,solid,history,timings. Solid fields: profile,body_id,
revision,frame,vertices,curves,edges,coedges,loops,faces,charts,patches,volume.
Exact readers: classify(chart_slot,u,v),classify_point(worldpoint),
edge_point(one_based_edge_id,t),coedge_uv(one_based_coedge_id,t),topology_input().
No existing operation type guard or packed tag is widened. Use result-owned
circle_y curve kind for XZ rings at fixedY, with exact data(radius,axial).

History fields match design: Origin(target,relation,parents,role,source_t,
source_uv,periodic,orientation), with explicit None for inapplicable maps.
HistoryRecord(source,status,images,generated); status modified for sourceV/E/CO,
consumed for sourceL/F/CH. History.from_input and History.origin reject nonkeys,
foreign namespaces and unknown IDs. Original source face's material-generation
relation is represented by the result's retained complete profile/solid binding,
separate from six-kind entity histories. Maps, roles, parent order and every
precise generating ID follow the design table. Output origin order follows
six-kind table order; forward images/generated links filter that order.

Sampler exports RevolutionRequest(result,candidates), RevolutionSamples,
prepare_grid(result,resolution=9), sample(engine,request). Batch fields are
result,request,rows,timings,backend,device; count and evaluated_count properties.
Keep existing generic helpers intact, with no unrelated refactor. Copy float UV
before exact labels, bound iterables at limit+1, precheck chart-count grid budget.
Descriptors, actual lineage/querybits/statuses and per-Patch conversion remain
mandatory. INVALID rows retain status-1/NaN; late native/host failure closes
all children and permits reuse of the borrowed Engine/result.

Independent APIs are qualify_result(result),qualify_samples(batch),
qualify_topology(result,adjacency), and optionally qualify_profile(profile).
Useful result statistics retain vertices/edges/coedges/loops/faces/charts,
history_inputs/history_outputs,modified/generated,genus; sample statistics
count,accepted,rejected,invalid,boundary,max_position_error,
max_normal_error_bound. Native error statistics use exact Fractions until
presentation. Oracle must not call product construction/validation/trim/material/
history helpers to supply expected answers. Existing generic exact polynomial
or native-adjacency representation helpers may be reused only with the actual
new geometry independently derived from original vertices/Frame.

## Red before implementation

After this plan's three reviews accept, owners first write direct tests and
subplans only; root writes the actual source-driven caller. Do not create the
new production modules until root has synchronized tests/caller and recorded a
healthy existing native body prerequisite followed by actual missing-revolution
failures on the fleet. Root saves private raw logs and public concise journal.
Use real complete geometry/native/history assertions in these first tests,
not a replacement import-only test. No laptop product execution.

Then geometry/sampler/oracle owners implement their own units. Geometry owner
runs construction tests and sends a passing record; sampler owner waits for
geometry API and runs actual CPU/serialized CUDA contracts; oracle owner waits
for both and qualifies original-source truth across applicable placements.
Root synchronizes own caller, whose initial red must be followed by a real
qualified CPU result before repeated benchmarks or performance interpretation.
Record meaningful research/design, reviewed-plan/red, test/caller,
implementation, qualification and measured-artifact commits as they land.

## Direct geometry and failure coverage

Verify whole source strict-support/identity contract and the extra radial bound;
complete rational control/domain bijection, exact full ring/line UV identities,
ordered periodic word, opposed pairs and distinct same-face seams; four endpoint
germs per anchor, connected genus-one shell, exact positive pi-volume and tight
bounds. Full history coverage is3N+3→13N,4N modified/9N generated. Falsify source
IDs consistently on both reciprocal sides and require precise-role rejection.
Exact original-value material truth checks sign-aware radical comparisons and
ALL supporting faces, including central hole/axis and non-square radial queries.

Exercise triangle, chamfer, actual verified16/32-vertex lattice profiles, minimum
1/256 edges/radius and all Frame scales, cyclic relabeling and equal-geometry
foreign namespaces. Include horizontal/vertical/sloped generating edges. Sample
actual rational-quarter seams and internal joins, where positions/normals agree
but parameter derivatives can differ. Test tiny native edges at Frame extrema,
actual copied float UV, deliberate descriptor/status/history/topology/normal
corruption and independent fault sensitivity without broad mutation-score claims.
Late construction and sampler exceptions preserve inputs/prior outputs, close
children, publish no partial result and permit retry. Existing operation guards
reject the new result wherever unsupported.

Run construction tests CPU once; new sampling and independent suites on CPU1,
CPU24,P1000,P1001. Run affected existing body,Boolean,extrusion,adjacency,runtime
suites on their relevant placements; separately run any existing CUDA-only skip
on CUDA. No redundant native sanitizer rebuild for unchanged native binaries.
Root compares the five protected native hashes before/after qualification.

## Actual callers, reference and costs

`examples/revolve_profile.py` takes recipe triangle/chamfer/many16,backend,
device,threads,grid,library-path,topology-library,output-directory. Defaultgrid9
produces972/1944/5184 complete candidates. Use actual different meridian polygons,
not repeated bodies; include annular/cylindrical/conical source edges across
recipes. Choose dyadic world placements. Consume exact material/boundary/UV
and forward/reverse history, native rows, and actual N ordinary ring pairs plus
N same-face seam pairs and typed mate/neighbor readers. Read semantic outputs
inside the useful wall; independent checking is separately timed.

Save recipe.tsv with exact source size/Frame and world_volume_pi_coefficient,
profile-points.tsv, all source/native pins, quality.tsv, whole graph.tsv,
history-forward/reverse.tsv, material.tsv, edge-points.tsv, coedge-uv.tsv,
samples.tsv, adjacency edge/use exports and raw ABI5 input/CSR arrays. The SVG
uses actual rational boundary polylines and native points with explicit source
and P100/CPU provenance; it is not a mesh render. Visually inspect it and check
saved coordinates. No private hostnames/paths/raw operational logs in public.

Optional `scripts/reference_revolution.py` runs in the existing isolated
usd-brep environment, creates exact same-world source endpoints and pure
rotational_sweep(curves,Frame.origin,(0,1,0),360,cap_ends=True). Confirm unchanged
original parameter ranges/three point probes, finite bounds/manifold/volume,
all exact physical-edge witnesses and eight spread native interior rows per
face; account for all quarter charts in witness selection. Reject rounded world
endpoints rather than silently changing source. Keep floating pi-volume error
relative1e-8, bounds1e-8*scale, boundary residual/distance1e-7*scale, with finite
checks. Foreign material/history are not compared without an actual API/adapter.

`benchmarks/revolution/run.py` serializes one excluded warmup plus three measured
fresh processes for three recipes and CPU1/CPU24/P1000/P1001:48 calls. Capture
controller/source/native pins and raw48 observations/12 summary cells. Keep
prepared-profile and points-to-consumed walls; profile preparation, constructor,
material/history,request,engine,upload/evaluate/download/convert/scatter/cleanup,
topology and actual reads remain visible. Independent qualification,exports,
reference and full child process have separate costs; nested medians nonadditive.
Record unpinned shared-fleet ranges and compare best CPU without causal thread/
clock claims. No GPU constructor or whole-operation win is assumed.

After direct greens and source/caller review acceptance, run repeated callers
and same-world reference comparisons; obtain actual artifact and final report
reviews. Resolve every valid finding, rerunning only affected checks. Publish
README geometry/API/result links, research indexes, roadmap/issue updates and
round-two journal notes. Required external alpha corpus remains incomplete;
no scope waiver or passing-subset certification. Merge through PR preserving
substantive commits and branch, then synchronize clean main to the fleet without
deleting or replacing builds/toolchains/private contexts. Revolution ends here;
fillet and offset precede the deferred integration/Python/NCSX timing work.
