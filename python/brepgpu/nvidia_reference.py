"""Genuine NVIDIA mesh records and the reference-only assembly boundary.

No surface construction, tessellation, repaired normals or replacement geometry.
"""
from dataclasses import dataclass
import math
from pathlib import Path
import struct


def require(value, message):
    if not value:
        raise ValueError(message)


@dataclass(frozen=True)
class ReferenceDefinitionMesh:
    id: int
    points: tuple
    normals: tuple
    physical_keys: tuple
    triangles: tuple
    face_ids: tuple
    vertex_bytes: bytes


def load_definition(path, *, expected_id=None):
    data = Path(path).read_bytes()
    require(len(data) >= 64, 'reference mesh header truncated')
    h = struct.unpack_from('<8sII6Q', data)
    require(h[:3] == (b'BGREFM01', 1, 0) and h[-1] == 0, 'reference mesh header invalid')
    nd, ni, nv, nt, nm = h[3:8]
    require(nd == 1 and ni == 0 and nm == 1 and 0 < nv <= 0xffffffff and 0 < nt <= 0xffffffff,
            'producer must emit one nonempty definition, zero instances, one material')
    require(len(data) == 64+40+56*nv+24*nt+32, 'reference mesh section sizes differ')
    id, fv, vc, ft, tc = struct.unpack_from('<5Q', data, 64)
    require(id != 0xffffffffffffffff and (fv, vc, ft, tc) == (0, nv, 0, nt),
            'reference definition range/ID invalid')
    require(expected_id is None or id == expected_id, 'reference original definition ID differs')
    vertex_bytes = data[104:104+56*nv]
    points, normals, keys, physical = [], [], [], {}
    for row in struct.iter_unpack('<6dQ', vertex_bytes):
        p, n, key = row[:3], row[3:6], row[6]
        require(all(map(math.isfinite, (*p, *n))) and 0 < math.hypot(*n) < math.inf,
                'reference point/normal is nonfinite or zero')
        require(key not in physical or physical[key] == p, 'reference physical key changed coordinates')
        physical[key] = p
        points.append(p); normals.append(n); keys.append(key)
    triangles, faces, used = [], [], set()
    for a, b, c, material, face in struct.iter_unpack('<4IQ', data[104+56*nv:-32]):
        require(len({a,b,c}) == 3 and max(a,b,c) < nv and material == 0,
                'reference triangle index/material invalid')
        require(len({keys[a],keys[b],keys[c]}) == 3, 'reference physical triangle collapsed')
        triangles.append((a,b,c)); faces.append(face); used.update((a,b,c))
    require(used == set(range(nv)), 'reference normal corners unused')
    material = struct.unpack_from('<4d', data, len(data)-32)
    require(all(map(math.isfinite, material)) and all(0 <= x <= 1 for x in material[:3]) and
            0 < material[3] <= 1, 'reference placeholder material invalid')
    return ReferenceDefinitionMesh(id, tuple(points), tuple(normals), tuple(keys),
                                   tuple(triangles), tuple(faces), vertex_bytes)


def write_assembly(path, meshes, assembly, offsets, materials, definition_materials):
    """Pack genuine local corner bytes once; add only original placed instances."""
    meshes, materials = tuple(meshes), tuple(materials)
    slots = {mesh.id: i for i,mesh in enumerate(meshes)}
    require(len(slots) == len(meshes), 'duplicate reference definition')
    require(len(offsets) == len(assembly.occurrences), 'reference explosion incomplete')
    require(set(definition_materials) == set(slots), 'reference material definitions incomplete')
    for slot in definition_materials.values():
        require(type(slot) is int and 0 <= slot < len(materials), 'reference material slot invalid')
    nv, nt = sum(len(m.points) for m in meshes), sum(len(m.triangles) for m in meshes)
    require(max(nv,nt,len(assembly.occurrences),len(meshes),len(materials)) <= 0xffffffff,
            'reference aggregate exceeds native slots')
    with Path(path).open('xb') as f:
        f.write(struct.pack('<8sII6Q', b'BGREFM01',1,0,len(meshes),len(assembly.occurrences),nv,nt,len(materials),0))
        fv=ft=0
        for m in meshes:
            f.write(struct.pack('<5Q',m.id,fv,len(m.points),ft,len(m.triangles)))
            fv+=len(m.points);ft+=len(m.triangles)
        for occurrence,offset in zip(assembly.occurrences,offsets):
            require(occurrence.definition_id in slots, 'reference placed definition absent')
            require(len(offset)==3 and all(map(math.isfinite,offset)), 'reference explosion offset invalid')
            f.write(struct.pack('<QII15d',occurrence.id,slots[occurrence.definition_id],0,
                                *occurrence.transform,*offset))
        for m in meshes:
            f.write(m.vertex_bytes)
        for m in meshes:
            material=definition_materials[m.id]
            for triangle,face in zip(m.triangles,m.face_ids):
                f.write(struct.pack('<4IQ',*triangle,material,face))
        for rgb,roughness in materials:
            require(len(rgb)==3 and all(math.isfinite(x) and 0<=x<=1 for x in rgb) and
                    math.isfinite(roughness) and 0<roughness<=1,'reference palette invalid')
            f.write(struct.pack('<4d',*rgb,roughness))


def qualify_physical_definition(mesh, source, definition):
    """Reuse the existing independent shell mechanism on genuine external keys."""
    from .imported_oracles import qualify_physical_solid
    vertices=dict(zip(mesh.physical_keys,mesh.points))
    triangles=tuple(tuple(mesh.physical_keys[i] for i in t) for t in mesh.triangles)
    stats=qualify_physical_solid(vertices,triangles)
    euler=(len(definition.vertex_ids)-len(definition.edge_ids)+
           sum(2-len(source.face(id).loop_ids) for id in definition.face_ids))
    require(stats['euler']==euler,'reference physical shell differs from original Euler characteristic')
    return stats


def write_source_witnesses(scene_path, output_directory, definitions):
    """Select finite probes from an already qualified owned physical mesh.

    Qualification provenance belongs to the caller; this does not relabel an
    arbitrary BGMESH01 as qualified. Geometry never enters the NVIDIA display.
    """
    data=Path(scene_path).read_bytes()
    require(len(data)>=80,'owned witness scene header truncated')
    h=struct.unpack_from('<8sII8Q',data)
    require(h[0]==b'BGMESH01' and h[1]==1 and h[2] in (0,1) and h[-1]==0,
            'owned witness scene header/profile invalid')
    nd,ni,nv,nt,nc,nm,nh=h[3:10]
    require(len(data)==80+40*nd+136*ni+56*nv+24*nt+192*nc+32*nm+32*nh,
            'owned witness scene lengths differ')
    defs=tuple(struct.unpack_from('<5Q',data,80+40*i) for i in range(nd))
    expected={d.id:set(d.face_ids) for d in definitions}
    require(len(expected)==nd and {row[0] for row in defs}==set(expected),
            'owned witness original definitions differ')
    vertex_start=80+40*nd+136*ni;triangle_start=vertex_start+56*nv
    output=Path(output_directory);output.mkdir(parents=True,exist_ok=False)
    previous_v=previous_t=0;counts=[]
    for id,fv,vc,ft,tc in defs:
        require((fv,ft)==(previous_v,previous_t) and vc>0 and tc>0 and fv+vc<=nv and ft+tc<=nt,
                'owned witness definition ranges differ')
        previous_v,previous_t=fv+vc,ft+tc
        groups={};triangles=[]
        for local in range(tc):
            a,b,c,material,face=struct.unpack_from('<4IQ',data,triangle_start+24*(ft+local))
            require(len({a,b,c})==3 and max(a,b,c)<vc,'owned witness triangle invalid')
            require(face in expected[id],'owned witness source face absent')
            triangles.append(((a,b,c),face));groups.setdefault(face,[]).append(local)
        require(set(groups)==expected[id],'owned witness source face coverage incomplete')
        count=min(64,tc)
        chosen={rows[len(rows)//2] for rows in groups.values()}
        chosen.update(((2*i+1)*tc)//(2*count) for i in range(count))
        with (output/f'definition-{id}.bgprobe').open('xb') as f:
            f.write(struct.pack('<8sQ',b'BGRPROB1',len(chosen)))
            for local in sorted(chosen):
                indices,face=triangles[local]
                xyz=tuple(struct.unpack_from('<3d',data,vertex_start+56*(fv+i)) for i in indices)
                require(all(math.isfinite(x) for p in xyz for x in p),'owned witness point nonfinite')
                a,b,c=xyz
                ab=tuple(b[k]-a[k] for k in range(3));ac=tuple(c[k]-a[k] for k in range(3))
                cross=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
                require(all(map(math.isfinite,cross)) and math.hypot(*cross)>0,'owned witness triangle collapsed')
                centroid=tuple(math.fsum(p[k]/3 for p in xyz) for k in range(3))
                f.write(struct.pack('<QQ3d',face,ft+local,*centroid))
        counts.append((id,len(chosen)))
    require((previous_v,previous_t)==(nv,nt),'owned witness geometry unowned')
    return tuple(counts)


@dataclass(frozen=True)
class NativeModelMatch:
    source: object
    scene_path: Path
    views_path: Path
    labels_path: Path
    assembly: object
    offsets: tuple


def load_native_model_match(directory, query, tolerance, *, source_sha256=None, reference_source=None):
    """Admit current published model transport; this is not geometry qualification."""
    import csv
    from dataclasses import replace
    from fractions import Fraction as F
    from .imported_source import load_source
    from .imported_assembly import assemble
    directory = Path(directory)
    def rows(path):
        with path.open(newline='', encoding='utf-8') as stream:
            return tuple(csv.DictReader(stream, delimiter='\t'))
    def confined(name):
        path = directory / name
        require(not Path(name).is_absolute() and path.resolve().is_relative_to(directory.resolve())
                and path.is_file(), 'native model artifact missing or outside model root')
        return path
    fields = rows(directory/'model.tsv')
    metadata = {r['key']:r['value'] for r in fields}
    require(len(metadata) == len(fields) and metadata.get('scope') == 'complete_assembly',
            'reference requires complete native model assembly')
    source = load_source(directory/'source.bgsource')
    require(metadata.get('source_digest') == source.source_sha256 and
            (source_sha256 is None or source.source_sha256 == source_sha256),
            'native model original source digest differs')
    if reference_source is not None:
        require(source.products == reference_source.products and source.usages == reference_source.usages
                and source.styles == reference_source.styles and
                source.root_product_id == reference_source.root_product_id,
                'fresh reference and native original graph/styles differ')
        require({(d.id,d.product_id,tuple(sorted(d.face_ids))) for d in source.definitions} ==
                {(d.id,d.product_id,tuple(sorted(d.face_ids))) for d in reference_source.definitions},
                'fresh reference and native original definition/face inventory differs')
    definitions = sorted(source.definitions, key=lambda d:d.id)
    assembly = assemble(source)
    require(int(metadata['selected_definitions']) == int(metadata['source_definitions']) == len(definitions)
            and int(metadata['source_faces']) == len(source.faces)
            and int(metadata['source_placements']) == len(assembly.occurrences),
            'native model complete inventory metadata differs')
    queries = rows(directory/'queries.tsv')
    word = [(int(r['query']),int(r['definition'])) for r in queries]
    nq = max((q for q,_ in word),default=-1)+1
    require(type(query) is int and 0 <= query < nq and
            len(word) == nq*len(definitions) and word == [(q,d.id) for q in range(nq) for d in definitions],
            'native model query definition inventory differs')
    selected = [r for r in queries if int(r['query']) == query]
    require(all(float(r['requested_positional_mm']) == tolerance and
                math.isfinite(float(r['requested_node_mm'])) and float(r['requested_node_mm']) > 0
                for r in selected), 'native model requested quality differs')
    faces = rows(directory/'faces.tsv')
    require([(int(r['query']),int(r['definition']),int(r['face'])) for r in faces] ==
            [(q,d.id,f) for q in range(nq) for d in definitions for f in sorted(d.face_ids)],
            'native model complete face index differs')
    for row in faces:
        confined(row['mesh']); confined(row['boundary'])
    scene_path = confined(f'query-{query}/scene.bgscene')
    views_path = confined(f'query-{query}/views.bgview')
    labels_path = confined(f'query-{query}/labels.tsv')
    data = scene_path.read_bytes()
    require(len(data) >= 80, 'native model scene header truncated')
    h = struct.unpack_from('<8sII8Q',data)
    require(h[:3] == (b'BGMESH01',1,1) and h[-1] == 0,
            'native model scene requires explicit source geometry profile1')
    nd,ni,nv,nt,nc,nm,nh = h[3:10]
    require(len(data) == 80+40*nd+136*ni+56*nv+24*nt+192*nc+32*nm+32*nh,
            'native model scene section sizes differ')
    scene_definitions = tuple(struct.unpack_from('<5Q',data,80+40*i) for i in range(nd))
    require([r[0] for r in scene_definitions] == [d.id for d in definitions]
            and ni == len(assembly.occurrences), 'native model scene inventory differs')
    vertex_start = 80+40*nd+136*ni
    triangle_start = vertex_start+56*nv
    previous_v=previous_t=0
    for d,(_,fv,vc,ft,tc) in zip(definitions,scene_definitions):
        require((fv,ft) == (previous_v,previous_t) and vc > 0 and tc > 0
                and fv+vc <= nv and ft+tc <= nt, 'native model scene partitions differ')
        seen=set()
        for at in range(ft,ft+tc):
            a,b,c,material,face=struct.unpack_from('<4IQ',data,triangle_start+24*at)
            require(len({a,b,c}) == 3 and max(a,b,c) < vc and (material < nm or material == 0xffffffff),
                    'native model witness triangle index/material invalid')
            seen.add(face)
        require(seen == set(d.face_ids), 'native model scene omitted original face')
        previous_v,previous_t=fv+vc,ft+tc
    require((previous_v,previous_t) == (nv,nt), 'native model scene has unowned geometry')
    if reference_source is not None:
        expected_faces={f.id:f for f in reference_source.faces}
        require(all(f.id in expected_faces and
                    (f.carrier_id,f.orientation,f.style_id) ==
                    (expected_faces[f.id].carrier_id,expected_faces[f.id].orientation,expected_faces[f.id].style_id)
                    for f in source.faces), 'fresh reference original face metadata differs')
    uses = {u.id:u for u in source.usages}
    identity = tuple(F(int(i//4 == i%4)) if i%4 != 3 else F(0) for i in range(12))
    def exact_transform(path):
        matrix=identity
        for identifier in path:
            rhs=tuple(map(F,uses[identifier].transform))
            matrix=tuple(sum((matrix[4*r+k]*rhs[4*k+c] for k in range(3)),F(0))+
                         (matrix[4*r+3] if c == 3 else 0)
                         for r in range(3) for c in range(4))
        return tuple(map(float,matrix))
    actual=[]; offsets=[]
    for i,occurrence in enumerate(assembly.occurrences):
        row=struct.unpack_from('<QII15d',data,80+40*nd+136*i)
        require(row[0] == occurrence.id and row[1] < nd and
                scene_definitions[row[1]][0] == occurrence.definition_id and row[2] <= nm
                and all(map(math.isfinite,row[3:])), 'native model original instance differs')
        matrix=row[3:15]
        require(struct.pack('<12d',*matrix) == struct.pack('<12d',*exact_transform(occurrence.path)),
                'native model placement differs from exact original composition')
        actual.append(replace(occurrence,transform=matrix)); offsets.append(row[15:18])
    labels=rows(labels_path)
    def escaped(text):
        return text.replace('\\','\\\\').replace('\t','\\t').replace('\n','\\n').replace('\r','\\r')
    products={p.id:p for p in source.products}
    require(len(labels) == len(actual), 'native model label inventory differs')
    for i,(row,occurrence) in enumerate(zip(labels,actual)):
        require((int(row['label']),int(row['occurrence']),int(row['definition']),int(row['product_id'])) ==
                (i+1,occurrence.id,occurrence.definition_id,occurrence.product_id)
                and row['usage_ids'] == '/'.join(map(str,occurrence.path))
                and row['product_name'] == escaped(products[occurrence.product_id].name)
                and row['usage_names'] == '/'.join(escaped(uses[u].name) for u in occurrence.path),
                'native model label/source path differs')
    return NativeModelMatch(source,scene_path,views_path,labels_path,
                            replace(assembly,occurrences=tuple(actual)),tuple(offsets))
