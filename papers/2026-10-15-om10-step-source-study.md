# Actual OM10 MAN STEP: bounded source inspection

2026-10-15. The operator supplied the OM10 MAN VA 01.3 STEP file. This supersedes the earlier acquisition barrier: actual source bytes are now present privately. The original was read without modification; a byte-identical ignored copy is retained under `quarantine/om10/`. The parent records acquisition hash and restoration details. This report uses only standard-library text inspection of original entity statements, references, product labels and coordinates. No CAD kernel, product import, geometry evaluation, tessellation, rendering, tests or new dependency ran.

**There is enough actual source to choose a real first component: OM00-00142, “Bride de fixation,” the movement fixing clamp.** It is much smaller in scope than reconstructing a complete wheel or bridge. The complete assembly also contains recognizable named train, barrel, balance, winding and calendar products. An interchange file is still not a complete manufacturing TDP or evidence that every encoded solid is valid.

## Representation and units

The header declares AP214 `AUTOMOTIVE_DESIGN`; the exporter identifies Inventor 2022 and the export is dated June 2022. The original header's author, organization and filesystem paths remain private.

The text contains 328,069 numbered entity statements. The counts below refer to distinct source records, not evaluated geometry or triangulated output:

| Source records | Count | Meaning |
|---|---:|---|
| PRODUCT / PRODUCT_DEFINITION | 138 each | Product definitions, including assemblies |
| Products with assembly children | 31 | Includes the single movement root |
| Leaf product definitions | 107 | Each maps to an encoded solid representation |
| NEXT_ASSEMBLY_USAGE_OCCURRENCE | 196 | Defined parent-child usage records |
| Expanded leaf occurrences from root | 166 | Repeated part usages counted through the actual assembly graph |
| MANIFOLD_SOLID_BREP / CLOSED_SHELL | 107 each | Distinct encoded solid/shell records |
| ADVANCED_FACE | 9,162 | Source faces across those stored solids, without instance expansion |
| EDGE_CURVE | 25,871 | Stored topological edge records |
| FACE_BOUND in addition to FACE_OUTER_BOUND | 606 | Additional loop records, not a count of physical drilled holes |

The root is PRODUCT_DEFINITION `#326778`, corresponding to OM10-00001. Product counts, usage-record counts, unique solids and expanded leaf counts answer different questions; do not call them all “number of parts.” For example the single clamp solid is used three times directly under the root.

All 139 geometric representation contexts assign length unit `#326163`, **millimetres**, angle radians and solid angle steradians. An additional centimetre length-unit definition exists, but none of these context unit assignments uses it. Source numbers must be interpreted through the referenced contexts rather than choosing whichever SI unit first appears in the file.

All 139 `UNCERTAINTY_MEASURE_WITH_UNIT` records declare length 0.01 mm with a model-connectivity uncertainty description. **This is source model uncertainty, not a manufacturing tolerance, a measured error, or permission to relax our operation tolerances.** Small exported roundoff values are present even in nominally axis-aligned geometry.

Surface records are 3,688 planes, 4,431 cylindrical surfaces, 341 conical surfaces, 49 toroidal surfaces, 18 spherical surfaces and 635 B-spline surfaces with knots. Curve records include 14,423 lines, 8,901 circles, 212 ellipses and 2,262 B-spline curves with knots. This is genuine analytic/spline B-rep content, not just a triangle export. Counting these types does not validate trims, orientation, manifoldness, absence of intersections or manufacturing suitability.

## First candidate: OM00-00142 fixing clamp

The source labels the part **“Bride de fixation.”** Its reference chain is:

`PRODUCT #327331 → PRODUCT_DEFINITION_FORMATION #327052 → PRODUCT_DEFINITION #326914 → PRODUCT_DEFINITION_SHAPE #326585 → SHAPE_DEFINITION_REPRESENTATION #326303 → SHAPE_REPRESENTATION #326775 → SHAPE_REPRESENTATION_RELATIONSHIP #1604 → ADVANCED_BREP_SHAPE_REPRESENTATION #1711 → MANIFOLD_SOLID_BREP #1818 → CLOSED_SHELL #174240`.

The assembly has three named clamp usages, represented by `NEXT_ASSEMBLY_USAGE_OCCURRENCE #922, #923, #925`. Their placements must be followed separately; the local shape is not the assembly placement.

Static inspection finds seven faces: five planes and two cylinders. The lower and upper planar caps are at local Z=0 and Z=0.3 mm. Each cap has an outer loop containing three straight edges and one circular arc, plus a separate complete circular inner loop. The inner cylinder radius is 0.6 mm and the outer arc radius is 0.9 mm. Both cylinder axes run along local positive Z through the local origin.

The upper cap `#174114` has outer loop `#37672` and inner loop `#37673`; the inner ring edge `#109492` uses the same topological vertex at both ends and a circle carrier of radius 0.6 mm. The lower cap `#174115` similarly has outer loop `#37674` and inner loop `#37675`. Thus the source encodes a **complete through-hole**, rather than an open notch. The outer arc connects the positive-Y and negative-Y ends with positive traversal around a positive-Z axis and positive-Y reference axis: it is the left semicircle. Three planar wall faces close the rectangular extension.

The resulting nominal outline is a rounded end with a rectangular tail, 3.1 mm in overall X span, 1.8 mm in Y span, and 0.3 mm thick, with a 1.2 mm diameter through-hole. These full nominal spans follow from the actual circular arc extremum as well as its endpoints. **Vertex coordinates alone** have X extrema 0 and 2.2 mm and would miss the leftmost point of the arc. The file's tiny nonzero axis/endpoint components must remain a declared source-rounding matter; this inspection does not silently snap the entire imported solid to an exact manufactured design.

This candidate exercises useful work with an understandable target: physical units, a thin plate, a planar profile containing an actual circular arc, a complete bore, honest assembly placement and component identity. Its thinness/overall-span ratio is about 0.097; uniform rescaling cannot make that ratio larger. The present convex straight-segment extrusion and minimum local height contract do not faithfully represent it. A polygonal silhouette or thickened plate could illustrate it, but should not be called the source component.

## Next candidates: real thin train wheels

These candidates are actual source definitions, not inferred names from a photo:

| Product | Original label | Stored solid | Source faces | Source thin direction |
|---|---|---|---:|---|
| OM10-00159 | Planche de minuterie | `#1804` | 243: 98 planes, 145 cylinders | Original vertices lie at local Z=0 and 0.15 mm |
| OM10-00128 | Planche de chaussée | `#1799` | 363: 122 planes, 241 cylinders | Original vertices lie at local Z=0 and 0.20 mm |
| OM10-00150 | Planche de grande moyenne | `#1761` | 418: 162 planes, 256 cylinders | Original vertices lie at local Z=0 and 0.20 mm |
| OM10-00162 | Planche de moyenne | `#1763` | 403: 156 planes, 247 cylinders | Original vertices lie at local Z=0 and 0.20 mm |
| OM10-00165 | Planche de seconde | `#1765` | 463: 180 planes, 283 cylinders | Original vertices lie at local Z=0 and 0.20 mm |

These thin-coordinate observations are source vertex inventories, not computed face extrema or a qualified thickness measurement for every face. Their large counts of cylindrical faces show that the stored toothed/apertured boundaries demand more than one convex polygon and one bore. They should follow the clamp, rather than being selected as an allegedly simple first extrusion. The actual 0.15 mm minuterie wheel also makes clear why a release note proposing 0.20 mm must not be generalized to every part in this file.

The named bridges are more involved still: OM10-00184 “Pont de rouage brut,” solid `#1768`, has 141 faces including 41 conical and two B-spline surfaces; OM10-00199 “Pont de barillet brut,” solid `#1800`, has 102 faces including six conical surfaces. Neither is just a flat polygon with decorative holes.

## What this file does and does not supply

The file supplies nominal B-rep geometry, product identities, assembly relations/placements, style information, and material-name/density property representations for leaf products. The 214 PROPERTY_DEFINITION records are those material-name/density properties; they are not 214 engineering tolerances.

This bounded inventory did not find dimensional-size/location, geometric-tolerance or drawing-model entities. It does not supply a complete set of toleranced production drawings, heat treatments, surface finishes, assembly clearances, gear manufacturing standards, inspection procedures or demonstrated performance. Public OM10 technical documentation discusses CC BY-SA 3.0 and movement marking; retaining private supplied CAD is not a conclusion about redistribution terms. The parent should preserve exact source access/terms before publishing derived geometry.

Proceed with a source-linked clamp design study: retain its source entities, units, arc/hole layout, thickness, rounding decisions and three occurrence placements, then define the smallest owned modeling capability that can build and mesh that actual part. Reading this STEP source has not created a BRepGPU STEP importer, a functional watch, or a validated manufacturing model.
