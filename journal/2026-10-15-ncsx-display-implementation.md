# NCSX display unit implementation

Implemented the whole fixed-camera display in `examples/ncsx_display.py`:
complete contiguous tile/sample/unit-normal admission, explicit Z-up camera
basis, world-coordinate native-normal Gouraud colors, two-sided copper/teal
lighting, signed-barycentric opaque orthographic depth buffer and RGBA8 PNG
encoding. Native geometry is never regenerated or rescaled in the renderer.
One common fixed source view maps at270pixels/unit, center(900,650), az35/elev25.

render writes transparent surface.png, an embedded1800x1200 SVG with goal/current
state and source/fitting/engineering scope captions, and scene.tsv containing
camera basis/projection, light/shader, palette, tile/triangle layout, normalization
and source-space conventions. It returns the SVG Path. Coil colors name source
families numerically, without guessing A/B/C mapping. Depth uses largest camera
Z, consistent pixel-center coverage and one diagonal for every grid cell.

Static AST parsing passed without imports or execution. Root owns actual
geometry, source symmetry/fit checks, native qualification, source pins and fleet
render/image review. No new library, build, product/fleet execution or Git action.
