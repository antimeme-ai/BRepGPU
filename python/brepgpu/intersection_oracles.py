"""Independent exact-map, geometric-span and actual two-source sample truth.

The arithmetic below reads raw a/b/d fields. It does not call construction,
production Scalar arithmetic/rounding or the production maps to obtain truth.
Generic validity and literal complete component counts are separate claims.
"""
from dataclasses import dataclass
from fractions import Fraction as F
import math
import struct


def require(condition, message):
    if not condition:
        raise AssertionError(message)


@dataclass(frozen=True)
class Value:
    a: F = F(0)
    b: F = F(0)
    d: int = 0

    def __post_init__(self):
        a,b,d=F(self.a),F(self.b),int(self.d)
        require(d>=0,"negative raw radicand")
        r=math.isqrt(d)
        if not b or r*r==d:a,b,d=a+b*r,F(0),0
        object.__setattr__(self,"a",a);object.__setattr__(self,"b",b);object.__setattr__(self,"d",d)

    def field(self, other):
        other=raw(other)
        if not self.b:return self.a,F(0),other.a,other.b,other.d
        if not other.b:return self.a,self.b,other.a,F(0),self.d
        ratio=F(other.d,self.d);n=math.isqrt(ratio.numerator);den=math.isqrt(ratio.denominator)
        if n*n!=ratio.numerator or den*den!=ratio.denominator:
            raise ArithmeticError("distinct quadratic fields")
        return self.a,self.b,other.a,other.b*F(n,den),self.d

    def __add__(self,other):
        a,b,c,e,d=self.field(other);return Value(a+c,b+e,d)
    __radd__=__add__
    def __neg__(self):return Value(-self.a,-self.b,self.d)
    def __sub__(self,other):return self+-raw(other)
    def __rsub__(self,other):return raw(other)+-self
    def __mul__(self,other):
        a,b,c,e,d=self.field(other);return Value(a*c+b*e*d,a*e+b*c,d)
    __rmul__=__mul__
    def __truediv__(self,other):
        other=raw(other);norm=other.a*other.a-other.b*other.b*other.d
        require(norm!=0,"zero exact denominator")
        return self*Value(other.a/norm,-other.b/norm,other.d)


def raw(value):
    if isinstance(value,Value):return value
    if hasattr(value,"a") and hasattr(value,"b") and hasattr(value,"d"):
        return Value(value.a,value.b,value.d)
    return Value(F.from_float(value) if isinstance(value,float) else F(value))


def bounds(value,bits=64):
    value=raw(value)
    if not value.b:return value.a,value.a
    denominator=1<<bits;k=math.isqrt(value.d << (2*bits))
    lo,hi=F(k,denominator),F(k+1,denominator)
    if value.b<0:lo,hi=hi,lo
    return value.a+value.b*lo,value.a+value.b*hi


def sign(value):
    value=raw(value)
    if not value.b:return (value.a>0)-(value.a<0)
    # Single-field equality is decided algebraically before interval refinement.
    if value.a*value.b<0 and value.a*value.a==value.b*value.b*value.d:return 0
    bits=16
    while True:
        lo,hi=bounds(value,bits)
        if lo>0:return 1
        if hi<0:return -1
        bits*=2


def compare(left,right):
    left,right=raw(left),raw(right)
    try:return sign(left-right)
    except ArithmeticError:
        # Distinct quadratic extensions cannot differ by a rational unless
        # both radical terms vanish; normalization has already handled that.
        bits=16
        while True:
            lo,hi=bounds(left,bits);rl,rh=bounds(right,bits)
            if lo>rh:return 1
            if hi<rl:return -1
            bits*=2


def equal(left,right):return compare(left,right)==0


def nearest(value):
    value=raw(value)
    if not value.b:return float(value.a)
    bits=16
    while True:
        lo,hi=bounds(value,bits)
        try:a,b=float(lo),float(hi)
        except OverflowError:bits*=2;continue
        if a==b:return a
        bits*=2


def poly(values):
    values=list(map(raw,values))
    require(bool(values),"empty polynomial")
    while len(values)>1 and sign(values[-1])==0:values.pop()
    return tuple(values)


def add(a,b):return poly((a[i] if i<len(a) else Value())+(b[i] if i<len(b) else Value())
                        for i in range(max(len(a),len(b))))
def neg(a):return tuple(-v for v in a)
def scale(a,k):return poly(v*raw(k) for v in a)
def mul(a,b):
    answer=[Value()]*(len(a)+len(b)-1)
    for i,x in enumerate(a):
        for j,y in enumerate(b):answer[i+j]=answer[i+j]+x*y
    return poly(answer)
def power(a,n):
    out=(Value(1),)
    for _ in range(n):out=mul(out,a)
    return out

def evaluate(a,t):
    out=Value()
    for v in reversed(a):out=out*raw(t)+v
    return out

def zero(a):return all(sign(v)==0 for v in a)

def divide_poly(numerator,denominator):
    remainder=list(poly(numerator));denominator=poly(denominator)
    require(not zero(denominator),"zero polynomial divisor")
    result=[Value()] * max(1,len(remainder)-len(denominator)+1)
    while len(remainder)>=len(denominator) and not zero(remainder):
        k=len(remainder)-len(denominator);factor=remainder[-1]/denominator[-1];result[k]=factor
        for i,c in enumerate(denominator):remainder[k+i]=remainder[k+i]-factor*c
        remainder=list(poly(remainder))
    return poly(result),poly(remainder)


def map_data(mapping,width):
    require(isinstance(mapping.numerators,tuple) and isinstance(mapping.denominator,tuple)
            and len(mapping.numerators)==width and all(isinstance(n,tuple) for n in mapping.numerators),
            "exact map shape/storage differs")
    return tuple(poly(n) for n in mapping.numerators),poly(mapping.denominator)


def map_values(mapping,t,width):
    n,d=map_data(mapping,width);den=evaluate(d,t)
    require(sign(den)>0,"map denominator not positive at exact parameter")
    return tuple(evaluate(v,t)/den for v in n)


def positive_denominator(d,lo,hi):
    if len(d)==1:require(sign(d[0])>0,"nonpositive constant denominator");return
    require(compare(lo,0)>=0 and sign(d[0])>0 and all(sign(c)>=0 for c in d),
            "declared piece has no supported whole-domain positive denominator")


def reduced(a):
    # Disk expressions from a rational quarter contain proved-positive 1+t²
    # factors. Divide them exactly, independently of construction's formula.
    a=poly(a)
    while len(a)>3:
        quotient,remainder=divide_poly(a,poly((1,0,1)))
        require(zero(remainder),"trim polynomial has an unexplained higher-degree factor")
        a=quotient
    return a


def nonnegative(a,lo,hi,split=True):
    a=reduced(a)
    require(sign(evaluate(a,lo))>=0 and sign(evaluate(a,hi))>=0,"piece endpoint outside closed domain/trim")
    if len(a)==3 and sign(a[2])>0:
        critical=-a[1]/(a[2]*2)
        if compare(lo,critical)<0 and compare(critical,hi)<0:
            value=sign(evaluate(a,critical))
            require(value>=0,"piece interior crosses excluded trim/domain")
            if split:require(value!=0,"piece lost an internal admitted trim event")


def face_of(ref):
    snapshot=ref.snapshot
    require(isinstance(ref.face_id,int) and 1<=ref.face_id<=len(snapshot.faces),"face identity outside original snapshot")
    face=snapshot.faces[ref.face_id-1]
    require(face.id==ref.face_id,"typed face identity differs")
    return face


def native_map(patch,uv_map):
    (u,v),d=map_data(uv_map,2);one_minus_u=add(d,neg(u));one_minus_v=add(d,neg(v))
    p,q=patch.degree_u,patch.degree_v
    controls=[tuple(raw(x) for x in patch.controls[i:i+4]) for i in range(0,len(patch.controls),4)]
    homogeneous=[poly((0,)) for _ in range(4)]
    for i in range(p+1):
        bu=scale(mul(power(u,i),power(one_minus_u,p-i)),math.comb(p,i))
        for j in range(q+1):
            bv=scale(mul(power(v,j),power(one_minus_v,q-j)),math.comb(q,j));basis=mul(bu,bv)
            for a in range(4):homogeneous[a]=add(homogeneous[a],scale(basis,controls[i*(q+1)+j][a]))
    w=homogeneous[3]
    return tuple(add(scale(homogeneous[a],patch.scale),scale(w,patch.origin[a])) for a in range(3)),w


def source_constraints(ref,slot,mapping,side):
    face=face_of(ref);n,d=map_data(mapping,2)
    if face.kind=="cylinder":return ((side+":z0",n[1]),(side+":z1",add(d,neg(n[1]))))
    answer=[]
    if face.kind in ("rectangle","rectangle_hole"):
        for row,axis in zip(n,("u","v")):
            answer.extend(((side+":"+axis+"0",row),(side+":"+axis+"1",add(d,neg(row)))))
    if face.kind in ("disk","rectangle_hole"):
        distance=add(power(add(scale(n[0],2),neg(d)),2),power(add(scale(n[1],2),neg(d)),2))
        term=add(scale(distance,9 if face.kind=="rectangle_hole" else 1),neg(power(d,2)))
        answer.append((side+(":hole" if face.kind=="rectangle_hole" else ":rim"),term if face.kind=="rectangle_hole" else neg(term)))
    return tuple((name,reduced(values)) for name,values in answer)


def joins_at(ref,slot,mapping,t,side):
    if face_of(ref).kind!="cylinder":return set()
    u=map_values(mapping,t,2)[0];q=ref.snapshot.charts[slot].quadrant
    if equal(u,0) or equal(u,1):
        atlas=(q+int(equal(u,1)))%4
        return {side+":atlas:"+("0/4" if atlas==0 else str(atlas))}
    return set()


def same_map(first,second,width):
    n,d=map_data(first,width);m,e=map_data(second,width)
    return all(zero(add(mul(a,e),neg(mul(b,d)))) for a,b in zip(n,m))


def qualify_result(result):
    """Prove submitted whole maps/domains and grouped graph; not arbitrary completeness."""
    from .body_oracles import qualify_snapshot
    refs=(result.left,result.right)
    for ref in refs:qualify_snapshot(ref.snapshot)
    require(sum(face_of(ref).kind=="cylinder" for ref in refs)==1,"not one plane and one wall")
    for records in (result.components,result.pieces,result.events):
        require(isinstance(records,tuple),"mutable result table")
        require(tuple(r.id for r in records)==tuple(range(len(records))),"nondense result identities")
    pieces=result.pieces;events=result.events
    endpoint={};locations={};constraint_sets={};join_sets={}
    for piece in pieces:
        require(0<=piece.component_id<len(result.components),"foreign component identity")
        require(isinstance(piece.span_id,int) and piece.span_id>=0,"invalid physical span")
        require(piece.kind in ("point","arc","segment"),"unknown piece kind")
        point=piece.kind=="point"
        require(compare(piece.lo,piece.hi)==0 if point else compare(piece.lo,piece.hi)<0,"invalid exact piece interval")
        _,spatial_den=map_data(piece.spatial_map,3);positive_denominator(spatial_den,piece.lo,piece.hi)
        wall_mapping=piece.left_map if face_of(result.left).kind=="cylinder" else piece.right_map
        wall_n,wall_d=map_data(wall_mapping,2)
        if piece.kind=="arc":
            require(zero(add(wall_n[0],neg(mul(poly((0,1)),wall_d)))),"arc parameter is not original rational quarter UV")
        elif piece.kind=="segment":
            spatial_n,_=map_data(piece.spatial_map,3)
            require(zero(add(spatial_n[2],neg(mul(poly((0,1)),spatial_den)))),"generator parameter is not exact world height")
            require(zero(add(wall_n[0],neg(scale(wall_d,map_values(wall_mapping,piece.lo,2)[0])))),"generator changes angular coordinate")
        active_whole=set()
        for ref,slot,mapping,side in zip(refs,(piece.left_slot,piece.right_slot),
                                        (piece.left_map,piece.right_map),("left","right")):
            require(slot in face_of(ref).chart_slots,"piece refers to a chart outside selected face")
            n,d=map_data(mapping,2);positive_denominator(d,piece.lo,piece.hi)
            original_n,original_d=native_map(ref.snapshot.patches[slot],mapping)
            spatial_n,_=map_data(piece.spatial_map,3)
            require(all(zero(add(mul(a,spatial_den),neg(mul(b,original_d))))
                        for a,b in zip(original_n,spatial_n)),"whole map does not equal ORIGINAL source controls")
            for numerator in n:
                nonnegative(numerator,piece.lo,piece.hi,False)
                nonnegative(add(d,neg(numerator)),piece.lo,piece.hi,False)
            for name,values in source_constraints(ref,slot,mapping,side):
                nonnegative(values,piece.lo,piece.hi,not point)
                if zero(values) or (point and sign(evaluate(values,piece.lo))==0):active_whole.add(name)
        require(set(piece.boundary_constraints)==active_whole,"piece boundary constraints differ from whole exact source")
        for location,t in (("point",piece.lo),) if point else (("lo",piece.lo),("hi",piece.hi)):
            key=(piece.id,location);locations[key]=map_values(piece.spatial_map,t,3)
            constraints=set();joins=set()
            for ref,slot,mapping,side in zip(refs,(piece.left_slot,piece.right_slot),
                                            (piece.left_map,piece.right_map),("left","right")):
                constraints.update(name for name,values in source_constraints(ref,slot,mapping,side)
                                   if sign(evaluate(values,t))==0)
                joins.update(joins_at(ref,slot,mapping,t,side))
            constraint_sets[key]=constraints;join_sets[key]=joins
    for event in events:
        require(isinstance(event.incidences,tuple) and bool(event.incidences),"empty/mutable event incidence")
        require(len(set(event.incidences))==len(event.incidences),"duplicate event occurrence")
        expected_constraints=set();expected_joins=set();position=None
        for key in event.incidences:
            require(key in locations and key not in endpoint,"missing or repeated endpoint occurrence")
            endpoint[key]=event.id;actual=locations[key]
            if position is None:position=actual
            else:require(all(equal(a,b) for a,b in zip(position,actual)),"event weld joins different exact world positions")
            expected_constraints.update(constraint_sets[key]);expected_joins.update(join_sets[key])
        require(set(event.constraints)==expected_constraints,"event constraint labels differ")
        require(set(event.joins)==expected_joins,"event atlas labels differ")
    require(set(endpoint)==set(locations),"endpoint occurrence omitted from graph")
    spans={}
    for piece in pieces:spans.setdefault(piece.span_id,[]).append(piece)
    require(set(spans)==set(range(len(spans))),"nondense physical span identities")
    for group in spans.values():
        first=group[0]
        for other in group[1:]:
            require(other.component_id==first.component_id and other.kind==first.kind,"span merges incompatible components/kinds")
            if first.kind=="point":
                require(endpoint[(other.id,"point")]==endpoint[(first.id,"point")],"point span merges positions")
            else:
                require(equal(first.lo,other.lo) and equal(first.hi,other.hi)
                        and same_map(first.spatial_map,other.spatial_map,3),"span merges different physical curves")
                require(tuple(endpoint[(other.id,k)] for k in ("lo","hi"))==
                        tuple(endpoint[(first.id,k)] for k in ("lo","hi")),"span endpoint identity differs")
    for component in result.components:
        owned=tuple(p.id for p in pieces if p.component_id==component.id)
        require(isinstance(component.piece_ids,tuple) and set(component.piece_ids)==set(owned)
                and len(component.piece_ids)==len(owned) and bool(owned),"component omits/repeats piece occurrences")
        ordered=[];seen_spans=set()
        for pid in component.piece_ids:
            p=pieces[pid]
            if ordered and ordered[-1].span_id==p.span_id:continue
            require(p.span_id not in seen_spans,"component revisits a physical span outside its occurrence group")
            ordered.append(p);seen_spans.add(p.span_id)
        reps=ordered
        if component.kind=="point":
            require(not component.closed and len(reps)==1 and reps[0].kind=="point","point graph has edges")
            require(bool(events[endpoint[(reps[0].id,"point")]].constraints),"isolated point has no exact boundary event")
            continue
        require(component.kind in ("circle","arc","segment") and all(p.kind!="point" for p in reps),"component kind differs")
        adjacency={}
        for p in reps:
            a,b=(endpoint[(p.id,k)] for k in ("lo","hi"))
            adjacency.setdefault(a,[]).append(b);adjacency.setdefault(b,[]).append(a)
        for first,second in zip(reps,reps[1:]):
            require(endpoint[(first.id,"hi")]==endpoint[(second.id,"lo")],"component span traversal is not oriented/continuous")
        if component.closed:
            require(endpoint[(reps[-1].id,"hi")]==endpoint[(reps[0].id,"lo")],"ordered closed traversal does not close")
        reached=set();pending=[next(iter(adjacency))]
        while pending:
            a=pending.pop()
            if a not in reached:reached.add(a);pending.extend(adjacency[a])
        require(reached==set(adjacency),"disconnected component")
        degrees=sorted(map(len,adjacency.values()))
        if component.closed:require(component.kind=="circle" and all(d==2 for d in degrees),"closed component is not a cycle")
        else:
            require(degrees.count(1)==2 and all(d in (1,2) for d in degrees),"open component is not one path")
            require(all(events[event_id].constraints for event_id,neighbors in adjacency.items() if len(neighbors)==1),
                    "open component ends away from an exact face boundary")
    return dict(components=len(result.components),pieces=len(pieces),spans=len(spans),events=len(events),
                circles=sum(c.kind=="circle" for c in result.components),arcs=sum(c.kind=="arc" for c in result.components),
                segments=sum(c.kind=="segment" for c in result.components),points=sum(c.kind=="point" for c in result.components))


def recipe_counts(result,recipe):
    """Literal complete counts/endpoints; whole-map validity is qualify_result's job.

    Callers pair this distinct completeness check with qualify_batch, which checks
    whole source maps/graph and actual native consumption once.
    """
    stats=dict(components=len(result.components),pieces=len(result.pieces),
               spans=len({p.span_id for p in result.pieces}),events=len(result.events),
               circles=sum(c.kind=="circle" for c in result.components),
               arcs=sum(c.kind=="arc" for c in result.components),
               segments=sum(c.kind=="segment" for c in result.components),
               points=sum(c.kind=="point" for c in result.components))
    plane=next(ref for ref in (result.left,result.right) if face_of(ref).kind!="cylinder")
    wall=next(ref for ref in (result.left,result.right) if face_of(ref).kind=="cylinder")
    require(plane.snapshot.shape=="box" and plane.face_id==1 and wall.snapshot.shape=="cylinder" and wall.face_id==3,
            "recipe source families/faces differ")
    expected_frame=((0.,0.,-.5),1.) if recipe=="circle" else ((0.,0.,-.125),.25) if recipe=="clipped" else None
    require(expected_frame is not None,"unknown complete recipe")
    require((plane.snapshot.frame.origin,plane.snapshot.frame.scale)==expected_frame
            and (wall.snapshot.frame.origin,wall.snapshot.frame.scale)==((0.,0.,0.),1.),"recipe source frames differ")
    expected=(1,4,4,4) if recipe=="circle" else (4,4,4,8)
    require(tuple(stats[k] for k in ("components","pieces","spans","events"))==expected,"literal complete component counts differ")
    require(all(c.kind==("circle" if recipe=="circle" else "arc") and c.closed==(recipe=="circle")
                and not c.tangent for c in result.components),"literal component classifications differ")
    wall_slots=[p.left_slot if wall is result.left else p.right_slot for p in result.pieces]
    require(sorted(wall_slots)==[2,3,4,5],"recipe omitted original quarter")
    for p in result.pieces:
        lo,hi=(0,1) if recipe=="circle" else (Value(0,F(1,7),7),Value(F(4,3),F(-1,3),7))
        require(equal(p.lo,lo) and equal(p.hi,hi),"literal exact quarter endpoints differ")
    return stats


_PHASES=("gather_ms","query_ms","evaluate_ms","download_ms","convert_ms","world_scatter_ms",
         "grade_ms","child_cleanup_ms","reused_wall_ms")
_DESCRIPTOR=struct.Struct("<4I6Q2I8d")


def qualify_batch(batch,request=None):
    """Grade original maps/native rows; bind requested row coverage when supplied.

    Without a request this validates the submitted rows, not their coverage of
    an unavailable prepared input. Actual operation callers pass their request.
    """
    stats=qualify_result(batch.result);result=batch.result;rows=batch.rows
    if request is not None:
        require(request.result is result and isinstance(request.candidates,tuple)
                and len(request.candidates)==len(rows),"batch is not bound to the actual prepared input")
    require(isinstance(rows,tuple) and batch.count==len(rows) and batch.evaluated_count==2*len(rows),"batch row/count differs")
    require(isinstance(batch.timings,tuple) and tuple(k for k,v in batch.timings)==_PHASES
            and all(math.isfinite(v) and v>=0 for k,v in batch.timings),"batch timing scopes differ")
    for name in ("source_descriptors","native_slots","native_uv"):
        require(type(getattr(batch,name)) is bytes,"mutable native artifact")
    dense=tuple((side,slot) for side,ref in enumerate((result.left,result.right)) for slot in face_of(ref).chart_slots) if rows else ()
    require(batch.dense_sources==dense,"actual source order differs")
    expected=b"";offset=0
    for side,slot in dense:
        patch=(result.left if side==0 else result.right).snapshot.patches[slot];lineage=patch.lineage
        expected+=_DESCRIPTOR.pack(1,136,patch.degree_u,patch.degree_v,offset,len(patch.controls)//4,
            lineage.body_id,lineage.entity_id,lineage.revision,lineage.chart_id,lineage.entity_kind,3,
            *patch.uv,*patch.origin,patch.scale)
        offset+=len(patch.controls)//4
    require(batch.source_descriptors==expected,"actual native descriptor bytes do not bind original source/frame")
    require(len(batch.native_slots)==8*len(rows) and len(batch.native_uv)==32*len(rows),"actual native input extents differ")
    indices={key:i for i,key in enumerate(dense)};slots=struct.unpack("<"+"I"*(2*len(rows)),batch.native_slots)
    uv=struct.unpack("<"+"d"*(4*len(rows)),batch.native_uv)
    maximum=Value();pair_maximum=F(0);tolerance=F(1,1<<36)
    for index,row in enumerate(rows):
        require(row.index==index and 0<=row.piece_id<len(result.pieces),"row identity/order differs")
        p=result.pieces[row.piece_id];t=raw(row.parameter)
        require((row.component_id,row.span_id,row.left_slot,row.right_slot)==
                (p.component_id,p.span_id,p.left_slot,p.right_slot),"row lost original occurrence")
        require(compare(t,p.lo)>=0 and compare(t,p.hi)<=0,"sample parameter outside exact piece")
        location="point" if p.kind=="point" else "lo" if equal(t,p.lo) else "hi" if equal(t,p.hi) else "interior"
        require(row.location==location and (location!="interior" or not t.b),"sample location/interior field differs")
        if request is not None:
            candidate=request.candidates[index]
            require((row.piece_id,row.component_id,row.span_id,row.location)==
                    (candidate.piece_id,candidate.component_id,candidate.span_id,candidate.location)
                    and equal(t,candidate.parameter),"row order/parameter differs from actual prepared input")
            require(all(equal(a,b) for a,b in zip(row.reference_position,candidate.reference_position))
                    and len(candidate.reference_position)==3,"row reference differs from prepared input")
        position=map_values(p.spatial_map,t,3)
        require(len(row.reference_position)==3 and all(equal(a,b) for a,b in zip(row.reference_position,position)),"row reference differs from whole original map")
        require((row.left_status,row.right_status)==(0,0),"native source status failure")
        for side,mapping,actual_uv,actual_position,slot in ((0,p.left_map,row.left_uv,row.left_position,p.left_slot),
                                                         (1,p.right_map,row.right_uv,row.right_position,p.right_slot)):
            exact_uv=map_values(mapping,t,2);rounded=tuple(nearest(v) for v in exact_uv)
            if request is not None:
                submitted_uv=candidate.left_uv if side==0 else candidate.right_uv
                require(len(submitted_uv)==2 and all(equal(a,b) for a,b in zip(submitted_uv,exact_uv)),
                        "prepared UV differs from original source map")
            require(struct.pack("<2d",*actual_uv)==struct.pack("<2d",*rounded),"rounded native UV differs from independent nearest")
            require(slots[2*index+side]==indices[(side,slot)],"actual native slot differs from occurrence")
            require(struct.pack("<2d",*uv[4*index+2*side:4*index+2*side+2])==struct.pack("<2d",*actual_uv),"actual native UV artifact differs")
            require(len(actual_position)==3 and all(math.isfinite(v) for v in actual_position),"nonfinite/wrong-width native position")
            for actual,reference in zip(actual_position,position):
                error=raw(actual)-reference
                if sign(error)<0:error=-error
                require(compare(error,tolerance)<=0,"actual native position violates exact model budget")
                if compare(error,maximum)>0:maximum=error
        for left,right in zip(row.left_position,row.right_position):
            error=abs(F.from_float(left)-F.from_float(right));pair_maximum=max(pair_maximum,error)
            require(error<=tolerance,"two actual carrier positions disagree")
    stats.update(count=len(rows),evaluated_count=2*len(rows),max_position_error=nearest(maximum),max_pair_error=float(pair_maximum))
    return stats
