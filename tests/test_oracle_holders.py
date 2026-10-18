"""The independent oracle finds a solid's holders at any depth, as the adapter does; fleet only.

Bead .37.32: the adapter's transfer_path (native/step_units.hpp) and the oracle's
_RepresentationGraph.transferred (python/brepgpu/imported_oracles.py) both searched
to depth 8, so a holder nine plain relationships from its product's representation
escaped the unit gate. The adapter now visits every representation once at any
depth; the oracle must agree with it, so the two never disagree on deeper holders.

Fixture: the adapter's nesting (tests/test_step_reader_raw.py deep_holder), a second
holder #1009 of box solid #41 joined to #40 by nine plain relationships, its context
in inches or in the assembly's millimetres.
Oracle: the STEP text's unit entities; the adapter's verdict on the same files.
"""
import dataclasses
import hashlib
from pathlib import Path
import tempfile
import unittest

from tests.test_step_ingress import _Adapter
from tests.test_step_reader_raw import deep_holder


class DeepHolderOracleTests(unittest.TestCase, _Adapter):

    def test_the_oracle_holds_a_deep_holder_and_moves_with_the_adapter(self):
        from brepgpu.imported_oracles import _RepresentationGraph, original_records, qualify_original_geometry
        from brepgpu.imported_source import load_source
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            # Millimetres: both admit, and the oracle now counts #1009 among the box's holders.
            run, mm_step, target = self.extract(deep_holder(False), directory, 'millimetres')
            self.assertEqual(run.returncode, 0, run.stderr)
            source = load_source(target)
            self.assertIn(1009, _RepresentationGraph(original_records(mm_step)).solid_holders(41))
            qualify_original_geometry(source, mm_step)
            # Inches: the adapter refuses by #1009's factor; the oracle, given the
            # millimetre twin's tables stamped with this file's digest (the same
            # entities apart from #1009's context), refuses on the same holder's unit.
            run, inch_step, _ = self.extract(deep_holder(True), directory, 'inches')
            self.assertEqual(run.returncode, 1, run.stderr)
            self.assertIn('representation 1009 context 1020 length unit 1021 resolves to 25.4 mm per unit', run.stderr)
            twin = dataclasses.replace(source, source_sha256=hashlib.sha256(inch_step.read_bytes()).hexdigest())
            with self.assertRaisesRegex(AssertionError, 'original solid context length unit differs from owned source'):
                qualify_original_geometry(twin, inch_step)


if __name__ == '__main__':
    unittest.main()
