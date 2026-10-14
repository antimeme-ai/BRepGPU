"""Original-source exact world constraints and complete corner-set truth.

No product classification/interval/plane helper supplies an expected answer.
The contact model enumerates rational constraint intersections, not intervals.
"""
from fractions import Fraction as F
from itertools import combinations


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def exact(value):
    if type(value) is float:
        return F.from_float(value)
    return F(value)


def add(a, b):
    return tuple(x+y for x, y in zip(a, b))


def subtract(a, b):
    return tuple(x-y for x, y in zip(a, b))


def scale(a, t):
    return tuple(t*x for x in a)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def rational_tuple(values, count, label):
    require(type(values) is tuple and len(values) == count and all(type(x) is F for x in values),
            label+" is not an owned exact rational tuple")


def point_truth(snapshot, point):
    """Read original vertices/circle curves, stay in world coordinates."""
    require(snapshot.shape in ("box", "cylinder", "plate"), "unsupported source body")
    origin = tuple(exact(x) for x in snapshot.frame.origin)
    s = exact(snapshot.frame.scale)
    positions = tuple(tuple(exact(x) for x in vertex.position) for vertex in snapshot.vertices)
    planes = tuple((origin[axis]+s*min(v[axis] for v in positions),
                    origin[axis]+s*max(v[axis] for v in positions)) for axis in range(3))
    names = {face.name: face.id for face in snapshot.faces}
    relevant = (2,) if snapshot.shape == "cylinder" else (0, 1, 2)
    if any(not planes[axis][0] <= point[axis] <= planes[axis][1] for axis in relevant):
        return "exterior", ()
    rho = None
    if snapshot.shape != "box":
        radii = tuple(exact(curve.data[0]) for curve in snapshot.curves if curve.kind == "circle")
        require(len(radii) == 2 and radii[0] == radii[1], "original rim radii differ")
        radius_squared = (s*radii[0])**2
        rho = (point[0]-origin[0])**2+(point[1]-origin[1])**2
        if (snapshot.shape == "cylinder" and rho > radius_squared) or (
                snapshot.shape == "plate" and rho < radius_squared):
            return "exterior", ()
    active = []
    for axis, low_name, high_name in ((0, "left", "right"), (1, "front", "back"), (2, "bottom", "top")):
        if axis not in relevant:
            continue
        if point[axis] == planes[axis][0]: active.append(names[low_name])
        if point[axis] == planes[axis][1]: active.append(names[high_name])
    if rho is not None and rho == radius_squared:
        active.append(names["wall"])
    return ("boundary", tuple(sorted(active))) if active else ("interior", ())


def qualify_point(result):
    rational_tuple(result.point, 3, "world query")
    label, faces = point_truth(result.snapshot, result.point)
    require(result.label == label, "material label differs from original exact world constraints")
    require(type(result.boundary_faces) is tuple and all(type(face) is int for face in result.boundary_faces)
            and result.boundary_faces == faces,
            "complete original supporting face IDs differ")
    return dict(label=label, boundary_faces=faces)


def rectangle_source(reference):
    body = reference.snapshot
    found = tuple(face for face in body.faces if face.id == reference.face_id)
    require(len(found) == 1, "source face identity does not resolve uniquely")
    face = found[0]
    require(face.kind == "rectangle" and len(face.chart_slots) == 1, "source face is not a single rectangle")
    chart = body.charts[face.chart_slots[0]]
    patch = chart.patch
    require((patch.degree_u, patch.degree_v) == (1, 1) and len(patch.controls) == 16,
            "original rectangle control shape differs")
    origin = tuple(exact(x) for x in patch.origin)
    s = exact(patch.scale)
    points, weights = [], []
    for index in range(4):
        control = tuple(exact(x) for x in patch.controls[4*index:4*index+4])
        require(control[3] > 0, "original affine control weight is not positive")
        weights.append(control[3])
        points.append(tuple(origin[axis]+s*control[axis]/control[3] for axis in range(3)))
    p00, p01, p10, p11 = points
    require(len(set(weights)) == 1, "original affine rectangle weights differ")
    u, v = subtract(p10, p00), subtract(p01, p00)
    require(p11 == add(p00, add(u, v)), "original rectangle is not affine")
    normal = cross(u, v)
    require(any(normal) and dot(u, v) == 0, "original rectangle axes invalid")
    require(sum(x != 0 for x in u) == sum(x != 0 for x in v) == 1,
            "original rectangle is not axis aligned")
    corners = (p00, p10, p11, p01)
    inequalities = []
    for a, b in zip(corners, corners[1:]+corners[:1]):
        inward = cross(normal, subtract(b, a))
        inequalities.append((inward, dot(inward, a)))
    return dict(origin=p00, u=u, v=v, normal=normal,
                oriented_normal=scale(normal, face.orientation), corners=corners,
                plane=(normal, dot(normal, p00)), inequalities=tuple(inequalities))


def solve3(equations):
    (a, da), (b, db), (c, dc) = equations
    determinant = dot(a, cross(b, c))
    if not determinant:
        return None
    numerator = add(scale(cross(b, c), da), add(scale(cross(c, a), db), scale(cross(a, b), dc)))
    return scale(numerator, 1/determinant)


def source_uv(source, point):
    delta = subtract(point, source["origin"])
    u, v, n = source["u"], source["v"], source["normal"]
    require(dot(n, delta) == 0, "world corner lies off original source plane")
    denominator = dot(u, cross(v, n))
    return dot(delta, cross(v, n))/denominator, dot(delta, cross(n, u))/denominator


def affine_rank(points):
    if not points:
        return -1
    first = points[0]
    differences = tuple(subtract(point, first) for point in points[1:])
    if not differences:
        return 0
    axis = differences[0]
    return 2 if any(any(cross(axis, other)) for other in differences[1:]) else 1


def contact_truth(left_reference, right_reference):
    left, right = rectangle_source(left_reference), rectangle_source(right_reference)
    parallel = not any(cross(left["normal"], right["normal"]))
    planes = (left["plane"], right["plane"])
    if parallel:
        coplanar = dot(planes[0][0], right["origin"]) == planes[0][1]
        carrier = "coplanar" if coplanar else "parallel_disjoint"
        normal_relation = "same" if dot(left["oriented_normal"], right["oriented_normal"]) > 0 else "opposite"
    else:
        require(dot(left["normal"], right["normal"]) == 0, "unsupported oblique source planes")
        carrier, normal_relation = "transverse", "orthogonal"
    boundaries = left["inequalities"]+right["inequalities"]
    systems = ((planes[0], *pair) for pair in combinations(boundaries, 2)) if parallel else (
        (planes[0], planes[1], boundary) for boundary in boundaries)
    corners = set()
    for system in systems:
        point = solve3(system)
        if point is not None and all(dot(n, point) == d for n, d in planes) and all(
                dot(n, point) >= d for n, d in boundaries):
            corners.add(point)
    ordered = tuple(sorted(corners))
    dimension = affine_rank(ordered)
    same = carrier == "coplanar" and set(left["corners"]) == set(right["corners"])
    if dimension < 0:
        relation = "disjoint"
    elif dimension == 2:
        require(carrier == "coplanar", "noncoplanar contact acquired area")
        relation = "coincident" if same else "overlap"
    elif dimension == 1 and carrier == "transverse":
        require(len(ordered) == 2, "convex segment extreme set differs")
        midpoint = scale(add(ordered[0], ordered[1]), F(1, 2))
        uv_left, uv_right = source_uv(left, midpoint), source_uv(right, midpoint)
        crossing = all(0 < x < 1 for x in uv_left+uv_right)
        relation = "crossing" if crossing else "touch"
    else:
        relation = "touch"
    return left, right, ordered, dimension, relation, carrier, same, normal_relation


def qualify_contact(result):
    left, right, expected, dimension, relation, carrier, same, normal_relation = contact_truth(result.left, result.right)
    require(type(result.dimension) is int and type(result.same_domain) is bool,
            "contact dimension/equality types differ")
    require((result.dimension, result.relation, result.carrier_relation, result.same_domain, result.normal_relation) ==
            (dimension, relation, carrier, same, normal_relation), "finite contact classification differs")
    require(type(result.vertices) is tuple and len(result.vertices) == len(expected),
            "complete contact vertex extent differs")
    positions = tuple(vertex.position for vertex in result.vertices)
    for vertex in result.vertices:
        rational_tuple(vertex.position, 3, "contact world position")
        rational_tuple(vertex.left_uv, 2, "left source UV")
        rational_tuple(vertex.right_uv, 2, "right source UV")
        require(vertex.left_uv == source_uv(left, vertex.position), "left original source parameter correspondence differs")
        require(vertex.right_uv == source_uv(right, vertex.position), "right original source parameter correspondence differs")
        require(all(0 <= x <= 1 for x in vertex.left_uv+vertex.right_uv), "contact corner outside closed original trim")
    require(set(positions) == set(expected), "complete exact contact corner set differs")
    if dimension in (0, 1):
        require(positions == expected, "point/segment endpoints are not lexicographic")
    if dimension == 2:
        uvs = tuple(vertex.left_uv for vertex in result.vertices)
        turns = []
        for index in range(len(uvs)):
            a, b, c = uvs[index-1], uvs[index], uvs[(index+1) % len(uvs)]
            turns.append((b[0]-a[0])*(c[1]-b[1])-(b[1]-a[1])*(c[0]-b[0]))
        require(all(turn > 0 for turn in turns), "area corners violate positive left-chart parameter orientation")
    return dict(dimension=dimension, relation=relation, vertices=len(expected))
