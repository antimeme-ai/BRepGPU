"""Original source -> real shared-boundary caller -> independent exact reader.

Expected geometry is evaluated from authored original controls, never native
cache descriptors. Fraction residual coefficients bound complete rational chords.
"""
from dataclasses import replace
from fractions import Fraction as F
from pathlib import Path
import argparse
import csv
import math
import struct
import subprocess
import sys
import tempfile
import time
from decimal import Decimal, localcontext

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'python'))
from brepgpu.imported_source import (ImportedSource, Product, SolidDefinition,
    Vertex, Edge, Coedge, Loop, Face, Carrier, Curve, ZERO_FRAME, write_source, load_source)


class Reader:
    def __init__(self, raw):
        self.raw, self.at = raw, 0
    def unpack(self, fmt):
        size = struct.calcsize(fmt)
        assert self.at + size <= len(self.raw), 'truncated shared-boundary artifact'
        result = struct.unpack_from(fmt, self.raw, self.at)
        self.at += size
        return result
    def exact(self):
        sign, nc, dc = self.unpack('<iII')
        assert sign in (-1, 0, 1) and dc and nc+dc <= (len(self.raw)-self.at)//4
        n, d = self.unpack(f'<{nc}I'), self.unpack(f'<{dc}I')
        assert d[-1] and (not n or n[-1])
        a = sum(w << (32*i) for i, w in enumerate(n))
        b = sum(w << (32*i) for i, w in enumerate(d))
        assert bool(a) == bool(sign) and math.gcd(a, b) == 1
        return F(sign*a, b)
    def exacts(self, n):
        return tuple(self.exact() for _ in range(n))
    def optional(self, read):
        flag, = self.unpack('<I')
        assert flag in (0, 1)
        return read() if flag else None
    def key(self):
        return self.unpack('<QQI')
    def line(self):
        return self.exacts(9), self.unpack('<i')[0]
    def cylinder(self):
        return self.unpack('<QQi'), self.exacts(7)


def read(path):
    r = Reader(path.read_bytes())
    magic, wire, numeric, definition, nn, ne, na, np, nu, ns, ni, digest = r.unpack('<8sII8Q32s')
    assert magic == b'BGACRV01' and wire in (1, 2, 3) and numeric == 1
    assert max(nn, ne, na, np, nu, ns, ni) <= len(r.raw)//4
    nodes, edges, aliases, physical, uses, sites, intervals = [], [], [], [], [], [], []
    for _ in range(nn):
        generator, source, generation, ordinal, revision, *xyz = r.unpack('<I4Q3d')
        assert all(math.isfinite(x) for x in xyz)
        t = r.exact()
        npath, = r.unpack('<I')
        path = r.unpack(f'<{npath}B')
        assert set(path) <= {0, 1}
        nodes.append(dict(key=(generator, source, generation, ordinal), revision=revision,
                          xyz=tuple(map(F, xyz)), t=t, path=path))
    for _ in range(ne):
        ids = r.unpack('<8Q')
        t0, t1, sense, same, degenerate, reserved = r.unpack('<2d4I')
        assert reserved == 0 and {sense, same, degenerate} <= {0, 1}
        edges.append(dict(id=ids[0], curve=ids[1], vertices=ids[2:4], nodes=ids[4:6],
                          intervals=ids[6:8], t=(F(t0), F(t1)), sense=sense,degenerate=degenerate))
    for _ in range(na):
        aliases.append(r.unpack('<2Q'))
    for _ in range(np):
        key, nodes2 = r.key(), r.unpack('<2I')
        ts, parameter = r.exacts(2), r.exact()
        image = r.optional(r.exact)
        correspondence, = r.unpack('<I')
        physical.append(dict(key=key, nodes=nodes2, t=ts, parameter=parameter,
                             image=image, correspondence=correspondence, line=r.optional(r.line)))
    for _ in range(nu):
        ids, meta, ranges = r.unpack('<5Q'), r.unpack('<iIIiiI'), r.unpack('<6Q')
        image = None
        if wire >= 2:
            present, = r.unpack('<I')
            assert present in (0, 1)
            if present:
                raw, traversal = r.unpack('<Qi')
                assert traversal in (-1, 1)
                image = dict(raw=raw, traversal=traversal, endpoints=r.exacts(4),
                             covector=r.exacts(2), scalar=r.exacts(2))
        uses.append(dict(ids=ids, meta=meta, sites=ranges[:2], intervals=ranges[2:4],
                         generations=ranges[4:], whole_image=image))
    for _ in range(ns):
        node, loop, junction, *branches = r.unpack('<IQI4i')
        t, s, u, v, x, y, guard = r.exacts(7)
        sites.append(dict(node=node, loop=loop, junction=junction, branches=tuple(branches),
                          t=t, s=s, uv=(u, v), chart=(x, y), guard=guard))
    for _ in range(ni):
        key = r.key()
        a, b, authority, correspondence = r.unpack('<4I')
        trace, source_trace, du, dv = r.exacts(4)
        intervals.append(dict(key=key, sites=(a, b), authority=authority,
            correspondence=correspondence, trace=trace, source_trace=source_trace, uv=(du, dv),
            domain=r.optional(r.exact), line=r.optional(r.line), cylinder=r.optional(r.cylinder)))
    wedges = []
    if wire >= 3:
        nw, = r.unpack('<Q')
        assert nw <= len(r.raw)//4
        for _ in range(nw):
            vertex, loop, incoming, outgoing = r.unpack('<4Q')
            a, b, kind = r.unpack('<3I')
            assert max(a,b) < ns and kind in (0,1,2)
            uv = r.exacts(4)
            assert sites[a]['uv'] == uv[:2] and sites[b]['uv'] == uv[2:]
            assert sites[a]['node'] == sites[b]['node'] and uv[:2] != uv[2:]
            wedges.append(dict(vertex=vertex, loop=loop, uses=(incoming,outgoing),
                               sites=(a,b), kind=kind, uv=uv))
    assert r.at == len(r.raw), 'trailing shared-boundary artifact bytes'
    return dict(definition=definition, digest=digest, nodes=nodes, edges=edges, aliases=aliases,
                physical=physical, uses=uses, sites=sites, intervals=intervals, wedges=wedges, wire=wire)


def source(kind='quadratic'):
    uv = ((0.,0.), (1.,0.), (1.,1.), (0.,1.))
    xyz = tuple((*p, 0.) for p in uv)
    curves, edges, uses = [], [], []
    for i, a in enumerate(uv):
        b = uv[(i+1)%4]
        if i == 0 and kind == 'speed':
            controls = (0.,0.,0.,1.)*3+(1.,0.,0.,1.)
            degree = 3
        elif i == 0 and kind in ('quadratic', 'weighted', 'degree32', 'shared'):
            degree = 32 if kind == 'degree32' else 2
            # Exact degree elevation of y=t(1-t)/2; integer lattice denominators
            # deliberately rounded in source double, then treated as exact input.
            controls = tuple(q for j in range(degree+1)
                for q in (j/degree, j*(degree-j)/(2*degree*(degree-1)), 0.,
                          8. if kind == 'weighted' and j == 1 else 1.))
        else:
            controls, degree = (*a,0.,1.,*b,0.,1.), 1
        curves.append(Curve(2000+i,3,4,ZERO_FRAME,(0.,0.),(0.,1.),degree,controls=controls))
        curves.append(Curve(3000+i,2,4,ZERO_FRAME,(0.,0.),(0.,1.),1,
                            controls=(*a,1.,*b,1.)))
        edges.append(Edge(400+i,(300+i,300+(i+1)%4),2000+i,(0.,1.),True,True,False,0.))
        uses.append(Coedge(500+i,400+i,800,1,3000+i,(0.,1.),reconstructed=True))
    frame=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
    result=ImportedSource('ad'*32,1.,0.,100,'authored complete shared curve source',
        (Product(100,'root','root',200),),
        (SolidDefinition(200,100,(900,),tuple(e.id for e in edges),(300,301,302,303)),),(),
        tuple(Vertex(300+i,p,0.) for i,p in enumerate(xyz)),tuple(edges),tuple(uses),
        (Loop(800,900,True,(500,501,502,503)),),(Face(900,200,9,1,(800,),0.),),
        (Carrier(9,1,frame,(0.,0.),(0.,1.,0.,1.)),),tuple(curves))


    if kind=='shared':
        extra=tuple(Coedge(600+i,400+i,801,-1,3000+i,(0.,1.)) for i in range(4))
        result=replace(result,
            definitions=(replace(result.definitions[0],face_ids=(900,901)),),
            coedges=(*result.coedges,*extra),
            loops=(*result.loops,Loop(801,901,True,(603,602,601,600))),
            faces=(*result.faces,Face(901,200,9,-1,(801,),0.)))
    if kind=='descending':
        edge=replace(result.edges[0],vertex_ids=result.edges[0].vertex_ids[::-1],
                     parameter_range=(1.,0.),curve_same_sense=False,same_parameter=False)
        use=replace(result.coedges[0],sense=-1,parameter_map_kind=2,
                    parameter_map=((1.,1.),(.5,.25),(0.,0.)))
        result=replace(result,edges=(edge,*result.edges[1:]),coedges=(use,*result.coedges[1:]))
    if kind in ('c0','nonclamped'):
        curve=(Curve(2000,3,5,ZERO_FRAME,(0.,0.),(0.,1.),1,(0.,.5,1.),(2,1,2),
                     (0.,0.,0.,1., .5,.125,0.,1., 1.,0.,0.,1.)) if kind=='c0' else
               Curve(2000,3,5,ZERO_FRAME,(0.,0.),(0.,1.),2,(-1.,0.,.5,1.,2.),(1,2,1,2,1),
                     (0.,0.,0.,1.)*2+(1.,0.,0.,1.)*2))
        result=replace(result,curves=(curve,*result.curves[1:]))
    if kind=='wire':
        curve=Curve(2004,3,4,ZERO_FRAME,(0.,0.),(0.,1.),2,
                    controls=(.25,.25,0.,1., .5,.625,0.,1., .75,.5,0.,1.))
        edge=Edge(404,(304,305),2004,(0.,1.),True,True,False,0.)
        definition=replace(result.definitions[0],edge_ids=(*result.definitions[0].edge_ids,404),
                           vertex_ids=(*result.definitions[0].vertex_ids,304,305))
        result=replace(result,definitions=(definition,),curves=(*result.curves,curve),
                       edges=(*result.edges,edge),vertices=(*result.vertices,
                       Vertex(304,(.25,.25,0.),0.),Vertex(305,(.75,.5,0.),0.)))
    return result


def curve_h(curve,t):
    if curve.kind==4: return de_casteljau(homogeneous(curve),t)
    from functools import lru_cache
    knots=tuple(F(k) for k,m in zip(curve.knots,curve.multiplicities) for _ in range(m))
    p=curve.degree
    @lru_cache(None)
    def basis(i,d):
        if d==0:
            return F(knots[i]<t<=knots[i+1]) if t==knots[-p-1] else F(knots[i]<=t<knots[i+1])
        value=F(0)
        if knots[i+d]!=knots[i]: value+=(t-knots[i])/(knots[i+d]-knots[i])*basis(i,d-1)
        if knots[i+d+1]!=knots[i+1]: value+=(knots[i+d+1]-t)/(knots[i+d+1]-knots[i+1])*basis(i+1,d-1)
        return value
    net=homogeneous(curve)
    return tuple(sum(basis(i,p)*row[j] for i,row in enumerate(net)) for j in range(4))


def curve_local(curve,a,b):
    if curve.kind==4: return restrict(homogeneous(curve),a,b)
    assert curve.degree in (1,2)
    assert not any(min(a,b)<F(k)<max(a,b) for k in curve.knots), 'physical chord crosses original knot'
    first,last=curve_h(curve,a),curve_h(curve,b)
    if curve.degree==1: return [first,last]
    middle=curve_h(curve,(a+b)/2)
    return [first,tuple(2*m-(x+y)/2 for x,m,y in zip(first,middle,last)),last]


def homogeneous(curve):
    stride = curve.dimension+1
    points = [tuple(map(F, curve.controls[i:i+stride]))
              for i in range(0, len(curve.controls), stride)]
    return points  # BGSRC stores literal HOMOGENEOUS coordinates, not Cartesian+weight.


def de_casteljau(net, t):
    while len(net)>1:
        net = [tuple((1-t)*a+t*b for a,b in zip(p,q)) for p,q in zip(net,net[1:])]
    return net[0]


def restrict(net, a, b):
    # Blossom evaluated with repeated endpoint arguments: independent exact
    # restriction, no native control nets or numerical bounds used as inputs.
    n = len(net)-1
    result=[]
    for k in range(n+1):
        rows=net
        for t in [a]*(n-k)+[b]*k:
            rows=[tuple((1-t)*x+t*y for x,y in zip(p,q)) for p,q in zip(rows,rows[1:])]
        result.append(rows[0])
    return result


def whole_chord(net, a, b, qa, qb, bound, allowance):
    # R=N-W*L has degree n+1, W>0; exact convex-hull norm bound on R/W.
    local=restrict(net,a,b)
    n=len(local)-1
    wmin=min(p[3] for p in local)
    assert wmin>0 and bound>=0
    residual=[]
    for k in range(n+2):
        coeff=[]
        for axis in range(3):
            value=F(0)
            if k<=n:
                value += F(n+1-k,n+1)*(local[k][axis]-local[k][3]*qa[axis])
            if k:
                value += F(k,n+1)*(local[k-1][axis]-local[k-1][3]*qb[axis])
            coeff.append(value)
        weight=F(0)
        if k<=n: weight+=F(n+1-k,n+1)*local[k][3]
        if k: weight+=F(k,n+1)*local[k-1][3]
        residual.append(tuple(coeff)+(weight,))
    def fits(rows, depth):
        lower=min(row[3] for row in rows)
        if all(sum(x*x for x in row[:3])<=allowance**2*lower**2 for row in rows): return
        assert depth, 'independent whole original chord bound exceeds allowance'
        pyramid=[rows]
        while len(pyramid[-1])>1:
            pyramid.append([tuple((a+b)/2 for a,b in zip(p,q))
                            for p,q in zip(pyramid[-1],pyramid[-1][1:])])
        fits([row[0] for row in pyramid],depth-1)
        fits([row[-1] for row in reversed(pyramid)],depth-1)
    fits(residual,12)
    # A finite exact falsification of the separately emitted upper bound; the
    # numerical unit's direct coefficient oracle supplies its general contract.
    for t in (F(i,16) for i in range(17)):
        r=de_casteljau(residual,t)
        assert sum(x*x for x in r[:3])<=bound**2*r[3]**2, 'emitted chord upper is false'




def check_line(record,curve,ts,trace_points,traversal,source_value=None):
    assert record is not None
    values,direction=record
    covector,cs,ss,tt=values[:3],values[3:5],values[5:7],values[7:9]
    assert any(covector) and direction==traversal and set(tt)==set(ts)
    for t,scalar,selected in zip(tt,cs,ss):
        if source_value is None:
            h=curve_h(curve,t)
            point=tuple(h[j]/h[3] for j in range(3))
        else:
            point=source_value(t)
        assert scalar==sum(covector[j]*point[j] for j in range(3))
        p=trace_points[t]
        assert selected==sum(covector[j]*p[j] for j in range(3))


def check(result, original, positional, node_cap, kind, carrier_value=None, definition=None):
    if carrier_value is None:
        carrier_value=lambda use,uv: uv+(F(0),)
    expected_definition=200 if definition is None else definition.id
    edge_ids={e.id for e in original.edges} if definition is None else set(definition.edge_ids)
    face_ids=None if definition is None else set(definition.face_ids)
    loop_ids={l.id for l in original.loops if face_ids is None or l.face_id in face_ids}
    assert result['definition']==expected_definition and result['digest']==bytes.fromhex(original.source_sha256)
    assert [e['id'] for e in result['edges']]==sorted(edge_ids)
    assert {u['ids'][0] for u in result['uses']}=={u.id for u in original.coedges if u.loop_id in loop_ids}
    curves={c.id:c for c in original.curves}
    vertices={v.id:tuple(map(F,v.xyz)) for v in original.vertices}
    physical=result['physical']
    nodes=result['nodes']
    keyxyz={}
    for row in nodes:
        assert keyxyz.setdefault(row['key'],row['xyz'])==row['xyz'], 'shared key changed XYZ'
    for edge in result['edges']:
        first,count=edge['nodes']; pi,pn=edge['intervals']
        chain=nodes[first:first+count]
        assert len(chain)==count and count>=2 and pn==count-1
        assert chain[0]['t']==edge['t'][0] and chain[-1]['t']==edge['t'][1]
        assert chain[0]['xyz']==vertices[edge['vertices'][0]]
        assert chain[-1]['xyz']==vertices[edge['vertices'][1]]
        direction=1 if edge['t'][1]>edge['t'][0] else -1
        assert all(direction*(b['t']-a['t'])>0 for a,b in zip(chain,chain[1:]))
        curve=curves[edge['curve']]
        for row in physical[pi:pi+pn]:
            a,b=[nodes[j] for j in row['nodes']]
            assert row['t']==(a['t'],b['t']) and row['key'][0]==edge['id']
            whole_chord(curve_local(curve,a['t'],b['t']),F(0),F(1),a['xyz'],b['xyz'],row['parameter'],
                        positional if row['correspondence']==0 else row['parameter'])
            if row['line']:
                check_line(row['line'],curve,row['t'],{a['t']:a['xyz'],b['t']:b['xyz']},direction)
            if row['correspondence']==0:
                assert row['parameter']<=positional
            else:
                assert row['image'] is not None and row['image']<=positional and row['line']
                # Authored t^3 has exactly the line segment image although its
                # equal-parameter error is large. This must remain one interval.
                local=curve_local(curve,a['t'],b['t'])
                points=[tuple(p[i]/p[3] for i in range(3)) for p in local]
                line_direction=tuple(points[-1][i]-points[0][i] for i in range(3))
                axis=next((i for i,x in enumerate(line_direction) if x),None)
                assert axis is not None
                for p in points:
                    q=(p[axis]-points[0][axis])/line_direction[axis]
                    assert 0<=q<=1 and all(p[i]==points[0][i]+q*line_direction[i] for i in range(3))
                for p,q in ((points[0],a['xyz']),(points[-1],b['xyz'])):
                    assert sum((x-y)**2 for x,y in zip(p,q))<=row['image']**2
                if kind=='speed' and edge['id']==400: assert count==2
        for row in chain:
            h=curve_h(curve,row['t']); p=tuple(h[i]/h[3] for i in range(3))
            assert sum((p[i]-row['xyz'][i])**2 for i in range(3))<=F(positional)**2
    for use in result['uses']:
        first,count=use['sites']; pi,pn=use['intervals']
        sites=result['sites'][first:first+count]
        assert len(sites)==count and pn==count-1 and count>=2
        original_use=next(u for u in original.coedges if u.id==use['ids'][0])
        edge=next(e for e in result['edges'] if e['id']==original_use.edge_id)
        efirst,ecount=edge['nodes']; pfirst,pcount=edge['intervals']
        order=list(range(efirst,efirst+ecount))
        expected_physical=physical[pfirst:pfirst+pcount]
        if original_use.sense<0:
            order.reverse(); expected_physical=list(reversed(expected_physical))
        assert [site['node'] for site in sites]==order, 'use traversal differs from original incidence'
        assert count==ecount and pn==pcount

        for site in sites:
            assert site['loop']==original_use.loop_id and site['guard']<=node_cap
            node=nodes[site['node']]
            assert node['t']==site['t']
            original_use=next(u for u in original.coedges if u.id==use['ids'][0])
            expected=site['t']
            if original_use.parameter_map_kind==2:
                pairs=original_use.parameter_map
                for (a,b),(c,d) in zip(pairs,pairs[1:]):
                    if min(F(a),F(c))<=site['t']<=max(F(a),F(c)):
                        expected=F(b)+(site['t']-F(a))*(F(d)-F(b))/(F(c)-F(a)); break
                else: raise AssertionError('mapped t outside original map')
            assert site['s']==expected
            # Compare ORIGINAL Ce to ORIGINAL S(chosenUV), never actual Q.
            curve=curves[edge['curve']]
            h=curve_h(curve,site['t'])
            p=tuple(h[i]/h[3] for i in range(3))
            q=carrier_value(original_use,site['uv'])
            assert sum((p[i]-q[i])**2 for i in range(3))<=site['guard']**2
        for j,interval in enumerate(result['intervals'][pi:pi+pn]):
            assert interval['sites']==(first+j,first+j+1)
            assert interval['key']==expected_physical[j]['key']
            a,b=sites[j:j+2]
            for site in (a,b):
                p=carrier_value(original_use,site['uv']); q=nodes[site['node']]['xyz']
                assert sum((x-y)**2 for x,y in zip(p,q))<=interval['trace']**2
            if interval['line']:
                check_line(interval['line'],curves[edge['curve']],(a['t'],b['t']),
                           {a['t']:carrier_value(original_use,a['uv']),
                            b['t']:carrier_value(original_use,b['uv'])},
                           1 if b['t']>a['t'] else -1)
            assert interval['trace']>=0 and interval['source_trace']>=0
            if interval['domain'] is not None:
                assert interval['domain']+interval['trace']<=positional
            else:
                assert interval['trace']<=positional
    return result



def dec(value):
    value=F(value)
    return Decimal(value.numerator)/Decimal(value.denominator)


def original_analytic(curve,t):
    t=dec(t); o,x,y=curve.frame[:3],curve.frame[3:6],curve.frame[6:9]
    if curve.kind==1: return tuple(dec(a)+t*dec(b) for a,b in zip(o,x))
    sine,cosine=t,Decimal(1)
    a,b=t,Decimal(1)
    for k in range(1,100):
        a*=-t*t/Decimal(2*k*(2*k+1)); b*=-t*t/Decimal((2*k-1)*2*k)
        sine+=a; cosine+=b
    return tuple(dec(a)+dec(curve.radii[0])*dec(b)*cosine+dec(curve.radii[1])*dec(c)*sine
                 for a,b,c in zip(o,x,y))


def check_seam(result,original,positional,node_cap):
    assert result['definition']==200 and result['digest']==bytes.fromhex(original.source_sha256)
    assert [row['id'] for row in result['edges']]==[e.id for e in original.edges]
    assert {u['ids'][0] for u in result['uses']}=={u.id for u in original.coedges}
    nodes=result['nodes']; physical=result['physical']; curves={c.id:c for c in original.curves}
    vertices={v.id:tuple(map(F,v.xyz)) for v in original.vertices}
    keyxyz={}
    with localcontext() as ctx:
        ctx.prec=90
        for row in nodes:
            assert keyxyz.setdefault(row['key'],row['xyz'])==row['xyz']
        for edge in result['edges']:
            first,count=edge['nodes']; pi,pn=edge['intervals']; chain=nodes[first:first+count]
            assert count>=2 and pn==count-1 and len(chain)==count
            assert (chain[0]['t'],chain[-1]['t'])==edge['t']
            assert chain[0]['xyz']==vertices[edge['vertices'][0]]
            assert chain[-1]['xyz']==vertices[edge['vertices'][1]]
            assert all(a['t']<b['t'] for a,b in zip(chain,chain[1:]))
            curve=curves[edge['curve']]
            for interval in physical[pi:pi+pn]:
                a,b=[nodes[i] for i in interval['nodes']]
                departures=[]
                for row in (a,b):
                    truth=original_analytic(curve,row['t'])
                    departures.append(sum((x-dec(y))**2 for x,y in zip(truth,row['xyz'])).sqrt())
                # Original frame is the unit orthogonal XY frame: M2=1 for
                # circles, zero for the literal vertical line.
                m2=Decimal(0) if curve.kind==1 else Decimal(1)
                upper=m2*dec(b['t']-a['t'])**2/8+max(departures)
                assert upper<=dec(positional) and upper<=dec(interval['parameter'])
        for use in result['uses']:
            original_use=next(u for u in original.coedges if u.id==use['ids'][0])
            edge=next(e for e in result['edges'] if e['id']==original_use.edge_id)
            first,count=use['sites']; pi,pn=use['intervals']; ef,en=edge['nodes']; ei,ec=edge['intervals']
            sites=result['sites'][first:first+count]
            order=list(range(ef,ef+en)); paired=physical[ei:ei+ec]
            if original_use.sense<0: order.reverse(); paired=list(reversed(paired))
            assert [site['node'] for site in sites]==order and count==en and pn==ec
            for site in sites:
                assert site['t']==site['s']==nodes[site['node']]['t'] and site['guard']<=node_cap
                assert site['branches'][:2]==(0,0) and site['loop']==800
                # Cylinder S(u,v)=(cos(u),sin(u),v) uses the original guide
                # angle, not an inferred wrapping/VertexID equivalence.
                carrier=replace(curves[2000],frame=(0.,0.,float(site['uv'][1]),1.,0.,0.,0.,1.,0.,0.,0.,1.))
                selected=original_analytic(carrier,site['uv'][0])
                truth=original_analytic(curves[edge['curve']],site['t'])
                assert sum((x-y)**2 for x,y in zip(selected,truth)).sqrt()<=dec(site['guard'])
            for j,interval in enumerate(result['intervals'][pi:pi+pn]):
                assert interval['sites']==(first+j,first+j+1) and interval['key']==paired[j]['key']
                assert interval['trace']<=positional and interval['source_trace']>=0
                a,b=sites[j:j+2]
                if interval['cylinder']:
                    ids,values=interval['cylinder']
                    assert ids[:2]==(edge['curve'],9) and ids[2] in (-1,1)
                    assert values[:2]==(a['t'],b['t']) and values[2:6]==a['uv']+b['uv']
                    assert values[6]==a['uv'][1]==b['uv'][1]
                    assert a['uv'][0]==a['t'] and b['uv'][0]==b['t']
        right=next(u for u in result['uses'] if u['ids'][0]==501)
        left=next(u for u in result['uses'] if u['ids'][0]==503)
        rs=result['sites'][right['sites'][0]:sum(right['sites'])]
        ls=result['sites'][left['sites'][0]:sum(left['sites'])]
        assert {s['node'] for s in rs}=={s['node'] for s in ls}
        assert {s['uv'][0] for s in rs}=={F(6.283185307179586)}
        assert {s['uv'][0] for s in ls}=={F(0)}
    return result


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('caller',type=Path)
    parser.add_argument('output',type=Path)
    parser.add_argument('--cuda-module',type=Path)
    parser.add_argument('--trace-fixtures',type=Path)
    parser.add_argument('--family',action='append')
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='shared-curves-',dir=args.output) as directory:
        root=Path(directory)
        observations=[]; processes=[]; stages=[]
        def observe(dest,kind,name,history,elapsed):
            with (dest/'queries.tsv').open() as f:
                rows=list(csv.DictReader(f,delimiter='\t'))
            observations.extend(dict(family=kind,placement=name,history=history,**row) for row in rows)
            processes.append(dict(family=kind,placement=name,history=history,
                                  queries=len(rows),process_wall_seconds=elapsed))
            with (dest/'timings.tsv').open() as f:
                stages.extend(dict(family=kind,placement=name,history=history,**row)
                              for row in csv.DictReader(f,delimiter='\t'))
            return rows
        def save_observations():
            for name,rows in (('queries',observations),('processes',processes),('stages',stages)):
                if rows:
                    with (args.output/f'{name}.tsv').open('w') as f:
                        writer=csv.DictWriter(f,fieldnames=list(rows[0]),delimiter='\t')
                        writer.writeheader(); writer.writerows(rows)
        families=args.family or ['quadratic','weighted','degree32','speed','descending','wire','c0','nonclamped','shared']
        if args.trace_fixtures and not args.family: families.append('seam')
        for kind in families:
            original=load_source(args.trace_fixtures/'seam.bgsource') if kind=='seam' else source(kind)
            path=root/f'{kind}.bgsource'
            write_source(original,path)
            backends=[('cpu1',['--threads','1']),('cpu4',['--threads','4'])]
            if args.cuda_module:
                backends += [(f'p100{d}',['--threads','4','--backend','p100','--cuda-module',
                              str(args.cuda_module),'--device',str(d)]) for d in (0,1)]
            baseline=None
            for name,backend in backends:
                dest=root/f'{kind}-{name}'
                eps=(.02,.08,.02,.02)
                command=[str(args.caller),'--source',str(path),'--definition','200','--output',str(dest),
                    '--node-source-mm','1e-8',*backend]
                for e in eps:
                    command+=['--positional-mm',str(e)]
                tick=time.monotonic()
                completed=subprocess.run(command,capture_output=True,text=True,timeout=90)
                elapsed=time.monotonic()-tick
                assert completed.returncode==0, (kind,name,completed.stderr)
                values=[(check_seam(read(dest/f'query-{i}.bgacrv'),original,F(e),F(1e-8)) if kind=='seam'
                         else check(read(dest/f'query-{i}.bgacrv'),original,F(e),F(1e-8),kind))
                        for i,e in enumerate(eps)]
                raw=(dest/'query-0.bgacrv').read_bytes()
                assert raw==(dest/'query-2.bgacrv').read_bytes()==(dest/'query-3.bgacrv').read_bytes()
                if baseline is None: baseline=raw
                else: assert raw==baseline, 'placement changed source construction'
                stats=observe(dest,kind,name,'fine-coarse-fine-repeat',elapsed)
                assert int(stats[-1]['point_hits'])>0
                if name.startswith('p100'):
                    assert int(stats[-1]['device_points'])>0 and int(stats[-1]['device_bounds'])>0
                # A retained fine numerical cache must not force fine leaves on
                # the fresh coarse topology query.
                cold=root/f'{kind}-{name}-cold'
                command=[str(args.caller),'--source',str(path),'--definition','200','--output',str(cold),
                    '--node-source-mm','1e-8','--positional-mm','.08',*backend]
                tick=time.monotonic()
                subprocess.run(command,check=True,capture_output=True,text=True,timeout=90)
                observe(cold,kind,name,'fresh-coarse',time.monotonic()-tick)
                save_observations()
                assert (cold/'query-0.bgacrv').read_bytes()==(dest/'query-1.bgacrv').read_bytes()
            print(f'complete-original-shared-curves\t{kind}\tPASS',flush=True)
        original=source('quadratic')
        path=root/'refusal.bgsource'
        write_source(original,path)
        def invoke(dest,extra=()):
            return subprocess.run([str(args.caller),'--source',str(path),'--definition','200',
                '--output',str(dest),'--positional-mm','.002','--node-source-mm','1e-8',
                '--threads','4',*extra],capture_output=True,text=True,timeout=90)
        target=root/'existing'
        target.mkdir(); (target/'sentinel').write_bytes(b'keep original')
        assert invoke(target).returncode and (target/'sentinel').read_bytes()==b'keep original'
        bad_vertex=replace(original.vertices[0],xyz=(0.,0.,.125))
        bad=replace(original,vertices=(bad_vertex,*original.vertices[1:]))
        write_source(bad,path)
        target=root/'bad-original-vertex'
        failed=invoke(target)
        assert failed.returncode and not target.exists() and 'source-node' in failed.stderr, (failed.returncode,failed.stderr,target.exists())
        bad_curve=replace(original.curves[0],controls=(0.,0.,0.,1., .5,.125,.5,1., 1.,0.,0.,1.))
        write_source(replace(original,curves=(bad_curve,*original.curves[1:])),path)
        target=root/'endpoint-invisible-bulge'
        failed=invoke(target)
        assert failed.returncode and not target.exists() and 'source-node' in failed.stderr, (failed.returncode,failed.stderr,target.exists())
        write_source(original,path)
        target=root/'capacity'
        assert invoke(target,('--max-nodes','1')).returncode and not target.exists()
        bad_use=replace(original.coedges[0],parameter_map_kind=3,parameter_map=((0.,0.),(1.,1.)))
        write_source(replace(original,coedges=(bad_use,*original.coedges[1:])),path)
        target=root/'unsupported-map'
        failed=invoke(target)
        assert failed.returncode and not target.exists() and 'parameter map' in failed.stderr
        write_source(original,path)
        normal=root/'units-normal'
        assert invoke(normal).returncode==0
        write_source(replace(original,units_mm=25.4),path)
        metadata=root/'units-metadata'
        assert invoke(metadata).returncode==0
        assert (normal/'query-0.bgacrv').read_bytes()==(metadata/'query-0.bgacrv').read_bytes()
        constant_use=replace(original.coedges[0],parameter_map_kind=2,
                             parameter_map=((0.,.25),(1.,.25)))
        constant=replace(original,coedges=(constant_use,*original.coedges[1:]),
                         edges=(replace(original.edges[0],same_parameter=False),*original.edges[1:]))
        write_source(constant,path)
        target=root/'constant-original-guide-map'
        succeeded=invoke(target)
        assert succeeded.returncode==0,succeeded.stderr
        check(read(target/'query-0.bgacrv'),constant,F(.002),F(1e-8),'constant-map')
        # A zero-range declared wire quotient has two original endpoint
        # occurrences and no positive physical interval. Its Ce still requires
        # an ORIGINAL positive evaluator span; never invent t +/- epsilon.
        frame=(.5,.5,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.)
        quotient=replace(original,
            definitions=(replace(original.definitions[0],edge_ids=(*original.definitions[0].edge_ids,404),
                vertex_ids=(*original.definitions[0].vertex_ids,304,305)),),
            vertices=(*original.vertices,Vertex(304,(.5,.5,0.),0.),Vertex(305,(.5,.5,0.),0.)),
            edges=(*original.edges,Edge(404,(304,305),2004,(.5,.5),True,True,True,0.)),
            curves=(*original.curves,Curve(2004,3,1,frame,(0.,0.),(0.,1.))))
        write_source(quotient,path)
        target=root/'literal-quotient'
        succeeded=invoke(target)
        assert succeeded.returncode==0,succeeded.stderr
        value=read(target/'query-0.bgacrv')
        edge=next(e for e in value['edges'] if e['id']==404)
        first,count=edge['nodes']
        assert count==2 and edge['degenerate'] and edge['intervals'][1]==0
        a,b=value['nodes'][first:first+count]
        assert a['key']==b['key'] and a['xyz']==b['xyz']==(F(1,2),F(1,2),F(0))
        assert a['t']==b['t']==F(1,2)
        assert set(value['aliases'])=={(304,304),(305,304)}
        assert not list(root.glob('*.tmp-*')), 'failed caller left partial staging output'
        print('complete-original-shared-curves\trefusals-publication-units\tPASS',flush=True)



if __name__=='__main__':
    main()
