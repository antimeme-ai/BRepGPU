# Independent pair-scene source and enclosure review

Reviewed `python/brepgpu/pair_scene.py` as the complete body/selection/world-bound/
consumer unit against PAIR_DISCOVERY_DESIGN.md and PAIR_DISCOVERY_PLAN.md.
Reviewed SHA256:
`a21594a7cf7654b9cc76fa1469cf64bd11da2232e925bf303d882df5269d7e5d`.
Source inspection only; no product imports, native execution or fleet work.
**No implementation blocker found.**

`_gather` incrementally caps body, selection-key and chart inputs, admits exact
factory snapshots and rejects duplicate body namespaces and unknown/duplicate
namespaced face keys. It selects complete faces in body/original chart order.
Each source retains its exact snapshot and original patch slot/face/chart identity;
selected native patch-set slots do not replace those original source slots.
Construction failures precede scene publication.

`prepare_scene` requires a live CPU engine. It builds depth-zero AABB roots on
selected original patches using the existing qualified CPU builder, then
`_local_roots` checks actual download metadata/count/root uniqueness, patch-slot
association, whole-chart zero cell levels and leaf children. Both native children
use owned cleanup scopes. Empty selections skip geometry construction/arithmetic.
The oracle independently checks actual bounds against original controls; these
structural checks alone do not certify conservative geometry.

`_world_box` interprets actual local endpoint and frame binary64 values exactly
as Fractions. `_outward` rounds once, checks the represented Fraction, and steps
toward the required infinity only when rounded inward. Final containment and
finite/order checks prevent a narrowed or unrepresentable endpoint being
published. The factory frame's positive power-of-two scale preserves endpoint
order; origins remain explicit per source. Immutable scene boxes are the exact
records passed to index upload, while actual resident downloads support separate
qualification. Body topology remains on CPU.

`PairDiscovery` owns both indexes, retains exact scenes and borrows the engine.
Partial construction drains adopted resources. `_close_owned` attempts both
indexes, preserves the primary BaseException, retains an unconsumed/retryable
handle after failed close, and safely attaches secondary cleanup details.
The guarded page-child scope similarly prevents a cleanup failure replacing a
primary download/consumer error; successful output follows child cleanup.

`_page_copy` binds all named page fields to exact scene dimensions and requested
start/budget/backend/profile scope. It validates examined count/next cursor,
full-domain versus suffix/end flags, actual word extent/range and zero tail bits.
Frozen host metadata, tuple words and retained scenes survive public native
closure. `survivors` enumerates local set bits in order, maps start+local through
the retained right count and yields exact source objects; padding is rejected,
and no all-exclusion list is allocated. Its results are candidates, not contacts.

Direct tests now compare default multi-body selection to every original chart,
not only to the sources the consumer happened to return. This guards complete
input-domain coverage that result metadata alone cannot establish. Independent
original-control/Fraction and actual-box bit truth remains the geometric check;
source inspection and successful status flags do not replace it. General trims,
SSI, graph edits and Boolean operations remain outside this consumer.
