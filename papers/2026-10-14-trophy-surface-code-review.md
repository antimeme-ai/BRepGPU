# Orbital surface caller code review

2026-10-14. Static review of `examples/trophy_surface.py`; no product import or
execution. The actual geometry construction is accepted. Thirteen torus
carriers produce208 degree(2,2) patches and60,112 dyadic samples at n17. The
display consumes53,248 quadrilaterals, not the design's optional triangle split.

The tensor controls correctly multiply independent homogeneous circle nets,
pack v fastest and translate by center*W. All weights are1..4. The largest
possible translated control component is21/32; the source also asserts every
actual control against the native unit-cube/weight envelope. All affine axis
permutations are proper cyclic rotations. The exact analytic torus position,
du and dv expressions and Fraction conversion at actual submitted parameter
bits are correct. Every requested output component meets the existing grade
check; nonfinite/status errors abort. Lighting uses normalized cross products
of the returned native first derivatives. Both u/v seam index calculations
correctly match adjacent quarter charts including wraparound and compare
position/oriented normal rather than unequal raw parameter speeds.

The recipe is spatially bounded and each ring has R>r>0. Orthographic camera
rotation affects display only. Depth-sorted sampled quads may have visibility
artifacts at overlaps; the emitted report correctly calls the result a surface
assembly without fusion, trimming or a chord bound. Exact controls/recipe and
raw native structure-of-arrays jets are retained, with dyadic query ordering
specified explicitly. No new production/native implementation is involved.

Findings sent to root before fleet execution:

- Require returned query count, order, slots and binary64 UVs to match the actual
  submitted request, and verify exposed returned descriptors against original
  degree/control-offset/count/frame/UV/lineage records. Excess samples or wrong
  retained metadata must not pass while the binary export header asserts the
  requested extent. Original control bytes are not returned by this ABI, so the
  retained emitted input controls remain the explicit source for those bytes.
- The SVG says P100 even when the CLI selects CPU. Derive its backend/device
  label from actual invocation/output metadata or remove the unsupported CLI
  alternative; do not publish a CPU render labelled as GPU execution.
- The analytic truth shares the small quadrant/axis transform helpers with the
  control builder. This still independently checks rational tensor evaluation,
  but could share an assembly-orientation mistake. Separate literal analytic
  quadrant/permutation formulas or original recipe anchor/basis witnesses would
  strengthen the actual orientation/placement claim without a new test harness.

Follow-up inspection accepts all integrated changes. Returned count, order,
slots/UV buffers and exposed original patch degree/control offset/count/frame/
flags are checked before consumption. The SVG receives the actual selected
backend/device label. Independent analytic quadrant bases and explicit axis
index tuples replace shared control-builder transform logic in the expected
torus maps. The caller is accepted for fleet execution with no remaining
source finding. Actual numerical and artifact outcomes remain execution evidence.

## Actual visibility correction

The fleet numerical batch subsequently passed all60,112 samples: reported
maximum position/du/dv errors5.52e-17/1.69e-16/1.04e-16, seam position error0
and unit-normal seam error2.86e-15. The initial image exposed mean-depth polygon
sorting errors at crossing carriers. The updated display plan was reviewed
before the replacement rasterizer was implemented.

Source inspection accepts `surface_png` and updated `render`. Each of the53,248
v-fast sample quads splits consistently into two triangles. Barycentric weights
use signed projected area and therefore handle either winding. Pixel-center
sampling, integer bounds from ceil(min−0.5)/floor(max−0.5), framebuffer clipping
and negligible-area rejection bound the raster work. Depth is linearly
interpolated in the orthographic projection; greater view-z replaces initially
negative-infinite depth. Vertex colors derive from the actual checked native
normal arrays, with Gouraud interpolation and clamped byte channels. No analytic
normal is substituted, no geometry coordinates are regenerated, and no visibility
ordering depends on a triangle's mean depth.

The encoder emits PNG signature,13-byte IHDR for RGBA8/noninterlaced data,
one filter0 prefix per scanline, zlib-compressed IDAT and IEND. Chunk lengths
and dimensions use network byte order; CRC32 includes chunk kind plus payload.
Unused pixels remain transparent; covered pixels are opaque. The complete raster
is saved and embedded in the labeled SVG without losing the independent SVG
background/captions. Display source is accepted; actual updated image review
remains required. The render can consume the already qualified saved jets.
Preserve the numerical-run source pin and separately identify the later renderer
source when those file versions differ. No native rebuild or new product claim
is justified by this renderer correction.
