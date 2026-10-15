# Same-world revolution: non-green foreign volume comparison

2026-10-15. Bounded investigation after the actual triangle foreign volume failed
the planned relative1e-8 comparison. No owned geometry change, threshold change,
GPU job, new dependency or native build. All reference execution used the existing
isolated fleet interpreter; local arithmetic used only saved exact/float records.

The actual input triangle is(64,-96),(192,-64),(112,128), divided by256, origin0,
scale1. Independently using its determinant area and triangle centroid gives
area53/256, centroid-radius23/48 and volume coefficient2*area*centroid=1219/6144.
This matches the owned exact pi coefficient. Analytic float volume is
0.6233075268108655. Foreign volume is0.6233075396566397: absolute difference
1.2845774111092112e-8, relative2.0609046992930012e-8. The retained planned absolute
limit is6.233075268108656e-9. **Volume comparison FAIL; overall comparison FAIL.**
The exact formula and actual input/source bits provide no evidence for changing
owned construction or its measure.

Pinned SmPyTypes.cpp's Brep.volume doc and SmApiQueries.cpp implementation clamp
relative_accuracy into[1e-8,1e-1]. Root's prior request1e-10 therefore cannot tighten
this public API; it returned the identical value. SmApiBrepComputeVolume computes
about the bounding-box midpoint through ComputePreciseProperties. SmBrep's core
sums per-face contributions; SmFace's nested integrations use area-derived error
scales and relative desired accuracy. An integration-control parameter is not an
independent guaranteed enclosure on final global volume error. The source and
observed result explain why a tighter request is ineffective; they do not prove
a unique numerical root cause or excuse our failed threshold. No alternative
accuracy setting, decomposition or reference API was tried to chase a passing
answer. [Public API](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html).

A bounded CPU-only same-input diagnostic allowed the already observed volume
failure to remain recorded while completing all other original comparisons.
Manifold=True. Original curve domains/three point probes remain unchanged.
Complete finite bounds pass with maximum error1.1102230246251565e-16. All54
boundary witnesses pass:30 exact physical-edge witnesses and24 actual native
interior witnesses(two in each of12quarter charts). Maximum foreign distance and
recomputed closest-point residual are3.1401849173675503e-16. Bounds limit1e-8
and boundary limit1e-7 are unchanged. These successful finite boundary queries
do not prove complete foreign material/history or a globally exact surface.

Root authorized one adapter fix: scripts/reference_revolution.py restores the
volume request1e-8, collects all original threshold outcomes, writes outcomes,
comparison, bounds, closest points, original curves, exact endpoint bits, source
pins and costs, then raises if any criterion failed. Malformed/nonfinite source,
domain, volume, bounds or query data still fail immediately. A real same-triangle
run of this actual adapter produced all8 artifacts and exited1 with 'reference
comparison failed: volume'. The outcomes are source preservation PASS, manifold
PASS, volume FAIL, bounds PASS, boundary PASS. No failed criterion is removed or
reported green. The owned48-run cost campaign is a separate operation and does
not become a foreign-reference pass. Alpha external-corpus qualification remains
incomplete. Pin additions are in revolution-runtime-sources.tsv.
