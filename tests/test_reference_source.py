"""Direct original-source export contracts; execute on the fleet only."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class ReferenceSourceTests(unittest.TestCase):
    def test_fresh_original_source_export(self):
        executable = Path(os.environ.get('BREPGPU_REFERENCE_SOURCE',
            'build/om10-reference-source/brepgpu-reference-source')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            source = Path(__file__).parent/'fixtures'/'reference-source.step'
            completed = subprocess.run((str(executable), str(source),
                str(Path(directory)/'export')), capture_output=True, text=True)
            self.assertEqual(completed.returncode, 0, completed.stderr)
            from brepgpu.reference_source import load_reference_source
            from brepgpu.imported_assembly import assemble
            result = load_reference_source(Path(directory)/'export'/'source.bgocct')
            self.assertEqual(result.root_product_id, 5)
            self.assertEqual(tuple(d.id for d in result.definitions), (41, 395))
            self.assertEqual(tuple(len(d.faces) for d in result.definitions), (6, 3))
            self.assertEqual(tuple(u.id for u in result.usages), (380, 386, 502))
            assembly = assemble(result)
            self.assertEqual(tuple(o.definition_id for o in assembly.occurrences), (41, 41, 395))
            self.assertEqual(assembly.occurrences[1].transform[3::4], (4., -3., 2.))


if __name__ == '__main__':
    unittest.main()
