"""The independent oracle checks every face's bounds and the mapped items it reads; fleet only.

B2-reader-export review round 1: qualify_original_geometry's per-face bound checks
had moved under the corrected-faces branch, so they ran on one leftover face only
(on OM10, 9,161 of 9,162 faces skipped them). And original_mapped_occurrences skipped
a mapped representation that reaches a solid through related shape representation
relationships. Each corruption below must be refused by its named message.

Fixtures: void-block.step (outer and void shells, one edge loop per face) and
apex-cone.step (a vertex loop), extracted by the adapter; the adapter's .37.26 mapped-
item structures. Oracle: the STEP text.
"""
import dataclasses
import tempfile
import unittest

from tests.test_step_ingress import FIXTURES, _Adapter, styled_mapped
from tests.test_step_reader_raw import with_root_items


class FaceBoundOracleTests(unittest.TestCase, _Adapter):

    def read(self, name, directory):
        from brepgpu.imported_source import load_source
        run, step, target = self.extract((FIXTURES/name).read_text(), directory, name.split('.')[0])
        self.assertEqual(run.returncode, 0, run.stderr)
        return load_source(target), step

    def refuses(self, source, step, message):
        from brepgpu.imported_oracles import qualify_original_geometry
        with self.assertRaisesRegex(AssertionError, message):
            qualify_original_geometry(source, step)

    def replaced(self, source, collection, row, **changes):
        rows = tuple(dataclasses.replace(r, **changes) if r.id == row.id else r for r in getattr(source, collection))
        return dataclasses.replace(source, **{collection: rows})

    def test_every_face_bound_is_checked(self):
        from brepgpu.imported_oracles import qualify_original_geometry
        with tempfile.TemporaryDirectory() as directory:
            source, step = self.read('void-block.step', directory)
            qualify_original_geometry(source, step)
            # The last face and loop are what the old code checked once; corrupt others.
            face, other = source.faces[0], source.faces[1]
            loop = source.loop(face.loop_ids[0])
            coedge = source.coedge(loop.coedge_ids[0])
            # Exchange the two faces' bounds consistently (each loop names its new face),
            # so the structural checks pass and only the comparison with the raw
            # advanced_face's bounds can see it.
            swapped = dict.fromkeys(face.loop_ids, other.id) | dict.fromkeys(other.loop_ids, face.id)
            exchanged = dataclasses.replace(
                source,
                faces=tuple(dataclasses.replace(f, loop_ids=other.loop_ids) if f.id == face.id else
                            dataclasses.replace(f, loop_ids=face.loop_ids) if f.id == other.id else f
                            for f in source.faces),
                loops=tuple(dataclasses.replace(l, face_id=swapped[l.id]) if l.id in swapped else l
                            for l in source.loops))
            cases = (
                ('face bounds', exchanged, 'original face bounds differ'),
                ('outer/inner designation', self.replaced(source, 'loops', loop, outer=not loop.outer),
                 'original outer/inner loop designation differs'),
                ('coedge loop word', self.replaced(source, 'loops', loop, coedge_ids=tuple(reversed(loop.coedge_ids))),
                 'original directed coedge loop word differs'),
                ('oriented-edge sense', self.replaced(source, 'coedges', coedge, sense=-coedge.sense),
                 'original oriented-edge/bound traversal sense differs'))
            for name, bad, message in cases:
                with self.subTest(name):
                    self.refuses(bad, step, message)

    def test_vertex_loop_is_checked(self):
        from brepgpu.imported_oracles import qualify_original_geometry
        with tempfile.TemporaryDirectory() as directory:
            source, step = self.read('apex-cone.step', directory)
            qualify_original_geometry(source, step)
            loop = next(l for l in source.loops if l.vertex_id)
            other = next(v.id for v in source.vertices if v.id != loop.vertex_id)
            self.refuses(self.replaced(source, 'loops', loop, vertex_id=other), step, 'original vertex loop differs')


class MappedItemOracleTests(unittest.TestCase):
    """original_mapped_occurrences refuses the mapped items it does not read, as the
    exporter and the adapter refuse them, rather than skipping them."""

    def test_unread_mapped_items_refuse_by_name(self):
        from pathlib import Path
        from brepgpu.imported_oracles import original_mapped_occurrences, original_records
        related = with_root_items('#962', records=(
            "#960 = SHAPE_REPRESENTATION('',(#11),#27);\n#961 = REPRESENTATION_MAP(#11,#960);\n"
            "#962 = MAPPED_ITEM('',#961,#11);\n#963 = SHAPE_REPRESENTATION_RELATIONSHIP('','',#960,#40);\n"))
        with tempfile.TemporaryDirectory() as directory:
            for name, text, message in (
                    ('direct', styled_mapped(plain=False), 'maps the representation of a product'),
                    ('related', related, 'reaches a solid through related shape_representation_relationships')):
                with self.subTest(name):
                    step = Path(directory)/f'{name}.step'
                    step.write_text(text)
                    with self.assertRaisesRegex(AssertionError, message):
                        original_mapped_occurrences(original_records(step))


if __name__ == '__main__':
    unittest.main()
