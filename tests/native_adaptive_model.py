"""Complete original two-definition assembly -> model artifacts and display buffers.

Reuses the component source/chain/facet oracle and scene structural reader. Native
runs are deliberately finite; --saved-output only reads retained artifacts.
"""
from dataclasses import replace
from fractions import Fraction as F
from pathlib import Path
import argparse
import math
import struct
import subprocess
import tempfile

from brepgpu.imported_source import Product, Usage, Style, write_source, load_source
from native_adaptive_component import (box_source, check_output as check_component,
    read_tsv, exact_metadata)
from native_adaptive_trimmed_caller import read_face, face_semantics
from native_adaptive_scene import check_output as check_scene
from native_mesher_cli import png

IDENTITY = (1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.)
QUALITIES = (.01,.02,.01)


def model_source():
    first = box_source()
    offset, scales = 10000, (2.,1.,.5)
    def point(values):
        return tuple(v*s for v,s in zip(values,scales))
    def curve(row):
        controls = list(row.controls)
        if row.dimension == 3:
            for at in range(0,len(controls),4):
                controls[at:at+3] = point(controls[at:at+3])
        return replace(row,id=row.id+offset,controls=tuple(controls))
    second_faces = tuple(replace(f,id=f.id+offset,definition_id=201,
        carrier_id=f.carrier_id+offset,loop_ids=tuple(i+offset for i in f.loop_ids))
        for f in first.faces)
    second = replace(first.definitions[0],id=201,product_id=102,
        face_ids=tuple(i+offset for i in first.definitions[0].face_ids),
        edge_ids=tuple(i+offset for i in first.definitions[0].edge_ids),
        vertex_ids=tuple(i+offset for i in first.definitions[0].vertex_ids))
    def style(identifier,rgb):
        return Style(identifier,(),(*rgb,1.),True)
    return replace(first,source_sha256='d4'*32,
        products=(Product(100,'root','root'),Product(101,'first\tbox\\name','first',200,10),
                  Product(102,'second\nbox','second',201,11)),
        definitions=(replace(first.definitions[0],product_id=101),second),
        usages=(Usage(710,100,101,'direct\tusage',IDENTITY,20),
                Usage(711,100,101,'quarter\\turn',(0.,-1.,0.,3.,1.,0.,0.,0.,0.,0.,1.,.5),30),
                Usage(712,100,102,'last\nusage',(1.,0.,0.,6.,0.,1.,0.,0.,0.,0.,1.,0.),0)),
        vertices=(*first.vertices,*(replace(v,id=v.id+offset,xyz=point(v.xyz)) for v in first.vertices)),
        curves=(*first.curves,*(curve(c) for c in first.curves)),
        edges=(*first.edges,*(replace(e,id=e.id+offset,curve_id=e.curve_id+offset,
            vertex_ids=tuple(i+offset for i in e.vertex_ids)) for e in first.edges)),
        coedges=(*first.coedges,*(replace(u,id=u.id+offset,edge_id=u.edge_id+offset,
            loop_id=u.loop_id+offset,curve_id=u.curve_id+offset) for u in first.coedges)),
        loops=(*first.loops,*(replace(l,id=l.id+offset,face_id=l.face_id+offset,
            coedge_ids=tuple(i+offset for i in l.coedge_ids)) for l in first.loops)),
        faces=(replace(first.faces[0],style_id=50),*first.faces[1:],*second_faces),
        carriers=(*first.carriers,*(replace(c,id=c.id+offset,
            frame=tuple(x for at in range(0,12,3) for x in point(c.frame[at:at+3]))) for c in first.carriers)),
        styles=(style(10,(.1,.2,.8)),style(11,(.2,.7,.3)),style(20,(.8,.2,.1)),
                style(30,(.8,.7,.1)),style(50,(.1,.8,.3))))


def escaped(value):
    return value.replace('\\','\\\\').replace('\t','\\t').replace('\n','\\n').replace('\r','\\r')


def original_instances(src,definitions):
    products = {p.id:p for p in src.products}
    result = []
    for ordinal,usage in enumerate(src.usages,1):
        product = products[usage.child_product_id]
        if product.definition_id in definitions:
            style = usage.style_id or product.style_id
            material = next(i for i,s in enumerate(src.styles) if s.id == style)+2 if style else 1
            result.append((ordinal,product.definition_id,usage.transform,material))
    return result


def check_views(path,width,height):
    raw = path.read_bytes()
    assert struct.unpack_from('<8sII',raw) == (b'BGVIEW01',1,2)
    fmt = '<4I19dII2d'; size = struct.calcsize(fmt)
    assert len(raw) == 16+2*size
    views = [struct.unpack_from(fmt,raw,16+i*size) for i in range(2)]
    for i,row in enumerate(views):
        assert row[:4] == (width,height,1,0) and row[4] == i
        assert row[-3:] == (0,0.,0.)
    # Presentation fits each frame separately; orientation/pixel center/background
    # are shared, while center and scale follow the actual exploded extents.
    assert views[0][5:14] == views[1][5:14]
    assert views[0][17:19] == views[1][17:19] == (width*.5,height*.54)
    assert views[0][20:] == views[1][20:]
    assert all(math.isfinite(x) for row in views for x in row)
    assert all(row[19] > 0 for row in views)


def check_output(src,output,qualities,definitions=None,mesh_only=False,width=320,height=240):
    definitions = sorted(d.id for d in src.definitions) if definitions is None else sorted(definitions)
    assert (output/'source.bgsource').read_bytes() == write_bytes(src,output.parent/'expected.bgsource')
    index,queries = read_tsv(output/'faces.tsv'),read_tsv(output/'queries.tsv')
    expected_faces = [(q,d,face) for q in range(len(qualities)) for d in definitions
        for face in sorted(next(row for row in src.definitions if row.id == d).face_ids)]
    assert [(int(row['query']),int(row['definition']),int(row['face'])) for row in index] == expected_faces
    assert len(queries) == len(qualities)*len(definitions)
    assert [(int(r['query']),int(r['definition'])) for r in queries] == [
        (q,d) for q in range(len(qualities)) for d in definitions]
    metadata = {row['key']:row['value'] for row in read_tsv(output/'model.tsv')}
    assert int(metadata['selected_definitions']) == len(definitions)
    assert int(metadata['source_definitions']) == len(src.definitions)
    assert int(metadata['source_faces']) == len(src.faces)
    if metadata['backend'] == 'p100':
        for field in ('curve_device_points_query','curve_device_bounds_query',
                      'curve_uploaded_bytes_query','curve_downloaded_bytes_query',
                      'surface_device_points_cumulative','surface_device_bounds_cumulative',
                      'surface_uploaded_bytes_cumulative','surface_downloaded_bytes_cumulative'):
            assert sum(int(row[field]) for row in queries) > 0,field
    expected = original_instances(src,definitions)
    geometry = []
    for q,quality in enumerate(qualities):
        selected = [r for r in index if int(r['query']) == q]
        face_paths = {}
        query_geometry = []
        for identifier in definitions:
            definition = next(d for d in src.definitions if d.id == identifier)
            rows = [r for r in selected if int(r['definition']) == identifier]
            row = next(r for r in queries if int(r['query']) == q and int(r['definition']) == identifier)
            local,node = exact_metadata(row['local_positional_exact']),exact_metadata(row['local_node_exact'])
            assert local == F(quality) and node == F(1e-8)
            assert float(row['requested_positional_mm']) == quality and float(row['requested_node_mm']) == 1e-8
            paths = []
            prefix = Path(f'query-{q}')/f'definition-{identifier}'
            for face in rows:
                for field in ('mesh','boundary'):
                    path = Path(face[field])
                    assert not path.is_absolute() and '..' not in path.parts and path.parent == prefix
                mesh = read_face(output/face['mesh'])
                assert int(face['boundary_epoch']) >= 0
                assert int(face['face_boundary_generation']) == mesh['key'][2]
                assert mesh['upper'] == local and mesh['node_source'] == node
                paths.append((face['face'],face['boundary'],face['mesh']))
                face_paths[int(face['face'])] = (face['boundary'],face['mesh'])
                query_geometry.append((int(face['face']),face_semantics(mesh)))
            final = read_tsv(output/prefix/'final-boundary.tsv')
            assert len(final) == 1
            count = check_component(src,'box',output,local,paths,output/prefix/final[0]['snapshot'],definition,node)
            assert len(rows) == int(row['faces']) == len(definition.face_ids)
            assert count == int(row['facets']) and int(row['contact_forbidden_pairs']) == 0
        geometry.append(query_geometry)
        labels = read_tsv(output/f'query-{q}'/'labels.tsv')
        assert len(labels) == len(expected)
        for number,(label,(occurrence,definition,_,_)) in enumerate(zip(labels,expected),1):
            usage = src.usages[occurrence-1]
            product = next(p for p in src.products if p.id == usage.child_product_id)
            assert label == dict(label=str(number),
                occurrence=str(occurrence),definition=str(definition),product_id=str(product.id),
                product_name=escaped(product.name),usage_ids=str(usage.id),usage_names=escaped(usage.name))
        directory = output/f'query-{q}'
        if mesh_only:
            assert all(not (directory/name).exists() for name in ('scene.bgscene','views.bgview','assembled.png','exploded.png'))
        else:
            triangles = check_scene(src,'plane','model',output,scene_path=directory/'scene.bgscene',
                face_paths=face_paths,expected_instances=expected,expected_definitions=definitions,
                debug_outputs=False,check_geometry=False)
            assert triangles == sum(int(r['facets']) for r in queries if int(r['query']) == q)
            check_views(directory/'views.bgview',width,height)
            png(directory/'assembled.png',width,height,geometry=True)
            png(directory/'exploded.png',width,height,geometry=True)
    if len(qualities) == 3:
        assert geometry[0] == geometry[2], 'fine/coarse/fine source geometry/ancestry changed'
    timings = read_tsv(output/'timings.tsv')
    assert all(float(row['seconds']) >= 0 for row in timings)
    scopes = {row['scope'] for row in timings}
    assert {'source.decode','source.compile_banks','prepublication_horizon'} <= scopes
    return geometry


def compare_outputs(reference,output):
    """Compare already original-checked artifacts; routing counters stay explicit."""
    left,right = read_tsv(reference/'faces.tsv'),read_tsv(output/'faces.tsv')
    assert len(left) == len(right)
    exact = 0
    for a,b in zip(left,right):
        assert tuple(a[k] for k in ('query','definition','face')) == tuple(b[k] for k in ('query','definition','face'))
        p,q = reference/a['mesh'],output/b['mesh']
        exact += p.read_bytes() == q.read_bytes()
        assert face_semantics(read_face(p)) == face_semantics(read_face(q))
        assert (reference/a['boundary']).read_bytes() == (output/b['boundary']).read_bytes()
    for query in sorted({int(r['query']) for r in left}):
        for filename in ('scene.bgscene','views.bgview','labels.tsv'):
            assert (reference/f'query-{query}'/filename).read_bytes() == (output/f'query-{query}'/filename).read_bytes()
    print(f'PASS\tmatched model artifacts\tmesh bytes {exact}/{len(left)}\tall mesh semantics/boundary/scene/views/labels equal')


def write_bytes(src,path):
    write_source(src,path)
    return path.read_bytes()


def command(options,source,output,qualities):
    args = [str(options.executable.resolve()),'--source',str(source),'--output',str(output),
        '--node-source-mm','1e-8','--threads',str(options.threads),'--face-owners',str(min(2,options.threads)),
        '--width','320','--height','240','--explosion','.8']
    for quality in qualities:
        args.extend(('--positional-mm',str(quality)))
    if options.module:
        args.extend(('--backend','p100','--cuda-module',str(options.module.resolve()),'--device',str(options.device)))
        assert options.display_module
        args.extend(('--display-module',str(options.display_module.resolve())))
    return args


def invoke(args,root,name):
    proc = subprocess.run(args,text=True,capture_output=True,timeout=120)
    (root/f'{name}.stdout').write_text(proc.stdout)
    (root/f'{name}.stderr').write_text(proc.stderr)
    return proc


def run(options,root):
    src = model_source(); source = root/'original.bgsource'; write_source(src,source)
    args = command(options,source,root/'native',QUALITIES)
    if not options.resume:
        proc = invoke(args,root,'native')
        assert proc.returncode == 0,(proc.returncode,proc.stdout,proc.stderr)
    check_output(src,root/'native',QUALITIES)
    if options.ports:
        assert not options.module, 'transaction ports run once on CPU'
        for name,extra,definitions,mesh_only in (
            ('subset',['--definition','201'],[201],False),
            ('mesh-only',['--mesh-only'],None,True)):
            target = root/name
            completed = invoke(command(options,source,target,(.01,))+extra,root,name)
            assert completed.returncode == 0,completed.stderr
            check_output(src,target,(.01,),definitions,mesh_only)
        sentinel = root/'existing'; sentinel.mkdir(); (sentinel/'sentinel').write_bytes(b'existing-result\n')
        refused = invoke(command(options,source,sentinel,(.01,)),root,'existing')
        assert refused.returncode == 1 and 'output already exists; publication never replaces it' in refused.stderr
        assert list(sentinel.iterdir()) == [sentinel/'sentinel'] and (sentinel/'sentinel').read_bytes() == b'existing-result\n'
        cases = [('unplaced',replace(src,usages=src.usages[:2]),['--definition','201'],
                  ('source.envelopes_and_initial_assembly','0','selected definition has no original placement')),
                 ('source-node',replace(src,vertices=tuple(replace(v,xyz=(*v.xyz[:2],v.xyz[2]+.125))
                    if v.id == 10300 else v for v in src.vertices)),[],
                  ('query.complete_component','3','original endpoint source-node cap exceeded'))]
        for name,original,extra,error in cases:
            badsource = root/f'{name}.bgsource'; write_source(original,badsource)
            target = root/f'refused-{name}'
            failed = invoke(command(options,badsource,target,(.01,))+extra,root,name)
            assert failed.returncode == 1 and tuple(failed.stderr.strip().split('\t')[1:]) == error,failed.stderr
            assert not target.exists() and not list(root.glob(f'{target.name}.tmp-*'))
    print('PASS\tmodel original definitions/chains/facets/scene/placements/materials/PNG\t3 queries')


def scratch_policy(options,root):
    assert options.threads == 1 and not options.module, 'scratch policy ports run once on CPU1'
    src = model_source(); source = root/'original.bgsource'; write_source(src,source)
    outputs = []
    for name,extra,effective,policy in (
            ('implicit',[],64,'default_min_memory_64'),
            ('explicit',['--scratch-mib','256'],256,'explicit')):
        target = root/name
        completed = invoke(command(options,source,target,(.01,))+['--mesh-only']+extra,root,name)
        assert completed.returncode == 0,completed.stderr
        outputs.append(target)
        check_output(src,target,(.01,),mesh_only=True)
        metadata = {row['key']:row['value'] for row in read_tsv(target/'model.tsv')}
        assert (metadata['scratch_mib'],metadata['scratch_bytes'],metadata['scratch_policy']) == (
            str(effective),str(effective << 20),policy)
        assert metadata['memory_mib'] == '1024'
    left,right = outputs
    for a,b in zip(read_tsv(left/'faces.tsv'),read_tsv(right/'faces.tsv')):
        for field in ('mesh','boundary'):
            assert (left/a[field]).read_bytes() == (right/b[field]).read_bytes(),field
    for name,extra,diagnostic in (
            ('zero',['--scratch-mib','0'],'scratch allowance must be positive'),
            ('above-global',['--scratch-mib','1025'],'scratch allowance must be positive'),
            ('overflow',['--scratch-mib',str(1 << 44)],'scratch allowance must be positive'),
            ('reservation-width',['--threads','2','--memory-mib','512','--scratch-mib','256'],
             'mandatory worker scratch reservations exceed memory allowance')):
        target = root/name
        failed = invoke(command(options,source,target,(.01,))+['--mesh-only']+extra,root,name)
        assert failed.returncode == 1 and 'error\toptions\t0\t' in failed.stderr,failed.stderr
        assert diagnostic in failed.stderr,failed.stderr
        assert not target.exists() and not list(root.glob(f'{target.name}.tmp-*'))
    # Valid options can still exhaust the SAME backing resource after source setup.
    name = 'actual-resource'; target = root/name
    failed = invoke(command(options,source,target,(.01,))+[
        '--mesh-only','--memory-mib','1','--scratch-mib','1'],root,name)
    assert failed.returncode == 1 and 'run memory reservation exhausted' in failed.stderr,failed.stderr
    assert 'error\toptions\t' not in failed.stderr,failed.stderr
    assert not target.exists() and not list(root.glob(f'{target.name}.tmp-*'))
    print('PASS\tmodel scratch policy\timplicit/explicit same original outputs; parser/backing refusals')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable',type=Path)
    parser.add_argument('--workspace',type=Path)
    parser.add_argument('--module',type=Path)
    parser.add_argument('--display-module',type=Path)
    parser.add_argument('--threads',type=int,default=1)
    parser.add_argument('--device',type=int,choices=(0,1),default=0)
    parser.add_argument('--ports',action='store_true')
    parser.add_argument('--scratch-policy-only',action='store_true')
    parser.add_argument('--resume',action='store_true',help='read completed native model output; run remaining ports only')
    parser.add_argument('--saved-output',type=Path)
    parser.add_argument('--compare-output',type=Path,help='compare saved output to already original-checked reference')
    options = parser.parse_args()
    if options.saved_output:
        src = load_source(options.saved_output/'source.bgsource')
        check_output(src,options.saved_output,QUALITIES)
        if options.compare_output:
            compare_outputs(options.compare_output,options.saved_output)
        print('PASS\tretained model original-source reader')
    else:
        assert options.executable
        if options.workspace:
            options.workspace.mkdir(parents=True,exist_ok=options.resume)
            (scratch_policy if options.scratch_policy_only else run)(options,options.workspace.resolve())
        else:
            with tempfile.TemporaryDirectory(prefix='native-adaptive-model-') as temporary:
                (scratch_policy if options.scratch_policy_only else run)(options,Path(temporary))


if __name__ == '__main__':
    main()
