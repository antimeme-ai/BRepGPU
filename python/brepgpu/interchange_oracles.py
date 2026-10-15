"""Independent original-source and actual-native carrier qualification.

No parser, converter normalization, or native basis helper supplies expected
values. Source truth uses Cox--de Boor; actual controls use expanded Bernstein.
"""
from fractions import Fraction as F
from math import comb, frexp, isfinite, isnan
import ctypes as C
import hashlib
import struct
from types import SimpleNamespace


def _require(ok, message):
    if not ok:raise AssertionError(message)


def _f(x):
    if type(x) is F:return x
    if type(x) is int:return F(x)
    if type(x) is float and isfinite(x):return F.from_float(x)
    raise ValueError('oracle requires a finite int/Fraction/binary64 value')


def _bits(x):return struct.pack('<d',x)


def _equal(a,b):
    if type(a) is not type(b):return False
    if type(a) is float:return _bits(a)==_bits(b)
    if type(a) is tuple:return len(a)==len(b) and all(_equal(x,y) for x,y in zip(a,b))
    return a==b


def _power2(x):
    return x>0 and x.numerator&(x.numerator-1)==0 and x.denominator&(x.denominator-1)==0


def _basis(knots, count, degree, x):
    """Original knot recurrence; endpoint limits come from the active domain."""
    knots=tuple(map(_f,knots));x=_f(x)
    lo,hi=knots[degree],knots[count]
    if not lo<=x<=hi:raise ValueError('source parameter outside active domain')
    values=[F(int(a<x<=b if x==hi else a<=x<b)) for a,b in zip(knots,knots[1:])]
    previous=None
    for d in range(1,degree+1):
        previous=values
        values=[(F(0) if knots[i+d]==knots[i] else (x-knots[i])*previous[i]/(knots[i+d]-knots[i]))+
                (F(0) if knots[i+d+1]==knots[i+1] else (knots[i+d+1]-x)*previous[i+1]/(knots[i+d+1]-knots[i+1]))
                for i in range(len(values)-1)]
    if degree==0:return tuple(values[:count]),(F(0),)*count
    derivative=tuple((F(0) if knots[i+degree]==knots[i] else degree*previous[i]/(knots[i+degree]-knots[i]))-
                     (F(0) if knots[i+degree+1]==knots[i+1] else degree*previous[i+1]/(knots[i+degree+1]-knots[i+1]))
                     for i in range(count))
    return tuple(values[:count]),derivative


def _quotient(control,bu,bv,du,dv):
    sums=[]
    for us,vs in ((bu,bv),(du,bv),(bu,dv)):
        sums.append(tuple(sum((us[i]*vs[j]*control[i*len(bv)+j][axis]
                               for i in range(len(bu)) for j in range(len(bv))),F(0)) for axis in range(4)))
    a,au,av=sums;w=a[3]
    _require(w>0,'nonpositive oracle rational denominator')
    return (tuple(a[k]/w for k in range(3)),
            tuple((au[k]*w-a[k]*au[3])/(w*w) for k in range(3)),
            tuple((av[k]*w-a[k]*av[3])/(w*w) for k in range(3)))


def source_jet(nurbs,u,v):
    """Exact original source S, SU, SV at the actual supplied source UV."""
    bu,du=_basis(nurbs.knots_u,nurbs.count_u,nurbs.order_u-1,u)
    bv,dv=_basis(nurbs.knots_v,nurbs.count_v,nurbs.order_v-1,v)
    controls=tuple(tuple(_f(x)*_f(w) for x in p)+(_f(w),) for p,w in zip(nurbs.points,nurbs.weights))
    _require(len(controls)==nurbs.count_u*nurbs.count_v,'source net extent')
    return _quotient(controls,bu,bv,du,dv)


def _bernstein(n,t):
    t=_f(t)
    b=tuple(F(comb(n,i))*t**i*(1-t)**(n-i) for i in range(n+1))
    if n==0:return b,(F(0),)
    lower=tuple(F(comb(n-1,i))*t**i*(1-t)**(n-1-i) for i in range(n))
    return b,tuple(n*((lower[i-1] if i else 0)-(lower[i] if i<n else 0)) for i in range(n+1))


def local_jet(patch,s,t):
    """Exact normalized first jet of actual binary64 homogeneous controls."""
    bu,du=_bernstein(patch.degree_u,s);bv,dv=_bernstein(patch.degree_v,t)
    controls=tuple(tuple(map(_f,patch.controls[i:i+4])) for i in range(0,len(patch.controls),4))
    _require(len(controls)==len(bu)*len(bv),'actual control extent')
    return _quotient(controls,bu,bv,du,dv)


def _source_faces(conversion):
    """Rebuild packing from retained typed source records, not converted faces."""
    faces=[]
    for prim in conversion.document.prims:
        if prim.type_name!='BrepArray':continue
        attrs={a.name:a for a in prim.attributes}
        _require(len(attrs)==len(prim.attributes),'duplicate source attributes')
        def field(name,type_name):
            _require(name in attrs and attrs[name].type_name==type_name,'original field type '+name)
            value=attrs[name].value
            _require(type(value) is tuple,'source arrays must be immutable')
            return value
        types=field('face:surfaceType','token');trims=field('face:trimType','token')
        ranges=field('face:range','double2');loops=field('face:loopCount','uint')
        uses=field('faceuse:faceIndex','uint');signs=field('faceuse:orientationType','token')
        _require(len(trims)==len(loops)==len(types) and len(ranges)==2*len(types),'complete source face extents')
        _require(len(uses)==len(signs),'source face-use extents')
        _require(all(type(i) is int and 0<=i<len(types) for i in uses),'source use index')
        _require(all(type(s) is str and s in ('same','opposite') for s in signs),'source use orientation')
        nurbs_count=sum(t=='BrepSurfaceNurbAPI' for t in types)
        arrays={}
        if nurbs_count:
            for name,kind in (('uOrder','uint'),('vOrder','uint'),('uVertexCount','uint'),('vVertexCount','uint'),
                              ('uKnots','double'),('vKnots','double'),('controlVertices','point3d'),('weights','double')):
                arrays[name]=field('brep:surface:nurb:'+name,kind)
            _require(all(len(arrays[name])==nurbs_count for name in ('uOrder','vOrder','uVertexCount','vVertexCount')),'source NURBS record extents')
        ni=ku=kv=cp=0
        for index,kind in enumerate(types):
            source=None
            if kind=='BrepSurfaceNurbAPI':
                ou,ov,nu,nv=(arrays[name][ni] for name in ('uOrder','vOrder','uVertexCount','vVertexCount'))
                _require(all(type(i) is int and i>0 for i in (ou,ov,nu,nv)) and ou<=nu and ov<=nv,'source orders/counts')
                source=SimpleNamespace(order_u=ou,order_v=ov,count_u=nu,count_v=nv,
                    knots_u=arrays['uKnots'][ku:ku+nu+ou],knots_v=arrays['vKnots'][kv:kv+nv+ov],
                    points=arrays['controlVertices'][cp:cp+nu*nv],weights=arrays['weights'][cp:cp+nu*nv])
                _require(len(source.knots_u)==nu+ou and len(source.knots_v)==nv+ov and len(source.points)==len(source.weights)==nu*nv,'source slice extents')
                _require(all(type(x) is float and isfinite(x) for x in (*source.knots_u,*source.knots_v,*source.weights)),'finite source scalars')
                _require(all(type(p) is tuple and len(p)==3 and all(type(x) is float and isfinite(x) for x in p) for p in source.points),'finite source points')
                _require(all(w>0 for w in source.weights),'positive source weights')
                _require(all(a<=b for knots in (source.knots_u,source.knots_v) for a,b in zip(knots,knots[1:])),'source knot monotonicity')
                source.domain=(source.knots_u[ou-1],source.knots_u[nu],source.knots_v[ov-1],source.knots_v[nv])
                _require(source.domain[0]<source.domain[1] and source.domain[2]<source.domain[3],'source active domain')
                ni+=1;ku+=nu+ou;kv+=nv+ov;cp+=nu*nv
            a,b=ranges[2*index:2*index+2]
            _require(len(a)==len(b)==2 and type(loops[index]) is int,'source face metadata')
            faces.append((prim.path,index,kind,trims[index],(a[0],b[0],a[1],b[1]),loops[index],
                          tuple((i,s) for i,(f,s) in enumerate(zip(uses,signs)) if f==index),source))
        if nurbs_count:
            _require(ku==len(arrays['uKnots']) and kv==len(arrays['vKnots']) and cp==len(arrays['controlVertices'])==len(arrays['weights']),'complete source packed coverage')
    return tuple(faces)


def _supported(n):
    if n is None:return False
    if n.order_u>4 or n.order_v>4:return False
    if n.count_u!=n.order_u or n.count_v!=n.order_v:return False
    u0,u1,v0,v1=n.domain
    if n.knots_u!=(u0,)*n.order_u+(u1,)*n.order_u or n.knots_v!=(v0,)*n.order_v+(v1,)*n.order_v:return False
    if any(not F(1,1<<20)<=_f(b)-_f(a)<=1<<20 for a,b in ((u0,u1),(v0,v1))):return False
    if any(not 2**-20<=b-a<=2**20 for a,b in ((u0,u1),(v0,v1))):return False
    if max((abs(_f(x)) for p in n.points for x in p),default=F(0))>1<<20:return False
    # The smallest exact power-of-two raising min(weight) to at least 1 is
    # sufficient: every larger factor only raises max(weight) further.
    exponent=1-frexp(min(n.weights))[1]
    factor=F(2)**exponent
    return all(1<=_f(w)*factor<=256 for w in n.weights)


def _qualify_source(conversion):
    _require(type(conversion.source_bytes) is bytes,'owned immutable original bytes')
    _require(conversion.sha256==hashlib.sha256(conversion.source_bytes).hexdigest(),'original source hash')
    _require(type(conversion.body_id) is int and 0<conversion.body_id<1<<64,'owned body namespace')
    _require(conversion.trim_scope=='uninterpreted','trim qualification must remain explicit')
    original=_source_faces(conversion)
    _require(type(conversion.faces) is tuple and type(conversion.patches) is tuple,'immutable source outputs')
    _require(len(conversion.faces)==len(original),'complete source face inventory')
    slot=0;qualified={}
    for ordinal,(face,raw) in enumerate(zip(conversion.faces,original)):
        path,index,kind,trim,face_range,loop_count,uses,n=raw
        label=f'{path} face {index}'
        _require(type(face.ordinal) is int and face.ordinal==ordinal,label+' ordinal')
        _require(_equal(face.key,(conversion.sha256,path,index)),label+' source key')
        _require(face.surface_type==kind and face.trim_type==trim and _equal(face.face_range,face_range),label+' original face metadata')
        _require(type(face.loop_count) is int and face.loop_count==loop_count and _equal(face.faceuses,uses),label+' original loops/use identity')
        if n is None:_require(face.nurbs is None,label+' non-NURBS source')
        else:
            _require(face.nurbs is not None,label+' original NURBS retained')
            for name in ('order_u','order_v','count_u','count_v','knots_u','knots_v','points','weights','domain'):
                _require(_equal(getattr(face.nurbs,name),getattr(n,name)),label+' original '+name)
        for name in ('position_bound','du_bound','dv_bound'):_require(type(getattr(face,name)) is F,label+' exact bound type')
        if not _supported(n):
            _require(face.status=='unsupported' and type(face.reason) is str and bool(face.reason) and face.patch_slot is None,label+' explicit unsupported outcome')
            _require(face.position_bound==face.du_bound==face.dv_bound==0,label+' unsupported has no replacement bounds')
            continue
        _require(face.status=='converted' and face.reason=='' and type(face.patch_slot) is int and face.patch_slot==slot,label+' accepted slot/order')
        _require(slot<len(conversion.patches),label+' actual patch exists')
        patch=conversion.patches[slot]
        _require(type(patch.degree_u) is int and type(patch.degree_v) is int and (patch.degree_u,patch.degree_v)==(n.order_u-1,n.order_v-1),label+' degree')
        _require(_equal(patch.uv,n.domain) and _equal(patch.origin,(0.,0.,0.)),label+' domain/frame origin')
        scale=_f(patch.scale)
        _require(_power2(scale) and F(1,1<<20)<=scale<=1<<20 and all(abs(_f(x))<=scale for p in n.points for x in p),label+' spatial envelope')
        lin=patch.lineage
        _require(lin is not None and (lin.body_id,lin.entity_kind,lin.entity_id,lin.revision,lin.chart_id)==(conversion.body_id,5,ordinal+1,1,ordinal+1),label+' typed lineage')
        _require(type(patch.controls) is tuple and len(patch.controls)==4*len(n.weights) and all(type(x) is float and isfinite(x) for x in patch.controls),label+' emitted homogeneous extent/type')
        controls=tuple(patch.controls[i:i+4] for i in range(0,len(patch.controls),4))
        multiplier=_f(controls[0][3])/_f(n.weights[0])
        _require(_power2(multiplier),label+' exact common power-of-two weights')
        delta=F(0)
        for p,w,h in zip(n.points,n.weights,controls):
            ew=_f(w)*multiplier
            _require(1<=ew<=256 and _f(h[3])==ew,label+' retained normalized weight')
            for x,y in zip(p,h[:3]):
                _require(_bits(y)==_bits(float(_f(x)*ew/scale)),label+' one-round homogeneous coefficient')
                _require(abs(_f(y))<=ew,label+' native coordinate envelope')
                delta=max(delta,abs(_f(y)/ew*scale-_f(x)))
        ratio=max(map(_f,n.weights))/min(map(_f,n.weights))
        lu,lv=(_f(n.domain[1])-_f(n.domain[0]),_f(n.domain[3])-_f(n.domain[2]))
        bounds=(delta,4*patch.degree_u*ratio*delta/lu,4*patch.degree_v*ratio*delta/lv)
        _require((face.position_bound,face.du_bound,face.dv_bound)==bounds,label+' complete coefficient conversion bounds')
        qualified[slot]=(face,n,patch,bounds)
        slot+=1
    _require(slot==len(conversion.patches),'no extra owned patches')
    return qualified


def _descriptors(conversion,batch):
    _require(len(batch._descriptors)==len(conversion.patches),'full native descriptor inventory')
    offset=0
    for patch,d in zip(conversion.patches,batch._descriptors):
        expected={'abi_version':1,'struct_size':C.sizeof(d),'degree_u':patch.degree_u,'degree_v':patch.degree_v,
                  'control_offset':offset,'control_count':len(patch.controls)//4,'flags':3}
        expected.update((name,getattr(patch.lineage,name)) for name in ('body_id','entity_kind','entity_id','revision','chart_id'))
        for name,value in expected.items():_require(getattr(d,name)==value,'native descriptor '+name)
        for name,values in (('uv',patch.uv),('origin',patch.origin)):
            _require(all(_bits(a)==_bits(b) for a,b in zip(getattr(d,name),values)),'native descriptor '+name)
        _require(_bits(d.scale)==_bits(patch.scale),'native descriptor scale')
        offset+=len(patch.controls)//4


def qualify(conversion,local_batch,world_batch):
    """Qualify actual complete first jets; caller first binds submitted queries."""
    records=_qualify_source(conversion)
    n=local_batch.count
    _require(type(n) is int and n>=0 and world_batch.count==n,'native query counts')
    _require(local_batch._converted is False and world_batch._converted is True,'local/source frame identity')
    for batch in (local_batch,world_batch):
        _require(batch.order==1 and batch.info.order==1 and batch.info.terms==3,'actual native first-jet terms')
        _require(batch.info.abi_version==1 and batch.info.struct_size==C.sizeof(batch.info),'native result info version')
        _require(batch.info.count==n and batch.info.patch_count==len(conversion.patches),'actual native result info counts')
        _require(batch.info.backend in (0,1) and batch.info.flags in (0,1),'observed native backend/info')
        _require((isfinite(batch.info.kernel_ms) and batch.info.kernel_ms>=0) if batch.info.flags else isnan(batch.info.kernel_ms),'native optional profiling field')
        _require(len(batch.data)==9*n and len(batch.statuses)==n and len(batch._slots)==n and len(batch._uv)==2*n,'complete native array extents')
        _descriptors(conversion,batch)
    _require(local_batch.info.backend==world_batch.info.backend,'local/world actual backend')
    _require(tuple(local_batch._slots)==tuple(world_batch._slots),'local/world query slots')
    _require(all(_bits(a)==_bits(b) for a,b in zip(local_batch._uv,world_batch._uv)),'local/world actual query UV bits')
    stats=dict(faces=len(conversion.faces),converted_faces=len(records),unsupported_faces=len(conversion.faces)-len(records),
               queries=n,successful_queries=0,invalid_queries=0)
    for key in ('local_position_error','local_partial_error','local_du_error','local_dv_error','source_position_error','source_du_error','source_dv_error',
                'source_position_budget','source_du_budget','source_dv_budget'):stats['max_'+key]=F(0)
    local_lineage=local_batch.lineage;world_lineage=world_batch.lineage
    for i,slot in enumerate(local_batch._slots):
        s,t=local_batch._uv[2*i:2*i+2]
        status=1 if slot>=len(conversion.patches) else 2 if not(isfinite(s) and isfinite(t) and 0<=s<=1 and 0<=t<=1) else 0
        _require(local_batch.statuses[i]==world_batch.statuses[i]==status,f'query {i} exact native status')
        if status:
            _require(all(value!=value for batch in (local_batch,world_batch) for term in range(3) for value in batch.vector(term,i)),f'query {i} invalid NaN fields')
            stats['invalid_queries']+=1;continue
        face,source,patch,bounds=records[slot]
        _require(local_lineage[i]==world_lineage[i]==patch.lineage,f'query {i} lineage reader')
        expected=local_jet(patch,s,t)
        u0,u1,v0,v1=map(_f,source.domain);lu,lv=u1-u0,v1-v0
        source_expected=source_jet(source,u0+lu*_f(s),v0+lv*_f(t))
        factors=(patch.scale,patch.scale/(patch.uv[1]-patch.uv[0]),patch.scale/(patch.uv[3]-patch.uv[2]))
        exact_factors=(_f(patch.scale),_f(patch.scale)/lu,_f(patch.scale)/lv)
        for term,(exact_values,source_values) in enumerate(zip(expected,source_expected)):
            local_values=local_batch.vector(term,i);world_values=world_batch.vector(term,i)
            epsilon=F(1,1<<(40 if term==0 else 30));g=_f(factors[term]);f=exact_factors[term]
            error_key=('position','du','dv')[term]
            for a,y,e,truth in zip(local_values,world_values,exact_values,source_values):
                _require(type(a) is float and type(y) is float and isfinite(a) and isfinite(y),f'{face.key} finite consumed values')
                error=abs(_f(a)-e)
                _require(error<=epsilon,f'{face.key} local first-jet grade')
                product=float(_f(a)*g)
                wanted=product+0. # Native adds +0 to EVERY component, including derivatives.
                _require(_bits(y)==_bits(wanted),f'{face.key} exact native frame product/+0')
                rho=abs(_f(product)-_f(a)*g)
                budget=bounds[term]+abs(g)*epsilon+abs(g-f)*abs(e)+rho
                observed=abs(_f(y)-truth)
                _require(observed<=budget,f'{face.key} source-world {error_key} budget')
                local_key='max_local_position_error' if term==0 else 'max_local_partial_error'
                stats[local_key]=max(stats[local_key],error)
                if term:stats['max_local_'+error_key+'_error']=max(stats['max_local_'+error_key+'_error'],error)
                stats['max_source_'+error_key+'_error']=max(stats['max_source_'+error_key+'_error'],observed)
                stats['max_source_'+error_key+'_budget']=max(stats['max_source_'+error_key+'_budget'],budget)
        stats['successful_queries']+=1
    return stats
