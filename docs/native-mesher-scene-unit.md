# Native semantic mesh to source display

Implement runtime-plan section7 with the existing v2 display consumer. The
adapter imports no mesh position from a source evaluator: every display corner
uses its stored physical-node XYZ and the required stored/cofactor flags. It
retains source face orientation only as the normal-reversal flag; final FACETS
already have physical winding. Original corner UV locators select analytic
carriers or indexed original rational roots. Derived display controls round
those original root coefficients once; they determine cosmetic normals, never
the stored mesh positions or geometric error admission.

The owned SceneStore retains NativeMesh/GeometryStore leases and packed record
arrays. Per-definition corner candidates sort/unique by physical-node reference,
carrier span, source face, exact UV words and normal flags, then prefix-fill
triangles. A dense corner-remap array replaces one heap map per displayed
vertex. Placed vertex/primitive references remain flat arrays. Finite/count
limits apply before allocation/narrowing. Normal/evaluation failure aborts the
required display; it never drops geometry. Pole sheet signs come from the
retained cap's original cut parameters under certified source trigonometry,
not the sign of a radius recomputed at rounded singular UV.

Source occurrence appearance needs one finite extension within the unmerged
source profile/API2: Triangle.material==UINT32_MAX means inherit the occurrence
appearance, and Instance.reserved is a1-based material override (0 absent).
Explicit original face styles keep their triangle material index. API1/profile0
rejects these values. API2/profile1 permits inherited triangles only when every
placed occurrence supplies a valid material. Triangle setup resolves the actual
material into its existing Prepared.material field, so CPU and CUDA shading and
all record sizes remain unchanged. This avoids duplicating geometry to display
two occurrences of the same definition with different source styles. Existing
source-profile scenes with explicit triangle materials and override0 stay valid.

The BGMESH01 writer emits explicit little-endian fields; BGVIEW01 writes the
existing request records. The same arrays feed the CPU v2 API or CUDA v2 module
directly. Native output publication writes the semantic mesh, scene, view
requests, display images, labels and stage TSVs under a temporary sibling
directory and renames only after the required operations finish. A failed run
cannot replace an existing successful result.

Direct checks use real NativeMesh producer output and literal binary/normal/
appearance answers. Every corner must preserve physical identity/XYZ; a wrong
source-position flag or extra winding reversal must fail. Two source occurrences
with different appearances share geometry and produce the expected two resolved
materials. CPU/P100 consume the same records; old-profile compatibility is checked
only for affected API/flag cases. No new image, geometry or compression dependency
is adopted.
