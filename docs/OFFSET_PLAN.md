# Complete signed analytic offset implementation plan

All three independent design reviews accepted before this plan. Deliver the
whole declared sharp-cylinder offset, independently qualify/measure/publish and
merge it, completing the separate feature breadth units. No wider offset,
native build/dependency, mesher, integration tuning or shipping-language change.

## Ownership and exact public contract

Geometry owner owns python/brepgpu/offset.py,tests/test_offset_construction.py,
docs/OFFSET.md: strict original-source admission, all geometry/readers/history/
source-foot records and atomic publication as one invariant. Oracle owner owns
python/brepgpu/offset_oracles.py,tests/test_offset.py: independent whole source-
geometry/material/domain/map/foot/history/native/ABI5 expected truth. Runtime
owner owns python/brepgpu/offset_sampling.py,tests/test_offset_sampling.py: narrow
operation factory/request, actual native provenance/oriented rows and child
lifetimes. Root owns actual caller/reference/controller, integration reviews,
reports/docs/artifacts, feature-branch checkpoints and PR merge. Each owner
writes its complete subplan+journal and owns findings before returning. No
cross-lane product edits or agent Git commits. All product/tests/reference/
benchmarks execute on fleet only; laptop source/AST/artifact inspection is fine.
Serialize native GPU lanes runtime→oracle→root final campaign.

Exports geometry: UnsupportedOffset(ValueError),UVMap2,Origin,HistoryRecord,
History,Footpoint,PointClassification,OffsetSolid,OffsetResult,offset_solid.
Reuse only owned primitive records, Frame/namespace,EntityKey and Measure whose
actual meanings fit. Exact source BodySnapshot cylinder and exact Frame/table/
carrier/packed validation: no replacement make_body and no production testoracle.
Distance exact nonzero /256[-1/8,+1/8]. Result fields source,distance,join,solid,
history,timings; solid fields source,distance,join,body_id,revision,frame,vertices,
curves,edges,coedges,loops,faces,charts,patches,volume. join='intersection'.
Use factory-only immutable result/solid/history/Footpoint/PointClassification.
Typed publication, complete new source-bound namespace and unchanged originals
on failures follow OFFSET_DESIGN.md. No old guards/tags/formats are widened.

UVMap2(kind,data,domain) explicitly requires its exact target domain tuple;
fixed-arity data/domain ingestion bounded by width+1, unknownkind rejects before
reading data. 'affine':6Fractions; 'circle':oneFraction radiusratio. Evaluate exact
numeric inputs excludingbool within declared domain; output raw original UV may
leave source bounds. Origin schema exactly design: typed owner/map/relation
pairings, modified-only sole original same-kind causal parent, role source_kind,
absolute orientations, E/CO identity boundary_parameter source_t. Source UVs name
actual original F/CH incidence; normal_foot relation is analytic-carrier extension,
not bounded-original-face inclusion. All23sources/23outputs complete ordered
history; one modified image/no births each, no numeric key aliases/foreign IDs.

source_foot(slot,u,v) returns the design's exact Footpoint for admitted target
queries only. It reads original raw source UV/analytic carrier extension exactly,
never sends extended sourceUV to old native code/classifier and never uses old
float coercion as exact truth. Differentiate original carrier-square membership,
cap disk rejection within that square, and source trim invalid outside the square.
Preserve query/owner/worldpoint/normal/distance exactly and validate whole signed-
normal identity, including original output rim's two distinct incident feet.
History carrier descriptors remain evaluable on the full declared input rectangle.

Sampler exports OffsetRequest(result,candidates),OffsetSamples,
prepare_grid(result,resolution=17),sample(engine,request). Existing generic
_check/_rows/_child_scope are reused unchanged; all guards remain offset-specific.
Fields/properties use existing request/batch idiom: result,candidates,labels,
admitted_indices; result,request,rows,timings,backend,device,count,evaluated_count.
MAX_CANDIDATES1,048,576, resolution2..257; fail limit+1/pregrid before allocation.
Copy actual floats before exacttrim, explicit rejected/invalid status-1/NaNs,
full metadata/query/Lineage/backend/Frame/status verification and all3child
cleanup/retry on borrowed Engine. No new lifetime/residency feature.

Oracle APIs qualify_result(result),qualify_samples(batch),
qualify_topology(result,actualAdjacency). Result stats include vertices,edges,
coedges,loops,faces,charts,history_inputs/history_outputs,modified/retained/
deleted/generated,genus,volume_rational/volume_pi, source-foot/material/trim/reader
checks. Sample stats count/accepted/rejected/invalid/boundary and exact
max_position_error/max_normal_error. Derive all new expected answers from actual
original canonical source identity/geometry, exactd andFrame, not production
helpers or freshly constructed expected output. Existing source qualifier may
check original once with additional strict source-type checks where needed.
Existing exact polynomial/ABI representation helpers fit their unchanged claims.
All wall directions are exact unit rational normals; no sqrt oracle needed.

## Direct red, implementation and adversarial review

After all three plan reviews accept, owners write tests+subplans only, root
writes the actual caller. No new production modules before root fleet-syncs
those tests/caller, proves healthy native source prerequisite and records actual
missing-offset failures. Then root explicitly releases complete implementation.
Owner construction CPU greens precede native consumers; source stable before
performance campaign. Root reviews whole constructor/sampler/oracle, oracle owner
reviews constructor, geometry owner reviews oracle/tests and saved actual outputs,
runtime owner reviews actual caller/reference/controller and final cost report.
Resolve valid findings; journal rejected findings. Reviews read real outputs and
fault sensitivity, not merely successful status. No tests for tests or campaign
reruns after final greens without a new concern/source change.

Eight construction methods cover full correct complete construction, both signs/
bounds/oddgrid/allFrame scales, original strict admission/preservation, exact
material/curve/coedge/Footpoint readers, complete source map domains/relations/
owners/history and output graph, source square-vs-disk distinction/extension,
immutability/boundediterables and actual latevalidation rollback. Full closed
source/result graph includes actual rim start/end ports and two seam occurrences;
no duplicate output canonical body may supply truth. Eight runtime methods cover
actual carriers/normals/all6charts/provenance, exact copied query admission,
explicit nonadmitted rows and budgets, unsupported request/result types, native
metadata/status rejection, closed Engine, child cleanup and healthy retry.
Twelve independent methods cover whole expected family, all material supports,
positive sharp-vs-Euclidean rim counterexample, inward containment/erosion,
non-axis/nextafter/subnormal literal witnesses, original-domain feet for cap IN
square/OUTdisk and wall OUTV, exact UV/ring/seam joins, strict original/returned
metadata and maps, reciprocal-but-wrong ancestry/context owners, deliberately
false normals/control/status/readers, ABI5 full typed slot permutations and
ordinary/sameface pairs. Qualify actual native samples with direct worldposition
2^-36 and orientednormal2^-30 bounds, not CPU/GPU parity.

Run8constructionCPU;8sampler methods each CPU1/24/P1000/1;12independent methods
eachplacement, zero unexpected skips. Run affected existing body/Boolean/extrusion/
revolution/fillet/native runtime/adjacency suites using both existing library
paths. Existing CUDA-only test skipped on CPU is separately run on selected CUDA.
Keep exact commands/counts/findings in journal and private operational logs; public
qualification records summarize their actual claims. No broad mutation score.
All five protected native library hashes must stay unchanged.

## Actual caller and immutable measured inputs

Root examples/offset_cylinder.py binds actual source and requested d/join on both
returned result/solid before consumers. Retain actual originalsource fields,
requested recipe, graph/carriers and complete forward/reverse history. Consume
exact material probes (old/new rims,cap/wall,inside/outside,newcorner positive-
distance counterexample), five rational t values per edge and three per coedge,
all available history source maps at correct native/raw argument domains.
Read actual source_foot for at least three admitted exact queries per chart,
including both rims and non-axis directions; positivecap witness can be inside
old square/outside disk and positivewall witness outside oldV. Retain point/
normal/distance/domain outputs and require their actual identities. Do not demand
oldboundedface admission for an extension. Include actual CPU/P100 Engine/native
sample/cleanup and actual ABI5 paired+sameface mate/neighbor readers. Independent
qualification and exports are outside usefulwall and timed separately.

Source-to-consumed starts after actual make_body('cylinder',alreadyOwnedFrame).
Factory-to-consumed starts before source creation. Keep source_creation_outside,
cpu_offset,material_boundary_history_consume,request_prepare,engine_setup,
sample_and_consume,engine_cleanup,topology_pack_library_load,adjacency_build,
actual_adjacency_reader,complete_source_to_consumed_cleanup,
complete_factory_to_consumed_cleanup,independent_qualification and artifact costs;
nested construction/sampling/adjacency phases remain named. Full childwall separate.
Actual SVG shows original source overlay and new exact full ring/seam curves
plus admitted native points, labels signed distance/sharpjoins/CPUconstruction,
and remains separate from result meshing/NCSX rendering. Root raster/view review.

Publish source-vertices.tsv,recipe.tsv,quality.tsv,graph.tsv,completehistories,
history-map-reads.tsv,source-foot.tsv,material/edge/UV/native rows,rawABI5 arrays
and actual reader rows,ownedsourcepins+actuallyusednativehashes,SVG and timings.
Source/native pins stable across all final calls; no private paths/hostnames/logs.
Grid17×6charts=1734fixedcandidates; inset/outset/odd recipes perdesign. Final48
freshprocess calls=3recipes×4placements×(1excludedwarmup+3measured). No simultaneous
project GPU tests/reference/tuning workloads while measuring. Report honest
fixed-smallgraphscope,bestCPU/ranges,setup/cleanup/transfers/host cost and nested
nonadditive medians. No GPUconstruction or isolatedkernel claims.

## Optional reference and delivery

Use existing isolated USD BRep1.0.0, public create_cylinder with sourceworldbase,
radius,height and pure offset_brep(source,worldd,extended_offset=True,
self_intersection=True). Actual world factory inputs including all original
anchors/bounds andd are checked exactly representable. Preserve original full
body/ALL reference-edge endpoint/range/threepoint scalarprobes before/after;
probe all returned edge records without requiring a particular foreign split.
Reference returns a new object/sourceunchanged; original source need not be copied.
Discard result handles at process end. No sourcecurve may be read after a
consuming API; selected operation is explicitly pure and preservation is measured.

For each final recipe CPU1measuredcaller artifact, compare finite source/result
bounds/manifold/volume and actual63boundary witnesses(15exactedge+8interior per
sixactualnativecharts). Original source factory geometry must match exact
requested cylinder before comparing output. Save every criterion's measured
value/unchangedlimit before nonzero failure. Volume requestedaccuracy1e-8,
volumerelative1e-8,bounds1e-8*scale,bothboundarydistance/residual1e-7*scale; never
swap tolerance/API/accuracy based on outcome. Pin actualadapter+input artifacts.
Foreign geometry witness passes do not certify full foreign material/history,
externalcommunitysuite or prior revolutionvolumeissue43.

Acquire/pin/read OCCT original offsetcases andretain exactparameter/runner gaps;
ownedcanonical normalizedcases are not communityreplays. Do not cut alpha or
change promised alphacorpus/scope. Report everyfail/unsupported honestly. Update
READMEwithactualgraphic/API,roadmap/AGENTS/ISSUES/papersindex/RETURN_PASS and
roundtwonotes/journal. Meaningfulcheckpointcommits pushedthroughfeaturebranch;
PR preservescommits, closesissue47, keepbranch. Review actualhead/mergeability,
merge, then synchronizeclean main/fleet source withoutdeleting native/toolchain/
private context. RecordactualmergeSHA+handoff bookkeeping. This completes the
separate analytic offset breadth unit; choose return-pass priorities only after.
