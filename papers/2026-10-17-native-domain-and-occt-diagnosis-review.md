# Current native domain and external OCCT diagnosis

Read-only review after parent reported native boundary waves and the external
OCCT attempt with no face triangulations. No build, probe, geometry execution or
source repair by this lane. Actual repair proposals require their own source
facts and scoped review; the rendering objective does not relax source gates.

## Native selected-domain source facts

Read adaptive_regions.cpp::prepare_face_domain and coarse_requests. Count/zero
area, zero-length selected edges, crossing/contact and hole placement are folded
into one coarse flag. The retry then selects every face interval with positive
source_to_selected_trace_upper. This explains the breadth of possible shared
boundary invalidation; it does not identify why the reported two plane domains
are invalid.

A positive conservative interval upper is not itself a witness that the affected
domain issue improves under midpoint splitting. It can include fixed original
source versus chosen-trace departure or numerical enclosure width. Exact issue
kind and affected original source occurrences/chosen coordinates must determine
the next proposal. No threshold widening, hidden source-envelope extension,
discarded uses, physical-Q change or boundary-error-as-interior-map shortcut
follows from the repeated requests. Current original node and whole selected
trace-to-SAME-Q guards remain acceptance obligations.

## External OCCT diagnostic review

Scoped ACCEPT of the narrow proposal to retain post-mesher status flags,
triangulated root-face count and first missing ORIGINAL FaceID before the
existing all-face guard. One retained small original definition, identical
absolute deflection 0.005 mm, angular deflection 0.5 rad and serial operation is
the proposed bounded diagnostic, not a repaired or successful full watch.

Read adopted OCCT revision b8f597c677811d1f9f4d8a97f5ae2825c0353a42:
BRepMesh_IncrementalMesh.cxx::Perform resets myStatus to NoError, ORs face/wire
status masks only when GetModel is nonnull, and calls setDone afterward.
IMeshTools_MeshBuilder.cxx::Perform separately records failures for BuildModel,
edge discretization, healing/preprocessing, face meshing and postprocessing.
BRepMesh_ModelBuilder.cxx::performInternal can return a null model on a void
bounding box. These are inspected source paths, not diagnoses of this input.
Therefore IsDone and even flags zero cannot independently establish any face
triangulation or mesh-builder success. If the bounded probe remains ambiguous,
an explicit retained context/builder status is the relevant next observation.

Existing strict original face mapping, finite nonempty per-face triangulations
and complete-image gating stay in force. No healing, skipped face, changed
tolerance, fabricated triangles or alternative source binding is selected.

Subsequent concrete small-definition diagnosis: three planar faces mesh, the
cylinder does not, and all four incident curved-support pcurves are absent.
Scoped ACCEPT of the literal selected preparation in
docs/occt-watch-reference-unit.md: reuse unchanged prepare_source_uv RAW stage,
prepare_query=false, on a deep geometry/topology copy. This includes disclosed
bounded conic guide coefficient selection and guarded seam/whole-period
translations, with existing EdgeState/FaceState and ordered-use/period guards.
No optional query columns, endpoint edits, domain correction or pole connectors
are imported. Original physical bytes/manifest stay unchanged. Resolve every
pre-admitted original FaceID through prepared.mapped(original_face), never a new
copy storage ordinal. Same actual four-face failure then the existing authored
nine-face operation is the bounded proposed closure. No execution by reviewer.

Actual caller and standalone CMake source review: ACCEPT. The caller retains
the original admitted shape map, builds ordered use bindings from those shapes,
sets prepare_query=false explicitly, meshes only prepared.raw_shape and maps
every original face through prepared.mapped. The solid and meshing-face values
retain their copied OCCT handles after the local PreparedSourceUV object dies;
no borrowed copier map or new storage ordinal escapes. Preparation reporting
records projection/seam/chart counts and finite-probe residual separately. The
unchanged helper and existing TKShHealing dependency are linked only into the
standalone reference. Owner reports the same actual four-face case now has
flags zero and all four faces triangulated; that result was not rerun here.

Scoped local chosen-domain recovery PLAN review: ACCEPT. The selected private
typed first failure and edge-origin rows preserve the exact crossing pair rather
than requesting every positive-upper interval on the face. Origins must follow
edge reversal and connector insertion. For a=Q1-Q0 and b=Q2-Q0, squared circle
curvature is 4|a cross b|^2/(|a|^2|b|^2|b-a|^2); all denominator factors are
checked nonzero and exact cross multiplication compares the estimates. Choosing
the flatter side only when both estimates are positive, or the curved side when
the other is zero, is a finite recovery proposal, not an acceptance or general
convergence result. One existing witnessed BoundaryKey, local retained-bound
fallback and unresolved when no eligible interval exists preserve source
authority. Exact contact/winding/area admission and finite caps remain unchanged.

Actual local recovery source review: ACCEPT. DomainSegmentIndex returns indices
in the original segment array. The origin rows mirror ordinary edge pushes,
connector insertion and reversal; zero/area/nesting witnesses select only the
implicated loop or edge. Curvature uses captured physical Q, checked nonzero
lengths and exact rational ratio comparison. A zero three-sample estimate does
not prove a whole general curve straight; requested narrowing of that comment
does not change the selected heuristic. No execution by this reviewer.

Subsequent external reference QUERY-copy PLAN review: ACCEPT after reading the
existing helper stages. The second deep copy runs the disclosed columns,
finite joints, natural-domain and analytic-pole construction, with physical
EdgeState/FaceState checks and original-use quotient admission. Pole connectors
are explicit copied trim topology reusing the original physical vertex; they
are not new original edges. Each original FaceID resolves through query_mapped.
Per-use perturbation allocation and finite residual reporting do not establish
exact trim preservation or certified requested deflection. The same six failed
definitions must meet unchanged all-face/noncollapsed gates before the existing
nine-face check and one further complete attempt. This is plan acceptance, not
a prediction that those failures close; no reviewer execution.
