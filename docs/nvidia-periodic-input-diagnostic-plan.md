# Two-input NVIDIA periodic-chart diagnostic

Purpose: diagnose whether the adopted OCCT representation-preparation APIs let the
unchanged official NVIDIA converter consume the two retained failing inputs. This is
an ignored diagnostic executable, not a production exporter change or full-watch run.
All geometry/compilation runs on the fleet in a new context directory; no existing
producer, source file, SDK library, original STEP, allowance or definition is overwritten.

Actual source: NVIDIA c9c5979 `OcctStagingDriver::DriveFace` combines a 97-point
spatial-curve walk and declared vertices, then checks span>2π+1e-5. Fresh raw inputs
have no pcurves and small endpoint disagreement; both exceed that angular cutoff.
The three public converter entries and `occt_to_usd` have no topology-split setting.
OCCT b8f597c shape-healing guide§4.3.4 documents
`ShapeUpgrade_ShapeDivideClosed`, `SetNbSplitPoints(1)`; its
`ShapeUpgrade_ClosedFaceDivide::SplitSurface` needs finite UV bounds and seam pcurves.
Calling it on the raw missing-pcurve shapes is not the selected preparation.

One private driver reads BinToolsV4 into the original definition and creates source
face/loop/use bindings from its actual storage identities. Call the existing reviewed
`prepare_source_uv` with `prepare_query=false`, existing projection precision1e-7mm,
source residual.001mm and positional.005mm; use its RAW copied representation only.
Its physical source-coefficient guards remain active. No query guide/cap approximation,
ShapeFix_Shape, tolerance inflation or SetSameParameter shortcut is added.
Then use the documented `ShapeUpgrade_ShapeDivideClosed` with one cut, precision and
maximum tolerance explicitly bounded by the retained input topology tolerance (report
values), and the SAME copied source. Retain the reshape history. If missing bounds,
failed status, lost original-face descendants or changed source analytic basis
coefficients make the route unsupported, record that outcome rather than repair it.
Report original/prepared/split face/edge/vertex counts, source tolerances, face-history
coverage and perface periodic spans. Inspect surviving analytic/trimmed analytic basis
coefficients by value against the raw copy; new cut entities are explicitly new topology.
This is a source-linked diagnostic, not proof of general source equivalence/isotopy.

Serialize a NEW derivative BinToolsV4 file with no triangulations/normals. Invoke the
existing genuine NVIDIA producer on each input individually at chord.005 and heal0;
record conversion/import/tessellation/export status and counts. Preserve the prior
raw failures. Run no display, whole-watch process or parameter/source search. A pass
only establishes this named two-input representation route. A failure is the diagnostic
answer; no automatic fallback/healing escalation. Root reviews this plan before code.
Owned diagnostic files remain ignored context; tracked focused paper and journal retain
source functions, provenance hashes and actual outcomes. Root owns any later production
policy/API integration and complete-source reference rerun.

Reviewed diagnostic amendment: the initial split closing precision1e-7mm is smaller
than1716's declared input max6.2737832424940552e-7mm (its actual seam endpoint gap is
6.27377696872e-7mm). OCCT passes this precision into ShapeFix_ComposeShell::Init.
Root accepts ONE1716-only repeat using the retained input max as SetPrecision's
source-derived closing precision; SetMaxTolerance remains that same input max.
No original source tolerances/coordinates are reset or increased. Preserve the first
failure and the new-cut1e-5mm metadata disclosure. If still refused, stop this route.

Reviewed RAW-only serializer amendment (no extra inputs run): expose ignored driver option
`--raw-only`, taking the same input and NEW output path, serialize prep.raw_shape after
RAW source guards and BEFORE constructing ShapeDivideClosed. Report RAW projection/seam/
chart-approximation counters and preparation time, preserving original source inventory.
Polygon owner can then compare its same two fixed spiral inputs under identical original
SDK settings; this distinguishes missing source pcurves from later polygon export.
No new projector, query stage, chart splitting or global healing is added to that control.
Root aligns this short amendment with polygon-owner evidence before execution.
