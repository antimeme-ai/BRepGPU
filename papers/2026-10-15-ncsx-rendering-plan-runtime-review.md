# NCSX first-render plan runtime review

2026-10-15. Accepted `docs/NCSX_RENDER_PLAN.md` as a bounded source-derived
rendering caller over existing native operations. No new kernel, library or
as-built/alpha claim is selected. Source-only review; no implementation or
execution by this reviewer.

Three actual order30 centerlines, authentic inspected symmetries and18coil
instances are a coherent source contract. Four cubic-by-linear rectangular pack
strips per segment provide a cheaper explicit cosmetic cross-section than
rational tubes, while retaining native first-jet normals. Periodic rail endpoint
and tangent reuse avoids manufactured gaps. Adjacent rectangular strips have
intended sharp normals: check their seam positions, preserving separate normal
occurrences rather than demanding normal equality across a pack corner.

The separate136-mode/nfp3 plasma surface, retained mode convention and36x24
periodic bicubic interpolation are correctly identified as a display fit, not a
vacuum vessel or a validated equilibrium pairing. Sampled source-fit residuals
are separate from native evaluation grades and never imply a global fit bound.
The actual final control bits, not ideal pre-quantization spline coefficients,
are the independent expanded-polynomial oracle's inputs.

Two implementation clarifications were sent to root. State the periodic frame:
project the source toroidal direction into the centerline tangent's normal
plane, normalize it, and obtain its companion by a cross product. Reject zero
tangent, a radial-axis source point, or an unusably small projected direction;
do not silently choose a discontinuous fallback frame. This avoids introducing
closed parallel-transport holonomy/correction machinery for this first artifact.
Native position/first-derivative grades2^-40/2^-30 apply in the named normalized
control frame. Fourier residuals after the common scale4 conversion must name
source/world units rather than reuse unscaled numerical error grades.

Checking every actual control's homogeneous envelope, complete query and source
binding, finite/status/first-jet outputs and all periodic position seams is
appropriate to the actual caller claim. The generic depth-buffer renderer must
consume qualified native samples, keep per-patch sample layout explicit and
preserve current recipe/camera/light/palette/dimension metadata. Display-only
iterations may reuse saved jets while retaining separate numerical/renderer
source pins. A first actual image inspection still determines framing and
occlusion quality; no new general renderer or redundant test suite is needed.

Accepted before code with the two stated contract clarifications carried into
implementation. Authentic-source attribution, unsupported engineering detail,
cosmetic cross-section and goal-versus-current state must remain visible in the
README/artifact captions.
