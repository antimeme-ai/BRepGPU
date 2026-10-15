"""Owned static USD BRep carrier conversion; trims remain uninterpreted.

Only the declared ASCII subset is accepted. No reference runtime, topology
importer, fitted controls, knot insertion or coordinate welding participates.
"""
from dataclasses import dataclass
from fractions import Fraction as F
from hashlib import sha256
import math

from . import Lineage, Patch
from .body import _namespace
from .usda_subset import (MAX_BYTES, SourceError, UnsupportedSource,
                          SourceBudgetError, parse)


@dataclass(frozen=True)
class SourceNurbs:
    order_u: int
    order_v: int
    count_u: int
    count_v: int
    knots_u: tuple
    knots_v: tuple
    points: tuple
    weights: tuple

    def __post_init__(self):
        for name in ('knots_u', 'knots_v', 'weights'):
            object.__setattr__(self, name, tuple(getattr(self, name)))
        object.__setattr__(self, 'points', tuple(tuple(p) for p in self.points))

    @property
    def domain(self):
        return (self.knots_u[self.order_u-1], self.knots_u[self.count_u],
                self.knots_v[self.order_v-1], self.knots_v[self.count_v])


@dataclass(frozen=True)
class SourceFace:
    key: tuple
    ordinal: int
    surface_type: str
    trim_type: str
    face_range: tuple
    loop_count: int
    faceuses: tuple
    nurbs: SourceNurbs | None
    status: str
    reason: str
    patch_slot: int | None
    position_bound: F
    du_bound: F
    dv_bound: F

    def __post_init__(self):
        object.__setattr__(self, 'key', tuple(self.key))
        object.__setattr__(self, 'face_range', tuple(self.face_range))
        object.__setattr__(self, 'faceuses', tuple(tuple(use) for use in self.faceuses))

    @property
    def trim_scope(self):
        return 'uninterpreted'


@dataclass(frozen=True, init=False)
class Conversion:
    source_bytes: bytes
    sha256: str
    document: object
    body_id: int
    faces: tuple
    patches: tuple

    def __init__(self, *args, **kwargs):
        raise TypeError('use convert(data) or load(path) to construct Conversion')

    @property
    def trim_scope(self):
        return 'uninterpreted'


_NURB = 'BrepSurfaceNurbAPI'
_NPREFIX = 'brep:surface:nurb:'
_FIELDS = (('uOrder','uint'), ('vOrder','uint'),
           ('uVertexCount','uint'), ('vVertexCount','uint'),
           ('uKnots','double'), ('vKnots','double'),
           ('controlVertices','point3d'), ('weights','double'))


def _field(attributes, name, type_name, *, optional=False):
    attr = attributes.get(name)
    if attr is None:
        if optional:
            return ()
        raise SourceError('missing required attribute '+name)
    if attr.type_name != type_name:
        raise SourceError(name+' requires '+type_name+'[]')
    return attr.value


def _extent(values, count, name):
    if len(values) != count:
        raise SourceError(name+' has incorrect packed extent')


def _validate_nurbs(nurbs):
    for order, count, knots in ((nurbs.order_u,nurbs.count_u,nurbs.knots_u),
                                (nurbs.order_v,nurbs.count_v,nurbs.knots_v)):
        if not 1 <= order <= count:
            raise SourceError('NURBS requires positive order <= vertex count')
        if any(b<a for a,b in zip(knots,knots[1:])):
            raise SourceError('NURBS knots must be nondecreasing')
        if not knots[order-1] < knots[count]:
            raise SourceError('NURBS active knot domain must be nonempty')
    if any(w<=0 for w in nurbs.weights):
        raise SourceError('NURBS weights must be strictly positive')


def _packed_prim(prim):
    """Validate complete typed extents before slicing any individual carrier."""
    attrs = {attr.name:attr for attr in prim.attributes}
    surfaces = _field(attrs,'face:surfaceType','token')
    nfaces = len(surfaces)
    trims = _field(attrs,'face:trimType','token')
    ranges = _field(attrs,'face:range','double2')
    loops = _field(attrs,'face:loopCount','uint')
    use_faces = _field(attrs,'faceuse:faceIndex','uint')
    use_orientation = _field(attrs,'faceuse:orientationType','token')
    for values,count,name in ((trims,nfaces,'face:trimType'),
                              (ranges,2*nfaces,'face:range'),
                              (loops,nfaces,'face:loopCount'),
                              (use_orientation,len(use_faces),'faceuse:orientationType')):
        _extent(values,count,name)
    if any(trim not in ('general','rectangular') for trim in trims):
        raise SourceError('invalid face trim type')
    if any(not surface for surface in surfaces):
        raise SourceError('empty face surface type')
    if any(face>=nfaces for face in use_faces):
        raise SourceError('faceuse refers to a nonexistent face')
    if any(o not in ('same','opposite') for o in use_orientation):
        raise SourceError('invalid faceuse orientation')
    uses = [[] for _ in range(nfaces)]
    for index,(face,orientation) in enumerate(zip(use_faces,use_orientation)):
        uses[face].append((index,orientation))
    nnurbs = sum(surface==_NURB for surface in surfaces)
    fields = {name:_field(attrs,_NPREFIX+name,type_name,optional=not nnurbs)
              for name,type_name in _FIELDS}
    for name in ('uOrder','vOrder','uVertexCount','vVertexCount'):
        _extent(fields[name],nnurbs,_NPREFIX+name)
    # Counts are bounded uint32 values; do not allocate from their products.
    counts = tuple(zip(fields['uVertexCount'],fields['vVertexCount']))
    npoints = sum(u*v for u,v in counts)
    _extent(fields['controlVertices'],npoints,_NPREFIX+'controlVertices')
    _extent(fields['weights'],npoints,_NPREFIX+'weights')
    _extent(fields['uKnots'],sum(c+o for c,o in zip(fields['uVertexCount'],fields['uOrder'])),_NPREFIX+'uKnots')
    _extent(fields['vKnots'],sum(c+o for c,o in zip(fields['vVertexCount'],fields['vOrder'])),_NPREFIX+'vKnots')
    nurbs_records = []
    cp = ku = kv = 0
    for index,(nu,nv) in enumerate(counts):
        ou,ov = fields['uOrder'][index],fields['vOrder'][index]
        nurbs = SourceNurbs(ou,ov,nu,nv,
                            fields['uKnots'][ku:ku+nu+ou],
                            fields['vKnots'][kv:kv+nv+ov],
                            fields['controlVertices'][cp:cp+nu*nv],
                            fields['weights'][cp:cp+nu*nv])
        _validate_nurbs(nurbs)
        nurbs_records.append(nurbs)
        cp += nu*nv
        ku += nu+ou
        kv += nv+ov
    rows = []
    ni = 0
    for index,surface in enumerate(surfaces):
        nurbs = None
        if surface==_NURB:
            nurbs = nurbs_records[ni]
            ni += 1
        a,b = ranges[2*index:2*index+2]
        if a[0]>b[0] or a[1]>b[1]:
            raise SourceError('face range minimum exceeds maximum')
        rows.append((index,surface,trims[index],(a[0],b[0],a[1],b[1]),
                     loops[index],tuple(uses[index]),nurbs))
    return rows


def _pow2(exponent):
    return F(2**exponent) if exponent>=0 else F(1,2**(-exponent))


def _carrier(nurbs, lineage):
    """Return (patch, bounds, reason); unsupported is explicit, never a fit."""
    zero = (F(0),)*3
    if nurbs is None:
        return None,zero,'analytic or unknown surface type is not supported'
    if nurbs.count_u!=nurbs.order_u or nurbs.count_v!=nurbs.order_v:
        return None,zero,'multiple spans or non-Bezier control count'
    if nurbs.order_u>4 or nurbs.order_v>4:
        return None,zero,'degree exceeds three'
    for knots,order in ((nurbs.knots_u,nurbs.order_u),(nurbs.knots_v,nurbs.order_v)):
        if knots != (knots[0],)*order+(knots[-1],)*order:
            return None,zero,'single span is not clamped at both endpoints'
    domain = nurbs.domain
    lu,lv = F(domain[1])-F(domain[0]),F(domain[3])-F(domain[2])
    if not all(_pow2(-20)<=length<=_pow2(20) for length in (lu,lv)):
        return None,zero,'source span outside native frame envelope'
    if not all(math.isfinite(length) and length>0
               for length in (domain[1]-domain[0],domain[3]-domain[2])):
        return None,zero,'rounded source span collapsed or overflowed'
    # Minimum admissible power of two fixes the entire feasible scale interval.
    # Fraction permits exact scaling even for subnormal original weights.
    exponent = 1-math.frexp(min(nurbs.weights))[1]
    factor = _pow2(exponent)
    weights = tuple(F(w)*factor for w in nurbs.weights)
    if any(not 1<=w<=256 for w in weights):
        return None,zero,'no common power-of-two weight scale in [1,256]'
    emitted_weights = tuple(float(w) for w in weights)
    if any(F(w)!=original for w,original in zip(emitted_weights,weights)):
        return None,zero,'weight scaling cannot preserve exact source ratios'
    maximum = max(abs(F(x)) for p in nurbs.points for x in p)
    scale = next((float(_pow2(e)) for e in range(-20,21) if maximum<=_pow2(e)),None)
    if scale is None:
        return None,zero,'coordinates exceed spatial normalization envelope'
    controls = tuple(tuple(float(F(x)*w/F(scale)) for x in p)+(ew,)
                     for p,w,ew in zip(nurbs.points,weights,emitted_weights))
    delta = max(abs(F(c[k])/F(c[3])*F(scale)-F(p[k]))
                for c,p in zip(controls,nurbs.points) for k in range(3))
    ratio = max(weights)/min(weights)
    bounds = (delta,4*(nurbs.order_u-1)*ratio*delta/lu,
                    4*(nurbs.order_v-1)*ratio*delta/lv)
    patch = Patch(nurbs.order_u-1,nurbs.order_v-1,controls,lineage,
                  uv=domain,origin=(0.,0.,0.),scale=scale)
    return patch,bounds,''


def convert(data: bytes) -> Conversion:
    if type(data) is not bytes:
        raise TypeError('source must be immutable bytes')
    if len(data)>MAX_BYTES:
        raise SourceBudgetError('source exceeds byte budget')
    document = parse(data)
    digest = sha256(data).hexdigest()
    # Validate every prim before allocating a namespace or publishing any face.
    packed = tuple((prim,_packed_prim(prim)) for prim in document.prims
                   if prim.type_name=='BrepArray')
    body_id = _namespace()
    faces,patches = [],[]
    for prim,rows in packed:
        for index,surface,trim,face_range,loops,uses,nurbs in rows:
            ordinal = len(faces)
            lineage = Lineage(body_id,5,ordinal+1,1,ordinal+1)
            patch,bounds,reason = _carrier(nurbs,lineage)
            slot = None if patch is None else len(patches)
            if patch is not None:
                patches.append(patch)
            faces.append(SourceFace((digest,prim.path,index),ordinal,surface,trim,
                         face_range,loops,uses,nurbs,
                         'unsupported' if patch is None else 'converted',reason,
                         slot,*bounds))
    result = object.__new__(Conversion)
    for name,value in (('source_bytes',data),('sha256',digest),('document',document),
                       ('body_id',body_id),('faces',tuple(faces)),('patches',tuple(patches))):
        object.__setattr__(result,name,value)
    return result


def load(path) -> Conversion:
    with open(path,'rb') as stream:
        data = stream.read(MAX_BYTES+1)
    return convert(data)
