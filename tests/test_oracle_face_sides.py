"""The independent oracle accepts a corrected face side only with a record and its own check; fleet only.

Bead .37.36 (dispatcher decision): the STEP source adapter writes the outward side
where a face's raw use (same_sense composed with its shell) points into the
material, S1 for one face against its own bound and S2 for a whole shell inside
out, each with a source_face_orientation_raw record. qualify_original_geometry
accepts a written side other than the raw use only where that record exists and
an independent check (original_face_sides: the reference exporter's ray parity,
no code shared with the adapter) confirms the raw use points into the material.

Fixtures: the adapter's OrientationTests blocks (tests/test_step_reader_raw.py
flipped_block) for the side check, and the same edits on the whole void block for
qualification. Oracles: the divergence-theorem volume (1000 / 936 mm^3 when every
written side is outward); a forged record must refuse.
"""
import dataclasses
from pathlib import Path
import tempfile
import unittest

from tests import test_step_ingress as ingress
from tests.test_step_ingress import _Adapter
from tests.test_step_reader_raw import BLOCK_FACES, flipped_block

ALL = [face for face, _ in BLOCK_FACES]


def flipped_void_block(same_sense=(), bounds=()):
    """flipped_block's edits applied to the whole void-block.step (the outer cube's faces
    keep their ids): the adapter's plain_block leaves the void's entities in the file,
    which the oracle's complete-ID check refuses before it reaches orientation."""
    from tests.test_step_reader_raw import BLOCK_SURFACES
    from tests.test_step_ingress import FIXTURES, substituted
    text = (FIXTURES/'void-block.step').read_text()
    for face, sense in BLOCK_FACES:
        other = '.T.' if sense == '.F.' else '.F.'
        if face in same_sense:
            text = substituted(text, f"#{face} = ADVANCED_FACE('',(#{face+1}),#{BLOCK_SURFACES[face]},{sense});",
                               f"#{face} = ADVANCED_FACE('',(#{face+1}),#{BLOCK_SURFACES[face]},{other});")
        if face in bounds:
            text = substituted(text, f"#{face+1} = FACE_BOUND('',#{face+2},{sense});", f"#{face+1} = FACE_BOUND('',#{face+2},{other});")
    return text


class FaceSideOracleTests(unittest.TestCase, _Adapter):

    def read(self, text, directory, name):
        from brepgpu.imported_source import load_source
        run, step, target = self.extract(text, directory, name)
        self.assertEqual(run.returncode, 0, run.stderr)
        return load_source(target), step

    def test_independent_sides_of_the_fixtures(self):
        from brepgpu.imported_oracles import original_face_sides
        with tempfile.TemporaryDirectory() as directory:
            for name, text, inward in (('plain', ingress.plain_block(), set()),
                                       ('s1', flipped_block(same_sense=(57,)), {57}),
                                       ('s2', flipped_block(same_sense=ALL, bounds=ALL), set(ALL))):
                with self.subTest(name):
                    step = Path(directory)/f'{name}.step'
                    step.write_text(text)
                    sides = original_face_sides(step, ALL)
                    self.assertEqual({f for f, side in sides.items() if side == 'into_material'}, inward)
                    self.assertEqual({f for f, side in sides.items() if side == 'outward'}, set(ALL)-inward)

    def test_recorded_and_confirmed_corrections_pass(self):
        """S1 (face #57) and S2 (all six): the adapter writes the outward side with a record;
        the oracle passes (before .37.36's oracle change it said 'original face
        carrier/orientation/shell differs')."""
        from brepgpu.imported_oracles import qualify_original_geometry
        with tempfile.TemporaryDirectory() as directory:
            for name, text, inward in (('s1', flipped_void_block(same_sense=(57,)), {57}),
                                       ('s2', flipped_void_block(same_sense=ALL, bounds=ALL), set(ALL))):
                with self.subTest(name):
                    source, step = self.read(text, directory, name)
                    self.assertEqual({d.source_ids[1] for d in source.diagnostics
                                      if d.stage == 'source_face_orientation_raw'}, inward)
                    self.assertAlmostEqual(ingress.table_volume(source), 936., places=9)
                    qualify_original_geometry(source, step)

    def test_unconfirmed_or_unrecorded_sides_refuse(self):
        from brepgpu.imported_oracles import qualify_original_geometry
        from brepgpu.imported_source import Diagnostic
        with tempfile.TemporaryDirectory() as directory:
            plain, step = self.read(flipped_void_block(), directory, 'plain')
            qualify_original_geometry(plain, step)
            s1, s1_step = self.read(flipped_void_block(same_sense=(57,)), directory, 's1')
            definition = plain.definitions[0].id
            forged = Diagnostic('source_face_orientation_raw', 1, (definition, 57), 'forged')
            turned = tuple(dataclasses.replace(f, orientation=-f.orientation) if f.id == 57 else f for f in plain.faces)
            cases = (
                # A forged record on an unflipped face, its side turned: the parity check refuses.
                ('forged record, turned side', dataclasses.replace(plain, faces=turned, diagnostics=plain.diagnostics+(forged,)),
                 step, 'not confirmed by the independent check'),
                # A record on a face written as its raw use.
                ('record on a raw side', dataclasses.replace(plain, diagnostics=plain.diagnostics+(forged,)),
                 step, 'has an orientation record but is written as its raw use'),
                # A turned side without a record.
                ('turned without record', dataclasses.replace(plain, faces=turned), step,
                 'original face carrier/orientation/shell differs'),
                # The adapter's S1 correction with its record removed.
                ('correction without record', dataclasses.replace(s1, diagnostics=tuple(
                    d for d in s1.diagnostics if d.stage != 'source_face_orientation_raw')), s1_step,
                 'original face carrier/orientation/shell differs'))
            for name, source, path, message in cases:
                with self.subTest(name), self.assertRaisesRegex(AssertionError, message):
                    qualify_original_geometry(source, path)


if __name__ == '__main__':
    unittest.main()
