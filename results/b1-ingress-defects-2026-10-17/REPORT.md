# B1: ingress and admission defects

B1 is the fast-mesher campaign maneuver that fixes the four defects the
lifecycle map found by reading
([papers/2026-10-17-geometry-lifecycle-map.md](../../papers/2026-10-17-geometry-lifecycle-map.md)
§3.4). Its brief is
[docs/fast-mesher-campaign/briefs/B1-ingress-defects.md](../../docs/fast-mesher-campaign/briefs/B1-ingress-defects.md),
and its beads are `brepgpu-12s.1` and `.1.1`–`.1.4`.

**How it ran.**
- Build, tests and one corpus run were on the fleet host, in a lane directory.
  Nothing ran on the laptop, and nothing ran on OM10 bytes.
- The dispatcher authorised a read-only ingress run over 500 corpus files, as a
  before-and-after integrity check. Nothing was meshed.

**How it was written and reviewed.**
- The lane could not write this file itself, so the dispatcher wrote it from
  the lane's returns.
- An independent adversarial reviewer read the change twice:
  - **First pass, not ready:** 1 blocker and 4 should-fix findings.
  - **Second pass, ready:** 2 should-fix findings and nits.
- The lane fixed every required finding and re-ran the suites. See
  [Review](#review).

## 1. Void shells dropped (`brepgpu-12s.1.1`)

**Study.**
- `native/step_source/main.cpp` walked only `raw->Outer()`.
- OCCT does translate the voids: `StepToTopoDS_Builder::Init(BrepWithVoids)`,
  `.cxx:179-250`, reverses a void shell marked `.F.`.
- Our tables kept only the outer faces and wrote no diagnostic.

**Fix.**
- **Walk order:** the outer shell first, then each void shell in file order.
- **Orientation:** a face's orientation is its use in the solid, `same_sense`
  composed with the shell's orientation, as OCCT composes it. The reviewer
  checked this against `StepToTopoDS_Builder.cxx:224-246`.
- **Shell ordinal:** the `FACES___` reserved u32 now carries it (0 = outer,
  k = k-th void).
- **Compatibility:**
  - void-free sources keep identical tables;
  - the adapter revision string changes to v5;
  - readers older than this change refuse a void source explicitly.
- **Readers:** both readers (C++ `Face::shell`, Python `Face.shell`) require
  each definition's ordinals to run from 0 without gaps.
- **Refusals, each with a named message:**
  - a face that appears in two shells;
  - an OCCT solid whose shell or face count differs from the walked count (this
    also catches a void OCCT failed to translate);
  - a void with orientation `.T.` (ABSR WR4 requires `.F.`);
  - an oriented outer shell marked `.F.` (WR3).
- **Accepted with a record:** an oriented outer shell marked `.T.`, which OCCT
  reads unambiguously, gets a `source_oriented_outer_shell` severity-1 record.
- **Diagnostics:** one `source_void_shell` record per void.
- **Inspection:** `brepgpu_source_inspect` prints shell counts.
- **Oracles:** the Python oracles now know `BREP_WITH_VOIDS`, `CLOSED_SHELL` and
  `ORIENTED_CLOSED_SHELL`. They check each face's orientation and ordinal against
  the STEP text.

**Test.** `tests/fixtures/void-block.step`: a 10 mm block with a centred 4 mm
cubic void, written by OCCT 8.0.1. Volume is computed from the table faces by
the divergence theorem.

| | Faces | Table volume | OCCT volume | Outcome |
|---|---|---|---|---|
| Before | 6 | 1000 | 936 | `qualify_source` failed ("complete original IDs: definitions") |
| After | 12 (shells 6 + 6) | 936 | 936 | passes |
| After, void flipped to `.T.` | | | | refused by name |
| After, gap in shell ordinals | | | | refused by both readers |

**Downstream.**
- Python `imported_meshing` gives signed volume 936 on a `FACE_OUTER_BOUND`
  variant (1000 for the void-free twin).
- On the OCCT-written `FACE_BOUND` file it refuses with "expected one outer UV
  chart boundary". It refuses plain faces written that way too.
- The native adaptive model refuses the plain block exactly as it refuses the
  void block ("retained face trace no longer matches physical chain").

So the void is not the cause of either refusal. Both are old-mesher defects,
recorded and closed as retired (`brepgpu-12s.37.15`, `.37.16`). The fixture is
carried into the new mesher's authored families. The test pins the current
refusals and fails by design once a mesher accepts the block.

## 2. Invented parameter map (`brepgpu-12s.1.2`)

**Study.**
- Non-SameParameter coedges got a 9-sample straight-line t→s map labelled
  kind 3.
- Every native consumer refuses kind 3, but Python `_curve_knots_in_edge` still
  read its rows as split parameters.

**Reachability.** STEP input cannot reach this site today:
- With shape processing off, OCCT never clears SameParameter: `BRep_TEdge`
  defaults it to true, and the clearing in `TranslateEdge.cxx:354-358` is
  commented out.
- `CheckPCurve` clears it only when `read.stdsameparameter.mode` is set.
- `source_uv` already requires identity correspondence.

The site becomes reachable once ingress keeps the file's pcurves (operator
decision D-master).

**Fix.** A new `measure_source_parameter_map` in `native/source_uv` follows the
method of OCCT's `Approx_SameParameter` (`BuildInitialDistribution`,
`CheckSameParameter`, and the Newton step in `ProjectPointOnCurve`).

- **Projection:** points of the curve on the surface are projected onto the 3D
  curve.
- **Knot rows:** every interior knot of the 3D curve becomes a row at exactly
  its t, with s found by bisection on the projection.
  - A node within projection tolerance (divided by curve speed) of a knot is
    moved onto the knot.
  - A strict projection failure inside the bisection is decided by a projection
    onto the closed bracket.
  - The nodes must remain strictly increasing.
- **Residual:** measured at every node and at 1/4, 1/2 and 3/4 of every piece,
  against `source_residual_mm`. A failing piece is split. The residual is
  **sampled at nodes and quarter points, not a bound between samples**, and the
  code, diagnostic and messages say so.
- **Kind 4, not kind 2.** The map is published as a new kind 4, with rows of
  (t, s, sampled residual in mm). Kind 2 means exact original affine pieces, so
  publishing under kind 2 would have let a 1e-3 mm approximation be consumed as
  exact. Kind 4 is documented beside kinds 2 and 3 in
  `docs/adaptive-traces-unit.md` and `docs/native-mesher-geometry-plan.md`.
- **Readers:** both readers decode the residuals and require them finite and
  non-negative.
- **Consumers:** every consumer refuses kind 4 with the named message
  `measured_parameter_map_refusal` until it charges the residual to its error
  budget:
  - C++: `adaptive_traces.cpp`, `geometry_boundary.cpp`,
    `geometry_envelopes.cpp`, `adaptive_assembly.cpp` (two sites),
    `adaptive_caller_io.cpp`;
  - Python: `imported_meshing._mapped_parameter` and `_curve_knots_in_edge`.

  The reviewer searched every reader of the map kind and found no path that
  consumes kind 4 as exact.
- **Also fixed:** `_curve_knots_in_edge` mishandled kind 2 with several rows. It
  now maps each pcurve knot through its own piece, and every row becomes a split
  point.
- **Refusals:** the coedge is refused, named, with its residual, if:
  - a point lies off the curve;
  - a projection fails or is not monotone (the message reports the affine-seed
    residual and does not claim it exceeds the bound);
  - the node cap is hit.

**Tests.** STEP cannot reach the site, so the tests use authored native edges in
`tests/source_uv_fixture.cpp`:
- identity;
- P(s) = (s², 0), forwards and reversed;
- a pcurve offset 0.01 mm;
- `parameter-map-inflection`: g(s) = s + (s − 1/32)³ minus its chord, bound
  5e-6 mm. Zero error at the midpoint and 1.1e-5 mm at the quarter points, so it
  fails if the quarter samples are removed;
- `parameter-map-curve-knot`: a 3D knot at t = 0.4 must give the row
  (0.4, 0.3);
- `parameter-map-near-knot`: a knot 1.6e-10 from a node must give the single row
  (0.4, 0.25). The old code published a second row 1.6e-10 away.

Also: a `map4` fixture in `tests/native_adaptive_trace_fixture.py`, with
`native_adaptive_traces` checking that residuals are kept and the refusal is
named, plus Python tests for both meshing consumers and the multi-row kind-2
split.

| | Kind | Off-curve case | Offset case |
|---|---|---|---|
| Before | 3, for all cases | published silently | published silently |
| After | 4 | refused by name | refused: "residual 0.01 mm exceeds bound 0.001 mm" |

After the fix, an independent re-evaluation at 1001 points stays within the
bound.

## 3. Plane-angle unit unchecked (`brepgpu-12s.1.3`)

**Study.**
- Our code reads no raw angle. OCCT converts every angle per representation
  context before our code sees it. Lane M4-G6 reached the same conclusion
  independently
  ([papers/2026-10-17-units-parameter-semantics.md](../../papers/2026-10-17-units-parameter-semantics.md)).
- Multiplying by the declared factor anywhere in our code would therefore
  convert twice (the PrePoMax bug).
- The real risk is elsewhere. When a context has no plane-angle unit, or one
  OCCT cannot follow, OCCT silently assumes degrees
  (`STEPConstruct_UnitContext.cxx:281`).
- The Python oracle compared the raw semi-angle without applying the unit.

**Fix.**
- **Which contexts count:** for each solid, only the shape representations OCCT
  actually transfers through. That means one used by a
  `SHAPE_DEFINITION_REPRESENTATION`, or reached from one through representation
  relationships or mapped items. The search is breadth-first with a visited set,
  to depth 8.
- **Which unit is accepted:** exactly one interpretable unit, the SI radian or a
  conversion-based unit whose measure is in SI radians. The angle-reading mode
  must be File.
- **Missing unit:** refused only when the solid holds an angle-bearing value. A
  solid without one is accepted, with a `source_plane_angle_unit` "absent"
  diagnostic.
  - Angle-bearing values are a cone, a conic trimmed curve, a pcurve or
    rectangular-trimmed surface on an angular surface, or a surface of
    revolution.
  - The reviewer checked this list against every OCCT use of the angle factor.
  - This narrower rule is the default for operator bead `brepgpu-12s.66`.
- **Refused always:** a missing (with angle-bearing values), ambiguous, nested
  or uninterpretable unit.
- **Comparison only:** each cone's translated semi-angle is compared with the
  raw value times the declared factor, to within 1e-12. No stored value is
  multiplied. The comparison also catches OCCT's semi-angle clamp.
- **Unit-less contexts:** a context with no unit assignment is treated as
  declaring none. This is a conservative refusal backed by one fleet
  measurement. In that case OCCT read a cone in a unit-less B-rep context,
  linked to a degree shape representation, in radians. OCCT does carry caller
  factors on other paths (`STEPControl_ActorRead.cxx:1012-1014`, `:2340-2357`);
  the code comment says so.
- **Oracle:** the Python oracle resolves units per solid through the same
  representation graph, and recognises the full set of shape-representation
  subtypes.

**Tests.** `tests/fixtures/degree-cone.step` is a 30° frustum with two trimmed
circles, written by OCCT and edited to a DEGREE unit. Variants:
- a radian twin;
- a missing unit;
- a DEGREE defined through a GRAD conversion;
- an uninterpretable unit;
- a `DRAUGHTING_MODEL` that lists the solid with a radian context (it must not
  change the result).

| | Oracle | Missing-unit and nested-unit files | Stored semi-angle |
|---|---|---|---|
| Before | failed ("original carrier parameters differ") | accepted (exit 0) | not checked |
| After | passes | refused, naming the plane-angle unit | exactly 30 × 0.017453292519943295 |

After the fix, the radian twin's tables are identical to the degree file's, which
guards against double conversion.

**Not fixed here.** 41 corpus files write the degree as 0.0174532925. That makes
360° fall 7.18e-9 rad short of 2π, which is more than OCCT's 1e-9 closure
tolerance. These files are not falsely refused, because the check uses the same
declared factor OCCT used.

## 4. Two placement admission rules (`brepgpu-12s.1.4`)

**Study.** Placement admission was split between two rules:
- the display's source profile (`native/om10/validation.hpp:177`) accepted any
  affine instance with positive determinant;
- both readers refuse scale and shear.

**Fix.**
- **One predicate,** `brepgpu::rigid_placement_status` in the new
  `native/rigid_placement.hpp`. It is today's reader rule, unchanged: Gram
  entries within 1e-9, and a positive determinant.
- **Callers:** the C++ reader and the display's source profile call it.
- **Python:** `imported_source.rigid_placement_status` mirrors it, with the same
  operation order.
- **Scope:** widening admission to mirrored and uniformly scaled placements
  (D-placements) is M8 work, under bead `brepgpu-12s.25.4`.
- **Remaining difference:** the display checks composed world matrices, while
  the readers check each usage. Tolerance can accumulate so that the display
  refuses what the readers admitted. That is a refusal, never a silent accept.

**Test.**
- `tests/fixtures/rigid-placements.tsv` holds 11 matrices. The native test
  `native_placement_admission` (reader and CPU display) and the Python test check
  them against the same expected column. A refusal counts only with the message
  "nonrigid instance matrix".
- The STEP case is `source-uv.step` with one occurrence replaced by a scale-2
  `CARTESIAN_TRANSFORMATION_OPERATOR_3D`.

| | Outcome |
|---|---|
| Before | the display admitted 4 matrices the readers refuse (Gram just outside tolerance, scale 2, slight scale, shear) |
| After | all 11 agree, and both readers refuse the scaled STEP occurrence |

## Suites (fleet lane directory, final)

| Suite | Result |
|---|---|
| `test_step_ingress` (new) | 22/22 OK. Red against the first-pass build: 16 of 21 failed |
| `test_source_uv` | 8 OK |
| `test_imported_source` | 5 OK |
| `test_imported_assembly` | 5 OK |
| `test_om10_scene` | 2 OK |
| `test_imported_geometry` | 6 OK |
| `test_imported_meshing` | 35 OK |
| `test_imported_domain_sweep` | 8 OK |
| om10 ctest, CPU and both P100s | 3/3 |
| Native mesher ctest | 127/129 |

The two native mesher failures, `native_adaptive_trimmed_cylinder_seam_cpu1` and
`_cpu4`, also fail on an unmodified copy (bead `brepgpu-12s.37.11`).

Commands, with paths relative to the lane directory:

```
cmake -S native/step_source -B build/om10-step-source -DOpenCASCADE_DIR=<pinned OCCT 8.0.1 install>
cmake -S native/mesher -B build/native-mesher -DCMAKE_CXX_COMPILER=/usr/bin/clang++-18 && ctest -j16
cmake -S native/om10 -B build/om10-release -DBREPGPU_OM10_ENABLE_CUDA=ON -DCMAKE_CUDA_COMPILER=<cuda-12.8.1>/bin/nvcc && ctest
PYTHONPATH=python:examples:tests BREPGPU_FACE_DOMAIN_LIBRARY=<face-domain build> python3 -m unittest tests.<suite>
```

**Corpus ingress run.** Read-only, over the 500 STEP files from lane M4-G6's
census.
- The adapter before and after the change: both 230 accepted and 270 refused,
  the same files for the same reasons. Zero new refusals.
- The three files containing `BREP_WITH_VOIDS` are refused earlier ("leaf
  product does not bind one source solid"), so the void path does not run on
  them yet.
- Top refusal reasons, unchanged:

  | Reason | Files |
  |---|---|
  | inch length units | 140 |
  | source-UV residual | 43 |
  | nonplanar multiple-wire faces | 30 |
  | non-mm parameters | 14 |
  | leaf-product binding | 13 |

  These belong to the lifted reader that lane G6 specifies, and to beads under
  `brepgpu-12s.37`.

`scripts/qualify_om10.py` was not re-run with the changed oracle. The OM10
source study records all its contexts in radians.

## Review

| Pass | Verdict | Findings |
|---|---|---|
| First | not ready | **Blocker:** the measured map was published as kind 2, so it would have been consumed as exact. **Should-fix:** the residual is sampled, not bounded, and the tests could not tell; a `.T.` void was accepted; the unit resolver refused legal input (missing unit on planar solids, presentation contexts); no consumer was run on the void fixture. Nits: test message checks, a misleading refusal message, Python oracle parity, comments. |
| Second | ready once this report is updated | **Should-fix:** the knot bisection could refuse when a knot lies within tolerance of a node. Nits: the comment on unit-less contexts claimed more than was measured; the representation search had no visited set; the `.T.` outer shell was accepted with no record; two C++/Python parity gaps. |

All required findings are fixed, as described above. One optional item is not
done: testing the kind-4 refusal message at a fourth consumer site. The other
sites refuse kind 4 through their existing "not kind 2" branch regardless.

## Further findings

Filed under B2 (`brepgpu-12s.37`):
- `.37.11`: the cylinder-seam ctest pair fails on unmodified code.
- `.37.12`: the independent oracle accepts only `ITEM_DEFINED_TRANSFORMATION`
  placements.
- `.37.13`: stricter refusal-only placement rules remain elsewhere.
- `.37.14`: mesh a void-bearing source end to end.
- `.37.15` and `.37.16`: old-mesher refusals of the plain block, closed as
  retired.
- `.37.4`: the uncertainty is taken as the maximum over all contexts.

Passed to lane M4-G1 as a study item: OCCT appears to read a scale-2
`CARTESIAN_TRANSFORMATION_OPERATOR_3D` in an assembly relationship as scale 0.5
(`STEPControl_ActorRead::ComputeSRRWT`, `.cxx:2495-2527`).
