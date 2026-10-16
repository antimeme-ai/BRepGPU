"""Read actual native packet output against exact input rectangle chains."""

from __future__ import annotations

import argparse
from collections import Counter
from fractions import Fraction
import subprocess


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def reduce_chain(edges: Counter) -> Counter:
    result = Counter()
    for (a, b), count in edges.items():
        remainder = count - edges[(b, a)]
        if remainder > 0:
            result[(a, b)] = remainder
    return result


def check_case(name: str, rows: list[list[str]]) -> tuple[int, int]:
    points = {}
    leaves = {}
    lines = {}
    sides = {}
    triangles = []
    rims = []
    demoted = set()
    for row in rows:
        kind = row[0]
        if kind == "point":
            require(len(row) == 6, f"{name}: point schema")
            slot, un, ud, vn, vd = map(int, row[1:])
            require(slot not in points and ud > 0 and vd > 0, f"{name}: point inventory")
            points[slot] = (Fraction(un, ud), Fraction(vn, vd))
        elif kind == "leaf":
            require(len(row) == 15, f"{name}: leaf schema")
            values = list(map(int, row[1:]))
            slot = values[0]
            box = tuple(Fraction(values[i], values[i + 1]) for i in (1, 3, 5, 7))
            require(slot not in leaves, f"{name}: repeated leaf")
            leaves[slot] = (box, values[9:13], values[13])
        elif kind == "line":
            require(len(row) >= 7, f"{name}: line schema")
            slot, axis, numerator, denominator = map(int, row[1:5])
            require(slot not in lines and axis in (0, 1), f"{name}: line inventory")
            lines[slot] = (axis, Fraction(numerator, denominator), list(map(int, row[5:])))
        elif kind == "side":
            require(len(row) == 7, f"{name}: side schema")
            leaf, side, line, first, count, reverse = map(int, row[1:])
            require((leaf, side) not in sides and reverse in (0, 1), f"{name}: side inventory")
            sides[(leaf, side)] = (line, first, count, reverse)
        elif kind == "triangle":
            require(len(row) == 5, f"{name}: triangle schema")
            triangles.append(tuple(map(int, row[1:])))
        elif kind == "rim":
            require(len(row) == 4, f"{name}: rim schema")
            rims.append(tuple(map(int, row[1:])))
        elif kind == "demote":
            require(len(row) == 2 and int(row[1]) not in demoted, f"{name}: demotion schema")
            demoted.add(int(row[1]))
        else:
            raise AssertionError(f"{name}: unknown output row {kind}")

    require(points and leaves, f"{name}: missing authored inputs")
    require(len(set(points.values())) == len(points), f"{name}: noncanonical input point IDs")
    enabled = {slot for slot, (_, _, flag) in leaves.items() if flag}
    require(demoted <= enabled, f"{name}: demoted unavailable material")
    remaining = set(enabled)
    reference_demoted = set()
    while remaining:
        marked = set()
        vertices = {points[p] for leaf in remaining for p in leaves[leaf][1]}
        for u, v in vertices:
            quadrants = [set() for _ in range(4)]
            incident = set()
            for leaf in remaining:
                (u0, u1, v0, v1), _, _ = leaves[leaf]
                if u0 <= u <= u1 and v0 <= v <= v1:
                    incident.add(leaf)
                flags = (u0 <= u < u1 and v0 <= v < v1,
                         u0 <= u < u1 and v0 < v <= v1,
                         u0 < u <= u1 and v0 < v <= v1,
                         u0 < u <= u1 and v0 <= v < v1)
                for quadrant, present in zip(quadrants, flags):
                    if present:
                        quadrant.add(leaf)
            require(all(len(q) <= 1 for q in quadrants), f"{name}: invalid overlapping input leaves")
            pattern = tuple(bool(q) for q in quadrants)
            if pattern in ((True, False, True, False), (False, True, False, True)):
                marked.update(incident)
        if not marked:
            break
        remaining -= marked
        reference_demoted |= marked
    require(demoted == reference_demoted, f"{name}: unjustified or missing packet demotion")
    active = enabled - demoted
    active_corners = {p for slot in active for p in leaves[slot][1]}
    expected_boundary = Counter()
    triangles_by_leaf = {slot: [] for slot in active}
    line_keys = [(axis, constant) for axis, constant, _ in lines.values()]
    require(len(set(line_keys)) == len(line_keys), f"{name}: duplicate supporting line")

    for line, (axis, constant, events) in lines.items():
        require(len(events) >= 2 and len(set(events)) == len(events), f"{name}: repeated line event")
        require(all(p in active_corners for p in events), f"{name}: invented line point")
        require(all(points[p][1 - axis] == constant for p in events), f"{name}: wrong supporting line")
        coordinates = [points[p][axis] for p in events]
        require(coordinates == sorted(set(coordinates)), f"{name}: event ordering")

    for leaf, a, b, c in triangles:
        require(leaf in active and len({a, b, c}) == 3, f"{name}: triangle ownership")
        require(all(p in points for p in (a, b, c)), f"{name}: unknown triangle point")
        pa, pb, pc = (points[p] for p in (a, b, c))
        determinant = (pb[0] - pa[0]) * (pc[1] - pa[1]) - (pb[1] - pa[1]) * (pc[0] - pa[0])
        require(determinant > 0, f"{name}: nonpositive actual triangle")
        triangles_by_leaf[leaf].append((a, b, c))

    for leaf in sorted(active):
        (u0, u1, v0, v1), corners, _ = leaves[leaf]
        require(u0 < u1 and v0 < v1, f"{name}: invalid input rectangle")
        require([points[p] for p in corners] == [(u0, v0), (u1, v0), (u0, v1), (u1, v1)],
                f"{name}: tensor corner order")
        boundary = Counter()
        sample_count = 0
        boundary_points = set()
        for side in range(4):
            axis = 0 if side in (0, 2) else 1
            constant = (v0, u1, v1, u0)[side]
            begin, end = ((u0, u1) if axis == 0 else (v0, v1))
            expected = sorted((p for p in active_corners if points[p][1 - axis] == constant
                               and begin <= points[p][axis] <= end), key=lambda p: points[p][axis])
            if side >= 2:
                expected.reverse()
            require((leaf, side) in sides, f"{name}: missing side")
            line, first, count, reverse = sides[(leaf, side)]
            require(line in lines and first >= 0 and count >= 2, f"{name}: side range")
            line_axis, line_constant, events = lines[line]
            require((line_axis, line_constant) == (axis, constant), f"{name}: side line mismatch")
            require(first + count <= len(events), f"{name}: side range overflow")
            actual = events[first:first + count]
            if reverse:
                actual.reverse()
            require(actual == expected, f"{name}: omitted/reordered hanging sample")
            sample_count += len(actual) - 1
            boundary_points.update(actual)
            boundary.update(zip(actual, actual[1:]))
        actual_chain = Counter()
        leaf_triangles = triangles_by_leaf[leaf]
        require(len(leaf_triangles) == sample_count - 2, f"{name}: k-2 triangle count")
        require(all(p in boundary_points for triangle in leaf_triangles for p in triangle),
                f"{name}: unnecessary center/construction point")
        for a, b, c in leaf_triangles:
            actual_chain.update(((a, b), (b, c), (c, a)))
        require(reduce_chain(actual_chain) == boundary, f"{name}: exact positive rectangle coverage")
        expected_boundary.update(boundary)

    require(set(sides) == {(leaf, side) for leaf in active for side in range(4)},
            f"{name}: extra packet sides")
    expected_rim = reduce_chain(expected_boundary)
    actual_rim = Counter((a, b) for leaf, a, b in rims)
    require(all(leaf in active for leaf, _, _ in rims), f"{name}: rim owner")
    require(actual_rim == expected_rim, f"{name}: actual union rim")
    degrees = Counter(a for a, _ in actual_rim)
    require(all(count == 1 for count in degrees.values()), f"{name}: weak/branching rim left unresolved")
    return len(active), len(triangles)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", required=True)
    parser.add_argument("--source", required=True)
    args = parser.parse_args()
    result = subprocess.run([args.fixture, args.source], check=True, text=True, stdout=subprocess.PIPE)
    current = None
    rows = []
    names = set()
    total = 0
    for line in result.stdout.splitlines():
        row = line.split("\t")
        if row[0] == "case":
            require(current is None and len(row) == 2 and row[1] not in names, "case envelope")
            current = row[1]
            rows = []
        elif row[0] == "end":
            require(current is not None and len(row) == 1, "case end")
            _, triangles = check_case(current, rows)
            names.add(current)
            total += triangles
            current = None
        else:
            require(current is not None, "output outside case")
            rows.append(row)
    require(current is None and len(names) >= 8, "incomplete native case inventory")
    print(f"PASS native packet Fraction coverage: {len(names)} cases, {total} actual triangles")


if __name__ == "__main__":
    main()
