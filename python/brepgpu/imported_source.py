"""Owned immutable imported source values and strict BGSRC001 binary I/O.

OCCT is confined to the separate source extractor. No foreign handles or JSON
are part of this module's model or persistence.
"""
from __future__ import annotations

from dataclasses import dataclass, field
import math
from pathlib import Path
import struct
from types import MappingProxyType


ZERO_FRAME = (0.0,) * 12
IDENTITY = (1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0)
_TAGS = (b'META____', b'PRODUCTS', b'SOLIDS__', b'USAGES__', b'VERTICES',
         b'EDGES___', b'COEDGES_', b'LOOPS___', b'FACES___', b'CARRIERS',
         b'CURVES__', b'STYLES__', b'DIAGS___')


@dataclass(frozen=True)
class Product:
    id: int
    name: str
    reference: str
    definition_id: int = 0
    style_id: int = 0


@dataclass(frozen=True)
class SolidDefinition:
    id: int
    product_id: int
    face_ids: tuple
    edge_ids: tuple
    vertex_ids: tuple


@dataclass(frozen=True)
class Usage:
    id: int
    parent_product_id: int
    child_product_id: int
    name: str
    transform: tuple
    style_id: int = 0


@dataclass(frozen=True)
class Vertex:
    id: int
    xyz: tuple
    tolerance: float


@dataclass(frozen=True)
class Edge:
    id: int
    vertex_ids: tuple
    curve_id: int
    parameter_range: tuple
    curve_same_sense: bool
    same_parameter: bool
    degenerate: bool
    tolerance: float


@dataclass(frozen=True)
class Coedge:
    id: int
    edge_id: int
    loop_id: int
    sense: int
    curve_id: int
    parameter_range: tuple
    periodic_shift: tuple = (0, 0)
    reconstructed: bool = True
    parameter_map_kind: int = 1
    parameter_map: tuple = ()
    # Kind 4 (measured piecewise-linear) only: sampled residual mm per row.
    parameter_residuals: tuple = ()


@dataclass(frozen=True)
class Loop:
    id: int
    face_id: int
    outer: bool
    coedge_ids: tuple


@dataclass(frozen=True)
class Face:
    id: int
    definition_id: int
    carrier_id: int
    orientation: int
    loop_ids: tuple
    tolerance: float
    style_id: int = 0
    # 0 for the outer shell, k for the k-th BREP_WITH_VOIDS void shell. The
    # orientation is the face's use in the solid (shell sense composed).
    shell: int = 0


@dataclass(frozen=True)
class SurfaceSpan:
    bounds: tuple
    degrees: tuple
    controls: tuple


@dataclass(frozen=True)
class Carrier:
    id: int
    kind: int
    frame: tuple
    radii: tuple
    uv_bounds: tuple
    degrees: tuple = (0, 0)
    knot_u: tuple = ()
    mult_u: tuple = ()
    knot_v: tuple = ()
    mult_v: tuple = ()
    control_shape: tuple = (0, 0)
    controls: tuple = ()
    spans: tuple = ()


@dataclass(frozen=True)
class CurveSpan:
    parameter_range: tuple
    degree: int
    controls: tuple


@dataclass(frozen=True)
class Curve:
    id: int
    dimension: int
    kind: int
    frame: tuple
    radii: tuple
    parameter_range: tuple
    degree: int = 0
    knots: tuple = ()
    multiplicities: tuple = ()
    controls: tuple = ()
    spans: tuple = ()


@dataclass(frozen=True)
class Style:
    id: int
    source_ids: tuple
    rgba: tuple
    has_color: bool
    material_name: str = ''
    density_unit: str = ''
    density: float = 0.0
    has_density: bool = False


@dataclass(frozen=True)
class Diagnostic:
    stage: str
    severity: int
    source_ids: tuple
    message: str


@dataclass(frozen=True)
class ImportedSource:
    source_sha256: str
    units_mm: float
    source_uncertainty_mm: float
    root_product_id: int
    adapter_revision: str
    products: tuple
    definitions: tuple
    usages: tuple
    vertices: tuple
    edges: tuple
    coedges: tuple
    loops: tuple
    faces: tuple
    carriers: tuple
    curves: tuple
    styles: tuple = ()
    diagnostics: tuple = ()
    _indices: object = field(init=False, repr=False, compare=False)

    def __post_init__(self):
        indices = {}
        row_types = {'products':Product,'definitions':SolidDefinition,'usages':Usage,
                     'vertices':Vertex,'edges':Edge,'coedges':Coedge,'loops':Loop,
                     'faces':Face,'carriers':Carrier,'curves':Curve,'styles':Style}
        immutable_types=tuple(row_types.values())+(SurfaceSpan,CurveSpan,Diagnostic)
        def immutable(value):
            if type(value) in (str,int,float,bool,type(None)):return
            if type(value) is tuple:
                for item in value:immutable(item)
                return
            if type(value) in immutable_types:
                for name in value.__dataclass_fields__:immutable(getattr(value,name))
                return
            raise TypeError('imported source values must be owned immutable native values')
        for name in ('products', 'definitions', 'usages', 'vertices', 'edges',
                     'coedges', 'loops', 'faces', 'carriers', 'curves', 'styles'):
            rows = getattr(self, name)
            if type(rows) is not tuple:
                raise TypeError(f'{name} must be an immutable tuple')
            table = {}
            for row in rows:
                if type(row) is not row_types[name]:raise TypeError(f'{name}: wrong owned row type')
                immutable(row)
                if type(row.id) is not int or row.id in table or not 0 < row.id < 2**64:
                    raise ValueError(f'{name}: duplicate or invalid ID {row.id}')
                table[row.id] = row
            indices[name] = MappingProxyType(table)
        object.__setattr__(self, '_indices', MappingProxyType(indices))
        if type(self.diagnostics) is not tuple or any(type(d) is not Diagnostic for d in self.diagnostics):
            raise TypeError('diagnostics must be an immutable tuple of owned values')
        immutable(self.diagnostics)

    def _get(self, collection, identifier):
        try:
            return self._indices[collection][identifier]
        except KeyError:
            raise ValueError(f'{collection}: unknown source ID {identifier}') from None

    def product(self, identifier): return self._get('products', identifier)
    def definition(self, identifier): return self._get('definitions', identifier)
    def usage(self, identifier): return self._get('usages', identifier)
    def vertex(self, identifier): return self._get('vertices', identifier)
    def edge(self, identifier): return self._get('edges', identifier)
    def coedge(self, identifier): return self._get('coedges', identifier)
    def loop(self, identifier): return self._get('loops', identifier)
    def face(self, identifier): return self._get('faces', identifier)
    def carrier(self, identifier): return self._get('carriers', identifier)
    def curve(self, identifier): return self._get('curves', identifier)
    def style(self, identifier): return self._get('styles', identifier)


def _finite(values, owner):
    if any(not math.isfinite(x) for x in values):
        raise ValueError(f'{owner}: nonfinite scalar')


def _knots(knots, mults, degree, count, owner):
    _finite(knots, owner)
    if (len(knots) < 2 or len(knots) != len(mults)
            or any(a >= b for a, b in zip(knots, knots[1:]))
            or any(type(m) is not int or not 0 < m <= degree + 1 for m in mults)
            or sum(mults) != count + degree + 1):
        raise ValueError(f'{owner}: invalid knot/control relation')


def _controls(controls, dimension, expected, owner):
    _finite(controls, owner)
    if len(controls) != (dimension + 1) * expected:
        raise ValueError(f'{owner}: invalid control count')
    if any(w <= 0 for w in controls[dimension::dimension + 1]):
        raise ValueError(f'{owner}: nonpositive homogeneous weight')


RIGID_PLACEMENT_TOLERANCE = 1e-9


def rigid_placement_status(m):
    """Mirror of brepgpu::rigid_placement_status (native/rigid_placement.hpp).

    Same binary64 operations in the same order, so both readers and the
    display classify every row-major 3x4 map identically.
    """
    if any(not math.isfinite(x) for x in m):
        return 'nonfinite'
    for row in range(3):
        for col in range(3):
            value = 0.0
            for k in range(3):
                value += m[4*row+k]*m[4*col+k]
            if not abs(value-(1.0 if row == col else 0.0)) <= RIGID_PLACEMENT_TOLERANCE:
                return 'nonorthonormal'
    det = (m[0]*(m[5]*m[10]-m[6]*m[9]) - m[1]*(m[4]*m[10]-m[6]*m[8])
           + m[2]*(m[4]*m[9]-m[5]*m[8]))
    return 'rigid' if det > 0 else 'improper'


def validate_source(source):
    """Validate wire structure and source ownership, not geometric correctness."""
    if type(source) is not ImportedSource:
        raise TypeError('expected ImportedSource')
    try:
        raw_hash = bytes.fromhex(source.source_sha256)
    except (ValueError, TypeError):
        raise ValueError('invalid source SHA-256') from None
    if len(raw_hash) != 32:
        raise ValueError('invalid source SHA-256 length')
    _finite((source.units_mm, source.source_uncertainty_mm), 'source units')
    if source.units_mm <= 0 or source.source_uncertainty_mm < 0:
        raise ValueError('invalid source units/uncertainty')
    source.product(source.root_product_id)
    for product in source.products:
        if product.definition_id:
            if source.definition(product.definition_id).product_id != product.id:
                raise ValueError(f'product {product.id}: definition owner mismatch')
        if product.style_id: source.style(product.style_id)
    for definition in source.definitions:
        source.product(definition.product_id)
        for name, identifiers in (('faces', definition.face_ids),
                                  ('edges', definition.edge_ids),
                                  ('vertices', definition.vertex_ids)):
            if len(set(identifiers)) != len(identifiers):
                raise ValueError(f'definition {definition.id}: duplicate {name}')
            for identifier in identifiers: source._get(name, identifier)
        if any(source.face(i).definition_id != definition.id for i in definition.face_ids):
            raise ValueError(f'definition {definition.id}: face owner mismatch')
        shells = {source.face(i).shell for i in definition.face_ids}
        if definition.face_ids and shells != set(range(len(shells))):
            raise ValueError(f'definition {definition.id}: face shell ordinals are not contiguous')
    for usage in source.usages:
        source.product(usage.parent_product_id)
        source.product(usage.child_product_id)
        _finite(usage.transform, f'usage {usage.id}')
        if len(usage.transform) != 12: raise ValueError('invalid placement length')
        status = rigid_placement_status(usage.transform)
        if status in ('nonfinite', 'nonorthonormal'):
            raise ValueError(f'usage {usage.id}: nonrigid placement')
        if status != 'rigid': raise ValueError(f'usage {usage.id}: reflected placement')
        if usage.style_id: source.style(usage.style_id)
    for vertex in source.vertices:
        _finite(vertex.xyz + (vertex.tolerance,), f'vertex {vertex.id}')
        if len(vertex.xyz) != 3 or vertex.tolerance < 0: raise ValueError('invalid vertex')
    for edge in source.edges:
        if len(edge.vertex_ids) != 2: raise ValueError('edge needs two endpoint occurrences')
        for i in edge.vertex_ids: source.vertex(i)
        if edge.curve_id:
            curve=source.curve(edge.curve_id)
            if curve.dimension != 3: raise ValueError('spatial curve dimension')
            if curve.kind==4 and any(not 0<=t<=1 for t in edge.parameter_range):
                raise ValueError('active edge range leaves natural Bezier carrier domain')
        elif not edge.degenerate: raise ValueError('nondegenerate edge has no curve')
        _finite(edge.parameter_range + (edge.tolerance,), f'edge {edge.id}')
        if len(edge.parameter_range) != 2 or edge.tolerance < 0: raise ValueError('invalid edge')
    for coedge in source.coedges:
        edge = source.edge(coedge.edge_id)
        loop = source.loop(coedge.loop_id)
        if coedge.id not in loop.coedge_ids: raise ValueError('coedge missing from owner')
        curve=source.curve(coedge.curve_id)
        if curve.dimension != 2: raise ValueError('pcurve dimension')
        if curve.kind==4 and any(not 0<=t<=1 for t in coedge.parameter_range):
            raise ValueError('active pcurve range leaves natural Bezier carrier domain')
        if coedge.sense not in (-1, 1) or coedge.parameter_map_kind not in (1,2,3,4):
            raise ValueError('invalid coedge map/sense')
        if coedge.parameter_map_kind == 4:
            if (len(coedge.parameter_residuals) != len(coedge.parameter_map)
                    or any(type(r) is not float or not math.isfinite(r) or r < 0 for r in coedge.parameter_residuals)):
                raise ValueError('measured parameter map needs one finite nonnegative residual per row')
        elif coedge.parameter_residuals:
            raise ValueError('only measured parameter maps carry residuals')
        if not edge.same_parameter and coedge.parameter_map_kind == 1:
            raise ValueError('SameParameter=false requires explicit parameter map')
        _finite(coedge.parameter_range, f'coedge {coedge.id}')
        if coedge.parameter_map_kind != 1:
            if len(coedge.parameter_map) < 2: raise ValueError('missing parameter map endpoints')
            for pair in coedge.parameter_map:
                if len(pair) != 2: raise ValueError('invalid parameter map pair')
                _finite(pair, 'parameter map')
            direction = 1 if edge.parameter_range[1] >= edge.parameter_range[0] else -1
            if any(direction*(b[0]-a[0]) <= 0 for a,b in zip(coedge.parameter_map,coedge.parameter_map[1:])):
                raise ValueError('nonmonotone edge parameter map')
    for loop in source.loops:
        face = source.face(loop.face_id)
        if loop.id not in face.loop_ids or not loop.coedge_ids:
            raise ValueError('invalid loop ownership')
        if any(source.coedge(i).loop_id != loop.id for i in loop.coedge_ids):
            raise ValueError('loop/coedge owner mismatch')
    for face in source.faces:
        definition = source.definition(face.definition_id)
        if face.id not in definition.face_ids: raise ValueError('face missing from definition')
        source.carrier(face.carrier_id)
        if face.orientation not in (-1,1) or not face.loop_ids: raise ValueError('invalid face')
        if type(face.shell) is not int or not 0 <= face.shell < 2**32: raise ValueError('invalid face shell')
        if any(source.loop(i).face_id != face.id for i in face.loop_ids):
            raise ValueError('face/loop owner mismatch')
        _finite((face.tolerance,), 'face tolerance')
        if face.tolerance < 0: raise ValueError('negative face tolerance')
        if face.style_id: source.style(face.style_id)
    for carrier in source.carriers:
        _finite(carrier.frame + carrier.radii + carrier.uv_bounds, f'carrier {carrier.id}')
        if len(carrier.frame) != 12 or len(carrier.radii) != 2 or len(carrier.uv_bounds) != 4:
            raise ValueError('invalid carrier dimensions')
        u0,u1,v0,v1 = carrier.uv_bounds
        if u0 >= u1 or v0 >= v1 or carrier.kind not in (1,2,3,4,5,6):
            raise ValueError('invalid carrier domain/type')
        if carrier.kind == 6:
            p,q = carrier.degrees
            nu,nv = carrier.control_shape
            if not (0 <= p <= 3 and 0 <= q <= 3 and nu > p and nv > q):
                raise ValueError('unsupported surface degree/control shape')
            _controls(carrier.controls,3,nu*nv,'surface')
            _knots(carrier.knot_u,carrier.mult_u,p,nu,'surface U')
            _knots(carrier.knot_v,carrier.mult_v,q,nv,'surface V')
            if not carrier.spans: raise ValueError('B-spline surface has no extracted spans')
            for span in carrier.spans:
                _finite(span.bounds,'span bounds')
                if span.degrees != carrier.degrees: raise ValueError('span degree mismatch')
                _controls(span.controls,3,(p+1)*(q+1),'surface span')
    for curve in source.curves:
        _finite(curve.frame + curve.radii + curve.parameter_range, f'curve {curve.id}')
        if curve.dimension not in (2,3) or curve.kind not in (1,2,3,4,5):
            raise ValueError('unsupported curve dimension/type')
        if len(curve.frame) != 12 or len(curve.radii) != 2 or len(curve.parameter_range) != 2:
            raise ValueError('invalid curve dimensions')
        if curve.kind in (4,5):
            if not 0 <= curve.degree <= 32: raise ValueError('unsupported curve degree')
            n = len(curve.controls)//(curve.dimension+1)
            _controls(curve.controls,curve.dimension,n,'curve')
            if curve.kind == 4 and n != curve.degree+1: raise ValueError('Bezier degree mismatch')
            if curve.kind == 4 and curve.parameter_range != (0.0,1.0):
                raise ValueError('Bezier carrier uses its natural [0,1] parameter domain')
            if curve.kind == 5: _knots(curve.knots,curve.multiplicities,curve.degree,n,'curve')
            for span in curve.spans:
                _finite(span.parameter_range,'curve span')
                if span.degree != curve.degree: raise ValueError('curve span degree mismatch')
                _controls(span.controls,curve.dimension,span.degree+1,'curve span')
    for style in source.styles:
        _finite(style.rgba+(style.density,),f'style {style.id}')
        if len(style.rgba) != 4 or any(not 0 <= c <= 1 for c in style.rgba):
            raise ValueError('invalid source color')
        if style.has_density and style.density < 0: raise ValueError('negative density')
    if any(d.severity >= 2 for d in source.diagnostics):
        raise ValueError('source adapter reported an error diagnostic')
    return source


class _Reader:
    def __init__(self, data): self.data, self.pos = memoryview(data), 0
    def take(self,n):
        if n < 0 or n > len(self.data)-self.pos: raise ValueError('truncated source binary')
        result = self.data[self.pos:self.pos+n]; self.pos += n
        return result
    def number(self,fmt): return struct.unpack('<'+fmt,self.take(struct.calcsize('<'+fmt)))[0]
    def numbers(self,fmt,n):
        width=struct.calcsize('<'+fmt)
        if n > (len(self.data)-self.pos)//width: raise ValueError('oversized binary vector')
        return tuple(struct.unpack('<'+str(n)+fmt,self.take(width*n))) if n else ()
    def vector(self,fmt): return self.numbers(fmt,self.number('Q'))
    def string(self):
        try: return bytes(self.take(self.number('I'))).decode('utf-8')
        except UnicodeDecodeError: raise ValueError('invalid UTF-8 source string') from None
    def zero(self):
        if self.number('I'): raise ValueError('nonzero reserved source field')
    def boolean(self):
        x=self.number('I')
        if x not in (0,1): raise ValueError('invalid source boolean')
        return bool(x)
    def row(self): return _Reader(self.take(self.number('Q')))
    def end(self):
        if self.pos != len(self.data): raise ValueError('trailing source record bytes')


def _surface_span(r):
    return SurfaceSpan(r.numbers('d',4),r.numbers('I',2),r.vector('d'))


def _curve_span(r):
    interval=r.numbers('d',2); degree=r.number('I'); r.zero()
    return CurveSpan(interval,degree,r.vector('d'))


def _nested(r,read):
    count=r.number('Q')
    if count > (len(r.data)-r.pos)//8: raise ValueError('oversized nested source records')
    result=[]
    for _ in range(count):
        rr=r.row(); result.append(read(rr)); rr.end()
    return tuple(result)


def _read_row(tag,r):
    q=lambda:r.number('Q')
    if tag == b'META____': return r.string()
    if tag == b'PRODUCTS':
        i,d,s=q(),q(),q(); return Product(i,r.string(),r.string(),d,s)
    if tag == b'SOLIDS__': return SolidDefinition(q(),q(),r.vector('Q'),r.vector('Q'),r.vector('Q'))
    if tag == b'USAGES__':
        i,p,c,s=q(),q(),q(),q(); return Usage(i,p,c,r.string(),r.numbers('d',12),s)
    if tag == b'VERTICES': return Vertex(q(),r.numbers('d',3),r.number('d'))
    if tag == b'EDGES___':
        i,a,b,c=q(),q(),q(),q(); interval=r.numbers('d',2)
        same,sparam,deg=r.boolean(),r.boolean(),r.boolean(); r.zero()
        return Edge(i,(a,b),c,interval,same,sparam,deg,r.number('d'))
    if tag == b'COEDGES_':
        i,e,l,c=q(),q(),q(),q(); sense=r.number('i'); recon=r.boolean()
        interval=r.numbers('d',2); shift=r.numbers('i',2); kind=r.number('I'); r.zero()
        values=r.vector('d'); width=3 if kind==4 else 2
        if len(values)%width: raise ValueError('parameter map vector is not whole rows')
        rows=tuple(zip(values[::width],values[1::width]))
        return Coedge(i,e,l,sense,c,interval,shift,recon,kind,rows,values[2::3] if width==3 else ())
    if tag == b'LOOPS___':
        i,f=q(),q(); outer=r.boolean(); r.zero(); return Loop(i,f,outer,r.vector('Q'))
    if tag == b'FACES___':
        i,d,c,s=q(),q(),q(),q(); orientation=r.number('i'); shell=r.number('I'); tolerance=r.number('d')
        return Face(i,d,c,orientation,r.vector('Q'),tolerance,s,shell)
    if tag == b'CARRIERS':
        i=q(); kind=r.number('I'); r.zero(); frame=r.numbers('d',12); radii=r.numbers('d',2)
        uv=r.numbers('d',4); degree=r.numbers('I',2); shape=r.numbers('I',2)
        ku,mu,kv,mv=r.vector('d'),r.vector('I'),r.vector('d'),r.vector('I')
        controls=r.vector('d'); spans=_nested(r,_surface_span)
        return Carrier(i,kind,frame,radii,uv,degree,ku,mu,kv,mv,shape,controls,spans)
    if tag == b'CURVES__':
        i=q(); dim=r.number('I'); kind=r.number('I'); frame=r.numbers('d',12)
        radii=r.numbers('d',2); interval=r.numbers('d',2); degree=r.number('I'); r.zero()
        knots,mults,controls=r.vector('d'),r.vector('I'),r.vector('d'); spans=_nested(r,_curve_span)
        return Curve(i,dim,kind,frame,radii,interval,degree,knots,mults,controls,spans)
    if tag == b'STYLES__':
        i=q(); ids=r.vector('Q'); rgba=r.numbers('d',4); color,density=r.boolean(),r.boolean()
        name,unit=r.string(),r.string(); return Style(i,ids,rgba,color,name,unit,r.number('d'),density)
    if tag == b'DIAGS___':
        severity=r.number('I'); r.zero(); stage=r.string(); ids=r.vector('Q')
        return Diagnostic(stage,severity,ids,r.string())
    raise ValueError('unknown source section')


def load_source(path):
    """Read an exact, bounded BGSRC001 snapshot and validate its ownership."""
    path=Path(path)
    if path.stat().st_size > 2**30: raise ValueError('source binary exceeds 1 GiB limit')
    data=path.read_bytes(); r=_Reader(data)
    if bytes(r.take(8)) != b'BGSRC001' or r.number('I') != 1:
        raise ValueError('unsupported source magic/version')
    r.zero(); digest=bytes(r.take(32)).hex(); units,uncertainty=r.numbers('d',2)
    root=r.number('Q'); count=r.number('I'); r.zero(); length=r.number('Q')
    if count != len(_TAGS) or length != len(data): raise ValueError('source file count/length mismatch')
    tables=[]
    for expected in _TAGS:
        tag=bytes(r.take(8)); count=r.number('Q'); payload=r.number('Q')
        if tag != expected: raise ValueError('source section order/type mismatch')
        section=_Reader(r.take(payload))
        if count > payload//8: raise ValueError('oversized source section count')
        rows=[]
        for _ in range(count):
            row=section.row(); rows.append(_read_row(tag,row)); row.end()
        section.end(); tables.append(tuple(rows))
    r.end()
    if len(tables[0]) != 1: raise ValueError('source must have one metadata row')
    return validate_source(ImportedSource(digest,units,uncertainty,root,tables[0][0],*tables[1:]))


class _Writer:
    def __init__(self): self.data=bytearray()
    def number(self,fmt,value): self.data.extend(struct.pack('<'+fmt,value))
    def numbers(self,fmt,values):
        if values: self.data.extend(struct.pack('<'+str(len(values))+fmt,*values))
    def vector(self,fmt,values): self.number('Q',len(values)); self.numbers(fmt,values)
    def string(self,text):
        raw=text.encode('utf-8'); self.number('I',len(raw)); self.data.extend(raw)
    def zero(self): self.number('I',0)
    def row(self,writer): self.number('Q',len(writer.data)); self.data.extend(writer.data)


def _write_row(tag,value):
    w=_Writer(); q=lambda x:w.number('Q',x); d=lambda x:w.number('d',x)
    if tag == b'META____': w.string(value); return w
    x=value
    if tag == b'PRODUCTS':
        for v in (x.id,x.definition_id,x.style_id): q(v)
        w.string(x.name); w.string(x.reference)
    elif tag == b'SOLIDS__':
        q(x.id); q(x.product_id)
        for v in (x.face_ids,x.edge_ids,x.vertex_ids): w.vector('Q',v)
    elif tag == b'USAGES__':
        for v in (x.id,x.parent_product_id,x.child_product_id,x.style_id): q(v)
        w.string(x.name); w.numbers('d',x.transform)
    elif tag == b'VERTICES': q(x.id); w.numbers('d',x.xyz); d(x.tolerance)
    elif tag == b'EDGES___':
        for v in (x.id,*x.vertex_ids,x.curve_id): q(v)
        w.numbers('d',x.parameter_range)
        for v in (x.curve_same_sense,x.same_parameter,x.degenerate,0): w.number('I',int(v))
        d(x.tolerance)
    elif tag == b'COEDGES_':
        for v in (x.id,x.edge_id,x.loop_id,x.curve_id): q(v)
        w.number('i',x.sense); w.number('I',int(x.reconstructed)); w.numbers('d',x.parameter_range)
        w.numbers('i',x.periodic_shift); w.number('I',x.parameter_map_kind); w.zero()
        rows=(tuple((*pair,r) for pair,r in zip(x.parameter_map,x.parameter_residuals))
              if x.parameter_map_kind==4 else x.parameter_map)
        w.vector('d',tuple(v for row in rows for v in row))
    elif tag == b'LOOPS___':
        q(x.id); q(x.face_id); w.number('I',int(x.outer)); w.zero(); w.vector('Q',x.coedge_ids)
    elif tag == b'FACES___':
        for v in (x.id,x.definition_id,x.carrier_id,x.style_id): q(v)
        w.number('i',x.orientation); w.number('I',x.shell); d(x.tolerance); w.vector('Q',x.loop_ids)
    elif tag == b'CARRIERS':
        q(x.id); w.number('I',x.kind); w.zero(); w.numbers('d',x.frame)
        w.numbers('d',x.radii); w.numbers('d',x.uv_bounds); w.numbers('I',x.degrees)
        w.numbers('I',x.control_shape)
        for fmt,v in (('d',x.knot_u),('I',x.mult_u),('d',x.knot_v),('I',x.mult_v),('d',x.controls)):
            w.vector(fmt,v)
        q(len(x.spans))
        for span in x.spans:
            sw=_Writer(); sw.numbers('d',span.bounds); sw.numbers('I',span.degrees)
            sw.vector('d',span.controls); w.row(sw)
    elif tag == b'CURVES__':
        q(x.id); w.number('I',x.dimension); w.number('I',x.kind); w.numbers('d',x.frame)
        w.numbers('d',x.radii); w.numbers('d',x.parameter_range); w.number('I',x.degree); w.zero()
        w.vector('d',x.knots); w.vector('I',x.multiplicities); w.vector('d',x.controls)
        q(len(x.spans))
        for span in x.spans:
            sw=_Writer(); sw.numbers('d',span.parameter_range); sw.number('I',span.degree)
            sw.zero(); sw.vector('d',span.controls); w.row(sw)
    elif tag == b'STYLES__':
        q(x.id); w.vector('Q',x.source_ids); w.numbers('d',x.rgba)
        w.number('I',int(x.has_color)); w.number('I',int(x.has_density))
        w.string(x.material_name); w.string(x.density_unit); d(x.density)
    elif tag == b'DIAGS___':
        w.number('I',x.severity); w.zero(); w.string(x.stage); w.vector('Q',x.source_ids); w.string(x.message)
    else: raise ValueError('unknown source section')
    return w


def write_source(source,path):
    """Write the same owned binary for analytic fixtures and source snapshots."""
    validate_source(source)
    tables=((source.adapter_revision,),source.products,source.definitions,source.usages,
            source.vertices,source.edges,source.coedges,source.loops,source.faces,
            source.carriers,source.curves,source.styles,source.diagnostics)
    body=_Writer()
    for tag,rows in zip(_TAGS,tables):
        section=_Writer()
        for row in rows: section.row(_write_row(tag,row))
        body.data.extend(tag); body.number('Q',len(rows)); body.number('Q',len(section.data))
        body.data.extend(section.data)
    header=_Writer(); header.data.extend(b'BGSRC001'); header.number('I',1); header.zero()
    header.data.extend(bytes.fromhex(source.source_sha256)); header.numbers('d',(source.units_mm,source.source_uncertainty_mm))
    header.number('Q',source.root_product_id); header.number('I',len(_TAGS)); header.zero()
    header.number('Q',88+len(body.data)); Path(path).write_bytes(header.data+body.data)
