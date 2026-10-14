# Independent canonical-body topology code review

Reviewed `python/brepgpu/body.py` against BODY_SAMPLING_DESIGN.md and
BODY_SAMPLING_PLAN.md. Reviewed source SHA256:
`47d07b392eb0c858de7b0d483524a9dce3708d96634ca7722b865a57a08111a9`.
Source inspection only; coordinator owns actual fleet execution.

No implementation blocker found in this bounded factory/topology unit.

- `Frame` (line 52) copies its origin and admits only finite bounded offsets
  and the five positive power-of-two scales. `make_body` publishes only tuples,
  frozen records, existing tuple-copying Patch values and packed bytes. The
  factory-only snapshot prevents ordinary caller insertion of arbitrary graphs.
- `_namespace` (line 224) serializes monotonically allocated 64-bit IDs and
  rejects exhaustion. Failed construction may consume an ID, without recycling
  or publishing a partial snapshot. Dense slots remain snapshot-local.
- `make_body` (line 281) shares twelve box edges between face uses. Cylinder
  rims each remain one closed edge and the wall one face/four charts. Both
  wall seam occurrences refer to the same spatial edge at atlas a=0/4.
  Reversing wall uses for the plate gives inward hole-wall normals and opposed
  incidence with cap-hole loops. Face orientations match the declared plane
  maps; chart joins need not have matching parameter derivative speeds.
- The wall homogeneous controls represent the declared rational quarter circle
  exactly: denominator 1+s², X numerator r(1−s²), Y numerator 2rs, with affine
  height. Plane nets are exact dyadic corners. Native patch slot equals chart
  slot and each patch retains its body/face/chart lineage and frame.
- `classify` (line 198) checks local UV before arithmetic and uses exact integer
  ratios for disk/hole signs. Cardinal disk UVs are boundaries; internal wall
  joins are interior except at height endpoints. Rounded plate-hole UVs are
  classified by their actual bits, consistent with absence of an exact binary
  hole-boundary witness. No spatial tolerance replaces trim membership.
- `_packed` (line 240) writes declared little-endian formats with record-based
  offsets/counts, deterministic typed tags and zero geometry padding. Logical
  and exported controls share the same order. Body frame is in the header;
  body/face/chart lineage is reconstructible from the declared typed tables.
  These are CPU exports, without a native/GPU topology or loader claim.

The independent oracle additionally binds complete rational edge/UV identities
and outer-loop domains to actual native control nets; it checks every packed
record against logical values. Finite sample geometry alone does not establish
that topology. This review covers the three constructed analytic fixtures;
mesh connectivity, general trims, import, topology edits and Boolean algorithms
remain outside the implementation.
