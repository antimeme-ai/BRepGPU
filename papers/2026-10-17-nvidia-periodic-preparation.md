# Source-preserving periodic input preparation

The two original NVIDIA conversion refusals are resolved with a named copied OCCT
representation. The SDK, input CAD, requested chord height and original entity
tolerances are unchanged. This is reference-boundary preparation, not owned meshing
or independent whole-source distance qualification.

## Mechanism and source

Pinned OCCT b8f597c shape-healing guide§4.3.2 and
`ShapeUpgrade_ShapeDivideAngle::InitTool`/`ShapeUpgrade_SplitSurfaceAngle::Compute`
select angle subdivision of the actual finite U interval. MaxAngle=pi uses the literal
OCCT segment-count formula including Precision::Angular; it does not promise exactly
three charts or each width mathematically<=pi. The earlier closure-based divider did
not split the offending cylinder. Existing guarded `prepare_source_uv(query=false)`
provides RAW pcurves before subdivision; later query/pole changes are excluded.

`native/reference_periodic` retains original surface/curve basis values, source
vertex XYZ/tolerances and original face/edge senses. History covers every original
face and every prepared face. Original edge descendants form a directed chain through
the mapped original endpoints. Same-basis periodic curve ranges are compared as exact
dyadic atomic intervals modulo public Geom_Curve::Period(), including multiplicity.
This uses OCCT's nominal API period, not an assertion that stored2pi is the mathematical
period. Nonperiodic ranges require exact linear coverage. Wider-than-one-period input
and unsupported basis/history refuse. Distinct original entities cannot silently merge.

The actual cone exposed one-step nonperiodic endpoint changes by OCCT subdivision.
The tool restores only exact original3D endpoint words at history-identical original
vertices on the same nonperiodic basis. Pcurve ranges/coefficients remain unchanged;
SameRange and SameParameter are set false on these derivative edges.
`BRep_Builder::Range(...,Only3d=true)` alone does not update those flags. All edits are
listed in range-restorations.tsv; exact image and directed incidence guards still run.
No approximate interval comparison or acceptance threshold was introduced.

`ShapeExtend_WireData(wire,false)` supplies a connected source binding word where
storage order is not connected. Its full edge-identity/sense multiset must equal the
storage inventory, including repeated seam occurrences. RAW's directed-cycle guard
remains unchanged. This changes adapter traversal, not source wires.

## Actual original-input results

Input definitions1716/1811 are diagnostic input selections, never product dispatch.
Official SDK release archive SHA256a30bcd6f1c9028bd29092288e0963d5450ab2b9105fc92a034a6c8ce9d8de2b3,
build1.0.0.v1-0-0.7419.dbb624c8, unchanged original producer
SHA2564406fce4f9efb89d305e41de26d4b86be6f0b34f52433e98ee453dcaa60ffcd3.
See the prior usage diagnosis for full archive/library provenance.

| Original input | Original→prepared faces | Prep main elapsed seconds | Genuine triangles | SDK definition seconds |
| --- | ---: | ---: | ---: | ---: |
|1716|6→11|0.0106204|424|0.077342434|
|1811|117→122|0.115486|2096|0.772496446|

Both heal0/chord.005mm runs import/tessellate/extract/write completely, input/output
manifold1, lamina/spine0. These SDK calls preserve the old omitted angular settings;
root's complete pipeline separately uses its explicit25/25 policy. Preparation times above are native main elapsed, excluding process launch; the
root worker records whole subprocess wall separately. These are bounded controls,
not complete-watch timings. All failed predecessors remain in context.
Final added orientation guards produce byte-identical prepared BReps to these actual
SDK inputs:1716dc150a109f69d16c3b4280962a2502d169ad55bd015a719088374dc25b63538e;
1811b50412c8b84486b1f8575d46519e660f010d60e0631b34ff92b89da75a6e1360.

Original vertex/edge/face tolerances remain equal.1716 generated cut edges carry
1e-5mm construction tolerance, explicitly reported even though this exceeds the
source-derived divider MaxTolerance setting. Original maximum remains
6.2737832424940552e-7mm.1811 original maximum1.5649075974457773e-4mm remains unchanged;
48original-line endpoint ranges are restored with both equality flags invalidated.
These metadata changes are named; a small cut tolerance alone is not distance proof.

Reports are source-history.tsv (original and prepared BinTools indices, serializer
indices, directed child order and mapped endpoint indices), tolerances.tsv,
range-restorations.tsv, stages.tsv, metadata.tsv and status.tsv. Face ancestry is
never inferred by equating original face IDs to SDK face ordinals.

## Scope and next consumer

The standalone tool uses the existing adopted OCCT/reference bridge and owned exact
arithmetic, no new dependency. The complete reference worker tries original official
conversion first and invokes preparation only on conversion failure. It retains failed
conversion/preparation/reconversion costs and applies its separate explicit Plane range
policy before SDK import. Root owns that worker and full-watch data. Authored source
controls and precise source-review outcomes are recorded in the unit journal.

Authored result scope: seven cylinder/cone periodic/partial sources pass the same tool
and independent Fraction history reader, including shifted U, proper reversed axes and
source-tolerated endpoint offsets. Three produce complete genuine SDK mesh exports;
four shifted/reversed cases pass SDK import+tessellation then refuse existing raw
nontriangle export even after explicit Plane preparation. These are reported as
IMPORT_TESS_PASS/EXPORT_UNSUPPORTED, not complete results. The two actual original
inputs above do fully export. Mutated radius/vertex cases refuse without prepared
output; three existing reversed seam/pole/source guard methods remain green.

## Follow-up1808 physical topology diagnosis (no repair selected yet)

Attempt9 successfully exports all107 definitions but existing physical topology admission
fails1808 only. Old attempt8 heal0/1 both have8816physical vertices/17632triangles and
no invalid edge pair. New heal0/1 both have10082vertices/20164triangles and exactly
three physical endpoint pairs with four uses each. SDK reports12spine halfedges and
outputManifoldfalse before raw extraction (`SmBrep.cpp`37603–37616); the SDK status
and zero per-face failures alone do not imply closed output.

Pairs are SDK face ordinals597/592,554/549,509/506, not original face IDs. On the first,
top Plane triangle(6967,6965,6966) and curved-wall triangle(6967,6966,6965) have identical
physical keys/opposite winding. All points lie atY=.27; wall corner normals haveY=0,
so its triangle cross product is orthogonal to every wall normal. The other two pairs
show the same structure. This is a boundary-fan topology fault already present in
returned SDK output, not evidence of a raw physical-key welding error.

Falsified explanations: original/corrected USD have1112general trim tags, zero rectangular.
New producer on ORIGINAL unwidened USD at.005/25/25heal0 produces exactly the same
mesh bytes as widened input (SHA6ec083ca8489001b413f9718ee0308bbf183d4e5eb6e26f2167ecf86aba599cf).
Thus full-natural-curve Plane hulls are unnecessarily huge but do not cause this fault.
Angular controls25/0 and0/25 separately still produce12spines; disabling only one does
not repair. Disabling both is not selected as a quality fix. USD has no serialized
2D pcurve attributes here, so RAW pcurve preparation is not selected as a blind control.

Root selects one bounded public-API advancing-front control under unchanged enabled
quality constraints. Literal args/status/cleanup and source grounding are appended to
the existing unit plan. No product change, welding, triangle dropping or cap relaxation.

The AF control exactly preserves wrapper smoothingfalse/attributestrue/sharedcontext,
quality parameters and strict partial-output disposal. Parent separately selected a
full legacy0/0 chord-only baseline at the same.005mm with unchanged preparation and
closed-shell gates. That baseline must be reported under its own parameter policy;
it does not convert the failed25/25 output into a success or establish distance
qualification. No production retry/body is selected by the diagnostic alone.

The single public advancing-front control was externally censored at its180s limit,
without a completed definition report or exported mesh. It is neither a native FAIL
nor a successful topology repair. Exact CPU/wall and timeout status were not retained
because GNUtime was inside the terminated process group and the outer shell ran later
cat commands; that shell's zero exit is not a native result. No further AF control or
production retry is selected. Source/raw/SDK gates remain unchanged.
