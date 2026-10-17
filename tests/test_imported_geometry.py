"""Direct source/mesh contracts; meaningful geometry cases grow with owned records."""
from pathlib import Path
from dataclasses import replace
import math
import tempfile
import unittest


def pierced_prism_source():
    """Literal thin L-shaped plate with two rectangular through holes.

    This writes original planes/lines/topology directly, without calling an
    extrusion, Boolean, triangulator, foreign CAD or product fixture helper.
    """
    from brepgpu.imported_source import (ImportedSource, Product, SolidDefinition,
        Vertex, Edge, Coedge, Loop, Face, Carrier, Curve)
    outlines = (((0., 0.), (4., 0.), (4., 4.), (2.5, 4.), (2.5, 2.), (0., 2.)),
                ((.5, .5), (.5, 1.), (1., 1.), (1., .5)),
                ((3., 2.5), (3., 3.), (3.5, 3.), (3.5, 2.5)))
    points = tuple(point for outline in outlines for point in outline)
    n, height = len(points), .25
    offsets, cursor = [], 0
    for outline in outlines:
        offsets.append(tuple(range(cursor, cursor + len(outline))))
        cursor += len(outline)
    nexts = {index: outline[(slot+1) % len(outline)] for outline in offsets
             for slot, index in enumerate(outline)}
    vertices = tuple(Vertex(6000+layer*n+i, (*point, layer*height), 0.)
                     for layer in range(2) for i, point in enumerate(points))
    curves, edges = [], []
    def line_curve(id, dimension, origin, direction, length):
        o = tuple(origin)+(0.,)*(3-len(origin))
        d = tuple(direction)+(0.,)*(3-len(direction))
        return Curve(id, dimension, 1, (*o, *d, 0., 0., 0., 0., 0., 0.), (0., 0.), (0., length))
    lengths, directions = [], []
    for i, point in enumerate(points):
        other = points[nexts[i]]
        dx, dy = other[0]-point[0], other[1]-point[1]
        length = math.hypot(dx, dy)
        lengths.append(length)
        directions.append((dx/length, dy/length))
    for layer in range(2):
        for i in range(n):
            slot = layer*n+i
            a, b = 6000+layer*n+i, 6000+layer*n+nexts[i]
            curves.append(line_curve(7000+slot, 3, (*points[i], layer*height), (*directions[i], 0.), lengths[i]))
            edges.append(Edge(5000+slot, (a,b), 7000+slot, (0., lengths[i]), True, True, False, 0.))
    for i in range(n):
        slot = 2*n+i
        curves.append(line_curve(7000+slot, 3, (*points[i], 0.), (0.,0.,1.), height))
        edges.append(Edge(5000+slot, (6000+i,6000+n+i), 7000+slot, (0.,height), True,True,False,0.))
    faces, loops, coedges, carriers = [], [], [], []
    def add_face(frame, bounds, orientation, loop_descriptions):
        fid, carrier_id = 1000+len(faces), 2000+len(carriers)
        carriers.append(Carrier(carrier_id,1,frame,(0.,0.),bounds))
        loop_ids = []
        for outer, descriptions in loop_descriptions:
            lid, ids = 3000+len(loops), []
            for edge_slot, sense, origin, direction in descriptions:
                cid = 4000+len(coedges)
                curve_id = (1<<63)+cid
                edge = edges[edge_slot]
                curves.append(line_curve(curve_id,2,origin,direction,edge.parameter_range[1]))
                coedges.append(Coedge(cid,edge.id,lid,sense,curve_id,edge.parameter_range))
                ids.append(cid)
            loops.append(Loop(lid,fid,outer,tuple(ids)))
            loop_ids.append(lid)
        faces.append(Face(fid,9000,carrier_id,orientation,tuple(loop_ids),0.))
    axes = (1.,0.,0.,0.,1.,0.,0.,0.,1.)
    bottom, top = [], []
    for li, outline in enumerate(offsets):
        bottom.append((li == 0, tuple((i,-1,points[i],directions[i]) for i in outline[::-1])))
        top.append((li == 0, tuple((n+i,1,points[i],directions[i]) for i in outline)))
    add_face((0.,0.,0.,*axes),(0.,4.,0.,4.),-1,bottom)
    add_face((0.,0.,height,*axes),(0.,4.,0.,4.),1,top)
    for i in range(n):
        dx,dy = directions[i]
        frame = (*points[i],0.,dx,dy,0.,0.,0.,1.,dy,-dx,0.)
        length = lengths[i]
        add_face(frame,(0.,length,0.,height),1,((True,(
            (i,1,(0.,0.),(1.,0.)),
            (2*n+nexts[i],1,(length,0.),(0.,1.)),
            (n+i,-1,(0.,height),(1.,0.)),
            (2*n+i,-1,(0.,0.),(0.,1.)))),))
    definition = SolidDefinition(9000,1,tuple(f.id for f in faces),tuple(e.id for e in edges),
                                 tuple(v.id for v in vertices))
    return ImportedSource('0'*64,1.,0.,1,'literal independent plate fixture',
                          (Product(1,'thin pierced L','fixture',9000),),(definition,),(),
                          vertices,tuple(edges),tuple(coedges),tuple(loops),tuple(faces),
                          tuple(carriers),tuple(curves))


class ImportedGeometryDirectTests(unittest.TestCase):
    def test_retained_spline_endpoint_choice_has_direct_source_joint_oracle(self):
        from tests.test_imported_meshing import spline_endpoint_sheet_fixture
        from brepgpu.imported_meshing import mesh_imported
        from brepgpu.imported_oracles import _joint_recovery
        source=spline_endpoint_sheet_fixture();mesh=mesh_imported(source)
        face=source.faces[0];joint=mesh.faces[0].joint_recoveries[0]
        vertices={(v.definition_id,v.physical_key):v for v in mesh.vertices}
        def check(record):
            return _joint_recovery(source,face,record,vertices,mesh.quality,2.**-32,{})
        self.assertEqual(check(joint),(60,11,12))
        for record,message in (
                (replace(joint,uv=(.45015,0.)),'invented a new UV'),
                (replace(joint,uv=joint.outgoing.uv),'minimum retained candidate'),
                (replace(joint,source_vertex_id=50),'endpoint identity'),
                (replace(joint,outgoing=replace(joint.outgoing,pcurve_uv=(.45,.01))),
                 'raw recovered pcurve')):
            with self.subTest(message=message),self.assertRaisesRegex(AssertionError,message):
                check(record)

    def test_original_curve_departure_cannot_hide_behind_planar_carrier(self):
        from brepgpu.imported_meshing import mesh_imported
        from brepgpu.imported_oracles import qualify_mesh
        source=pierced_prism_source();mesh=mesh_imported(source)
        edge=source.edges[0];curve=source.curve(edge.curve_id)
        shifted=replace(curve,frame=(*curve.frame[:2],curve.frame[2]+.0008,*curve.frame[3:]))
        wrong=replace(source,curves=tuple(shifted if c.id==curve.id else c for c in source.curves))
        with self.assertRaisesRegex(AssertionError,'original spatial endpoint departure'):
            qualify_mesh(replace(mesh,source=wrong))

    def test_source_reader_rejects_non_source_bytes(self):
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'invalid.bgsource'
            path.write_bytes(b'not an owned CAD source')
            with self.assertRaises(ValueError):
                load_source(path)

    def test_mesher_rejects_an_absent_actual_source(self):
        from brepgpu.imported_meshing import mesh_imported
        with self.assertRaises((TypeError, ValueError)):
            mesh_imported(None, None)

    def test_actual_nonconvex_two_hole_thin_solid_complete_domains_and_genus(self):
        from brepgpu.imported_meshing import mesh_imported, MeshQuality
        from brepgpu.imported_oracles import qualify_mesh
        source = pierced_prism_source()
        mesh = mesh_imported(source, MeshQuality(tolerance_mm=.005,source_residual_mm=1.e-8))
        stats = qualify_mesh(mesh)
        self.assertEqual(stats['definitions'],1)
        self.assertEqual(stats['faces'],16)
        self.assertEqual(stats['solid_eulers'],(-2,))  # exactly two through holes
        self.assertLessEqual(stats['max_finite_interpolation_probe_error'],2.**-44)
        self.assertEqual(mesh.bounds(9000),(0.,0.,0.,4.,4.,.25))

    def test_missing_reversed_duplicate_cells_and_original_parameter_corruption_raise(self):
        from brepgpu.imported_meshing import mesh_imported
        from brepgpu.imported_oracles import qualify_mesh
        mesh = mesh_imported(pierced_prism_source())
        first = mesh.triangles[0]
        reverse = replace(first,vertex_keys=first.vertex_keys[::-1],corners=first.corners[::-1])
        face = mesh.faces[0]
        first_loop = face.boundary_loops[0]
        shifted = replace(first_loop[0],source_parameter=first_loop[0].source_parameter+.125)
        bad_boundary = replace(face,boundary_loops=((shifted,*first_loop[1:]),*face.boundary_loops[1:]))
        cases = (replace(mesh,triangles=(reverse,*mesh.triangles[1:])),
                 replace(mesh,triangles=(first,*mesh.triangles)),
                 replace(mesh,faces=(replace(face,triangle_indices=face.triangle_indices[1:]),*mesh.faces[1:])),
                 replace(mesh,faces=(bad_boundary,*mesh.faces[1:])))
        for index, changed in enumerate(cases):
            with self.subTest(case=index),self.assertRaises(AssertionError):
                qualify_mesh(changed)


if __name__ == '__main__':
    unittest.main()
