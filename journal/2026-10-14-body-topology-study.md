# First body topology study

Read current doctrine/topology architecture, retained adapter/quarter-cylinder
source and pinned OCCT closed-surface pcurve/SameParameter/SameRange headers.
Studied one minimal body/consumer unit using immutable Python records and owned
packed bytes with existing native patch evaluation. Saved advice at
papers/2026-10-14-body-topology-study.md. No library adoption, product code,
native execution, build or new agents.

Confirmed canonical one-closed-edge rim representation and counts: box
8/12/6/6/24, cylinder2/3/3/3/6, plate10/15/7/9/30 for V/E/F/loops/coedges.
Explained opposed seam uses on the same wall, closed-edge double endpoint
incidence, wall/cavity/cap traversal and plate χ=0 after holed-face correction.
Four quadrant charts stay on one periodic face without invented body edges.
Native derivatives are local/one-sided at joins; normalized normals agree.

Sent exact-snapshot prepared-input binding and affine-carrier-type wording
findings to coordinator; corrected design now retains frame/chart/patch order
and rejects same-size other snapshots. Factory-only construction is planned.
Exact pcurve expressions and binary-input trim classification remain distinct
from rounded pcurve UVs. Packed topology remains CPU data, not a new GPU ABI.

Re-read the corrected BODY_SAMPLING_DESIGN.md topology clauses. Accepted with
no remaining blocker; exact trim sign, same-face seam occurrences, closed-edge
counts/Euler, frame/parameter maps and binding/failure outputs agree with the
narrow study. Ready for implementation-plan review. No additional research,
code or execution was added.
