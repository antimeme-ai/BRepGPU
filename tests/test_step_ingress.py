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
        degrees (it reads the cone in radians), so the cone has no declared unit."""
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
                            'solid 15 has no declared plane-angle unit but holds angle-bearing conical_surface #45')

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


if __name__ == '__main__':
    unittest.main()
