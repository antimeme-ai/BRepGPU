# Revolution complete caller/reference/runtime source review

2026-10-15. Source-only review of examples/revolve_profile.py,
benchmarks/revolution/run.py and scripts/reference_revolution.py against accepted
design/plan and observed installed reference interface. No product execution.

RCR1: timed history consumption currently looks up source/target records and
compares each record to itself; the emitted source_t/source_uv/periodic affine
maps are not evaluated as actual consumer outputs within the useful wall. Full
serialization and independent map qualification occur outside. Requested a
bounded actual reader: evaluate each applicable map at deterministic parameters
appropriate to its entity role, retain source-coordinate/periodic outputs, export
those outputs after timing. This is useful source mapping, not another oracle.
Disposition pending root integration; other caller scopes accepted.

Actual source XY/world endpoints, Frame-origin positiveY axis and360-degree pure
reference operation are aligned. Before/after source curve domain/point probes
are present; input exact Fractions must survive binary64 endpoint conversion.
Foreign bounds shape/finite checks, manifold/volume and closest-point finite
residual/distance limits are explicit. Two interior witnesses per actual quarter
chart guarantee all four quarters per face rather than only aggregate face
coverage. Foreign history/material truth and alpha corpus completion are not claimed.

Grid9 gives actual3/6/16-edge different meridians and972/1944/5184 native rows.
Complete prepared-profile and points walls include CPU publication/request,
Engine/child cleanup, native frame/normal/scatter consumption and ABI5 actual
ring/same-face seam mate/neighbor reads. Independent qualification and export/SVG
are outside, with nested native/host timing rows not additive. Controller runs
fresh processes and excludes explicit warmups; full child wall includes launches
and all child work. Source and loaded-native hashes are checked before/after.

SVG reads actual exact ring/seam polylines and admitted native positions/normals,
with explicit wireframe/sample/nonmesh scope. Minor wording requested: 'Two
original seam uses' becomes 'Two distinct seam uses', because the source profile
has one original coedge that yields two output occurrences. No invented depth
or tessellation capability is claimed by this point/wireframe display.

Final source disposition: **ACCEPTED.** Root resolved RCR1 by evaluating all
applicable emitted affine source-t/source-UV/periodic maps at explicit exact
parameters for vertices, edges/uses, raw face/loop domains and normalized charts.
The actual original profile reader consumes the resulting source UV, and the
retained values go to history-map-reads.tsv after timing. This work is inside
material_boundary_history_consume and both useful walls; it does not replace
outside independent map qualification. Re-read the actual implementation and
export binding. Seam caption now says 'Two distinct seam uses'. No open finding.
