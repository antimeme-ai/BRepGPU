"""Exact supplied-input geometry and analytic projection expectations.

No iterative reference solver: expectations use closed analytic fixtures and
exact geometry at the actual returned binary64 parameters.
"""
from dataclasses import dataclass
from fractions import Fraction
import math

from .fixtures import (Fixture, ERROR_TARGETS, exact_jet, plane,
                       rational_cylinder, qualification_fixtures)

E0, E1 = ERROR_TARGETS[:2]


@dataclass(frozen=True)
class ProjectionCase:
    name: str
    fixture: Fixture
    target: tuple
    seed: tuple
    status: object = 0
    curvature: object = 16
    answer: object = None
    distance_answer: object = None
    rank: int = 0
    max_iterations: int = 64
    max_backtracks: int = 24
    tolerance: float = 2.0**-24
    exact_updates: object = None
    exact_evaluations: object = None
    eigenvalue: float = 1.0


@dataclass(frozen=True)
class Geometry:
    point: tuple
    distance_squared: Fraction
    gradient: tuple
    metric: tuple
    hessian: tuple
    residual: Fraction
    distance_allowance: Fraction
    residual_allowance: Fraction


def ratio(value):
    return value if isinstance(value,Fraction) else Fraction.from_float(float(value))


def dot(a,b):return sum(x*y for x,y in zip(a,b))


def clip(value):return max(Fraction(0),min(Fraction(1),value))


def ulp_budget(value):
    return ratio(64*math.ulp(max(1.0,float(abs(value)))))


def geometry(fixture,target,uv):
    jets=exact_jet(fixture,*uv)
    r=tuple(p-ratio(x) for p,x in zip(jets[0],target))
    gradient=tuple(dot(r,d) for d in jets[1:3])
    metric=((dot(jets[1],jets[1]),dot(jets[1],jets[2])),
            (dot(jets[1],jets[2]),dot(jets[2],jets[2])))
    hessian=((metric[0][0]+dot(r,jets[3]),metric[0][1]+dot(r,jets[4])),
             (metric[1][0]+dot(r,jets[4]),metric[1][1]+dot(r,jets[5])))
    rho=max(abs(ratio(u)-clip(ratio(u)-g)) for u,g in zip(uv,gradient))
    distance=dot(r,r)
    distance_budget=sum(2*abs(x)*E0+E0*E0 for x in r)+ulp_budget(distance)
    gradient_budgets=tuple(
        sum(abs(x)*E1+abs(d)*E0+E0*E1 for x,d in zip(r,derivative))
        +ulp_budget(sum(abs(x*d) for x,d in zip(r,derivative)))
        for derivative in jets[1:3])
    return Geometry(jets[0],distance,gradient,metric,hessian,rho,
                    distance_budget,max(gradient_budgets)+ulp_budget(1))


def orthogonal_plane():
    return Fixture("orthogonal_plane",1,1,tuple(float(v) for i in (0,1) for j in (0,1)
                                              for v in (i,j,0,1)))


def extruded(name,x,z,degree):
    return Fixture(name,degree,1,tuple(float(v) for a,b in zip(x,z) for t in (0,1)
                                     for v in (a,t,b,1)))


def parabolic():return extruded("parabolic_cylinder",(-1,0,1),(1,-1,1),2)
def weak_parabolic():return extruded("weak_boundary_parabola",(0,.5,1),(0,0,1),2)
def cubic():return extruded("cubic_extrusion",(-1,1,-1,1),(0,0,0,0),3)


def conditioned(epsilon):
    return Fixture("conditioned_"+float(epsilon).hex(),1,1,
                   tuple(float(v) for s in (0,1) for t in (0,1)
                         for v in (s,epsilon*t,0,1)))


def normal_parabolic_cases():
    result=[]
    for index,a in enumerate((-.5,-.25,.25,.5)):
        for t in (.25,.75):
            lam=1/16
            target=(a-2*a*lam,t,a*a+lam)
            result.append(ProjectionCase(f"parabolic_normal_{a}_{t}",parabolic(),target,
                (.125 if index%2 else .875,1-t),answer=((a+1)/2,t),
                distance_answer=ratio(lam)**2*(4*ratio(a)**2+1)))
    return tuple(result)


def projection_cases():
    cases=[]
    p=orthogonal_plane()
    for a in (-.25,.25,1.25):
        for b in (-.25,.75,1.25):
            uv=(max(0,min(1,a)),max(0,min(1,b)))
            distance=(ratio(uv[0])-ratio(a))**2+(ratio(uv[1])-ratio(b))**2+Fraction(1,4)
            cases.append(ProjectionCase(f"plane_{a}_{b}",p,(a,b,.5),(.75,.25),
                                        answer=uv,distance_answer=distance))
    cases.append(ProjectionCase("tilted_coupled_boundary",plane(),(-.25,.5,.25),(.75,.25),
                                answer=(0.,.5),distance_answer=Fraction(1,16)))
    c=rational_cylinder()
    for seed in ((.125,.875),(.875,.125),(.5,.25)):
        initial=seed==(.5,.25)
        cases.append(ProjectionCase("radial_cylinder_"+str(seed),c,(.75,1.,.25),seed,
                     answer=(.5,.25),distance_answer=Fraction(1,16),
                     exact_updates=0 if initial else None,exact_evaluations=1 if initial else None))
    cases += [ProjectionCase("cylinder_lower",c,(1.5,-.25,.5),(.75,.25),answer=(0.,.5),distance_answer=Fraction(5,16)),
              ProjectionCase("cylinder_upper_corner",c,(-.25,1.5,1.25),(.25,.25),answer=(1.,1.),distance_answer=Fraction(3,8))]
    for s,dist,curvature in ((0.,Fraction(65,16),16),(1.,Fraction(73,16),16),(.5,Fraction(81,16),32)):
        cases.append(ProjectionCase("negative_cylinder_"+str(s),c,(-.75,-1.,.25),(s,.25),
                     curvature=curvature,answer=(s,.25),distance_answer=dist,exact_updates=0,exact_evaluations=1))
    cases.append(ProjectionCase("cylinder_axis_continuum",c,(0.,0.,.5),(.25,.5),curvature=64,
                                answer=(.25,.5),distance_answer=1,exact_updates=0,exact_evaluations=1))
    for s,dist,curvature in ((.25,Fraction(1,2),16),(.75,Fraction(1,2),16),(.5,Fraction(9,16),32)):
        cases.append(ProjectionCase("parabolic_stationary_"+str(s),parabolic(),(0.,.5,.75),(s,.5),
                     curvature=curvature,answer=(s,.5),distance_answer=dist,exact_updates=0,exact_evaluations=1))
    cases += [ProjectionCase("quartic_flat",parabolic(),(0.,.5,.5),(.5,.5),curvature=64,
                             answer=(.5,.5),distance_answer=Fraction(1,4),exact_updates=0,exact_evaluations=1),
              ProjectionCase("weak_boundary_saddle",weak_parabolic(),(0.,.5,.75),(0.,.5),curvature=32,
                             answer=(0.,.5),distance_answer=Fraction(9,16),exact_updates=0,exact_evaluations=1),
              ProjectionCase("weak_positive_multiplier",weak_parabolic(),(-2.0**-25,.5,.75),(0.,.5),curvature=32,
                             answer=(0.,.5),distance_answer=Fraction(9,16)+Fraction(1,1<<50),exact_updates=0,exact_evaluations=1)]
    corner=Fixture("weak_corner",1,1,tuple(float(v) for i in (0,1) for j in (0,1) for v in (i,j,i*j,1)))
    cases.append(ProjectionCase("forbidden_corner_eigenvector",corner,(0.,0.,-2.),(0.,0.),curvature=64,
                                answer=(0.,0.),distance_answer=4,exact_updates=0,exact_evaluations=1))
    a=2.0**-10
    threshold=Fixture("curvature_equality",1,1,(-1.,0.,0.,1., 0.,-a,0.,1., 0.,a,0.,1., 1.,0.,0.,1.))
    cases.append(ProjectionCase("curvature_threshold_equality",threshold,(0.,0.,0.),(.5,.5),curvature=64,
                                answer=(.5,.5),distance_answer=0,exact_updates=0,exact_evaluations=1))
    epsilon=2.0**-19
    cases.append(ProjectionCase("admitted_non_nearest_seed",conditioned(epsilon),(.25,epsilon/2,.25),(.25,.25),
                                curvature=64,exact_updates=0,exact_evaluations=1))
    epsilon=2.0**-21
    cases.append(ProjectionCase("positive_metric_below_floor",conditioned(epsilon),(.25,epsilon/2,.25),(.25,.25),
                                status=8,curvature=None,rank=8,exact_updates=0,exact_evaluations=1))
    epsilon=2.0**-20
    cases.append(ProjectionCase("positive_metric_at_floor",conditioned(epsilon),(.5,epsilon/2,1.),(.5,.5),
                                status=8,curvature=None,rank=8,exact_updates=0,exact_evaluations=1))
    singular=extruded("singular_at_lower_bound",(0,0,1),(0,0,0),2)
    fold=extruded("folded_plane",(1,-1,1),(0,0,0),2)
    line=Fixture("rank_one_carrier",1,0,(0.,0.,0.,1.,1.,0.,0.,1.))
    constant=Fixture("constant_carrier",0,0,(.25,.5,.75,1.))
    for name,f,target,seed in (("singular_answer",singular,(0.,.5,.25),(0.,.5)),
                               ("singular_fold",fold,(.25,.5,.25),(.5,.5)),
                               ("rank_one",line,(.5,.25,.25),(.5,.75)),
                               ("rank_zero",constant,(0.,0.,0.),(.5,.5))):
        cases.append(ProjectionCase(name,f,target,seed,status=8,curvature=None,rank=4,
                                    exact_updates=0,exact_evaluations=1))
    cases += [ProjectionCase("zero_updates_unresolved",p,(.25,.75,.5),(.75,.25),status=5,
                             curvature=None,max_iterations=0,exact_updates=0,exact_evaluations=1),
              ProjectionCase("zero_updates_stationary",p,(.25,.75,.5),(.25,.75),max_iterations=0,
                             answer=(.25,.75),distance_answer=Fraction(1,4),exact_updates=0,exact_evaluations=1),
              ProjectionCase("stable_distance_change",p,(.5,.5,1.),(.5+2.0**-30,.5),tolerance=2.0**-32,
                             answer=(.5,.5),distance_answer=1,exact_updates=1,exact_evaluations=3),
              ProjectionCase("cubic_exhausted_search",cubic(),(2.0**-10,.5,0.),(33/64,.5),status=6,
                             curvature=None,max_backtracks=0,exact_updates=0,exact_evaluations=2)]
    cases.extend(normal_parabolic_cases())
    # General rational/control-degree cases check the actual returned geometry,
    # first-order predicate and retained limits without predicting a basin.
    for f in qualification_fixtures():
        cases.append(ProjectionCase("general_"+f.name,f,(.125,-.375,.75),(.25,.75),
                                    status=None,curvature=None))
    for name,target,seed,status in (
        ("bad_slot_precedence",(math.nan,0.,0.),(math.nan,.5),1),
        ("bad_seed_nan",(.25,.75,.5),(math.nan,.5),2),
        ("bad_seed_below",(.25,.75,.5),(math.nextafter(0.,-math.inf),.5),2),
        ("bad_target_wide",(math.nextafter(4.,math.inf),0.,0.),(.5,.5),2),
        ("bad_target_nonfinite",(math.inf,0.,0.),(.5,.5),2)):
        cases.append(ProjectionCase(name,p,target,seed,status=status,curvature=None))
    return tuple(cases)


def qualify_row(case,values,status,updates,evaluations,flags):
    """Return direct violations and scalar witness maxima for one public row."""
    failures=[]
    metrics={}
    def require(condition,message):
        if not condition:failures.append(message)
    if case.status is not None:require(status==case.status,f"status {status}, expected {case.status}")
    if case.status in (1,2):
        require(all(math.isnan(x) for x in values),"invalid request fields are not all NaN")
        require(flags==0 and updates==evaluations==0,"invalid request flags/counters are not zero")
        return failures,metrics
    if status in (1,2,3,4):
        require(all(math.isnan(x) for x in values),"failed numerical fields are not all NaN")
        require(flags==0,"invalid/arithmetic flags not zero")
        if status in (1,2):require(updates==evaluations==0,"invalid row counters not zero")
        require(case.status in (1,2,3,4),"unexpected invalid/arithmetic result for valid fixture")
        return failures,metrics
    require(status in (0,5,6,7,8),"unknown status")
    require(all(math.isfinite(x) for x in values),"candidate has nonfinite fields")
    if not all(math.isfinite(x) for x in values):return failures,metrics
    s,t,*tail=values
    require(0<=s<=1 and 0<=t<=1,"candidate outside rectangle")
    if not (0<=s<=1 and 0<=t<=1):return failures,metrics
    truth=geometry(case.fixture,case.target,(s,t))
    seed_truth=geometry(case.fixture,case.target,case.seed)
    for axis in range(3):
        error=abs(ratio(tail[axis])-truth.point[axis])
        metrics["point_"+str(axis)]=error
        require(error<=E0,f"point component {axis} exceeds E0")
    d_error=abs(ratio(tail[3])-truth.distance_squared)
    r_error=abs(ratio(tail[4])-truth.residual)
    metrics["distance"]=d_error;metrics["residual"]=r_error
    require(d_error<=truth.distance_allowance,"distance disagrees with exact supplied-input geometry")
    require(r_error<=truth.residual_allowance,"rho disagrees with exact box predicate")
    require(tail[3]>=0 and 0<=tail[4]<=1,"distance/rho outside their nonnegative ranges")
    require(truth.distance_squared<=seed_truth.distance_squared+truth.distance_allowance+seed_truth.distance_allowance,
            "final exact distance exceeds seed beyond corpus budget")
    require(flags&1!=0 and flags&~127==0,"candidate flag missing or unknown flags set")
    require(bool(flags&2)==(s in (0,1) or t in (0,1)),"boundary flag disagrees with exact returned UV")
    require(0<=updates<=case.max_iterations,"accepted updates exceed option")
    require(1+updates<=evaluations<=1+case.max_iterations*(case.max_backtracks+2),
            "evaluation counters outside necessary work bounds")
    require(evaluations<=3329,"global evaluation bound exceeded")
    if case.exact_updates is not None:require(updates==case.exact_updates,"unexpected exact-case update count")
    if case.exact_evaluations is not None:require(evaluations==case.exact_evaluations,"unexpected exact-case evaluation count")
    curvature=flags&112
    if status==0:
        require(curvature in (16,32,64),"stationary row lacks exactly one curvature bit")
        require(flags&12==0,"stationary row has rank flags")
        require(tail[4]<=case.tolerance,"reported stationary rho exceeds requested tolerance")
        require(truth.residual<=ratio(case.tolerance)+truth.residual_allowance,"exact stationary rho exceeds corpus allowance")
        if case.curvature is not None:require(curvature==case.curvature,"curvature differs from exact fixture")
    else:
        require(curvature==0,"unresolved row has curvature bits")
        if status==8:require(flags&12 in (4,8),"rank refusal lacks exactly one rank flag")
        else:require(flags&12==0,"non-rank solver status has rank bits")
        if case.rank:require(flags&12==case.rank,"rank subtype differs from exact fixture")
    if case.answer is not None:
        minimum=min(case.eigenvalue,1.0)
        uv_budget=8*(ratio(case.tolerance)+truth.residual_allowance)/ratio(minimum)+ulp_budget(1)
        for axis,expected in enumerate(case.answer):
            require(abs(ratio(values[axis])-ratio(expected))<=uv_budget,"UV differs from conditioned analytic answer")
        if case.distance_answer is not None:
            # Conservative map propagation over these bounded, low-degree
            # named basins: |Ss|+|St| <= 8, residual components <= 5.
            spatial_budget=8*uv_budget
            known_budget=30*spatial_budget+3*spatial_budget**2+truth.distance_allowance
            require(abs(ratio(tail[3])-ratio(case.distance_answer))<=known_budget,
                    "distance differs from analytic answer")
    if updates==0:
        require((s,t)==case.seed,"zero-update candidate did not retain exact seed")
    return failures,metrics
