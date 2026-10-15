# Selected box-edge fillet: runtime, source and reference study

2026-10-15. Research only. Read current doctrine/AGENTS, actual canonical box
records/carriers, feature history/samplers/callers, acquired NVIDIA/OCCT/Truck
sources and live public documentation. Read-only fleet introspection confirmed
installed usd-brep1.0.0 signatures; no geometry was constructed or mutated,
no test/build/dependency was run/adopted.

## Selected complete operation

Root selects fillet_edge(source,source_edge_key,radius): actual factory canonical
box, ONE full original vertical edge among IDs9..12, local exact1/256-grid radius
in[1/16,1/2]. The source box has XY half-width3/4 and Z endpoints±1/2, original
Frame and62 six-kind entities. Edge must carry the source namespace/revision and
actual edge kind/ID, not merely a coincident line. Validate full immutable source
records before creating output; reject invalid type/foreign key, duplicates,
unsupported nonvertical selection/radius without repairing or consuming source.
No chaining, all-edge filleting, arbitrary spine, variable radius, concave blend,
three-edge corner patch or mesher is selected. This is a genuine solid feature,
not a surface preview or a substituted rounded primitive.

At the selected corner signs(sx,sy), put c=3/4-r. Replace the convex XY corner
with a tangent quarter circle centered(sx*c,sy*c), extruded over the COMPLETE
original Z extent. It joins the two original side planes and two caps; there
are no floating fillet ends requiring sphere/rolling-ball end patches. The
section has five vertices/edges, so output has10 vertices,15 edges,30 uses,
7 loops,7 faces and7 charts=76 entities, connected genus0 shell. Original corner
vertices/vertical edge are removed; exact tangent endpoints and new cap arcs/
tangency lines must retain precise original ancestry and ordered incidence.
Tangent joins are distinct physical edges even when normals agree.

Keep all six original plane carriers' control/UV/Frame bits, with fresh output
Lineage and changed exact cap/side trims. The untouched source remains retained
as source. Native carrier-domain admission does not equal trimmed-face membership:
cap corner material is removed; adjacent side rectangles shorten. Their original
UV thresholds involve2r/3 and generally are not binary64. Do not rebuild those
carriers to conceal trims, snap sample coordinates to mathematical boundaries,
or classify exact Fraction queries after converting to float. Native requests
copy actual binary64 UV before exact admission, as the existing feature wrappers.

New fillet carrier has degree(2,1), one rational quarter by the existing exact
homogeneous triples(A,B,W)=(1,0,1),(1,1,1),(0,2,2), signed/permuted to the selected
corner. For each angular control and original Z endpoint emit
(cx*W+r*A,cy*W+r*B,z*W,W), with the appropriate rotation signs. Centers, r and
original Z are dyadic; all numerator bounds remain within3/4*W, allweights1/1/2,
so no fitting, sqrt-weight rounding, new native tag or native build is required.
Quarter parameter is rational, not angle/arc length. Native angular×Z derivatives
supply an outward normal when the perimeter arc is CCW. Exact source-aware
normal truth derives from rational circle components and the face orientation,
including plane/cylinder tangent limits, not CPU/GPU parity.

Exact material: first enforce original box XYZ bounds; for this selected corner
let X=sx*x,Y=sy*y. Only if both X>c andY>c require
(X-c)^2+(Y-c)^2<=r^2. Sign/quadrant guards are essential: a full disk intersection
would remove valid material elsewhere. Return every supporting output face,
including both tangent planes/cylinder where appropriate. Local volume is
(9/4-r^2)+(r^2/4)*pi; multiply both coefficients by Frame.scale^3. Tight bounds
remain the original box bounds because unrounded corners/side extrema survive.
Both cap arcs and side tangency lines must agree as complete exact geometry with
original carrier UV maps and new cylinder, not just a few endpoint probes.

## Native path and failure/caller scope

Separate immutable fillet result/solid/request/sample factories preserve old
canonical/Boolean/extrusion/revolution guards. Existing ABI1 Patch descriptors,
per-Patch Frame conversion, full copied-UV/Lineage/status checks, oriented rows
and child scopes fit the actual new result.solid patches/charts/faces contract.
Existing generic Boolean metadata/row helpers can be reused unchanged where
fields match. Current typed ABI5 explicit TopologyInput needs no rebuild; retain
the actual result and read all15 paired edges/30 mate-neighbor occurrences.
Seven charts make defaultgrid17 only2023 candidate rows, some rejected by exact
cap/side trims. A chosen grid9 gives567; record actual admitted counts. Do not
replicate patches or increase grid just to manufacture a GPU-sized batch.

Bound arbitrary request iterables at cap+1 and complete chart grids before
allocation. Validate constructor geometry/topology/history privately, publish
once after complete success and preserve source/prior outputs on failure.
Sampling borrows Engine, owns patch/query/result children and releases them
under late status/provenance/normal/host exceptions; no partial batch or hidden
fallback. Retry with the same Engine/result must work. Optional unprofiled kernel
timing stays absent, not an invented number. Construction remains CPU-owned;
old protected binaries remain unchanged.

The complete actual caller should read source and result geometry, exact
material/edge/UV maps, all forward/reverse history and evaluate emitted ancestry
maps at real parameter domains inside its prepared-source→consumed wall. Publish
source-creation→consumed and full child walls separately. Include Engine/context,
packing, transfers/evaluation/download/frame/normal/scatter, cleanup and actual
ABI5 construction/readers. Keep independent whole-feature/native qualification,
artifacts/SVG and optional foreign execution outside those useful walls; nested
phase medians are nonadditive. Use three actual edge/radius/Frame combinations,
including min/maxradius and different selected corner signs. Graph size is fixed;
this is a small-feature comparison, not model scaling. An actual cap-arc/tangency
wireframe plus admitted native samples can display the rounded edge clearly;
it does not supply a result mesher or imply mesh occlusion correctness.

## Actual foreign API: mutating selection, not pure sweep ownership

Live installed fillet_edges(brep,edges,*,radius,xsect_type=1,continuity=1,
thumbweight=1) mutates and returns the SAME Brep. Integer1 selects circular;
FilletXSect enum values have a different interpretation for this particular API.
Use source.copy() for a fresh deep-copy work Brep, enumerate/select the edge on
THAT copy, and call the mutator. Do not pass original Brep-owned handles to its
copy. Selected borrowed edge/face/curve handles can become invalid after mutation;
retain scalar before probes and reacquire result handles afterward. Probe the
untouched original separately before/after to establish the selected retention
scope. Source SmApiFilletEdges delegates SetFilletParameters/DoFillet, then
StitchAndOrient; no transactional rollback guarantee was found. Isolate failed
foreign mutation and discard the work body. [NVIDIA API](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html).

Actual Edge.vertices() returns start/end Vertex handles and Vertex.point() reads
coordinates; Edge.parameter_range()/point_at() are public. There is no verified
Edge.start_point() or Vertex.position() method. Use endpoint identities by exact
world coordinates to find exactly one complete vertical edge at the selected
corner, then record its actual domain and interior probes. Never assume foreign
edge enumeration equals owned IDs9..12. create_box uses lower corner and length/
width/height, so construct origin+scale*(-3/4,-3/4,-1/2), dimensions
(3/2,3/2,1)*scale and worldradius=r*scale. Reject endpoint/radius rounding for
selected dyadic fixtures. Full original/result bounds and source-retention probes
prevent an accidental translated-axis or stale-handle comparison.

circular_fillet affects EVERY edge and is a different experiment.
fillet_preview is pure but returns standalone surfaces; its live docstring
explicitly reports known kernel crashes for some inputs. It cannot establish
closed-solid topology/material or replace this selected-edge comparison.
These source/API observations select no production dependency.

## Reference volume and other implementations

Retain the previous operation's failure discipline. Brep.volume(relative_accuracy)
clamps requests to[1e-8,1e-1]; its nested integration control does not enclose
actual final volume error. Brep.mass_properties has no Python selective-property
flag in the inspected signature, clamps to[1e-4,1e-1] and defaultcentroid mode
makes two integration passes; it is not a tighter substitute. Internal C++
property flags are not available through that actual Python signature. Preselect
volume/bounds/boundary limits, record requested method/accuracy, preserve all
per-criterion outcomes/artifacts and exit nonzero for any failed comparison.
Do not change API/flags/tolerance after observing a failure to chase green.
Compare manifold/complete finite bounds/analytic rational+pi volume and actual
native face-interior plus exact edge witnesses. Material/history and required
external alpha corpus remain unestablished by those finite reference queries.

OCCT MakeFillet supports selected Add(radius,edge), builds tangent contours,
constructs a result and exposes generated/modified/deleted history plus faulty
contours and partial BadShape. IsDone is explicitly required; a partial result
is not a complete feature. Rational versus quasi-angular/polynomial circular
representations are separate choices. Its acquired one-edge box test checks
BRepCheck_Analyzer validity, useful but insufficient for our exact maps/history
and native consumer claims. [OCCT MakeFillet](https://occt3d.com/dev/doc/refman/html/class_b_rep_fillet_a_p_i___make_fillet.html).

Truck's acquired truck-drafting corner::fillet is TWO-dimensional: trims two
oriented planar edges and creates a tangent circle connector, returns new edge0/
connector/edge1, with closed-wire convenience. Its source provides useful source-
edge/trim/connector separation, not a general3D B-rep fillet operation. The actual
selectedbox feature can use the same mathematical section idea while building
its own complete cap/side shell; adopting Truck is not proposed.
[Truck source](https://github.com/ricosjp/truck/blob/88ed005249e5e3a6b07f62425399435905cd3ab6/truck-drafting/src/corner.rs).

Pinned file catalog: fillet-runtime-sources.tsv; bibliography: fillet-runtime.bib.
No fillet geometry was executed during this research.
