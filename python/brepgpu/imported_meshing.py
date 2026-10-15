"""Owned source-keyed trimmed meshing of imported analytic/spline B-reps.

No OCCT tessellation is read here. Face UV occurrence and physical position
identity are separate, including periodic seams and singular chart quotients.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
from fractions import Fraction
import math
from pathlib import Path
import struct
import time

from .imported_source import ImportedSource, validate_source


@dataclass(frozen=True)
class MeshQuality:
    tolerance_mm: float = 0.005
    source_residual_mm: float = 0.001
    max_refinement: int = 20
    max_edge_samples: int = 16384

    def __post_init__(self):
        if (not math.isfinite(self.tolerance_mm) or self.tolerance_mm <= 0
                or not math.isfinite(self.source_residual_mm)
                or self.source_residual_mm < 0
                or type(self.max_refinement) is not int or not 1 <= self.max_refinement <= 40
                or type(self.max_edge_samples) is not int or not 4 <= self.max_edge_samples <= 2**20):
            raise ValueError('invalid imported mesh quality')


@dataclass(frozen=True)
class MeshVertex:
    physical_key: int
    definition_id: int
    xyz: tuple
    source_kind: int
    source_id: int
    source_parameter: float
    uv: tuple


@dataclass(frozen=True)
class MeshCorner:
    uv: tuple
    carrier_slot: int
    face_orientation: int


@dataclass(frozen=True)
class MeshTriangle:
    face_id: int
    vertex_keys: tuple
    corners: tuple
    collapsed: bool = False


@dataclass(frozen=True)
class BoundaryOccurrence:
    coedge_id: int
    edge_id: int
    sample_key: int
    source_parameter: float
    uv: tuple
    pcurve_parameter: float
    pcurve_uv: tuple = ()
    recovery_kind: str = 'pcurve'


@dataclass(frozen=True)
class FaceMesh:
    face_id: int
    boundary_loops: tuple
    triangle_indices: tuple
    chart_closures: tuple = ()
    joint_recoveries: tuple = ()


@dataclass(frozen=True)
class ChartClosure:
    face_id: int
    loop_id: int
    source_vertex_id: int
    incoming_coedge_id: int
    outgoing_coedge_id: int
    uv_start: tuple
    uv_end: tuple
    kind: str


@dataclass(frozen=True)
class JointRecovery:
    face_id: int
    loop_id: int
    source_vertex_id: int
    incoming: BoundaryOccurrence
    outgoing: BoundaryOccurrence
    uv: tuple
    kind: str


@dataclass(frozen=True)
class EdgeSamples:
    edge_id: int
    parameters: tuple
    vertex_keys: tuple
    positions: tuple


@dataclass(frozen=True)
class SceneInstance:
    id: int
    definition_id: int
    transform: tuple
    explosion_offset: tuple


@dataclass(frozen=True)
class SceneMaterial:
    rgb: tuple
    roughness: float


@dataclass(frozen=True)
class ImportedMesh:
    source: ImportedSource
    quality: MeshQuality
    vertices: tuple
    triangles: tuple
    faces: tuple
    edge_samples: tuple
    timings: tuple
    error_metrics: tuple = ()

    def bounds(self,definition_id):
        points=[v.xyz for v in self.vertices if v.definition_id == definition_id]
        if not points: raise ValueError(f'no mesh vertices for definition {definition_id}')
        return tuple(min(p[k] for p in points) for k in range(3))+tuple(max(p[k] for p in points) for k in range(3))

    @property
    def definition_bounds(self):
        return tuple((d.id,self.bounds(d.id)) for d in self.source.definitions)

    def write_scene(self,path,instances,materials,definition_materials=()):
        return _write_scene(self,path,instances,materials,definition_materials)


def _add(a,b): return tuple(x+y for x,y in zip(a,b))
def _sub(a,b): return tuple(x-y for x,y in zip(a,b))
def _scale(a,s): return tuple(x*s for x in a)
def _dot(a,b): return sum(x*y for x,y in zip(a,b))
def _norm(a): return math.sqrt(_dot(a,a))
def _distance(a,b): return _norm(_sub(a,b))
def _cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])


def _orient(a,b,c):
    """Filtered orientation; exact binary64 rational sign on uncertain input."""
    if any(isinstance(x,Fraction) for p in (a,b,c) for x in p):
        ax,bx=Fraction(a[0])-Fraction(c[0]),Fraction(b[0])-Fraction(c[0])
        ay,by=Fraction(a[1])-Fraction(c[1]),Fraction(b[1])-Fraction(c[1])
        value=ax*by-ay*bx
        return (value>0)-(value<0)
    ax,bx=a[0]-c[0],b[0]-c[0]; ay,by=a[1]-c[1],b[1]-c[1]
    left,right=ax*by,ay*bx; value=left-right
    if not math.isfinite(value): raise ValueError('nonfinite planar predicate')
    if abs(value) > 3.3306690738754716e-16*(abs(left)+abs(right)):
        return 1 if value > 0 else -1
    ax,bx=Fraction(a[0])-Fraction(c[0]),Fraction(b[0])-Fraction(c[0])
    ay,by=Fraction(a[1])-Fraction(c[1]),Fraction(b[1])-Fraction(c[1])
    exact=ax*by-ay*bx
    return (exact > 0)-(exact < 0)


def _on_segment(a,b,p):
    return (_orient(a,b,p)==0 and min(a[0],b[0]) <= p[0] <= max(a[0],b[0])
            and min(a[1],b[1]) <= p[1] <= max(a[1],b[1]))


def _segments_touch(a,b,c,d):
    if (max(a[0],b[0]) < min(c[0],d[0]) or max(c[0],d[0]) < min(a[0],b[0])
            or max(a[1],b[1]) < min(c[1],d[1]) or max(c[1],d[1]) < min(a[1],b[1])):
        return False
    x,y,z,w=_orient(a,b,c),_orient(a,b,d),_orient(c,d,a),_orient(c,d,b)
    return ((x*y < 0 and z*w < 0) or (x==0 and _on_segment(a,b,c))
            or (y==0 and _on_segment(a,b,d)) or (z==0 and _on_segment(c,d,a))
            or (w==0 and _on_segment(c,d,b)))


def _winding(poly,p):
    n=0
    for a,b in zip(poly,poly[1:]+poly[:1]):
        if _on_segment(a,b,p): return None
        if a[1] <= p[1] < b[1] and _orient(a,b,p)>0: n+=1
        if b[1] <= p[1] < a[1] and _orient(a,b,p)<0: n-=1
    return n


def _area(poly):
    return math.fsum(a[0]*b[1]-a[1]*b[0] for a,b in zip(poly,poly[1:]+poly[:1]))/2


def _casteljau(controls,t):
    row=list(controls)
    for n in range(len(row)-1,0,-1):
        row=[tuple((1-t)*a+t*b for a,b in zip(row[j],row[j+1])) for j in range(n)]
    return row[0]


def _control_rows(flat,dimension):
    stride=dimension+1
    return tuple(tuple(flat[i:i+stride]) for i in range(0,len(flat),stride))


def _split_controls(controls,t):
    row=list(controls); left=[row[0]]; right=[row[-1]]
    while len(row)>1:
        row=[tuple((1-t)*a+t*b for a,b in zip(row[i],row[i+1])) for i in range(len(row)-1)]
        left.append(row[0]);right.append(row[-1])
    return tuple(left),tuple(reversed(right))


def _restrict_controls(controls,a,b):
    if a==0 and b==1: return controls
    if a>b: return tuple(reversed(_restrict_controls(controls,b,a)))
    if b<1: controls=_split_controls(controls,b)[0]
    if a>0: controls=_split_controls(controls,a/b)[1]
    return controls


def _recover(control):
    w=control[-1]
    if w<=0 or not math.isfinite(w): raise ValueError('invalid homogeneous evaluation weight')
    result=tuple(x/w for x in control[:-1])
    if any(not math.isfinite(x) for x in result): raise ValueError('nonfinite source evaluation')
    return result


def _curve_span(curve,t):
    for span in curve.spans:
        a,b=span.parameter_range
        if a <= t <= b:
            return span
    raise ValueError(f'curve {curve.id}: parameter {t} outside source spans')


def _curve_interval_span(curve,a,b):
    lo,hi=sorted((a,b))
    for span in curve.spans:
        if span.parameter_range[0]<=lo<hi<=span.parameter_range[1]:return span
    raise ValueError(f'curve {curve.id}: interval crosses original spans: {a},{b}')


def evaluate_curve(curve,t):
    """Evaluate original curve parameters; degree-five support is preserved."""
    o=curve.frame[:3];x=curve.frame[3:6];y=curve.frame[6:9]
    if curve.kind==1: result=_add(o,_scale(x,t))
    elif curve.kind in (2,3):
        result=_add(o,_add(_scale(x,curve.radii[0]*math.cos(t)),_scale(y,curve.radii[1]*math.sin(t))))
    elif curve.kind==4:
        if curve.parameter_range!=(0.0,1.0):raise ValueError('Bezier carrier requires natural [0,1] domain')
        result=_recover(_casteljau(_control_rows(curve.controls,curve.dimension),t))
    else:
        span=_curve_span(curve,t);a,b=span.parameter_range
        result=_recover(_casteljau(_control_rows(span.controls,curve.dimension),(t-a)/(b-a)))
    result=result[:curve.dimension]
    if any(not math.isfinite(v) for v in result): raise ValueError('nonfinite curve output')
    return result


def _surface_span(carrier,u,v):
    for span in carrier.spans:
        a,b,c,d=span.bounds
        if a <= u <= b and c <= v <= d:
            return span
    raise ValueError(f'carrier {carrier.id}: UV outside original spans: {u},{v}')


def _bezier_surface(span,u,v):
    p,q=span.degrees;a,b,c,d=span.bounds;s,t=(u-a)/(b-a),(v-c)/(d-c)
    controls=_control_rows(span.controls,3)
    rows=[_casteljau(controls[i*(q+1):(i+1)*(q+1)],t) for i in range(p+1)]
    h=_casteljau(rows,s)
    hu=_scale(_casteljau([_scale(_sub(rows[i+1],rows[i]),p) for i in range(p)],s),1/(b-a)) if p else (0,0,0,0)
    vrows=[_casteljau([_scale(_sub(controls[i*(q+1)+j+1],controls[i*(q+1)+j]),q) for j in range(q)],t) if q else (0,0,0,0) for i in range(p+1)]
    hv=_scale(_casteljau(vrows,s),1/(d-c))
    pos=_recover(h);du=tuple((hu[i]-pos[i]*hu[3])/h[3] for i in range(3));dv=tuple((hv[i]-pos[i]*hv[3])/h[3] for i in range(3))
    return pos,du,dv


def evaluate_carrier(carrier,u,v):
    """Original-domain position and first partials for meshing consumers."""
    u,v=float(u),float(v)
    if carrier.kind==6: return _bezier_surface(_surface_span(carrier,u,v),u,v)
    o,x,y,z=(carrier.frame[i:i+3] for i in (0,3,6,9))
    if carrier.kind==1: return _add(o,_add(_scale(x,u),_scale(y,v))),x,y
    e=_add(_scale(x,math.cos(u)),_scale(y,math.sin(u)))
    ep=_add(_scale(x,-math.sin(u)),_scale(y,math.cos(u)))
    r,a=carrier.radii
    if carrier.kind==2:
        return _add(o,_add(_scale(e,r),_scale(z,v))),_scale(ep,r),z
    if carrier.kind==3:
        radius=r+v*math.sin(a)
        return (_add(o,_add(_scale(e,radius),_scale(z,v*math.cos(a)))),
                _scale(ep,radius),_add(_scale(e,math.sin(a)),_scale(z,math.cos(a))))
    if carrier.kind==4:
        return (_add(o,_add(_scale(e,r*math.cos(v)),_scale(z,r*math.sin(v)))),
                _scale(ep,r*math.cos(v)),_add(_scale(e,-r*math.sin(v)),_scale(z,r*math.cos(v))))
    if carrier.kind==5:
        radius=r+a*math.cos(v)
        return (_add(o,_add(_scale(e,radius),_scale(z,a*math.sin(v)))),
                _scale(ep,radius),_add(_scale(e,-a*math.sin(v)),_scale(z,a*math.cos(v))))
    raise ValueError('unsupported carrier')


def _frame_coordinates(carrier,point,*,exact=False):
    values=tuple(Fraction(x) for x in carrier.frame) if exact else carrier.frame
    target=tuple(Fraction(x) for x in point) if exact else point
    delta=_sub(target,values[:3]);axes=[values[i:i+3] for i in (3,6,9)]
    determinant=_dot(axes[0],_cross(axes[1],axes[2]))
    if not math.isfinite(determinant) or determinant==0:raise ValueError('singular original analytic frame')
    covectors=[_scale(_cross(axes[1],axes[2]),1/determinant),
               _scale(_cross(axes[2],axes[0]),1/determinant),
               _scale(_cross(axes[0],axes[1]),1/determinant)]
    return tuple(_dot(delta,covector) for covector in covectors),covectors


def _lift_angle(angle,seed):
    period=2*math.pi;u=angle+math.floor((seed-angle)/period+.5)*period
    grade=32*math.ulp(max(1.,abs(u),abs(seed)))
    if abs(u-seed)>=math.pi-grade:raise ValueError('analytic inverse branch ambiguity')
    return u


def _cylinder_coordinates(carrier,point):
    if carrier.kind!=2 or carrier.radii[0]<=0:raise ValueError('regular original cylinder required')
    return _frame_coordinates(carrier,point)


def _cylinder_uv(carrier,point,seed):
    local,_=_cylinder_coordinates(carrier,point)
    if not all(math.isfinite(x) for x in local) or math.hypot(*local[:2])==0:
        raise ValueError('cylinder inverse axis ambiguity')
    return _lift_angle(math.atan2(local[1],local[0]),seed[0]),local[2]


def _plane_uv(carrier,point):
    if carrier.kind!=1:raise ValueError('original plane required')
    return _frame_coordinates(carrier,point,exact=True)[0][:2]


def _cone_uv(carrier,point,seed):
    if carrier.kind!=3:raise ValueError('original cone required')
    local,_=_frame_coordinates(carrier,point);cosine=math.cos(carrier.radii[1])
    if cosine==0 or not math.isfinite(cosine):raise ValueError('cone axial inverse is singular')
    if math.hypot(*local[:2])==0:raise ValueError('cone inverse axis ambiguity')
    v=local[2]/cosine;radius=carrier.radii[0]+v*math.sin(carrier.radii[1])
    if radius==0:raise ValueError('regular cone inverse cannot select apex branch')
    angle=math.atan2(local[1],local[0])+(math.pi if radius<0 else 0.)
    return _lift_angle(angle,seed[0]),v


def _cone_apex(carrier,point):
    sine=math.sin(carrier.radii[1]);cosine=math.cos(carrier.radii[1])
    if sine==0 or cosine==0:return False
    apex=-carrier.radii[0]/sine
    expected=_add(carrier.frame[:3],_scale(carrier.frame[9:12],apex*cosine))
    local,_=_frame_coordinates(carrier,point)
    return _distance(expected,point)<=2**-32 and abs(local[2]/cosine-apex)<=2**-40*max(1.,abs(apex))


def _cone_generator(source,edge,carrier,quality,cache):
    key=(edge.id,carrier.id)
    if key in cache:return cache[key]
    curve=source.curve(edge.curve_id);result=None
    if curve.kind==1:
        a,b=edge.parameter_range;mid=evaluate_curve(curve,(a+b)/2)
        local,_=_frame_coordinates(carrier,mid);cosine=math.cos(carrier.radii[1])
        if cosine and math.hypot(*local[:2]):
            v=local[2]/cosine;radius=carrier.radii[0]+v*math.sin(carrier.radii[1])
            angle=math.atan2(local[1],local[0])+(math.pi if radius<0 else 0.)
            residual=0.
            for t in (a,b):
                point=evaluate_curve(curve,t);q,_=_frame_coordinates(carrier,point)
                residual=max(residual,_distance(evaluate_carrier(carrier,angle,q[2]/cosine)[0],point))
            if residual<=quality.source_residual_mm:result=(angle,residual)
    cache[key]=result
    return result


def _source_uv(source,edge,carrier,t,xyz,seed,quality,profiles):
    if carrier.kind==1:return _plane_uv(carrier,xyz),'plane_source_inverse'
    if carrier.kind==2:return _cylinder_uv(carrier,xyz,seed),'cylinder_source_inverse'
    if carrier.kind!=3:raise ValueError('unsupported owned source inverse family')
    profile=_cone_generator(source,edge,carrier,quality,profiles)
    endpoint=t in edge.parameter_range
    if profile is not None and (not endpoint or _cone_apex(carrier,xyz)):
        local,_=_frame_coordinates(carrier,xyz)
        v=(-carrier.radii[0]/math.sin(carrier.radii[1]) if endpoint else local[2]/math.cos(carrier.radii[1]))
        return (_lift_angle(profile[0],seed[0]),v),'cone_source_generator'
    return _cone_uv(carrier,xyz,seed),'cone_source_inverse'


def _curve_derivative_bounds(curve,a,b):
    """Uniform original-t speed and acceleration, no arc parameter remapping."""
    if curve.dimension!=3:raise ValueError('spatial curve derivative bounds required')
    if curve.kind==1:return _norm(curve.frame[3:6]),0.
    if curve.kind in (2,3):
        bound=math.hypot(curve.radii[0]*_norm(curve.frame[3:6]),curve.radii[1]*_norm(curve.frame[6:9]))
        return bound,bound
    lo,hi=min(a,b),max(a,b)
    if hi<=lo:raise ValueError('positive original curve interval required')
    if curve.kind==4:
        controls=_restrict_controls(_control_rows(curve.controls,3),lo,hi)
        degree=curve.degree
    else:
        span=_curve_interval_span(curve,lo,hi);sa,sb=span.parameter_range
        controls=_restrict_controls(_control_rows(span.controls,3),(lo-sa)/(sb-sa),(hi-sa)/(sb-sa))
        degree=span.degree
    w=min(h[3] for h in controls);an=max(_norm(h[:3]) for h in controls)
    first=[_scale(_sub(y,x),degree/(hi-lo)) for x,y in zip(controls,controls[1:])]
    second=[_scale(_sub(y,x),(degree-1)/(hi-lo)) for x,y in zip(first,first[1:])]
    a1=max((_norm(h[:3]) for h in first),default=0.);w1=max((abs(h[3]) for h in first),default=0.)
    a2=max((_norm(h[:3]) for h in second),default=0.);w2=max((abs(h[3]) for h in second),default=0.)
    return a1/w+an*w1/w**2,a2/w+(2*a1*w1+an*w2)/w**2+2*an*w1*w1/w**3


def _angular_trim_bound(curve,carrier,a,b,seed_a,seed_b,xyz_a,xyz_b,metric):
    speed,acceleration=_curve_derivative_bounds(curve,a,b);dt=abs(b-a)
    if not math.isfinite(speed+acceleration):return math.inf
    midpoint=evaluate_curve(curve,(a+b)/2);q,cov=_frame_coordinates(carrier,midpoint)
    xx,yy,xy=_dot(cov[0],cov[0]),_dot(cov[1],cov[1]),_dot(cov[0],cov[1])
    op=math.sqrt((xx+yy+math.hypot(xx-yy,2*xy))/2)
    b1,b2=op*speed,op*acceleration
    if curve.kind==1:
        direction=curve.frame[3:6]
        b1=math.hypot(_dot(cov[0],direction),_dot(cov[1],direction));b2=0.
    rho=math.hypot(*q[:2])-b1*dt/2
    if not math.isfinite(rho) or rho<=0:return math.inf
    variation=b1*dt/rho
    if variation>=math.pi:return math.inf
    inverse=_cylinder_uv if carrier.kind==2 else _cone_uv
    ca,cb=evaluate_curve(curve,a),evaluate_curve(curve,b)
    ua,ub=inverse(carrier,ca,seed_a),inverse(carrier,cb,seed_b)
    grade=64*math.ulp(max(1.,abs(ua[0]),abs(ub[0])))
    if abs(ub[0]-ua[0])>variation+grade:return math.inf
    va,vb=inverse(carrier,xyz_a,seed_a),inverse(carrier,xyz_b,seed_b)
    endpoint=max(sum(metric[i]*abs(x[i]-y[i]) for i in range(2)) for x,y in ((ua,va),(ub,vb)))
    u2=b2/rho+2*b1*b1/rho**2
    v2=_norm(cov[2])*acceleration/(abs(math.cos(carrier.radii[1])) if carrier.kind==3 else 1.)
    return (metric[0]*u2+metric[1]*v2)*dt*dt/8+endpoint


def _cylinder_trim_bound(curve,carrier,a,b,seed_a,seed_b,xyz_a,xyz_b,metric):
    return _angular_trim_bound(curve,carrier,a,b,seed_a,seed_b,xyz_a,xyz_b,metric)


def _owned_trim_bound(source,edge,carrier,a,b,seeds,positions,metric,quality,profiles):
    curve=source.curve(edge.curve_id);dt=abs(b-a)
    if carrier.kind==2:return _angular_trim_bound(curve,carrier,a,b,*seeds,*positions,metric)
    if carrier.kind==3 and _cone_generator(source,edge,carrier,quality,profiles) is None:
        return _angular_trim_bound(curve,carrier,a,b,*seeds,*positions,metric)
    original=(evaluate_curve(curve,a),evaluate_curve(curve,b));underlying=[];actual=[]
    if carrier.kind==1:
        underlying=[_plane_uv(carrier,p) for p in original]
        _,cov=_frame_coordinates(carrier,original[0])
        acceleration=_curve_derivative_bounds(curve,a,b)[1]
        chord=(metric[0]*_norm(cov[0])+metric[1]*_norm(cov[1]))*acceleration*dt*dt/8
    else:
        profile=_cone_generator(source,edge,carrier,quality,profiles);cosine=math.cos(carrier.radii[1])
        for point,seed in zip(original,seeds):
            local,_=_frame_coordinates(carrier,point)
            underlying.append((_lift_angle(profile[0],seed[0]),local[2]/cosine))
        grade=64*math.ulp(max(1.,*(abs(p[0]) for p in underlying)))
        if abs(underlying[0][0]-underlying[1][0])>grade:return math.inf
        chord=0.
    for t,point,seed in zip((a,b),positions,seeds):
        actual.append(_source_uv(source,edge,carrier,t,point,seed,quality,profiles)[0])
    endpoint=max(sum(metric[i]*float(abs(x[i]-y[i])) for i in range(2)) for x,y in zip(underlying,actual))
    return chord+endpoint


def _curve_scalar_range(curve,a,b,covector,origin):
    """Linear-functional range on ORIGINAL active parameter geometry."""
    lo,hi=sorted((a,b))
    def value(point):return _dot(covector,_sub(point,origin))
    if curve.kind==1:
        values=[value(evaluate_curve(curve,t)) for t in (lo,hi)]
    elif curve.kind in (2,3):
        center=value(curve.frame[:3]);x=curve.radii[0]*_dot(covector,curve.frame[3:6]);y=curve.radii[1]*_dot(covector,curve.frame[6:9])
        amplitude=math.hypot(x,y)
        if hi-lo>=2*math.pi:values=[center-amplitude,center+amplitude]
        else:
            values=[value(evaluate_curve(curve,t)) for t in (lo,hi)]
            critical=math.atan2(y,x)
            for target in (critical,critical+math.pi):
                k=math.ceil((lo-target)/(2*math.pi));t=target+k*2*math.pi
                if lo<=t<=hi:values.append(center+(amplitude if target==critical else -amplitude))
    else:
        values=[]
        if curve.kind==4:
            nets=[_restrict_controls(_control_rows(curve.controls,3),lo,hi)]
        else:
            nets=[]
            for span in curve.spans:
                sa,sb=span.parameter_range;x,y=max(lo,sa),min(hi,sb)
                if x<y:nets.append(_restrict_controls(_control_rows(span.controls,3),(x-sa)/(sb-sa),(y-sa)/(sb-sa)))
        for net in nets:values.extend(value(_recover(h)) for h in net)
    if not values or any(not math.isfinite(x) for x in values):raise ValueError('invalid original active curve range')
    return math.nextafter(min(values),-math.inf),math.nextafter(max(values),math.inf)


def _cone_v_envelope(source,carrier,edge_ids):
    cosine=math.cos(carrier.radii[1])
    if not cosine:raise ValueError('cone axial inverse is singular')
    _,cov=_frame_coordinates(carrier,carrier.frame[:3]);values=[];apex_values=[]
    for identifier in edge_ids:
        edge=source.edge(identifier)
        if edge.curve_id:values.extend(_curve_scalar_range(source.curve(edge.curve_id),*edge.parameter_range,cov[2],carrier.frame[:3]))
        for v in edge.vertex_ids:
            point=source.vertex(v).xyz
            values.append(_frame_coordinates(carrier,point)[0][2])
            if _cone_apex(carrier,point):apex_values.append(-carrier.radii[0]/math.sin(carrier.radii[1]))
    result=[value/cosine for value in values]+apex_values
    if not result or any(not math.isfinite(x) for x in result):raise ValueError('invalid source-owned cone V range')
    return math.nextafter(min(result),-math.inf),math.nextafter(max(result),math.inf)



def _spline_linear_columns(carrier,cache):
    key=('spline_columns',carrier.id)
    if key in cache:return cache[key]
    result=[]
    if (carrier.kind==6 and carrier.degrees[1]==1 and carrier.control_shape[1]==2
            and len(carrier.knot_v)==2 and carrier.mult_v==(2,2)
            and carrier.mult_u[0]==carrier.degrees[0]+1 and carrier.mult_u[-1]==carrier.degrees[0]+1):
        rows=_control_rows(carrier.controls,3)
        for index,u in ((0,carrier.knot_u[0]),(carrier.control_shape[0]-1,carrier.knot_u[-1])):
            a,b=rows[2*index:2*index+2]
            if a[3]<=0 or a[3]!=b[3]:continue
            origin=tuple(Fraction(x)/Fraction(a[3]) for x in a[:3])
            end=tuple(Fraction(x)/Fraction(b[3]) for x in b[:3])
            delta=tuple(y-x for x,y in zip(origin,end));norm=sum(x*x for x in delta)
            if norm:result.append((u,*carrier.knot_v,origin,delta,norm))
    cache[key]=tuple(result);return cache[key]


def _column_uv(model,point,inside=True):
    u,v0,v1,origin,delta,norm=model
    difference=tuple(Fraction(x)-y for x,y in zip(point,origin))
    scalar=sum(x*y for x,y in zip(difference,delta))/norm
    if any(x!=scalar*y for x,y in zip(difference,delta)):return None
    v=Fraction(v0)+scalar*(Fraction(v1)-Fraction(v0))
    if inside and not Fraction(v0)<=v<=Fraction(v1):return None
    return u,v


def _spline_boundary_profile(source,coedge,carrier,cache):
    key=('spline_profile',carrier.id,coedge.id)
    if key in cache:return cache[key]
    edge=source.edge(coedge.edge_id);curve=source.curve(edge.curve_id) if edge.curve_id else None
    candidates=[]
    if curve is not None and curve.kind==1:
        for model in _spline_linear_columns(carrier,cache):
            if _column_uv(model,curve.frame[:3],False) is None:continue
            direction=tuple(Fraction(x) for x in curve.frame[3:6]);delta,norm=model[4:]
            if not any(direction):continue
            scalar=sum(x*y for x,y in zip(direction,delta))/norm
            if any(x!=scalar*y for x,y in zip(direction,delta)):continue
            if any(_column_uv(model,source.vertex(i).xyz) is None for i in edge.vertex_ids):continue
            endpoints=[tuple(Fraction(o)+Fraction(t)*d for o,d in zip(curve.frame[:3],direction)) for t in edge.parameter_range]
            if any(_column_uv(model,point) is None for point in endpoints):continue
            candidates.append(model)
    if candidates:
        if coedge.periodic_shift!=(0,0):raise ValueError(f'carrier {carrier.id} coedge {coedge.id}: unsupported periodic spline boundary branch')
        pc=source.curve(coedge.curve_id);shift=coedge.periodic_shift[0]*2*math.pi
        if pc.kind==1:guide=[Fraction(pc.frame[0])+Fraction(t)*Fraction(pc.frame[3])+Fraction(shift) for t in coedge.parameter_range]
        elif pc.kind in (4,5):
            guide=[Fraction(h[0])/Fraction(h[2])+Fraction(shift) for h in _control_rows(pc.controls,2)]
        else:raise ValueError(f'carrier {carrier.id} coedge {coedge.id}: unproved spline boundary branch guide')
        half=(Fraction(carrier.knot_u[-1])-Fraction(carrier.knot_u[0]))/2
        candidates=[model for model in candidates if all(abs(Fraction(u)-Fraction(model[0]))<half for u in guide)]
        if len(candidates)!=1:raise ValueError(f'carrier {carrier.id} coedge {coedge.id}: ambiguous spline boundary branch')
        cache[key]=candidates[0]
    else:cache[key]=None
    return cache[key]


def _spline_boundary_uv(source,coedge,carrier,t,point,seed,cache):
    if carrier.kind!=6:return None
    profile=_spline_boundary_profile(source,coedge,carrier,cache)
    if profile is not None:
        uv=_column_uv(profile,point)
        if uv is None:raise ValueError(f'carrier {carrier.id} coedge {coedge.id}: source spline boundary point leaves column')
        return uv,'spline_boundary_generator'
    edge=source.edge(coedge.edge_id)
    if edge.curve_id and source.curve(edge.curve_id).kind==1:return None
    if t==edge.parameter_range[0]:vertex=edge.vertex_ids[0]
    elif t==edge.parameter_range[1]:vertex=edge.vertex_ids[1]
    else:return None
    loop=source.loop(coedge.loop_id);index=loop.coedge_ids.index(coedge.id);candidates=[]
    for side in (-1,1):
        neighbor=source.coedge(loop.coedge_ids[(index+side)%len(loop.coedge_ids)])
        neighbor_edge=source.edge(neighbor.edge_id)
        if side<0:
            joint=(neighbor_edge.vertex_ids[1 if neighbor.sense>0 else 0],edge.vertex_ids[0 if coedge.sense>0 else 1])
        else:
            joint=(edge.vertex_ids[1 if coedge.sense>0 else 0],neighbor_edge.vertex_ids[0 if neighbor.sense>0 else 1])
        if joint!=(vertex,vertex):continue
        model=_spline_boundary_profile(source,neighbor,carrier,cache)
        if model is None:continue
        uv=_column_uv(model,source.vertex(vertex).xyz)
        if uv is not None:candidates.append(uv)
    if not candidates:return None
    half=(Fraction(carrier.knot_u[-1])-Fraction(carrier.knot_u[0]))/2
    candidates=set(uv for uv in candidates if abs(Fraction(seed[0])-Fraction(uv[0]))<half)
    if len(candidates)!=1:raise ValueError(f'carrier {carrier.id} coedge {coedge.id}: ambiguous spline boundary vertex branch')
    return candidates.pop(),'spline_boundary_vertex_inverse'


def _recover_global_uv(carrier,uv):
    """Bounded recovery of foreign UV only at original spline global ends."""
    if carrier.kind!=6:return tuple(uv)
    result=list(uv)
    for axis,knots in enumerate((carrier.knot_u,carrier.knot_v)):
        if not knots:raise ValueError('source spline has no original domain knots')
        lo,hi=knots[0],knots[-1]
        grade=128*math.ulp(max(1.0,abs(lo),abs(hi)))
        if result[axis]<lo:
            if lo-result[axis]>grade:raise ValueError('foreign UV outside global recovery grade')
            result[axis]=lo
        elif result[axis]>hi:
            if result[axis]-hi>grade:raise ValueError('foreign UV outside global recovery grade')
            result[axis]=hi
    return tuple(result)


def _positive_midpoint(a,b,opposite):
    """Return a shared-edge point giving positive children in every parent."""
    def valid(p):return all(_orient(x,p,z)>0 and _orient(p,y,z)>0 for x,y,z in opposite)
    point=tuple((a[i]+b[i])/2 for i in range(2))
    if not any(isinstance(x,Fraction) for p in (a,b) for x in p) and valid(point):return point
    point=tuple((Fraction(a[i])+Fraction(b[i]))/2 for i in range(2))
    if not valid(point):raise ValueError('exact shared-edge midpoint has nonpositive child')
    return point


def _guard_carrier_refinement(length,scale,max_refinement,created):
    if not math.isfinite(scale) or scale<=0 or not math.isfinite(length):
        raise ValueError('invalid carrier refinement metric')
    floor=math.ldexp(scale,-max_refinement)
    if length<=floor:
        raise ValueError(f'carrier metric resolution exhausted: length={length!r} floor={floor!r} scale={scale!r}')
    if created>=1<<max_refinement:
        raise ValueError(f'carrier interior construction work cap exhausted: created={created} cap={1<<max_refinement}')


def _outward(value,direction):
    rounded=float(value)
    if (direction<0 and Fraction(rounded)>value) or (direction>0 and Fraction(rounded)<value):
        return math.nextafter(rounded,-math.inf if direction<0 else math.inf)
    return rounded


def _uv_rounding_bound(uv,metric):
    return math.fsum(metric[i]*float(abs(Fraction(uv[i])-Fraction(float(uv[i])))) for i in range(2))


def _point_segment(p,a,b):
    delta=_sub(b,a);length=_dot(delta,delta)
    t=max(0.0,min(1.0,_dot(_sub(p,a),delta)/length)) if length else 0
    return _distance(p,_add(a,_scale(delta,t)))


def _curve_chord_bound(curve,a,b):
    if curve.kind==1: return 0.0
    if curve.kind in (2,3): return max(curve.radii)*(b-a)**2/8
    if a==b: return 0.0
    if curve.kind==4:
        controls=_restrict_controls(_control_rows(curve.controls,curve.dimension),min(a,b),max(a,b))
    else:
        span=_curve_interval_span(curve,a,b);lo,hi=span.parameter_range
        if min(a,b)<lo-1e-12 or max(a,b)>hi+1e-12:
            return math.inf
        controls=_restrict_controls(_control_rows(span.controls,curve.dimension),
                                    max(0,(min(a,b)-lo)/(hi-lo)),min(1,(max(a,b)-lo)/(hi-lo)))
    points=tuple(_recover(c) for c in controls)
    return max(_point_segment(p,points[0],points[-1]) for p in points)


def _surface_metric(carrier,v_bounds=None):
    """Conservative global first-partial norm bounds for UV trim chords."""
    if carrier.kind==6:
        result=[0.0,0.0]
        for span in carrier.spans:
            derivatives=_span_derivative_bounds(span)
            result[0]=max(result[0],derivatives[0]);result[1]=max(result[1],derivatives[1])
        return tuple(result)
    x,y,z=(carrier.frame[i:i+3] for i in (3,6,9));ex=_norm(x)+_norm(y);nz=_norm(z)
    r,a=carrier.radii;u0,u1,v0,v1=carrier.uv_bounds
    if v_bounds is not None:v0,v1=v_bounds
    if carrier.kind==1:return _norm(x),_norm(y)
    if carrier.kind==2:return r*ex,nz
    if carrier.kind==3:return max(abs(r+v0*math.sin(a)),abs(r+v1*math.sin(a)))*ex,abs(math.sin(a))*ex+abs(math.cos(a))*nz
    if carrier.kind==4:return r*ex,r*(ex+nz)
    return (r+a)*ex,a*(ex+nz)


def _span_derivative_bounds(span):
    p,q=span.degrees;a,b,c,d=span.bounds;rows=_control_rows(span.controls,3)
    # Homogeneous quotient bounds. The source OM10 splines have unit weights,
    # but rational inputs retain the complete positive-weight calculation.
    wmin=min(h[3] for h in rows);wmax=max(abs(h[3]) for h in rows)
    an=max(_norm(h[:3]) for h in rows)
    def derivative(i,j):
        if i>p or j>q:return 0.0,0.0
        net=[list(rows[k*(q+1):(k+1)*(q+1)]) for k in range(p+1)]
        pu,pv=p,q
        for _ in range(i):net=[[_scale(_sub(net[k+1][l],net[k][l]),pu/(b-a)) for l in range(pv+1)] for k in range(pu)];pu-=1
        for _ in range(j):net=[[_scale(_sub(row[l+1],row[l]),pv/(d-c)) for l in range(pv)] for row in net];pv-=1
        vals=[h for row in net for h in row]
        return max((_norm(h[:3]) for h in vals),default=0),max((abs(h[3]) for h in vals),default=0)
    au,wu=derivative(1,0);av,wv=derivative(0,1)
    auu,wuu=derivative(2,0);auv,wuv=derivative(1,1);avv,wvv=derivative(0,2)
    su=au/wmin+an*wu/wmin**2;sv=av/wmin+an*wv/wmin**2
    suu=auu/wmin+(2*au*wu+an*wuu)/wmin**2+2*an*wu*wu/wmin**3
    suv=auv/wmin+(au*wv+av*wu+an*wuv)/wmin**2+2*an*wu*wv/wmin**3
    svv=avv/wmin+(2*av*wv+an*wvv)/wmin**2+2*an*wv*wv/wmin**3
    return su,sv,suu,suv,svv


def _surface_error(carrier,uvs,cache):
    u0=min(p[0] for p in uvs);u1=max(p[0] for p in uvs)
    v0=min(p[1] for p in uvs);v1=max(p[1] for p in uvs)
    u0,u1,v0,v1=_outward(u0,-1),_outward(u1,1),_outward(v0,-1),_outward(v1,1)
    du,dv=u1-u0,v1-v0
    if carrier.kind==1:return 0.0
    if carrier.kind==6:
        spans=[(i,s) for i,s in enumerate(carrier.spans) if s.bounds[0]<u1 and s.bounds[1]>u0 and s.bounds[2]<v1 and s.bounds[3]>v0]
        if not spans:spans=[(0,_surface_span(carrier,(u0+u1)/2,(v0+v1)/2))]
        ds=[]
        for i,s in spans:
            key=(carrier.id,i)
            if key not in cache:cache[key]=_span_derivative_bounds(s)
            ds.append(cache[key])
        if len(spans)>1:
            p,q=carrier.degrees
            c1=all(p-m>=1 for k,m in zip(carrier.knot_u,carrier.mult_u) if u0<k<u1)
            c1=c1 and all(q-m>=1 for k,m in zip(carrier.knot_v,carrier.mult_v) if v0<k<v1)
            if not c1:
                # A genuinely C0 crossed knot needs the conservative convergent
                # first-derivative bound. C1 piecewise-C2 maps instead retain
                # the Taylor integral remainder with a uniform span maximum.
                return max(d[0] for d in ds)*du+max(d[1] for d in ds)*dv
        muu,muv,mvv=(max(d[i] for d in ds) for i in (2,3,4))
    else:
        x,y,z=(carrier.frame[i:i+3] for i in (3,6,9));ex=_norm(x)+_norm(y);nz=_norm(z);r,a=carrier.radii
        if carrier.kind==2:muu,muv,mvv=r*ex,0,0
        elif carrier.kind==3:muu,muv,mvv=max(abs(r+v0*math.sin(a)),abs(r+v1*math.sin(a)))*ex,abs(math.sin(a))*ex,0
        elif carrier.kind==4:
            def trig_max_cos(lo,hi):
                if math.ceil(lo/math.pi)<=math.floor(hi/math.pi):return 1.0
                return min(1.0,max(abs(math.cos(lo)),abs(math.cos(hi)))+1e-15)
            muu=r*ex*trig_max_cos(v0,v1)
            muv=r*ex*trig_max_cos(v0-math.pi/2,v1-math.pi/2)
            mvv=r*(ex+nz)
        else:muu,muv,mvv=(r+a)*ex,a*ex,a*(ex+nz)
    # Barycentric Taylor terms cancel at the interpolated UV. Weighted
    # coordinate variances are at most range²/4 and the mixed absolute
    # covariance is bounded by Cauchy, giving the sharp factor1/8.
    return 0.125*(muu*du*du+2*muv*du*dv+mvv*dv*dv)


def _triangulate(loops,points):
    """Multiple-hole visibility bridges followed by strict occurrence ears."""
    outer=list(loops[0]);holes=[list(h) for h in loops[1:]]
    original=[[points[i] for i in loop] for loop in loops]
    while holes:
        hole=max(holes,key=lambda h:max((points[i][0],-points[i][1]) for i in h))
        hi=max(range(len(hole)),key=lambda i:(points[hole[i]][0],-points[hole[i]][1]))
        h=hole[hi];hp=points[h];chosen=None
        boundaries=[outer]+holes
        candidates=sorted((_distance(hp,points[o]),oi,o) for oi,o in enumerate(outer) if points[o]!=hp)
        for _,oi,o in candidates:
            op=points[o]
            if hp==op:continue
            visible=True
            for boundary in boundaries:
                for a,b in zip(boundary,boundary[1:]+boundary[:1]):
                    pa,pb=points[a],points[b]
                    if pa in (hp,op) or pb in (hp,op):
                        other=pb if pa in (hp,op) else pa
                        if other not in (hp,op) and _on_segment(hp,op,other):visible=False;break
                        continue
                    if _segments_touch(hp,op,pa,pb):visible=False;break
                if not visible:break
            if visible:
                midpoint=((hp[0]+op[0])/2,(hp[1]+op[1])/2)
                if _winding(original[0],midpoint)!=1 or any(_winding(p,midpoint)!=0 for p in original[1:]):continue
                chosen=oi;break
        if chosen is None:raise ValueError('no visible bridge for source face hole')
        oi=chosen;cycle=hole[hi:]+hole[:hi]
        outer=outer[:oi+1]+cycle+[h,outer[oi]]+outer[oi+1:];holes.remove(hole)
    # Doubly linked occurrence list: duplicate bridge coordinates are separate
    # occurrences. Endpoints equal to an ear corner do not falsely block it.
    n=len(outer);prev=[(i-1)%n for i in range(n)];nxt=[(i+1)%n for i in range(n)]
    alive=[True]*n;remaining=n;triangles=[];cursor=0;stalled=0
    while remaining>3:
        if not alive[cursor]:cursor=nxt[cursor];continue
        ai,bi,ci=prev[cursor],cursor,nxt[cursor]
        a,b,c=(points[outer[i]] for i in (ai,bi,ci))
        ear=_orient(a,b,c)>0
        if ear:
            for j in range(n):
                if not alive[j] or j in (ai,bi,ci):continue
                p=points[outer[j]]
                if p in (a,b,c):continue
                if p[0]<min(a[0],b[0],c[0]) or p[0]>max(a[0],b[0],c[0]) or p[1]<min(a[1],b[1],c[1]) or p[1]>max(a[1],b[1],c[1]):continue
                if _orient(a,b,p)>=0 and _orient(b,c,p)>=0 and _orient(c,a,p)>=0:ear=False;break
        if ear:
            triangles.append((outer[ai],outer[bi],outer[ci]));alive[bi]=False;nxt[ai]=ci;prev[ci]=ai
            remaining-=1;cursor=ci;stalled=0
        else:
            cursor=nxt[cursor];stalled+=1
            if stalled>remaining:raise ValueError('source trim polygon has no positive ear')
    end=[i for i in range(n) if alive[i]]
    a=end[0];b=nxt[a];c=nxt[b]
    if _orient(points[outer[a]],points[outer[b]],points[outer[c]])<=0:
        raise ValueError('degenerate final source trim ear')
    triangles.append((outer[a],outer[b],outer[c]))
    return triangles



def _improve_connectivity(triangles,points,boundary_edges,metric):
    """Owned strict Lawson flips; fixed vertices and source constraints survive."""
    from collections import deque
    if len(metric)!=2 or any(not math.isfinite(x) or x<=0 for x in metric):
        raise ValueError('connectivity requires finite positive metric')
    exact={i:tuple(Fraction(x) for x in p) for i,p in points.items()}
    denominators=tuple(math.lcm(*(p[k].denominator for p in exact.values())) for k in range(2))
    exact={i:tuple(p[k].numerator*(denominators[k]//p[k].denominator) for k in range(2)) for i,p in exact.items()}
    rational_weights=tuple(Fraction(metric[k])**2/denominators[k]**2 for k in range(2))
    common=math.lcm(*(w.denominator for w in rational_weights))
    weights=tuple(w.numerator*(common//w.denominator) for w in rational_weights)
    def orientation(a,b,c):
        ax,ay=exact[a];bx,by=exact[b];cx,cy=exact[c]
        return (bx-ax)*(cy-ay)-(by-ay)*(cx-ax)
    active=list(triangles);incidence={}
    def edges(tri):return tuple(tuple(sorted((a,b))) for a,b in zip(tri,tri[1:]+tri[:1]))
    def insert(tid,tri):
        if len(set(tri))!=3 or orientation(*tri)<=0:
            raise ValueError('connectivity requires positive triangles')
        for edge in edges(tri):
            owners=incidence.setdefault(edge,set());owners.add(tid)
            if len(owners)>2:raise ValueError('nonmanifold connectivity edge')
    for tid,tri in enumerate(active):insert(tid,tri)
    pending=set(incidence)-set(boundary_edges);queue=deque(sorted(pending))
    flips=0;limit=8*len(points)**2
    while queue:
        edge=queue.popleft();pending.remove(edge)
        owners=incidence.get(edge,())
        if edge in boundary_edges or len(owners)!=2:continue
        left,right=sorted(owners);old=active[left];other=active[right]
        for j in range(3):
            a,b,c=old[j],old[(j+1)%3],old[(j+2)%3]
            if tuple(sorted((a,b)))==edge:break
        d=next(i for i in other if i not in edge)
        if len({a,b,c,d})!=4:continue
        if orientation(b,a,d)<=0:continue
        replacement=((c,d,b),(d,c,a));diagonal=tuple(sorted((c,d)))
        if diagonal in incidence:continue
        if any(orientation(*tri)<=0 for tri in replacement):continue
        # Exact integer determinant = saved-UV determinant * positive W*Du*Dv.
        # Common denominators change arithmetic only, preserving exact zero/sign.
        delta=[tuple(x-y for x,y in zip(exact[i],exact[d])) for i in (a,b,c)]
        lift=[weights[0]*x*x+weights[1]*y*y for x,y in delta]
        (ax,ay),(bx,by),(cx,cy)=delta
        determinant=(lift[0]*(bx*cy-by*cx)+lift[1]*(cx*ay-cy*ax)+lift[2]*(ax*by-ay*bx))
        if determinant<=0:continue
        if flips>=limit:raise ValueError('connectivity flip work cap exhausted')
        flips+=1
        affected=set(edges(old))|set(edges(other))
        for tid,tri in ((left,old),(right,other)):
            for e in edges(tri):
                incidence[e].remove(tid)
                if not incidence[e]:del incidence[e]
        for tid,tri in zip((left,right),replacement):active[tid]=tri;insert(tid,tri)
        affected.update(edges(replacement[0]));affected.update(edges(replacement[1]))
        for e in sorted(affected):
            if e not in boundary_edges and e in incidence and e not in pending:
                pending.add(e);queue.append(e)
    return active


def _mapped_parameter(source,edge,coedge,t,carrier,quality):
    if coedge.parameter_map_kind==1:return t
    pairs=coedge.parameter_map
    direction=1 if edge.parameter_range[1]>=edge.parameter_range[0] else -1
    left,right=pairs[0],pairs[-1]
    for a,b in zip(pairs,pairs[1:]):
        if direction*(t-a[0])>=0 and direction*(b[0]-t)>=0:left,right=a,b;break
    f=(t-left[0])/(right[0]-left[0]);seed=left[1]+f*(right[1]-left[1])
    if coedge.parameter_map_kind==2:return seed
    raise ValueError(f'edge {edge.id}: unproved non-SameParameter continuation map is unsupported')


def _curve_knots_in_edge(source,edge,incidents):
    start,end=edge.parameter_range;lo,hi=sorted((start,end));result={start,end}
    if edge.curve_id:
        c=source.curve(edge.curve_id)
        result.update(t for t in c.knots if lo<t<hi)
    for coedge,carrier in incidents:
        # Owned cylinder UV depends on the ORIGINAL spatial curve. Foreign
        # branch-guide knot artifacts do not constrain that derived trim.
        if carrier.kind in (1,2,3):continue
        pc=source.curve(coedge.curve_id)
        if coedge.parameter_map_kind==1:result.update(t for t in pc.knots if lo<t<hi)
        elif coedge.parameter_map_kind==2:
            (ta,sa),(tb,sb)=coedge.parameter_map[0],coedge.parameter_map[-1]
            if sb!=sa:
                result.update(ta+(tb-ta)*(s-sa)/(sb-sa) for s in pc.knots if lo<ta+(tb-ta)*(s-sa)/(sb-sa)<hi)
        else:result.update(t for t,_ in coedge.parameter_map if lo<t<hi)
    return sorted(result,reverse=start>end)


def mesh_imported(source,quality=None,*,progress=None):
    """Mesh every supplied solid definition without a foreign mesh fallback."""
    if type(source) is not ImportedSource:raise TypeError('expected ImportedSource')
    if source.units_mm!=1.0:raise ValueError('imported meshing requires explicit millimetre source parameters')
    if quality is None:quality=MeshQuality()
    if type(quality) is not MeshQuality:raise TypeError('expected MeshQuality')
    if progress is not None and not callable(progress):raise TypeError('progress must be callable')
    begin=time.perf_counter();validate_source(source)
    vertices=[];triangles=[];face_meshes=[];edge_samples=[]
    vertex_by_key={};endpoint_keys={};edge_records={};incidents={};edge_owners={}
    carrier_slots={c.id:i for i,c in enumerate(source.carriers)}
    metric_cache={};derivative_cache={};profiles={};carrier_edges={};next_key=1
    errors={'max_edge_chord_bound_mm':0.0,'max_source_endpoint_residual_mm':0.0,
            'max_foreign_pcurve_to_edge_residual_mm':0.0,'max_source_joint_gap_mm':0.0,
            'max_surface_interpolation_bound_mm':0.0,'max_native_uv_rounding_bound_mm':0.0,
            'foreign_uv_recovery_count':0,'max_foreign_uv_recovery_shift_mm':0.0,
            'cylinder_inverse_sample_count':0,'joint_recovery_count':0,
            'max_cylinder_inverse_correction_mm':0.0,'max_corrected_surface_to_edge_residual_mm':0.0,
            'plane_inverse_sample_count':0,'cone_inverse_sample_count':0,'cone_generator_sample_count':0,
            'max_plane_inverse_correction_mm':0.0,'max_cone_inverse_correction_mm':0.0,
            'max_cone_generator_line_residual_mm':0.0,'spline_boundary_sample_count':0,
            'foreign_uv_unevaluated_count':0,'max_spline_boundary_u_shift':0.0,'max_spline_boundary_v_shift':0.0}
    def make_vertex(definition,xyz,kind,identifier,t=0.0,uv=(0.0,0.0)):
        nonlocal next_key
        if any(not math.isfinite(x) for x in xyz):raise ValueError('nonfinite imported mesh vertex')
        row=MeshVertex(next_key,definition,tuple(xyz),kind,identifier,t,tuple(uv))
        vertices.append(row);vertex_by_key[next_key]=row;next_key+=1
        return row.physical_key
    for definition in source.definitions:
        for vi in definition.vertex_ids:
            endpoint_keys[(definition.id,vi)]=make_vertex(definition.id,source.vertex(vi).xyz,1,vi)
        for ei in definition.edge_ids:
            if ei in edge_owners and edge_owners[ei]!=definition.id:
                raise ValueError('shared topological edge across distinct solid definitions requires separate definition sampling')
            edge_owners[ei]=definition.id
    for face in source.faces:
        carrier=source.carrier(face.carrier_id)
        for li in face.loop_ids:
            for ci in source.loop(li).coedge_ids:
                coedge=source.coedge(ci);incidents.setdefault(coedge.edge_id,[]).append((coedge,carrier))
                carrier_edges.setdefault(carrier.id,set()).add(coedge.edge_id)
    for edge_index,edge in enumerate(source.edges):
        owner=edge_owners[edge.id];uses=incidents.get(edge.id,())
        curve=source.curve(edge.curve_id) if edge.curve_id else None
        if not uses:raise ValueError(f'edge {edge.id}: no source coedge occurrence')
        for _,carrier in uses:
            if carrier.id not in metric_cache:
                vb=_cone_v_envelope(source,carrier,carrier_edges[carrier.id]) if carrier.kind==3 else None
                metric_cache[carrier.id]=_surface_metric(carrier,vb)
        start,end=edge.parameter_range
        if start==end and not edge.degenerate:raise ValueError(f'edge {edge.id}: zero parameter extent')
        values=_curve_knots_in_edge(source,edge,uses)
        def position(t):
            if t==start:return source.vertex(edge.vertex_ids[0]).xyz
            if t==end:return source.vertex(edge.vertex_ids[1]).xyz
            return evaluate_curve(curve,t)
        def bound(a,b):
            result=0.0 if edge.degenerate else _curve_chord_bound(curve,a,b)
            for coedge,carrier in uses:
                sa=_mapped_parameter(source,edge,coedge,a,carrier,quality)
                sb=_mapped_parameter(source,edge,coedge,b,carrier,quality)
                mu,mv=metric_cache[carrier.id]
                if carrier.kind in (1,2,3):
                    if curve is None:raise ValueError('analytic inverse requires original spatial curve')
                    pc=source.curve(coedge.curve_id)
                    seeds=[evaluate_curve(pc,t) for t in (sa,sb)]
                    seeds=[(uv[0]+coedge.periodic_shift[0]*2*math.pi,uv[1]) for uv in seeds]
                    result=max(result,_owned_trim_bound(source,edge,carrier,a,b,seeds,(position(a),position(b)),(mu,mv),quality,profiles))
                else:
                    pc=source.curve(coedge.curve_id)
                    seeds=[evaluate_curve(pc,t) for t in (sa,sb)]
                    seeds=[(uv[0]+coedge.periodic_shift[0]*2*math.pi,uv[1]) for uv in seeds]
                    owned=[_spline_boundary_uv(source,coedge,carrier,t,position(t),seed,profiles) for t,seed in zip((a,b),seeds)]
                    if all(o is not None and o[1]=='spline_boundary_generator' for o in owned):
                        underlying=[_spline_boundary_uv(source,coedge,carrier,t,evaluate_curve(curve,t),seed,profiles)[0] for t,seed in zip((a,b),seeds)]
                        result=max(result,max(sum(m*abs(x-y) for m,x,y in zip((mu,mv),o[0],u)) for o,u in zip(owned,underlying)))
                    else:
                        uv_error=_curve_chord_bound(pc,sa,sb)*math.hypot(mu,mv)
                        perturb=max((sum(m*abs(x-y) for m,x,y in zip((mu,mv),o[0],seed)) if o is not None else 0.) for o,seed in zip(owned,seeds))
                        result=max(result,uv_error+perturb)
            return result
        samples=[values[0]]
        def refine(a,b,depth):
            accepted_bound=bound(a,b)
            if accepted_bound<=quality.tolerance_mm/8:
                errors['max_edge_chord_bound_mm']=max(errors['max_edge_chord_bound_mm'],accepted_bound)
                samples.append(b);return
            if depth>=quality.max_refinement or len(samples)>=quality.max_edge_samples:
                raise ValueError(f'edge {edge.id}: chord refinement exhausted')
            mid=(a+b)/2
            if mid in (a,b):raise ValueError(f'edge {edge.id}: parameter subdivision exhausted binary64; interval={(a,b)!r} bound={accepted_bound!r} source_curve={(curve.id,curve.kind) if curve else None!r} uses={tuple((co.id,ca.id,ca.kind) for co,ca in uses)!r}')
            refine(a,mid,depth+1);refine(mid,b,depth+1)
        for a,b in zip(values,values[1:]):refine(a,b,0)
        if len(samples)>quality.max_edge_samples:raise ValueError('source edge exceeds sample budget')
        keys=[];positions=[]
        for i,t in enumerate(samples):
            if i==0:key=endpoint_keys[(owner,edge.vertex_ids[0])]
            elif i==len(samples)-1:key=endpoint_keys[(owner,edge.vertex_ids[1])]
            elif edge.degenerate:key=endpoint_keys[(owner,edge.vertex_ids[0])]
            else:key=make_vertex(owner,evaluate_curve(curve,t),2,edge.id,t)
            xyz=vertex_by_key[key].xyz
            if curve:
                residual=_distance(xyz,evaluate_curve(curve,t))
                errors['max_source_endpoint_residual_mm']=max(errors['max_source_endpoint_residual_mm'],residual)
                if residual>quality.source_residual_mm:raise ValueError(f'edge {edge.id}: source endpoint-to-curve residual exceeds quality')
            keys.append(key);positions.append(xyz)
        record=EdgeSamples(edge.id,tuple(samples),tuple(keys),tuple(positions))
        edge_samples.append(record);edge_records[edge.id]=record
        if progress is not None and (edge_index+1)%1000==0:progress('edges',edge_index+1,len(source.edges),edge.id)
    sampled=time.perf_counter()
    for face_index,face in enumerate(source.faces):
        carrier=source.carrier(face.carrier_id);slot=carrier_slots[carrier.id]
        boundaries=[];chart_closures=[];joint_recoveries=[]
        for li in face.loop_ids:
            loop=source.loop(li);occurrences=[];coedge_points=[]
            for ci in loop.coedge_ids:
                coedge=source.coedge(ci);edge=source.edge(coedge.edge_id);record=edge_records[edge.id]
                order=list(range(len(record.parameters)))
                if coedge.sense<0:order.reverse()
                segment=[]
                for index in order:
                    t=record.parameters[index];s=_mapped_parameter(source,edge,coedge,t,carrier,quality)
                    raw_uv=evaluate_curve(source.curve(coedge.curve_id),s)
                    uv=(raw_uv[0]+coedge.periodic_shift[0]*2*math.pi,
                        raw_uv[1]+coedge.periodic_shift[1]*2*math.pi)
                    owned=_spline_boundary_uv(source,coedge,carrier,t,record.positions[index],uv,profiles)
                    try:recovered=_recover_global_uv(carrier,uv)
                    except ValueError:
                        if owned is None:raise
                        recovered=None;errors['foreign_uv_unevaluated_count']+=1
                    if recovered is not None and recovered!=uv:
                        mu,mv=metric_cache[carrier.id]
                        shift=mu*abs(recovered[0]-uv[0])+mv*abs(recovered[1]-uv[1])
                        errors['foreign_uv_recovery_count']+=1
                        errors['max_foreign_uv_recovery_shift_mm']=max(errors['max_foreign_uv_recovery_shift_mm'],shift)
                    raw_point=evaluate_carrier(carrier,*recovered)[0] if recovered is not None else None
                    if raw_point is not None:
                        raw_residual=_distance(raw_point,record.positions[index])
                        errors['max_foreign_pcurve_to_edge_residual_mm']=max(errors['max_foreign_pcurve_to_edge_residual_mm'],raw_residual)
                    recovery_kind='pcurve'
                    if owned is not None:
                        recovered,recovery_kind=owned;errors['spline_boundary_sample_count']+=1
                        for axis,label in enumerate(('u','v')):
                            metric='max_spline_boundary_'+label+'_shift'
                            errors[metric]=max(errors[metric],abs(float(recovered[axis])-uv[axis]))
                    if carrier.kind in (1,2,3):
                        recovered,recovery_kind=_source_uv(source,edge,carrier,t,record.positions[index],recovered,quality,profiles)
                        family={1:'plane',2:'cylinder',3:'cone'}[carrier.kind]
                        errors[family+'_inverse_sample_count']+=1
                        correction=_distance(raw_point,evaluate_carrier(carrier,*recovered)[0])
                        metric_name='max_'+family+'_inverse_correction_mm'
                        errors[metric_name]=max(errors[metric_name],correction)
                        if recovery_kind=='cone_source_generator':
                            errors['cone_generator_sample_count']+=1
                            profile=_cone_generator(source,edge,carrier,quality,profiles)
                            errors['max_cone_generator_line_residual_mm']=max(errors['max_cone_generator_line_residual_mm'],profile[1])
                    uv=recovered
                    residual=_distance(evaluate_carrier(carrier,*uv)[0],record.positions[index])
                    errors['max_corrected_surface_to_edge_residual_mm']=max(errors['max_corrected_surface_to_edge_residual_mm'],residual)
                    if residual>quality.source_residual_mm:
                        raise ValueError(f'face {face.id} coedge {ci}: corrected source recovery residual {residual:.17g} exceeds {quality.source_residual_mm}; t={t!r} s={s!r} rawUV={raw_uv!r} UV={uv!r} xyz={record.positions[index]!r} carrier={(carrier.id,carrier.kind,carrier.radii)!r}')
                    segment.append(BoundaryOccurrence(ci,edge.id,record.vertex_keys[index],t,uv,s,raw_uv,recovery_kind))
                coedge_points.append(segment)
            for j,segment in enumerate(coedge_points):
                following=coedge_points[(j+1)%len(coedge_points)];end=segment[-1];start=following[0]
                if end.sample_key!=start.sample_key:raise ValueError(f'face {face.id}: disconnected original loop vertex')
                if carrier.kind==6 and any(o.recovery_kind.startswith('spline_boundary_') for o in (end,start)):
                    if end.uv!=start.uv:raise ValueError(f'face {face.id} loop {li}: source spline boundary joint changes column branch')
                    vertex=vertex_by_key[end.sample_key]
                    if vertex.source_kind!=1:raise ValueError(f'face {face.id} loop {li}: spline boundary joint lacks original vertex')
                    def foreign_spline(o):
                        use=source.coedge(o.coedge_id)
                        return replace(o,uv=(o.pcurve_uv[0]+use.periodic_shift[0]*2*math.pi,o.pcurve_uv[1]),recovery_kind='pcurve')
                    joint_recoveries.append(JointRecovery(face.id,li,vertex.source_id,foreign_spline(end),foreign_spline(start),end.uv,'spline_boundary_vertex_inverse'))
                    errors['joint_recovery_count']+=1
                if carrier.kind in (1,2,3):
                    def foreign(o):
                        use=source.coedge(o.coedge_id)
                        return replace(o,uv=(o.pcurve_uv[0]+(use.periodic_shift[0]*2*math.pi if carrier.kind!=1 else 0.),o.pcurve_uv[1]),recovery_kind='pcurve')
                    incoming,outgoing=foreign(end),foreign(start)
                    uv_scale=max(1.,*(abs(x) for x in carrier.uv_bounds))
                    vertex=vertex_by_key[end.sample_key]
                    if vertex.source_kind!=1:raise ValueError('analytic joint lacks original vertex')
                    is_apex=carrier.kind==3 and _cone_apex(carrier,vertex.xyz)
                    if not is_apex and _distance(incoming.uv,outgoing.uv)>2**-32*uv_scale:
                        if carrier.kind!=1 and abs(incoming.uv[0]-outgoing.uv[0])>=math.pi:
                            raise ValueError('analytic joint changes chart branch')
                        canonical,_=_source_uv(source,source.edge(outgoing.edge_id),carrier,outgoing.source_parameter,vertex.xyz,outgoing.uv,quality,profiles)
                        if _distance(evaluate_carrier(carrier,*canonical)[0],vertex.xyz)>quality.source_residual_mm:
                            raise ValueError('analytic canonical joint exceeds source residual')
                        segment[-1]=replace(end,uv=canonical);following[0]=replace(start,uv=canonical)
                        family={1:'plane',2:'cylinder',3:'cone'}[carrier.kind]
                        joint_recoveries.append(JointRecovery(face.id,li,vertex.source_id,incoming,outgoing,canonical,family+'_vertex_inverse'))
                        errors['joint_recovery_count']+=1
                        end,start=segment[-1],following[0]
                if end.uv!=start.uv:
                    gap=_distance(evaluate_carrier(carrier,*end.uv)[0],evaluate_carrier(carrier,*start.uv)[0])
                    errors['max_source_joint_gap_mm']=max(errors['max_source_joint_gap_mm'],gap)
                    vertex=vertex_by_key[end.sample_key]
                    if vertex.source_kind!=1:raise ValueError('source loop joint lacks original vertex identity')
                    singular=None
                    if carrier.kind==4:
                        pole=math.copysign(math.pi/2,end.uv[1])
                        if (abs(end.uv[1]-pole)<=2**-44 and abs(start.uv[1]-pole)<=2**-44
                                and abs(end.uv[0]-start.uv[0])<=2*math.pi+32*math.ulp(2*math.pi)):
                            expected=_add(carrier.frame[:3],_scale(carrier.frame[9:12],math.copysign(carrier.radii[0],pole)))
                            if _distance(expected,vertex.xyz)<=2**-32:singular='sphere_pole'
                    elif carrier.kind==3 and math.sin(carrier.radii[1])!=0:
                        apex=-carrier.radii[0]/math.sin(carrier.radii[1])
                        if abs(end.uv[1]-apex)<=2**-40 and abs(start.uv[1]-apex)<=2**-40 and abs(end.uv[0]-start.uv[0])<=2*math.pi+32*math.ulp(2*math.pi):
                            expected=_add(carrier.frame[:3],_scale(carrier.frame[9:12],apex*math.cos(carrier.radii[1])))
                            if _distance(expected,vertex.xyz)<=2**-32:singular='cone_apex'
                    if singular:
                        chart_closures.append(ChartClosure(face.id,li,vertex.source_id,end.coedge_id,start.coedge_id,end.uv,start.uv,singular))
                    else:
                        uv_scale=max(1.,*(abs(x) for x in carrier.uv_bounds))
                        if _distance(end.uv,start.uv)>2**-32*uv_scale:
                            raise ValueError(f'face {face.id}: nonclosing source UV joint {end.uv!r} -> {start.uv!r}')
                        # The shared original vertex fixes this chart joint's
                        # identity. Its tiny foreign pcurve endpoint discrepancy
                        # is retained by each point's raw pcurve_uv; the chosen
                        # polygon corner is the outgoing original start value.
            closure_incoming={c.incoming_coedge_id for c in chart_closures if c.loop_id==li}
            for segment in coedge_points:
                occurrences.extend(segment[:-1])
                if segment[-1].coedge_id in closure_incoming:occurrences.append(segment[-1])
            if len(occurrences)<3:raise ValueError(f'face {face.id}: fewer than three UV boundary occurrences')
            # Normalize source loop traversal to the positive polygonal domain.
            # This changes traversal only, never physical/source identity.
            signed=_area([o.uv for o in occurrences])
            if signed==0:raise ValueError(f'face {face.id}: zero UV loop area')
            if (signed>0)!=bool(loop.outer):occurrences.reverse()
            boundaries.append((loop.outer,tuple(occurrences)))
        outer=[b for is_outer,b in boundaries if is_outer]
        holes=[b for is_outer,b in boundaries if not is_outer]
        if len(outer)!=1:raise ValueError(f'face {face.id}: expected one outer UV chart boundary')
        boundary_loops=(outer[0],*holes)
        points={};keys={};levels={};node=0;node_loops=[];boundary_edges=set()
        for boundary in boundary_loops:
            ids=[]
            for occurrence in boundary:
                points[node]=occurrence.uv;keys[node]=occurrence.sample_key;levels[node]=0;ids.append(node);node+=1
            for a,b in zip(ids,ids[1:]+ids[:1]):
                if points[a]==points[b]:raise ValueError(f'face {face.id}: duplicate adjacent UV occurrence')
                boundary_edges.add(tuple(sorted((a,b))))
            node_loops.append(ids)
        initial_node_count=node
        mu,mv=metric_cache[carrier.id]
        metric_scale=math.hypot(mu*(max(p[0] for p in points.values())-min(p[0] for p in points.values())),
                                mv*(max(p[1] for p in points.values())-min(p[1] for p in points.values())))
        # Direct domain simplicity is required before ears; no epsilon cleanup.
        segments=[(loop_index,i,a,b) for loop_index,loop in enumerate(node_loops)
                  for i,(a,b) in enumerate(zip(loop,loop[1:]+loop[:1]))]
        for i,(li,ii,a,b) in enumerate(segments):
            for lj,jj,c,d in segments[i+1:]:
                if li==lj and ({a,b}&{c,d}):continue
                if _segments_touch(points[a],points[b],points[c],points[d]):
                    raise ValueError(f'face {face.id}: intersecting sampled UV boundary')
        for hole in node_loops[1:]:
            if _winding([points[i] for i in node_loops[0]],points[hole[0]])!=1:
                raise ValueError(f'face {face.id}: hole outside source outer boundary')
        try:initial=_triangulate(node_loops,points)
        except ValueError as exc:raise ValueError(f'face {face.id}: {exc}') from None
        if any(_surface_error(carrier,[points[i] for i in tri],derivative_cache)>quality.tolerance_mm*.75 for tri in initial):
            try:initial=_improve_connectivity(initial,points,boundary_edges,metric_cache[carrier.id])
            except ValueError as exc:raise ValueError(f'face {face.id}: {exc}') from None
        active={i:(tri,0) for i,tri in enumerate(initial)};next_tri=len(active)
        edge_incidence={}
        def add_incidence(tid,tri):
            for a,b in zip(tri,tri[1:]+tri[:1]):edge_incidence.setdefault(tuple(sorted((a,b))),set()).add(tid)
        def remove_triangle(tid):
            tri,depth=active.pop(tid)
            for a,b in zip(tri,tri[1:]+tri[:1]):
                ek=tuple(sorted((a,b)));edge_incidence[ek].remove(tid)
                if not edge_incidence[ek]:del edge_incidence[ek]
            return tri,depth
        for tid,(tri,_) in active.items():add_incidence(tid,tri)
        queue=list(active);cursor=0
        while cursor<len(queue):
            tid=queue[cursor];cursor+=1
            if tid not in active:continue
            tri,depth=active[tid];uvs=[points[i] for i in tri]
            error=_surface_error(carrier,uvs,derivative_cache)
            if error<=quality.tolerance_mm*0.75:
                errors['max_surface_interpolation_bound_mm']=max(errors['max_surface_interpolation_bound_mm'],error)
                continue
            candidates=[]
            for a,b in zip(tri,tri[1:]+tri[:1]):
                ek=tuple(sorted((a,b)))
                if ek in boundary_edges:continue
                mu,mv=metric_cache[carrier.id]
                score=(mu*(points[a][0]-points[b][0]))**2+(mv*(points[a][1]-points[b][1]))**2
                candidates.append((score,ek))
            if candidates:
                _,ek=max(candidates);a,b=ek
                affected=list(edge_incidence[ek]);opposite=[]
                for old in affected:
                    oldtri,_=active[old]
                    for j in range(3):
                        x,y,z=oldtri[j],oldtri[(j+1)%3],oldtri[(j+2)%3]
                        if {x,y}=={a,b}:break
                    opposite.append((points[x],points[y],points[z]))
                uv=_positive_midpoint(points[a],points[b],opposite)
                new_level=1+max(levels[a],levels[b])
                try:_guard_carrier_refinement(math.sqrt(float(max(candidates)[0])),metric_scale,quality.max_refinement,node-initial_node_count)
                except ValueError as exc:
                    edges_detail=tuple((tuple(points[i] for i in (a,b)),tuple(sorted((a,b))) in boundary_edges,
                                        tuple((vertex_by_key[keys[i]].source_kind,vertex_by_key[keys[i]].source_id,vertex_by_key[keys[i]].source_parameter) for i in (a,b)))
                                       for a,b in zip(tri,tri[1:]+tri[:1]))
                    raise ValueError(f'face {face.id}: {exc}; UV={uvs!r}; error={error!r}; selected={(points[a],points[b])!r}; all_edges={edges_detail!r}; carrier={(carrier.id,carrier.kind,carrier.radii)!r}') from None
                new=node;node+=1;points[new]=uv;levels[new]=new_level;keys[new]=make_vertex(face.definition_id,evaluate_carrier(carrier,*uv)[0],3,face.id,uv=uv)
                for old in affected:
                    oldtri,olddepth=remove_triangle(old)
                    for j in range(3):
                        x,y,z=oldtri[j],oldtri[(j+1)%3],oldtri[(j+2)%3]
                        if {x,y}=={a,b}:break
                    for replacement in ((x,new,z),(new,y,z)):
                        active[next_tri]=(replacement,max(levels[i] for i in replacement));add_incidence(next_tri,replacement);queue.append(next_tri);next_tri+=1
            else:
                uv=(math.fsum(p[0] for p in uvs)/3,math.fsum(p[1] for p in uvs)/3)
                if any(isinstance(x,Fraction) for p in uvs for x in p) or any(_orient(a,b,uv)<=0 for a,b in zip(uvs,uvs[1:]+uvs[:1])):
                    uv=tuple(sum(Fraction(p[k]) for p in uvs)/3 for k in range(2))
                if any(_orient(a,b,uv)<=0 for a,b in zip(uvs,uvs[1:]+uvs[:1])):raise ValueError('exact centroid has nonpositive child')
                new_level=1+max(levels[i] for i in tri)
                diameter=max(math.hypot(mu*float(a[0]-b[0]),mv*float(a[1]-b[1])) for a,b in zip(uvs,uvs[1:]+uvs[:1]))
                try:_guard_carrier_refinement(diameter,metric_scale,quality.max_refinement,node-initial_node_count)
                except ValueError as exc:raise ValueError(f'face {face.id}: {exc}; centroid UV={uvs!r}') from None
                new=node;node+=1;points[new]=uv;levels[new]=new_level;keys[new]=make_vertex(face.definition_id,evaluate_carrier(carrier,*uv)[0],3,face.id,uv=uv)
                oldtri,olddepth=remove_triangle(tid)
                for a,b in zip(oldtri,oldtri[1:]+oldtri[:1]):
                    replacement=(a,b,new);active[next_tri]=(replacement,max(levels[i] for i in replacement))
                    add_incidence(next_tri,replacement);queue.append(next_tri);next_tri+=1
        indices=[]
        singular_keys={endpoint_keys[(face.definition_id,source.edge(source.coedge(c).edge_id).vertex_ids[0])]
                       for li in face.loop_ids for c in source.loop(li).coedge_ids
                       if source.edge(source.coedge(c).edge_id).degenerate}
        singular_keys.update(endpoint_keys[(face.definition_id,c.source_vertex_id)] for c in chart_closures)
        for tri,depth in active.values():
            physical=tuple(keys[i] for i in tri);uvs=tuple(points[i] for i in tri)
            if _orient(*uvs)<=0:raise ValueError(f'face {face.id}: nonpositive refined UV triangle {uvs!r} levels={tuple(levels[i] for i in tri)!r}')
            rounding=max(_uv_rounding_bound(uv,metric_cache[carrier.id]) for uv in uvs)
            if rounding>quality.tolerance_mm/16:raise ValueError('native UV rounding exceeds quality')
            errors['max_native_uv_rounding_bound_mm']=max(errors['max_native_uv_rounding_bound_mm'],rounding)
            collapsed=len(set(physical))<3
            if collapsed and any(physical.count(k)>1 and k not in singular_keys for k in set(physical)):
                raise ValueError('unsupported collapsed source triangle')
            xyz=tuple(vertex_by_key[k].xyz for k in physical)
            if not collapsed:
                area=_cross(_sub(xyz[1],xyz[0]),_sub(xyz[2],xyz[0]))
                if _norm(area)==0:raise ValueError(f'face {face.id}: zero physical triangle area; UV={uvs!r} XYZ={xyz!r} generators={tuple((vertex_by_key[k].source_kind,vertex_by_key[k].source_id,vertex_by_key[k].source_parameter) for k in physical)!r}')
            # UV cells/physical key order follow positive carrier orientation;
            # outward physical orientation additionally applies the face sign.
            indices.append(len(triangles))
            triangles.append(MeshTriangle(face.id,physical,tuple(MeshCorner(uv,slot,face.orientation) for uv in uvs),collapsed))
        face_meshes.append(FaceMesh(face.id,tuple(boundary_loops),tuple(indices),tuple(chart_closures),tuple(joint_recoveries)))
        if progress is not None and (face_index+1)%100==0:progress('faces',face_index+1,len(source.faces),face.id)
    if (2*errors['max_edge_chord_bound_mm']+errors['max_surface_interpolation_bound_mm']
            +errors['max_native_uv_rounding_bound_mm']>quality.tolerance_mm):
        raise ValueError('combined mesh approximation components exceed quality')
    end=time.perf_counter()
    return ImportedMesh(source,quality,tuple(vertices),tuple(triangles),tuple(face_meshes),
                        tuple(edge_samples),(('validate_and_sample',sampled-begin),('triangulate_refine',end-sampled),('complete_mesh',end-begin)),tuple(errors.items()))


def _write_scene(mesh,path,instances,materials,definition_materials):
    instances=tuple(instances);materials=tuple(materials)
    if not materials:raise ValueError('native scene requires materials')
    palette=dict(definition_materials);carrier_rows=[];control_rows=[];carrier_map={}
    for slot,carrier in enumerate(mesh.source.carriers):
        if carrier.kind==6:
            for si,span in enumerate(carrier.spans):
                offset=len(control_rows)//4;control_rows.extend(span.controls)
                carrier_map[(slot,si)]=len(carrier_rows)
                carrier_rows.append((carrier.id,6,(0.0,)*12,(0.0,0.0),span.bounds,offset,len(span.controls)//4,span.degrees))
        else:
            carrier_map[(slot,None)]=len(carrier_rows)
            # Lifted original UV domains are enlarged below from actual corners.
            carrier_rows.append((carrier.id,carrier.kind,carrier.frame,carrier.radii,list(carrier.uv_bounds),0,0,(0,0)))
    vertex_by_key={v.physical_key:v for v in mesh.vertices}
    definitions=[];display_vertices=[];display_triangles=[]
    face_definition={f.id:f.definition_id for f in mesh.source.faces};grouped={d.id:[] for d in mesh.source.definitions}
    for tri in mesh.triangles:grouped[face_definition[tri.face_id]].append(tri)
    for definition in mesh.source.definitions:
        first_v,first_t=len(display_vertices),len(display_triangles);mapping={}
        face_ids=set(definition.face_ids);material=palette.get(definition.id,0)
        if not 0<=material<len(materials):raise ValueError('invalid definition material slot')
        for tri in grouped[definition.id]:
            if tri.collapsed:continue
            local=[]
            for key,corner in zip(tri.vertex_keys,tri.corners):
                carrier=mesh.source.carriers[corner.carrier_slot]
                if carrier.kind==6:
                    span=_surface_span(carrier,*corner.uv);span_index=carrier.spans.index(span)
                    cs=carrier_map[(corner.carrier_slot,span_index)]
                else:
                    cs=carrier_map[(corner.carrier_slot,None)];bounds=carrier_rows[cs][4]
                    bounds[0]=min(bounds[0],corner.uv[0]);bounds[1]=max(bounds[1],corner.uv[0])
                    bounds[2]=min(bounds[2],corner.uv[1]);bounds[3]=max(bounds[3],corner.uv[1])
                vertex=vertex_by_key[key];flags=(1 if vertex.source_kind!=3 else 0)|(2 if corner.face_orientation<0 else 0)
                identity=(key,corner.uv,cs,flags,tri.face_id)
                if identity not in mapping:
                    mapping[identity]=len(display_vertices)-first_v
                    display_vertices.append((*vertex.xyz,*(float(x) for x in corner.uv),cs,flags,key))
                local.append(mapping[identity])
            if tri.corners[0].face_orientation<0:local[1],local[2]=local[2],local[1]
            display_triangles.append((*local,material,tri.face_id))
        definitions.append((definition.id,first_v,len(display_vertices)-first_v,first_t,len(display_triangles)-first_t))
    definition_slots={d[0]:i for i,d in enumerate(definitions)}
    if len({i.id for i in instances})!=len(instances):raise ValueError('duplicate scene instance ID')
    content=bytearray()
    def pack(fmt,*values):content.extend(struct.pack('<'+fmt,*values))
    content.extend(b'BGMESH01');pack('II',1,0)
    pack('8Q',len(definitions),len(instances),len(display_vertices),len(display_triangles),len(carrier_rows),len(materials),len(control_rows)//4,0)
    for d in definitions:pack('5Q',*d)
    for i in instances:
        if type(i) is not SceneInstance or i.definition_id not in definition_slots:raise ValueError('invalid scene instance')
        pack('QII15d',i.id,definition_slots[i.definition_id],0,*i.transform,*i.explosion_offset)
    for v in display_vertices:pack('5dIIQ',*v)
    for t in display_triangles:pack('4IQ',*t)
    for row in carrier_rows:
        identifier,kind,frame,radii,bounds,offset,count,degrees=row
        pack('QII18dQQIIQ',identifier,kind,0,*frame,*radii,*bounds,offset,count,*degrees,0)
    for m in materials:
        if type(m) is not SceneMaterial or len(m.rgb)!=3:raise ValueError('invalid scene material')
        pack('4d',*m.rgb,m.roughness)
    if control_rows:pack(str(len(control_rows))+'d',*control_rows)
    Path(path).write_bytes(content)
    return (('definitions',len(definitions)),('instances',len(instances)),('vertices',len(display_vertices)),
            ('triangles',len(display_triangles)),('carriers',len(carrier_rows)))
