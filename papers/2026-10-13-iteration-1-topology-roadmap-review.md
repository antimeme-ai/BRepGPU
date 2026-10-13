# Adversarial review: topology, integration and delivered-value coherence

Review date: 2026-10-13. Reviewed `docs/TOPOLOGY.md`, `docs/ACCELERATION_ROADMAP.md` and `docs/ITERATION_1_DESIGN.md`, with the pinned source evidence in [the acceleration study](2026-10-13-acceleration-roadmap-study.md). This review changes no product code and does not certify runtime behavior.

The numerical delivery has an honest scope: owned untrimmed rational-patch jets, batched Python/native calls, explicit failures and complete timing. A supported source adapter is optional and existing SMLib Boolean acceleration is explicitly disclaimed. Two high identity defects found in the initial draft were corrected during review and re-read. Two further issues concern the future SSI/topology architecture and should be corrected before that architecture is treated as settled; they do not require expanding iteration one into a Boolean kernel.

## Findings

### T1 — High, corrected and re-read: spatial identity included occurrence orientation/chart

The initial topology draft included chart and orientation in an intersection event's shared spatial-edge grouping key. Opposite incident face uses and periodic seam charts would then produce different keys, defeating the promised shared edge identity.

The revised text separates an operand-order-normalized spatial key using source entities and branch/segment provenance from per-coedge occurrence keys carrying owning face/loop, chart, UV curve and orientation. This directly fixes the semantic conflict. A future implementation must canonicalize source/branch events across patch subdivisions as well; a per-seed or per-patch-pair identifier must not become the spatial authority.

Source basis: OCCT separates underlying entity, location and orientation; its closed-surface representation stores two pcurves on one surface. SMLib also distinguishes edge and ordered edgeuse occurrences. [OCCT shape identity](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/TopoDS/TopoDS_Shape.hxx), [closed-surface curves](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRep/BRep_CurveOnClosedSurface.hxx), [SMLib edgeuses](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/inc/SmEdgeuse.h).

### T2 — High, corrected and re-read: dense query slots lacked exact patch-set binding

The initial numerical design described immutable query sets containing dense patch slots without specifying which patch set owned those slots. A query set built for one face collection could be evaluated against a different same-sized collection, silently returning positions on unrelated geometry.

The revised design creates query sets against an exact patch-set identity/revision and exposes `evaluate(query_set, order)` with no replacement patch-set parameter. Query sets retain their patch storage; results retain query sets and provenance. Closing public handles does not free dependent storage, while context close invalidates children. Retention costs are now disclosed. This removes slot reinterpretation and dangling dependency storage at the design level.

The implementation plan should use a direct hostile case: two same-sized patch sets with different control nets, queries created for the first, then attempts to substitute or close handles. The intended result is pinned geometry or an explicit invalid-handle result, never a successful reinterpretation. This is one direct lifetime/identity oracle, not an additional certification layer.

### T3 — High for future topology, pending at this review checkpoint: edge-to-UV parameter correspondence is unspecified

The topology record table names one spatial curve per edge and a UV curve/chart plus orientation per coedge. It does not specify the edge's restricted spatial-curve interval, the coedge's UV-curve interval, or how the two parameterizations correspond. Matching endpoints and orientation does not define an interior map. Two curves can trace the same locus with different speeds/ranges; a curve can also double back or switch branches while retaining approximate proximity.

Required decision: define a canonical edge parameter over an explicit segment range and either (a) require/verify same-parameter UV curves under that parameter, or (b) store an explicit bounded correspondence map for each coedge. Chart/period offsets and traversal orientation remain separate. The geometry compatibility check must test the declared correspondence across supported segments, not infer it from one bool or sample endpoints. State the treatment of degenerate/pole boundaries where ordinary curve parameterization fails.

This is a future Boolean/section-output requirement. Iteration one's normalized patch rectangle and derivative transformations already state their parameter contract and do not create edges.

Source basis: OCCT `BRep_TEdge` stores same-parameter and same-range flags separately from tolerance and degeneration; `BRep_Tool::CurveOnSurface` returns an explicit parameter interval. [Edge state](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRep/BRep_TEdge.hxx), [curve-on-surface range](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRep/BRep_Tool.hxx).

### T4 — High for future SSI, pending at this review checkpoint: transverse completeness versus deferred small loops

Roadmap stage four promises every supported transverse branch, while stage seven adds small loops among difficult cases. Transversality by itself does not impose a minimum loop radius or spacing. For example, the unit sphere intersected with `z=1-epsilon`, for any positive `epsilon<1`, has a transverse circular intersection of radius `sqrt(2*epsilon-epsilon^2)`, which becomes arbitrarily small as epsilon approaches zero. Finite mesh/sampling resolution can miss it even though there is no tangent point on the curve.

Required decision: stage four must support discovery/completion of all transverse closed loops under an explicit supported-family contract, including conditioning/separation assumptions where needed. If the implementation cannot establish those assumptions or exclude the remaining domain, it returns unresolved rather than empty; a whole-operation CPU fallback must retain that outcome if it also cannot resolve the case. Stage seven can broaden separation/conditioning/singularity coverage, but cannot be the first time loop discovery exists.

Source basis: the 2025 hybrid CAD Boolean slides explicitly list multiple loops within one triangle region as a limitation. The recent SSI studies emphasize starting-point and topology completeness rather than successful tracing alone. [Hybrid limitation, slide 37](https://yangjieyin.github.io/homepage/SIG2025/SIG25_slides.pdf), [SSI literature and access limits](2026-10-13-gpu-topology.md).

### T5 — Medium, future identity refinement: body namespace and result/history roles need a precise packed schema

Entity handles name a body, kind and 64-bit ID, while patch lineage lists parent face ID and source snapshot revision. The final ABI must carry the complete source identity or a globally unique namespace; a revision number alone is not a unique snapshot across independent bodies. Likewise, a Boolean combining bodies must define whether unchanged entity IDs retain their source namespace or move into the output body namespace, and emit a history relation covering that transfer.

The current prose already excludes cross-session promises and pointer-derived IDs, so this is not a numerical blocker. It becomes a blocker if an implementation serializes only the numeric face ID/revision and then assumes global uniqueness or interprets unchanged entity handles in another body. Define it in the implementation schema rather than relying on field labels.

### T6 — Medium, optional adapter claim: sampled correspondence is qualification, not a global extraction bound

The optional adapter checks positions and parameter derivatives against the reference. That is useful falsification, but finite sample agreement alone cannot certify correspondence everywhere after knot fixup or an interior parameter change. The adapter must label whether provenance refers to the original carrier or the materialized copy, retain the returned chart, and state what extraction correspondence its supported fixtures establish. If it claims a whole-patch geometric bound later, add a direct bound mechanism rather than promoting sample parity to that claim.

The draft is already correct to avoid `TrimWithDomain` as an assumed identity-preserving extractor and to separate analytic cylinder angle charts from NURBS charts. This finding asks for precise claim wording, not a general knot extractor in iteration one. [Knot-copy/fixup implementation](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmBSplineSurface.cpp), [trim warning](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmBSplineSurfaceNLib.cpp).

## Coherence checks that passed

- **IDs versus slots versus runtime handles:** distinct roles; no pointer/coordinate identity crosses the GPU boundary. Compaction rewrites dependent references and keeps stable IDs. Slot results carry their snapshot.
- **Ordered boundary occurrences:** loop order is canonical; CSR/ranges represent adjacency without an independently edited next-pointer graph. Seams may use one edge twice on the same face, with distinct UV occurrences. Open-boundary/nonmanifold support should be declared separately from the closed-manifold two-use rule when introduced.
- **Transactions:** edits build candidate storage, validate before publication, and leave old readers valid. Unresolved classifications/boxes and resource failures return diagnostics. Whole-operation CPU fallback or an explicit shared-event protocol avoids unprincipled coordinate welding.
- **History versus compaction:** compaction is a permutation of slots; split/merge/generated/removed history is a relation. The text does not promise every face name has one successor.
- **Actual public APIs:** shipped SMLib Python bindings lack direct control/weight/knot/split access. Public C++ APIs permit an experimental extraction seam; the design does not invent an evaluator callback or claim `boolean_with_curves` accepts supplied curves. Unmodified Boolean modeling remains on the CPU.
- **Source UV versus analytic chart:** first-patch provenance uses the returned NURBS rectangle, with nonlinear conversions outside the affine helper. Carrier sampling does not claim trim membership or a chord-qualified mesh.
- **Optional adapter versus first delivery:** owned analytical fixtures are enough to deliver the explicitly scoped numerical API. If the adapter fails, report that separately; no real-CAD ingestion or plate-modeling speedup is then established. This is coherent with the first-delivery promise.
- **Delivered value:** stage two introduces actual batched projection; stage three must emit bounded carrier/face meshes or conservative candidate pairs; stage five returns an editable solid under a narrow contract. Complete tasks include CPU packing, classification, transfers, validation and fallback time. Microbenchmarks diagnose placement but do not substitute for those outputs.

## Review disposition

T1 and T2 are resolved in the text re-read during this review. The first numerical iteration remains appropriately narrow and can proceed to an adversarially reviewed implementation plan once the other review lanes' blocking findings are integrated. T3 and T4 should be resolved in the future-architecture documents before claiming a settled section/Boolean design. T5 and T6 must remain explicit constraints when the packed ABI and optional adapter are specified.

## Final re-review disposition

Re-read the revised topology architecture, roadmap and first-iteration design after all six findings were integrated. **No remaining blocking finding in this review lane.**

- T1 remains resolved: spatial branch/segment keys are separate from oriented chart/face-use occurrences.
- T2 remains resolved: immutable queries retain an exact patch set; evaluation cannot substitute another set, and results retain their query/patch provenance and ownership.
- T3 is resolved: edges and UV curves have explicit stored ranges, generated edges use a common canonical `t` with declared maps, and unknown imported correspondence remains unresolved. Publication requires agreement along the bounded correspondence rather than endpoints alone.
- T4 is resolved: regular SSI includes interior closed loops under established family/regularity/separation conditions; unestablished assumptions or unresolved boxes fail the complete operation. Later work broadens tighter-loop/singularity coverage rather than introducing loop discovery for the first time.
- T5 is resolved at the architecture level: source lineage is body/revision/kind/entity plus chart, Boolean results use a new body namespace with explicit input-to-output history, and single-body unchanged entities keep their IDs. The implementation plan still must instantiate that tuple in its native metadata layout.
- T6 is resolved: sampled adapter correspondence is explicitly diagnostic rather than a whole-patch bound, and numerical qualification uses actual copied controls without inheriting source trim/topology correctness. Recipe-generated plate carriers carry recipe lineage rather than invented imported-face identity.

This approves the reviewed scope and relationships for proceeding to a concrete implementation plan. It does not certify an unimplemented adapter, guarantee every future SSI case, or establish a GPU modeling speedup. Full section/Boolean algorithm designs still require their own supported-family bounds, failure semantics and adversarial review when those stages are planned.
