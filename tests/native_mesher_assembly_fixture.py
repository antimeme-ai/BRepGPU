"""Nested/reused placements over the existing independently authored source."""
from dataclasses import replace
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from native_mesher_boundary_fixture import main as boundary_fixture
from brepgpu.imported_source import Product, Usage, IDENTITY, load_source, write_source


def main():
    boundary_fixture()
    source = load_source(sys.argv[1])
    quarter = (0., -1., 0., 13., 1., 0., 0., -7., 0., 0., 1., .5)
    scaled = list(IDENTITY)
    scaled[0] = 1. + 2.**-40
    # Three exact compositions retain terms that intermediate double matrix
    # products would discard. Definition201 deliberately has no usage path.
    usages = (
        Usage(205, 100, 101, "direct", IDENTITY),
        Usage(201, 100, 105, "quarter", quarter),
        Usage(202, 105, 106, "scale A", tuple(scaled)),
        Usage(203, 106, 107, "scale B", tuple(scaled)),
        Usage(204, 107, 101, "scale C", tuple(scaled)),
    )
    write_source(replace(source, products=source.products + (
        Product(105, "rotation", "assembly"), Product(106, "scale A", "assembly"),
        Product(107, "scale B", "assembly")), usages=usages), sys.argv[1])


if __name__ == "__main__":
    main()
