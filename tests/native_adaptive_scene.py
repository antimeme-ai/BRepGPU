"""Actual component artifacts -> native display buffers and original normals."""
from collections import defaultdict
from dataclasses import replace
from fractions import Fraction as F
from pathlib import Path
import argparse
import math
import struct
import subprocess
import tempfile

from native_adaptive_component import box_source
from native_adaptive_cap_fixture import cap_source
from native_adaptive_curves_caller import Reader, read as read_curves, check as check_curves, check_seam
from native_adaptive_trimmed_caller import (
    read_face, qualify_face, cylinder_source, c0_source, curved_source, source_value, cylinder_trig, orient)
from brepgpu.imported_source import Product, Usage, Style, write_source


def styled_source():
    src = box_source()
    def style(identifier,rgb):
        return Style(identifier,(),(*rgb,1.),True)
    return replace(src,source_sha256='d2'*32,
        products=(Product(100,'assembly','assembly'),Product(101,'box','box',200,10)),
        definitions=(replace(src.definitions[0],product_id=101),),
        usages=(Usage(710,100,101,'direct',(1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.),20),
                Usage(711,100,101,'quarter',(0.,-1.,0.,3.,1.,0.,0.,0.,0.,0.,1.,.5),30)),
        styles=(style(10,(.1,.2,.8)),style(20,(.8,.2,.1)),style(30,(.8,.7,.1)),style(40,(.1,.8,.3))),
        faces=(replace(src.faces[0],style_id=40),*src.faces[1:]))


def source_case(family):
    if family == 'box':
        return box_source(),'plane'
    if family == 'styled':
        return styled_source(),'plane'
    if family == 'cylinder':
        return cylinder_source(),'cylinder'
    if family == 'c0':
        return c0_source(),'c0_hanging'
    if family in ('north','cone'):
        return cap_source(family),family
    return curved_source('weighted'),'weighted'


def read_scene(path):
    r = Reader(path.read_bytes())
    assert r.unpack('<8sII') == (b'BGMESH01',1,1)
    nd,ni,nv,nt,nc,nm,nh,reserved = r.unpack('<8Q')
    assert reserved == 0 and max(nd,ni,nv,nt,nc,nm,nh) <= len(r.raw)//4
    definitions = [r.unpack('<5Q') for _ in range(nd)]
    instances = []
    for _ in range(ni):
        identifier,definition,material,*values = r.unpack('<QII15d')
        instances.append(dict(id=identifier,definition=definition,material=material,
                              matrix=values[:12],explosion=values[12:]))
    vertices = []
    for _ in range(nv):
        x,y,z,u,v,carrier,flags,physical = r.unpack('<5dIIQ')
        vertices.append(dict(xyz=(x,y,z),uv=(u,v),carrier=carrier,flags=flags,physical=physical))
    triangles = [r.unpack('<4IQ') for _ in range(nt)]
    carriers = []
    for _ in range(nc):
        identifier,kind,flags,*tail = r.unpack('<QII18dQQIIQ')
        assert tail[-1] == 0 and flags == 0
        carriers.append(dict(id=identifier,kind=kind,frame=tail[:12],radii=tail[12:14],box=tail[14:18],
                             controls=tail[18:20],degree=tuple(tail[20:22])))
    materials = [r.unpack('<4d') for _ in range(nm)]
    controls = [r.unpack('<4d') for _ in range(nh)]
    assert r.at == len(r.raw)
    return dict(definitions=definitions,instances=instances,vertices=vertices,triangles=triangles,
                carriers=carriers,materials=materials,controls=controls)


def unit(vector):
    length = math.sqrt(sum(float(x)**2 for x in vector))
    assert length > 0
    return tuple(float(x)/length for x in vector)


def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])


def source_normal(src,face,uv,display,kind,mapped=None):
    carrier = next(c for c in src.carriers if c.id == face.carrier_id)
    u,v = map(F,uv)
    if kind == 'plane':
        normal = cross(tuple(map(F,carrier.frame[3:6])),tuple(map(F,carrier.frame[6:9])))
    elif kind == 'cylinder':
        sine,cosine = cylinder_trig(u)
        normal = (sum(cosine)/2,sum(sine)/2,F(0))
    elif kind in ('north','cone'):
        x,y,z = (tuple(map(F,carrier.frame[a:a+3])) for a in (3,6,9))
        sine,cosine = (sum(box)/2 for box in cylinder_trig(u))
        e = tuple(cosine*a+sine*b for a,b in zip(x,y))
        ep = tuple(-sine*a+cosine*b for a,b in zip(x,y))
        if carrier.kind == 4:
            if mapped is not None:
                lo,hi = cylinder_trig(mapped['cut_v'])[1]
                assert lo > 0 or hi < 0, 'original cut sheet unresolved'
                sheet = 1 if lo > 0 else -1
                assert sheet == mapped['sheet']
                normal = tuple(sheet*(-1 if mapped['kind'] == 1 else 1)*a
                               for a in cross(x,y))
            else:
                sv,cv = (sum(box)/2 for box in cylinder_trig(v))
                normal = cross(tuple(cv*a for a in ep),
                               tuple(-sv*a+cv*b for a,b in zip(e,z)))
        else:
            sa,ca = (sum(box)/2 for box in cylinder_trig(F(carrier.radii[1])))
            normal = tuple(sa*a+ca*b for a,b in zip(cross(ep,e),cross(ep,z)))
            if mapped is not None:
                radii = [F(carrier.radii[0])+mapped['cut_v']*s
                         for s in cylinder_trig(F(carrier.radii[1]))[0]]
                assert min(radii) > 0 or max(radii) < 0, 'original cut sheet unresolved'
                scale = 1 if min(radii) > 0 else -1
                assert scale == mapped['sheet']
            else:
                scale = F(carrier.radii[0])+v*sa
            normal = tuple(scale*a for a in normal)
    elif kind == 'weighted':
        normal = (-2*v/(1+v),-u,F(1))
    else:
        assert kind == 'c0_hanging'
        if display['box'][1] == .5:
            zu,zv = F(1,4),v/4
        else:
            assert display['box'][0] == .5
            offset = u-F(1,2)
            zu,zv = -F(1,4)+4*offset+4*v*v,(F(1,4)+8*offset)*v
        normal = (-zu,-zv,F(1))
    return unit(tuple(face.orientation*x for x in normal))


def positive_overlap(triangle,box):
    polygon = triangle
    for side,bound in enumerate(map(F,box)):
        axis,sign = side//2,1 if side%2 == 0 else -1
        result = []
        for a,b in zip(polygon,polygon[1:]+polygon[:1]):
            da,db = a[axis]-bound,b[axis]-bound
            inside_a,inside_b = sign*da >= 0,sign*db >= 0
            if inside_a:
                result.append(a)
            if inside_a != inside_b:
                t = da/(da-db)
                result.append(tuple(x+t*(y-x) for x,y in zip(a,b)))
        polygon = result
    return any(orient(polygon[0],polygon[i],polygon[i+1]) > 0 for i in range(1,len(polygon)-1))


def close_vector(a,b):
    assert len(a) == len(b) and all(math.isfinite(float(x)) for x in a)
    assert all(abs(float(x)-float(y)) <= 1e-10*max(1.,abs(float(y))) for x,y in zip(a,b)), (a,b)


def check_output(src,kind,family,output,*,scene_path=None,face_paths=None,
                 expected_instances=None,expected_definitions=None,debug_outputs=True,
                 check_geometry=True):
    scene = read_scene(scene_path or output/'actual.bgscene')
    definitions = (sorted(d.id for d in src.definitions) if expected_definitions is None
                   else sorted(expected_definitions))
    assert [d[0] for d in scene['definitions']] == definitions
    vertices,carriers = scene['vertices'],scene['carriers']
    jets = None
    if debug_outputs:
        raw = (output/'actual.jets').read_bytes()
        assert len(raw) == len(vertices)*128
        jets = [struct.unpack_from('<15dII',raw,i*128) for i in range(len(vertices))]
    physical_to_source,source_to_physical = {},{}
    normal_expected = {}
    crease = defaultdict(set)
    used = set()
    for definition,first_vertex,vertex_count,first_triangle,triangle_count in scene['definitions']:
        source_definition = next(d for d in src.definitions if d.id == definition)
        actual_faces = [f for f in src.faces if f.id in source_definition.face_ids]
        face_triangles = defaultdict(list)
        for row in scene['triangles'][first_triangle:first_triangle+triangle_count]:
            assert max(row[:3]) < vertex_count
            face_triangles[row[4]].append(row)
        assert set(face_triangles) == set(source_definition.face_ids)
        for face in actual_faces:
            paths = ((str(face.id)+'.bgacrv',str(face.id)+'.bgatrm') if face_paths is None
                     else face_paths[face.id])
            boundary = read_curves(output/paths[0])
            mesh = read_face(output/paths[1])
            if check_geometry and kind == 'cylinder':
                check_seam(boundary,src,mesh['upper'],mesh['node_source'])
            elif check_geometry and kind not in ('north','cone'):
                def value(use,uv):
                    if kind != 'plane':
                        return source_value(kind,*uv)
                    loop = next(loop for loop in src.loops if loop.id == use.loop_id)
                    f = next(f for f in src.faces if f.id == loop.face_id)
                    original = next(c for c in src.carriers if c.id == f.carrier_id)
                    return tuple(F(original.frame[a])+uv[0]*F(original.frame[3+a])+uv[1]*F(original.frame[6+a]) for a in range(3))
                check_curves(boundary,src,mesh['upper'],mesh['node_source'],'plain',value)
            if check_geometry:
                qualify_face(src,boundary,mesh,mesh['upper'],mesh['node_source'],kind,face=face)
            assert len(face_triangles[face.id]) == len(mesh['facets'])
            for facet_index,(row,facet) in enumerate(zip(face_triangles[face.id],mesh['facets'])):
                roles = facet['sites'] if face.orientation > 0 else (facet['sites'][0],facet['sites'][2],facet['sites'][1])
                exact_triangle = [mesh['sites'][i]['uv'] for i in facet['sites']]
                for local,site_id in zip(row[:3],roles):
                    global_id = first_vertex+local
                    used.add(global_id)
                    display,site = vertices[global_id],mesh['sites'][site_id]
                    assert tuple(map(F,display['xyz'])) == site['xyz']
                    mapped = mesh['mapped'].get(facet_index)
                    pole = mapped is not None and site_id == facet['sites'][mapped['roles'][0]]
                    expected_uv = site['uv']
                    flags = 33+(2 if face.orientation < 0 else 0)
                    if pole:
                        assert mapped['kind'] == (0 if kind == 'north' else 2)
                        piece = mesh['cap_pieces'][mapped['piece']]
                        mapped = dict(mapped,cut_v=(piece['uv'][1]+piece['uv'][3])/2)
                        expected_uv = ((piece['uv'][0]+piece['uv'][2])/2,sum(piece['finite'])/2)
                        assert mapped['sector'] == (piece['uv'][0],piece['uv'][2])
                        flags |= 4+(8 if mapped['sheet'] < 0 else 0)+(16 if mapped['kind'] == 1 else 0)
                    assert display['uv'] == tuple(float(x) for x in expected_uv)
                    assert display['flags'] == flags
                    carrier = carriers[display['carrier']]
                    original = next(c for c in src.carriers if c.id == face.carrier_id)
                    assert carrier['id'] == original.id and carrier['kind'] == original.kind
                    if carrier['kind'] == 6:
                        assert all(F(carrier['box'][2*a]) <= site['uv'][a] <= F(carrier['box'][2*a+1]) for a in (0,1))
                        assert positive_overlap(exact_triangle,carrier['box']), 'normal root is outside actual facet sector'
                        span = next(s for s in original.spans if s.bounds == tuple(carrier['box']))
                        first,count = carrier['controls']
                        assert carrier['degree'] == span.degrees
                        assert scene['controls'][first:first+count] == [tuple(span.controls[i:i+4]) for i in range(0,len(span.controls),4)]
                        if kind == 'c0_hanging' and site['uv'][0] == F(1,2):
                            crease[display['physical'],site['uv']].add(tuple(carrier['box']))
                    else:
                        assert tuple(carrier['frame']) == original.frame
                    if site['kind'] == 1:
                        key = ('source',definition,*boundary['nodes'][site['source'][0]]['key'])
                    else:
                        key = ('face',definition,face.id,site_id)
                    assert physical_to_source.setdefault(display['physical'],key) == key
                    assert source_to_physical.setdefault(key,display['physical']) == display['physical']
                    expected = source_normal(src,face,display['uv'],carrier,kind,mapped if pole else None)
                    if jets is not None:
                        close_vector(jets[global_id][9:12],expected)
                        assert struct.pack('<3d',*jets[global_id][12:15]) == struct.pack('<3d',*display['xyz'])
                        assert jets[global_id][-1] & 2
                    normal_expected[global_id] = expected
                style = face.style_id
                assert row[3] == (next(i for i,s in enumerate(src.styles) if s.id == style)+1 if style else 2**32-1)
    assert used == set(range(len(vertices)))
    assert set(physical_to_source) == set(range(1,len(physical_to_source)+1))
    if family == 'c0':
        assert any(len(roots) == 2 for roots in crease.values()), 'C0 sides collapsed to canonical point root'
    assert scene['materials'] == [(0.7,0.7,0.7,0.5)]+[(*s.rgba[:3],.5) for s in src.styles]
    if expected_instances is None:
        expected_matrices = [u.transform for u in src.usages] if family == 'styled' else [(1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.)]
        assert [tuple(i['matrix']) for i in scene['instances']] == expected_matrices
        assert [i['material'] for i in scene['instances']] == ([3,4] if family == 'styled' else [1])
    else:
        assert len(scene['instances']) == len(expected_instances)
        for actual,expected in zip(scene['instances'],expected_instances):
            identifier,definition,matrix,material = expected
            assert actual['id'] == identifier
            assert scene['definitions'][actual['definition']][0] == definition
            assert tuple(actual['matrix']) == matrix and actual['material'] == material
    for frame in ((0,1) if debug_outputs else ()):
        raw = (output/f'frame-{frame}.projected').read_bytes()
        assert len(raw) == sum(scene['definitions'][i['definition']][2] for i in scene['instances'])*96
        at = 0
        for instance in scene['instances']:
            identifier,first,count,_,_ = scene['definitions'][instance['definition']]
            m = tuple(map(F,instance['matrix']))
            columns = tuple(tuple(m[4*a+b] for a in range(3)) for b in range(3))
            cofactor = (cross(columns[1],columns[2]),cross(columns[2],columns[0]),cross(columns[0],columns[1]))
            for local in range(count):
                projected = struct.unpack_from('<3Q2q7d',raw,at*96); at += 1
                assert projected[:3] == (instance['id'],identifier,first+local)
                vertex = vertices[first+local]
                xyz = tuple(map(F,vertex['xyz']))
                world = tuple(sum(m[4*a+b]*xyz[b] for b in range(3))+m[4*a+3]+frame*F(instance['explosion'][a]) for a in range(3))
                close_vector(projected[6:9],world)
                normal = normal_expected[first+local]
                close_vector(projected[9:12],unit(tuple(sum(cofactor[b][a]*F(normal[b]) for b in range(3)) for a in range(3))))
    return len(scene['triangles'])


def run(options,root):
    src,kind = source_case(options.family)
    original = root/'original.bgsource'; write_source(src,original)
    output = root/'native'
    command = [str(options.executable.resolve()),str(original),str(output)]
    if options.module:
        command.extend((str(options.module.resolve()),str(options.device)))
    if options.render:
        command.append('--render')
    proc = subprocess.run(command,text=True,capture_output=True,timeout=120)
    (root/'stdout.txt').write_text(proc.stdout); (root/'stderr.txt').write_text(proc.stderr)
    assert proc.returncode == 0,(proc.returncode,proc.stderr)
    count = check_output(src,kind,options.family,output)
    if options.render:
        assert all((output/f'frame-{i}.png').read_bytes().startswith(b'\x89PNG\r\n\x1a\n') for i in (0,1))
    print(proc.stdout.strip())
    print(f'original artifact/normal/placement display PASS\t{options.family}\t{count} facets')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable',type=Path,required=True)
    parser.add_argument('--family',choices=('box','cylinder','weighted','c0','styled','north','cone'),required=True)
    parser.add_argument('--workspace',type=Path)
    parser.add_argument('--module',type=Path)
    parser.add_argument('--device',type=int,choices=(0,1),default=0)
    parser.add_argument('--render',action='store_true')
    options = parser.parse_args()
    if options.workspace:
        options.workspace.mkdir(parents=True,exist_ok=False)
        run(options,options.workspace.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='native-adaptive-scene-') as temporary:
            run(options,Path(temporary))


if __name__ == '__main__':
    main()
