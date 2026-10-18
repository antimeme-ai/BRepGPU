"""Placement oracles and the one placement rule (B2); fleet only.

Relations are oriented as the standard and OCCT read them: the
transformation maps rep_1 into rep_2, and the NAUO with the representations'
own products says which side is the child.

The independent STEP oracle must read cartesian_transformation_operator_3d
placements as ISO 10303-42 defines them, so it agrees with the adapter where
OCCT reads them correctly and refuses where OCCT reads them wrongly. The Python
assembly must apply the same rigid placement rule as the readers and displays.
Expected matrices are derived by hand from the Part 42 functions (base_axis,
first_proj_axis, second_proj_axis) in the AP242 MIM long form.
"""
import math
from pathlib import Path
import tempfile
import unittest

from tests.test_step_ingress import FIXTURES, _Adapter, placement_corpus, substituted


IDT = "#384 = ITEM_DEFINED_TRANSFORMATION('','',#11,#19);"
ROTATED_BOX = 386  # the NEXT_ASSEMBLY_USAGE_OCCURRENCE that #384 places
TINY = 1.110223024625e-16  # the x-ratio of frame #19's ref_direction
# Frame #19: origin (4,-3,2), axis (0,0,1), ref_direction (TINY,1,0), so the
# IDT(#11 identity, #19) map is [x y z | origin] with y = z x x = (-1,TINY,0).
TWIN_MATRIX = (TINY, -1., 0., 4., 1., TINY, 0., -3., 0., 0., 1., 2.)


def operator(axis1, axis2, origin, scale, axis3, extra=''):
    return f"#384 = CARTESIAN_TRANSFORMATION_OPERATOR_3D('','','',{axis1},{axis2},{origin},{scale},{axis3});" + extra


# name: (#384 replacement, Part 42 matrix of usage #386)
CASES = {
    # axis1 = #19's ref_direction, axis2 = its y, axis3 = its axis, origin #20:
    # the same rigid placement as the IDT.
    'twin': (operator('#22', '#600', '#20', '1.', '#21', "\n#600 = DIRECTION('',(-1.,1.110223024625E-16,0.));"),
             TWIN_MATRIX),
    # All axes and the scale omitted: d1 = (0,0,1), first_proj_axis default
    # (1,0,0), second_proj_axis default (0,1,0); a pure translation.
    'defaults': (operator('$', '$', '#20', '$', '$'), (1., 0., 0., 4., 0., 1., 0., -3., 0., 0., 1., 2.)),
    # axis2 negated: u[2] keeps axis2's sense, so the map mirrors (det -1).
    # OCCT rebuilds y as axis3 x axis1 and reads the twin's rotation.
    'mirror': (operator('#22', '#600', '#20', '1.', '#21', "\n#600 = DIRECTION('',(1.,-1.110223024625E-16,0.));"),
               (TINY, 1., 0., 4., 1., -TINY, 0., -3., 0., 0., 1., 2.)),
    # axis3 omitted, axis1 (1,0,0), axis2 (0,1,1): d1 = (0,0,1) and u[2] is
    # axis2 projected off z and x, (0,1,0); the identity rotation. OCCT takes
    # axis3 = axis1 x axis2 = (0,-1,1)/sqrt 2, a 45 degree turn about x.
    'omitted-axis3': (operator('#14', '#600', '#20', '1.', '$', "\n#600 = DIRECTION('',(0.,1.,1.));"),
                      (1., 0., 0., 4., 0., 1., 0., -3., 0., 0., 1., 2.)),
    # Scale 2 about origin #12 enlarges (P -> 2P). OCCT reads 0.5.
    'scale-2': (operator('$', '$', '#12', '2.', '$'), (2., 0., 0., 0., 0., 2., 0., 0., 0., 0., 2., 0.)),
}


class CartesianOperatorOracleTests(unittest.TestCase, _Adapter):
    """bead .37.12: the oracle checks CARTESIAN_TRANSFORMATION_OPERATOR_3D usages."""

    def text(self, name):
        return substituted((FIXTURES/'source-uv.step').read_text(), IDT, CASES[name][0])

    def original(self, name, directory):
        step = Path(directory)/f'{name}.step'
        step.write_text(self.text(name))
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        return _original_usage_transforms(original_records(step))

    def test_operator_matrices_follow_part_42(self):
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        # The twin's hand matrix is also the IDT reading of the unedited file.
        idt = _original_usage_transforms(original_records(FIXTURES/'source-uv.step'))[ROTATED_BOX]
        for a, b in zip(idt, TWIN_MATRIX):
            self.assertLessEqual(abs(a-b), 2.**-52, idt)
        with tempfile.TemporaryDirectory() as directory:
            for name, (_, expected) in CASES.items():
                with self.subTest(name):
                    actual = self.original(name, directory)[ROTATED_BOX]
                    self.assertEqual(len(actual), 12)
                    for a, b in zip(actual, expected):
                        self.assertLessEqual(abs(a-b), 2.**-52, (name, actual))

    def extracted(self, name, directory):
        from brepgpu.imported_source import load_source
        run, step, target = self.extract(self.text(name), directory, name)
        self.assertEqual(run.returncode, 0, run.stderr)
        return load_source(target), step

    def test_rigid_operators_pass_the_oracle(self):
        from brepgpu.imported_assembly import assemble
        from brepgpu.imported_oracles import qualify_original_assembly, qualify_original_geometry
        with tempfile.TemporaryDirectory() as directory:
            for name in ('twin', 'defaults'):
                with self.subTest(name):
                    source, step = self.extracted(name, directory)
                    result = qualify_original_geometry(source, step)
                    self.assertLessEqual(result['max_original_placement_error'], 2.**-32)
                    composed = qualify_original_assembly(source, step, assemble(source))
                    self.assertLessEqual(composed['max_original_composed_placement_error'], 2.**-32)

    def test_operators_the_adapter_misreads_are_refused_by_the_oracle(self):
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.imported_source import rigid_placement_status
        with tempfile.TemporaryDirectory() as directory:
            for name in ('mirror', 'omitted-axis3'):
                with self.subTest(name):
                    # The adapter (through OCCT) emits a proper rotation, so the
                    # readers admit it; only the raw reading can refuse it.
                    source, step = self.extracted(name, directory)
                    usage = next(u for u in source.usages if u.id == ROTATED_BOX)
                    self.assertEqual(rigid_placement_status(usage.transform), 'rigid')
                    with self.assertRaisesRegex(AssertionError, 'original two-frame usage placement differs'):
                        qualify_original_geometry(source, step)

    def test_invalid_operators_refuse_by_name(self):
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        base = (FIXTURES/'source-uv.step').read_text()
        cases = {
            'scale zero': (operator('$', '$', '#12', '0.', '$'), 'scale violates WR1'),
            'negative scale': (operator('$', '$', '#12', '-1.', '$'), 'scale violates WR1'),
            'axis1 parallel to axis3': (operator('#13', '$', '#12', '$', '#21'), 'axis1 is parallel to axis3'),
            'axis1 within rounding of axis3': (operator('#600', '$', '#12', '$', '#21',
                                                        "\n#600 = DIRECTION('',(0.,1.E-17,1.));"),
                                               'axis1 is parallel to axis3'),
            'zero direction': (operator('#600', '$', '#12', '$', '$', "\n#600 = DIRECTION('',(0.,0.,0.));"),
                               'original source direction degenerates'),
            # Another functionally_defined_transformation: the 2D operator
            # (its WR1 needs dim 2) is not a placement of 3D representations.
            'two-dimensional operator': (
                "#384 = CARTESIAN_TRANSFORMATION_OPERATOR_2D('','','',$,$,#600,$);"
                "\n#600 = CARTESIAN_POINT('',(0.,0.));",
                'unsupported original representation transformation operator'),
        }
        with tempfile.TemporaryDirectory() as directory:
            for name, (line, message) in cases.items():
                with self.subTest(name):
                    step = Path(directory)/'invalid.step'
                    step.write_text(substituted(base, IDT, line))
                    with self.assertRaisesRegex(AssertionError, message):
                        _original_usage_transforms(original_records(step))


RELATION = "#383 = ( REPRESENTATION_RELATIONSHIP('','',#40,#10)"  # rep_1 child box, rep_2 parent


def relation(rep1, rep2):
    return f"#383 = ( REPRESENTATION_RELATIONSHIP('','',{rep1},{rep2})"


# name: ((old, new) edits of source-uv.step). Each reads as the IDT placement
# of usage #386, TWIN_MATRIX, once oriented as the NAUO and WR2 say.
ORIENTED = {
    # rep_1 is the parent's representation: the transformation maps parent
    # into child, frames listed in that order (#19 in #10, #11 in #40).
    'reversed-relation': ((RELATION, relation('#10', '#40')),
                          (IDT, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#19,#11);")),
    # Relation as written, frames listed the other way round (G1 P3): #19
    # belongs only to rep_2 and #11 to both, so only the swapped pair meets WR2.
    'swapped-frames': ((IDT, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#19,#11);"),),
    # Reversed relation placed by the inverse operator: axes are the rows of
    # the twin's rotation, origin -R^T (4,-3,2).
    'reversed-operator': ((RELATION, relation('#10', '#40')),
                          (IDT, operator('#600', '#601', '#602', '1.', '#21',
                                         "\n#600 = DIRECTION('',(1.110223024625E-16,-1.,0.));"
                                         "\n#601 = DIRECTION('',(1.,1.110223024625E-16,0.));"
                                         "\n#602 = CARTESIAN_POINT('',(3.,4.,-2.));"))),
}

# The box's representation (rep_1 of every usage) and the assembly's (rep_2).
BOX_REP = "#40 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#41),#371);"
ASSEMBLY_REP = "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23),#27);"
# PDM Schema Usage Guide r4.3 §3.4.3, Examples 14 and 15: an
# item_defined_transformation from a rotated, translated transform_item_1
# (#1030) to transform_item_2 (#1080), and the guide's own conversion
# C = B A^-1, v = u - C t to the equivalent cartesian operator.
PDM_MATRIX = (0.7071068, -0.3535534, -0.6123724, 3.4835639,
              0.7071068, 0.3535534, 0.6123724, -0.8977775,
              0.0, -0.8660254, 0.5, 0.3660254)
PDM_ITEMS = ((BOX_REP, BOX_REP.replace('(#11,#41)', '(#11,#41,#703)')),
             (ASSEMBLY_REP, ASSEMBLY_REP.replace('#23)', '#23,#707)')),
             (IDT, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#703,#707);"
                   "\n#700 = CARTESIAN_POINT('',(1.,1.,3.));"
                   "\n#701 = DIRECTION('',(0.,-0.8660254,0.5));"
                   "\n#702 = DIRECTION('',(1.,0.,0.));"
                   "\n#703 = AXIS2_PLACEMENT_3D('',#700,#701,#702);"
                   "\n#704 = CARTESIAN_POINT('',(2.,2.,1.));"
                   "\n#705 = DIRECTION('',(0.,0.,1.));"
                   "\n#706 = DIRECTION('',(0.7071068,0.7071068,0.));"
                   "\n#707 = AXIS2_PLACEMENT_3D('',#704,#705,#706);"))
# Example 15 prints axis2 as (-0.3535..., 0.3535..., +0.8660...) and with a
# decimal comma; the columns of its own C (and axis1 x axis2 = axis3) need
# -0.8660..., so the sign is corrected here. As printed, Part 42 reads a mirror.
PDM_OPERATOR = ((IDT, operator('#710', '#711', '#712', '$', '#713',
                               "\n#710 = DIRECTION('',(0.7071067811865,0.7071067811865,0.));"
                               "\n#711 = DIRECTION('',(-0.3535533905933,0.3535533905933,-0.8660254037844));"
                               "\n#712 = CARTESIAN_POINT('',(3.4835639,-0.8977775,0.3660254));"
                               "\n#713 = DIRECTION('',(-0.6123724356958,0.6123724356958,0.5));")),)
# The box placed by a cartesian operator in a reversed relation, scale 2:
# rep_1 (parent) maps into rep_2 (child) by P -> (4,-3,2) + 2 U P with U a
# quarter turn about z, so the usage reads U^T (P - (4,-3,2)) / 2.
REVERSED_SCALED = ((RELATION, relation('#10', '#40')),
                   (IDT, operator('#600', '#601', '#20', '2.', '$',
                                  "\n#600 = DIRECTION('',(0.,1.,0.));"
                                  "\n#601 = DIRECTION('',(-1.,0.,0.));")))
REVERSED_SCALED_MATRIX = (0., .5, 0., 1.5, -.5, 0., 0., 2., 0., 0., .5, -1.)
# Usage #380 (first box, identity placement) related by a plain
# shape_representation_relationship, with no transformation.
IDENTITY_RELATION = (("#377 = ( REPRESENTATION_RELATIONSHIP('','',#40,#10) \n"
                      "REPRESENTATION_RELATIONSHIP_WITH_TRANSFORMATION(#378) \n"
                      "SHAPE_REPRESENTATION_RELATIONSHIP() );",
                      "#377 = SHAPE_REPRESENTATION_RELATIONSHIP('','',#40,#10);"),)
FIRST_BOX = 380
UNTRANSFORMED = 'original placing relation carries no transformation'
# Usage #386 related to the cylinder's representation #394 instead of the
# assembly's; #11 is an item of both, so WR2 holds and only the products differ.
UNRELATED = ((RELATION, relation('#40', '#394')),
             (IDT, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#11,#11);"))


def edited(*edits):
    text = (FIXTURES/'source-uv.step').read_text()
    for old, new in edits:
        text = substituted(text, old, new)
    return text


class RelationOrientationTests(unittest.TestCase, _Adapter):
    """Further finding (3): the oracle orients each relation by its NAUO and WR2."""

    def test_oriented_relations_read_the_same_placement(self):
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        with tempfile.TemporaryDirectory() as directory:
            for name, edits in ORIENTED.items():
                with self.subTest(name):
                    step = Path(directory)/f'{name}.step'
                    step.write_text(edited(*edits))
                    actual = _original_usage_transforms(original_records(step))[ROTATED_BOX]
                    for a, b in zip(actual, TWIN_MATRIX):
                        self.assertLessEqual(abs(a-b), 2.**-50, (name, actual))

    def test_oriented_relations_pass_the_oracle_end_to_end(self):
        from brepgpu.imported_assembly import assemble
        from brepgpu.imported_oracles import qualify_original_assembly, qualify_original_geometry
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            for name, edits in ORIENTED.items():
                with self.subTest(name):
                    run, step, target = self.extract(edited(*edits), directory, name)
                    self.assertEqual(run.returncode, 0, run.stderr)
                    source = load_source(target)
                    result = qualify_original_geometry(source, step)
                    self.assertLessEqual(result['max_original_placement_error'], 2.**-32)
                    composed = qualify_original_assembly(source, step, assemble(source))
                    self.assertLessEqual(composed['max_original_composed_placement_error'], 2.**-32)

    def test_spec_examples_and_inverse_read_exactly(self):
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        cases = {'pdm-item-defined': (PDM_ITEMS, PDM_MATRIX, 1e-6),
                 'pdm-operator': (PDM_OPERATOR, PDM_MATRIX, 1e-6),
                 'reversed-scaled-operator': (REVERSED_SCALED, REVERSED_SCALED_MATRIX, 2.**-50)}
        with tempfile.TemporaryDirectory() as directory:
            for name, (edits, expected, tolerance) in cases.items():
                with self.subTest(name):
                    step = Path(directory)/f'{name}.step'
                    step.write_text(edited(*edits))
                    actual = _original_usage_transforms(original_records(step))[ROTATED_BOX]
                    for a, b in zip(actual, expected):
                        self.assertLessEqual(abs(a-b), tolerance, (name, actual))

    def test_axis2_near_the_x_z_plane_keeps_u_orthonormal(self):
        """axis1 (1,1,0), axis2 = sqrt 2 x + 1.4e-9 y: u[2] must be y exactly.

        Projecting axis2 off x by subtraction leaves the rounding of axis2 . x
        (about 2e-16) against a 1.4e-9 remainder, a 1e-7 tilt of u[2].
        """
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        edits = ((IDT, operator('#600', '#601', '#20', '1.', '$',
                                "\n#600 = DIRECTION('',(1.,1.,0.));"
                                "\n#601 = DIRECTION('',(0.999999999,1.000000001,0.));")),)
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'near-plane.step'
            step.write_text(edited(*edits))
            m = _original_usage_transforms(original_records(step))[ROTATED_BOX]
            h = math.sqrt(.5)
            for a, b in zip((m[1], m[5], m[9]), (-h, h, 0.)):
                self.assertLessEqual(abs(a-b), 2.**-52, m)
            columns = [(m[j], m[4+j], m[8+j]) for j in range(3)]
            for i in range(3):
                for j in range(3):
                    dot = sum(a*b for a, b in zip(columns[i], columns[j]))
                    self.assertLessEqual(abs(dot-(i == j)), 4*2.**-52, (i, j, m))

    def test_untransformed_placing_relations_refuse_by_name(self):
        """No source defines the placement of an untransformed relation.

        OCCT reads it as the identity; agreeing with that would not be an
        independent check, so the oracle refuses both forms by name. The
        relation still does not join the box's and the assembly's products.
        """
        from brepgpu.imported_oracles import (_original_usage_transforms, _representation_products,
                                              original_records)
        complex_form = ((IDENTITY_RELATION[0][0],
                         "#377 = ( REPRESENTATION_RELATIONSHIP('','',#40,#10) SHAPE_REPRESENTATION_RELATIONSHIP() );"),)
        with tempfile.TemporaryDirectory() as directory:
            for name, edits in (('simple', IDENTITY_RELATION), ('complex', complex_form)):
                with self.subTest(name):
                    step = Path(directory)/f'{name}.step'
                    step.write_text(edited(*edits))
                    records = original_records(step)
                    self.assertEqual(_representation_products(records)(40), {35})
                    with self.assertRaisesRegex(AssertionError, UNTRANSFORMED):
                        _original_usage_transforms(records)

    def test_untransformed_placing_relation_refuses_end_to_end(self):
        from brepgpu.imported_assembly import assemble
        from brepgpu.imported_oracles import qualify_original_assembly
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(edited(*IDENTITY_RELATION), directory, 'untransformed')
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            with self.assertRaisesRegex(AssertionError, UNTRANSFORMED):
                qualify_original_assembly(source, step, assemble(source))

    def test_occurrence_shape_definition_names_no_product(self):
        """An SDR on the NAUO's product_definition_shape adds no product.

        rep_2 is a representation whose only SDR is attached to usage #386's
        own placement shape #385; the reading then rests on rep_1 (the box's)
        alone and is the twin.
        """
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        edits = ((RELATION, relation('#40', '#730')),
                 (IDT, IDT + "\n#730 = SHAPE_REPRESENTATION('',(#19),#27);"
                       "\n#731 = SHAPE_DEFINITION_REPRESENTATION(#385,#730);"))
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'occurrence-sdr.step'
            step.write_text(edited(*edits))
            actual = _original_usage_transforms(original_records(step))[ROTATED_BOX]
            for a, b in zip(actual, TWIN_MATRIX):
                self.assertLessEqual(abs(a-b), 2.**-50, actual)

    def test_axis1_near_axis3_keeps_u_orthonormal(self):
        """axis1 1e-6 rad from axis3 (1,1,1): one projection pass leaves x
        about 1e-10 off orthogonal to z; u must be orthonormal to rounding."""
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        edits = ((IDT, operator('#600', '$', '#20', '1.', '#601',
                                "\n#600 = DIRECTION('',(1.,1.,1.000001732));"
                                "\n#601 = DIRECTION('',(1.,1.,1.));")),)
        with tempfile.TemporaryDirectory() as directory:
            step = Path(directory)/'near-axis3.step'
            step.write_text(edited(*edits))
            m = _original_usage_transforms(original_records(step))[ROTATED_BOX]
            columns = [(m[j], m[4+j], m[8+j]) for j in range(3)]
            for i in range(3):
                for j in range(3):
                    dot = sum(a*b for a, b in zip(columns[i], columns[j]))
                    self.assertLessEqual(abs(dot-(i == j)), 4*2.**-52, (i, j, m))

    def test_pdm_operator_passes_the_oracle_end_to_end(self):
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(edited(*PDM_OPERATOR), directory, 'pdm-operator')
            self.assertEqual(run.returncode, 0, run.stderr)
            result = qualify_original_geometry(load_source(target), step)
            self.assertLessEqual(result['max_original_placement_error'], 2.**-32)

    def test_adapter_misreads_a_rotated_transform_item_1(self):
        """Reader defect (M8): OCCT 8.0.1 places by item_1^-1 o item_2.

        STEPControl_ActorRead.cxx:2488 calls SetTransformation(item_2 frame,
        item_1 frame), which gp_Trsf.cxx:172-194 builds as To^-1 o From. The
        guide's C = B A^-1 equals that only when item_1 is the identity frame.
        The oracle reads the guide's value and refuses the adapter's by name.
        """
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.imported_source import load_source
        s3 = math.sqrt(3.)/2
        # A^-1 o B: rotation A^T B, translation A^T (u - t), A = 60 deg about x.
        a_rows = ((1., 0., 0.), (0., .5, s3), (0., -s3, .5))
        b = ((math.sqrt(.5), -math.sqrt(.5), 0.), (math.sqrt(.5), math.sqrt(.5), 0.), (0., 0., 1.))
        shift = (1., 1., -2.)
        occt = tuple(value for i in range(3) for value in
                     (*(sum(a_rows[i][k]*b[k][j] for k in range(3)) for j in range(3)),
                      sum(a_rows[i][k]*shift[k] for k in range(3))))
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(edited(*PDM_ITEMS), directory, 'pdm-item-defined')
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            usage = next(u for u in source.usages if u.id == ROTATED_BOX)
            for a, b_ in zip(usage.transform, occt):
                self.assertLessEqual(abs(a-b_), 1e-6, usage.transform)
            with self.assertRaisesRegex(AssertionError, 'original two-frame usage placement differs'):
                qualify_original_geometry(source, step)

    def test_unrelated_product_side_refuses_end_to_end(self):
        """rep_2 is another product's representation: OCCT reads it forward."""
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            run, step, target = self.extract(edited(*UNRELATED), directory, 'unrelated')
            self.assertEqual(run.returncode, 0, run.stderr)
            with self.assertRaisesRegex(AssertionError, 'original representation relation does not orient its usage'):
                qualify_original_geometry(load_source(target), step)

    def test_unorientable_relations_refuse_by_name(self):
        from brepgpu.imported_oracles import _original_usage_transforms, original_records
        orientation = 'original representation relation does not orient its usage'
        cases = {
            # Both sides are the child's representation: rep_2 lacks the
            # parent for forward and rep_1 lacks it for backward, so neither
            # reading holds.
            'same representation twice': ((RELATION, relation('#40', '#40')),),
            # rep_1 #740 is shape of both the assembly (#4) and the box (#34),
            # rep_2 #741 of no product: forward and backward both hold.
            'both readings hold': (
                (RELATION, relation('#740', '#741')),
                (IDT, IDT + "\n#740 = SHAPE_REPRESENTATION('',(#11),#27);"
                      "\n#741 = SHAPE_REPRESENTATION('',(#19),#27);"
                      "\n#742 = SHAPE_DEFINITION_REPRESENTATION(#4,#740);"
                      "\n#743 = SHAPE_DEFINITION_REPRESENTATION(#34,#740);")),
            # rep_2 is the cylinder product's (#389): an unrelated product.
            'unrelated product': UNRELATED,
            # Neither representation belongs to a product: no evidence.
            'unowned representations': (
                (RELATION, relation('#601', '#602')),
                (IDT, IDT + "\n#601 = SHAPE_REPRESENTATION('',(#11),#27);"
                      "\n#602 = SHAPE_REPRESENTATION('',(#19),#27);")),
        }
        frames = 'original item-defined transformation frames do not belong to its representations'
        axis2 = 'original transformation operator axis2 lies in the axis1-axis3 plane'
        nauo = 'original placed product relationship is not a NAUO'
        with tempfile.TemporaryDirectory() as directory:
            for name, edits, message in (
                    # axis2 (1,0,1) lies in the plane of axis1 (1,0,0) and the default z.
                    ('axis2 in the axis1-axis3 plane',
                     ((IDT, operator('#14', '#600', '#20', '$', '$', "\n#600 = DIRECTION('',(1.,0.,1.));")),), axis2),
                    # Default axis2 (0,1,0) parallel to axis1.
                    ('default axis2 parallel to axis1',
                     ((IDT, operator('#600', '$', '#20', '$', '$', "\n#600 = DIRECTION('',(0.,1.,0.));")),), axis2),
                    # The placement's product_definition_shape names a plain
                    # product_definition_relationship, not a NAUO.
                    ('placement of a plain relationship',
                     (("#385 = PRODUCT_DEFINITION_SHAPE('Placement','Placement of an item',#386",
                       "#385 = PRODUCT_DEFINITION_SHAPE('Placement','Placement of an item',#720"),
                      (IDT, IDT + "\n#720 = PRODUCT_DEFINITION_RELATIONSHIP('r','r','',#5,#35);")), nauo)):
                with self.subTest(name):
                    step = Path(directory)/'refused.step'
                    step.write_text(edited(*edits))
                    with self.assertRaisesRegex(AssertionError, message):
                        _original_usage_transforms(original_records(step))
            for name, edits in cases.items():
                with self.subTest(name):
                    step = Path(directory)/'refused.step'
                    step.write_text(edited(*edits))
                    with self.assertRaisesRegex(AssertionError, orientation):
                        _original_usage_transforms(original_records(step))
            with self.subTest('frame in neither representation'):
                step = Path(directory)/'refused.step'
                step.write_text(edited((IDT, "#384 = ITEM_DEFINED_TRANSFORMATION('','',#11,#600);"
                                             "\n#600 = AXIS2_PLACEMENT_3D('',#20,#21,#22);")))
                with self.assertRaisesRegex(AssertionError, frames):
                    _original_usage_transforms(original_records(step))


class AssemblyPlacementRuleTests(unittest.TestCase):
    """bead .37.13: the Python assembly applies the readers' one placement rule."""

    def test_assembly_rule_matches_corpus(self):
        from brepgpu.imported_assembly import rigid_transform
        corpus = placement_corpus()
        self.assertGreaterEqual(len(corpus), 12)
        for name, expected, transform in corpus:
            with self.subTest(name):
                if expected == 'rigid':
                    self.assertEqual(rigid_transform(transform, name), transform)
                    continue
                message = 'rotation is not orthonormal' if expected == 'nonorthonormal' else 'proper rotation'
                with self.assertRaisesRegex(ValueError, f'{name}: .*{message}'):
                    rigid_transform(transform, name)

    def test_composed_placement_follows_the_same_rule(self):
        from brepgpu.imported_assembly import compose, rigid_transform
        rows = {name: transform for name, _, transform in placement_corpus()}
        # Each factor's off-diagonal Gram entry is 5e-10. Two composed reach
        # 1e-9, still admitted; three reach 1.5e-9, so that composed
        # occurrence refuses by name although every usage was admitted.
        inside = rows['gram-offdiag-inside']
        twice = compose(inside, inside)
        self.assertTrue(math.isclose(twice[1], 1e-9, rel_tol=1e-6))
        with self.assertRaisesRegex(ValueError, 'composed placement: rotation is not orthonormal'):
            compose(twice, rigid_transform(inside))


if __name__ == '__main__':
    unittest.main()
