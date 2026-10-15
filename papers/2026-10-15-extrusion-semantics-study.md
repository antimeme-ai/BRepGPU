# Owned convex-profile extrusion: geometry and history study

2026-10-15. Research only; no product execution, implementation, build or dependency adoption. Read current doctrine, body/Boolean/interchange contracts, the actual pinned OCCT, Truck and NVIDIA sweep sources listed in [the source catalog](extrusion-semantics-sources.tsv), and the primary lecture notes below. This report recommends the parent's selected polygon family; the earlier rectangle/hole alternative is deferred.

## Recommended complete operation

Admit an owned immutable planar profile with 3..32 original vertices, one straight CCW boundary word, one face and one chart. Local coordinates lie on the exact 1/256 grid in [-3/4,3/4]²; the profile lies at local z=0 in the existing bounded positive power-of-two Frame. The carrier is the actual fixed [-1,1]² plane Patch, with raw UV ((x+1)/2,(y+1)/2), rather than a fitted bounding plane. The profile owns typed vertex, edge, coedge, loop, face and chart identities; equal coordinates in another profile do not create ancestry.

For each directed edge a→b require cross(b−a,p−a)>0 for **every nonincident original vertex** p, using exact grid integers. This admits only strict convex CCW simple boundary order. Positive adjacent turns and positive area alone are insufficient: winding stars can satisfy those weaker checks. Reject repeated closing vertices, duplicates, zero/collinear edges, clockwise words, concavity, self crossings and off-grid inputs. Do not repair winding or sort points. Bound input consumption before materializing an unbounded iterator, and reject bool aliases and nonfinite values under an explicit numeric policy.

Admit height h on the same exact grid, 1/4≤h≤2, and construct the centered solid P×[-h/2,h/2]. Centering makes the two cap images modified descendants of every source entity; neither cap is falsely reported as retained at z=0. This creates a genuine nonrectangular feature from its submitted profile, rather than relabeling a canonical box or previous Boolean result. A separate result type should preserve existing canonical-only API boundaries.

The whole boundary is exactly (∂P×[-h/2,h/2])∪(P×{-h/2,+h/2}). Strict convexity and h>0 prove that nonadjacent sides cannot intersect, and caps meet sides only on their declared shared edges. No general SSI, arrangement or triangulation is needed for this admitted family.

## Exact carriers, trim domains and orientation

For source edge Cᵢ(u)=(1−u)aᵢ+u aᵢ₊₁, use the complete side carrier

`Sᵢ(u,v) = (Cᵢ(u).x,Cᵢ(u).y,(v−1/2)h)`, 0≤u,v≤1.

Its U-major/V-fast homogeneous controls are `(aᵢ.x,aᵢ.y,−h/2,1)`, `(aᵢ.x,aᵢ.y,+h/2,1)`, `(aᵢ₊₁.x,aᵢ₊₁.y,−h/2,1)`, `(aᵢ₊₁.x,aᵢ₊₁.y,+h/2,1)`. Both cap carriers retain the source fixed-square XY controls, weights, raw UV and Frame, changing only their local z to the appropriate cap level. All construction coefficients are exactly binary64 representable: XY denominators 256, z denominators at most 512, and all local control components are within [-1,1]. This is a zero coefficient-rounding statement, not a zero native evaluation/world-addition error statement.

The cap carrier square is larger than the actual polygon. Exact polygon halfplanes must trim it. Native candidate classification must use the actual submitted binary64 UV values; rounding a rational boundary query need not keep it on the boundary. Side patches have the entire unit-square domain. Caps have raw derivatives (2,0,0),(0,2,0); side axial derivative is h·e_z. Do not silently substitute physical-distance V for normalized v.

Upper cap face sense is +1 and its boundary follows the source word. Lower cap face sense is −1 and its ordered coedge word is reversed with opposite occurrence signs. Intrinsic cap edge parameter directions can remain source directions. Natural side S_u×S_v points outward, proportional to (dy,−dx,0). Its positive boundary word is lower edge forward, end vertical edge upward, upper edge backward, start vertical edge downward. Every shared physical edge then has two opposite uses. Vertical edges are shared by source vertex identity, never by XYZ welding. There are no periodic seams or closed edges in this family.

## Complete source history and proposed Origin schema

Reuse the six-kind typed EntityKey convention, but keep the extrusion origin record specific to its actual maps. Recommend a frozen record with:

| Field | Meaning |
| --- | --- |
| target | Fresh output EntityKey |
| relation | modified or generated |
| parents | Distinct original profile EntityKeys, never output keys |
| role | lower_cap, upper_cap, side, swept_vertex, side_lower, side_upper, side_start or side_end |
| orientation | Integer ±1, occurrence/face sense relative to the declared intrinsic source map |
| local_translation | Optional exact (dx,dy,dz) for translated cap images |
| source_t | Optional exact (a,b,c), giving original edge parameter a+b·u+c·v |
| source_uv | Optional two exact (a,b,c) rows, giving original raw profile UV |
| axial | Optional exact (a,b,c), giving local output z=a+b·u+c·v |

For output edges/coedges, the common map coordinates mean u=t and v=0. For face/chart maps they mean that record's raw (u,v). Coedge traversal sign is separate from the intrinsic parameter map and must be applied exactly once. Null maps mean inapplicable, not an unstated identity. A loop's ordered occurrence word remains the authoritative boundary correspondence; a single affine map does not encode its cyclic sequence.

Cap images have local translation (0,0,±h/2). Source edge/coedge parameters map identically; cap face/chart raw UV maps identically. Lower cap face/loop/use orientation is −1, upper +1, while intrinsic cap edge directions and vertex correspondence remain unchanged. Source loop UV may be identified as the containing raw UV chart, but its complete reversed/preserved boundary word must also be retained. The world translation is Frame.scale times the local vector; it is not a displacement of h/2 world units at every scale.

Each generated vertical edge is supported by its original profile vertex, has axial (−h/2,h,0), and intrinsic upward direction. Each generated side face/chart is supported by its original edge and its own original coedge occurrence, source_t=(0,1,0), axial=(−h/2,0,h), orientation +1. Each side loop has the same source support and explicit four-use boundary word. Generated side horizontal uses are supported by the original edge and coedge, source_t=(0,1,0), axial=(±h/2,0,0), lower sign +1/upper −1. Generated side vertical uses are supported by the original vertex plus the **own side's original edge** (and source coedge if the final schema consistently includes it); axial=(−h/2,h,0), start sign −1/end +1. That edge parent distinguishes two different face occurrences of the same physical vertical edge. Do not pretend the new axial parameter is an interval on a source profile edge.

| Six-kind tables | Source | Output |
| --- | ---: | ---: |
| vertices | n | 2n |
| edges | n | 3n |
| coedges | n | 6n |
| loops | 1 | n+2 |
| faces | 1 | n+2 |
| charts | 1 | n+2 |
| total | 3n+3 | 14n+6 |

Every source entity has exactly two same-kind modified cap images: 6n+6 output origins. The 8n generated outputs comprise n vertical edges, n side faces, n side charts, n side loops and all 4n side uses. All 3n+3 source dispositions are modified; none is deleted or retained. Generated descendants can additionally appear in source history through their declared support. Forward and reverse maps must be exact inverses of the complete origin-parent relation, while modified/generated filtering preserves this distinction. Geometrically equal non-source keys must fail history queries.

Construct, validate and publish atomically. A late ownership/map/topology/history failure must expose no partial output and must leave the source profile usable. Output source frames and immutable profile records must survive later consumer failures.

## Useful exact readers and qualification

Material is the conjunction of the original polygon closed halfplanes and the two height bounds after exact inversion of the source Frame. Return every zero supporting face: three at a vertex, two along an edge, one at an ordinary face point. Cap trim classification uses the original whole polygon, not merely its bounding box. Exact edge/coedge evaluation should read stored rational records without the existing convenience evaluators' float coercion.

An independent model can derive the whole expected boundary from the submitted source polygon and height, checking each side/cap map and complete ordered word. Check two opposite uses per edge, ownership and one three-port vertex link cycle, connected shell and Euler V−E+F=2. Exact shoelace area and area·h·Frame.scale³ volume support the whole-boundary oracle but cannot replace it. Native first jets must be graded against the actual controls/submitted bits and normalized side parameter derivatives; world unit normals need an independent square-root interval for generally irrational (dy,−dx,0)/sqrt(dx²+dy²).

Use asymmetric triangle/pentagon and an explicit admitted high-vertex lattice polygon, rather than rectangles alone. The upper bound of 32 needs an actual strictly verified lattice fixture, not an assumed regular polygon rounded to the grid. Include hostile winding stars, duplicates, collinear/CW/concave words, height/envelope/type failures, equal-geometry different namespaces, source mutation attempts and late-construction rollback. Existing sampling and native CSR readers are meaningful consumers of the new output; CPU/GPU sampling or adjacency does not imply GPU feature construction. General result meshing remains unsupported until separately designed.

## What the actual references teach

OCCT 8.0.1 MakePrism specifies dimension-raising sweeps and separate first/last/generated history. The actual wrapper calls BuildCurves3d on its input before constructing the prism, so Copy is not a sufficient blanket promise that reference-side caches never change; prepare a disposable reference copy. Its implementation preserves generating parameters and cap vertex UV, while directing-edge parameters span physical vector length. Canonization can change side carrier type and raw coordinates. Compare semantic world geometry, topology and normalized source maps, not arbitrary face order or assumed raw V identity. Its OCC31294 regression explicitly distinguishes an actual generating entity from a new geometrically equal entity.

Truck's actual face sweep reverses the lower face, maps the upper face and connects their boundary wires. Its wire connector caches vertical edges in a HashMap keyed by original VertexID. This directly supports our source-identity sharing and four-use side word; no coordinate weld is needed.

NVIDIA's pinned Python linear_sweep is a pure capped-by-default operation whose direction magnitude is ignored and whose separate distance supplies length. The C++ wrapper copies profile curves before invoking the consuming internal sweep and publishes a new Brep after success. This differs from the previously studied consuming Boolean API. A future reference adapter must identify the installed runtime's API/version and cap placement before comparison; this source read alone does not certify runtime equivalence.

Sauer's author-hosted notes define extrusion by adding a parameter times a fixed direction and give its two Jacobian columns, then connect ruled surfaces to tensor-product patches (§5.2–5.3). These are the mathematical basis for the bilinear construction above. Do not repeat the apparent degenerate equal-curve example in Remark 5.6(3): a nonzero extrusion uses a translated second curve, as the defining equation itself states.

## Acquisition and scope boundaries

Original Sauer PDF retained unchanged, 9,719,637 bytes, SHA256 `8e852d1a94081728e6594d4387f82b394ecfc47aa0f3252773c9cca04abebe9b`; title/contents and PDF pages 80–84 read. Source and exact restoration URL appear in the catalog and bibliography. Existing intact OCCT/Truck/USD BRep archives and their restoration records were reused; no new source dependency was adopted. Their specific files and hashes are catalogued, rather than counting entire repositories as read.

Unsupported in this first unit: holes, concavity, curved edges, non-grid profiles, arbitrary rational imported profiles, arbitrary sweep directions, draft/twist/taper, negative/zero height, revolution, fillet, offset, general triangulation and imported-solid conversion. Interchange's accepted carrier coefficients do not supply interpreted closed profile trims. This report selects geometry/history semantics, not a permanent implementation language. External community-suite qualification remains an alpha requirement under the existing policy; this bounded operation does not make it green.
