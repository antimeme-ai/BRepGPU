# Cached shared source curves and occurrence traces

Status: finite implementation design/plan accepted in scoped numerical,
construction and runtime peer review, with the concrete resolutions below.
Product units start with direct RED against these interfaces. This implements
checkpoint 2 of ADAPTIVE_MESHER_PLAN.md; trimmed
region construction, complete components and assembly publication follow it.

## Complete operation

Original SourceStore + one definition + positional and original-node allowances
produce one physical edge sample chain and every incident occurrence's selected
trace. Return actual XYZ, exact original parameters, original vertex/edge keys,
per-use UV/branch maps and whole-interval approximation bounds. The operation
must execute on parallel CPU and CPU+P100, with real curve geometry on the GPU.
It must not construct the previous eager GeometryStore/PreparedRun merely to
obtain source handles. A curve evaluator or a set of unassociated polylines is
not this delivery.

SourceStore coordinates are already mm. Positional allowance and the original
node-source cap are separate caller inputs. Original source tolerance remains
metadata. A definition-local operation makes no assembly-transform error claim.
All edges/uses of the requested definition are inventoried; an unsupported item
refuses complete publication rather than disappearing from successful output.

## Decisions and their sources

The three focused studies record actual reading scopes, source revisions,
mathematical assumptions and alternatives:

- papers/native-adaptive-curves-geometry.md: original roots, numerical bounds
  and actual point construction.
- papers/native-adaptive-curves-construction.md: shared occurrence traces,
  original/source-domain distinctions and seam/image correspondence.
- papers/native-adaptive-curves-runtime.md: cache and sole physical ownership,
  query reselection and CPU/P100 execution.

Pinned NVIDIA revision is c9c5979f04cea8c38213004d4f04df0dcfb40d05;
pinned OCCT revision is b8f597c677811d1f9f4d8a97f5ae2825c0353a42.
Reference restoration is in QUARANTINE.md. Primary mathematical sources include
Filip/Magedson/Markot 1986, Theorem 2; Balazs/Guthe/Klein 2004, original-domain
composition and bounded tessellation; and Marussig/Hughes 2017, competing trim
representations. Acquired papers/read scopes are retained under papers/.

| Implementation step | Grounding and owned choice | Direct check |
|---|---|---|
| Compile only live original spatial curves/pcurves | SmCurveCache span blocks; original Cox/de Boor/Bernstein basis. Use bounded polynomial-time extraction, not exponentially recursive high-degree basis evaluation or adapter span hints. | Independent original rational formulas, nonclamped/C0 knots and misleading adapter hints. |
| Bound the actual emitted chord | Positive rational Bernstein convex enclosure of N-WL divided by positive Wmin; analytic vector h² M2/8 from Filip Theorem 2 plus actual endpoint departures. | Exact Fraction whole-interval residual and analytic interpolation model; endpoint offsets remain visible. |
| Retain one physical owner | SmTess::TessellateEdge and OCCT EdgeDiscret::process/Tessellate2d share a physical schedule across uses. Reuse DefinitionBoundaryForest's existing owner/horizon. | Shared-edge and same-face seam occurrences reference identical physical keys/XYZ. |
| Retain original source meaning | Marussig/Hughes gap/overlap analysis; existing source-node and authoritative/reconstructed contracts. Bounds compare original V/Ce against original carrier at chosen UV. | Source mismatch and guide-speed cases; Q cannot replace original V/Ce in the guard. |
| Avoid speed-only refinement | Checked original homogeneous collinearity plus monotone scalar image; analytic-plane inverse and explicit correspondence. The image map replaces an incorrect affine-t assumption. | Nonlinear stationary-endpoint straight-line curve with differently parameterized reconstructed guide. |
| Retain distinct seam charts | Original occurrence topology and unwrapped pcurve coordinates; never weld by XYZ or normalize a stored binary64 angle into a symbolic period. | One vertical cylinder edge, two uses at literal u=0 and stored u=2pi, both shift=0. |
| Reuse numerical work across queries | SmCurveCache ordered subdivision summaries, existing common RN profile and scan/compaction runtime. Geometry caches persist; a fresh query forest gives canonical coarse reselection. | Fine/coarse/fine and fresh-cold coarse geometry, actual cache hits and GPU work. |

These are mechanisms and assumptions to study, not imported implementation code.
Each production unit records its exact source/formula, direct RED, implementation,
GREEN and actual-source review. A citation alone does not establish that a
reference stopping rule or sampled test satisfies our source/error contract.

## Geometry and trace procedures

Use a dedicated one-dimensional CurveSourceCache and typed curve descriptors,
not a surface descriptor with an invented zero dimension. Roots retain original
curve ID/dimension/span/domain and positive homogeneous controls or original
analytic frames. Original root and query restriction are distinct. The degree
ceiling and extraction complexity must match the existing finite source contract.
Sample from the canonical original root under rn64_anchor32_p128_v1; wider
selected-original fallback is explicit. Restriction follows its recorded cache
path. Actual endpoint source vertices stay fixed.

For rational C=N/W and the SAME actual chord L, elevate/multiply Bernstein
coefficients to enclose R=N-WL and use max coefficient norm divided by Wmin.
No scalar low-degree derivative inequality may silently admit higher degrees.
For analytic line/circle/ellipse, compile original-frame derivative constants
once; bound the interval by M2 h²/8 plus maximum actual endpoint departure.
Do not approximate original angular parameters by a new rational conic without
its explicit parameter correspondence. Numerical floors promote precision or
return typed finite exhaustion; minimum parameter size never forces acceptance.

Each physical interval must settle the required spatial chord and all incident
selected trace requirements before publication. Curve approximation uses the
positional allowance. Source-node residual remains original V/Ce versus original
S(chosenUV), with the original V/Ce endpoint guard separate; Q is absent from
that comparison. Approximation/rounding is charged to actual chord/facet bounds.
Authoritative pcurve domain approximation and reconstructed guide recovery are
distinct policies. This boundary checkpoint retains their maps and bounds; it
does not claim the full face's onto/orientation-preserving trim-domain map.

Checked collinear/monotone-image profiles use their stated scalar maps rather
than equal parameter speed. Per-occurrence UV remains separate from physical
sample identity. Full-circle seams retain literal source values and adjacent
occurrence-sector identity, including distinct occurrences on one face with
equal periodic_shift. Singular caps/quotients require their explicit source-map
procedure; unsupported profiles return a named finite outcome.

## Native ownership and integration

AdaptiveBoundarySession retains original source/definition and curve roots.
begin takes a qualified immutable exact table and per-face positional/node-source
allowances. step is nonblocking, result returns an immutable SharedCurveSnapshot,
and release_result/cancel/close follow existing healthy/failed-drain ownership.
Every incident use is obtained from original SourceStore CSR, not a caller's
optional subset. Results retain source and exact-value leases through all views.

Reuse DefinitionBoundaryForest as the sole physical owner. Instantiate a fresh
query forest on begin, retaining independent numerical roots/nodes/samples across
queries. Thus coarse selection does not inherit fine leaves or cache allocation
ordinals. Original edge parameter and source identity qualify retained samples;
ephemeral BoundarySampleSlot does not. Endpoint vertices are immutable. If a
constructed interior Q promotes, publish it through one validated physical-owner
revision transaction and invalidate all incident bounds/occurrences together.

NativeScheduler executes actual disjoint CPU geometry batches with private
scratch/stop contexts and canonical whole-horizon folding. P100 consumes typed
curve operations using the owned resident module and one pending ticket;
geometry restriction/evaluation/bounds run on device. Exact identity, source
topology and genuinely unresolved geometry remain explicit native work.
Count complete capacity before owner mutation. Report native/device reservations,
host-device traffic, failures and work without mixing them with network traffic.

Root owns CMake, complete source-to-boundary caller, output and independent
whole-operation reader. Numerical and construction owners own coherent source
contracts; runtime owns the sole authority/session and device lifetime seam.
The literal public field layouts and finite source procedures are selected in
the three focused reports and adaptive-curves-runtime-unit.md. Root owns the
separate device and caller unit plans; these reference the same numerical PODs
and source/result leases.

## Constructive review resolutions

All three scoped owners reviewed this plan and the caller plan against actual
source. Runtime and construction reviewed the numerical procedures. Root read
the actual reports, original owner and reference paths, and integrated these
concrete findings before implementation:

- Geometry uses a dedicated dimension2/3, degree0..32 curve cache. Original Cox
  extraction is a local Bernstein dynamic program; paired midpoint restrictions
  share one O(p²) pyramid. Parameter and proved monotone-image bounds remain
  distinct, and a valid image profile is tried BEFORE speed-only subdivision.
- Per-use carrier traces use direct original-span affine-UV restriction, degree
  at most6 for rational surfaces, or original analytic directional derivatives.
  No PatchSession or second surface-refinement controller is created per use.
  Tight coupled rational residuals and checked harmonic cancellation address
  loose bounds without changing the SAME actual chord.
- First recovery admits original guide-derived candidates verified against the
  original-node cap, with explicit plane and cylinder locators. Unimplemented
  general Newton/inverse recovery is not promised. Unknown map kind3 and failed
  recovery return named finite unsupported outcomes.
- Matching cylinder circles may use a checked CylinderAffineIdentity descriptor:
  original harmonic/frame/height equality and chosen angle literally equal to
  spatial t. General cylinder_arc_order is not admitted as a bare tag. Actual
  carrier-trace-to-chord and source-curve-to-selected-trace bounds are separate.
- Supported nonzero period shifts retain a symbolic true-period descriptor on
  the relevant analytic periodic axis and charge chosen dyadic-UV departure.
  NURBS/nonperiodic axes do not infer a period. Stored binary64 2pi is never
  replaced by symbolic2pi. Authoritative endpoint equality requires source
  descriptor identity, exact rational equality or a checked analytic identity;
  overlapping numerical enclosures are not equality.
- Each UseCurveInterval retains its BoundaryKey, authority, correspondence and
  actual bound components. Joint identity is loop-adjacent occurrence sector,
  not physical VertexID plus shift. Every source occurrence survives.
- Definition-default allowances cover wires; optional face restrictions cannot
  omit incidence. Add owner-only split requests to the existing forest so a wire
  needs no invented face. Logical physical keys remain distinct from constructed
  revisions. Stage guarded promotions and commit them atomically with split
  evaluations; original vertices stay immutable and all incidents invalidate.

These resolutions select procedures, not a performance result. Direct RED,
implementation and actual-source review remain required for each owned unit.

## Finite delivery sequence

1. Close source studies, exact geometry/session/trace interfaces and this plan
   under cross-review. Keep original mathematical derivations separate from
   statements actually read in a reference.
2. Write direct REDs against actual original source and missing native operations.
   Use independent authored shared-plane, weighted rational, C0/nonclamped,
   nonlinear straight-line, cylinder seam/circle and source-mismatch families.
3. Implement original curve geometry service, sole physical owner/session and
   per-occurrence trace construction along the reviewed dependency interfaces.
   Integrate the real caller; no fixture supplies absent production machinery.
4. Run affected original-output oracles and CPU1/parallel CPU/both-P100 parity.
   Scope host memory/concurrency and CUDA protocol/memory checks to these fault
   classes. Review actual source, fix concrete findings, stop repeated testing
   when the relevant checks pass.
5. Measure qualified complete cold and declared warm constructions with setup,
   output and close accounted for, preserving failures/censors. Publish the
   scoped result and continue to trim-region construction. No watch retry or
   extra certification/tuning campaign replaces that next dependency.
