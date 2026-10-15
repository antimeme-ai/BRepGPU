# Actual convex-profile extrusion mesh

3 original profile vertices, 3/4 local height; 6 shared output vertices and 8 triangles. World tolerance 1/1048576; actual rounding/error bound 0/1.

The actual ExtrusionResult, original profile and complete typed history are retained. Exact planar faces need no geometric approximation. The cap mesh covers the actual trimmed profile polygon, and original coedges/UV maps name each occurrence. Source and result tables preserve original controls and Frames. The PLY and views use one shared rounded coordinate array; facet normals are not smoothed. No hidden-line removal, general triangulation/quality guarantee, GPU mesher or alpha qualification claim.
