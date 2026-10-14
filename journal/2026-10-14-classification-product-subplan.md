# Complete exact classification product subplan

Build one owned python/brepgpu/classification.py unit after root's actual red.
Root adds the lazy body entry points; this lane owns complete point/contact
semantics, arithmetic and immutable result ownership, not a file fragment.

Agreed values: PointClassification(snapshot,point,label,boundary_faces),
ContactVertex(position,left_uv,right_uv), FaceContact(left,right,carrier_relation,
dimension,relation,same_domain,normal_relation,vertices). All factory-only/frozen.
Functions classify_point/classify_points/contact_faces; UnsupportedContact.

Strict int/Fraction/finite float inputs become exact rationals before frame
subtraction/division. Closed canonical inequalities establish feasibility before
collecting original face IDs. Invalid point/iterator requests raise with no partial
batch publication. No native/GPU/filter or language/integration work.

For rectangular FaceRefs read all four affine homogeneous controls and exact frame,
confirm single nonzero distinct U/V axes and fourth-corner identity. Derive exact
closed world intervals, intersect them completely, classify dimension/carrier/
finite relation and oriented normal relation, then emit complete ordered convex
vertices with exact UV inversion on both original charts. Unsupported trim/carrier
families raise instead of emitting an empty relation.

Root serializes fleet direct green and compatibility execution. Independent
source-geometry oracles and adversarial code review attack full face/material
truth; fix valid findings before qualification/mutants/caller measurement.
No product code is written before root reports the healthy-environment red.
