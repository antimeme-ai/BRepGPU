"""The STEP source adapter reads mapped items and carriers raw (B2-reader-adapter); fleet only.

G1's authority rule: the raw Part 21 entity is authoritative and OCCT's
translation only an evaluator (papers/2026-10-17-occt-ingress-translation-edits.md
section 4). Beads: .37.27 (a mapped-item cycle OCCT recurses on is refused by
name before the transfer), .37.26 (a solid-bearing mapped item no usage
publishes is refused, not dropped), .37.28 and .37.30 (closed B-spline carriers
and edge curves OCCT made periodic are written as the file's own nets), .37.29
(a carrier's UV hint covers every face on it), .37.31 (projected UV curves stay
in their B-spline carrier's natural domain), .37.32 (holders at any depth),
.37.36 (a face's side from its own bound and its closed shell). Placements (.37.20) are in tests/test_placement_oracles.py.
Oracles: the STEP text (entity ids, nets, knots), both readers, and the
published pcurves evaluated independently.
"""
import math
import os
import re
from pathlib import Path
import subprocess
import tempfile
import unittest

from tests import test_step_ingress as ingress
from tests.test_step_ingress import FIXTURES, _Adapter, appended, inspect_counts, styled_mapped, substituted


ROOT_REP = "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23),#27);"


def with_root_items(*items, records=''):
    """source-uv.step with extra items in the assembly's shape representation #10."""
    text = substituted((FIXTURES/'source-uv.step').read_text(), ROOT_REP,
                       f"#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23,{','.join(items)}),#27);")
    return appended(text, records)


def deep_holder(inches):
    """source-uv.step with a second holder #1009 of the box solid #41, joined to the box's
    representation #40 by nine plain shape_representation_relationships through
    #1001..#1008 (a nesting deeper than 8). #1009's context is #1020, in inches when
    inches is true, else the assembly's millimetre context #27."""
    reps = ''.join(f"#{1000+k} = SHAPE_REPRESENTATION('',(#11),#27);\n" for k in range(1, 9))
    context = '#1020' if inches else '#27'
    reps += f"#1009 = SHAPE_REPRESENTATION('',(#11,#41),{context});\n"
    links = ''.join(f"#{1030+k} = SHAPE_REPRESENTATION_RELATIONSHIP('','',#{1000+k+1},#{1000+k if k else 40});\n"
                    for k in range(9))
    unit = ("#1020 = ( GEOMETRIC_REPRESENTATION_CONTEXT(3) GLOBAL_UNIT_ASSIGNED_CONTEXT((#1021,#29)) "
            "REPRESENTATION_CONTEXT('','') );\n" + '\n'.join(ingress.conversion_unit(1021, 'INCH', 25.4, 1022)))
    return appended((FIXTURES/'source-uv.step').read_text(), reps + links + (unit if inches else ''))


class DeepHolderTests(unittest.TestCase, _Adapter):
    """.37.32: transfer_path searched to depth 8 only, so a holder nine links from its
    product's representation escaped the unit gate (and the .37.26 check). Every
    representation is now visited once, at any depth."""

    def test_deep_holder_in_inches_refuses_by_its_factor(self):
        with tempfile.TemporaryDirectory() as directory:
            run, _, _ = self.extract(deep_holder(True), directory)
            self.assertEqual(run.returncode, 1, run.stderr)
            self.assertIn('representation 1009 context 1020 length unit 1021 resolves to 25.4 mm per unit', run.stderr)

    def test_deep_holder_in_millimetres_is_admitted(self):
        """Guard: the same nesting in millimetres reads, with the box solid unchanged."""
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(deep_holder(False), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual({d.id: d.product_id for d in load_source(target).definitions}, {41: 35, 395: 389})


def shell_edge_uses(source):
    """The closure failures each source_shell_orientation_refused record carries."""
    return [tuple(v.value for v in d.values if v.name in ('edges_used_once', 'edges_used_more_than_twice', 'edges_used_twice_same_direction'))
            for d in source.diagnostics if d.stage == 'source_shell_orientation_refused']


BLOCK_FACES = ((17, '.F.'), (57, '.T.'), (97, '.F.'), (119, '.T.'), (141, '.F.'), (153, '.T.'))
BLOCK_SURFACES = {17: 52, 57: 92, 97: 114, 119: 136, 141: 148, 153: 160}


def flipped_block(same_sense=(), bounds=()):
    """plain_block (void-block.step without its void, a 10 mm cube) with the listed faces'
    same_sense and/or their one face_bound's orientation flipped, nothing else."""
    text = ingress.plain_block()
    for face, sense in BLOCK_FACES:
        other = '.T.' if sense == '.F.' else '.F.'
        if face in same_sense:
            text = substituted(text, f"#{face} = ADVANCED_FACE('',(#{face+1}),#{BLOCK_SURFACES[face]},{sense});",
                               f"#{face} = ADVANCED_FACE('',(#{face+1}),#{BLOCK_SURFACES[face]},{other});")
        if face in bounds:
            text = substituted(text, f"#{face+1} = FACE_BOUND('',#{face+2},{sense});", f"#{face+1} = FACE_BOUND('',#{face+2},{other});")
    return text


class OrientationTests(unittest.TestCase, _Adapter):
    """.37.36: a face's raw use (same_sense composed with its shell) is held to the
    standard's propositions: its outer bound runs counter-clockwise about the face
    normal, and in a closed shell every edge is used once in each direction. The
    oracle is the cube's divergence-theorem volume from the published tables
    (tests/test_step_ingress.py table_volume): 1000 mm^3 exactly when every face's
    written side is outward."""

    def read(self, text):
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            inspected, _ = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
            return load_source(target), run

    def records(self, source):
        return {d.source_ids[1]: {v.name: v.value for v in d.values}
                for d in source.diagnostics if d.stage == 'source_face_orientation_raw'}

    def read_all(self, texts):
        """read() of several independent fixtures, extracted concurrently (each case still
        runs in full; only the wall time is shared). Results in order."""
        from concurrent.futures import ThreadPoolExecutor
        with ThreadPoolExecutor(max_workers=max(1, min(len(texts), os.cpu_count() or 1))) as pool:
            return list(pool.map(self.read, texts))

    def test_a_consistent_block_is_left_as_it_is(self):
        source, run = self.read(ingress.plain_block())
        self.assertAlmostEqual(ingress.table_volume(source), 1000., places=9)
        self.assertEqual(self.records(source), {})
        self.assertIn('source_faces_orientation_raw\t0\n', run.stdout)

    def test_same_sense_against_its_bound_and_the_shell_is_corrected(self):
        """S1: face #57's same_sense flipped, its bound and loop left: the bound and all
        four of its edges' uses agree with the shell, so the shell's side is written."""
        twin, _ = self.read(ingress.plain_block())
        source, _ = self.read(flipped_block(same_sense=(57,)))
        self.assertAlmostEqual(ingress.table_volume(source), 1000., places=9)
        self.assertEqual({f.id: f.orientation for f in source.faces}, {f.id: f.orientation for f in twin.faces})
        record = self.records(source)[57]
        self.assertEqual((record['raw_same_sense'], record['raw_sense'], record['written_sense']), (0., -1., 1.))
        self.assertEqual((record['bound_sense'], record['shell_edges_used_once_each_way']), (1., 1.))
        self.assertAlmostEqual(record['bound_signed_volume_mm3'], 1000., places=6)
        self.assertEqual(set(self.records(source)), {57})

    def test_a_shell_inside_out_as_a_whole_is_turned(self):
        """S2: every face's same_sense and bound flipped together: no face contradicts its
        own bound and every edge is still used once each way, but the shell encloses the
        cube negatively, so every face's side is turned."""
        all_faces = [face for face, _ in BLOCK_FACES]
        twin, _ = self.read(ingress.plain_block())
        source, _ = self.read(flipped_block(same_sense=all_faces, bounds=all_faces))
        self.assertAlmostEqual(ingress.table_volume(source), 1000., places=9)
        self.assertEqual({f.id: f.orientation for f in source.faces}, {f.id: f.orientation for f in twin.faces})
        records = self.records(source)
        self.assertEqual(set(records), set(all_faces))
        self.assertTrue(all(r['raw_signed_volume_mm3'] < -999. and r['shell_edges_used_once_each_way'] == 1. for r in records.values()))

    def test_a_bound_against_the_shell_refuses_that_face(self):
        """Ambiguous: face #57's bound flipped, its same_sense left. Its loop now runs the
        same way as its neighbours' over all four edges, so the closed shell fails the
        closure proposition and has no material side: every face is refused by name
        (reason 7) with the shell's record, the definition incomplete."""
        from brepgpu.imported_source import FACE_REFUSED, DEFINITION_INCOMPLETE
        source, run = self.read(flipped_block(bounds=(57,)))
        # Coordinator ruling on review round 2, finding 2: #57's four edges are each used
        # twice the same way, so the shell is not closed and no face of it is admitted.
        self.assertEqual({f.id for f in source.faces if f.status == FACE_REFUSED}, {17, 57, 97, 119, 141, 153})
        self.assertEqual(shell_edge_uses(source), [(0., 0., 4.)])
        self.assertEqual([d.status for d in source.definitions], [DEFINITION_INCOMPLETE])
        refused, = [d for d in source.diagnostics if d.stage == 'source_face_refused' and d.source_ids[0] == 57]
        values = {v.name: v.value for v in refused.values}
        self.assertEqual((refused.source_ids[0], values['reason'], values['shell_edges_used_once_each_way']), (57, 7., 0.))
        self.assertIn('orientation_undecided', refused.message)
        self.assertIn('face_refusal\torientation_undecided\t6\n', run.stdout)
        self.assertEqual(self.records(source), {})


class OrientationReviewTests(unittest.TestCase, _Adapter):
    """.37.36 review findings 1 and 2: no correction is written that the shell's signed
    volume on the written uses does not verify, and a face with a vertex loop is decided
    from its own edge loop and its pole, never left silent."""

    read = OrientationTests.read
    records = OrientationTests.records

    def refused_ids(self, source):
        return sorted(d.source_ids[0] for d in source.diagnostics if d.stage == 'source_face_refused')

    def test_bounds_against_the_shell_majority_refuse_the_whole_shell(self):
        """Every bound reversed and same_sense flipped on 4, 5 or 3 of the six faces: the
        faces that follow their bounds point inward together, so no reading is verified.
        Before: S1 flipped the correct faces (volume -1000) or admitted the inward ones
        (333). After: every face refused, with the shell's record."""
        from brepgpu.imported_source import FACE_REFUSED
        all_faces = [face for face, _ in BLOCK_FACES]
        for name, flipped in (('4 of 6', (17, 57, 97, 119)), ('5 of 6', (17, 57, 97, 119, 141)), ('3 of 6', (17, 57, 97))):
            with self.subTest(name):
                source, _ = self.read(flipped_block(same_sense=flipped, bounds=all_faces))
                self.assertEqual(self.records(source), {})
                self.assertEqual({f.id for f in source.faces if f.status == FACE_REFUSED}, set(all_faces))
                shell, = [d for d in source.diagnostics if d.stage == 'source_shell_orientation_refused']
                values = {v.name: v.value for v in shell.values}
                self.assertEqual((shell.source_ids, values['shell_edges_used_once_each_way']), ((15, 16), 1.))
                self.assertLess(values['bound_signed_volume_mm3'], -999.)

    def test_an_inside_out_shell_with_one_face_restored_refuses_the_shell(self):
        """All but #153 inside out: no face contradicts its bound, the edges are not each
        used once each way, and the raw uses enclose the cube negatively."""
        all_faces = [face for face, _ in BLOCK_FACES]
        source, _ = self.read(flipped_block(same_sense=all_faces[:-1], bounds=all_faces[:-1]))
        # #153's four edges are each used twice the same way: the shell is not closed, so
        # every face is refused, #141 too (coordinator ruling on round 2, finding 2).
        self.assertEqual(self.refused_ids(source), [17, 57, 97, 119, 141, 153])
        self.assertEqual(shell_edge_uses(source), [(0., 0., 4.)])
        self.assertEqual(self.records(source), {})

    def cone(self, edit=None):
        text = (FIXTURES/'apex-cone.step').read_text()
        return text if edit is None else substituted(text, *edit)

    def test_the_apex_cone_is_decided_without_the_pole_wire(self):
        """The cone face #30 (base circle and apex vertex loop) is a pole face: its side is
        its edge loop's about the apex's iso-line. The owned volume of the as-read cone is
        pi 25 10 / 3 = 261.80 (OCCT's is 174.53); nothing is corrected or refused."""
        source, _ = self.read(self.cone())
        self.assertEqual(self.records(source), {})
        self.assertEqual(self.refused_ids(source), [])
        self.assertEqual({f.id: f.orientation for f in source.faces}, {17: 1, 30: 1})

    def test_an_inward_pole_face_is_corrected_and_verified(self):
        """Before: #30 set to .F. was written -1 with no record (the face was never tested).
        After: its loop gives the outward side, the cone's owned signed volume verifies it,
        and an S1 record says so; the same for the flipped base face #17."""
        for face, edit in ((30, ("#30 = ADVANCED_FACE('',(#31,#38),#33,.T.);", "#30 = ADVANCED_FACE('',(#31,#38),#33,.F.);")),
                           (17, ("#17 = ADVANCED_FACE('',(#18),#26,.T.);", "#17 = ADVANCED_FACE('',(#18),#26,.F.);"))):
            with self.subTest(face):
                source, _ = self.read(self.cone(edit))
                self.assertEqual({f.id: f.orientation for f in source.faces}, {17: 1, 30: 1})
                record = self.records(source)[face]
                self.assertEqual((record['raw_sense'], record['written_sense']), (-1., 1.))
                self.assertAlmostEqual(record['bound_signed_volume_mm3'], math.pi*25*10/3, places=6)
                self.assertEqual(set(self.records(source)), {face})

    def test_natural_faces_inside_out_are_turned_by_their_owned_volume(self):
        """A sphere and a closed B-spline bounded by one vertex loop each: no bound decides
        their side; the owned quadrature over the natural domain does (4/3 pi 125, and the
        B-spline's own volume). Flipped, each is turned (S2) with a record."""
        vertex = ingress.VertexLoopTests('test_sole_vertex_loop_on_a_closed_bspline_is_admitted')
        for name, text, expected in (('sphere', vertex.sphere(5.), 4/3*math.pi*125), ('closed B-spline', bspline_closed_in_v()[0], None)):
            with self.subTest(name):
                plain, _ = self.read(text)
                self.assertEqual((self.records(plain), self.refused_ids(plain)), ({}, []))
                flipped = substituted(text, "#30 = ADVANCED_FACE('',(#38),#33,.T.);", "#30 = ADVANCED_FACE('',(#38),#33,.F.);")
                source, _ = self.read(flipped)
                record = self.records(source)[30]
                self.assertEqual((record['raw_sense'], record['written_sense']), (-1., 1.))
                if expected:
                    self.assertAlmostEqual(-record['raw_signed_volume_mm3'], expected, places=6)

    def test_a_void_inside_out_is_turned(self):
        """void-block with its void shell's six faces flipped in same_sense and bound: the
        void encloses positively, so all six are turned (S2) and the table volume is 936."""
        text = (FIXTURES/'void-block.step').read_text()
        for face in (167, 207, 247, 269, 291, 303):
            line = next(l for l in text.splitlines() if l.startswith(f'#{face} = ADVANCED_FACE('))
            text = substituted(text, line, line[:-5] + ('.T.);' if line.endswith('.F.);') else '.F.);'))
            bound = next(l for l in text.splitlines() if l.startswith(f'#{face+1} = FACE_BOUND('))
            text = substituted(text, bound, bound[:-5] + ('.T.);' if bound.endswith('.F.);') else '.F.);'))
        source, _ = self.read(text)
        self.assertAlmostEqual(ingress.table_volume(source), 936., places=9)
        self.assertEqual(set(self.records(source)), {167, 207, 247, 269, 291, 303})


def lens():
    """apex-cone.step rebuilt as a lens: its base face #17 on a flat bilinear B-spline #26
    over [-6,6]^2 (u along x, v along -y), its other face #30 (now without the apex
    vertex loop, same_sense .F.) on the biquadratic dome #33, z = (25 - x^2 - y^2)/10 in
    the same parameters, and its one edge #21 on a closed rational quadratic B-spline
    circle of radius 5 #961, with the file's own pcurve on each surface: the same circle
    as a closed rational 2D B-spline through u = (x+6)/12, v = (6-y)/12, so every curve
    shares the 3D curve's parameter. The dome encloses 2 pi 156.25 / 10 mm^3."""
    w = math.sqrt(.5)
    ring = ((5., 0.), (5., 5.), (0., 5.), (-5., 5.), (-5., 0.), (-5., -5.), (0., -5.), (5., -5.), (5., 0.))
    weights = ','.join(repr(1. if i % 2 == 0 else w) for i in range(9))
    def curve(entity, first):
        ids = ','.join(f'#{first+i}' for i in range(9))
        return (f"#{entity} = ( BOUNDED_CURVE() B_SPLINE_CURVE(2,({ids}),.CIRCULAR_ARC.,.T.,.F.) "
                "B_SPLINE_CURVE_WITH_KNOTS((3,2,2,2,3),(0.,0.25,0.5,0.75,1.),.UNSPECIFIED.) CURVE() "
                f"GEOMETRIC_REPRESENTATION_ITEM() RATIONAL_B_SPLINE_CURVE(({weights})) REPRESENTATION_ITEM('') );")
    text = (FIXTURES/'apex-cone.step').read_text()
    text = substituted(text, "#21 = EDGE_CURVE('',#22,#22,#24,.T.);", "#21 = EDGE_CURVE('',#22,#22,#960,.T.);")
    text = substituted(text, "#26 = PLANE('',#27);",
                       "#26 = B_SPLINE_SURFACE_WITH_KNOTS('',1,1,((#950,#951),(#952,#953)),.UNSPECIFIED.,.F.,.F.,.F.,"
                       "(2,2),(2,2),(0.,1.),(0.,1.),.UNSPECIFIED.);")
    text = substituted(text, "#30 = ADVANCED_FACE('',(#31,#38),#33,.T.);", "#30 = ADVANCED_FACE('',(#31),#33,.F.);")
    dome = [[f'#{1000+3*i+j}' for j in range(3)] for i in range(3)]
    text = substituted(text, "#33 = CONICAL_SURFACE('',#34,5.,0.4636476090008061);",
                       f"#33 = B_SPLINE_SURFACE_WITH_KNOTS('',2,2,({','.join('(' + ','.join(r) + ')' for r in dome)}),.UNSPECIFIED.,.F.,.F.,.F.,"
                       "(3,3),(3,3),(0.,1.),(0.,1.),.UNSPECIFIED.);")
    a = (36., -36., 36.)
    records = ("#950 = CARTESIAN_POINT('',(-6.,6.,0.));\n#951 = CARTESIAN_POINT('',(-6.,-6.,0.));\n"
               "#952 = CARTESIAN_POINT('',(6.,6.,0.));\n#953 = CARTESIAN_POINT('',(6.,-6.,0.));\n"
               "#960 = SURFACE_CURVE('',#961,(#962,#966),.PCURVE_S1.);\n" + curve(961, 970) + "\n"
               "#962 = PCURVE('',#26,#963);\n#963 = DEFINITIONAL_REPRESENTATION('',(#964),#965);\n" + curve(964, 980) + "\n"
               "#965 = ( GEOMETRIC_REPRESENTATION_CONTEXT(2) PARAMETRIC_REPRESENTATION_CONTEXT() REPRESENTATION_CONTEXT('2D SPACE','') );\n"
               "#966 = PCURVE('',#33,#967);\n#967 = DEFINITIONAL_REPRESENTATION('',(#968),#965);\n" + curve(968, 990) + "\n")
    records += ''.join(f"#{970+i} = CARTESIAN_POINT('',({x!r},{y!r},0.));\n" for i, (x, y) in enumerate(ring))
    records += ''.join(f"#{980+i} = CARTESIAN_POINT('',({(x+6)/12!r},{(6-y)/12!r}));\n" for i, (x, y) in enumerate(ring))
    records += ''.join(f"#{990+i} = CARTESIAN_POINT('',({(x+6)/12!r},{(6-y)/12!r}));\n" for i, (x, y) in enumerate(ring))
    records += ''.join(f"#{1000+3*i+j} = CARTESIAN_POINT('',({-6.+6*i!r},{6.-6*j!r},{(25-a[i]-a[j])/10!r}));\n" for i in range(3) for j in range(3))
    return appended(text, records), ring, w


def moved(text, dx, first=17, last=None):
    """Every CARTESIAN_POINT from #first (to #last) moved by dx along x: the frames of the
    representation itself (#12 and below) stay, so the geometry moves within it."""
    def move(m):
        n = int(m.group(1))
        if n < first or (last is not None and n > last):
            return m.group(0)
        return f"#{n} = CARTESIAN_POINT('',(" + repr(float(m.group(2)) + dx) + ","
    return re.sub(r"#(\d+) = CARTESIAN_POINT\('',\(([-0-9.E+]+),", move, text)


PLANE_POLE = ("#942 = FACE_BOUND('',#943,.T.);\n#943 = VERTEX_LOOP('',#944);\n#944 = VERTEX_POINT('',#945);\n"
              "#945 = CARTESIAN_POINT('',(0.,5.,5.));\n")


def with_plane_pole(text):
    """Face #17 (the x = 0 plane) given a vertex loop as well: OCCT drops it, so the
    adapter refuses that face (reason 1, dropped_by_occt) before its orientation."""
    out = re.sub(r"#17 = ADVANCED_FACE\('',\(#18\),#52,\.(T|F)\.\);", lambda m: "#17 = ADVANCED_FACE('',(#18,#942),#52,." + m.group(1) + ".);", text)
    assert out != text
    return appended(out, PLANE_POLE)


class OrientationRound2Tests(unittest.TestCase, _Adapter):
    """Review round 2, findings 1, 2 and 5 (attacks round2.py, round2d.py, round2e/g.py)."""

    read = OrientationTests.read
    records = OrientationTests.records

    def statuses(self, source):
        from brepgpu.imported_source import FACE_REFUSED
        return sorted(f.id for f in source.faces if f.status == FACE_REFUSED)

    def test_a_refused_face_refuses_its_whole_shell(self):
        """Finding 1. With a face refused, the shell's volume is no evidence, and a bound
        agreeing with same_sense does not show the shell outward. Before: the inside-out
        cube with face #17 refused was written inward (volume -1000) with no record."""
        all_faces = [face for face, _ in BLOCK_FACES]
        for name, text in (('inside out', flipped_block(same_sense=all_faces, bounds=all_faces)), ('correct', ingress.plain_block())):
            with self.subTest(name):
                source, _ = self.read(with_plane_pole(text))
                self.assertEqual(self.statuses(source), sorted(all_faces))
                self.assertEqual(self.records(source), {})
                self.assertEqual([d.source_ids for d in source.diagnostics if d.stage == 'source_shell_orientation_refused'], [(15, 16)])
        cone = substituted((FIXTURES/'apex-cone.step').read_text(), "#41 = CARTESIAN_POINT('',(0.,0.,10.));", "#41 = CARTESIAN_POINT('',(0.,0.,9.));")
        source, _ = self.read(cone)
        self.assertEqual(self.statuses(source), [17, 30])

    def test_closed_shells_far_away_are_verified_about_their_centre(self):
        """Finding 2. The cube moved 1000 mm: S1 on face #57 and on #17, and S2, are written
        and verified (the shares are taken about the shell's box centre)."""
        all_faces = [face for face, _ in BLOCK_FACES]
        for name, text, corrected in (('S1 #57', flipped_block(same_sense=(57,)), {57}), ('S1 #17', flipped_block(same_sense=(17,)), {17}),
                                      ('S2', flipped_block(same_sense=all_faces, bounds=all_faces), set(all_faces))):
            with self.subTest(name):
                source, _ = self.read(moved(text, 1000.))
                self.assertAlmostEqual(ingress.table_volume(source), 1000., places=6)
                self.assertEqual(set(self.records(source)), corrected)
                self.assertEqual(self.statuses(source), [])

    def test_open_shells_are_decided_by_the_propositions_alone(self):
        """Finding 2, with the coordinator's ruling. A CLOSED_SHELL with face #17 or #57
        left out uses four edges once: it fails the closure proposition, violates the
        schema and has no material side, so every face of it is refused by name with the
        shell's record (edges used once 4), the definition incomplete, at any position and
        whether its faces are correct or all inside out (which face-local tests cannot
        tell apart). HEAD admitted these. With #17 out and #97 flipped, #97's three shared
        edges are also used twice the same way."""
        from brepgpu.imported_source import DEFINITION_INCOMPLETE
        all_faces = [face for face, _ in BLOCK_FACES]
        shell = "#16 = CLOSED_SHELL('',(#17,#57,#97,#119,#141,#153));"
        for left_out in (17, 57):
            for dx in (0., 1000.):
                for name, text in (('correct', ingress.plain_block()), ('inside out', flipped_block(same_sense=all_faces, bounds=all_faces))):
                    with self.subTest(left_out=left_out, dx=dx, name=name):
                        text = substituted(text, shell, shell.replace(f"#{left_out},", "").replace(f",#{left_out})", ")"))
                        source, run = self.read(moved(text, dx))
                        self.assertEqual((self.records(source), self.statuses(source)), ({}, sorted(set(all_faces) - {left_out})))
                        self.assertEqual(shell_edge_uses(source), [(4., 0., 0.)])
                        self.assertEqual([d.status for d in source.definitions], [DEFINITION_INCOMPLETE])
                        self.assertIn('source_shells_not_closed_consistent\t1\n', run.stdout)
        text = substituted(flipped_block(same_sense=(97,), bounds=(97,)), shell, shell.replace("#17,", ""))
        source, _ = self.read(text)
        self.assertEqual(self.statuses(source), [57, 97, 119, 141, 153])
        self.assertEqual(shell_edge_uses(source), [(4., 0., 3.)])

    def test_a_pole_loop_that_does_not_wind_the_pole_refuses(self):
        """Finding 5. half-cone.step: the lateral face #17's edge loop is the base half-arc
        and the two generators through the apex, so it does not wind around the apex
        (its u changes by pi, not 2 pi); the region between it and the pole is not
        determined. Before: its side was judged from the wrong region and admitted."""
        source, _ = self.read((FIXTURES/'half-cone.step').read_text())
        self.assertEqual(self.statuses(source), [17, 18, 19])
        lateral, = [d for d in source.diagnostics if d.stage == 'source_face_refused' and d.source_ids[0] == 17]
        self.assertIn('does not wind once around the pole', lateral.message)


BLOCK_SHELL = "#16 = CLOSED_SHELL('',(#17,#57,#97,#119,#141,#153));"


def block_copy(scale, offset, inverted=False):
    """The plain block's faces #17..#164 renumbered +1000, scaled about the origin and
    shifted by offset; with inverted, every face's same_sense and bound reversed (the
    copy inside out)."""
    out = []
    for line in ingress.plain_block().split('\n'):
        m = re.match(r'#(\d+) = ', line)
        if not m or not 17 <= int(m.group(1)) <= 164:
            continue
        line = re.sub(r'#(\d+)', lambda k: f'#{int(k.group(1)) + 1000}' if 17 <= int(k.group(1)) <= 164 else k.group(0), line)
        line = re.sub(r"CARTESIAN_POINT\('',\(([-0-9.E+]+),([-0-9.E+]+),([-0-9.E+]+)\)\)",
                      lambda k: "CARTESIAN_POINT('',(%r,%r,%r))" % tuple(float(k.group(i + 1)) * scale + offset[i] for i in range(3)), line)
        if inverted:
            line = re.sub(r"(ADVANCED_FACE\(.*?,)\.(T|F)\.\);", lambda k: k.group(1) + ('.F.);' if k.group(2) == 'T' else '.T.);'), line)
            line = re.sub(r"(FACE_BOUND\('',#\d+,)\.(T|F)\.\);", lambda k: k.group(1) + ('.F.);' if k.group(2) == 'T' else '.T.);'), line)
        out.append(line)
    return '\n'.join(out) + '\n'


def two_parts(scale, offset, small_inverted=False, big_inverted=False):
    """One CLOSED_SHELL holding the 10 mm cube and a disjoint (or nested) scaled copy."""
    all_faces = [face for face, _ in BLOCK_FACES]
    base = flipped_block(same_sense=all_faces, bounds=all_faces) if big_inverted else ingress.plain_block()
    text = substituted(base, BLOCK_SHELL, BLOCK_SHELL.replace('));', ',' + ','.join(f'#{f + 1000}' for f in all_faces) + '));'))
    return appended(text, block_copy(scale, offset, small_inverted))


APEX_FACE = "#30 = ADVANCED_FACE('',(#31,#38),#33,.T.);"


def open_cone():
    """apex-cone.step without its apex vertex loop: the lateral face is bounded by the
    base circle alone, and its region reaches the apex (review round 3, finding 2)."""
    return substituted((FIXTURES/'apex-cone.step').read_text(), APEX_FACE, "#30 = ADVANCED_FACE('',(#31),#33,.T.);")


def hemisphere(pole_loop=False):
    """The reviewer's hemisphere (attacks/hemi.py): apex-cone.step's cone replaced by a
    sphere of radius 5 about the origin, bounded by the equator alone, or with
    pole_loop by the equator and a vertex loop at its pole (0, 0, 5)."""
    text = (FIXTURES/'apex-cone.step').read_text() if pole_loop else open_cone()
    text = substituted(text, "#33 = CONICAL_SURFACE('',#34,5.,0.4636476090008061);", "#33 = SPHERICAL_SURFACE('',#34,5.);")
    return substituted(text, "#41 = CARTESIAN_POINT('',(0.,0.,10.));", "#41 = CARTESIAN_POINT('',(0.,0.,5.));") if pole_loop else text


def turned(text, faces, bounds=True):
    """The listed faces' same_sense reversed, and with bounds their bounds' orientations."""
    for face in faces:
        line = re.search(rf"#{face} = ADVANCED_FACE\('',\(([#0-9,]+)\),#(\d+),\.(T|F)\.\);", text)
        text = substituted(text, line.group(0), line.group(0)[:-5] + ('.F.);' if line.group(3) == 'T' else '.T.);'))
        if bounds:
            for bound in line.group(1).split(','):
                b = re.search(rf"{bound} = (FACE_BOUND|FACE_OUTER_BOUND)\('',#(\d+),\.(T|F)\.\);", text)
                text = substituted(text, b.group(0), b.group(0)[:-5] + ('.F.);' if b.group(3) == 'T' else '.T.);'))
    return text


def with_window(text, sphere):
    """A hole in the lateral face #30 (the cone's, or the sphere's), bounded by two
    latitude arcs and two generators (cone) or meridian arcs (sphere) at angles 0.5 and
    1.5 rad, and closed by a window face #790 on the same carrier #33. The window loop
    runs counter-clockwise about the outward normal, the hole loop the other way."""
    th = (0.5, 1.5)
    zs = (1., 3.) if sphere else (2., 6.)
    radius = (lambda z: math.sqrt(25. - z * z)) if sphere else (lambda z: 5. - z / 2.)
    records, n = [], [700]

    def ent(body):
        records.append(f'#{n[0]} = {body};')
        n[0] += 1
        return n[0] - 1
    vertex = {}
    for z in zs:
        for t in th:
            vertex[z, t] = ent(f"VERTEX_POINT('',#{ent(f'CARTESIAN_POINT({chr(39)*2},({radius(z)*math.cos(t)!r},{radius(z)*math.sin(t)!r},{z!r}))')})")
    up, east = ent("DIRECTION('',(0.,0.,1.))"), ent("DIRECTION('',(1.,0.,0.))")
    arc = {}
    for z in zs:
        frame = ent(f"AXIS2_PLACEMENT_3D('',#{ent(f'CARTESIAN_POINT({chr(39)*2},(0.,0.,{z!r}))')},#{up},#{east})")
        arc[z] = ent(f"EDGE_CURVE('',#{vertex[z, th[0]]},#{vertex[z, th[1]]},#{ent(f'CIRCLE({chr(39)*2},#{frame},{radius(z)!r})')},.T.)")
    side = {}
    for t in th:
        if sphere:
            frame = ent(f"AXIS2_PLACEMENT_3D('',#{ent(f'CARTESIAN_POINT({chr(39)*2},(0.,0.,0.))')},#{ent(f'DIRECTION({chr(39)*2},({math.sin(t)!r},{-math.cos(t)!r},0.))')},#{ent(f'DIRECTION({chr(39)*2},({math.cos(t)!r},{math.sin(t)!r},0.))')})")
            curve = ent(f"CIRCLE('',#{frame},5.)")
        else:
            p0 = (radius(zs[0]) * math.cos(t), radius(zs[0]) * math.sin(t), zs[0])
            p1 = (radius(zs[1]) * math.cos(t), radius(zs[1]) * math.sin(t), zs[1])
            d = [b - a for a, b in zip(p0, p1)]
            length = math.sqrt(sum(x * x for x in d))
            point = ent("CARTESIAN_POINT('',(%r,%r,%r))" % p0)
            direction = ent("DIRECTION('',(%r,%r,%r))" % tuple(x / length for x in d))
            curve = ent(f"LINE('',#{point},#{ent(f'VECTOR({chr(39)*2},#{direction},1.)')})")
        side[t] = ent(f"EDGE_CURVE('',#{vertex[zs[0], t]},#{vertex[zs[1], t]},#{curve},.T.)")
    use = lambda edge, sense: ent(f"ORIENTED_EDGE('',*,*,#{edge},{sense})")
    window = ent(f"EDGE_LOOP('',(#{use(arc[zs[0]], '.T.')},#{use(side[th[1]], '.T.')},#{use(arc[zs[1]], '.F.')},#{use(side[th[0]], '.F.')}))")
    hole = ent(f"EDGE_LOOP('',(#{use(side[th[0]], '.T.')},#{use(arc[zs[1]], '.T.')},#{use(side[th[1]], '.F.')},#{use(arc[zs[0]], '.F.')}))")
    hole_bound = ent(f"FACE_BOUND('',#{hole},.T.)")
    assert n[0] < 790
    records.append(f"#790 = ADVANCED_FACE('',(#791),#33,.T.);\n#791 = FACE_OUTER_BOUND('',#{window},.T.);")
    face = re.search(r"#30 = ADVANCED_FACE\('',\(([#0-9,]+)\),#33,\.T\.\);", text).group(0)
    text = substituted(text, face, face.replace("),#33", f",#{hole_bound}),#33"))
    text = substituted(text, "#16 = CLOSED_SHELL('',(#17,#30));", "#16 = CLOSED_SHELL('',(#17,#30,#790));")
    return appended(text, '\n'.join(records) + '\n')


class OrientationRound3Tests(unittest.TestCase, _Adapter):
    """Review round 3: each edge-connected part of a shell verified on its own (finding
    1); a face whose edge loop winds a surface pole with no vertex loop there decided as a
    pole face (finding 2) with its UV hint reaching the pole (finding 3); pole faces with
    holes (finding 5). Before each fix, on the build before this round, these fail."""

    read = OrientationTests.read
    records = OrientationTests.records
    statuses = OrientationRound2Tests.statuses

    def written(self, source):
        return {f.id: f.orientation for f in source.faces}

    def sides(self, run):
        return [line for line in run.stdout.splitlines() if line.startswith('source_face_sides\t')]

    def test_each_part_of_a_shell_is_verified_on_its_own(self):
        """Finding 1 (attacks/round3.py). The cube and a half-size copy 20 mm away in one
        CLOSED_SHELL. Both correct: admitted as read. The copy inside out: only the copy
        is turned (S2), about its own box centre. The cube inside out and the copy correct:
        only the cube is turned. Before: one volume sign over both parts admitted the
        inward copy, and turned the correct copy inward with the cube. Each case records
        the shell's two components."""
        all_faces = [face for face, _ in BLOCK_FACES]
        small = [f + 1000 for f in all_faces]
        for name, text, turned_faces in (('both correct', two_parts(.5, (20., 0., 0.)), []),
                                         ('copy inside out', two_parts(.5, (20., 0., 0.), small_inverted=True), small),
                                         ('cube inside out', two_parts(.5, (20., 0., 0.), big_inverted=True), all_faces)):
            with self.subTest(name):
                source, run = self.read(text)
                self.assertEqual((sorted(self.records(source)), self.statuses(source)), (sorted(turned_faces), []))
                parts, = [d for d in source.diagnostics if d.stage == 'source_shell_components']
                self.assertEqual((parts.source_ids, {v.name: v.value for v in parts.values}), ((15, 16, 17, 1017), {'components': 2.}))
                # Every written side is outward: the cube's 1000 mm^3 plus the copy's 125.
                self.assertAlmostEqual(ingress.table_volume(source), 1125., places=9)

    def test_a_part_inside_another_parts_box_is_refused(self):
        """Finding 1, nested: an inward half-size copy inside the cube, in the same shell.
        It may be a cavity written into the outer shell (then inward is right) or an
        inside-out island; its role does not fix its sign, so its faces are refused by
        name, and the cube, verified on its own, stands. Before: admitted as read."""
        source, _ = self.read(two_parts(.5, (2.5, 2.5, 2.5), small_inverted=True))
        self.assertEqual(self.statuses(source), sorted(f + 1000 for f, _ in BLOCK_FACES))
        shell, = [d for d in source.diagnostics if d.stage == 'source_shell_orientation_refused']
        self.assertIn("within another component's box", shell.message)
        self.assertEqual({v.name: v.value for v in shell.values}['component_first_face'], 1017.)

    def test_the_hemisphere_bounded_by_its_equator_is_decided(self):
        """Finding 2 (attacks/hemi.py). The sphere face's equator winds both poles; its
        same_sense and bound together read the region with the pole at (0, 0, 5) and the
        outward side, and the owned Green sums from that pole's iso-line give the volume
        that verifies it: admitted as read, a pole face. Before: OCCT's volume of 0 refused
        the shell. Inside out, or with only its same_sense reversed, the raw uses are not
        verified, and turning every face and correcting the sphere face alone (which also
        moves its region to the other pole) are both readings: refused by name."""
        source, run = self.read(hemisphere())
        self.assertEqual((self.statuses(source), self.records(source)), ([], {}))
        self.assertIn('source_face_sides\tpole\t1\t0', self.sides(run))
        for name, text in (('inside out', turned(hemisphere(), (17, 30))), ('same_sense reversed', turned(hemisphere(), (30,), bounds=False))):
            with self.subTest(name):
                source, _ = self.read(text)
                self.assertEqual(self.statuses(source), [17, 30])
                shell, = [d for d in source.diagnostics if d.stage == 'source_shell_orientation_refused']
                self.assertIn('two poles', shell.message)

    def test_a_cone_without_its_apex_vertex_loop_is_decided(self):
        """Finding 2: the cone bounded by its base circle alone. Its only pole is the apex
        (the other side is unbounded), so its region is the apex's: admitted as read.
        Inside out: both faces turned (S2). Its same_sense alone reversed: it contradicts
        its bound, and the bound side is written (S1). Before: decided from OCCT's
        classifier and volume, which do not see the apex."""
        source, run = self.read(open_cone())
        self.assertEqual((self.statuses(source), self.records(source)), ([], {}))
        self.assertIn('source_face_sides\tpole\t1\t0', self.sides(run))
        source, _ = self.read(turned(open_cone(), (17, 30)))
        self.assertEqual((self.statuses(source), sorted(self.records(source))), ([], [17, 30]))
        self.assertEqual(self.written(source), {17: 1, 30: 1})
        source, _ = self.read(turned(open_cone(), (30,), bounds=False))
        self.assertEqual((self.statuses(source), sorted(self.records(source))), ([], [30]))
        self.assertEqual(self.written(source), {17: 1, 30: 1})

    def test_the_uv_hint_of_a_face_winding_a_pole_reaches_the_pole(self):
        """Finding 3: the hemisphere's hint was v in +-2e-8 (its equator only) while the
        face reaches its pole at v = -pi/2; the open cone's reaches the apex at
        v = -5 / sin(semi-angle). Consumers' UV boxes and the face's extent are now sound."""
        from brepgpu.imported_source import load_source
        for name, text, pole in (('hemisphere', hemisphere(), -math.pi / 2), ('open cone', open_cone(), -5. / math.sin(0.4636476090008061))):
            with self.subTest(name), tempfile.TemporaryDirectory() as directory:
                run, _, target = self.extract(text, directory)
                self.assertEqual(run.returncode, 0, run.stderr)
                u0, u1, v0, v1 = load_source(target).carrier(33).uv_bounds
                self.assertLessEqual(v0, pole + 1e-12)
                self.assertGreaterEqual(v1, 0.)
                self.assertGreaterEqual(u1 - u0, 2 * math.pi - 1e-12)

    def two_winding_loops(self):
        """The drill-point cone with a second full circle at z = 5 in its bounds, closed by
        a disc there: two loops wind the apex."""
        text = substituted((FIXTURES/'apex-cone.step').read_text(), APEX_FACE, "#30 = ADVANCED_FACE('',(#31,#38,#707),#33,.T.);")
        text = substituted(text, "#16 = CLOSED_SHELL('',(#17,#30));", "#16 = CLOSED_SHELL('',(#17,#30,#710));")
        return appended(text, "#700 = CARTESIAN_POINT('',(2.5,0.,5.));\n#701 = VERTEX_POINT('',#700);\n#702 = CARTESIAN_POINT('',(0.,0.,5.));\n"
                              "#703 = AXIS2_PLACEMENT_3D('',#702,#13,#14);\n#704 = CIRCLE('',#703,2.5);\n#705 = EDGE_CURVE('',#701,#701,#704,.T.);\n"
                              "#706 = EDGE_LOOP('',(#708));\n#707 = FACE_BOUND('',#706,.T.);\n#708 = ORIENTED_EDGE('',*,*,#705,.F.);\n"
                              "#709 = ORIENTED_EDGE('',*,*,#705,.T.);\n#710 = ADVANCED_FACE('',(#711),#713,.T.);\n#711 = FACE_OUTER_BOUND('',#712,.T.);\n"
                              "#712 = EDGE_LOOP('',(#709));\n#713 = PLANE('',#703);\n")

    def test_pole_faces_with_a_hole_stop_at_the_multiple_wire_conversion(self):
        """Finding 5, as far as it is reachable. A drill-point cone (apex vertex loop) and a
        spherical cap (with and without a vertex loop at its pole), each with a window cut
        into the face and closed by a window face on the same carrier, and the drill-point
        cone with a second winding loop. The adapter now combines non-winding holes in the
        Green sums and refuses several winding loops by name (pole_winding), but no file
        reaches that: the source-UV stage refuses every nonplanar face with more than one
        edge loop for the whole file, as at HEAD ("unsupported nonplanar multiple-wire
        conversion", 39 corpus files). These fixtures become the decided cases when that
        conversion lands."""
        for name, text in (('drill point', with_window((FIXTURES/'apex-cone.step').read_text(), sphere=False)),
                           ('cap with pole loop', with_window(hemisphere(pole_loop=True), sphere=True)),
                           ('cap', with_window(hemisphere(), sphere=True)),
                           ('two winding loops', self.two_winding_loops())):
            with self.subTest(name), tempfile.TemporaryDirectory() as directory:
                run, _, _ = self.extract(text, directory)
                self.assertEqual(run.returncode, 1)
                self.assertIn('source UV: face 30 carrier 33: unsupported nonplanar multiple-wire conversion', run.stderr)


def with_cube(text, scale, offset, inverted):
    """text (a two-face solid, shell #16 of #17 and #30) with a scaled copy of the cube
    added to the same CLOSED_SHELL (attacks/round4.py)."""
    all_faces = [face for face, _ in BLOCK_FACES]
    text = substituted(text, "#16 = CLOSED_SHELL('',(#17,#30));", "#16 = CLOSED_SHELL('',(#17,#30," + ','.join(f'#{f + 1000}' for f in all_faces) + '));')
    return appended(text, block_copy(scale, offset, inverted))


def truncated_sphere():
    """The reviewer's truncated sphere (attacks/trunc.py, x axis): a sphere of radius 5
    about the x axis cut by the plane z = -3, the cut a circle of radius 4 bounding both
    faces. The circle winds neither of the sphere's poles (at x = +-5), so the sphere
    face's same_sense chooses between the large and the small cap."""
    text = substituted((FIXTURES/'apex-cone.step').read_text(), "#23 = CARTESIAN_POINT('',(5.,0.,0.));", "#23 = CARTESIAN_POINT('',(4.,0.,-3.));")
    text = substituted(text, "#24 = CIRCLE('',#25,5.);", "#24 = CIRCLE('',#970,4.);")
    text = substituted(text, "#26 = PLANE('',#27);", "#26 = PLANE('',#972);")
    text = substituted(text, APEX_FACE, "#30 = ADVANCED_FACE('',(#31),#33,.T.);")
    text = substituted(text, "#33 = CONICAL_SURFACE('',#34,5.,0.4636476090008061);", "#33 = SPHERICAL_SURFACE('',#973,5.);")
    return appended(text, "#970 = AXIS2_PLACEMENT_3D('',#971,#13,#14);\n#971 = CARTESIAN_POINT('',(0.,0.,-3.));\n#972 = AXIS2_PLACEMENT_3D('',#971,#28,#14);\n"
                          "#973 = AXIS2_PLACEMENT_3D('',#12,#974,#975);\n#974 = DIRECTION('',(1.,0.,0.));\n#975 = DIRECTION('',(0.,0.,1.));\n")


FIXTURE_RUNS = {}


class OrientationRound4Tests(unittest.TestCase, _Adapter):
    """Review round 4: nesting tested against boxes that hold each part's faces (item 1);
    faces on closed surfaces whose same_sense chooses their region (item 2)."""

    read = OrientationTests.read
    records = OrientationTests.records
    statuses = OrientationRound2Tests.statuses

    def test_a_part_under_a_dome_or_apex_is_nested(self):
        """Item 1 (attacks/round4.py N1-N4). A 1 mm cube in the same CLOSED_SHELL under the
        hemisphere's dome or the cone's apex: as a cavity skin (inward), and under the dome
        as an outward island. The parts' edge boxes are flat disks at z = 0, which held no
        point of the cube, so S2 turned the skin outward with records. The faces' extents
        reach the pole and the apex: the cube's point lies within them, so it may be nested,
        its sign is not determined, and its faces are refused by name; the dome or cone,
        verified on its own, stands."""
        cube = sorted(f + 1000 for f, _ in BLOCK_FACES)
        cone = (FIXTURES/'apex-cone.step').read_text()
        for name, text in (('N1 cavity under the dome', with_cube(hemisphere(), .1, (-.5, -.5, 1.5), True)),
                           ('N2 island under the dome', with_cube(hemisphere(), .1, (-.5, -.5, 1.5), False)),
                           ('N3 cavity under the apex', with_cube(cone, .1, (-.5, -.5, 1.5), True)),
                           ('N4 cavity on the cone base', with_cube(cone, .1, (-.5, -.5, 0.), True))):
            with self.subTest(name):
                source, _ = self.read(text)
                self.assertEqual((self.statuses(source), self.records(source)), (cube, {}))
                shell, = [d for d in source.diagnostics if d.stage == 'source_shell_orientation_refused']
                self.assertIn("within another component's box", shell.message)
                self.assertEqual({v.name: v.value for v in shell.values}['component_first_face'], 1017.)

    def fixture(self):
        executable = Path(os.environ.get('BREPGPU_FACE_REGIONS_FIXTURE',
                                         Path(__file__).resolve().parents[1]/'build/om10-step-source/brepgpu-face-regions-fixture')).resolve()
        # One run per process: its cases are deterministic, and its contact cases take seconds.
        if executable not in FIXTURE_RUNS:
            FIXTURE_RUNS[executable] = subprocess.run((str(executable),), capture_output=True, text=True)
        run = FIXTURE_RUNS[executable]
        self.assertEqual(run.returncode, 0, run.stderr)
        rows = [line.split('\t') for line in run.stdout.splitlines()]
        self.extra = {row[0]: row[1:] for row in rows if row[0] in ('whole', 'zone')}
        self.contacts = [row for row in rows if row[0] in ('contact', 'plane', 'uncertain', 'undecided', 'apex')]
        return ({row[1]: row[2] == '1' for row in rows if row[0] == 'closed'},
                {tuple(row[:3]): (row[3] == '1', row[4] == '1', float(row[5]), float(row[6]), row[7]) for row in rows if row[0] not in ('closed', 'whole', 'zone', 'contact', 'plane', 'uncertain', 'undecided', 'apex')})

    def test_same_sense_chooses_the_region_on_a_closed_surface(self):
        """Item 2, at function level (native/step_source/face_regions.hpp through
        tests/face_regions_fixture.cpp): no file reaches it, since the source-UV stage
        refuses these faces. On the x-axis sphere of radius 5 the circle z = -3 winds no
        pole. Its sense in UV and the face's same_sense together choose the large cap
        (flux about the centre 5 x 80 pi) or the small one (5 x 20 pi); the two are
        complements (sum 500 pi); never a contradiction. A loop whose uses do not join in
        one chart, a torus meridian alone (it winds and separates nothing) and a band run
        twice around are refused as region-ambiguous; a small loop on the torus and a band
        between two parallel circles are decided, the band on a torus by same_sense and on
        a sphere (whose complement is two caps) as the only reading. Spheres and tori are closed
        surfaces, cylinders and cones are not."""
        closed, cases = self.fixture()
        self.assertEqual(closed, {'sphere': True, 'torus': True, 'cylinder': False, 'cone': False})
        large, small = 400 * math.pi, 100 * math.pi
        for loop in ('forward', 'backward'):
            fluxes = []
            for same in ('T', 'F'):
                decided, inside, area, flux, why = cases['cap', loop, same]
                self.assertTrue(decided, why)
                fluxes.append(flux)
            # Owned quadrature over a projected pcurve: within 1e-6 relative of the exact caps.
            for got, want in zip(sorted(fluxes), (small, large)):
                self.assertAlmostEqual(got / want, 1., delta=1e-6)
        # Reversing the loop with same_sense reverses the side and keeps the region.
        self.assertEqual(cases['cap', 'forward', 'T'][3], cases['cap', 'backward', 'F'][3])
        self.assertEqual(cases['cap', 'split', 'T'][0], False)
        # Moved by a whole period, the second use joins once unwrapped: the same reading
        # (to quadrature: the loop is cut, so the Gauss nodes move).
        period, forward = cases['cap', 'period', 'T'], cases['cap', 'forward', 'T']
        self.assertEqual(period[:2], forward[:2])
        self.assertAlmostEqual(period[3] / forward[3], 1., delta=1e-7)
        self.assertIn('do not join in one chart', cases['cap', 'split', 'T'][4])
        self.assertTrue(cases['torus', 'square', 'T'][0])
        self.assertAlmostEqual(cases['torus', 'square', 'T'][2], 1., places=12)
        self.assertEqual(cases['torus', 'meridian', 'T'][0], False)
        self.assertIn('around the closed surface do not cancel', cases['torus', 'meridian', 'T'][4])
        # A torus band between two parallel circles: both bands are regions, one per
        # same_sense, complements of each other (their fluxes sum to the whole torus's,
        # 3 x 2 pi^2 R r^2). Twice around is no band.
        whole = float(self.extra['whole'][1])
        self.assertAlmostEqual(whole / (6 * math.pi ** 2 * 4.), 1., delta=1e-9)
        band = [cases['torus', 'band', same] for same in ('T', 'F')]
        self.assertTrue(all(b[0] for b in band))
        self.assertEqual(sorted(b[1] for b in band), [False, True])
        self.assertAlmostEqual(sum(b[2] for b in band), 4 * math.pi ** 2, places=9)
        self.assertAlmostEqual((band[0][3] + band[1][3]) / whole, 1., delta=1e-9)
        self.assertEqual(cases['torus', 'twice', 'T'][0], False)
        self.assertIn('other than once in one direction', cases['torus', 'twice', 'T'][4])
        # A sphere zone: its complement is two caps, so the zone is the only reading,
        # with its loops' sense (decided, unique); UV area 2 pi x 0.7.
        decided, unique, area, flux, why = self.extra['zone']
        self.assertEqual((decided, unique), ('1', '1'), why)
        self.assertAlmostEqual(abs(float(area)), 2 * math.pi * .7, places=9)

    def test_a_neighbour_is_held_to_contact_outside_its_strip(self):
        """Review round 7, at function level (face_contact.hpp region_meets through the
        fixture). A torus patch read as its complement, against a neighbour sharing one of
        its edges and tangent to the torus along it: the cylinder of radius 5 at the
        outer equator, or the plane z = 1 at the top circle. Over the common edge alone,
        the neighbour stays within the resolution of the complement only in the strip
        its curvatures bound (sqrt(2 resolution / 1) here): no meeting, no false contact.
        Continued past the patch's corner (an L shape, or a lobe beyond a bridge), it
        comes within h^2 / 2 of the complement about 1 to 10 mm from the common edge,
        without crossing it: h = 1e-4 to 4.2e-4 (gaps 5e-9 to 8.8e-8 mm) meets, h =
        4.6e-4 (1.06e-7, above the 1e-7 resolution) and 2e-3 do not; every neighbour's
        strip is bounded (none held to crossings only). Before: every neighbour sharing
        a vertex was held to crossings only, decided by BOPAlgo's fuzzy section, whose
        threshold counts the shapes' tolerances: the 1.06e-7 gap met."""
        self.fixture()
        rows = {(row[0], float(row[1]), round(float(row[2]), 6)): (int(row[3]), int(row[4])) for row in self.contacts}
        expected = {('contact', 1., 0.): 0, ('contact', 1.5, 0.): 1, ('contact', 1.5, 3e-4): 1, ('contact', 1.5, 4.6e-4): 0, ('contact', 1.5, 2e-3): 0,
                    ('plane', 1., 0.): 0, ('plane', 1.5, 1e-4): 1, ('plane', 1.5, 3e-4): 1, ('plane', 1.5, 4.2e-4): 1, ('plane', 1.5, 4.6e-4): 0, ('plane', 1.5, 2e-3): 0,
                    ('plane', -1., 1e-4): 1, ('plane', -1., 3e-4): 1, ('plane', -1., 4.6e-4): 0, ('plane', -1., 2e-3): 0,
                    # Round 8: near-contacts only just outside a strip are uncertain (2);
                    # a blind-band target the cells cannot reach leaves it undecided (3);
                    # an apex-cone neighbour (bounded by its base circle alone) meets at a
                    # 5e-8 gap at its apex and not at 1e-3.
                    ('uncertain', 1.0003, 3e-4): 2, ('undecided', 1., 0.): 3, ('apex', 5e-8, 0.): 1, ('apex', 1e-3, 0.): 0}
        self.assertEqual({k: v[0] for k, v in rows.items()}, expected)
        self.assertEqual({v[1] for v in rows.values()}, {0})
        # OCCT's own box of the apex face stops at its base (z = 2), missing the apex at
        # z = 1 + gap (finding 2: the neighbour's box is shape_extent's).
        self.assertTrue(all(float(row[5]) > 1.9 for row in self.contacts if row[0] == 'apex'))

    def test_the_x_axis_truncated_sphere_stops_in_source_uv(self):
        """Item 2: the reviewer's x-axis truncated sphere, the case the two-region rule is
        for, stops today at the source-UV stage for the whole file (its projected cut misses
        the circle by more than the grade), whatever its senses."""
        for sphere_sense in ('.T.', '.F.'):
            text = truncated_sphere()
            if sphere_sense == '.F.':
                text = turned(text, (30,), bounds=False)
            with self.subTest(sphere_sense), tempfile.TemporaryDirectory() as directory:
                run, _, _ = self.extract(text, directory)
                self.assertEqual(run.returncode, 1)
                self.assertIn('source UV: face 30 carrier 33 use 36: C/S residual', run.stderr)


class SolidBuilder:
    """Authors one manifold solid into apex-cone.step's product and context (its own
    solid #15..#59 removed), entities from #200 (the reviewer's construction in
    attacks/fillet.py)."""

    def __init__(self):
        self.records, self.next, self.vertices, self.lines = [], 200, {}, {}

    def ent(self, body):
        self.records.append(f'#{self.next} = {body};')
        self.next += 1
        return f'#{self.next - 1}'

    def pt(self, p):
        return self.ent("CARTESIAN_POINT('',(%r,%r,%r))" % tuple(float(c) for c in p))

    def dr(self, d):
        return self.ent("DIRECTION('',(%r,%r,%r))" % tuple(float(c) for c in d))

    def ax(self, o, z, x):
        return self.ent(f"AXIS2_PLACEMENT_3D('',{self.pt(o)},{self.dr(z)},{self.dr(x)})")

    def vertex(self, p):
        p = tuple(float(c) for c in p)
        if p not in self.vertices:
            self.vertices[p] = self.ent(f"VERTEX_POINT('',{self.pt(p)})")
        return self.vertices[p]

    def line(self, a, b):
        if (a, b) in self.lines:
            return self.lines[a, b], True
        if (b, a) in self.lines:
            return self.lines[b, a], False
        d = [b[i] - a[i] for i in range(3)]
        length = math.sqrt(sum(c * c for c in d))
        curve = self.ent(f"LINE('',{self.pt(a)},{self.ent(f'VECTOR({chr(39) * 2},{self.dr([c / length for c in d])},1.)')})")
        self.lines[a, b] = self.ent(f"EDGE_CURVE('',{self.vertex(a)},{self.vertex(b)},{curve},.T.)")
        return self.lines[a, b], True

    def circle(self, centre, axis, ref, radius):
        return self.ent(f"CIRCLE('',{self.ax(centre, axis, ref)},{float(radius)!r})")

    def arc(self, a, b, curve):
        return self.ent(f"EDGE_CURVE('',{self.vertex(a)},{self.vertex(b)},{curve},.T.)")

    def loop(self, uses):
        return self.ent("EDGE_LOOP('',(%s))" % ','.join(self.ent(f"ORIENTED_EDGE('',*,*,{e},{'.T.' if f else '.F.'})") for e, f in uses))

    def polygon(self, points):
        return [self.line(points[i], points[(i + 1) % len(points)]) for i in range(len(points))]

    def face(self, surface, loops, sense='.T.'):
        bounds = [self.ent(f"FACE_OUTER_BOUND('',{self.loop(loops[0])},.T.)")] + [self.ent(f"FACE_BOUND('',{self.loop(l)},.T.)") for l in loops[1:]]
        return self.ent(f"ADVANCED_FACE('',({','.join(bounds)}),{surface},{sense})")

    def plane(self, o, normal, ref, loops):
        return self.face(self.ent(f"PLANE('',{self.ax(o, normal, ref)})"), loops)

    def box_below(self, half, depth, top_loops):
        """The faces of a box [-half, half]^2 x [-depth, 0], its top face z = 0 bounded by
        the square and top_loops (holes, clockwise about +z)."""
        h, d = half, depth
        return [self.plane((0, 0, 0), (0, 0, 1), (1, 0, 0), [self.polygon([(-h, -h, 0), (h, -h, 0), (h, h, 0), (-h, h, 0)])] + top_loops),
                self.plane((0, 0, -d), (0, 0, -1), (1, 0, 0), [self.polygon([(-h, -h, -d), (-h, h, -d), (h, h, -d), (h, -h, -d)])]),
                self.plane((h, 0, 0), (1, 0, 0), (0, 1, 0), [self.polygon([(h, -h, -d), (h, h, -d), (h, h, 0), (h, -h, 0)])]),
                self.plane((-h, 0, 0), (-1, 0, 0), (0, 1, 0), [self.polygon([(-h, -h, -d), (-h, -h, 0), (-h, h, 0), (-h, h, -d)])]),
                self.plane((0, h, 0), (0, 1, 0), (1, 0, 0), [self.polygon([(-h, h, -d), (-h, h, 0), (h, h, 0), (h, h, -d)])]),
                self.plane((0, -h, 0), (0, -1, 0), (1, 0, 0), [self.polygon([(-h, -h, -d), (h, -h, -d), (h, -h, 0), (-h, -h, 0)])])]

    def text(self, faces):
        cone = (FIXTURES/'apex-cone.step').read_text()
        text = '\n'.join(line for line in cone.split('\n') if not re.match(r'#(1[5-9]|[2-5][0-9]) = ', line))
        shell = self.ent(f"CLOSED_SHELL('',({','.join(faces)}))")
        return appended(text, f"#15 = MANIFOLD_SOLID_BREP('',{shell});\n" + '\n'.join(self.records) + '\n')


def fillet(sense='.T.'):
    """attacks/fillet.py: a 20 x 20 x 10 box with a quarter-torus bump (R 4, r 1; the upper
    half of the torus over u in [0, pi]) on its top face, closed by two half-disks in
    y = 0. Returns the text and the torus face's id."""
    b, R, r = SolidBuilder(), 4., 1.
    o, i = R + r, R - r
    torus = b.ent(f"TOROIDAL_SURFACE('',{b.ax((0, 0, 0), (0, 0, 1), (1, 0, 0))},{R!r},{r!r})")
    a1 = b.arc((o, 0, 0), (-o, 0, 0), b.circle((0, 0, 0), (0, 0, 1), (1, 0, 0), o))
    b1 = b.arc((i, 0, 0), (-i, 0, 0), b.circle((0, 0, 0), (0, 0, 1), (1, 0, 0), i))
    m0 = b.arc((o, 0, 0), (i, 0, 0), b.circle((R, 0, 0), (0, -1, 0), (1, 0, 0), r))
    mp = b.arc((-o, 0, 0), (-i, 0, 0), b.circle((-R, 0, 0), (0, 1, 0), (-1, 0, 0), r))
    s1, _ = b.line((i, 0, 0), (o, 0, 0))
    s2, _ = b.line((-o, 0, 0), (-i, 0, 0))
    faces = b.box_below(10., 10., [[(b1, 1), (s2, 0), (a1, 0), (s1, 0)]])
    faces.append(b.plane((0, 0, 0), (0, -1, 0), (1, 0, 0), [[(s1, 1), (m0, 1)]]))
    faces.append(b.plane((0, 0, 0), (0, -1, 0), (1, 0, 0), [[(s2, 1), (mp, 0)]]))
    faces.append(b.face(torus, [[(a1, 1), (mp, 1), (b1, 0), (m0, 0)]], sense))
    return b.text(faces), int(faces[-1][1:])


def ring_on_block(sense='.T.'):
    """A torus ring (axis z, centre z = 0.6, R 4, r 1) standing on a 20 x 20 x 10 block and
    sunk 0.4 into it: the block's top face z = 0 has the hole r = 4.8 and keeps the disk
    r <= 3.2 inside the ring; the ring is the torus above z = 0, one face bounded by the
    circles r = 4.8 and r = 3.2 and the seam over the tube's top at u = 0. Its loop runs
    the band v in [-0.64, 3.79] in one chart. A genuine complement-type face: the other
    reading (the groove below z = 0, the other side) also bounds a valid solid. Returns
    the text and the ring face's id."""
    b, R, r, zc = SolidBuilder(), 4., 1., .6
    po, pi_ = (4.8, 0., 0.), (3.2, 0., 0.)
    torus = b.ent(f"TOROIDAL_SURFACE('',{b.ax((0, 0, zc), (0, 0, 1), (1, 0, 0))},{R!r},{r!r})")
    outer = b.arc(po, po, b.circle((0, 0, 0), (0, 0, 1), (1, 0, 0), 4.8))
    inner = b.arc(pi_, pi_, b.circle((0, 0, 0), (0, 0, 1), (1, 0, 0), 3.2))
    seam = b.arc(po, pi_, b.circle((R, 0, zc), (0, -1, 0), (.8, 0, -.6), r))
    faces = b.box_below(10., 10., [[(outer, 0)]])
    faces.append(b.plane((0, 0, 0), (0, 0, 1), (1, 0, 0), [[(inner, 1)]]))
    faces.append(b.face(torus, [[(outer, 1), (seam, 1), (inner, 0), (seam, 0)]], sense))
    return b.text(faces), int(faces[-1][1:])


def cut_ball(sense='.T.'):
    """A ball of radius 5 about the origin cut by the plane y = 3: the sphere face is the
    large part, bounded by the circle of radius 4 in y = 3, closed by the disk there. The
    sphere's axis is y, so the cut is a latitude, winding both poles: by same_sense the
    face reads the large part (with the pole at y = -5) or the cap (with y = 5). (About
    another axis the cut is no iso-line and the file stops in source UV, as the x-axis
    truncated sphere does.) Returns the text and the sphere face's id."""
    b = SolidBuilder()
    sphere = b.ent(f"SPHERICAL_SURFACE('',{b.ax((0, 0, 0), (0, 1, 0), (1, 0, 0))},5.)")
    cut = b.arc((4., 3., 0.), (4., 3., 0.), b.circle((0, 3, 0), (0, 1, 0), (1, 0, 0), 4.))
    faces = [b.plane((0, 3, 0), (0, 1, 0), (1, 0, 0), [[(cut, 1)]]), b.face(sphere, [[(cut, 0)]], sense)]
    return b.text(faces), int(faces[-1][1:])


class OrientationRound5Tests(unittest.TestCase, _Adapter):
    """Review round 5: a face on a closed surface whose raw reading is the complement of
    its loops (or either region of a band) and whose other reading also verifies is held
    to bounding a solid: a reading that meets another face of its part away from its
    boundary is not written."""

    read = OrientationTests.read
    records = OrientationTests.records
    statuses = OrientationRound2Tests.statuses

    def outcome(self, text, face):
        source, run = self.read(text)
        written = {f.id: f.orientation for f in source.faces}[face]
        hint = source.carrier(next(f.carrier_id for f in source.faces if f.id == face)).uv_bounds
        tests = int(next(line.split('\t')[1] for line in run.stdout.splitlines() if line.startswith('source_region_intersection_tests\t')))
        return source, written, hint, tests

    def test_the_reviewers_fillet_with_its_sense_reversed_is_corrected(self):
        """attacks/fillet.py with the torus face's same_sense reversed: by same_sense it
        reads the whole torus less its patch, which passes through the box's top face, so
        it bounds no solid, though its volume (the box less the torus) is positive. The
        patch with the other side meets nothing and verifies: written (S1) with a record.
        Before: written as read, with no record."""
        text, face = fillet('.F.')
        source, written, _, tests = self.outcome(text, face)
        self.assertEqual((self.statuses(source), sorted(self.records(source))), ([], [face]))
        self.assertEqual(written, 1)
        self.assertEqual(tests, 1)
        self.assertIn('meets another face of its part', source.diagnostics[[d.stage for d in source.diagnostics].index('source_face_orientation_raw')].message)

    def test_the_correct_fillet_is_unchanged(self):
        """The fillet as authored: its raw reading is the patch inside its loops, so the
        test does not run; admitted as read."""
        text, face = fillet()
        source, written, _, tests = self.outcome(text, face)
        self.assertEqual((self.statuses(source), self.records(source), written, tests), ([], {}, 1, 0))

    def test_a_ring_on_a_block_is_read_as_written(self):
        """The ring's raw reading (the torus above the block, the band through the tube's
        top: its hint holds v = pi/2) meets no other face away from its circles: it stands.
        Its sense reversed, the raw reading is the groove below z = 0 with the other side,
        which also bounds a valid solid and meets nothing: by G1 it stands too (the band
        is tested either way; nothing in the file tells the two apart)."""
        for sense, side in (('.T.', 1), ('.F.', -1)):
            with self.subTest(sense):
                text, face = ring_on_block(sense)
                source, written, hint, tests = self.outcome(text, face)
                self.assertEqual((self.statuses(source), self.records(source), written), ([], {}, side))
                self.assertEqual(tests, 1)
                if side == 1:
                    self.assertTrue(hint[2] <= math.pi / 2 <= hint[3])

    def test_a_cut_ball_is_read_as_written(self):
        """The cut ball's sphere face reads the large part; the other reading (the cap,
        the other side) does not verify, so no test runs: admitted as read. Its sense reversed: the raw uses enclose the cap
        inside out, and on a two-region surface that is refused by name (round 3)."""
        text, face = cut_ball()
        source, written, hint, tests = self.outcome(text, face)
        self.assertEqual((self.statuses(source), self.records(source), tests), ([], {}, 0))
        text, face = cut_ball('.F.')
        source, _, _, _ = self.outcome(text, face)
        self.assertEqual(self.statuses(source), sorted(f.id for f in source.faces))
        shell, = [d for d in source.diagnostics if d.stage == 'source_shell_orientation_refused']
        self.assertIn('two poles', shell.message)


def boss(sense='.F.', top=10., flange=None, h=20., br=5., fr=2., apex=None):
    """attacks/boss.py: a 40 x 40 x 5 plate with a cylindrical boss (radius 5) and a concave
    torus fillet (R 7, r 2) at its foot, the fillet one band loop with a seam; with
    flange=(radius, z) the boss ends at z = top under a flange plate whose underside is
    z = top. The fillet's tube reaches z = 4. Returns the text and the fillet's id."""
    b = SolidBuilder()
    R = br + fr
    c7 = b.arc((R, 0, 0), (R, 0, 0), b.circle((0, 0, 0), (0, 0, 1), (1, 0, 0), R))
    c5 = b.arc((br, 0, fr), (br, 0, fr), b.circle((0, 0, fr), (0, 0, 1), (1, 0, 0), br))
    c10 = b.arc((br, 0, top), (br, 0, top), b.circle((0, 0, top), (0, 0, 1), (1, 0, 0), br))
    meridian = b.arc((br, 0, fr), (R, 0, 0), b.circle((R, 0, fr), (0, -1, 0), (1, 0, 0), fr))
    side, _ = b.line((br, 0, fr), (br, 0, top))
    faces = b.box_below(h, 5., [[(c7, 0)]])
    torus = b.ent(f"TOROIDAL_SURFACE('',{b.ax((0, 0, fr), (0, 0, 1), (1, 0, 0))},{R!r},{fr!r})")
    faces.append(b.face(torus, [[(meridian, 1), (c7, 1), (meridian, 0), (c5, 0)]], sense))
    fillet_face = faces[-1]
    if apex is not None:
        # attacks/boss_apex.py: the boss a cone from its foot circle to an apex
        # vertex loop at z = apex.
        cone = b.ent(f"CONICAL_SURFACE('',{b.ax((0, 0, fr), (0, 0, -1), (1, 0, 0))},{br!r},{math.atan(br / (apex - fr))!r})")
        loop = b.ent(f"VERTEX_LOOP('',{b.vertex((0, 0, apex))})")
        faces.append(b.ent(f"ADVANCED_FACE('',({b.ent(f'FACE_BOUND({chr(39) * 2},{b.loop([(c5, 1)])},.T.)')},{b.ent(f'FACE_BOUND({chr(39) * 2},{loop},.T.)')}),{cone},.T.)"))
        return b.text(faces), int(fillet_face[1:])
    faces.append(b.face(b.ent(f"CYLINDRICAL_SURFACE('',{b.ax((0, 0, 0), (0, 0, 1), (1, 0, 0))},{br!r})"), [[(c5, 1), (side, 1), (c10, 0), (side, 0)]]))
    if flange is None:
        faces.append(b.plane((0, 0, top), (0, 0, 1), (1, 0, 0), [[(c10, 1)]]))
    else:
        radius, z = flange
        f1 = b.arc((radius, 0, top), (radius, 0, top), b.circle((0, 0, top), (0, 0, 1), (1, 0, 0), radius))
        f2 = b.arc((radius, 0, z), (radius, 0, z), b.circle((0, 0, z), (0, 0, 1), (1, 0, 0), radius))
        rim, _ = b.line((radius, 0, top), (radius, 0, z))
        faces.append(b.plane((0, 0, top), (0, 0, -1), (1, 0, 0), [[(f1, 0)], [(c10, 1)]]))
        faces.append(b.face(b.ent(f"CYLINDRICAL_SURFACE('',{b.ax((0, 0, 0), (0, 0, 1), (1, 0, 0))},{radius!r})"), [[(f1, 1), (rim, 1), (f2, 0), (rim, 0)]]))
        faces.append(b.plane((0, 0, z), (0, 0, 1), (1, 0, 0), [[(f2, 1)]]))
    return b.text(faces), int(fillet_face[1:])


class OrientationRound6Tests(unittest.TestCase, _Adapter):
    """Review round 6: contact within the source resolution meets (ruling 3), and every
    region choice has a typed record (ruling 2)."""

    read = OrientationTests.read
    read_all = OrientationTests.read_all
    records = OrientationTests.records
    statuses = OrientationRound2Tests.statuses

    def choice(self, source, face):
        found = [d for d in source.diagnostics if d.stage == 'source_face_region_choice' and d.source_ids[1] == face]
        return {v.name: v.value for v in found[0].values} if found else None

    def test_reversed_fillets_under_a_flange_decide_by_fold(self):
        """attacks/boss_gap.py, round 8: the boss under a flange, the fillet's same_sense
        reversed, tangent, overlapping or apart by 1e-9 to 1e-4 mm. The reversed reading
        (the rest of the torus tube) folds back over the plate and the boss at their
        tangent common edges, so it is no solid's boundary whatever the flange's gap, and
        the fillet is corrected (S1) by the fold test alone, with no contact test. Contact
        within the resolution is held at function level
        (test_a_neighbour_is_held_to_contact_outside_its_strip)."""
        cases = (('tangent', 4.), ('gap 1e-9', 4. + 1e-9), ('gap 5e-8', 4. + 5e-8), ('gap 1e-6', 4. + 1e-6), ('gap 1e-4', 4. + 1e-4),
                 ('overlap 1e-9', 4. - 1e-9), ('overlap 1e-4', 4. - 1e-4))
        built = [boss('.T.', top=top, flange=(10., 6.)) for _, top in cases]
        read = self.read_all([text for text, _ in built])
        for (name, _), (_, face), (source, run) in zip(cases, built, read):
            with self.subTest(name):
                self.assertEqual((self.statuses(source), sorted(self.records(source))), ([], [face]))
                choice = self.choice(source, face)
                self.assertEqual((choice['decided_by'], choice['test_ran'], choice['written_other_reading']), (1., 0., 1.))
                self.assertIn('source_region_intersection_tests\t0\n', run.stdout)

    def test_boss_fillets_decide_by_tangent_fold(self):
        """Round 8, ruling 1 (attacks/blind.py, boss_apex.py): boss fillets of R 7 / r 2,
        R 50 / r 5 and R 302 / r 2, and the boss ending in an apex cone. Authored, the
        fillet is fold-free at its tangent edges (the plate, the boss) and stands; reversed,
        it folds back over them and is corrected (S1). Either way the fold test decides,
        with no contact test. Before: 5 to 24 s of contact tests each."""
        cases = [(sense, dict(h=h, br=br, fr=fr, top=top)) for h, br, fr, top in ((20., 5., 2., 10.), (80., 45., 5., 60.), (400., 300., 2., 100.)) for sense in ('.F.', '.T.')]
        cases += [(sense, dict(apex=10.)) for sense in ('.F.', '.T.')]
        built = [boss(sense, **kw) for sense, kw in cases]
        read = self.read_all([text for text, _ in built])
        for (sense, kw), (_, face), (source, run) in zip(cases, built, read):
            with self.subTest(sense=sense, **kw):
                self.assertEqual((self.statuses(source), sorted(self.records(source))), ([], [face] if sense == '.T.' else []))
                choice = self.choice(source, face)
                self.assertEqual((choice['decided_by'], choice['test_ran']), (1., 0.))
                self.assertGreaterEqual(choice['tangent_edges'], 1.)
                self.assertEqual((choice['raw_smooth_edges'] > 0, choice['raw_fold_edges'] > 0), (sense == '.F.', sense == '.T.'))
                self.assertIn('source_region_intersection_tests\t0\n', run.stdout)

    def test_every_region_choice_has_a_record(self):
        """Every region choice has a source_face_region_choice record, with how it was
        decided: the boss fillet as authored (a side of a band, fold-free at its tangent
        edges: decided by fold, standing) and reversed (folding: decided by fold,
        corrected); the ring's band (no tangent edge: the contact test, meeting nothing);
        the reviewer's torus-bump fillet reversed (the complement of its loops; it meets
        its top face at a right angle, so no tangent edge: the contact test, corrected);
        the cut ball (one of two poles' sides, the other reading not verifying: neither
        test). The torus-bump fillet as authored reads the region inside its loops and has
        none. Before round 6: none of them had a record."""
        cases = (('boss fillet', boss('.F.')[0], (2., 0., 0., 1.)), ('boss fillet reversed', boss('.T.')[0], (2., 0., 1., 1.)),
                 ('ring', ring_on_block()[0], (2., 1., 0., 2.)), ('fillet reversed', fillet('.F.')[0], (1., 1., 1., 2.)),
                 ('cut ball', cut_ball()[0], (3., 0., 0., 0.)), ('fillet', fillet()[0], None))
        read = self.read_all([case[1] for case in cases])
        for (name, text, expected), (source, _) in zip(cases, read):
            if expected is None:
                self.assertEqual([d for d in source.diagnostics if d.stage == 'source_face_region_choice'], [])
                continue
            with self.subTest(name):
                choices = [{v.name: v.value for v in d.values} for d in source.diagnostics if d.stage == 'source_face_region_choice']
                self.assertEqual([(c['region_kind'], c['test_ran'], c['written_other_reading'], c['decided_by']) for c in choices], [expected])
                ran = expected[1]
                # Review round 7: a tested record states the band left blind beside its
                # neighbours' common boundaries (its strip and the last-level cell).
                if ran:
                    self.assertGreater(choices[0]['blind_strip_half_width_mm'], 0.)
                    self.assertGreaterEqual(choices[0]['blind_cell_mm'], 0.)
                    self.assertLess(choices[0]['blind_strip_half_width_mm'] + choices[0]['blind_cell_mm'], 2.)


class UncertaintyFloorTests(unittest.TestCase, _Adapter):
    """B2 .37.39: the G6 UR-7 floor compares a declared uncertainty with the diagonal of a
    box that holds the translated solid (native/shape_extent.hpp), not BRepBndLib's,
    which boxes a face over its pcurves' UV box: the whole unbounded carrier where they
    are missing, and short of the region where they do not reach (a cone's apex)."""

    def record(self, text):
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            record, = [d for d in load_source(target).diagnostics if d.stage == 'source_uncertainty']
        return {v.name: v.value for v in record.values}

    def test_the_floor_is_taken_over_the_whole_cone(self):
        """apex-cone.step (base radius 5, apex 10 above it) declaring 1.5e-14 mm. Its
        true box is 10 x 10 x 10 (diagonal 17.32), so 1.5e-14 lies below 1e-15 of it and
        makes no usable claim. Before: BRepBndLib's box stopped at the base circle
        (diagonal 14.14) and the value was usable. The diagonal now holds the solid."""
        text = substituted((FIXTURES/'apex-cone.step').read_text(), "LENGTH_MEASURE(1.E-07)", "LENGTH_MEASURE(1.5E-14)")
        values = self.record(text)
        self.assertGreaterEqual(values['diagonal_mm'], math.sqrt(300.) - 1e-9)
        self.assertEqual(values['usable'], 0.)
        # The default 1e-7 stays usable over the larger diagonal.
        self.assertEqual(self.record((FIXTURES/'apex-cone.step').read_text())['usable'], 1.)


class ReviewFindingTests(unittest.TestCase, _Adapter):
    """The independent review's findings 5-9 and 11 (B2-reader-adapter)."""

    def test_a_file_pcurve_made_periodic_is_written_as_its_raw_net(self):
        """Finding 6: StepToGeom periodizes a closed 2D B-spline pcurve as it does a 3D
        curve. On both faces of the lens the file's own net is written, with a
        source_pcurve_raw_net record each; the lens reads with its owned volume."""
        from brepgpu.imported_source import load_source
        text, ring, w = lens()
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
        records = [d for d in source.diagnostics if d.stage == 'source_pcurve_raw_net']
        self.assertEqual(sorted(d.source_ids for d in records), [(20, 21, 17), (36, 21, 30)])
        weights = tuple(1. if i % 2 == 0 else w for i in range(9))
        for use_id in (20, 36):
            curve = source.curve(source.coedge(use_id).curve_id)
            self.assertEqual((curve.dimension, curve.degree, curve.knots, curve.multiplicities), (2, 2, (0., .25, .5, .75, 1.), (3, 2, 2, 2, 3)))
            self.assertEqual(curve.controls, tuple(c for (x, y), wi in zip(ring, weights) for c in ((x+6)/12*wi, (6-y)/12*wi, wi)))
        self.assertEqual([d for d in source.diagnostics if d.stage in ('source_face_refused', 'source_face_orientation_raw')], [])

    def test_a_surface_knot_vector_occt_edits_refuses(self):
        """Finding 8: a repeated u knot of the B-spline carrier #92 (OCCT merges it,
        StepToGeom.cxx:1015-1083) refuses the carrier by name instead of writing OCCT's
        merged net."""
        text = substituted(wiggling_boundary_block(), "(2,2),(2,2),(0.,1.),(0.,1.),.UNSPECIFIED.);",
                           "(1,1,2),(2,2),(0.,0.,1.),(0.,1.),.UNSPECIFIED.);")
        with tempfile.TemporaryDirectory() as directory:
            run, _, _ = self.extract(text, directory)
        self.assertEqual(run.returncode, 1, run.stderr)
        self.assertIn('B-spline carrier 92: its raw knot vector is one OCCT edits', run.stderr)

    def test_an_endpoint_off_its_vertex_is_not_charged_to_the_domain_correction(self):
        """Finding 5: the wiggling edge's first pole 0.02 mm off its vertex along the face.
        The domain correction checks only what it changes, the corrected curve's sampled
        residual, so the vertex gap never becomes its refusal. HEAD admits this file;
        round 2 refused it with 'original endpoint correspondence exceeds source grade'.
        Where a curve's own residual fails, the correction leaves it to the C/S residual
        check, which names that cause as HEAD did (the corpus table in the report)."""
        from brepgpu.imported_source import load_source
        text = substituted(wiggling_boundary_block(), "#910 = CARTESIAN_POINT('',(10.,0.0,0.0));",
                           "#910 = CARTESIAN_POINT('',(10.,0.0,0.02));")
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
        self.assertEqual([d.source_ids for d in source.diagnostics if d.stage == 'source_chart_domain_correction'], [(15, 57, 60)])

    def test_a_frame_along_x_without_ref_direction_takes_part_42s_default(self):
        """Finding 7: first_proj_axis without arg is (0,1,0) when the axis is +-(1,0,0)."""
        from brepgpu.imported_source import load_source
        base = (FIXTURES/'source-uv.step').read_text()
        frame = "#19 = AXIS2_PLACEMENT_3D('',#20,#21,#22);"
        for axis, expected in (('1.', (0., 0., 1., 4., 1., 0., 0., -3., 0., 1., 0., 2.)),
                               ('-1.', (0., 0., -1., 4., 1., 0., 0., -3., 0., -1., 0., 2.))):
            with self.subTest(axis):
                text = appended(substituted(base, frame, "#19 = AXIS2_PLACEMENT_3D('',#20,#700,$);"), f"#700 = DIRECTION('',({axis},0.,0.));\n")
                with tempfile.TemporaryDirectory() as directory:
                    run, _, target = self.extract(text, directory)
                    self.assertEqual(run.returncode, 0, run.stderr)
                    usage = next(u for u in load_source(target).usages if u.id == 386)
                for a, b in zip(usage.transform, expected):
                    self.assertLessEqual(abs(a-b), 2.**-52, usage.transform)

    def test_a_translation_difference_above_the_uncertainty_is_written_raw(self):
        """Finding 9a: item_1 turned 1e-10 rad about z and item_2 5000 mm out. OCCT's
        A^-1 B and the raw B A^-1 differ by 5e-7 mm in each of the two 5000 mm translation
        entries: inside 2^-32 of 5000 (1.2e-6) but above the contexts' 1e-7 mm
        uncertainty. The raw value is written."""
        from brepgpu.imported_source import load_source
        base = (FIXTURES/'source-uv.step').read_text()
        text = substituted(base, "#40 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#41),#371);",
                           "#40 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#41,#703),#371);")
        text = substituted(text, "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23),#27);",
                           "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23,#707),#27);")
        text = substituted(text, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#11,#19);",
                           "#384 = ITEM_DEFINED_TRANSFORMATION('','',#703,#707);\n#700 = CARTESIAN_POINT('',(0.,0.,0.));\n"
                           "#701 = DIRECTION('',(0.,0.,1.));\n#702 = DIRECTION('',(1.,1.E-10,0.));\n#703 = AXIS2_PLACEMENT_3D('',#700,#701,#702);\n"
                           "#704 = CARTESIAN_POINT('',(5000.,5000.,0.));\n#706 = DIRECTION('',(1.,0.,0.));\n#707 = AXIS2_PLACEMENT_3D('',#704,#701,#706);")
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
        record, = [d for d in source.diagnostics if d.stage == 'source_placement_raw' and d.source_ids[0] == 386]
        values = {v.name: v.value for v in record.values}
        self.assertLessEqual(values['max_relative_entry_difference'], 2.**-32)
        self.assertGreater(values['max_translation_difference_mm'], values['translation_resolution_mm'])
        self.assertEqual(values['translation_resolution_mm'], 1e-7)
        usage = next(u for u in source.usages if u.id == 386)
        self.assertEqual((usage.transform[3], usage.transform[7]), (5000., 5000.))

    def test_a_rotation_difference_over_a_far_child_is_written_raw(self):
        """Review round 2, finding 3 (attack rot9a.py): item_1 turned 1.5e-10 rad about z,
        item_2 a frame along y, both at the origin, and the box's geometry moved 5000 mm
        along x within its own representation. The rotation entries differ by 1.5e-10
        (inside 2^-32) and the translations not at all, but over the box the two readings
        move points by 7.5e-7 mm, above the 1e-7 mm uncertainty: the raw value is
        written, with a record."""
        from brepgpu.imported_source import load_source
        base = moved((FIXTURES/'source-uv.step').read_text(), 5000., first=41, last=370)
        text = substituted(base, "#40 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#41),#371);",
                           "#40 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#41,#703),#371);")
        text = substituted(text, "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23),#27);",
                           "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23,#707),#27);")
        text = substituted(text, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#11,#19);",
                           "#384 = ITEM_DEFINED_TRANSFORMATION('','',#703,#707);\n#700 = CARTESIAN_POINT('',(0.,0.,0.));\n"
                           "#701 = DIRECTION('',(0.,0.,1.));\n#702 = DIRECTION('',(1.,1.5E-10,0.));\n#703 = AXIS2_PLACEMENT_3D('',#700,#701,#702);\n"
                           "#705 = DIRECTION('',(0.,1.,0.));\n#706 = DIRECTION('',(1.,0.,0.));\n#707 = AXIS2_PLACEMENT_3D('',#700,#705,#706);")
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
        record, = [d for d in source.diagnostics if d.stage == 'source_placement_raw' and d.source_ids[0] == 386]
        values = {v.name: v.value for v in record.values}
        self.assertLessEqual(values['max_relative_entry_difference'], 2.**-32)
        self.assertLessEqual(values['max_translation_difference_mm'], values['translation_resolution_mm'])
        self.assertGreater(values['max_point_difference_mm'], values['translation_resolution_mm'])

    def test_the_same_rotation_difference_over_a_near_curved_child_is_written_as_occts(self):
        """Round 2, finding 3, guard on the extent: the same turned item_1 and y-axis item_2
        placing the cylinder part, whose edges carry no pcurves in OCCT's translation (its
        edge curves are the circles, not the surface curves). OCCT's box of that product
        spans the whole unbounded cylinder; the adapter's extent is each face's carrier over
        its UV hint, a few mm, over which the readings move points by under 1e-9 mm: OCCT's
        value is written and no record is made. A reader that boxed OCCT's shape wrote the
        placement raw on OM10 for rounding-level differences."""
        from brepgpu.imported_source import load_source
        text = (FIXTURES/'source-uv.step').read_text()
        text = substituted(text, "#394 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#395),#493);",
                           "#394 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#395,#703),#493);")
        text = substituted(text, "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23),#27);",
                           "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23,#707),#27);")
        text = substituted(text, "#500 = ITEM_DEFINED_TRANSFORMATION('','',#11,#23);",
                           "#500 = ITEM_DEFINED_TRANSFORMATION('','',#703,#707);\n#700 = CARTESIAN_POINT('',(0.,0.,0.));\n"
                           "#701 = DIRECTION('',(0.,0.,1.));\n#702 = DIRECTION('',(1.,1.5E-10,0.));\n#703 = AXIS2_PLACEMENT_3D('',#700,#701,#702);\n"
                           "#705 = DIRECTION('',(0.,1.,0.));\n#706 = DIRECTION('',(1.,0.,0.));\n#707 = AXIS2_PLACEMENT_3D('',#700,#705,#706);")
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
        self.assertEqual([d.source_ids[0] for d in source.diagnostics if d.stage == 'source_placement_raw'], [])
        self.assertIn('source_placements_raw\t0\n', run.stdout)
        usage = next(u for u in source.usages if u.id == 502)
        # The written value is OCCT's A^-1 B with A turned: its rotation keeps the 1.5e-10 term.
        self.assertTrue(any(abs(abs(x) - 1.5e-10) < 1e-15 for x in usage.transform[:3] + usage.transform[4:7] + usage.transform[8:11]))

    def test_a_child_on_a_surface_of_revolution_is_refused_before_its_extent_matters(self):
        """Round 3, finding 4, as far as it is reachable. The cylinder part with its cylinder
        written as the equal surface_of_revolution of its generator, placed with the 1.5e-10
        rotation difference. The extent now treats any carrier OCCT boxes from a sample
        grid (offset, revolution, extrusion) as unbounded, so a rotation difference over it
        writes the raw placement; but the source format carries planes, cylinders, cones,
        spheres, tori and B-splines only, and such a carrier refuses the file by name first,
        as at HEAD. Every admitted carrier's box is a bound (exact, or the B-spline poles)."""
        text = (FIXTURES/'source-uv.step').read_text()
        text = substituted(text, "#411 = CYLINDRICAL_SURFACE('',#412,0.75);", "#411 = SURFACE_OF_REVOLUTION('',#720,#723);\n"
                           "#720 = LINE('',#721,#722);\n#721 = CARTESIAN_POINT('',(-0.75,2.25,0.125));\n#722 = VECTOR('',#414,1.);\n#723 = AXIS1_PLACEMENT('',#413,#414);")
        with tempfile.TemporaryDirectory() as directory:
            run, _, _ = self.extract(text, directory)
        self.assertEqual(run.returncode, 1)
        self.assertIn('unsupported surface at source 411', run.stderr)

    def test_a_cycle_through_a_draughting_relationship_refuses_before_transfer(self):
        """Finding 11: the root #10 holds mapped item #952 of #950, which a
        mechanical_design_and_draughting_relationship #955 joins back to #10. OCCT's
        TransferRelatedSRR transfers both sides of it (:2708, :1507-1563)."""
        text = with_root_items('#952', records=(
            "#950 = SHAPE_REPRESENTATION('',(#11),#27);\n#951 = REPRESENTATION_MAP(#11,#950);\n"
            "#952 = MAPPED_ITEM('',#951,#11);\n#955 = MECHANICAL_DESIGN_AND_DRAUGHTING_RELATIONSHIP('','',#950,#10);\n"))
        with tempfile.TemporaryDirectory() as directory:
            run, _, _ = self.extract(text, directory)
        self.assertEqual(run.returncode, 1, (run.returncode, run.stderr[-300:]))
        self.assertIn('OCCT 8.0.1 would recurse without bound', run.stderr)
        self.assertIn('mechanical_design_and_draughting_relationship 955', run.stderr)


class MappedItemTests(unittest.TestCase, _Adapter):
    """Beads .37.27 and .37.26."""

    def refused(self, text, directory, name):
        run, _, _ = self.extract(text, directory, name)
        return run

    def test_mapped_item_cycles_refuse_by_name_before_transfer(self):
        """.37.27: OCCT 8.0.1 transfers a mapped item's representation, then every plain
        shape_representation_relationship sharing it, and binds nothing until it returns
        (STEPControl_ActorRead.cxx:2070, :2679-2710), so a cycle overflows its stack.

        - relation: the root #10 holds mapped item #795 of the cylinder's #394, which the
          plain #862 joins back to #10 (the styled_mapped fixture with usage #860).
        - mapped: the root holds #952 mapping #950, which holds #954 mapping the root.
        """
        mapped = with_root_items('#952', records=(
            "#950 = SHAPE_REPRESENTATION('',(#11,#954),#27);\n#951 = REPRESENTATION_MAP(#11,#950);\n"
            "#952 = MAPPED_ITEM('',#951,#11);\n#953 = REPRESENTATION_MAP(#11,#10);\n"
            "#954 = MAPPED_ITEM('',#953,#11);\n"))
        cases = {'relation': (styled_mapped(), ('mapped item 795 of representation 394',
                                                'plain shape_representation_relationship 862')),
                 'mapped': (mapped, ('mapped item 952 of representation 950', 'mapped item 954 of representation 10'))}
        with tempfile.TemporaryDirectory() as directory:
            for name, (text, named) in cases.items():
                with self.subTest(name):
                    run = self.refused(text, directory, name)
                    self.assertEqual(run.returncode, 1, (run.returncode, run.stderr[-400:]))
                    self.assertIn('OCCT 8.0.1 would recurse without bound', run.stderr)
                    self.assertIn('refused before transfer', run.stderr)
                    for part in named:
                        self.assertIn(part, run.stderr)

    def test_solid_bearing_mapped_items_refuse_by_name(self):
        """.37.26: a mapped item in a transferred representation whose mapped representation
        brings a solid is an occurrence the usage table cannot publish.

        - direct: the root holds #795 mapping the cylinder's ABSR #394 (styled_mapped without
          its plain usage); before, the three usages were written and #795 left no trace.
        - related: the root holds #962 mapping #960, a representation holding only a frame,
          which the plain #963 joins to the box's #40 (OCCT's TransferRelatedSRR brings the
          box with it).
        - own frame: the B2-harness fixture places its part's own solid through mapped item
          #901 at (0, 5, 0) in the part's representation #900.
        """
        related = with_root_items('#962', records=(
            "#960 = SHAPE_REPRESENTATION('',(#11),#27);\n#961 = REPRESENTATION_MAP(#11,#960);\n"
            "#962 = MAPPED_ITEM('',#961,#11);\n#963 = SHAPE_REPRESENTATION_RELATIONSHIP('','',#960,#40);\n"))
        cases = {'direct': (styled_mapped(plain=False), 'mapped item 795 in representation 10 places representation 394'),
                 'related': (related, 'mapped item 962 in representation 10 places representation 960'),
                 'own frame': ((FIXTURES/'mapped-item-solid.step').read_text(),
                               'mapped item 901 in representation 900 places representation 10')}
        with tempfile.TemporaryDirectory() as directory:
            for name, (text, message) in cases.items():
                with self.subTest(name):
                    run = self.refused(text, directory, name.replace(' ', '-'))
                    self.assertEqual(run.returncode, 1, run.stderr[-400:])
                    self.assertIn(message, run.stderr)
                    self.assertIn('which reaches a solid', run.stderr)

    def test_mapped_item_without_a_solid_is_not_refused(self):
        """Guard: a mapped item of a representation that holds only a frame places no solid."""
        text = with_root_items('#972', records=(
            "#970 = SHAPE_REPRESENTATION('',(#11),#27);\n#971 = REPRESENTATION_MAP(#11,#970);\n"
            "#972 = MAPPED_ITEM('',#971,#11);\n"))
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual({u.id for u in load_source(target).usages}, {380, 386, 502})


def bspline_closed_in_v():
    """VertexLoopTests' closed B-spline face: u degree 2 (3 rows), v degree 1 over a
    5-pole ring closed in v, knots (0..4) multiplicities (2,1,1,1,2); pole at (0,0,5)."""
    top, bottom = (0., 0., 5.), (0., 0., -5.)
    ring = ((5., 0., 0.), (0., 5., 0.), (-5., 0., 0.), (0., -5., 0.), (5., 0., 0.))
    rows = ((top,)*5, ring, (bottom,)*5)
    authored = ingress.VertexLoopTests('test_sole_vertex_loop_on_a_closed_bspline_is_admitted')
    return authored.bspline(rows, (0., 1., 2., 3., 4.), (2, 1, 1, 1, 2), '.T.'), rows


def wiggling_boundary_block(amplitude=3e-4):
    """void-block.step without its void, its x = 10 face #57 carried by a flat bilinear
    B-spline #92 on exactly that square (u along y, v along z, knots 0..1), and that
    face's edge #61 at y = 0 a cubic B-spline #66 that wiggles +-amplitude in y about
    the boundary iso-line u = 0 (poles at z = 0, 10/6, ..., 10). Projections past the
    side land on it; the projected fit overshoots the side between them."""
    text = substituted((FIXTURES/'void-block.step').read_text(), "#15 = BREP_WITH_VOIDS('',#16,(#165));",
                       "#15 = MANIFOLD_SOLID_BREP('',#16);")
    text = substituted(text, "#92 = PLANE('',#93);",
                       "#92 = B_SPLINE_SURFACE_WITH_KNOTS('',1,1,((#900,#901),(#902,#903)),.UNSPECIFIED.,.F.,.F.,.F.,"
                       "(2,2),(2,2),(0.,1.),(0.,1.),.UNSPECIFIED.);")
    ys = [0.] + [(-amplitude if i % 2 == 0 else amplitude) for i in range(5)] + [0.]
    zs = [10*i/6 for i in range(7)]
    text = substituted(text, "#66 = LINE('',#67,#68);",
                       f"#66 = B_SPLINE_CURVE_WITH_KNOTS('',3,({','.join(f'#{910+i}' for i in range(7))}),"
                       ".UNSPECIFIED.,.F.,.F.,(4,1,1,1,4),(0.,0.25,0.5,0.75,1.),.UNSPECIFIED.);")
    return appended(text, "#900 = CARTESIAN_POINT('',(10.,0.,0.));\n#901 = CARTESIAN_POINT('',(10.,0.,10.));\n"
                          "#902 = CARTESIAN_POINT('',(10.,10.,0.));\n#903 = CARTESIAN_POINT('',(10.,10.,10.));\n" +
                    ''.join(f"#{910+i} = CARTESIAN_POINT('',(10.,{y!r},{z!r}));\n" for i, (y, z) in enumerate(zip(ys, zs))))


def pcurve_samples(source, face, count=64):
    """Every published UV curve of a face, sampled at count+1 points over its interval."""
    points = []
    for loop_id in face.loop_ids:
        for use_id in source.loop(loop_id).coedge_ids:
            use = source.coedge(use_id)
            a, b = use.parameter_range
            points += [(use_id, ingress_point(source.curve(use.curve_id), b if k == count else a + (b-a)*k/count))
                       for k in range(count+1)]
    return points


def ingress_point(curve, t):
    from brepgpu.imported_oracles import curve_point_float
    return curve_point_float(curve, t)


class CarrierTests(unittest.TestCase, _Adapter):
    """Beads .37.28 and .37.29."""

    def test_closed_bspline_carrier_is_the_files_own_net(self):
        """.37.28: OCCT forces the closed B-spline periodic (TranslateFace ConvertToPeriodic);
        the adapter wrote that net, which both readers refused ('knot/control relation').
        The carrier row is now the raw entity's net, read back by both readers and equal to
        the STEP text; OCCT's periodic form is recorded."""
        from brepgpu.imported_source import load_source
        text, rows = bspline_closed_in_v()
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            inspected, _ = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
            self.assertIn('source_carrier_raw_nets\t1\n', run.stdout)
        carrier = source.carrier(33)
        self.assertEqual((carrier.kind, carrier.degrees, carrier.control_shape), (6, (2, 1), (3, 5)))
        self.assertEqual((carrier.knot_u, carrier.mult_u), ((0., 1.), (3, 3)))
        self.assertEqual((carrier.knot_v, carrier.mult_v), ((0., 1., 2., 3., 4.), (2, 1, 1, 1, 2)))
        self.assertEqual(carrier.controls, tuple(c for row in rows for p in row for c in (*p, 1.)))
        record, = [d for d in source.diagnostics if d.stage == 'source_carrier_raw_net']
        values = {v.name: v.value for v in record.values}
        self.assertEqual((record.source_ids, values['u_periodic'], values['v_periodic']), ((33, 30), 0., 1.))
        self.assertGreaterEqual(values['max_sampled_distance_mm'], 0.)
        self.assertEqual({f.status for f in source.faces}, {0})

    def test_closed_bspline_edge_curve_is_the_files_own_net(self):
        """.37.28, edge curves (found in this unit): StepToGeom::MakeBSplineCurve makes a
        closed raw curve periodic (StepToGeom.cxx:921-926), and the adapter wrote that net,
        which both readers refused. half-cylinders.step with its top circle #31 replaced by
        a closed rational quadratic B-spline circle (9 poles, knots 0..1 by quarters,
        multiplicities 3,2,2,2,3): the curve row is now the raw net, which the oracle
        compares with the STEP text, and both readers read every row."""
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.imported_source import load_source
        w = math.sqrt(.5)
        poles = ((1., 0.), (1., 1.), (0., 1.), (-1., 1.), (-1., 0.), (-1., -1.), (0., -1.), (1., -1.), (1., 0.))
        ids = ','.join(f'#{300+i}' for i in range(9))
        curve = (f"#31 = ( BOUNDED_CURVE() B_SPLINE_CURVE(2,({ids}),.CIRCULAR_ARC.,.T.,.F.) "
                 "B_SPLINE_CURVE_WITH_KNOTS((3,2,2,2,3),(0.,0.25,0.5,0.75,1.),.UNSPECIFIED.) CURVE() "
                 f"GEOMETRIC_REPRESENTATION_ITEM() RATIONAL_B_SPLINE_CURVE(({','.join(repr(1. if i % 2 == 0 else w) for i in range(9))})) "
                 "REPRESENTATION_ITEM('') );")
        text = substituted((FIXTURES/'half-cylinders.step').read_text(), "#31 = CIRCLE('',#32,1.);", curve)
        text = appended(text, ''.join(f"#{300+i} = CARTESIAN_POINT('',({x!r},{y!r},2.));\n" for i, (x, y) in enumerate(poles)))
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            inspected, _ = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
            self.assertIn('source_curve_raw_nets\t1\n', run.stdout)
            # The top plane's two UV curves are OCCT's on-plane projections of the
            # periodic curve (G1 E1), written unperiodized (an exact representation).
            self.assertIn('source_uv_curves_unperiodized\t2\n', run.stdout)
            qualify_original_geometry(source, step)
        spline = source.curve(31)
        self.assertEqual((spline.kind, spline.degree, spline.knots, spline.multiplicities),
                         (5, 2, (0., .25, .5, .75, 1.), (3, 2, 2, 2, 3)))
        weights = tuple(1. if i % 2 == 0 else w for i in range(9))
        self.assertEqual(spline.controls, tuple(c for (x, y), wi in zip(poles, weights) for c in (x*wi, y*wi, 2.*wi, wi)))
        self.assertEqual([d.source_ids[0] for d in source.diagnostics if d.stage == 'source_curve_raw_net'], [31])

    def test_projected_uv_curves_stay_in_the_natural_domain(self):
        """.37.31: a B-spline carrier is defined on its knot rectangle only. The projected
        UV curve of the wiggling edge #61 on face #57 overshot u = 0 by about 2e-7, and
        the hint (BRepTools::UVBounds, clipped to the rectangle) left those samples out.
        Every published UV sample of the face must now lie in the knot rectangle and in
        the hint; the corrected coedge #60 has a record whose bound fits the chord
        allocation (0.005 mm / 8), and both readers read the file."""
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(wiggling_boundary_block(), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            inspected, _ = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
        carrier = source.carrier(92)
        self.assertEqual((carrier.kind, carrier.knot_u, carrier.knot_v), (6, (0., 1.), (0., 1.)))
        u0, u1, v0, v1 = carrier.uv_bounds
        for use_id, (u, v) in pcurve_samples(source, source.face(57)):
            self.assertTrue(-1e-15 <= u <= 1+1e-15 and -1e-15 <= v <= 1+1e-15, (use_id, u, v))
            self.assertTrue(u0-1e-9 <= u <= u1+1e-9 and v0-1e-9 <= v <= v1+1e-9, (use_id, u, v, carrier.uv_bounds))
        records = [d for d in source.diagnostics if d.stage == 'source_chart_domain_correction']
        self.assertEqual([d.source_ids for d in records], [(15, 57, 60)])
        values = {v.name: v.value for v in records[0].values}
        self.assertGreater(values['max_u_move'], 1e-8)
        self.assertLessEqual(values['uv_metric_mm'], .005/8)
        self.assertLessEqual(values['max_sampled_residual_mm'], .001)

    def test_uncorrectable_escaping_uv_curves_refuse_by_name(self):
        """.37.31 follow-up: a UV curve the domain correction cannot take (rational,
        periodic, trimmed, a conic) that leaves a B-spline carrier's knot rectangle by
        more than the recovery grade refuses by name, with its form and largest escape
        as typed fields, never passing as it is. OCCT's projector cannot be made to
        return these forms from a STEP file, so source_uv's own predicate is driven
        directly on authored curves over [0,1]^2 (tests/source_uv_fixture.cpp)."""
        cases = {'rational': ((5, 1, 0, 0), 'rational B-spline curve', 1e-3),
                 'periodic': ((5, 0, 1, 0), 'periodic B-spline curve', .01),
                 'trimmed': ((2, 0, 1, 1), 'periodic trimmed circle', .1),
                 'circle': ((2, 0, 1, 0), 'periodic circle', .1)}
        with tempfile.TemporaryDirectory() as directory:
            for name, (flags, form, escape) in cases.items():
                with self.subTest(name):
                    run = subprocess.run((str(ingress.uv_fixture()), f'natural-domain-{name}', str(Path(directory)/'x')),
                                         capture_output=True, text=True)
                    self.assertEqual(run.returncode, 3, run.stdout + run.stderr)
                    row = run.stdout.split('\t')
                    self.assertEqual((row[0], int(row[1]), int(row[2])), ('refused', 7, 8))
                    self.assertEqual(tuple(map(int, row[3:7])), flags)
                    self.assertTrue(escape*.5 <= float(row[7]) <= escape*1.5, row)
                    self.assertIn(f'face 7 coedge 8 natural domain: projected {form}', run.stderr)
            run = subprocess.run((str(ingress.uv_fixture()), 'natural-domain-rational-inside', str(Path(directory)/'x')),
                                 capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(run.stdout, 'escape\t0\n')

    def test_carrier_hint_covers_every_face_on_it(self):
        """.37.29: half-cylinders.step bounds its lateral surface by two faces, #17 (u in
        [0, pi]) and #18 (u in [pi, 2 pi]), on one CYLINDRICAL_SURFACE #50. The hint was
        written from the first face only. Every published pcurve sample of both faces must
        lie in the carrier's hint, and the hint must not exceed their union."""
        from brepgpu.imported_oracles import curve_point_float
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract((FIXTURES/'half-cylinders.step').read_text(), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            inspected, _ = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
        carrier = source.carrier(50)
        faces = [f for f in source.faces if f.carrier_id == 50]
        self.assertEqual(sorted(f.id for f in faces), [17, 18])
        points = []
        for face in faces:
            for loop_id in face.loop_ids:
                for use_id in source.loop(loop_id).coedge_ids:
                    use = source.coedge(use_id)
                    a, b = use.parameter_range
                    points += [curve_point_float(source.curve(use.curve_id), b if k == 16 else a + (b-a)*k/16)
                               for k in range(17)]
        u0, u1, v0, v1 = carrier.uv_bounds
        grade = 1e-9*max(1., *map(abs, carrier.uv_bounds))
        for u, v in points:
            self.assertTrue(u0-grade <= u <= u1+grade and v0-grade <= v <= v1+grade, ((u, v), carrier.uv_bounds))
        lo_u, hi_u = min(p[0] for p in points), max(p[0] for p in points)
        lo_v, hi_v = min(p[1] for p in points), max(p[1] for p in points)
        self.assertLessEqual(max(lo_u-u0, u1-hi_u, lo_v-v0, v1-hi_v), 1e-6, carrier.uv_bounds)
        self.assertGreaterEqual(hi_u-lo_u, 2*math.pi-1e-6)


if __name__ == '__main__':
    unittest.main()
