"""Original coefficient algebra shared by the existing saved-output readers.

No native cache/control net or emitted numerical certificate is an input.
"""
from bisect import bisect_left, bisect_right
from fractions import Fraction as F
from functools import lru_cache
from math import comb
import time


class OracleUnresolved(RuntimeError):
    pass


def add1(a, b):
    out = [F(0)] * max(len(a), len(b))
    for i, x in enumerate(a): out[i] += x
    for i, x in enumerate(b): out[i] += x
    while len(out) > 1 and not out[-1]: out.pop()
    return tuple(out)


def scale1(a, s):
    return tuple(x*s for x in a)


def mul1(a, b):
    out = [F(0)] * (len(a)+len(b)-1)
    for i, x in enumerate(a):
        for j, y in enumerate(b): out[i+j] += x*y
    return tuple(out)


def compose1(a, lo, hi):
    result = (F(0),)
    for x in reversed(a): result = add1(mul1(result,(lo,hi-lo)),(x,))
    return result


def eval1(a, t):
    value = F(0)
    for x in reversed(a): value = value*t+x
    return value


def derivative1(a):
    return tuple(i*x for i,x in enumerate(a[1:],1)) or (F(0),)


def bernstein1(a, degree=None):
    degree = len(a)-1 if degree is None else degree
    return tuple(sum(a[j]*F(comb(i,j),comb(degree,j)) for j in range(min(i,len(a)-1)+1))
                 for i in range(degree+1))


def split1(rows):
    pyramid = [rows]
    while len(pyramid[-1]) > 1:
        pyramid.append([tuple((x+y)/2 for x,y in zip(a,b))
                        for a,b in zip(pyramid[-1],pyramid[-1][1:])])
    return [r[0] for r in pyramid],[r[-1] for r in reversed(pyramid)]


def nonnegative_power(a, sign, max_nodes=4096):
    pending = [(tuple((sign*x,) for x in bernstein1(a)),0)]
    visits = 0
    while pending:
        rows,depth = pending.pop(); visits += 1
        if min(r[0] for r in rows) >= 0: continue
        if max(r[0] for r in rows) < 0:
            raise AssertionError('original scalar image is not monotone')
        if visits >= max_nodes or depth >= 20:
            raise OracleUnresolved('original scalar derivative sign unresolved')
        left,right = split1(rows)
        pending.extend(((left,depth+1),(right,depth+1)))


@lru_cache(maxsize=None)
def axis_spans(unique, multiplicities, p, count):
    """Cox level DP in each ORIGINAL normalized span's POWER basis."""
    knots = tuple(F(k) for k,m in zip(unique,multiplicities) for _ in range(m))
    assert len(unique) == len(multiplicities) and len(knots) == count+p+1
    assert 0 <= p <= 32 and knots[p] < knots[count]
    spans = []
    for span in range(p,count):
        a,b = knots[span:span+2]
        if a == b: continue
        level = {span:(F(1),)}
        for degree in range(1,p+1):
            next_level = {}
            for i in range(span-degree,span+1):
                value = (F(0),)
                if i in level and knots[i+degree] != knots[i]:
                    d = knots[i+degree]-knots[i]
                    value = add1(value,mul1(level[i],((a-knots[i])/d,(b-a)/d)))
                if i+1 in level and knots[i+degree+1] != knots[i+1]:
                    d = knots[i+degree+1]-knots[i+1]
                    value = add1(value,mul1(level[i+1],((knots[i+degree+1]-a)/d,-(b-a)/d)))
                if any(value): next_level[i] = value
            level = next_level
        spans.append((a,b,tuple((i,poly) for i,poly in sorted(level.items()) if 0 <= i < count)))
    return tuple(spans)


def _one_sided(a,b):
    assert a[-1] > 0 and b[-1] > 0
    return all(x*b[-1] == y*a[-1] for x,y in zip(a[:-1],b[:-1]))


def _interval_mul(a,b):
    p = [x*y for x in a for y in b]
    return min(p),max(p)


@lru_cache(maxsize=8192)
def original_trig(t):
    from native_adaptive_trimmed_caller import cylinder_trig
    try:
        return cylinder_trig.__wrapped__(t)
    except AssertionError as error:
        if 'unresolved' in str(error):
            raise OracleUnresolved(str(error)) from error
        raise


def enclose_expression(*weighted):
    """Exact common literal terms cancel BEFORE Taylor interval evaluation."""
    constant = [F(0)]*3; terms = {}
    for sign,(base,harmonics) in weighted:
        for axis in range(3): constant[axis] += sign*base[axis]
        for term in harmonics:
            if len(term) == 3:
                t,kind,coefficients = term; factors = ((t,kind),)
            else:
                factors,coefficients = term
            factors = tuple(sorted(factors))
            row = terms.setdefault(factors,[F(0)]*3)
            for axis in range(3): row[axis] += sign*coefficients[axis]
    lo,hi = list(constant),list(constant)
    for factors,row in terms.items():
        if not any(row): continue
        interval = (F(1),F(1))
        for t,kind in factors:
            sine,cosine = original_trig(t)
            interval = _interval_mul(interval,cosine if kind == 0 else sine)
        for axis in range(3):
            values = [row[axis]*x for x in interval]
            lo[axis] += min(values); hi[axis] += max(values)
    return tuple(zip(lo,hi))


def analytic_terms(carrier,uv):
    frame = tuple(map(F,carrier.frame)); u,v = uv
    origin = frame[:3]; x,y,z = (frame[k:k+3] for k in (3,6,9))
    def scaled(row,factor): return tuple(factor*a for a in row)
    if carrier.kind == 1:
        return tuple(origin[a]+u*x[a]+v*y[a] for a in range(3)),()
    terms = []
    if carrier.kind == 2:
        base = tuple(origin[a]+v*z[a] for a in range(3))
        terms = [(((u,0),),scaled(x,F(carrier.radii[0]))),
                 (((u,1),),scaled(y,F(carrier.radii[0])))]
    elif carrier.kind == 3:
        alpha = F(carrier.radii[1]); base = origin
        terms = [(((u,0),),scaled(x,F(carrier.radii[0]))),
                 (((u,1),),scaled(y,F(carrier.radii[0]))),
                 (((u,0),(alpha,1)),scaled(x,v)),
                 (((u,1),(alpha,1)),scaled(y,v)),(((alpha,0),),scaled(z,v))]
    else:
        assert carrier.kind in (4,5),'original analytic carrier unsupported'
        base = origin; minor = F(carrier.radii[0 if carrier.kind == 4 else 1])
        terms = [(((u,0),(v,0)),scaled(x,minor)),
                 (((u,1),(v,0)),scaled(y,minor)),(((v,1),),scaled(z,minor))]
        if carrier.kind == 5:
            terms.extend(((((u,0),),scaled(x,F(carrier.radii[0]))),
                          (((u,1),),scaled(y,F(carrier.radii[0])))))
    return base,tuple(terms)


def analytic_seconds(carrier,vs):
    xy = sum(abs(F(x)) for x in carrier.frame[3:9])
    z = sum(abs(F(x)) for x in carrier.frame[9:12])
    if carrier.kind == 1: return F(0),F(0),F(0)
    if carrier.kind == 2: return abs(F(carrier.radii[0]))*xy,F(0),F(0)
    if carrier.kind == 3:
        sa = original_trig(F(carrier.radii[1]))[0]
        r = max(abs(F(carrier.radii[0])+v*s) for v in vs for s in sa)
        return r*xy,max(map(abs,sa))*xy,F(0)
    if carrier.kind == 4:
        r = abs(F(carrier.radii[0])); mid = sum(vs)/2; width = (vs[1]-vs[0])/2
        sv,cv = original_trig(mid)
        return (r*xy*min(F(1),max(map(abs,cv))+width),
                r*xy*min(F(1),max(map(abs,sv))+width),r*(xy+z))
    assert carrier.kind == 5
    r,minor = map(lambda x:abs(F(x)),carrier.radii)
    return (r+minor)*xy,minor*xy,minor*(xy+z)


class OriginalSourceOracle:
    def __init__(self, source=None, deadline=None):
        self.source = source; self.deadline = deadline
        self.curves = {}; self.curve_ends = {}; self.surfaces = {}; self.surface_axes = {}
        self.curve_index = {} if source is None else {r.id:r for r in source.curves}
        self.carrier_index = {} if source is None else {r.id:r for r in source.carriers}
        self.face_index = {} if source is None else {r.id:r for r in source.faces}
        self.loop_index = {} if source is None else {r.id:r for r in source.loops}
        self.use_index = {} if source is None else {r.id:r for r in source.coedges}
        self.edge_index = {} if source is None else {r.id:r for r in source.edges}
        self.vertex_index = {} if source is None else {r.id:r for r in source.vertices}

    def check_time(self):
        if self.deadline is not None and time.monotonic() >= self.deadline:
            raise OracleUnresolved('original-output reader elapsed limit exhausted')

    def curve_roots(self,curve):
        self.check_time()
        if curve.id in self.curves: return self.curves[curve.id]
        assert curve.kind in (4,5) and 1 <= curve.degree <= 32
        stride = curve.dimension+1; count = len(curve.controls)//stride
        assert len(curve.controls) == count*stride and all(F(curve.controls[i*stride+curve.dimension])>0 for i in range(count))
        knots,mult = ((0.,1.),(curve.degree+1,curve.degree+1)) if curve.kind == 4 else (curve.knots,curve.multiplicities)
        roots = []
        for a,b,basis in axis_spans(knots,mult,curve.degree,count):
            channels = []
            for channel in range(stride):
                poly = (F(0),)
                for i,p in basis: poly = add1(poly,scale1(p,F(curve.controls[i*stride+channel])))
                channels.append(poly)
            roots.append((a,b,tuple(channels)))
        for left,right in zip(roots,roots[1:]):
            assert left[1] == right[0] and _one_sided(
                [eval1(c,F(1)) for c in left[2]],[eval1(c,F(0)) for c in right[2]]), 'original curve is not C0'
        self.curves[curve.id] = tuple(roots)
        self.curve_ends[curve.id] = tuple(r[1] for r in roots)
        return self.curves[curve.id]

    def curve_value(self,curve,t):
        roots = self.curve_roots(curve)
        i = bisect_left(self.curve_ends[curve.id],t)
        assert i < len(roots) and roots[i][0] <= t <= roots[i][1], 'original curve t outside natural domain'
        a,b,channels = roots[i]; h = [eval1(p,(t-a)/(b-a)) for p in channels]
        assert h[-1] > 0
        values = tuple(x/h[-1] for x in h[:-1])
        return values+(F(0),)*(3-len(values))

    def curve_terms(self,curve,t):
        if curve.kind in (4,5): return self.curve_value(curve,t),()
        assert curve.kind in (1,2,3)
        frame = tuple(map(F,curve.frame))
        if curve.kind == 1:
            return tuple(frame[a]+t*frame[3+a] for a in range(3)),()
        return frame[:3],((t,0,tuple(F(curve.radii[0])*frame[3+a] for a in range(3))),
                         (t,1,tuple(F(curve.radii[1])*frame[6+a] for a in range(3))))

    def surface_roots(self,carrier):
        self.check_time()
        if carrier.id in self.surfaces: return self.surfaces[carrier.id]
        assert carrier.kind == 6 and max(carrier.degrees) <= 3
        p,q = carrier.degrees; nu,nv = carrier.control_shape
        assert len(carrier.controls) == nu*nv*4 and all(F(carrier.controls[4*i+3])>0 for i in range(nu*nv))
        us = axis_spans(carrier.knot_u,carrier.mult_u,p,nu)
        vs = axis_spans(carrier.knot_v,carrier.mult_v,q,nv)
        roots = []
        for ua,ub,bu in us:
            for va,vb,bv in vs:
                channels = [dict() for _ in range(4)]
                for i,upi in bu:
                    for j,vpj in bv:
                        for a,x in enumerate(upi):
                            for b,y in enumerate(vpj):
                                for axis in range(4):
                                    h = F(carrier.controls[(i*nv+j)*4+axis])
                                    channels[axis][a,b] = channels[axis].get((a,b),F(0))+x*y*h
                roots.append(((ua,ub,va,vb),tuple(channels)))
        self.surfaces[carrier.id] = tuple(roots)
        self.surface_axes[carrier.id] = (tuple(r[1] for r in us),tuple(r[1] for r in vs),len(vs))
        # Closed source sectors agree along complete original edges, not samples.
        for i,(box,channels) in enumerate(roots):
            for delta,axis in ((len(vs),0),(1,1)):
                if (axis == 0 and i//len(vs)+1 >= len(us)) or (axis == 1 and i%len(vs)+1 >= len(vs)): continue
                other = roots[i+delta][1]
                def edge(polys,value):
                    out = []
                    for poly in polys:
                        result = (F(0),)
                        for (a,b),x in poly.items():
                            exponent,remaining = (a,b) if axis == 0 else (b,a)
                            row = (F(0),)*remaining+(x*value**exponent,)
                            result = add1(result,row)
                        out.append(result)
                    return out
                left,right = edge(channels,F(1)),edge(other,F(0))
                assert all(not any(add1(mul1(left[a],right[3]),scale1(mul1(right[a],left[3]),-1)))
                           for a in range(3)), 'original carrier is not C0'
        return self.surfaces[carrier.id]

    def surface_root(self,carrier,uv):
        roots = self.surface_roots(carrier); us,vs,nv = self.surface_axes[carrier.id]
        iu,iv = bisect_left(us,uv[0]),bisect_left(vs,uv[1])
        assert iu < len(us) and iv < len(vs), 'original UV outside natural rectangle'
        root = roots[iu*nv+iv]; box = root[0]
        assert all(box[2*a] <= uv[a] <= box[2*a+1] for a in range(2)), 'original UV outside natural rectangle'
        return root

    def carrier_terms(self,carrier,uv):
        if carrier.kind != 6: return analytic_terms(carrier,uv)
        box,channels = self.surface_root(carrier,uv)
        u,v = ((uv[a]-box[2*a])/(box[2*a+1]-box[2*a]) for a in range(2))
        h = [sum(x*u**a*v**b for (a,b),x in poly.items()) for poly in channels]
        assert h[3] > 0
        return tuple(x/h[3] for x in h[:3]),()

    def curve_segments(self,curve,a,b):
        assert a != b
        roots = self.curve_roots(curve); direction = 1 if b>a else -1
        first = bisect_right(self.curve_ends[curve.id],min(a,b))
        last = bisect_left(self.curve_ends[curve.id],max(a,b))
        selected = [(max(min(a,b),lo),min(max(a,b),hi),(lo,hi,channels))
                    for lo,hi,channels in roots[first:last+1] if max(min(a,b),lo)<min(max(a,b),hi)]
        assert selected and selected[0][0] == min(a,b) and selected[-1][1] == max(a,b)
        if direction < 0: selected.reverse()
        for low,high,(lo,hi,channels) in selected:
            x,y = (low,high) if direction > 0 else (high,low)
            yield x,y,tuple(compose1(p,(x-lo)/(hi-lo),(y-lo)/(hi-lo)) for p in channels)

    def prove_curve_chord(self,curve,ts,qa,qb,bound):
        a,b = ts; visits = 0
        for x,y,channels in self.curve_segments(curve,a,b):
            points = [tuple(qa[j]+(t-a)/(b-a)*(qb[j]-qa[j]) for j in range(3)) for t in (x,y)]
            self.prove_power_chord(channels,points[0],points[1],bound)
            visits += 1
        return visits

    @staticmethod
    def prove_power_chord(channels,qa,qb,bound):
        assert bound >= 0
        residual = [add1(channels[a],scale1(mul1(channels[-1],(qa[a],qb[a]-qa[a])),-1)) for a in range(3)]
        degree = max(map(len,(*residual,channels[-1])))-1
        rows = list(zip(*(bernstein1(p,degree) for p in (*residual,channels[-1]))))
        pending = [(rows,0)]; visits = 0
        while pending:
            rows,depth = pending.pop(); visits += 1
            lower = min(row[3] for row in rows)
            assert lower > 0,'original source denominator not positive'
            if all(sum(x*x for x in row[:3]) <= bound*bound*lower*lower for row in rows): continue
            if visits >= 4096 or depth >= 18:
                raise OracleUnresolved('whole original chord bound unresolved')
            left,right = split1(rows)
            pending.extend(((left,depth+1),(right,depth+1)))

    def prove_line_image(self,curve,ts,qa,qb,bound):
        assert bound >= 0
        a,b = ts; pa,pb = (self.curve_value(curve,t) for t in ts)
        d = tuple(y-x for x,y in zip(pa,pb)); axis = next((j for j,x in enumerate(d) if x),None)
        assert axis is not None,'constant source image unsupported'
        for x,y,channels in self.curve_segments(curve,a,b):
            for j in range(3):
                left = scale1(add1(channels[j],scale1(channels[-1],-pa[j])),d[axis])
                right = scale1(add1(channels[axis],scale1(channels[-1],-pa[axis])),d[j])
                assert not any(add1(left,scale1(right,-1))),'original source image is not a line'
            p,w = channels[axis],channels[-1]
            derivative = add1(mul1(derivative1(p),w),scale1(mul1(p,derivative1(w)),-1))
            assert any(derivative),'constant source subinterval is not strictly monotone'
            nonnegative_power(derivative,1 if d[axis]>0 else -1)
        assert max(sum((p[j]-q[j])**2 for j in range(3)) for p,q in ((pa,qa),(pb,qb))) <= bound*bound

    def prove_facet(self,carrier,uv,xyz,upper):
        assert upper >= 0
        if carrier.kind != 6:
            return self.prove_analytic_facet(carrier,uv,xyz,upper)
        from native_adaptive_trimmed_caller import orient
        roots = self.surface_roots(carrier)
        for point in uv: self.surface_root(carrier,point)
        center = tuple(sum(p[a] for p in uv)/3 for a in range(2))
        single = self.surface_root(carrier,center)
        if all(single[0][2*a] <= p[a] <= single[0][2*a+1] for p in uv for a in range(2)):
            candidates = [single]
        else:
            us,vs,nv = self.surface_axes[carrier.id]
            ranges = [(bisect_right(ends,min(p[a] for p in uv)),bisect_left(ends,max(p[a] for p in uv))) for a,ends in enumerate((us,vs))]
            candidates = [roots[i*nv+j] for i in range(ranges[0][0],ranges[0][1]+1)
                          for j in range(ranges[1][0],ranges[1][1]+1)]
        covered = False; work = [0]
        for box,channels in candidates:
            if any(max(p[a] for p in uv) <= box[2*a] or min(p[a] for p in uv) >= box[2*a+1] for a in range(2)): continue
            polygon = list(zip(uv,xyz))
            for side,bound in enumerate(box):
                axis,sign = side//2,1 if side%2 == 0 else -1; result = []
                for x,y in zip(polygon,polygon[1:]+polygon[:1]):
                    da,db = x[0][axis]-bound,y[0][axis]-bound
                    ina,inb = sign*da>=0,sign*db>=0
                    if ina: result.append(x)
                    if ina != inb:
                        t = da/(da-db)
                        result.append(tuple(tuple(a+t*(b-a) for a,b in zip(p,q)) for p,q in zip(x,y)))
                polygon = result
            for j in range(1,len(polygon)-1):
                piece = [polygon[k] for k in (0,j,j+1)]
                if orient(*(p[0] for p in piece)) > 0:
                    covered = True
                    self._prove_rational(box,channels,[p[0] for p in piece],[p[1] for p in piece],upper,0,work)
        assert covered,'original facet has no positive original span support'

    def _prove_rational(self,box,channels,uv,xyz,upper,depth,work):
        from native_adaptive_trimmed_caller import add,mul,scale,bernstein
        self.check_time(); work[0] += 1
        coordinates = [{(0,0):(uv[0][a]-box[2*a])/(box[2*a+1]-box[2*a]),
                        (1,0):(uv[1][a]-uv[0][a])/(box[2*a+1]-box[2*a]),
                        (0,1):(uv[2][a]-uv[0][a])/(box[2*a+1]-box[2*a])} for a in range(2)]
        powers = [[{(0,0):F(1)}] for _ in range(2)]
        for axis in range(2):
            for _ in range(3): powers[axis].append(mul(powers[axis][-1],coordinates[axis]))
        composed = []
        for poly in channels:
            result = {}
            for (a,b),x in poly.items(): result = add(result,scale(mul(powers[0][a],powers[1][b]),x))
            composed.append(result)
        residual = []
        for a in range(3):
            affine = {(0,0):xyz[0][a],(1,0):xyz[1][a]-xyz[0][a],(0,1):xyz[2][a]-xyz[0][a]}
            residual.append(add(composed[a],scale(mul(composed[3],affine),-1)))
        degree = max((sum(k) for p in (*residual,composed[3]) for k in p),default=0)
        weights = bernstein(composed[3],degree); lower = min(weights)
        assert lower > 0,'original triangle denominator not positive'
        coefficients = list(zip(*(bernstein(p,degree) for p in residual)))
        if all(sum(x*x for x in row) <= upper*upper*lower*lower for row in coefficients): return
        if depth >= 18 or work[0] >= 4096:
            raise OracleUnresolved('whole original rational facet unresolved')
        self._subdivide_facet(lambda p,q:self._prove_rational(box,channels,p,q,upper,depth+1,work),uv,xyz)

    @staticmethod
    def _subdivide_facet(prove,uv,xyz):
        p = [*uv,*(tuple((uv[i][a]+uv[j][a])/2 for a in range(2)) for i,j in ((0,1),(1,2),(2,0)))]
        q = [*xyz,*(tuple((xyz[i][a]+xyz[j][a])/2 for a in range(3)) for i,j in ((0,1),(1,2),(2,0)))]
        for ids in ((0,3,5),(3,1,4),(5,4,2),(3,4,5)): prove([p[i] for i in ids],[q[i] for i in ids])

    def prove_analytic_facet(self,carrier,uv,xyz,upper,depth=0,work=None):
        self.check_time(); work = [0] if work is None else work; work[0] += 1
        ru = max(p[0] for p in uv)-min(p[0] for p in uv); rv = max(p[1] for p in uv)-min(p[1] for p in uv)
        muu,muv,mvv = analytic_seconds(carrier,(min(p[1] for p in uv),max(p[1] for p in uv)))
        curvature = muu*ru*ru/8+muv*ru*rv/4+mvv*rv*rv/8
        boxes = [enclose_expression((1,self.carrier_terms(carrier,p)),(-1,(q,()))) for p,q in zip(uv,xyz)]
        if curvature <= upper and max(sum(max(abs(a),abs(b))**2 for a,b in box) for box in boxes) <= (upper-curvature)**2: return
        if depth >= 18 or work[0] >= 4096: raise OracleUnresolved('whole original analytic facet unresolved')
        self._subdivide_facet(lambda p,q:self.prove_analytic_facet(carrier,p,q,upper,depth+1,work),uv,xyz)

    def prove_trace(self,carrier,uv,qa,qb,upper):
        assert upper >= 0
        if carrier.kind != 6:
            du,dv = (uv[1][a]-uv[0][a] for a in range(2))
            muu,muv,mvv = analytic_seconds(carrier,tuple(sorted((uv[0][1],uv[1][1]))))
            curvature = (muu*du*du+2*muv*abs(du*dv)+mvv*dv*dv)/8
            self._analytic_chord(lambda t:self.carrier_terms(carrier,tuple(uv[0][a]+t*(uv[1][a]-uv[0][a]) for a in range(2))),curvature,qa,qb,upper)
            return
        roots = self.surface_roots(carrier); cuts = {F(0),F(1)}
        for point in uv: self.surface_root(carrier,point)
        us,vs,_ = self.surface_axes[carrier.id]
        for axis,ends in enumerate((us,vs)):
            if uv[1][axis] != uv[0][axis]:
                for bound in ends:
                    t = (bound-uv[0][axis])/(uv[1][axis]-uv[0][axis])
                    if 0<t<1: cuts.add(t)
        cuts = sorted(cuts)
        for lo,hi in zip(cuts,cuts[1:]):
            point = tuple(uv[0][a]+(lo+hi)/2*(uv[1][a]-uv[0][a]) for a in range(2))
            box,channels = self.surface_root(carrier,point)
            normalized = [((uv[0][a]+lo*(uv[1][a]-uv[0][a])-box[2*a])/(box[2*a+1]-box[2*a]),
                           (uv[0][a]+hi*(uv[1][a]-uv[0][a])-box[2*a])/(box[2*a+1]-box[2*a])) for a in range(2)]
            powers = [[(F(1),)] for _ in range(2)]
            for a in range(2):
                for _ in range(3): powers[a].append(mul1(powers[a][-1],(normalized[a][0],normalized[a][1]-normalized[a][0])))
            composed = []
            for poly in channels:
                result = (F(0),)
                for (a,b),x in poly.items(): result = add1(result,scale1(mul1(powers[0][a],powers[1][b]),x))
                composed.append(result)
            actual = [tuple(qa[a]+t*(qb[a]-qa[a]) for a in range(3)) for t in (lo,hi)]
            self.prove_power_chord(composed,*actual,upper)

    def _analytic_chord(self,evaluate,curvature,qa,qb,upper,lo=F(0),hi=F(1),depth=0,work=None):
        self.check_time(); work = [0] if work is None else work; work[0] += 1
        boxes = [enclose_expression((1,evaluate(t)),(-1,(q,()))) for t,q in ((lo,qa),(hi,qb))]
        if curvature<=upper and max(sum(max(abs(a),abs(b))**2 for a,b in box) for box in boxes) <= (upper-curvature)**2: return
        if depth >= 18 or work[0] >= 4096: raise OracleUnresolved('whole original analytic chord unresolved')
        mid=(lo+hi)/2; q=tuple((a+b)/2 for a,b in zip(qa,qb))
        self._analytic_chord(evaluate,curvature/4,qa,q,upper,lo,mid,depth+1,work)
        self._analytic_chord(evaluate,curvature/4,q,qb,upper,mid,hi,depth+1,work)


@lru_cache(maxsize=64)
def _standalone_curve_oracle(curve):
    return OriginalSourceOracle()


def original_curve_net(curve,a,b):
    pieces = list(_standalone_curve_oracle(curve).curve_segments(curve,a,b))
    assert len(pieces) == 1,'physical chord crosses original knot'
    channels = pieces[0][2]
    return list(zip(*(bernstein1(p,curve.degree) for p in channels)))
