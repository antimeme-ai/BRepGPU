# Convex profile extrusion delivery plan

All three design reviews accepted. Implement the complete declared polygon
profile→solid operation, direct fleet truth, native consumers and measured caller.
No dependencies, native rebuild, generalized body protocol or integration tuning.

## 1. CPU feature and history: one conceptual owner

Own python/brepgpu/extrusion.py and tests/test_extrusion_construction.py. Save a
bounded subplan/direct tests, record healthy existing body prerequisite then
actual missing new API red on fleet BEFORE implementation. Root coordinates Git;
sync only owned files. Every product/geometry/reference run stays on fleet.

API: make_profile(points,frame=Frame()), extrude(profile,height), named
UnsupportedExtrusion(ValueError). Exact source coordinates/grid checks precede any
float conversion. Both point iterables and vertex arity are bounded as design.
Frame must be exact owned Frame. Store all local vertex/curve/UV geometry as
immutable Fractions; native controls are exact float conversions of those dyadics.
Global strict support against all nonincident vertices, not adjacent turns alone.

Factory-only frozen fields:

- PlanarProfile: body_id,revision,frame,vertices,curves,edges,coedges,loops,faces,
  charts,patches,area. Full source3N+3 six-kind entities, fixed-square carrier.
  classify(u,v) exact original-value trim constants as design.
- ExtrudedSolid: profile,body_id,revision,frame,height,vertices,curves,edges,
  coedges,loops,faces,charts,patches,volume. Exact public classify(slot,u,v),
  classify_point(point),edge_point(edge_id,t),coedge_uv(coedge_id,t),
  topology_input(). PointClassification: solid,point,label,boundary_faces.
- ExtrusionResult: profile,height,solid,history,timings; retain actual source
  object in result and solid; fresh shared namespace/revision1.
- EntityKey: reuse existing owned generic boolean.EntityKey (re-export allowed).
  Origin/HistoryRecord/History operation-owned with EXACT design fields/maps;
  History.from_input/origin unknown keys KeyError, all input statuses modified,
  exactly2 cap images each, whole8N side births and complete reciprocal links.

Follow design record order/words. Input source curve directions survive bottom/
top translations; bottom occurrence word reverses. Build all profile/solid graph,
carrier bits/frames, exact source maps and full feature history in this single
unit. Private _validate_candidate boundary before immutable publication supports
late exception injection with original profile and prior result unchanged.
Validate complete loop/face/chart ownership, exact endpoint and whole affine
curve/incidentUV maps, opposed uses, connected shell/cyclic links and all history
before publish. Source profile validation has a similarly atomic factory boundary.
Timings distinguish admission, geometry/topology/history, validation and returned
operation; outer caller wall includes complete result publication.

## 2. Independent whole-feature and native-output truth

Own python/brepgpu/extrusion_oracles.py and tests/test_extrusion.py. Test-first
healthy prerequisite/missing-operation red, then product qualifier. Public flat
metrics: qualify_profile(profile), qualify_result(result), qualify_samples(batch),
qualify_topology(result,adjacency_result). Never import production private feature/
trim/validation/source-map helpers for expected geometry/history. Existing owned
polynomial and ABI5 independent oracles may serve their existing direct claims.

Read original actual profile/grid/Frame/height; derive complete source graph,
fixed cap carrier/polygon domains and every side from literal original endpoints.
Check all native control bits, UV/frame/Lineage, exact whole affine edge/bothUV
identities and public non-dyadic readers; role bijection forbids missing/duplicate
faces. Verify all source statuses, all14N+6 origins and every exact affine history
field/parent/role/parity; counts/Euler/volume alone are insufficient. Check strict
slot/key types, full record ownership, opposite uses, actual connected vertex
links and one embedded closed genus0 shell from full domains/strict convexity.

Public material probes cover interior/exterior, all caps/sides/edges/vertices,
exact offsets and nextafter values. Trim probes include all polygon edges/vertices,
non-dyadic boundary positions and rounded native neighbors, outside fixed carrier,
and malformed/nonfinite input. Actual profile.classify is also checked directly.
Sampling world component limit2^-36 and oriented unit-normal2^-30 uses actual
copied binary64 UV and original Frame; independent integer-square-root intervals
as design for non-axis normals (perfect-square and nonsquare cases). Rejected/
invalid native rows remain explicit status−1/NaNs. Check exact actual request
owner/gather/order/labels/backend. ABI5 output must match source incidence and
actual mates/neighbors under slot permutations, with no seam/boundary leftovers.

Use asymmetric triangle, chamfered polygon, verified32-vertex lattice fixture,
minimum edge/height, maximum bounds/heights, every Frame scale, translations and
cyclic source-order metamorphisms. Reject hostile positive-turn winding star,
clockwise/concave/repeated/collinear/off-grid/nonfinite/bool/oversized inputs and
unsupported heights. Geometry-equal independently allocated profiles must keep
distinct history. Direct falsified control/domain/orientation/typed/history/map/
sample outputs and late construction failure test named faults. No broad mutation
score/alpha claim; actual selected production mutants only if needed, fleet only.

## 3. Narrow sampler and ownership

Own python/brepgpu/extrusion_sampling.py and tests/test_extrusion_sampling.py.
Test-first red before product implementation. Public ExtrusionRequest(result,
candidates), prepare_grid(result,resolution=17), sample(engine,request) returning
factory ExtrusionSamples(result,request,rows,timings,backend,device). Request has
result,candidates,labels,admitted_indices; count/evaluated_count properties on batch.
Reuse Candidate,SampleRow and existing type-agnostic Boolean _check/_rows and
primary-error-preserving _child_scope after confirming actual fit. No changing
Boolean/canonical type guards. Separate extrusion factory/result guards remain.

Copy actual float UV before exact classification, grid full[0,1]^2 on every
chart;2..257 exclbool,1,048,576 total candidates, precheck chart_count*res² and
cap+1 iterable consumption. Max32-source profile has34 charts; do not inherit
canonical ten-chart cap. Full request/descriptor/querybits/status/backend/Lineage
checks inside useful wall, original per-Patch conversion and face-sense normals,
all rows in original order. Per-call patch/query/result children close; failure
publishes no partial batch and keeps original result + borrowed Engine usable.
Direct late host exception must close all3 real handles, preserve primary error
and support healthy retry; real corrupted native descriptor must reject. Empty/
all-rejected requests, source float bits, caps and non-axis side normals included.
Run CPU1/CPU24/P1000/P1001 tests serial GPU slots; no builds/dependencies.

## 4. Actual caller, optional foreign comparison and costs: root

Own examples/extrude_profile.py, scripts/reference_extrusion.py and
benchmarks/extrusion/run.py. Root bounded subplan + actual fleet missing caller
red before code. Use three genuinely different source recipes (triangle,
asymmetric chamfer, many-sided lattice polygon), same declared per-chart grid.
Export original exact points/frame/source keys and complete profile/output graph,
all carriers/UV/maps, both history directions, material/edge/use reads, actual
sample rows and ABI5 CSR/mates/neighbors. TSV/native binary, source/native hashes,
observed backend/device; no canonical tags/JSON/private metadata.

Profile preparation has its own outside timer; useful profile→consumed wall
includes extrusion/publication, exact material/boundary/history reading, request,
Engine/sampling/cleanup and separate topology library/build/actual reader. Also
report complete input-points→consumed wall including profile preparation. Keep
qualification, SVG/export, process import/launch and reference outside useful
feature timer; full child-process wall separately. No nested/median summation.
After source/direct greens, serialize one excluded warmup plus3 measured fresh
processes per recipe×CPU1/CPU24/P1000/P1001 (48calls). No concurrent project GPU
work. Preserve cold setup/cleanup and qualified output, no tuning. Exact output
boundary wireframe plus admitted native samples may illustrate actual result,
without pretending a general mesher or GPU constructor exists.

Use existing isolated usd_brep reference only: actual same lower-plane source
segments and pure capped linear_sweep, world distance h*scale. Published dyadic
recipes make exact world input endpoints representable; validate and record this
rather than silently round. Check original reference curves before/after public
API, actual finite complete bounds/manifold/volume and boundary/interior-face
closest-point witnesses. No foreign history/material API or community corpus pass
inferred. No adoption/install/build.

## 5. Review and land

Cross-review whole CPU/profile/history, independent oracle and sampler/caller
source after actual green. Fix all valid findings; journal rejected findings.
Review actual saved geometry/provenance/SVG and cost records against claims.
Run affected existing body/Boolean/native runtime/adjacency compatibility at
appropriate placements after own greens, with existing native binaries unchanged.
No extra native sanitizer rebuild for new Python-owned construction/adapters.
Update README/API, roadmap/AGENTS/issue/paper indexes and return-pass notes;
keep revolution/fillet/offset next, external alpha corpus non-green and deferred
integration/Python/NCSX timing explicit. Publish coherent branch commits, merge
PR preserving commits/branch, sync clean main and journal final fleet handoff.
