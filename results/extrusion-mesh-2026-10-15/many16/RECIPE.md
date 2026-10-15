# Actual convex-profile extrusion mesh

16 original profile vertices, 3/2 local height; 32 shared output vertices and 60 triangles. World tolerance 1/1048576; actual rounding/error bound 0/1.

The actual ExtrusionResult, original profile and complete typed history are retained. Exact planar faces need no geometric approximation. The cap mesh covers the actual trimmed profile polygon, and original coedges/UV maps name each occurrence. Source and result tables preserve original controls and Frames. The PLY and views use one shared rounded coordinate array; facet normals are not smoothed. No hidden-line removal, general triangulation/quality guarantee, GPU mesher or alpha qualification claim.
