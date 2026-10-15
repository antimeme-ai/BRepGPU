# Boolean-result mesh design review: caller, source export and costs

Reviewed docs/CUT_MESH_DESIGN.md against actual canonical mesh and Boolean
callers, indexed-reader implementation, factory-only input ownership and current
serial measurement practice. Accepted for this perspective; no open findings.

The design preserves the actual CutResult/CutSolid namespace, original cutter
frame and raw V band, and complete source/history rather than substituting a
canonical plate. The dedicated source exporter avoids invalid CutSolid packed
tables. Existing indexed consumption can be reused directly, with truthful SVG
labels and unchanged canonical defaults. Complete mesh records plus actual
reader buffers receive first independent qualification followed by direct repeat
correspondence. History consumption must perform actual forward/reverse lookups,
not only export/count records.

Implementation plan should name the prepared-result-to-consumed wall endpoints
explicitly: CutResult already exists; begin before tessellate and end after
indexed/history reading. Operand preparation and Boolean construction are named
separate costs. A standalone direct preparation-through-export enclosure may
also be recorded but is overlapping and must not be summed with its phases.
No native/GPU setup or transfers occur in this CPU mesher campaign.
