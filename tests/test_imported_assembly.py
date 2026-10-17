"""Independent literal nested-frame assembly answers; execute on fleet only."""
from dataclasses import dataclass, replace
import unittest


IDENTITY = (1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0.)


@dataclass(frozen=True)
class ProductFixture:
    id: int
    name: str
    reference: str
    definition_id: int


@dataclass(frozen=True)
class UsageFixture:
    id: int
    parent_product_id: int
    child_product_id: int
    name: str
    transform: tuple


@dataclass(frozen=True)
class DefinitionFixture:
    id: int
    product_id: int


@dataclass(frozen=True)
class SourceFixture:
    root_product_id: int
    products: tuple
    definitions: tuple
    usages: tuple

    def product(self, id):
        return next(p for p in self.products if p.id == id)

    def definition(self, id):
        return next(p for p in self.definitions if p.id == id)


def nested_source():
    # Root→subassembly rotates +90deg around Z then translates (10,20,0).
    rotate_translate = (0., -1., 0., 10., 1., 0., 0., 20., 0., 0., 1., 0.)
    # Subassembly→leaf translates local +2 X and +3 Z. R*T != T*R.
    translate = (1., 0., 0., 2., 0., 1., 0., 0., 0., 0., 1., 3.)
    products = (ProductFixture(1, 'root', 'OM-root', 0),
                ProductFixture(2, 'subassembly', 'OM-sub', 0),
                ProductFixture(3, 'leaf', 'OM-leaf', 30))
    usages = (UsageFixture(11, 1, 2, 'first', rotate_translate),
              UsageFixture(12, 1, 2, 'second', IDENTITY),
              UsageFixture(21, 2, 3, 'leaf-use', translate))
    return SourceFixture(1, products, (DefinitionFixture(30, 3),), usages)


class AssemblyDirectTests(unittest.TestCase):
    def test_actual_full_paths_and_noncommuting_nested_rigid_placement(self):
        from brepgpu.imported_assembly import assemble
        source = nested_source()
        assembly = assemble(source)
        self.assertEqual(assembly.root_product_id, 1)
        self.assertEqual(len(assembly.occurrences), 2)
        first, second = assembly.occurrences
        self.assertEqual((first.id, first.path, first.product_id, first.definition_id),
                         (1, (11, 21), 3, 30))
        self.assertEqual((second.id, second.path, second.product_id, second.definition_id),
                         (2, (12, 21), 3, 30))
        self.assertEqual(first.transform,
                         (0., -1., 0., 10., 1., 0., 0., 22., 0., 0., 1., 3.))
        self.assertEqual(second.transform,
                         (1., 0., 0., 2., 0., 1., 0., 0., 0., 0., 1., 3.))
        self.assertEqual((first.name, first.reference), ('leaf', 'OM-leaf'))

    def test_cycles_dangling_products_and_nonrigid_placements_raise(self):
        from brepgpu.imported_assembly import assemble
        source = nested_source()
        bad = (
            replace(source, usages=source.usages + (UsageFixture(22, 2, 1, 'cycle', IDENTITY),)),
            replace(source, usages=(replace(source.usages[0], child_product_id=999),) + source.usages[1:]),
            replace(source, usages=(replace(source.usages[0], transform=(2.,) + IDENTITY[1:]),) + source.usages[1:]),
        )
        for changed in bad:
            with self.subTest(source=changed), self.assertRaises(ValueError):
                assemble(changed)

    def test_duplicate_source_identifiers_and_ambiguous_solid_parent_raise(self):
        from brepgpu.imported_assembly import assemble
        source = nested_source()
        bad = (
            replace(source, products=source.products + source.products[:1]),
            replace(source, definitions=source.definitions + source.definitions),
            replace(source, usages=source.usages + source.usages[:1]),
            replace(source, definitions=(DefinitionFixture(30, 2),)),
            replace(source, usages=source.usages + (UsageFixture(31, 3, 2, 'solid-has-child', IDENTITY),)),
        )
        for index, changed in enumerate(bad):
            with self.subTest(case=index), self.assertRaises(ValueError):
                assemble(changed)

    def test_nonfinite_and_reflecting_placement_raise(self):
        from brepgpu.imported_assembly import assemble
        source = nested_source()
        for matrix in ((-1.,) + IDENTITY[1:],
                       IDENTITY[:3] + (float('inf'),) + IDENTITY[4:],
                       (float('nan'),) + IDENTITY[1:]):
            changed = replace(source, usages=(replace(source.usages[0], transform=matrix),) + source.usages[1:])
            with self.subTest(matrix=matrix), self.assertRaises(ValueError):
                assemble(changed)

    def test_world_explosion_preserves_exact_assembled_endpoint_and_source(self):
        from brepgpu.imported_assembly import assemble, explosion, placed_transform
        assembly = assemble(nested_source())
        first, second = assembly.occurrences
        state = explosion(assembly, ((4., -8., 2.), (-6., 0., 4.)))
        self.assertIs(placed_transform(first, state.offsets[0], 0.), first.transform)
        self.assertIs(placed_transform(second, state.offsets[1], -0.), second.transform)
        self.assertEqual(placed_transform(first, state.offsets[0], .5),
                         (0., -1., 0., 12., 1., 0., 0., 18., 0., 0., 1., 4.))
        self.assertEqual(placed_transform(first, state.offsets[0], 1.),
                         (0., -1., 0., 14., 1., 0., 0., 14., 0., 0., 1., 5.))
        self.assertEqual(first.transform,
                         (0., -1., 0., 10., 1., 0., 0., 22., 0., 0., 1., 3.))
        for alpha in (-.1, 1.1, float('nan'), float('inf'), True):
            with self.subTest(alpha=alpha), self.assertRaises(ValueError):
                placed_transform(first, state.offsets[0], alpha)
        with self.assertRaises(ValueError):
            explosion(assembly, ((0., 0., 0.),))
        with self.assertRaises(ValueError):
            explosion(assembly, ((0., 0., 0.), (float('nan'), 0., 0.)))


if __name__ == '__main__':
    unittest.main()
