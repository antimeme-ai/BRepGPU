"""Complete source-domain and boundary-word truth for bounded arrangements.

Expected regions come from input cell-minus-disks/disk domains, not product
cycle extraction, private caches, reader results or Euler characteristics.
"""
from collections import Counter
from fractions import Fraction as F
import math


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def exact(value):
    if type(value) is float:
        require(math.isfinite(value), "nonfinite oracle input")
        return F.from_float(value)
    require(type(value) in (int, F), "unsupported oracle input type")
    return F(value)


def rational_tuple(value, count, label):
    require(type(value) is tuple and len(value) == count and all(type(x) is F for x in value),
            label+" is not an owned exact rational tuple")


def _target(face):
    body, record = face.snapshot, face.face
    require(record.kind == "rectangle" and len(record.chart_slots) == 1, "original target is not one rectangle")
    patch = body.charts[record.chart_slots[0]].patch
    require((patch.degree_u, patch.degree_v) == (1, 1) and len(patch.controls) == 16,
            "original target control shape differs")
    controls = tuple(tuple(exact(x) for x in patch.controls[i:i+4]) for i in range(0,16,4))
    require(all(row[3] == 1 for row in controls), "original target weights are not unit affine")
    origin = tuple(exact(o)+exact(patch.scale)*x for o,x in zip(patch.origin,controls[0][:3]))
    u = tuple(exact(patch.scale)*(b-a) for a,b in zip(controls[0][:3],controls[2][:3]))
    v = tuple(exact(patch.scale)*(b-a) for a,b in zip(controls[0][:3],controls[1][:3]))
    require(all(controls[3][i] == controls[2][i]+controls[1][i]-controls[0][i] for i in range(3)),
            "original target fourth corner is not affine")
    ua, va = tuple(i for i,x in enumerate(u) if x), tuple(i for i,x in enumerate(v) if x)
    require(len(ua) == len(va) == 1 and ua != va, "original target axes differ")
    return dict(origin=origin,u=u,v=v,ua=ua[0],va=va[0])


def _world(target, uv):
    return tuple(o+u*uv[0]+v*uv[1] for o,u,v in zip(target["origin"],target["u"],target["v"]))


def _uv(target, point):
    ua, va = target["ua"], target["va"]
    answer = ((point[ua]-target["origin"][ua])/target["u"][ua],
              (point[va]-target["origin"][va])/target["v"][va])
    require(_world(target,answer) == tuple(point), "source world point lies off original target")
    return answer


def _scalar(value):
    require(not value.b and type(value.a) is F, "source map coefficient is not rational")
    return value.a


def _map(mapping, parameter):
    def polynomial(coefficients):
        answer = F(0)
        for coefficient in reversed(coefficients): answer = answer*parameter+_scalar(coefficient)
        return answer
    denominator = polynomial(mapping.denominator)
    require(denominator > 0, "original map denominator not positive")
    return tuple(polynomial(row)/denominator for row in mapping.numerators)


def _model(result, prove_sections=False):
    target = _target(result.face)
    for cuts in (result.u_cuts,result.v_cuts):
        require(type(cuts) is tuple and len(cuts) <= 64 and all(type(x) is F and 0 < x < 1 for x in cuts)
                and tuple(sorted(set(cuts))) == cuts, "canonical exact source cuts differ")
    require(type(result.sections) is tuple and len(result.sections) <= 256, "immutable source section table differs")
    us, vs = (F(0),)+result.u_cuts+(F(1),), (F(0),)+result.v_cuts+(F(1),)
    circles = []
    for index, section in enumerate(result.sections):
        if prove_sections:
            # One existing independent mechanism qualifies the retained source
            # maps/events; new region/source-index truth is checked below.
            from .intersection_oracles import qualify_result
            qualify_result(section)
        sides = tuple(i for i,ref in enumerate((section.left,section.right))
                      if ref.snapshot is result.face.snapshot and ref.face_id == result.face.face_id)
        require(len(sides) == 1, "section does not bind the original target face")
        side = sides[0]
        wall = (section.right,section.left)[side]
        require(wall.face.kind == "cylinder" and {target["ua"],target["va"]} == {0,1},
                "source is not a horizontal plane/wall circle")
        require(len(section.components) == 1 and len(section.pieces) == len(section.events) == 4,
                "source circle component/piece/event completeness differs")
        component = section.components[0]
        require(component.kind == "circle" and component.closed and not component.tangent
                and type(component.piece_ids) is tuple and len(component.piece_ids) == 4
                and set(component.piece_ids) == set(range(4)), "source circle domain differs")
        frame = wall.snapshot.frame
        radii = tuple(exact(curve.data[0])*exact(frame.scale) for curve in wall.snapshot.curves if curve.kind == "circle")
        require(len(radii) == 2 and radii[0] == radii[1] and radii[0] > 0, "original source rim radii differ")
        require(abs(target["u"][target["ua"]]) == abs(target["v"][target["va"]]), "UV circle axes are anisotropic")
        center_world = (exact(frame.origin[0]),exact(frame.origin[1]),target["origin"][2])
        center = _uv(target,center_world)
        radius = radii[0]/abs(target["u"][target["ua"]])
        cell = tuple(tuple(i for i in range(len(axis)-1) if axis[i] < c-radius < c+radius < axis[i+1])
                     for axis,c in zip((us,vs),center))
        require(all(len(choices) == 1 for choices in cell), "source circle not strictly inside exactly one cell")
        endpoints = {}
        for event in section.events:
            for incidence in event.incidences:
                require(incidence not in endpoints, "duplicate original circle event incidence")
                endpoints[incidence] = event.id
        points = []
        for pid in component.piece_ids:
            piece = section.pieces[pid]
            require(piece.id == pid and piece.kind == "arc" and _scalar(piece.lo) == 0 and _scalar(piece.hi) == 1,
                    "original circle piece/range differs")
            points.append(_uv(target,_map(piece.spatial_map,F(0))))
        twice_area = sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(points,points[1:]+points[:1]))
        require(twice_area != 0, "source circle has no oriented quarter traversal")
        circles.append(dict(index=index,section=section,component=component.id,piece_ids=component.piece_ids,
            event_ids=tuple(event.id for event in section.events),side=side,center=center,radius=radius,
            cell=tuple(choices[0] for choices in cell),anchor=endpoints[(component.piece_ids[0],"lo")],
            anchor_uv=points[0],orientation=1 if twice_area > 0 else -1))
    for i,a in enumerate(circles):
        for b in circles[:i]:
            require(sum((x-y)**2 for x,y in zip(a["center"],b["center"])) > (a["radius"]+b["radius"])**2,
                    "source disks overlap, touch or nest")
    return target,us,vs,tuple(circles)


def _boundary_source(face, a, b, target):
    matches = []
    for loop_slot in face.face.loop_slots:
        loop = face.snapshot.loops[loop_slot]
        for cslot in loop.coedge_slots:
            coedge = face.snapshot.coedges[cslot]
            require(coedge.uv_map.kind == "line", "original rectangular boundary is not linear")
            uv = tuple(exact(x) for x in coedge.uv_map.data)
            origin, finish = uv[:2], uv[2:]
            axes = tuple(i for i in range(2) if finish[i] != origin[i])
            require(len(axes) == 1, "original boundary UV map is not axial")
            axis = axes[0]
            t0 = (a[axis]-origin[axis])/(finish[axis]-origin[axis])
            t1 = (b[axis]-origin[axis])/(finish[axis]-origin[axis])
            mapped = lambda t: tuple(x+t*(y-x) for x,y in zip(origin,finish))
            if mapped(t0) == a and mapped(t1) == b and 0 <= t0 <= 1 and 0 <= t1 <= 1:
                edge = face.snapshot.edges[coedge.edge_slot]
                curve = face.snapshot.curves[edge.curve_slot]
                require(curve.kind == "line", "original boundary spatial curve is not linear")
                data = tuple(exact(x) for x in curve.data)
                frame = face.snapshot.frame
                for point,t in ((a,t0),(b,t1)):
                    spatial = tuple(exact(o)+exact(frame.scale)*(x+t*(y-x))
                                    for o,x,y in zip(frame.origin,data[:3],data[3:]))
                    require(spatial == _world(target,point), "whole affine source curve/target map differs")
                matches.append(("boundary",coedge.id,edge.id,t0,t1))
    require(len(matches) == 1, "boundary segment does not resolve one original coedge/subrange")
    return matches[0]


def _cycle(word):
    return min(word[i:]+word[:i] for i in range(len(word)))


def _locations(model, query):
    target,us,vs,circles = model
    u,v = tuple(exact(x) for x in query)
    if not 0 <= u <= 1 or not 0 <= v <= 1:return (0,)
    width,height = len(us)-1,len(vs)-1
    found = {0} if u in (0,1) or v in (0,1) else set()
    columns = tuple(i for i in range(width) if us[i] <= u <= us[i+1])
    rows = tuple(j for j in range(height) if vs[j] <= v <= vs[j+1])
    for j in rows:
        for i in columns:
            strictly_in_disk = False
            for circle in circles:
                if circle["cell"] != (i,j):continue
                distance = (u-circle["center"][0])**2+(v-circle["center"][1])**2
                if distance <= circle["radius"]**2:found.add(1+width*height+circle["index"])
                strictly_in_disk |= distance < circle["radius"]**2
            if not strictly_in_disk:found.add(1+j*width+i)
    return tuple(sorted(found))


def qualify_locations(result, queries, outputs):
    model = _model(result)
    queries,outputs = tuple(queries),tuple(outputs)
    require(len(queries) == len(outputs), "location query/output extent differs")
    for query,output in zip(queries,outputs):
        require(type(output) is tuple and all(type(x) is int for x in output)
                and output == _locations(model,query), "complete closed incident region lookup differs")
    return dict(queries=len(queries))


def qualify_arrangement(result):
    model = _model(result,True)
    target,us,vs,circles = model
    width,height = len(us)-1,len(vs)-1
    cells = width*height
    for name in ("vertices","edges","uses","loops","regions"):
        records = getattr(result,name)
        require(type(records) is tuple and tuple(record.id for record in records) == tuple(range(len(records)))
                and all(type(record.id) is int for record in records), "dense immutable "+name+" identities differ")
    require(type(result.outside_id) is int and result.outside_id == 0, "outside identity differs")
    expected_vertices = {("grid",i,j):(u,v) for j,v in enumerate(vs) for i,u in enumerate(us)}
    expected_vertices.update({("section",c["index"],c["anchor"]):c["anchor_uv"] for c in circles})
    vertex_keys = {}
    require(len(result.vertices) == len(expected_vertices), "complete source vertex extent differs")
    for vertex in result.vertices:
        require(vertex.source in expected_vertices and vertex.source not in vertex_keys,
                "event vertex missing, duplicated or welded across source identities")
        rational_tuple(vertex.uv,2,"vertex UV");rational_tuple(vertex.position,3,"vertex world")
        require(vertex.uv == expected_vertices[vertex.source] and vertex.position == _world(target,vertex.uv),
                "exact original vertex UV/world correspondence differs")
        vertex_keys[vertex.source] = vertex.id
    expected_edges = {}
    for j,v in enumerate(vs):
        for i in range(width):
            a,b = (us[i],v),(us[i+1],v)
            source = _boundary_source(result.face,a,b,target) if j in (0,height) else ("v_cut",j-1)
            expected_edges[("H",i,j)] = (vertex_keys[("grid",i,j)],vertex_keys[("grid",i+1,j)],source,(),())
    for i,u in enumerate(us):
        for j in range(height):
            a,b = (u,vs[j]),(u,vs[j+1])
            source = _boundary_source(result.face,a,b,target) if i in (0,width) else ("u_cut",i-1)
            expected_edges[("V",i,j)] = (vertex_keys[("grid",i,j)],vertex_keys[("grid",i,j+1)],source,(),())
    for circle in circles:
        anchor = vertex_keys[("section",circle["index"],circle["anchor"])]
        expected_edges[("C",circle["index"])] = (anchor,anchor,("section",circle["index"],circle["component"]),
                                                   circle["piece_ids"],circle["event_ids"])
    lookup = {(item[0],item[1],item[2]):key for key,item in expected_edges.items()}
    require(len(result.edges) == len(expected_edges), "complete input curve edge extent differs")
    edge_keys,seen_edge_keys = {},set()
    for edge in result.edges:
        key = lookup.get((edge.start,edge.end,edge.source))
        require(key is not None and key not in seen_edge_keys, "edge curve/source direction or full input coverage differs")
        expected = expected_edges[key]
        require(edge.kind == ("circle" if key[0] == "C" else "line"), "edge geometry kind differs")
        require(edge.piece_ids == expected[3] and type(edge.event_ids) is tuple
                and Counter(edge.event_ids) == Counter(expected[4]), "whole circle source piece/event coverage differs")
        edge_keys[edge.id] = key
        seen_edge_keys.add(key)
        parameters = (F(0),F(1,8),F(1,4),F(3,8),F(1,2),F(5,8),F(3,4),F(7,8),F(1)) if key[0] == "C" else (F(0),F(1,2),F(1))
        for t in parameters:
            if key[0] == "C":
                circle = circles[key[1]]
                part = min((4*t).numerator//(4*t).denominator,3)
                piece = circle["section"].pieces[circle["piece_ids"][part]]
                q = 4*t-part
                world = _map(piece.spatial_map,q)
                uv = _map(piece.left_map if circle["side"] == 0 else piece.right_map,q)
                require(world == _world(target,uv), "whole original map correspondence differs")
            else:
                a,b = expected_vertices[result.vertices[edge.start].source],expected_vertices[result.vertices[edge.end].source]
                uv = tuple((1-t)*x+t*y for x,y in zip(a,b));world = _world(target,uv)
            actual_uv,actual_world = result.edge_uv(edge.id,t),result.edge_point(edge.id,t)
            rational_tuple(actual_uv,2,"reader UV");rational_tuple(actual_world,3,"reader world")
            require(actual_uv == uv and actual_world == world, "actual source curve reader changed parameter/UV/world")
    require(len(result.uses) == 2*len(expected_edges), "directed use coverage differs")
    by_edge = {edge.id:[] for edge in result.edges}
    for use in result.uses:
        require(all(type(getattr(use,name)) is int for name in ("edge","region","twin","next","previous","loop")),
                "use references are not integer identities")
        require(type(use.edge) is int and use.edge in by_edge and type(use.orientation) is int
                and use.orientation in (-1,1), "invalid use edge/orientation")
        by_edge[use.edge].append(use)
    for uses in by_edge.values():
        require(len(uses) == 2 and sum(use.orientation for use in uses) == 0,
                "edge does not have exactly two opposed uses")
        a,b = uses
        require(a.twin == b.id and b.twin == a.id and a.id != b.id, "directed twin involution differs")
    perimeter = (tuple((("V",0,j),1) for j in range(height))+
                 tuple((("H",i,height),1) for i in range(width))+
                 tuple((("V",width,j),-1) for j in reversed(range(height)))+
                 tuple((("H",i,0),-1) for i in reversed(range(width))))
    expected_loops = {0:(None,(perimeter,))}
    for j in range(height):
        for i in range(width):
            outer = ((("H",i,j),1),(("V",i+1,j),1),(("H",i,j+1),-1),(("V",i,j),-1))
            holes = tuple(((("C",c["index"]),-c["orientation"]),) for c in circles if c["cell"] == (i,j))
            expected_loops[1+j*width+i] = outer,holes
    for c in circles:
        expected_loops[1+cells+c["index"]] = (((("C",c["index"]),c["orientation"]),),())
    require(len(result.regions) == 1+cells+len(circles), "complete cell-minus-disks/disk region bijection differs")
    seen_loops,seen_uses = set(),set()
    area_rational=area_pi=F(0)
    for region in result.regions:
        if region.id == 0:
            require((region.kind,region.cell,region.section,region.outer_loop,region.area) == ("outside",None,None,None,None),
                    "exterior domain metadata differs")
            require(region.witness_uv == (F(-1),F(-1)), "exterior witness differs")
        elif region.id <= cells:
            j,i = divmod(region.id-1,width)
            require((region.kind,region.cell,region.section) == ("cell",(i,j),None), "cell domain identity differs")
            local = tuple(c for c in circles if c["cell"] == (i,j))
            rational = (us[i+1]-us[i])*(vs[j+1]-vs[j]);pi = -sum((c["radius"]**2 for c in local),F(0))
            require((region.area.rational,region.area.pi) == (rational,pi), "cell-minus-disks exact area differs")
            rational_tuple(region.witness_uv,2,"cell witness")
            right = min(c["center"][0]-c["radius"] for c in local) if local else us[i+1]
            require(us[i] < region.witness_uv[0] < right and region.witness_uv[1] == (vs[j]+vs[j+1])/2,
                    "cell witness is not strict in the declared gap")
        else:
            c = circles[region.id-1-cells]
            require((region.kind,region.cell,region.section) == ("disk",c["cell"],c["index"]), "disk source domain identity differs")
            require((region.area.rational,region.area.pi) == (0,c["radius"]**2), "disk exact area differs")
            require(region.witness_uv == c["center"], "disk source center witness differs")
        if region.id:
            require(type(region.area.rational) is F and type(region.area.pi) is F, "area coefficients are not exact rationals")
            area_rational += region.area.rational;area_pi += region.area.pi
        outer_word,hole_words = expected_loops[region.id]
        require(type(region.holes) is tuple and len(region.holes) == len(hole_words), "complete hole ownership differs")
        actual_words = []
        loop_refs = (() if region.outer_loop is None else ((region.outer_loop,False),))+tuple((lid,True) for lid in region.holes)
        require((region.outer_loop is None) == (outer_word is None), "region outer component missing or fabricated")
        for lid,hole in loop_refs:
            require(type(lid) is int and 0 <= lid < len(result.loops) and lid not in seen_loops, "loop ownership duplicated or invalid")
            seen_loops.add(lid);loop = result.loops[lid]
            require(loop.region == region.id and type(loop.hole) is bool and loop.hole == hole
                    and type(loop.uses) is tuple and loop.uses, "ordered loop region/type differs")
            word=[]
            for index,uid in enumerate(loop.uses):
                require(type(uid) is int and 0 <= uid < len(result.uses) and uid not in seen_uses, "use omitted or multiply owned")
                seen_uses.add(uid);use = result.uses[uid]
                require(use.region == region.id and use.loop == lid and use.next == loop.uses[(index+1)%len(loop.uses)]
                        and use.previous == loop.uses[index-1], "loop next/previous/region binding differs")
                edge = result.edges[use.edge];following = result.uses[use.next];next_edge=result.edges[following.edge]
                end = edge.end if use.orientation == 1 else edge.start
                begin = next_edge.start if following.orientation == 1 else next_edge.end
                require(end == begin, "directed boundary endpoints do not close")
                word.append((edge_keys[use.edge],use.orientation))
            if hole:actual_words.append(_cycle(tuple(word)))
            else:require(_cycle(tuple(word)) == _cycle(outer_word), "complete oriented outer boundary word differs")
        require(Counter(actual_words) == Counter(_cycle(word) for word in hole_words), "complete oriented hole/perimeter words differ")
        require(_locations(model,region.witness_uv) == (region.id,), "strict witness does not identify its complete region domain")
    require(len(seen_loops) == len(result.loops) and len(seen_uses) == len(result.uses), "unreachable loops or directed uses remain")
    require((area_rational,area_pi) == (1,0), "bounded region areas do not partition original UV face")
    return dict(vertices=len(result.vertices),edges=len(result.edges),uses=len(result.uses),loops=len(result.loops),
                regions=len(result.regions),cells=cells,disks=len(circles),holes=len(circles))
