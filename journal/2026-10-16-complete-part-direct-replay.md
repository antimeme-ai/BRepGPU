# Complete original main plate through direct CDT

The original main plate is a development consumer, not a source-specific fast
path. Its selected definition closes over all438 original faces,532 loops,
2,532 coedges,1,266 edges and844 source vertices. No foreign edge incidents or
carrier faces were discarded. Original whole-source geometry/assembly outcomes
are retained with matching source bytes; a selected part is not falsely tested
against the whole STEP inventory.

With the final release direct library605e0b21c58093fe5270a30099b9338d18c1b2880de7465d8188553db2a7f968,
one CPU constructed95,766 physical vertices and191,688 triangles in209.607832s.
Existing internal stage walls were146.188608s validation/sampling and63.391254s
connectivity/refinement. Source decode6.924547s and selection.227630s are outside
that product wall. Peak construction process RSS was515,184KiB. These stages
overlap the complete product wall and must not be added to it.

Independent full-part qualification reached its separate300s bound; total
worker elapsed517.674057s. There was no completed whole-part pass. A bounded
three-second nonblocking sample returned188 valid stacks and108 sampling
errors, showing the original exact `_simple_domains` Fraction AABB pair loop.
This short sample locates real work; it does not estimate whole-run percentages.
The native geometry, source and quality stayed pinned during the trial.

The ranked sweep design replaces that reader's candidate enumeration and
preserves exact source predicates, area, winding, containment and original loop
adjacency. Root and independent numerical review accepted the written plan
before implementation. The first worker has stopped, so the new reader can be
installed without violating its historical source pins. Root will replay the
same part after relevant GREEN and code-review fixes; qualification remains a
separate measured stage. No watch render, GPU CDT or alpha result follows from
construction alone.
