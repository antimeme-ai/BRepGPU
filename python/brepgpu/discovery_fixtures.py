"""Exact original-input discovery oracles, without a reference traversal/solver.

Restriction expands Bernstein polynomials into powers, substitutes an exact
cell map, and converts back. Objective caches use exact power convolution.
"""
from dataclasses import dataclass
from fractions import Fraction
from functools import lru_cache
import math

from .fixtures import Fixture, ERROR_TARGETS, degree_fixture, plane, rational_cylinder
from .projection_fixtures import orthogonal_plane, parabolic, normal_parabolic_cases

DIRECTIONS = ((1,1,0),(1,-1,0),(1,0,1),(1,0,-1),(0,1,1),(0,1,-1),
              (1,1,1),(1,1,-1),(1,-1,1),(1,-1,-1))
CANDIDATE, GAP, COVERAGE, VISIT_LIMIT, REFINEMENT_LIMIT, ARITHMETIC = 1,2,4,8,16,32
STRIDES = (0,20,490)
F0 = Fraction(0)


def ratio(value):
    return value if isinstance(value,Fraction) else Fraction.from_float(float(value))


def _zeros(p,q):return [[F0 for _ in range(q+1)] for _ in range(p+1)]


@lru_cache(maxsize=128)
def source_power(fixture):
    """Four immutable dense power-coefficient matrices for original controls."""
    p,q=fixture.degree_u,fixture.degree_v
    channels=[]
    for axis in range(4):
        coefficients=_zeros(p,q)
        for i in range(p+1):
            for j in range(q+1):
                h=ratio(fixture.controls[4*(i*(q+1)+j)+axis])
                for a in range(i,p+1):
                    for b in range(j,q+1):
                        coefficients[a][b] += h*math.comb(p,i)*math.comb(p-i,a-i)*(-1)**(a-i) \
                            *math.comb(q,j)*math.comb(q-j,b-j)*(-1)**(b-j)
        channels.append(tuple(tuple(row) for row in coefficients))
    return tuple(channels)


def _restrict(coefficients,cell):
    ul,ui,vl,vi=cell
    alpha,delta=Fraction(ui,1<<ul),Fraction(1,1<<ul)
    beta,epsilon=Fraction(vi,1<<vl),Fraction(1,1<<vl)
    p,q=len(coefficients)-1,len(coefficients[0])-1
    result=_zeros(p,q)
    for r in range(p+1):
        for k in range(q+1):
            result[r][k]=delta**r*epsilon**k*sum(
                coefficients[a][b]*math.comb(a,r)*alpha**(a-r)
                *math.comb(b,k)*beta**(b-k)
                for a in range(r,p+1) for b in range(k,q+1))
    return tuple(tuple(row) for row in result)


def _bernstein(coefficients,p,q):
    return tuple(sum(coefficients[r][k]*Fraction(math.comb(i,r),math.comb(p,r))
                     *Fraction(math.comb(j,k),math.comb(q,k))
                     for r in range(i+1) for k in range(j+1))
                 for i in range(p+1) for j in range(q+1))


@lru_cache(maxsize=2048)
def restricted_controls(fixture,cell):
    channels=tuple(_bernstein(_restrict(c,cell),fixture.degree_u,fixture.degree_v)
                   for c in source_power(fixture))
    return tuple(tuple(c[i] for c in channels)
                 for i in range((fixture.degree_u+1)*(fixture.degree_v+1)))


def _multiply(a,b):
    p,q=len(a)+len(b)-2,len(a[0])+len(b[0])-2
    result=_zeros(p,q)
    for i,row in enumerate(a):
        for j,x in enumerate(row):
            for k,other in enumerate(b):
                for l,y in enumerate(other):result[i+k][j+l]+=x*y
    return tuple(tuple(row) for row in result)


@lru_cache(maxsize=512)
def base_coefficients(fixture,cell):
    """C0..C4 through exact power convolution, not native Bernstein products."""
    a=tuple(_restrict(c,cell) for c in source_power(fixture))
    p,q=2*fixture.degree_u,2*fixture.degree_v
    squares=tuple(_multiply(c,c) for c in a[:3])
    c0=tuple(tuple(sum(square[i][j] for square in squares)
                   for j in range(q+1)) for i in range(p+1))
    return tuple(_bernstein(c,p,q) for c in (c0,*(_multiply(c,a[3]) for c in a[:3]),_multiply(a[3],a[3])))


def exact_point(fixture,uv):
    s,t=map(ratio,uv)
    values=tuple(sum(coef*s**i*t**j for i,row in enumerate(channel)
                     for j,coef in enumerate(row)) for channel in source_power(fixture))
    if values[3]<=0:raise ValueError("positive exact denominator required")
    return tuple(x/values[3] for x in values[:3])


def exact_distance(fixture,target,uv):
    point=exact_point(fixture,uv)
    return point,sum((p-ratio(x))**2 for p,x in zip(point,target))


def box_distance(lower,upper,target):
    return sum(max(lo-x,F0,x-hi)**2 for lo,hi,x in zip(lower,upper,map(ratio,target)))


def diagonal_plane():
    return Fixture("diagonal_plane",1,1,tuple(float(x) for s in (0,1) for t in (0,1)
                                             for x in (s,s,t,1)))


def triple_plane():
    return Fixture("three_axis_plane",1,1,tuple(float(x) for s in (0,1) for t in (0,1)
                                                for x in (s-.5,t-.5,1-s-t,1)))


def negative_coefficients():
    return Fixture("negative_C0_line",1,0,(-.5,0.,0.,1.,.5,0.,0.,1.))


def subnormal_line(sign=1):
    return Fixture("subnormal_line_"+str(sign),1,0,(0.,0.,0.,1.,sign*2.**-1074,0.,0.,1.))


def weighted_line():
    return Fixture("weighted_line",1,0,(0.,0.,0.,1.,256.,0.,0.,256.))


def folded():
    return Fixture("folded_plane",2,1,tuple(float(v) for x in (1,-1,1) for t in (0,1)
                                           for v in (x,t,0,1)))


def rank_one():return Fixture("rank_one",1,0,(0.,0.,0.,1.,1.,0.,0.,1.))
def constant_third():return Fixture("constant_third",0,0,(1.,0.,0.,3.))
def constant_zero():return Fixture("constant_zero",0,0,(0.,0.,0.,1.))


@dataclass(frozen=True)
class DiscoveryCase:
    name:str
    fixture:Fixture
    target:tuple
    distance_answer:object=None
    depth:int=4
    max_visits:int=8191
    max_refinements:int=8
    tolerance_squared:float=2.**-24
    status:object=None
    required_flags:int=0
    forbidden_flags:int=0
    exact_visits:object=None
    exact_refinements:object=None
    exact_evaluations:object=None


def discovery_cases():
    cases=[]
    for p in range(4):
        for q in range(4):
            for pattern in ("constant","asymmetric","cancellation"):
                f=degree_fixture(p,q,pattern)
                target=(.125,-.375,.5)
                answer=sum((v-ratio(x))**2 for v,x in zip((Fraction(1,4),Fraction(-1,2),Fraction(3,4)),target)) if pattern=="constant" else None
                cases.append(DiscoveryCase(f.name,f,target,answer))
    f=orthogonal_plane()
    for a in (-.25,.25,1.25):
        for b in (-.25,.75,1.25):
            answer=(ratio(a)-max(F0,min(Fraction(1),ratio(a))))**2 \
                   +(ratio(b)-max(F0,min(Fraction(1),ratio(b))))**2+Fraction(1,4)
            cases.append(DiscoveryCase("plane_"+str((a,b)),f,(a,b,.5),answer))
    for x in (0.,math.nextafter(0.,-1.),math.nextafter(0.,1.),.5,math.nextafter(1.,0.),math.nextafter(1.,2.),1.):
        answer=(ratio(x)-max(F0,min(Fraction(1),ratio(x))))**2
        cases.append(DiscoveryCase("plane_boundary_"+x.hex(),f,(x,.5,0.),answer))
    cases += [DiscoveryCase("tilted_boundary",plane(),(-.25,.5,.25),Fraction(1,16)),
              DiscoveryCase("diagonal_norm2",diagonal_plane(),(.75,.25,.5),Fraction(1,8)),
              DiscoveryCase("diagonal_norm3",triple_plane(),(.25,.25,.25),Fraction(3,16)),
              DiscoveryCase("negative_C0",negative_coefficients(),(.25,0.,0.),F0),
              DiscoveryCase("one_third_false_zero",constant_third(),(float(Fraction(1,3)),0.,0.),
                            (Fraction(1,3)-ratio(float(Fraction(1,3))))**2),
              DiscoveryCase("constant_zero",constant_zero(),(0.,0.,0.),F0),
              DiscoveryCase("weighted_line",weighted_line(),(.5,0.,0.),F0),
              DiscoveryCase("rank_one",rank_one(),(.25,.5,.25),Fraction(5,16)),
              DiscoveryCase("fold_two_UV",folded(),(.25,.5,.25),Fraction(1,16)),
              DiscoveryCase("fold_singular",folded(),(0.,.5,.25),Fraction(1,16))]
    for sign in (-1,1):
        cases.append(DiscoveryCase("subnormal_"+str(sign),subnormal_line(sign),(0.,0.,0.),F0))
    c=rational_cylinder()
    for name,target,answer in (("radial",(.75,1.,.25),Fraction(1,16)),
                               ("axis",(0.,0.,.5),Fraction(1)),
                               ("endpoint_competition",(-.75,-1.,.25),Fraction(65,16)),
                               ("endpoint_tie",(-1.,-1.,.5),Fraction(5))):
        cases.append(DiscoveryCase("cylinder_"+name,c,target,answer))
    for depth in (0,2,4,8):
        cases.append(DiscoveryCase("cylinder_axis_depth_"+str(depth),c,(0.,0.,.5),Fraction(1),depth=depth,max_refinements=1))
        cases.append(DiscoveryCase("diagonal_depth_"+str(depth),diagonal_plane(),(.75,.25,.5),Fraction(1,8),depth=depth,max_refinements=1))
    cases += [DiscoveryCase("parabola_two_minima",parabolic(),(0.,.5,.75),Fraction(1,2)),
              DiscoveryCase("parabola_quartic",parabolic(),(0.,.5,.5),Fraction(1,4))]
    cases += [DiscoveryCase(c.name,c.fixture,c.target,c.distance_answer) for c in normal_parabolic_cases()]
    # Bracket precision and traversal accounting have intentionally separate axes.
    cases += [DiscoveryCase("cap_tight",f,(.5,.5,.5),Fraction(1,4),max_visits=1,
                            max_refinements=0,tolerance_squared=2.**-16,status=0,
                            required_flags=GAP|VISIT_LIMIT|REFINEMENT_LIMIT,forbidden_flags=COVERAGE,
                            exact_visits=1,exact_refinements=0,exact_evaluations=1),
              DiscoveryCase("cap_wide",f,(.25,.75,.5),Fraction(1,4),max_visits=1,
                            max_refinements=0,status=1,required_flags=VISIT_LIMIT|REFINEMENT_LIMIT,
                            forbidden_flags=COVERAGE|GAP,exact_visits=1,exact_refinements=0,exact_evaluations=1),
              DiscoveryCase("complete_wide",f,(.25,.75,.5),Fraction(1,4),depth=0,
                            max_refinements=0,status=1,required_flags=COVERAGE|REFINEMENT_LIMIT,
                            forbidden_flags=VISIT_LIMIT|GAP,exact_visits=1,exact_refinements=0,exact_evaluations=1),
              DiscoveryCase("complete_tight_root_leaf",f,(.25,.75,.5),Fraction(1,4),depth=0,
                            max_refinements=8,tolerance_squared=2.**-16,status=0,required_flags=GAP|COVERAGE,
                            forbidden_flags=VISIT_LIMIT|REFINEMENT_LIMIT,exact_visits=1,exact_refinements=1,
                            exact_evaluations=4),
              DiscoveryCase("constant_root_once",constant_zero(),(0.,0.,0.),F0,
                            max_refinements=8,tolerance_squared=2.**-16,status=0,required_flags=GAP|COVERAGE,
                            forbidden_flags=VISIT_LIMIT|REFINEMENT_LIMIT,exact_visits=1,exact_refinements=1,
                            exact_evaluations=2)]
    return tuple(cases)


@dataclass(frozen=True)
class NodeTruth:
    patch:int
    cell:tuple
    lower:tuple
    upper:tuple
    leaf:bool
    supports:tuple=()
    coefficients:tuple=()


@dataclass(frozen=True)
class HierarchyOracle:
    fixtures:tuple
    nodes:tuple
    roots:tuple
    leaves:tuple
    mode:int

    def node_lower(self,index,target):
        node=self.nodes[index]
        answer=box_distance(node.lower,node.upper,target)
        if self.mode==1:
            for n,(lo,hi) in zip(DIRECTIONS,node.supports):
                projection=sum(k*ratio(x) for k,x in zip(n,target))
                answer=max(answer,max(lo-projection,F0,projection-hi)**2/sum(k*k for k in n))
        elif self.mode==2:
            x=tuple(map(ratio,target));q=sum(v*v for v in x);c=node.coefficients
            objective=min((c[0][i]-2*sum(x[k]*c[k+1][i] for k in range(3))+q*c[4][i])/c[4][i]
                          for i in range(len(c[4])))
            answer=max(answer,F0,objective)
        return answer

    def lower_limit(self,slot,target,limited=False):
        if limited:return self.node_lower(self.roots[slot],target)
        return min(self.node_lower(index,target) for index in self.leaves[slot])


def qualify_hierarchy(fixtures,batch,depth,mode):
    """Return direct cache violations and a flat exact query lower oracle."""
    fixtures=tuple(fixtures);issues=[];nodes=batch.nodes;roots=tuple(map(int,batch.roots));bounds=batch.bounds
    count=len(nodes);stride=STRIDES[mode]
    expected=sum(1 if f.degree_u==f.degree_v==0 else (1<<(depth+1))-1 for f in fixtures)
    if count!=expected:issues.append("node count differs from complete admitted tree")
    if int(batch.info.node_count)!=count or int(batch.info.patch_count)!=len(fixtures):issues.append("hierarchy info counts differ")
    if int(batch.mode)!=mode or int(batch.info.depth)!=depth:issues.append("hierarchy mode/depth differs")
    if int(batch.info.node_size)!=80:issues.append("hierarchy node ABI size differs")
    if len(roots)!=len(fixtures) or len(bounds)!=count*stride:issues.append("root/cache extent differs")
    if len(roots)!=len(fixtures):return issues,None
    leaves=[[] for _ in fixtures];truth=[];visited=set();expected_preorder=0
    for slot,(f,root) in enumerate(zip(fixtures,roots)):
        stack=[(root,(0,0,0,0),0)]
        while stack:
            index,cell,level=stack.pop()
            if index in visited or not 0<=index<count:
                issues.append("duplicate/cyclic/out-of-range node");continue
            visited.add(index)
            if index!=expected_preorder:issues.append("nodes are not canonical preorder")
            expected_preorder+=1
            n=nodes[index]
            actual=(int(n.u_level),int(n.u_index),int(n.v_level),int(n.v_index))
            if actual!=cell or int(n.patch_slot)!=slot:issues.append("node cell/source identity differs")
            leaf=bool(int(n.flags)&1)
            expected_leaf=f.degree_u==f.degree_v==0 or level==depth
            if leaf!=expected_leaf or int(n.flags) not in (0,1):issues.append("node leaf/flags differs")
            if leaf:
                leaves[slot].append(index)
                if int(n.left)!=0xFFFFFFFF or int(n.right)!=0xFFFFFFFF:issues.append("leaf child sentinel differs")
            else:
                axis=0 if f.degree_v==0 or (f.degree_u and level%2==0) else 1
                ul,ui,vl,vi=cell
                left=(ul+1,2*ui,vl,vi) if axis==0 else (ul,ui,vl+1,2*vi)
                right=(ul+1,2*ui+1,vl,vi) if axis==0 else (ul,ui,vl+1,2*vi+1)
                stack.extend(((int(n.right),right,level+1),(int(n.left),left,level+1)))
    if len(visited)!=count:issues.append("unreachable node or incomplete root coverage")
    for index,n in enumerate(nodes):
        slot=int(n.patch_slot);cell=(int(n.u_level),int(n.u_index),int(n.v_level),int(n.v_index))
        if not 0<=slot<len(fixtures) or any(level>12 for level in cell[::2]):
            issues.append("invalid cell/source");return issues,None
        if any(i<0 or i>=(1<<level) for level,i in ((cell[0],cell[1]),(cell[2],cell[3]))):
            issues.append("invalid dyadic index");return issues,None
        lower=tuple(float(v) for v in n.lower);upper=tuple(float(v) for v in n.upper)
        if any(not math.isfinite(v) for v in (*lower,*upper)):
            issues.append("nonfinite box");return issues,None
        lower,upper=tuple(map(ratio,lower)),tuple(map(ratio,upper));f=fixtures[slot]
        controls=restricted_controls(f,cell)
        points=tuple(tuple(c[k]/c[3] for k in range(3)) for c in controls)
        for k in range(3):
            if not Fraction(-1)<=lower[k]<=min(p[k] for p in points) or not max(p[k] for p in points)<=upper[k]<=1:
                issues.append(f"node {index} axis {k} misses exact restricted control hull")
        supports=();coefficients=()
        if mode==1 and len(bounds)==count*stride:
            supports=tuple((ratio(bounds[index*20+2*d]),ratio(bounds[index*20+2*d+1])) for d in range(10))
            for d,(direction,(lo,hi)) in enumerate(zip(DIRECTIONS,supports)):
                projected=tuple(sum(k*x for k,x in zip(direction,p)) for p in points)
                if lo>min(projected) or hi<max(projected) or lo>hi:issues.append(f"node {index} direction {d} misses exact hull")
        if mode==2 and len(bounds)==count*stride:
            coefficients=base_coefficients(f,cell);active=len(coefficients[4])
            for polynomial in range(5):
                for coefficient in range(49):
                    offset=index*490+polynomial*98+coefficient*2
                    lo,hi=map(ratio,bounds[offset:offset+2])
                    if coefficient>=active:
                        if lo!=0 or hi!=0:issues.append("Bernstein padding is not exact zero")
                    elif not lo<=coefficients[polynomial][coefficient]<=hi:
                        issues.append(f"node {index} polynomial {polynomial} coefficient {coefficient} misses exact value")
                    if coefficient<active and polynomial==4 and not 1<=lo<=hi<=65536:
                        issues.append("active denominator violates positive envelope")
        truth.append(NodeTruth(slot,cell,lower,upper,bool(int(n.flags)&1),supports,coefficients))
    for index,n in enumerate(nodes):
        if not int(n.flags)&1:
            for child in (int(n.left),int(n.right)):
                if not 0<=child<count:continue
                a,b=truth[index],truth[child]
                if any(x>y for x,y in zip(a.lower,b.lower)) or any(x<y for x,y in zip(a.upper,b.upper)):
                    issues.append("child AABB is not nested in parent")
                if mode==1 and any(pl>cl or ph<ch for (pl,ph),(cl,ch) in zip(a.supports,b.supports)):
                    issues.append("child support is not nested in parent")
    if any(not leaf for leaf in leaves):return issues,None
    return issues,HierarchyOracle(fixtures,tuple(truth),roots,tuple(tuple(x) for x in leaves),mode)


def qualify_row(case,values,status,flags,visits,pruned,refinements,evaluations,oracle,slot,*,check_flat_lower=True):
    """Exact certificates; callers with analytic truth can omit recipe-floor check.

    Cache/standalone conformance retains the flat method-specific lower check.
    A known-global caller instead checks its actual lower against analytic truth.
    """
    errors=[];metrics={}
    if len(values)!=8:return ["field count differs"],metrics
    if status not in (0,1):return ["valid discovery case failed status "+str(status)],metrics
    if not all(math.isfinite(x) for x in values):return ["nonfinite valid fields"],metrics
    if not all(0<=u<=1 for u in values[:2]):return ["infeasible returned UV"],metrics
    if not flags&CANDIDATE or flags&ARITHMETIC or flags&~63:errors.append("valid candidate/flags differ")
    if bool(flags&GAP)!=(status==0):errors.append("gap status and flag disagree")
    if bool(flags&COVERAGE)==bool(flags&VISIT_LIMIT):errors.append("coverage and visit-limit are not independent exhaustive outcomes")
    if case.status is not None and status!=case.status:errors.append("specified status differs")
    if flags&case.required_flags!=case.required_flags or flags&case.forbidden_flags:errors.append("specified flags differ")
    lower,upper=map(ratio,values[6:8]);point,distance=exact_distance(case.fixture,case.target,values[:2])
    if not 0<=lower<=upper:errors.append("invalid bracket")
    if upper<distance:errors.append("upper misses exact original-UV distance")
    if check_flat_lower:
        limit=oracle.lower_limit(slot,case.target,bool(flags&VISIT_LIMIT))
        if lower>limit:errors.append("lower exceeds independent flat/root bound")
    elif case.distance_answer is None:
        raise ValueError("omitting flat lower qualification requires exact analytic global truth")
    if case.distance_answer is not None:
        global_distance=ratio(case.distance_answer)
        if not lower<=global_distance<=upper:errors.append("bracket misses analytic global minimum")
        if status==0 and distance-global_distance>ratio(case.tolerance_squared):errors.append("gap success witness exceeds squared-objective tolerance")
    if status==0 and upper-lower>ratio(case.tolerance_squared):errors.append("gap success exceeds exact squared tolerance")
    for k,(observed,exact) in enumerate(zip(values[2:5],point)):
        error=abs(ratio(observed)-exact);metrics["point_"+str(k)]=error
        if error>ERROR_TARGETS[0]:errors.append("approximate XYZ exceeds evaluator allowance")
    allowance=sum(2*abs(p-ratio(x))*ERROR_TARGETS[0]+ERROR_TARGETS[0]**2 for p,x in zip(point,case.target)) \
              +ratio(64*math.ulp(max(1.,float(distance))))
    metrics["approximate_distance"]=abs(ratio(values[5])-distance)
    if metrics["approximate_distance"]>allowance:errors.append("descriptive distance exceeds allowance")
    if not 1<=visits<=case.max_visits or visits>2*len(oracle.leaves[slot])-1:errors.append("visit bound differs")
    if not 0<=pruned<=visits or not 0<=refinements<=case.max_refinements:errors.append("prune/refinement bound differs")
    if not 1+refinements<=evaluations<=1+833*refinements:errors.append("actual evaluator-count bounds differ")
    if refinements==0 and evaluations!=1:errors.append("zero-refinement evaluation count differs")
    if flags&REFINEMENT_LIMIT and refinements!=case.max_refinements:errors.append("refinement limit before budget exhaustion")
    for name,actual,expected in (("visits",visits,case.exact_visits),("refinements",refinements,case.exact_refinements),("evaluations",evaluations,case.exact_evaluations)):
        if expected is not None and actual!=expected:errors.append("specified "+name+" differs")
    return errors,metrics
