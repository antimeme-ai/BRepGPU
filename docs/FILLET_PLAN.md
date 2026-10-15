# Complete one-edge fillet implementation plan

All three independent design reviews accepted before this plan. Implement the
whole declared source-box operation, qualify and measure it, publish/merge, then
move to offset. No wider/interacting fillet, native build, dependency adoption,
mesher, integration tuning or shipping-language change belongs in this branch.

## Ownership and APIs

Geometry owner: python/brepgpu/fillet.py,tests/test_fillet_construction.py,
docs/FILLET.md; own source admission, exact construction/readers, full history,
contacts and atomic publication as one unit. Oracle owner:
python/brepgpu/fillet_oracles.py,tests/test_fillet.py; own independent original-
source whole geometry/material/maps/tangency/native/topology truth. Runtime
owner: python/brepgpu/fillet_sampling.py,tests/test_fillet_sampling.py; own request,
provenance/rows and native child lifetimes. Root owns actual caller, optional
reference, benchmark controller, integration reviews/reports/docs/publication.
Each owner writes subplan+journal; root commits coherent checkpoints. No owner
edits other lanes' files or commits shared work. Source/static inspection locally;
ALL geometry/tests/native/reference/benchmark execution on the fleet. Serialize
GPU lanes explicitly; root uses GPU only after both test owners release them.

fillet.py exports UnsupportedFillet(ValueError),Map2,Origin,HistoryRecord,History,
ContactRail,PointClassification,FilletedSolid,FilletResult,fillet_edge. Reuse only
owned primitive records/Frame/namespace, EntityKey and Measure where actual
semantics fit. Exact factory BodySnapshot shape box and exact Frame are required;
validate full canonical original tables/controls/type/packed consistency without
creating a substitute source or using test-oracle helpers in production. Wrong
object/key types raise TypeError; unsupported source/selection/radius geometry
raises UnsupportedFillet or appropriate key ValueError/KeyError, atomically.
Radius exact local /256 lattice[1/16,1/2]; all four full vertical edgeIDs9..12.

Result fields: source,edge,radius,solid,history,contacts,timings. Solid fields:
source,edge,radius,body_id,revision,frame,vertices,curves,edges,coedges,loops,faces,
charts,patches,volume. edge is the retained actual typed source EntityKey. contacts
is a two-element tuple in incoming/outgoing order. FilletResult/FilletedSolid/
History/PointClassification factories immutable. Primitive records preserve
retained binary64 payload bits; changed/new exact coefficients are Fractions,
with integer quarter metadata. No old public guard or packed tag is widened.
Public readers and deterministic output ordering follow FILLET_DESIGN.md.

Map2(kind,data).evaluate(u,v) takes exact finite numeric arguments excluding bool,
both in[0,1]. affine has6Fraction coefficients; arc has3Fraction coefficients
and strict intq. Origin fields exactly the design's owner/role/map schema;
source_t/source_t_owner/source_t_relation are all set or all None; relation
coincident/spine is explicit. source_uv/source_uv_owner are paired, owners name
actual original face/chart context. A sourceCO UVowner is its original incident
face, even though it is not an image-parent. Origin.orientation is ABSOLUTE:
V/E+1,COtargettraversal,F/L/CHtargetfacesense; no second sign on intrinsic maps.
HistoryRecord(source,status,images,generated), all62 source records/six-kind order;
origins all76 target records/six-kind order. Every precise parent/owner/role and
map follows the design table, including 33R/24M/5D and19births. Deleted entities
can still parent generated outputs. from_input/origin reject foreign/nonkeys.

ContactRail fields: edge,source_face,source_coedge,blend_face,blend_u,source_uv,
blend_uv,source_t,normal. Keys are typed original/result entities; blend_u strict
int0/1; source_uv/blend_uv are Map2; source_t affineTriple giving selectedspine
parameter from(u=t,v=0); normal exactFraction3 outwardunitaxis. Validate actual
selected source coedge incidence, owner namespace, both complete curve/UV maps
and normal identities. No claim the new band interior lies on an old plane.

Sampler exports FilletRequest(result,candidates),FilletSamples,
prepare_grid(result,resolution=17),sample(engine,request); batch fields
result,request,rows,timings,backend,device and count/evaluated_count properties.
Use existing genuinely generic _check/_rows/_child_scope without refactoring
other operation guards. Actualfloatcopy before exact admission, complete budget
1,048,576 checked atlimit+1 and before7-chart grid allocation, resolution2..257
excludingbool. Keep explicit rejected/invalid rows status-1/NaNs and full metadata/
querybits/Lineage/backend/order/count/status checks. Borrow Engine; close all3
owned native children on failure and permit sameEngine/result retry.

Oracle APIs qualify_result(result),qualify_samples(batch),
qualify_topology(result,adjacency). Statistics retain vertices/edges/coedges/
loops/faces/charts,history_inputs/history_outputs,retained/modified/generated/
deleted,genus; samples count/accepted/rejected/invalid/boundary and exact maximum
position/normal errors. Original body oracle may qualify unchanged source once;
new expected answers must derive independently from originalbox/key/radius/Frame,
not production construction/trim/material/Map2/history helpers. Reuse existing
owned exact polynomial and ABI5 representation helpers only for their actual
unchanged claims. No CPU/GPU parity as geometry truth.

## Red then complete units and review

After all three plan reviews accept, owners write subplans and direct tests only.
Root writes actual source-driven caller. No new production modules until root
syncs tests/caller and records healthy existing native body prerequisite followed
by real missing-fillet test/caller failures on the fleet. Tests must already
contain complete output geometry/material/contact/history/native assertions.
Then root releases whole geometry/sampler/oracle implementation units. CPU
construction greens precede native consumer qualification. Record source/static,
fleet failures/findings and corrected greens; no laptop product run.

Root reviews full geometry and sampler, oracle owner independently reviews
construction, geometry owner reviews oracle/tests and actual saved caller
artifacts, runtime owner reviews actual caller/reference/controller and final
performance report. Resolve every valid finding before measuring final sources.
Source pins/native hashes must remain unchanged within the campaign. Meaningful
research/design,plan/red,tests/caller,implementation,qualification,reference and
measured-artefact/documentation checkpoints become separate commits.

Direct cases include all4edges,radius extrema and oddgrid,allFrame scales,
retained carrier/primitive bits, exact nonbinary shortened source ranges and cap
pcurves, complete source incidence/closed words,15pairedges and genus0. Verify
whole rational edge/bothUV/contact identities and full outwardnormal tangency on
two rails. Caps remain sharp; no G2/endblend claim. At a dyadic representable
contact (e.g radius3/8), evaluate both actualnative incident chart queries.
For nonbinary contacts, grade each actualfloatquery, admission and roundoff;
never force an unrepresentable join/snap or demand identical float points.

Exact material includes old removedcorner,center/artificialsupportlines,unused
circlepart,all tangent/cap strata/all supporting faces,subnormal/nextafter probes.
Check 33retained+24modified+5deleted source outcomes and 19generated roles/maps/
owners, including reciprocally consistent wrong ancestry and wrongcontextowners.
Direct corruption of controls/domains/UV/senses/contacts/native metadata/rows must
be rejected by relevant mechanism. Late final validation and sampler failures
publish nothing, preserve source/prior output, close actual handles and retry.
Bounds/radius/input generators/type aliases/foreign keys have explicit failures.
Named fault sensitivity is not a broad mutation or external alpha score.

Run constructor suite CPU once; sampler+independent suites CPU1,CPU24,P1000,P1001.
Run affected existing body/Boolean/extrusion/revolution/adjacency/runtime suites
on applicable placements; separately run existing CUDA-only skip on CUDA. No
native sanitizer rebuild for unchanged native code. Root compares five protected
native core/CUDA/topology hashes before/after. Broaden checks only for new edits,
failures or unresolved actual findings.

## Actual operations, reference and complete costs

examples/fillet_box_edge.py takes recipe small/mid/large,backend,device,threads,
grid,library-path,topology-library,output-directory. Each recipe constructs actual
sourcebox and selects typededge; vary edge/radius/Frame, same76entity/7chart graph,
defaultgrid17=2023candidates. Prepared-source→consumed wall includes full fillet
publication, exact material/edge/UV reads, forward/reverse history and evaluated
source_t/sourceUV maps, both contactrail maps/curve/normal reads, request+engine/
transfers/native provenance/frame/normal/scatter/cleanup, ABI5 and actual15pair/
30mate-neighbor readers. A second wall includes source creation from its already
owned Frame. Independent qualification,exports/SVG/reference/import/process work
outside; separate fullchildwall. Nested phases/medians nonadditive.

Save recipe.tsv (sourceedge/radius/Frame/observedplacement/exact volume coefficients),
all owned source/native pins,quality.tsv,whole source/output graph.tsv,
history-forward/reverse.tsv,history-map-reads.tsv,contacts.tsv/contact-map-reads.tsv,
material.tsv,edge-points.tsv,coedge-uv.tsv,samples.tsv,actual adjacency readers and
raw packed source/CSR arrays,costs.tsv and fillet.svg. SVG draws actual exactline/
quarterarc boundary polylines and nativepoints/normals, with explicit CPU feature
and observed backend. No general mesher or GPU-raster timing claim. Root views
raster preview locally; independent artifact review binds saved geometry/UV/history/
CSR/SVG. Keep private hosts/paths/rawlogs in ignoredcontext only.

scripts/reference_fillet.py uses existing isolated usd-brep reference. Construct
same exactworldsourcebox; verify allworldbounds/endpoints/radius exactlybinary64
or reject without implicitrounding. Preserveoriginal fullbody volume/bounds and
all originaledge endpoint/parameter/point probes. Copy original, select UNIQUE
copyedge from originalsourceworldendpoints, filletcopy via mutating fillet_edges
with worldradius and circularxsect1; discard stale copiedchildhandles aftercall.
Probeactualuntouchedoriginal again. Compare completefinitebounds/manifold/volume
and five exactsamples per physicaledge plus eightspreadnativeinteriorrows/face.
Keep limits volume1e-8relative,bounds1e-8scale,boundary1e-7scale unchanged. Write
allgeometriccomparison outcomes/inputs/witnesses/costs/sourcepins,then exitnonzero
if any criterion fails. Structural/nonfinite output remains a hard error. No
foreignhistory/material claim or implicitresolution of open revolutionissue43.

benchmarks/fillet/run.py executes oneexcludedwarmup+three measuredfreshprocesses
for each3recipes×CPU1/CPU24/P1000/P1001=48calls. They are fixedsizegeometry cases,
not larger models or inflated repeatedpatch throughput. Save raw48/summary12cells,
source/controller/nativepins and scope. Unpinnedsharedfleet/threeobservations show
ranges; no causalCPUthread/clock or wholeGPUwin without evidence. No tuninglane.

After directgreens/source+calleracceptance, run actualqualifiedCPUcaller, repeated
costs and all3sameworldreferences. Retain anyforeignfailedcriteria honestly.
Publish reviewedreport+READMEactualgeometry, APIs/research/bib/index/roadmap/issue
updates and roundtwonotes. All required externalcommunityalpha suites remain
incomplete/non-green; no scope/tolerance waiver. Merge PR preserving substantive
commits/featurebranch, then sync cleanmain without deleting builds/toolchains/
privatecontexts. Offset follows; integration/Python-role/NCSX timing remain later.
