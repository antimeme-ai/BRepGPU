"""STEP ingress and admission defects (B1) with authored fixtures; fleet only.

Voids, the plane-angle unit, the measured parameter map and the single rigid
placement rule. Every check compares the adapter's tables with an independent
answer: analytic volume, the original STEP text, a metamorphic twin, or a
re-evaluation of published rows.
"""
import dataclasses
import math
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT/'tests/fixtures'


def _tool(variable, default):
    return Path(os.environ.get(variable, ROOT/default)).resolve()


def adapter():
    return _tool('BREPGPU_STEP_SOURCE', 'build/om10-step-source/brepgpu-step-source')


def uv_fixture():
    return _tool('BREPGPU_SOURCE_UV_FIXTURE', 'build/om10-step-source/brepgpu-source-uv-fixture')


def source_inspect():
    return _tool('BREPGPU_SOURCE_INSPECT', 'build/native-mesher/brepgpu_source_inspect')


def substituted(text, old, new):
    """One authored edit of a committed fixture; the edit must match once."""
    if text.count(old) != 1:
        raise AssertionError(f'fixture substitution must match exactly once: {old!r}')
    return text.replace(old, new)


class _Adapter:
    def extract(self, step_text, directory, name='source'):
        step = Path(directory)/f'{name}.step'
        step.write_text(step_text)
        target = Path(directory)/f'{name}.bgsource'
        run = subprocess.run((str(adapter()), str(step), str(target)), capture_output=True, text=True)
        return run, step, target

    def read_unvalidated(self, target):
        """Decode tables without the reader's admission, to inspect refused rows."""
        from brepgpu import imported_source
        real = imported_source.validate_source
        imported_source.validate_source = lambda source: source
        try:
            return imported_source.load_source(target)
        finally:
            imported_source.validate_source = real


def _dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def _cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def table_volume(source):
    """Divergence-theorem volume of an all-planar polygonal source.

    V = 1/3 sum_f (n_out . p) A_f, n_out = plane normal * face orientation.
    A_f is |sum of the loops' signed polygon areas|: holes run opposite to the
    outer loop, so the sum is the face area whatever the loop labels are, and
    it does not depend on the stored orientation; only each face's sign does.
    """
    total = 0.
    for face in source.faces:
        carrier = source.carrier(face.carrier_id)
        if carrier.kind != 1:
            raise AssertionError('volume oracle needs planar carriers')
        origin, normal = carrier.frame[0:3], carrier.frame[9:12]
        signed = 0.
        for loop_id in face.loop_ids:
            points = []
            for use_id in source.loop(loop_id).coedge_ids:
                use = source.coedge(use_id)
                edge = source.edge(use.edge_id)
                points.append(source.vertex(edge.vertex_ids[0 if use.sense > 0 else 1]).xyz)
            newell = [0., 0., 0.]
            for i, p in enumerate(points):
                c = _cross(p, points[(i+1) % len(points)])
                newell = [a+b for a, b in zip(newell, c)]
            signed += _dot(newell, normal)/2
        total += face.orientation*_dot(normal, origin)*abs(signed)
    return total/3


def reference_volume(stdout):
    values = [float(line.split('\t')[2]) for line in stdout.splitlines() if line.startswith('reference_volume\t')]
    if len(values) != 1:
        raise AssertionError('adapter must print one OCCT reference volume')
    return values[0]


def mesh_volume(mesh):
    """Signed volume of an imported mesh, winding flipped by each face's sign."""
    vertices = {v.physical_key: v.xyz for v in mesh.vertices}
    total = 0.
    for triangle in mesh.triangles:
        a, b, c = (vertices[k] for k in triangle.vertex_keys)
        if triangle.corners[0].face_orientation < 0:
            b, c = c, b
        total += _dot(a, _cross(b, c))/6
    return total


def native_mesh(target, output):
    executable = _tool('BREPGPU_ADAPTIVE_MODEL', 'build/native-mesher/brepgpu-adaptive-model')
    return subprocess.run((str(executable), '--source', str(target), '--output', str(output),
                           '--positional-mm', '0.01', '--node-source-mm', '0.001', '--threads', '4',
                           '--mesh-only'), capture_output=True, text=True)


def inspect_counts(path):
    run = subprocess.run((str(source_inspect()), str(path)), capture_output=True, text=True)
    return run, dict(line.split('\t', 1) for line in run.stdout.splitlines() if '\t' in line)


class VoidShellTests(unittest.TestCase, _Adapter):
    """Defect 1: a BREP_WITH_VOIDS keeps every void shell, oriented."""

    def test_void_shell_reaches_tables_readers_and_volume(self):
        from brepgpu.imported_oracles import qualify_source, qualify_original_geometry
        fixture = FIXTURES/'void-block.step'
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(fixture.read_text(), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            from brepgpu.imported_source import load_source
            source = load_source(target)
            # Independent inventory: every original ADVANCED_FACE and the
            # BREP_WITH_VOIDS definition must be present.
            qualify_source(source, step)
            qualify_original_geometry(source, step)
            self.assertEqual(len(source.faces), 12)
            shells = {}
            for face in source.faces:
                shells.setdefault(face.shell, []).append(face)
            self.assertEqual(sorted((k, len(v)) for k, v in shells.items()), [(0, 6), (1, 6)])
            # Analytic hollow block and OCCT's volume of the translated solid.
            volume = table_volume(source)
            self.assertAlmostEqual(volume, 10.**3-4.**3, delta=1e-9)
            self.assertAlmostEqual(reference_volume(run.stdout), volume, delta=1e-9)
            records = [d for d in source.diagnostics if d.stage == 'source_void_shell']
            self.assertEqual([d.source_ids for d in records], [(15, 165, 166)])
            self.assertIn('orientation=.F.', records[0].message)
            inspected, counts = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
            self.assertEqual((counts.get('faces'), counts.get('shells'), counts.get('void_shells')), ('12', '2', '1'))

    def test_void_with_orientation_true_refuses(self):
        """ABSR WR4: every void has orientation .F.; a .T. void is refused by name."""
        flipped = substituted((FIXTURES/'void-block.step').read_text(),
                              "ORIENTED_CLOSED_SHELL('',*,#166,.F.)", "ORIENTED_CLOSED_SHELL('',*,#166,.T.)")
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(flipped, directory)
            self.assertNotEqual(run.returncode, 0, 'void with orientation .T. accepted')
            self.assertIn('ABSR WR4', run.stderr)
            self.assertFalse(target.exists())

    def test_oriented_outer_shell(self):
        """A .T. oriented outer shell is its closed shell (as OCCT reads it); .F. refuses (WR3)."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        base = (FIXTURES/'void-block.step').read_text()
        def oriented(flag):
            text = substituted(base, "#15 = BREP_WITH_VOIDS('',#16,(#165));", "#15 = BREP_WITH_VOIDS('',#400,(#165));")
            return substituted(text, "ENDSEC;\nEND-ISO", f"#400 = ORIENTED_CLOSED_SHELL('',*,#16,{flag});\nENDSEC;\nEND-ISO")
        with tempfile.TemporaryDirectory() as directory:
            first, _, a = self.extract(base, directory, 'plain')
            second, step, b = self.extract(oriented('.T.'), directory, 'oriented')
            self.assertEqual((first.returncode, second.returncode), (0, 0), first.stderr+second.stderr)
            a, b = load_source(a), load_source(b)
            self.assertEqual(a.faces, b.faces)
            self.assertEqual([d.source_ids for d in b.diagnostics if d.stage == 'source_oriented_outer_shell'],
                             [(15, 400, 16)])
            qualify_original_geometry(b, step)
            refused, _, target = self.extract(oriented('.F.'), directory, 'reversed')
            self.assertNotEqual(refused.returncode, 0)
            self.assertIn('ABSR WR3', refused.stderr)

    def test_void_block_meshes_to_its_volume_or_refuses_by_name(self):
        """Consumers: Python imported meshing gives 936; native refusal is not caused by the void."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_meshing import mesh_imported
        base = (FIXTURES/'void-block.step').read_text()
        # imported_meshing needs a FACE_OUTER_BOUND; OCCT wrote FACE_BOUND.
        outer = re.sub(r'= FACE_BOUND\(', '= FACE_OUTER_BOUND(', base)
        solid = substituted(outer, "#15 = BREP_WITH_VOIDS('',#16,(#165));", "#15 = MANIFOLD_SOLID_BREP('',#16);")
        with tempfile.TemporaryDirectory() as directory:
            run, _, void_target = self.extract(outer, directory, 'void')
            other, _, solid_target = self.extract(solid, directory, 'solid')
            self.assertEqual((run.returncode, other.returncode), (0, 0), run.stderr+other.stderr)
            mesh = mesh_imported(load_source(void_target))
            self.assertAlmostEqual(mesh_volume(mesh), 10.**3-4.**3, delta=1e-9)
            self.assertAlmostEqual(mesh_volume(mesh_imported(load_source(solid_target))), 10.**3, delta=1e-9)
            with self.assertRaisesRegex(ValueError, 'expected one outer UV chart boundary'):
                _, _, face_bound = self.extract(base, directory, 'face-bound')
                mesh_imported(load_source(face_bound))
            native = [native_mesh(target, Path(directory)/f'native-{name}')
                      for name, target in (('void', void_target), ('solid', solid_target))]
            if native[0].returncode == 0:
                self.fail('native mesher now meshes the void block; check its volume here')
            # Today the native caller refuses the block with or without its void.
            self.assertNotEqual(native[1].returncode, 0)
            self.assertEqual(native[0].stderr, native[1].stderr)
            self.assertIn('error\t', native[0].stderr)

    def test_shell_ordinals_must_be_contiguous_in_both_readers(self):
        from brepgpu import imported_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract((FIXTURES/'void-block.step').read_text(), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = imported_source.load_source(target)
            gap = dataclasses.replace(source, faces=tuple(
                dataclasses.replace(f, shell=2) if f.shell else f for f in source.faces))
            with self.assertRaisesRegex(ValueError, 'shell ordinals'):
                imported_source.validate_source(gap)
            bad = Path(directory)/'gap.bgsource'
            real = imported_source.validate_source
            imported_source.validate_source = lambda value: value
            try:
                imported_source.write_source(gap, bad)
            finally:
                imported_source.validate_source = real
            inspected, _ = inspect_counts(bad)
            self.assertNotEqual(inspected.returncode, 0)
            self.assertIn('shell ordinals', inspected.stderr)


RADIAN_UNIT = "#70 = ( NAMED_UNIT(*) PLANE_ANGLE_UNIT() SI_UNIT($,.RADIAN.) );"
DEGREE_UNIT = "#70 = ( CONVERSION_BASED_UNIT('DEGREE',#74) NAMED_UNIT(#75) PLANE_ANGLE_UNIT() );"
DEGREE_CONE = "#45 = CONICAL_SURFACE('',#46,10.,30.);"
DEGREE_FACTOR = 0.017453292519943295


class PlaneAngleUnitTests(unittest.TestCase, _Adapter):
    """Defect 3: the declared plane-angle unit governs every angle we keep."""

    def cone(self, source):
        cones = [c for c in source.carriers if c.kind == 3]
        self.assertEqual(len(cones), 1)
        return cones[0]

    def test_degree_context_cone_uses_declared_unit(self):
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        fixture = FIXTURES/'degree-cone.step'
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(fixture.read_text(), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            self.assertEqual(self.cone(source).radii, (10., 30.*DEGREE_FACTOR))
            self.assertLessEqual(abs(self.cone(source).radii[1]-math.pi/6), 2**-52)
            self.assertIn(f'source_plane_angle_factor\t{DEGREE_FACTOR!r}', run.stdout)
            qualify_original_geometry(source, step)

    def test_radian_twin_produces_the_same_source(self):
        """Metamorphic: the same cone stated in radians gives equal tables."""
        from brepgpu.imported_source import load_source
        degree = (FIXTURES/'degree-cone.step').read_text()
        radian = substituted(substituted(degree, DEGREE_UNIT, RADIAN_UNIT), DEGREE_CONE,
                             f"#45 = CONICAL_SURFACE('',#46,10.,{30.*DEGREE_FACTOR!r});")
        with tempfile.TemporaryDirectory() as directory:
            first, _, a = self.extract(degree, directory, 'degree')
            second, _, b = self.extract(radian, directory, 'radian')
            self.assertEqual((first.returncode, second.returncode), (0, 0), first.stderr+second.stderr)
            a, b = load_source(a), load_source(b)
            for name in ('carriers', 'curves', 'vertices', 'edges', 'coedges', 'loops', 'faces'):
                self.assertEqual(getattr(a, name), getattr(b, name), name)

    def assert_refused(self, text, pattern):
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertNotEqual(run.returncode, 0, 'unit-less or uninterpretable angle accepted')
            self.assertRegex(run.stderr, pattern)
            self.assertFalse(target.exists())

    def test_missing_plane_angle_unit_refuses(self):
        degree = (FIXTURES/'degree-cone.step').read_text()
        self.assert_refused(substituted(degree, '(#69,#70,#71)', '(#69,#71)'),
                            'no declared plane-angle unit but holds angle-bearing conical_surface #45')

    def test_missing_plane_angle_unit_without_angles_is_recorded(self):
        """G6 UR-4.4: no angle-bearing value, so a missing unit is recorded, not refused."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        text = substituted((FIXTURES/'void-block.step').read_text(), '((#316,#317,#318))', '((#316,#318))')
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            records = [d for d in source.diagnostics if d.stage == 'source_plane_angle_unit']
            self.assertEqual([(d.source_ids, d.message) for d in records],
                             [((15, 10), 'absent; solid holds no angle-bearing value')])
            qualify_original_geometry(source, step)

    def test_unit_less_brep_context_with_a_cone_refuses(self):
        """The B-rep context has no units; OCCT does not give it the SDR representation's
        degrees (it reads the cone in radians), so the cone has no declared unit.
        Since B2 (G6 UR-4.1) the missing length unit of that context refuses first: OCCT
        does not read this two-part context at all (ContextOfItems is null), which is
        why it took the cone in radians."""
        degree = (FIXTURES/'degree-cone.step').read_text()
        def unit_less(semi):
            text = substituted(degree, '#3 = SHAPE_DEFINITION_REPRESENTATION(#4,#10);', '#3 = SHAPE_DEFINITION_REPRESENTATION(#4,#80);')
            text = substituted(text, "#10 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#15),#68);",
                               "#10 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#15),#81);")
            text = substituted(text, DEGREE_CONE, f"#45 = CONICAL_SURFACE('',#46,10.,{semi});")
            return substituted(text, 'ENDSEC;\nEND-ISO',
                               "#80 = SHAPE_REPRESENTATION('',(#11),#68);\n"
                               "#81 = ( GEOMETRIC_REPRESENTATION_CONTEXT(3) REPRESENTATION_CONTEXT('','') );\n"
                               "#82 = SHAPE_REPRESENTATION_RELATIONSHIP('','',#10,#80);\nENDSEC;\nEND-ISO")
        # In degrees OCCT fails the face; in radians it would read silently.
        self.assert_refused(unit_less(repr(30.*DEGREE_FACTOR)),
                            'representation 10 has no readable representation context, so no length unit')

    def test_presentation_representation_does_not_govern_units(self):
        """Only the shape representation OCCT transfers through governs the solid."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        degree = (FIXTURES/'degree-cone.step').read_text()
        text = substituted(degree, 'ENDSEC;\nEND-ISO',
                           "#83 = DRAUGHTING_MODEL('',(#15),#84);\n"
                           "#84 = ( GEOMETRIC_REPRESENTATION_CONTEXT(3) GLOBAL_UNIT_ASSIGNED_CONTEXT((#69,#85,#71)) "
                           "REPRESENTATION_CONTEXT('','') );\n"
                           "#85 = ( NAMED_UNIT(*) PLANE_ANGLE_UNIT() SI_UNIT($,.RADIAN.) );\nENDSEC;\nEND-ISO")
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(self.cone(load_source(target)).radii, (10., 30.*DEGREE_FACTOR))
            qualify_original_geometry(load_source(target), step)

    def test_uninterpretable_plane_angle_unit_refuses(self):
        # DEGREE defined through another conversion-based unit, not radian.
        degree = (FIXTURES/'degree-cone.step').read_text()
        nested = substituted(degree, "#76 = ( NAMED_UNIT(*) PLANE_ANGLE_UNIT() SI_UNIT($,.RADIAN.) );",
                             "#76 = ( CONVERSION_BASED_UNIT('GRAD',#77) NAMED_UNIT(#75) PLANE_ANGLE_UNIT() );\n"
                             "#77 = PLANE_ANGLE_MEASURE_WITH_UNIT(PLANE_ANGLE_MEASURE(0.015707963267948967),#78);\n"
                             "#78 = ( NAMED_UNIT(*) PLANE_ANGLE_UNIT() SI_UNIT($,.RADIAN.) );")
        self.assert_refused(nested, 'plane-angle unit')


class ParameterMapTests(unittest.TestCase):
    """Defect 2: a non-SameParameter coedge gets a measured kind-4 map or a refusal.

    The authored edge is a line along x on the plane z = 0, so a curve-on-surface
    point and a 3D point differ only in x.
    """

    def run_mode(self, mode):
        with tempfile.TemporaryDirectory() as directory:
            return subprocess.run((str(uv_fixture()), mode, str(Path(directory)/'unused.brep')),
                                  capture_output=True, text=True)

    def published(self, mode):
        run = self.run_mode(mode)
        self.assertEqual(run.returncode, 0, run.stderr)
        rows = [line.split('\t') for line in run.stdout.splitlines()]
        kind = [int(r[1]) for r in rows if r[0] == 'parameter_map_kind']
        triples = [tuple(map(float, r[1:4])) for r in rows if r[0] == 'parameter_map_row']
        residual = [float(r[1]) for r in rows if r[0] == 'parameter_map_residual_mm']
        bound = [float(r[1]) for r in rows if r[0] == 'parameter_map_bound_mm']
        self.assertEqual((len(kind), len(residual), len(bound)), (1, 1, 1))
        self.bound = bound[0]
        self.assertEqual(kind[0], 4, 'a measured map is never published as an exact kind')
        self.assertTrue(all(0 <= r <= self.bound for _, _, r in triples))
        self.assertEqual(residual[0], max(r for _, _, r in triples))
        return [(t, s) for t, s, _ in triples]

    def check_rows(self, pairs, ta, tb, curve_x, surface_x):
        """Independent re-evaluation of the published piecewise-linear map at 1001 t."""
        self.assertGreaterEqual(len(pairs), 2)
        self.assertEqual((pairs[0][0], pairs[-1][0]), (ta, tb))
        direction = 1 if tb > ta else -1
        self.assertTrue(all(direction*(b[0]-a[0]) > 0 for a, b in zip(pairs, pairs[1:])))
        worst = 0.
        for i in range(1001):
            t = ta+(tb-ta)*i/1000
            for a, b in zip(pairs, pairs[1:]):
                if direction*(t-a[0]) >= 0 and direction*(b[0]-t) >= 0:
                    s = a[1]+(t-a[0])*(b[1]-a[1])/(b[0]-a[0])
                    break
            worst = max(worst, abs(curve_x(t)-surface_x(s)))
        self.assertLessEqual(worst, self.bound)
        return worst

    def test_identity_correspondence_is_measured(self):
        pairs = self.published('parameter-map-identity')
        self.check_rows(pairs, 0., 1., lambda t: t, lambda s: s)
        self.assertTrue(all(abs(t-s) <= 1e-12 for t, s in pairs))

    def test_reparametrized_pcurve_gets_measured_map(self):
        # P(s) = (s^2, 0): the true correspondence is s = sqrt(t), not s = t.
        pairs = self.published('parameter-map-reparametrized')
        self.assertEqual(self.bound, 1e-4)
        self.check_rows(pairs, 0., 1., lambda t: t, lambda s: s*s)
        self.assertTrue(all(abs(t-s*s) <= 1e-12 for t, s in pairs))
        # 17 initial nodes leave h^2/4 = 9.8e-4 mm per piece; the bound forces splits.
        self.assertGreater(len(pairs), 17)

    def test_reversed_use_orders_rows_from_its_start(self):
        pairs = self.published('parameter-map-reversed')
        self.check_rows(pairs, 1., 0., lambda t: t, lambda s: s*s)

    def test_quarter_point_samples_catch_an_inflection(self):
        """g has an inflection at the first piece's midpoint: its midpoint error is 0
        and its quarter-point error 3h^3/64 = 1.1e-5 mm exceeds the 5e-6 mm bound."""
        m, k = 1/32, 1.
        a1, a2, a3 = 1+k*(3*m*m-((1-m)**3+m**3)), -3*k*m, k
        pairs = self.published('parameter-map-inflection')
        self.assertEqual(self.bound, 5e-6)
        self.check_rows(pairs, 0., 1., lambda t: t, lambda s: ((a3*s+a2)*s+a1)*s)

    def test_three_dimensional_curve_knot_is_a_row(self):
        # C(t) is the line x = 0.75 t up to the knot t = 0.4 (x = 0.3), then faster.
        pairs = self.published('parameter-map-curve-knot')
        x = lambda t: .75*t if t <= .4 else .3+(t-.4)*.7/.6
        self.check_rows(pairs, 0., 1., x, lambda s: s)
        self.assertIn(.4, [t for t, _ in pairs])
        self.assertAlmostEqual(dict(pairs)[.4], .3, delta=1e-12)

    def test_three_dimensional_knot_near_a_node_snaps_to_it(self):
        """A 3D knot within projection tolerance of a node (not bitwise) moves that
        node onto the knot; it neither refuses nor adds a 1e-10-wide piece."""
        pairs = self.published('parameter-map-near-knot')
        x = lambda t: (.25+1e-10)/.4*t if t <= .4 else .25+1e-10+(t-.4)*(.75-1e-10)/.6
        self.check_rows(pairs, 0., 1., x, lambda s: s)
        self.assertIn((.4, .25), pairs)
        self.assertGreater(min(b[0]-a[0] for a, b in zip(pairs, pairs[1:])), 1e-6)

    def test_unreachable_correspondence_refuses_with_residual(self):
        # P(s) = (s, 0.01): every curve-on-surface point is 0.01 mm off C.
        run = self.run_mode('parameter-map-offset')
        self.assertNotEqual(run.returncode, 0, 'invented map published for an off-curve pcurve')
        match = re.search(r'sampled residual ([0-9.eE+-]+) mm exceeds bound 0\.001', run.stderr)
        self.assertIsNotNone(match, run.stderr)
        self.assertAlmostEqual(float(match[1]), .01, delta=1e-9)

    def test_python_consumers_refuse_measured_maps_by_name(self):
        from brepgpu.imported_source import Coedge, Edge
        from brepgpu.imported_meshing import _mapped_parameter, _curve_knots_in_edge
        edge = Edge(1, (1, 2), 3, (0., 1.), True, False, False, 0.)
        use = Coedge(4, 1, 5, 1, 6, (0., 1.), parameter_map_kind=4,
                     parameter_map=((0., 0.), (.5, .25), (1., 1.)), parameter_residuals=(0., 1e-4, 1e-4))
        with self.assertRaisesRegex(ValueError, r'measured parameter map \(kind 4\)'):
            _mapped_parameter(None, edge, use, .5, None, None)

        class Curves:
            def curve(self, id):
                return type('C', (), {'knots': (0., .5, 1.)})()
        carrier = type('K', (), {'kind': 6})()
        with self.assertRaisesRegex(ValueError, r'measured parameter map \(kind 4\)'):
            _curve_knots_in_edge(Curves(), edge, ((use, carrier),))
        # Kind 2 with several affine pieces: each pcurve knot maps through its own
        # piece and every row is a split point.
        exact = dataclasses.replace(use, parameter_map_kind=2, parameter_residuals=())
        self.assertEqual(_curve_knots_in_edge(Curves(), edge, ((exact, carrier),)),
                         [0., .5, .5+.5*(.5-.25)/.75, 1.])


def placement_corpus():
    rows = []
    for line in (FIXTURES/'rigid-placements.tsv').read_text().splitlines():
        if line and not line.startswith('#'):
            name, expected, *values = line.split('\t')
            rows.append((name, expected, tuple(float(v) for v in values)))
    return rows


class PlacementAdmissionTests(unittest.TestCase, _Adapter):
    """Defect 4: one rigid placement rule; the native test uses the same corpus."""

    def python_status(self, transform):
        from brepgpu.imported_source import (ImportedSource, Product, Usage, validate_source)
        source = ImportedSource('00'*32, 1., 0., 100, 'placement probe',
                                (Product(100, 'root', 'probe'), Product(101, 'child', 'probe')),
                                (), (Usage(99, 100, 101, 'probe usage', transform),),
                                (), (), (), (), (), (), ())
        try:
            validate_source(source)
        except ValueError as error:
            text = str(error)
            if 'nonrigid placement' in text:
                return 'nonorthonormal'
            if 'reflected placement' in text:
                return 'improper'
            return 'other:'+text
        return 'rigid'

    def test_python_reader_matches_corpus(self):
        corpus = placement_corpus()
        self.assertGreaterEqual(len(corpus), 10)
        for name, expected, transform in corpus:
            self.assertEqual(len(transform), 12, name)
            self.assertEqual(self.python_status(transform), expected, name)

    def test_scaled_occurrence_is_refused_by_every_reader(self):
        base = (FIXTURES/'source-uv.step').read_text()
        scaled = substituted(base, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#11,#19);",
                             "#384 = CARTESIAN_TRANSFORMATION_OPERATOR_3D('','','',$,$,#12,2.,$);")
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(scaled, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            emitted = {u.name: u.transform for u in self.read_unvalidated(target).usages}
            corpus = {name: (expected, transform) for name, expected, transform in placement_corpus()}
            self.assertEqual(corpus['step-cto-scale'], ('nonorthonormal', emitted['rotated-box']))
            from brepgpu.imported_source import load_source
            with self.assertRaisesRegex(ValueError, 'nonrigid placement'):
                load_source(target)
            inspected, _ = inspect_counts(target)
            self.assertNotEqual(inspected.returncode, 0)
            self.assertIn('nonrigid original source placement', inspected.stderr)


def reference_exporter():
    return _tool('BREPGPU_REFERENCE_SOURCE', 'build/om10-reference-source/brepgpu-reference-source')


def styles_fixture():
    return _tool('BREPGPU_STEP_STYLES_FIXTURE', 'build/om10-step-source/brepgpu-step-styles-fixture')


def units_fixture():
    return _tool('BREPGPU_STEP_UNITS_FIXTURE', 'build/om10-step-source/brepgpu-step-units-fixture')


SI_MM_316 = "#316 = ( LENGTH_UNIT() NAMED_UNIT(*) SI_UNIT(.MILLI.,.METRE.) );"
SI_MM_494 = "#494 = ( LENGTH_UNIT() NAMED_UNIT(*) SI_UNIT(.MILLI.,.METRE.) );"


def conversion_unit(entity, name, value, first):
    """A conversion-based length unit of `value` SI millimetres, entities from #first."""
    return (f"#{entity} = ( CONVERSION_BASED_UNIT('{name}',#{first}) LENGTH_UNIT() NAMED_UNIT(#{first+1}) );",
            f"#{first} = LENGTH_MEASURE_WITH_UNIT(LENGTH_MEASURE({value!r}),#{first+2});\n"
            f"#{first+1} = DIMENSIONAL_EXPONENTS(1.,0.,0.,0.,0.,0.,0.);\n"
            f"#{first+2} = ( LENGTH_UNIT() NAMED_UNIT(*) SI_UNIT(.MILLI.,.METRE.) );\n")


def appended(text, records):
    return substituted(text, 'ENDSEC;\nEND-ISO', records+'ENDSEC;\nEND-ISO')


def with_unit(text, line, entity, name, value, first):
    unit, extra = conversion_unit(entity, name, value, first)
    return appended(substituted(text, line, unit), extra)


TABLES = ('carriers', 'curves', 'vertices', 'edges', 'coedges', 'loops', 'faces')


def plain_block():
    """void-block.step without its void, which the reference exporter does not walk."""
    return substituted((FIXTURES/'void-block.step').read_text(),
                       "#15 = BREP_WITH_VOIDS('',#16,(#165));", "#15 = MANIFOLD_SOLID_BREP('',#16);")


class UnitAndUncertaintyTests(unittest.TestCase, _Adapter):
    """B2 .37.8, .37.9, .37.10, .37.4: units and uncertainty per representation context.

    Oracles: the STEP text's own unit entities (G6 UR-2), metamorphic twins, and
    the governing B-rep context's uncertainty in its own unit (UR-7).
    """

    def reference(self, text, directory, name='reference'):
        step = Path(directory)/f'{name}.step'
        step.write_text(text)
        out = Path(directory)/f'{name}-export'
        run = subprocess.run((str(reference_exporter()), str(step), str(out)), capture_output=True, text=True)
        return run, out/'source.bgocct'

    def test_conversion_based_millimetre_is_millimetre(self):
        """.37.8: a unit named 'MILLIMETRE' that is 1 mm reads exactly as the SI millimetre."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.reference_source import load_reference_source
        base = (FIXTURES/'void-block.step').read_text()
        named = with_unit(base, SI_MM_316, 316, 'MILLIMETRE', 1., 400)
        with tempfile.TemporaryDirectory() as directory:
            first, _, a = self.extract(base, directory, 'si')
            second, step, b = self.extract(named, directory, 'named')
            self.assertEqual((first.returncode, second.returncode), (0, 0), first.stderr+second.stderr)
            a, b = load_source(a), load_source(b)
            for name in TABLES:
                self.assertEqual(getattr(a, name), getattr(b, name), name)
            self.assertEqual((b.units_mm, b.source_uncertainty_mm), (1., 1e-7))
            qualify_original_geometry(b, step)
            run, manifest = self.reference(with_unit(plain_block(), SI_MM_316, 316, 'MILLIMETRE', 1., 400), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(load_reference_source(manifest).units_mm, 1.)

    def test_child_context_in_inches_refuses_by_factor(self):
        """.37.8: a child part whose own context is in inches; the root is millimetres."""
        child = with_unit((FIXTURES/'source-uv.step').read_text(), SI_MM_494, 494, 'INCH', 25.4, 600)
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(child, directory)
            self.assertNotEqual(run.returncode, 0, 'child context in inches accepted')
            self.assertIn('context 493 length unit 494 resolves to 25.4 mm per unit', run.stderr)
            self.assertFalse(target.exists())
            reference, manifest = self.reference(child, directory)
            self.assertNotEqual(reference.returncode, 0, 'reference export accepted inches')
            self.assertIn('resolves to 25.4 mm per unit', reference.stderr)

    def test_unit_named_millimetre_with_another_factor_refuses(self):
        """.37.8: the name is free text; the factor decides."""
        lying = with_unit(plain_block(), SI_MM_316, 316, 'millimetre', 25.4, 400)
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(lying, directory)
            self.assertNotEqual(run.returncode, 0, "25.4 mm unit named 'millimetre' accepted")
            self.assertIn('resolves to 25.4 mm per unit', run.stderr)
            reference, _ = self.reference(lying, directory)
            self.assertNotEqual(reference.returncode, 0)
            self.assertIn('resolves to 25.4 mm per unit', reference.stderr)

    def test_no_vertex_is_written_at_another_contexts_factor(self):
        """.37.9: a vertex is read with its own context's factor (25.4 here), never the root's.

        Today the child-inch file must be refused before any vertex is written; if a
        source is written, the child's vertices must be the raw coordinates times 25.4.
        """
        from brepgpu.imported_oracles import original_records
        child = with_unit((FIXTURES/'source-uv.step').read_text(), SI_MM_494, 494, 'INCH', 25.4, 600)
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(child, directory)
            if run.returncode != 0:
                self.assertIn('resolves to 25.4 mm per unit', run.stderr)
                return
            source = self.read_unvalidated(target)
            records = original_records(step)
            cylinder = source.definition(395)
            for vertex_id in cylinder.vertex_ids:
                point = records[records[vertex_id].args[1].id].args[1]
                self.assertEqual(source.vertex(vertex_id).xyz, tuple(25.4*float(x) for x in point),
                                 f'vertex {vertex_id} written at another context factor')

    def test_unit_name_disagreeing_with_its_factor_is_a_finding(self):
        """Review S9 (UR-2): 'INCH' that resolves to 1 mm is read as 1 mm, recorded."""
        from brepgpu.imported_source import load_source
        text = with_unit((FIXTURES/'void-block.step').read_text(), SI_MM_316, 316, 'INCH', 1., 400)
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            records = [d for d in load_source(target).diagnostics if d.stage == 'source_unit_name']
            self.assertEqual([d.source_ids for d in records], [(315, 316)])
            self.assertEqual({v.name: v.value for v in records[0].values},
                             {'resolved_mm_per_unit': 1., 'named_mm_per_unit': 25.4})

    def test_spaced_accuracy_name_governs_over_an_unnamed_value(self):
        """Review S10: 'distance accuracy value' (spaced) is a governing name."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        text = substituted((FIXTURES/'void-block.step').read_text(), "GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#319))",
                           "GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#319,#400))")
        text = substituted(text, "#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-07),#316,\n  'distance_accuracy_value'",
                           "#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(0.01),#316,\n  'distance accuracy value'")
        text = appended(text, "#400 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(0.5),#316,'','');\n")
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            self.assertEqual(source.source_uncertainty_mm, 0.01)
            qualify_original_geometry(source, step)

    def test_declared_zero_uncertainty_makes_no_usable_claim(self):
        """Nit (UR-7): a declared 0 is not absence; it is recorded as making no usable claim."""
        from brepgpu.imported_source import load_source
        text = substituted((FIXTURES/'void-block.step').read_text(),
                           '#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-07),#316,',
                           '#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(0.),#316,')
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            record = [d for d in load_source(target).diagnostics if d.stage == 'source_uncertainty'][0]
            values = {v.name: v.value for v in record.values}
            self.assertEqual((values['governing_mm'], values['usable']), (0., 0.))
            self.assertIn('no usable claim', record.message)

    def test_header_maximum_excludes_unusable_values(self):
        """Nit (UR-7): a declared value below the floor makes no claim, so it is not the
        header's file-wide maximum, in either exporter or in the oracle."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.reference_source import load_reference_source
        old = '#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-07),#316,'
        new = '#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-300),#316,'
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(substituted((FIXTURES/'void-block.step').read_text(), old, new), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            self.assertEqual(source.source_uncertainty_mm, 0.)
            self.assertIn('source_uncertainty_unusable\n', run.stdout)
            record = [d for d in source.diagnostics if d.stage == 'source_uncertainty'][0]
            self.assertEqual({v.name: v.value for v in record.values}['usable'], 0.)
            qualify_original_geometry(source, step)
            reference, manifest = self.reference(substituted(plain_block(), old, new), directory)
            self.assertEqual(reference.returncode, 0, reference.stderr)
            self.assertEqual(load_reference_source(manifest).uncertainty_mm, 0.)

    def test_old_source_version_refuses_by_version(self):
        """Nit: the format version moved to 2; an old file is refused naming the version."""
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract((FIXTURES/'void-block.step').read_text(), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            data = bytearray(target.read_bytes())
            self.assertEqual(data[8:12], (2).to_bytes(4, 'little'))
            data[8:12] = (1).to_bytes(4, 'little')
            old = Path(directory)/'old.bgsource'
            old.write_bytes(data)
            with self.assertRaisesRegex(ValueError, 'unsupported source version'):
                load_source(old)
            inspected, _ = inspect_counts(old)
            self.assertIn('unsupported source version', inspected.stderr)

    def test_missing_length_unit_refuses(self):
        text = substituted((FIXTURES/'void-block.step').read_text(), '((#316,#317,#318))', '((#317,#318))')
        with tempfile.TemporaryDirectory() as directory:
            run, _, _ = self.extract(text, directory)
            self.assertNotEqual(run.returncode, 0)
            self.assertIn('context 315 declares no length unit', run.stderr)

    def test_transfer_is_millimetre_under_another_global_unit(self):
        """.37.10: the XDE transfer ignores SetSystemLengthUnit; the document unit decides."""
        for unit in ('M', 'INCH'):
            run = subprocess.run((str(units_fixture()), unit, str(FIXTURES/'void-block.step')),
                                 capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout+run.stderr)
            self.assertIn('vertex_max_abs\t10\n', run.stdout, unit)
            self.assertIn('transfer_length_unit_mm\t1\n', run.stdout)

    def uncertainty_cases(self, void):
        presentation = appended(void,
            "#400 = MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION('',(#11),#401);\n"
            "#401 = ( GEOMETRIC_REPRESENTATION_CONTEXT(3) GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#402)) "
            "GLOBAL_UNIT_ASSIGNED_CONTEXT((#316,#317,#318)) REPRESENTATION_CONTEXT('','') );\n"
            "#402 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.),#316,'distance_accuracy_value','');\n")
        metres = appended(substituted(void, '#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-07),#316,',
                                      '#319 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-05),#403,'),
                          "#403 = ( LENGTH_UNIT() NAMED_UNIT(*) SI_UNIT($,.METRE.) );\n")
        assembly = (FIXTURES/'source-uv.step').read_text()
        assembly = substituted(assembly, '#31 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-07),#28,',
                               '#31 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(0.5),#28,')
        assembly = substituted(assembly, '#497 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.E-07),#494,',
                               '#497 = UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(2.E-03),#494,')
        # (name, text, governing B-rep value per solid, header value)
        return (('presentation context', presentation, {15: 1e-7}, 1e-7),
                ('measure in metres', metres, {15: 1e-5*1000.}, 1e-5*1000.),
                ('assembly', assembly, {41: 1e-7, 395: 2e-3}, 2e-3))

    def test_uncertainty_is_the_brep_contexts_own_value(self):
        """.37.4: neither a presentation context's value, nor a value in another unit,
        nor the assembly root's context governs a B-rep."""
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import qualify_original_geometry
        for name, text, solids, header in self.uncertainty_cases((FIXTURES/'void-block.step').read_text()):
            with self.subTest(name), tempfile.TemporaryDirectory() as directory:
                run, step, target = self.extract(text, directory)
                self.assertEqual(run.returncode, 0, run.stderr)
                source = load_source(target)
                self.assertEqual(source.source_uncertainty_mm, header)
                records = {d.source_ids[0]: d for d in source.diagnostics if d.stage == 'source_uncertainty'}
                self.assertEqual(sorted(records), sorted(solids))
                qualify_original_geometry(source, step)

    def test_reference_uncertainty_is_the_brep_contexts_own_value_in_mm(self):
        """.37.4 (G6): the reference export wrote the raw file maximum, unconverted."""
        from brepgpu.reference_source import load_reference_source
        for name, text, _, header in self.uncertainty_cases(plain_block()):
            with self.subTest(name), tempfile.TemporaryDirectory() as directory:
                run, manifest = self.reference(text, directory)
                self.assertEqual(run.returncode, 0, run.stderr)
                self.assertEqual(load_reference_source(manifest).uncertainty_mm, header)


class VertexLoopTests(unittest.TestCase, _Adapter):
    """B2 .37.7: a vertex_loop face bound (AP242 advanced_face WR6, WR7) is carried.

    apex-cone.step: a 10 mm cone of base radius 5 whose lateral face is bounded by
    its base circle and a vertex_loop at the apex (0, 0, 10), as the NIST CTC/FTC/STC
    models write drill points. Oracles: the STEP text (bounds, vertex point), both
    readers, and each consumer's named refusal of that face.
    """

    def source(self, directory, text=None):
        run, step, target = self.extract(text or (FIXTURES/'apex-cone.step').read_text(), directory)
        self.assertEqual(run.returncode, 0, run.stderr)
        from brepgpu.imported_source import load_source
        return run, step, target, load_source(target)

    def test_apex_vertex_loop_reaches_tables_and_readers(self):
        from brepgpu.imported_oracles import qualify_source, qualify_original_geometry
        with tempfile.TemporaryDirectory() as directory:
            run, step, target, source = self.source(directory)
            qualify_source(source, step)
            qualify_original_geometry(source, step)
            loops = {source.loop(i).id: source.loop(i) for i in source.face(30).loop_ids}
            self.assertEqual(sorted(loops), [32, 39])
            self.assertEqual((loops[39].vertex_id, loops[39].coedge_ids, loops[39].outer), (40, (), False))
            self.assertEqual(loops[32].vertex_id, 0)
            self.assertEqual(source.vertex(40).xyz, (0., 0., 10.))
            self.assertIn(40, source.definition(15).vertex_ids)
            records = [d for d in source.diagnostics if d.stage == 'source_vertex_loop']
            self.assertEqual([d.source_ids for d in records], [(30, 39, 40)])
            inspected, counts = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
            self.assertEqual((counts.get('faces'), counts.get('loops'), counts.get('vertex_loops')), ('2', '3', '1'))

    def test_consumers_refuse_the_vertex_loop_face_by_name(self):
        """Each consumer that cannot take a pole vertex bound refuses face 30, not silently."""
        from brepgpu.imported_meshing import mesh_imported
        with tempfile.TemporaryDirectory() as directory:
            _, _, target, source = self.source(directory)
            with self.assertRaisesRegex(ValueError, 'face 30: vertex_loop bound 39 at pole vertex 40'):
                mesh_imported(source)
            native = native_mesh(target, Path(directory)/'native')
            self.assertNotEqual(native.returncode, 0)
            self.assertIn('face 30: vertex_loop bound 39 at pole vertex 40', native.stderr)

    def test_readers_refuse_malformed_vertex_loops(self):
        from brepgpu import imported_source
        with tempfile.TemporaryDirectory() as directory:
            _, _, _, source = self.source(directory)
            def altered(loop_change, name):
                loops = tuple(loop_change(l) if l.id == 39 else l for l in source.loops)
                bad = dataclasses.replace(source, loops=loops)
                path = Path(directory)/f'{name}.bgsource'
                real = imported_source.validate_source
                imported_source.validate_source = lambda value: value
                try:
                    imported_source.write_source(bad, path)
                finally:
                    imported_source.validate_source = real
                return bad, path
            for name, change, message in (
                    ('foreign-vertex', lambda l: dataclasses.replace(l, vertex_id=22**3), 'vertices: unknown source ID|vertex loop'),
                    ('edge-and-vertex', lambda l: dataclasses.replace(l, coedge_ids=(36,)), 'vertex loop')):
                bad, path = altered(change, name)
                with self.subTest(name):
                    with self.assertRaisesRegex(ValueError, message):
                        imported_source.validate_source(bad)
                    inspected, _ = inspect_counts(path)
                    self.assertNotEqual(inspected.returncode, 0, name)

    APEX = "#41 = CARTESIAN_POINT('',(0.,0.,10.));"
    BASE = "#17 = ADVANCED_FACE('',(#18),#26,.T.);"
    PLANE_LOOP = ("#42 = FACE_BOUND('',#43,.T.);\n#43 = VERTEX_LOOP('',#44);\n#44 = VERTEX_POINT('',#45);\n"
                  "#45 = CARTESIAN_POINT('',(1.,1.,0.));\n")

    def refusals(self, source):
        """The vertex-loop refusals (reason 7, a shell's orientation, is the shell's)."""
        return {d.source_ids: d.message for d in source.diagnostics if d.stage == 'source_face_refused'
                and {v.name: v.value for v in d.values}.get('reason') != 7.}

    def accepted(self, text):
        with tempfile.TemporaryDirectory() as directory:
            return self.source(directory, text)[3]

    def test_pole_off_the_surface_refuses_that_face(self):
        """Review S5a: the pole must lie on the surface within the context uncertainty."""
        cone = (FIXTURES/'apex-cone.step').read_text()
        source = self.accepted(substituted(cone, self.APEX, "#41 = CARTESIAN_POINT('',(0.,0.,9.));"))
        self.assertEqual(list(self.refusals(source)), [(30, 39, 40)])
        self.assertIn('not on the face surface', self.refusals(source)[(30, 39, 40)])

    def test_pole_that_is_no_singular_point_refuses_that_face(self):
        """Review S5a: a vertex loop at the base-circle vertex #22 is on the cone but not its apex."""
        cone = (FIXTURES/'apex-cone.step').read_text()
        source = self.accepted(substituted(cone, "#39 = VERTEX_LOOP('',#40);", "#39 = VERTEX_LOOP('',#22);"))
        self.assertIn('not a singular point', self.refusals(source)[(30, 39, 22)])

    def test_plane_vertex_loop_refuses_that_face_not_the_file(self):
        """Review S5b, S5c: OCCT drops a plane's vertex loop; refuse face 17 by record. The
        dropped pole is untranslated when it is its own vertex, translated when an edge
        shares it; both are read correctly."""
        cone = (FIXTURES/'apex-cone.step').read_text()
        lone = appended(substituted(cone, self.BASE, "#17 = ADVANCED_FACE('',(#18,#42),#26,.T.);"), self.PLANE_LOOP)
        source = self.accepted(lone)
        self.assertEqual(list(self.refusals(source)), [(17, 43, 44)])
        self.assertEqual([d.source_ids[0] for d in source.diagnostics if d.stage == 'source_vertex_untranslated'], [44])
        self.assertEqual(source.vertex(44).xyz, (1., 1., 0.))
        shared = substituted(lone, "#43 = VERTEX_LOOP('',#44);", "#43 = VERTEX_LOOP('',#22);")
        source = self.accepted(shared)
        self.assertEqual(list(self.refusals(source)), [(17, 43, 22)])
        self.assertFalse([d for d in source.diagnostics if d.stage == 'source_vertex_untranslated'])

    def sphere(self, z):
        cone = (FIXTURES/'apex-cone.step').read_text()
        text = substituted(cone, "#16 = CLOSED_SHELL('',(#17,#30));", "#16 = CLOSED_SHELL('',(#30));")
        text = substituted(text, "#30 = ADVANCED_FACE('',(#31,#38),#33,.T.);", "#30 = ADVANCED_FACE('',(#38),#33,.T.);")
        text = substituted(text, "#33 = CONICAL_SURFACE('',#34,5.,0.4636476090008061);", "#33 = SPHERICAL_SURFACE('',#34,5.);")
        return substituted(text, self.APEX, f"#41 = CARTESIAN_POINT('',(0.,0.,{z!r}));")

    def test_sole_vertex_loop_on_a_sphere_binds_natural_bounds_only_on_the_surface(self):
        """Review S5a: OCCT's natural bounds bind only by its rule (sole bound, closed kind)
        and only for a pole on the surface."""
        source = self.accepted(self.sphere(5.))
        self.assertEqual(self.refusals(source), {})
        self.assertIn('natural bounds', [d.message for d in source.diagnostics if d.stage == 'source_vertex_loop'][0])
        self.assertIn((30, 39, 40), self.refusals(self.accepted(self.sphere(6.))))

    def test_reference_export_keeps_the_vertex_loop_face(self):
        from brepgpu.reference_source import load_reference_source
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'apex.step'
            step.write_text((FIXTURES/'apex-cone.step').read_text())
            out = Path(directory)/'export'
            run = subprocess.run((str(reference_exporter()), str(step), str(out)), capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(tuple(len(d.faces) for d in load_reference_source(out/'source.bgocct').definitions), (2,))

    # Review round 3 (R1, R2, R5 and nits): refusal is structural, per face, with
    # every loop's reason; OCCT's rule is read on the surface OCCT built.
    SECOND_PLANE_LOOP = ("#46 = FACE_BOUND('',#47,.T.);\n#47 = VERTEX_LOOP('',#48);\n#48 = VERTEX_POINT('',#49);\n"
                         "#49 = CARTESIAN_POINT('',(-1.,1.,0.));\n")

    def lone_plane_loop(self):
        cone = (FIXTURES/'apex-cone.step').read_text()
        return appended(substituted(cone, self.BASE, "#17 = ADVANCED_FACE('',(#18,#42),#26,.T.);"), self.PLANE_LOOP)

    def reasons(self, source):
        """Vertex-loop reasons only; reason 7 (the shell's orientation) is asserted where it matters."""
        return {d.source_ids: {v.name: v.value for v in d.values}.get('reason')
                for d in source.diagnostics if d.stage == 'source_face_refused'
                and {v.name: v.value for v in d.values}.get('reason') != 7.}

    def test_refused_face_is_structural_in_every_reader(self):
        """R1: a refused face is a status in FACES___, LOOPS___ and SOLIDS__, not only a record."""
        from brepgpu.imported_source import FACE_REFUSED, FACE_ADMITTED, DEFINITION_INCOMPLETE
        with tempfile.TemporaryDirectory() as directory:
            run, _, target, source = self.source(directory, self.lone_plane_loop())
            # Face 30 is refused with it through its edge loop 32 (reason 7): with face 17
            # refused, the shell's side is not verified (review round 2, finding 1).
            self.assertEqual({f.id: f.status for f in source.faces}, {17: FACE_REFUSED, 30: FACE_REFUSED})
            self.assertEqual({l.id: l.status for l in source.loops}, {19: 0, 43: 1, 32: 1, 39: 0})
            self.assertEqual([(d.id, d.status) for d in source.definitions], [(15, DEFINITION_INCOMPLETE)])
            self.assertIn('refused_faces\t2\n', run.stdout)
            self.assertIn('incomplete_definitions\t1\n', run.stdout)
            self.assertIn('face_refusal\tdropped_by_occt\t1\n', run.stdout)
            self.assertIn('face_refusal\torientation_undecided\t1\n', run.stdout)
            inspected, counts = inspect_counts(target)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
            self.assertEqual((counts.get('refused_faces'), counts.get('refused_loops'), counts.get('incomplete_definitions')),
                             ('2', '2', '1'))
            _, _, admitted, clean = self.source(directory, None)
            self.assertEqual({f.status for f in clean.faces} | {d.status for d in clean.definitions}, {0})

    def test_readers_refuse_an_inconsistent_status(self):
        """R1: both readers read the status and refuse what contradicts it or its record."""
        from brepgpu import imported_source
        with tempfile.TemporaryDirectory() as directory:
            _, _, _, source = self.source(directory, self.lone_plane_loop())
            def write(bad, name):
                path = Path(directory)/f'{name}.bgsource'
                real = imported_source.validate_source
                imported_source.validate_source = lambda value: value
                try:
                    imported_source.write_source(bad, path)
                finally:
                    imported_source.validate_source = real
                return path
            cases = (
                ('admitted face with a refused loop',
                 dataclasses.replace(source, faces=tuple(dataclasses.replace(f, status=0) for f in source.faces)),
                 'refused loop'),
                ('complete definition with a refused face',
                 dataclasses.replace(source, definitions=tuple(dataclasses.replace(d, status=0) for d in source.definitions)),
                 'incomplete'),
                ('refused loop without its record',
                 dataclasses.replace(source, diagnostics=tuple(d for d in source.diagnostics if d.stage != 'source_face_refused')),
                 'source_face_refused'),
                ('unknown status',
                 dataclasses.replace(source, loops=tuple(dataclasses.replace(l, status=7) if l.id == 43 else l for l in source.loops)),
                 'status'))
            for name, bad, message in cases:
                with self.subTest(name):
                    with self.assertRaisesRegex(ValueError, message):
                        imported_source.validate_source(bad)
                    inspected, _ = inspect_counts(write(bad, name.replace(' ', '-')))
                    self.assertNotEqual(inspected.returncode, 0, name)
                    self.assertRegex(inspected.stderr, message)

    def test_consumers_refuse_a_refused_face_by_its_status(self):
        """R1: every consumer reads the status before the bounds (Python, native, NVIDIA reference)."""
        from brepgpu.imported_meshing import mesh_imported
        from brepgpu.nvidia_reference import qualify_physical_definition
        with tempfile.TemporaryDirectory() as directory:
            _, _, target, source = self.source(directory, self.lone_plane_loop())
            with self.assertRaisesRegex(ValueError, 'face 17: refused at the source'):
                mesh_imported(source)
            native = native_mesh(target, Path(directory)/'native')
            self.assertNotEqual(native.returncode, 0)
            self.assertIn('face 17: refused at the source', native.stderr)
            with self.assertRaisesRegex(ValueError, 'definition 15 is incomplete at the source'):
                qualify_physical_definition(None, source, source.definitions[0])

    def test_every_refused_loop_keeps_its_reason(self):
        """Nit: two refused vertex loops on one face are two records, both reasons kept."""
        text = appended(substituted(self.lone_plane_loop(), "#17 = ADVANCED_FACE('',(#18,#42),#26,.T.);",
                                    "#17 = ADVANCED_FACE('',(#18,#42,#46),#26,.T.);"), self.SECOND_PLANE_LOOP)
        source = self.accepted(text)
        self.assertEqual(sorted(self.refusals(source)), [(17, 43, 44), (17, 47, 48)])
        self.assertEqual({l.id: l.status for l in source.loops if l.face_id == 17}, {19: 0, 43: 1, 47: 1})

    def test_untranslated_pole_tolerance_is_not_the_uncertainty(self):
        """Nit: the VERTICES tolerance is OCCT's translated vertex tolerance; an untranslated
        pole has none (0), and the context uncertainty is a field of its record."""
        source = self.accepted(self.lone_plane_loop())
        self.assertEqual(source.vertex(44).tolerance, 0.)
        record = [d for d in source.diagnostics if d.stage == 'source_vertex_untranslated'][0]
        self.assertEqual({v.name: v.value for v in record.values}, {'context_uncertainty_mm': 1e-7})

    def test_no_usable_uncertainty_refuses_the_face_as_not_evaluable(self):
        """R2 (UR-7): with no usable uncertainty the pole is not tested against a defaulted
        threshold; the face is refused as not evaluable, named."""
        cone = (FIXTURES/'apex-cone.step').read_text()
        absent = substituted(cone, "GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#64)) ", "")
        zero = substituted(cone, "LENGTH_MEASURE(1.E-07)", "LENGTH_MEASURE(0.)")
        for name, text in (('absent', absent), ('declared zero', zero)):
            with self.subTest(name):
                source = self.accepted(text)
                self.assertEqual(self.reasons(source), {(30, 39, 40): 3.})
                self.assertIn('not evaluable', self.refusals(source)[(30, 39, 40)])
                # The shell's other face is refused with it (reason 7): its side is unverified.
                self.assertEqual({f.id: f.status for f in source.faces}, {17: 1, 30: 1})
        # A pole on the base circle collapses the face's UV hint to that iso-line; with no
        # usable uncertainty it grows by one representable step, not a defaulted threshold.
        source = self.accepted(substituted(absent, "#39 = VERTEX_LOOP('',#40);", "#39 = VERTEX_LOOP('',#22);"))
        self.assertEqual(self.reasons(source), {(30, 39, 22): 3.})
        hint = next(c for c in source.carriers if c.id == 33).uv_bounds
        self.assertEqual((math.nextafter(hint[2], math.inf), math.nextafter(hint[3], -math.inf)), (0., 0.))

    TRIMMED_PLANE = "#29 = RECTANGULAR_TRIMMED_SURFACE('',#26,-10.,10.,-10.,10.,.T.,.T.);\n"

    def test_rule_reads_the_surface_occt_built_not_its_basis(self):
        """R5: on a rectangular_trimmed_surface over a plane OCCT translates the vertex loop
        (TranslateFace.cxx:547, :608-662), so the face is refused for its pole, not the file."""
        text = appended(substituted(self.lone_plane_loop(), "#17 = ADVANCED_FACE('',(#18,#42),#26,.T.);",
                                    "#17 = ADVANCED_FACE('',(#18,#42),#29,.T.);"), self.TRIMMED_PLANE)
        source = self.accepted(text)
        self.assertEqual(self.reasons(source), {(17, 43, 44): 6.})
        translation = [d.message for d in source.diagnostics if d.stage == 'source_vertex_loop' and d.source_ids[0] == 17]
        self.assertEqual(len(translation), 1)
        self.assertIn('one degenerated edge at the pole vertex', translation[0])

    def test_sole_vertex_loop_on_a_trimmed_sphere_refuses_that_face(self):
        """R5: a trimmed sphere is not the sphere OCCT's natural-bounds rule names; OCCT
        makes a degenerated edge and the bounded surface is not closed (ISO 10303-42)."""
        text = substituted(self.sphere(5.), "#30 = ADVANCED_FACE('',(#38),#33,.T.);", "#30 = ADVANCED_FACE('',(#38),#37,.T.);")
        text = appended(text, "#37 = RECTANGULAR_TRIMMED_SURFACE('',#33,0.,6.283185307179586,"
                              "-1.5707963267948966,1.5707963267948966,.T.,.T.);\n")
        source = self.accepted(text)
        self.assertEqual(self.reasons(source), {(30, 39, 40): 4.})
        self.assertIn('one degenerated edge', [d.message for d in source.diagnostics if d.stage == 'source_vertex_loop'][0])

    def bspline(self, rows, v_knots, v_mults, v_closed):
        """A sole-vertex-loop face on a B-spline (u degree 2, v degree 1), pole at (0, 0, 5)."""
        points, ids, n = [], [], 100
        for row in rows:
            ids.append([])
            for p in row:
                points.append(f"#{n} = CARTESIAN_POINT('',({p[0]!r},{p[1]!r},{p[2]!r}));\n")
                ids[-1].append(f'#{n}')
                n += 1
        net = ','.join('(' + ','.join(r) + ')' for r in ids)
        surface = (f"#33 = B_SPLINE_SURFACE_WITH_KNOTS('',2,1,({net}),.UNSPECIFIED.,.F.,{v_closed},.F.,"
                   f"(3,3),({','.join(map(str, v_mults))}),(0.,1.),({','.join(map(repr, v_knots))}),.UNSPECIFIED.);")
        text = substituted(self.sphere(5.), "#33 = SPHERICAL_SURFACE('',#34,5.);", surface)
        return appended(text, ''.join(points))

    def test_sole_vertex_loop_on_an_open_bspline_refuses_that_face(self):
        """R5 (ISO 10303-42): OCCT gives an open B-spline natural bounds; the face is refused."""
        a, b = (0., 0., 5.), (0., 0., 5.)
        rows = ((a, (5., 0., 5.)), ((0., 5., 5.), (5., 5., 5.)), ((0., 10., 5.), (5., 10., 5.)))
        source = self.accepted(self.bspline(rows, (0., 1.), (2, 2), '.F.'))
        self.assertEqual(self.reasons(source), {(30, 39, 40): 4.})

    def test_sole_vertex_loop_on_a_closed_bspline_is_admitted(self):
        """R5: a B-spline closed in v whose u sides collapse to points has no boundary."""
        top, bottom = (0., 0., 5.), (0., 0., -5.)
        ring = ((5., 0., 0.), (0., 5., 0.), (-5., 0., 0.), (0., -5., 0.), (5., 0., 0.))
        rows = ((top,)*5, ring, (bottom,)*5)
        # OCCT makes the closed B-spline periodic (TranslateFace ConvertToPeriodic) and the
        # carrier table has no periodic form, so the readers refuse the carrier row (a
        # separate defect, reported); the face's own admission is read without them.
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(self.bspline(rows, (0., 1., 2., 3., 4.), (2, 1, 1, 1, 2), '.T.'), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            source = self.read_unvalidated(target)
        self.assertEqual(self.refusals(source), {})
        self.assertEqual({f.status for f in source.faces}, {0})

    def revolution(self, trimmed):
        """A sole-vertex-loop face on a surface of revolution about z whose generatrix is a
        circle of radius 2 centred at (5, 0, 0) in the xz plane: a trimmed arc of one radian
        (an open band) or the whole circle (closed). The pole is the arc start (7, 0, 0)."""
        curve = ("#120 = TRIMMED_CURVE('',#121,(PARAMETER_VALUE(0.)),(PARAMETER_VALUE(1.)),.T.,.PARAMETER.);\n"
                 if trimmed else "")
        swept = '#120' if trimmed else '#121'
        text = substituted(self.sphere(5.), "#33 = SPHERICAL_SURFACE('',#34,5.);",
                           f"#33 = SURFACE_OF_REVOLUTION('',{swept},#125);")
        text = substituted(text, "#41 = CARTESIAN_POINT('',(0.,0.,5.0));", "#41 = CARTESIAN_POINT('',(7.,0.,0.));")
        return appended(text, curve +
            "#121 = CIRCLE('',#122,2.);\n#122 = AXIS2_PLACEMENT_3D('',#123,#124,#126);\n"
            "#123 = CARTESIAN_POINT('',(5.,0.,0.));\n#124 = DIRECTION('',(0.,1.,0.));\n"
            "#125 = AXIS1_PLACEMENT('',#12,#13);\n#126 = DIRECTION('',(1.,0.,0.));\n")

    def test_sole_vertex_loop_on_a_trimmed_arc_revolution_refuses_that_face(self):
        """Third review F2: Geom_TrimmedCurve::IsPeriodic reports its basis circle, so a trimmed
        arc generatrix is an open band; it is refused (reason 4). The whole circle is closed.
        The adapter's carrier table has no surface of revolution (it refuses the file at the
        carrier, a separate limit), so the shared rule is read through the reference
        exporter's records. Since review round 2 the exporter refuses the open band's
        definition first (its face sides cannot be checked: the shell is open), so
        its record is not reached there."""
        def refused(text):
            with tempfile.TemporaryDirectory() as directory:
                step = Path(directory)/'revolution.step'
                step.write_text(text)
                out = Path(directory)/'export'
                run = subprocess.run((str(reference_exporter()), str(step), str(out)), capture_output=True, text=True)
                self.assertEqual(run.returncode, 0, run.stderr)
                return [(r[2], r[3].split(':')[0]) for r in (l.split('\t') for l in (out/'records.tsv').read_text().splitlines())
                        if r[0] == 'source_face_refused']
        # Exporter path, review round 2 (unchecked face sides refuse): the trimmed
        # arc's band is one face whose sole bound closes nothing, so its shell is
        # open and bounds no material; the reference export refuses it by name
        # before any face record. The closed circle's torus exports with none.
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'band.step'
            step.write_text(self.revolution(True))
            run = subprocess.run((str(reference_exporter()), str(step), str(Path(directory)/'export')),
                                 capture_output=True, text=True)
            self.assertEqual(run.returncode, 1, run.stderr)
            self.assertIn("definition #15: a shell of OCCT's translation is not closed, so it bounds no material", run.stderr)
        self.assertEqual(refused(self.revolution(False)), [])

    def test_face_view_is_a_valid_source(self):
        """Third review F4: the one-face diagnostic view keeps its definition status, its own
        records, its vertex-loop vertices and no occurrence styles, so it passes the reader."""
        import sys
        sys.path.insert(0, str(ROOT/'scripts'))
        from diagnose_imported_face import face_view
        from brepgpu.imported_source import validate_source, DEFINITION_INCOMPLETE, DEFINITION_COMPLETE
        source = self.accepted(self.lone_plane_loop())
        # Both faces' views are incomplete: face 30 is refused with its shell (reason 7).
        for face, status in ((17, DEFINITION_INCOMPLETE), (30, DEFINITION_INCOMPLETE)):
            with self.subTest(face=face):
                view = face_view(source, face)
                validate_source(view)
                self.assertEqual(view.definitions[0].status, status)
                self.assertEqual(view.occurrence_styles, ())
                self.assertTrue(all(face in d.source_ids for d in view.diagnostics))
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(styled_assembly(nested=True), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            from brepgpu.imported_source import load_source
            validate_source(face_view(load_source(target), 43))

    def test_require_edge_loops_names_an_inconsistent_face(self):
        """Third review nit: a refused face without a refused loop is a named refusal, not
        StopIteration."""
        from brepgpu.imported_source import require_edge_loops
        source = self.accepted((FIXTURES/'apex-cone.step').read_text())
        bad = dataclasses.replace(source.face(30), status=1)
        with self.assertRaisesRegex(ValueError, 'face 30: refused at the source without a refused loop'):
            require_edge_loops(source, bad)

    def test_reference_export_records_the_faces_the_adapter_refuses(self):
        """Nit: the reference export keeps OCCT's face and records the adapter's refusal in a
        record file, not only on stdout."""
        from brepgpu.reference_source import load_reference_source
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'lone.step'
            step.write_text(self.lone_plane_loop())
            out = Path(directory)/'export'
            run = subprocess.run((str(reference_exporter()), str(step), str(out)), capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertEqual(tuple(len(d.faces) for d in load_reference_source(out/'source.bgocct').definitions), (2,))
            rows = [line.split('\t') for line in (out/'records.tsv').read_text().splitlines()]
            self.assertEqual(rows[0], ['stage', 'severity', 'ids', 'message'])
            refused = [r for r in rows[1:] if r[0] == 'source_face_refused']
            self.assertEqual([r[2] for r in refused], ['17 43 44'])
            self.assertIn('dropped_by_occt', refused[0][3])


def colour(entity, rgb):
    """A presentation_style_assignment #entity with a surface fill colour, entities to entity+6."""
    n = entity
    return (f"#{n} = PRESENTATION_STYLE_ASSIGNMENT((#{n+1}));\n#{n+1} = SURFACE_STYLE_USAGE(.BOTH.,#{n+2});\n"
            f"#{n+2} = SURFACE_SIDE_STYLE('',(#{n+3}));\n#{n+3} = SURFACE_STYLE_FILL_AREA(#{n+4});\n"
            f"#{n+4} = FILL_AREA_STYLE('',(#{n+5}));\n#{n+5} = FILL_AREA_STYLE_COLOUR('',#{n+6});\n"
            f"#{n+6} = COLOUR_RGB('',{rgb[0]!r},{rgb[1]!r},{rgb[2]!r});\n")


RGB = {'red': (1., 0., 0.), 'blue': (0., 0., 1.), 'yellow': (1., 1., 0.), 'cyan': (0., 1., 1.),
       'white': (1., 1., 1.), 'green': (0., 1., 0.), 'olive': (.5, .5, 0.), 'magenta': (1., 0., 1.)}
SUB_ASSEMBLY = (
    "#700 = SHAPE_DEFINITION_REPRESENTATION(#701,#707);\n#701 = PRODUCT_DEFINITION_SHAPE('','',#702);\n"
    "#702 = PRODUCT_DEFINITION('design','',#703,#706);\n#703 = PRODUCT_DEFINITION_FORMATION('','',#704);\n"
    "#704 = PRODUCT('fixture-sub','fixture-sub','',(#705));\n#705 = PRODUCT_CONTEXT('',#2,'mechanical');\n"
    "#706 = PRODUCT_DEFINITION_CONTEXT('part definition',#2,'design');\n#707 = SHAPE_REPRESENTATION('',(#11),#27);\n"
    "#710 = CONTEXT_DEPENDENT_SHAPE_REPRESENTATION(#711,#713);\n"
    "#711 = ( REPRESENTATION_RELATIONSHIP('','',#707,#10) REPRESENTATION_RELATIONSHIP_WITH_TRANSFORMATION(#712) "
    "SHAPE_REPRESENTATION_RELATIONSHIP() );\n#712 = ITEM_DEFINED_TRANSFORMATION('','',#11,#19);\n"
    "#713 = PRODUCT_DEFINITION_SHAPE('Placement','',#714);\n"
    "#714 = NEXT_ASSEMBLY_USAGE_OCCURRENCE('4','sub','',#5,#702,$);\n"
    "#720 = CONTEXT_DEPENDENT_SHAPE_REPRESENTATION(#721,#723);\n"
    "#721 = ( REPRESENTATION_RELATIONSHIP('','',#40,#707) REPRESENTATION_RELATIONSHIP_WITH_TRANSFORMATION(#722) "
    "SHAPE_REPRESENTATION_RELATIONSHIP() );\n#722 = ITEM_DEFINED_TRANSFORMATION('','',#11,#11);\n"
    "#723 = PRODUCT_DEFINITION_SHAPE('Placement','',#724);\n"
    "#724 = NEXT_ASSEMBLY_USAGE_OCCURRENCE('5','sub-box','',#702,#35,$);\n")


def styled_review():
    """The nested styled assembly plus the review's cases (entities from #760).

    S1: #762 a CDORSI teal whose item is MAPPED_ITEM #761 of the box representation,
        on first-box (SRRWT #377).
    S2: an override chain on the cylinder occurrence (SRRWT #499), listed out of order:
        #770 C overrides #772 B, #771 A overrides #630, #772 B overrides #771 A.
    S8: #780 a null style on box face #163.
    S11: #790 cyan on the sub-assembly occurrence (SRRWT #711), unrelated to #670 below it.
    """
    text = substituted(styled_assembly(nested=True), ",#670,#680),#27);",
                       ",#670,#680,#762,#770,#771,#772,#780,#790),#27);")
    return appended(text,
        "#760 = REPRESENTATION_MAP(#11,#40);\n#761 = MAPPED_ITEM('',#760,#11);\n"
        f"#762 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#763),#761,#610,(#377));\n{colour(763, (0., .5, .5))}"
        f"#770 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#800),#394,#772,(#499));\n{colour(800, (.1, .2, .3))}"
        f"#771 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#810),#394,#630,(#499));\n{colour(810, (.4, .5, .6))}"
        f"#772 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#820),#394,#771,(#499));\n{colour(820, (.7, .8, .9))}"
        "#780 = STYLED_ITEM('',(#781),#163);\n#781 = PRESENTATION_STYLE_ASSIGNMENT((NULL_STYLE(.NULL.)));\n"
        f"#790 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#830),#707,#610,(#711));\n{colour(830, RGB['cyan'])}")


def styled_mapped(plain=True):
    """S14 and review R3: plain styles on MAPPED_ITEMs held by the root's shape representation.

    A mapped item names no usage. It styles one occurrence only when that usage's placing
    relationship is a plain shape_representation_relationship between the holder and the
    mapped representation, with no transformation of its own; otherwise it collapses to the
    mapped product, recorded. #796 on #795 maps the cylinder representation #394. With
    plain=True usage #860 is placed by the plain #862 (#394, #10), so #796 styles (860,);
    usage #502 has its own transformation (#499) and is not the mapped item's. With
    plain=False only #502 exists and #796 collapses to the cylinder (a conflict with #630
    there, recorded). #799 on #798 maps the box, whose usages #380 and #386 each carry
    their own transformation: collapsed, recorded.
    """
    text = substituted(styled_assembly(), "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23),#27);",
                       "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23,#795,#798),#27);")
    text = substituted(text, ",#650,#660),#27);", ",#650,#660,#796,#799),#27);")
    records = ("#794 = REPRESENTATION_MAP(#11,#394);\n#795 = MAPPED_ITEM('',#794,#11);\n"
               f"#796 = STYLED_ITEM('',(#840),#795);\n{colour(840, (.9, .5, .1))}"
               "#797 = REPRESENTATION_MAP(#11,#40);\n#798 = MAPPED_ITEM('',#797,#11);\n"
               f"#799 = STYLED_ITEM('',(#850),#798);\n{colour(850, (.2, .9, .4))}")
    if plain:
        records += ("#860 = NEXT_ASSEMBLY_USAGE_OCCURRENCE('6','mapped-cylinder','',#5,#389,$);\n"
                    "#861 = CONTEXT_DEPENDENT_SHAPE_REPRESENTATION(#862,#863);\n"
                    "#862 = SHAPE_REPRESENTATION_RELATIONSHIP('','',#394,#10);\n"
                    "#863 = PRODUCT_DEFINITION_SHAPE('Placement','',#860);\n")
    return appended(text, records)


def styled_deep(levels=66):
    """Review R4: product P is reached from the root directly (#2900) and through a chain of
    `levels` sub-assemblies, so it has two usage paths. P's representation holds a mapped
    item of the box placed by the plain relationship of usage #2910; its style #2950 must
    collapse (one occurrence per path), however deep the second path is."""
    lines = ["#2990 = PRODUCT_CONTEXT('',#2,'mechanical');\n",
             "#2991 = PRODUCT_DEFINITION_CONTEXT('part definition',#2,'design');\n"]
    def product(b, name, items):
        lines.append(f"#{b} = PRODUCT('{name}','{name}','',(#2990));\n#{b+1} = PRODUCT_DEFINITION_FORMATION('','',#{b});\n"
                     f"#{b+2} = PRODUCT_DEFINITION('design','',#{b+1},#2991);\n#{b+3} = PRODUCT_DEFINITION_SHAPE('','',#{b+2});\n"
                     f"#{b+4} = SHAPE_REPRESENTATION('',({items}),#27);\n#{b+5} = SHAPE_DEFINITION_REPRESENTATION(#{b+3},#{b+4});\n")
        return b+2, b+4
    def usage(u, parent, child, plain=False):
        (pd, ps), (cd, cs) = parent, child
        relation = (f"SHAPE_REPRESENTATION_RELATIONSHIP('','',#{cs},#{ps})" if plain else
                    f"( REPRESENTATION_RELATIONSHIP('','',#{cs},#{ps}) REPRESENTATION_RELATIONSHIP_WITH_TRANSFORMATION(#{u+4}) "
                    "SHAPE_REPRESENTATION_RELATIONSHIP() )")
        lines.append(f"#{u} = NEXT_ASSEMBLY_USAGE_OCCURRENCE('{u}','u{u}','',#{pd},#{cd},$);\n"
                     f"#{u+1} = PRODUCT_DEFINITION_SHAPE('Placement','',#{u});\n"
                     f"#{u+2} = CONTEXT_DEPENDENT_SHAPE_REPRESENTATION(#{u+3},#{u+1});\n#{u+3} = {relation};\n"
                     + ("" if plain else f"#{u+4} = ITEM_DEFINED_TRANSFORMATION('','',#11,#11);\n"))
    root, box = (5, 10), (35, 40)
    parent = root
    for k in range(1, levels+1):
        child = product(3000+20*k, f'level-{k}', '#11')
        usage(3000+20*k+10, parent, child)
        parent = child
    p = product(2920, 'shared', '#11,#2941')
    usage(2930+3000+20*levels, parent, p)
    usage(2900, root, p)
    usage(2910, p, box, plain=True)
    lines.append(f"#2940 = REPRESENTATION_MAP(#11,#40);\n#2941 = MAPPED_ITEM('',#2940,#11);\n"
                 f"#2950 = STYLED_ITEM('',(#2960),#2941);\n{colour(2960, (.3, .6, .9))}"
                 "#2970 = MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION('',(#2950),#27);\n")
    return appended((FIXTURES/'source-uv.step').read_text(), ''.join(lines))


def styled_transitive():
    """Review round 3 nit: override links are followed transitively.

    #900 overrides #901, a null style, which overrides #620 on box face #43: #900 alone
    survives on #43, with no conflict through the colourless link. On the nested assembly
    #910 (on the sub-assembly occurrence, #711) overrides #911 (a CDORSI on first-box,
    #377), which overrides the inner #670 (#711, #721): outer, mid, inner, so #910
    supersedes #670 through #911, recorded as precedence, not a conflict.
    """
    text = substituted(styled_assembly(nested=True), ",#670,#680),#27);", ",#670,#680,#900,#901,#910,#911),#27);")
    return appended(text,
        f"#900 = OVER_RIDING_STYLED_ITEM('',(#920),#43,#901);\n{colour(920, (.2, .6, .6))}"
        "#901 = OVER_RIDING_STYLED_ITEM('',(#902),#43,#620);\n#902 = PRESENTATION_STYLE_ASSIGNMENT((NULL_STYLE(.NULL.)));\n"
        f"#910 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#930),#707,#911,(#711));\n{colour(930, (.6, .2, .6))}"
        f"#911 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#940),#40,#670,(#377));\n{colour(940, (.6, .6, .2))}")



def styled_assembly(nested=False, reversed_context=False):
    """source-uv.step with styles as CAx-IF Model Styling v1.10 writes them.

    #610 red on the box solid; #620 blue plain style on box face #43; #630 yellow on the
    cylinder's shape representation (root-style list, component part); #640 cyan and
    #650 white both on the cylinder solid (a conflict); #660 a CDORSI green on the
    rotated-box instance (style_context = its SRRWT #383). Nested adds a sub-assembly
    holding a third box, #670 olive on that instance through two relationships, and
    #680 magenta on the root's shape representation (an assembly-node style).
    """
    items = ['#610', '#620', '#630', '#640', '#650', '#660']
    records = (f"#610 = STYLED_ITEM('',(#611),#41);\n{colour(611, RGB['red'])}"
               f"#620 = STYLED_ITEM('',(#621),#43);\n{colour(621, RGB['blue'])}"
               f"#630 = STYLED_ITEM('',(#631),#394);\n{colour(631, RGB['yellow'])}"
               f"#640 = STYLED_ITEM('',(#641),#395);\n{colour(641, RGB['cyan'])}"
               f"#650 = STYLED_ITEM('',(#651),#395);\n{colour(651, RGB['white'])}"
               f"#660 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#661),#40,#610,(#383));\n{colour(661, RGB['green'])}")
    if nested:
        context = '#721,#711' if reversed_context else '#711,#721'
        records += (SUB_ASSEMBLY +
                    f"#670 = CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM('',(#671),#40,#610,({context}));\n"
                    f"{colour(671, RGB['olive'])}#680 = STYLED_ITEM('',(#681),#10);\n{colour(681, RGB['magenta'])}")
        items += ['#670', '#680']
    records = (f"#600 = MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION('',({','.join(items)}),#27);\n"
               + records)
    return appended((FIXTURES/'source-uv.step').read_text(), records)


class ColourTests(unittest.TestCase, _Adapter):
    """B2 .37.5: per-occurrence styles are read, and two styles on one item are a recorded conflict.

    Oracles: the STEP text's styled items (CAx-IF Model Styling v1.10 §4.2.1, §4.3, §5.1).
    """

    def read(self, text):
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            return load_source(target)

    def check_part_styles(self, source):
        styles = {s.id: s for s in source.styles}
        products = {p.id: p.style_id for p in source.products}
        self.assertEqual((products[35], products[389]), (610, 630))
        self.assertEqual({f.id: f.style_id for f in source.faces if f.style_id}, {43: 620})
        self.assertEqual({u.id: u.style_id for u in source.usages if u.style_id}, {})
        self.assertEqual(styles[660].rgba, (0., 1., 0., 1.))
        conflicts = [d.source_ids for d in source.diagnostics if d.stage == 'source_style_conflict']
        self.assertEqual(conflicts, [(395, 640, 650)])
        precedence = [d.source_ids for d in source.diagnostics if d.stage == 'source_style_precedence']
        self.assertEqual(precedence, [(43, 620, 610)])

    def test_occurrence_and_root_list_styles_are_read(self):
        source = self.read(styled_assembly())
        self.check_part_styles(source)
        self.assertEqual([(o.style_id, o.face_id, o.usage_ids) for o in source.occurrence_styles],
                         [(660, 0, (386,))])
        self.assertEqual({p.id: p.style_id for p in source.products}[5], 0)

    def test_nested_occurrence_path_and_assembly_node_style(self):
        for reversed_context in (False, True):
            with self.subTest(reversed_context=reversed_context):
                source = self.read(styled_assembly(nested=True, reversed_context=reversed_context))
                self.check_part_styles(source)
                self.assertEqual([(o.style_id, o.face_id, o.usage_ids) for o in source.occurrence_styles],
                                 [(660, 0, (386,)), (670, 0, (714, 724))])
                self.assertEqual({p.id: p.style_id for p in source.products}[5], 680)

    def rows(self, source):
        return {(o.usage_ids, o.face_id): o.style_id for o in source.occurrence_styles}

    def notes(self, source, stage):
        return [d.source_ids for d in source.diagnostics if d.stage == stage]

    def test_occurrence_style_on_a_mapped_item(self):
        """Review S1: a CDORSI whose item is a MAPPED_ITEM of the component's representation."""
        source = self.read(styled_review())
        self.assertEqual(self.rows(source)[((380,), 0)], 762)
        self.assertEqual(self.notes(source, 'source_style_unplaced'), [])

    def test_override_chain_does_not_depend_on_file_order(self):
        """Review S2: C overrides B overrides A, listed C, A, B: C alone survives, no conflict."""
        source = self.read(styled_review())
        self.assertEqual(self.rows(source)[((502,), 0)], 770)
        self.assertFalse([ids for ids in self.notes(source, 'source_style_conflict') if set(ids) & {770, 771, 772}])

    def test_colourless_styles_are_recorded(self):
        """Review S8: a null style is not shown, and says so."""
        source = self.read(styled_review())
        self.assertIn((780, 163), self.notes(source, 'source_style_unsupported'))

    def test_unrelated_nested_occurrence_styles_are_a_conflict(self):
        """Review S11: the sub-assembly's #790 and the inner #670 disagree without an override
        link; the deeper is kept by our rule, recorded."""
        source = self.read(styled_review())
        self.assertEqual((self.rows(source)[((714,), 0)], self.rows(source)[((714, 724), 0)]), (790, 670))
        self.assertIn((790, 670), self.notes(source, 'source_style_conflict'))

    def resolve(self, text):
        """The shared resolver on the model alone (OCCT's transfer recurses without bound on
        a plain relationship between a mapped item's holder and its representation)."""
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'styles.step'
            step.write_text(text)
            run = subprocess.run((str(styles_fixture()), str(step)), capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        lines = [l.split('\t') for l in run.stdout.splitlines()]
        rows = {(tuple(map(int, l[3:])), int(l[2])): int(l[1]) for l in lines if l[0] == 'occurrence'}
        notes = [(l[1], tuple(map(int, l[2:]))) for l in lines if l[0] == 'note']
        work = [int(l[1]) for l in lines if l[0] == 'style_path_work']
        return rows, notes, work

    def test_plain_styles_on_mapped_items(self):
        """Review S14, R3: a mapped item styles one occurrence only through a usage placed by a
        plain relationship between its holder and the mapped representation; a usage with its
        own transformation is not the mapped item's, so the style collapses, recorded."""
        rows, notes, _ = self.resolve(styled_mapped())
        self.assertEqual(rows[((860,), 0)], 796)
        self.assertNotIn(((502,), 0), rows)
        self.assertEqual([ids for stage, ids in notes if stage == 'source_style_collapsed'], [(799, 798, 40)])
        # Without the plain usage the two mapped items place solids that no usage
        # publishes, so the adapter refuses the file by name (bead .37.26,
        # tests/test_step_reader_raw.py); the resolver's collapse is read on the model.
        rows, notes, _ = self.resolve(styled_mapped(plain=False))
        self.assertNotIn(((502,), 0), rows)
        self.assertEqual(sorted(ids for stage, ids in notes if stage == 'source_style_collapsed'),
                         [(796, 795, 394), (799, 798, 40)])
        # Review finding 10: the conflict and the product styles, from the resolver on the
        # model (tests/step_styles_fixture.cpp asks it for every product's style).
        self.assertIn(('source_style_conflict', (394, 630, 796)), notes)
        products = self.product_styles(styled_mapped(plain=False))
        self.assertEqual((products[389], products[35]), (630, 799))

    def product_styles(self, text):
        """Every product's style as the shared resolver gives it, on the model alone."""
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'styles.step'
            step.write_text(text)
            run = subprocess.run((str(styles_fixture()), str(step)), capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        return {int(l.split('\t')[1]): int(l.split('\t')[2]) for l in run.stdout.splitlines() if l.startswith('product\t')}

    def test_occurrence_paths_are_counted_at_any_depth(self):
        """Review R4: paths are counted in one pass over the usage graph, saturating at two;
        a second path deeper than any cap still makes the occurrence ambiguous. The work is
        one unit per product and per usage."""
        rows, notes, work = self.resolve(styled_deep())
        self.assertFalse([key for key, style in rows.items() if style == 2950])
        self.assertIn(('source_style_collapsed', (2950, 2941, 40)), notes)
        products, usages = 3 + 66 + 1, 3 + 66 + 3
        self.assertEqual(work, [products + usages])

    def test_override_links_are_followed_transitively(self):
        """Round 3 nit: no false conflict through a colourless style or outer, mid, inner."""
        source = self.read(styled_transitive())
        self.assertEqual({f.id: f.style_id for f in source.faces}[43], 900)
        self.assertFalse([ids for ids in self.notes(source, 'source_style_conflict') if 43 in ids or 910 in ids])
        rows = self.rows(source)
        self.assertEqual((rows[((714,), 0)], rows[((380,), 0)]), (910, 911))
        self.assertNotIn(((714, 724), 0), rows)
        self.assertIn((910, 670), self.notes(source, 'source_style_precedence'))

    def test_reference_export_writes_the_same_styles(self):
        """The NVIDIA arm's qualification compares reference and owned styles one for one
        (scripts/qualify_om10_nvidia.py); both exporters resolve them alike."""
        from brepgpu.imported_source import load_source
        from brepgpu.reference_source import load_reference_source
        text = styled_assembly(nested=True)
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(text, directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            owned = load_source(target)
            out = Path(directory)/'export'
            reference = subprocess.run((str(reference_exporter()), str(step), str(out)), capture_output=True, text=True)
            self.assertEqual(reference.returncode, 0, reference.stderr)
            exported = load_reference_source(out/'source.bgocct')
            # Review S7: the reference format carries no diagnostics (they are records);
            # since bead .37.23 it carries the occurrence styles, row for row.
            self.assertIn('reference_occurrence_styles\t2\n', reference.stdout)
            self.assertEqual(exported.occurrence_styles, owned.occurrence_styles)
            self.assertIn('reference_style_note\tsource_style_conflict\t395\t640\t650\t', reference.stdout)
            self.assertEqual((exported.products, exported.usages, exported.styles),
                             (owned.products, owned.usages, owned.styles))
            self.assertEqual({f.id: f.style_id for d in exported.definitions for f in d.faces},
                             {f.id: f.style_id for f in owned.faces})

    def test_readers_refuse_a_broken_occurrence_path(self):
        from brepgpu import imported_source
        source = self.read(styled_assembly(nested=True))
        for name, path, message in (('not from the root', (724,), 'occurrence style path'),
                                    ('disconnected', (386, 724), 'occurrence style path'),
                                    ('unknown usage', (9999,), 'usages: unknown source ID')):
            bad = dataclasses.replace(source, occurrence_styles=(imported_source.OccurrenceStyle(670, 0, path),))
            with self.subTest(name), self.assertRaisesRegex(ValueError, message):
                imported_source.validate_source(bad)
        with tempfile.TemporaryDirectory() as directory:
            bad = dataclasses.replace(source, occurrence_styles=(imported_source.OccurrenceStyle(670, 0, (386, 724)),))
            path = Path(directory)/'bad.bgsource'
            real = imported_source.validate_source
            imported_source.validate_source = lambda value: value
            try:
                imported_source.write_source(bad, path)
            finally:
                imported_source.validate_source = real
            inspected, _ = inspect_counts(path)
            self.assertNotEqual(inspected.returncode, 0)
            self.assertIn('occurrence style path', inspected.stderr)


NUMBER_IN_TEXT = re.compile(r'=\s*[-+]?\d|\d\.\d|\d[eE][-+]?\d')


class DiagnosticRecordTests(unittest.TestCase, _Adapter):
    """B2 .37.1, .37.2: magnitudes are numeric fields; the BRepCheck record separates
    'could not check' from 'invalid', names the faulted entity, and carries no
    certification wording (G11 §3.1, §4)."""

    def sources(self):
        from brepgpu.imported_source import load_source
        cases = (('void-block', (FIXTURES/'void-block.step').read_text()),
                 ('degree-cone', (FIXTURES/'degree-cone.step').read_text()),
                 ('apex-cone', (FIXTURES/'apex-cone.step').read_text()),
                 ('styled-assembly', styled_assembly(nested=True)))
        with tempfile.TemporaryDirectory() as directory:
            result = {}
            for name, text in cases:
                run, _, target = self.extract(text, directory, name)
                self.assertEqual(run.returncode, 0, name+run.stderr)
                result[name] = load_source(target)
            return result

    def values(self, record):
        return {v.name: v.value for v in record.values}

    def test_no_magnitude_lives_in_message_text(self):
        for name, source in self.sources().items():
            for record in source.diagnostics:
                with self.subTest(name, stage=record.stage):
                    self.assertIsNone(NUMBER_IN_TEXT.search(record.message), record.message)

    def test_magnitudes_are_typed_fields(self):
        from brepgpu.imported_source import VALUE_KINDS
        sources = self.sources()
        def one(source, stage):
            records = [d for d in source.diagnostics if d.stage == stage]
            self.assertEqual(len(records), 1, stage)
            return records[0]
        angle = one(sources['degree-cone'], 'source_plane_angle_unit')
        self.assertEqual(self.values(angle), {'radians_per_unit': 0.017453292519943295})
        self.assertEqual(angle.values[0].kind, VALUE_KINDS['angle_rad'])
        void = one(sources['void-block'], 'source_void_shell')
        self.assertEqual(self.values(void), {'ordinal': 1., 'faces': 6.})
        uncertainty = one(sources['void-block'], 'source_uncertainty')
        self.assertEqual(self.values(uncertainty)['governing_mm'], 1e-7)
        self.assertEqual(self.values(uncertainty)['usable'], 1.)
        self.assertEqual(uncertainty.values[0].kind, VALUE_KINDS['length_mm'])

    def test_brepcheck_record_names_entities_and_separates_outcomes(self):
        for name, source in self.sources().items():
            with self.subTest(name):
                summaries = [d for d in source.diagnostics if d.stage == 'brepcheck']
                self.assertEqual(sorted(d.source_ids[0] for d in summaries), sorted(x.id for x in source.definitions))
                findings = [d for d in source.diagnostics if d.stage == 'brepcheck_finding']
                for d in summaries + findings:
                    self.assertNotRegex(d.message, '(?i)qualif|certif')
                for summary in summaries:
                    v = self.values(summary)
                    own = [f for f in findings if f.source_ids[0] == summary.source_ids[0]]
                    self.assertEqual((v['failed'], v['not_evaluated']),
                                     (sum(self.values(f)['outcome'] == 1 for f in own),
                                      sum(self.values(f)['outcome'] == 2 for f in own)))
                    self.assertGreater(v['checked_subjects'], 0)
                    # Every subject is either checked or has a not-evaluated finding of its own.
                    unchecked = sum(self.values(f)['outcome'] == 2 and f.source_ids[2] == 0 for f in own)
                    self.assertGreaterEqual(v['subjects'], v['checked_subjects'])
                    self.assertLessEqual(v['subjects'], v['checked_subjects']+unchecked)
                for finding in findings:
                    # (solid, subject, context): the subject or its context is an original entity
                    self.assertEqual(len(finding.source_ids), 3)
                    self.assertTrue(finding.source_ids[1] or finding.source_ids[2], finding)
                    self.assertIn(self.values(finding)['outcome'], (1., 2.))
        apex = [d for d in self.sources()['apex-cone'].diagnostics if d.stage == 'brepcheck_finding']
        self.assertTrue(any(30 in d.source_ids[1:] for d in apex), apex)


class TranslationLabelTests(unittest.TestCase, _Adapter):
    """B2 .37.18: the label says what ran. An empty ShapeProcess bitset is not 'no
    processing': OCCT's translation-layer edits still fire (M4-G1 section 2.1)."""

    def labels(self):
        from brepgpu.imported_source import load_source
        from brepgpu.reference_source import load_reference_source
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = self.extract((FIXTURES/'source-uv.step').read_text(), directory)
            self.assertEqual(run.returncode, 0, run.stderr)
            out = Path(directory)/'export'
            reference = subprocess.run((str(reference_exporter()), str(FIXTURES/'source-uv.step'), str(out)),
                                       capture_output=True, text=True)
            self.assertEqual(reference.returncode, 0, reference.stderr)
            return load_source(target).adapter_revision, load_reference_source(out/'source.bgocct').adapter_revision

    def test_label_names_the_translation_edits(self):
        for label in self.labels():
            with self.subTest(label=label):
                self.assertNotIn('shape-processing=none', label)
                self.assertEqual([t.strip() for t in label.split(';') if t.strip().startswith('shape-processing=')],
                                 ['shape-processing=translation-only'])
                self.assertIn('OCCT translation without ShapeProcess', label)
                self.assertIn('translation-layer edits apply', label)
                self.assertIn('E1-E16, G1-G22, T1-T8, U1-U4, P1-P4, R1-R3', label)


if __name__ == '__main__':
    unittest.main()
