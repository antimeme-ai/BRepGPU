"""Owned occurrence identity and rigid placement over the imported source graph.

Matrices are row-major 3x4 child-to-parent maps applied to column vectors.
Stable IDs follow depth-first traversal, with each parent's source usages sorted
by their original entity ID. A full usage path identifies a placed occurrence.
"""
from dataclasses import dataclass
import math

from .imported_source import rigid_placement_status


IDENTITY = (1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0.)


def _source_id(value, description):
    if type(value) is not int or not 0 < value < (1 << 63):
        raise ValueError(f'{description}: expected positive original source ID')
    return value


def _finite_tuple(values, size, description):
    try:
        result = tuple(float(x) for x in values)
    except (TypeError, ValueError, OverflowError) as error:
        raise ValueError(f'{description}: expected finite scalar values') from error
    if len(result) != size or not all(math.isfinite(x) for x in result):
        raise ValueError(f'{description}: expected {size} finite scalar values')
    return result


def rigid_transform(values, description='placement'):
    """Copy and validate a proper rigid 3x4 source map without changing its bits.

    The one placement rule (imported_source.rigid_placement_status, the
    mirror of native/rigid_placement.hpp) applies to every usage and to every
    composed occurrence, as the display checks composed world matrices.
    Consumers apply the matrix as given; the rule only admits the rigid class.
    """
    result = _finite_tuple(values, 12, description)
    status = rigid_placement_status(result)
    if status == 'nonorthonormal':
        raise ValueError(f'{description}: rotation is not orthonormal')
    if status != 'rigid':
        raise ValueError(f'{description}: placement must have a proper rotation')
    return result


def compose(parent, child):
    """Return parent(child(point)); reject arithmetic overflow in nested maps."""
    result = []
    for i in range(3):
        for j in range(3):
            result.append((parent[4*i]*child[j] + parent[4*i+1]*child[4+j])
                          + parent[4*i+2]*child[8+j])
        result.append(((parent[4*i]*child[3] + parent[4*i+1]*child[7])
                       + parent[4*i+2]*child[11]) + parent[4*i+3])
    return rigid_transform(result, 'composed placement')


def transform_point(transform, point):
    point = _finite_tuple(point, 3, 'point')
    output = tuple(((transform[4*i]*point[0]+transform[4*i+1]*point[1])
                    + transform[4*i+2]*point[2])+transform[4*i+3] for i in range(3))
    if not all(math.isfinite(x) for x in output):
        raise ValueError('point placement overflow')
    return output


@dataclass(frozen=True)
class Occurrence:
    id: int
    path: tuple
    product_id: int
    definition_id: int
    name: str
    reference: str
    transform: tuple


@dataclass(frozen=True)
class Group:
    path: tuple
    product_id: int
    name: str
    reference: str
    transform: tuple
    occurrence_ids: tuple


@dataclass(frozen=True)
class Assembly:
    root_product_id: int
    occurrences: tuple
    groups: tuple

    def occurrence(self, id):
        if type(id) is not int or not 1 <= id <= len(self.occurrences):
            raise ValueError('unknown occurrence ID')
        return self.occurrences[id-1]


def assemble(source, *, max_occurrences=1_000_000):
    """Expand every root leaf within an explicit allocation budget; never truncate."""
    if type(max_occurrences) is not int or max_occurrences < 1:
        raise ValueError('max_occurrences must be a positive integer')
    if source is None:
        raise ValueError('assembly needs an imported source')
    try:
        products, definitions, usages = source.products, source.definitions, source.usages
        root = _source_id(source.root_product_id, 'assembly root')
    except AttributeError as error:
        raise ValueError('assembly needs a source product/definition/usage graph') from error
    product_by_id, definition_by_id, children = {}, {}, {}
    for product in products:
        id = _source_id(product.id, 'product')
        if id in product_by_id:
            raise ValueError(f'duplicate product #{id}')
        if not isinstance(product.name, str) or not isinstance(product.reference, str):
            raise ValueError(f'product #{id}: names must be text')
        product_by_id[id] = product
        children[id] = []
    if root not in product_by_id:
        raise ValueError('root product does not resolve')
    for definition in definitions:
        id = _source_id(definition.id, 'solid definition')
        if id in definition_by_id:
            raise ValueError(f'duplicate definition #{id}')
        product = product_by_id.get(definition.product_id)
        if product is None or product.definition_id != id:
            raise ValueError(f'definition #{id}: product correspondence does not resolve')
        definition_by_id[id] = definition
    seen_usages = set()
    for usage in usages:
        id = _source_id(usage.id, 'usage')
        if id in seen_usages:
            raise ValueError(f'duplicate usage #{id}')
        seen_usages.add(id)
        if usage.parent_product_id not in product_by_id or usage.child_product_id not in product_by_id:
            raise ValueError(f'usage #{id}: product reference does not resolve')
        matrix = rigid_transform(usage.transform, f'usage #{id}')
        children[usage.parent_product_id].append((id, usage.child_product_id, matrix))
    for product in products:
        if product.definition_id:
            definition = definition_by_id.get(product.definition_id)
            if definition is None or definition.product_id != product.id:
                raise ValueError(f'product #{product.id}: solid reference does not resolve')
            if children[product.id]:
                raise ValueError(f'product #{product.id}: both solid and assembly children')
        elif not children[product.id]:
            raise ValueError(f'product #{product.id}: assembly has no children or solid')
        children[product.id].sort()
    # Check the entire encoded graph, including data outside the selected root.
    done, active, expansion_counts = set(), set(), {}

    def check(id):
        if id in active:
            raise ValueError(f'cyclic assembly graph at product #{id}')
        if id in done:
            return
        active.add(id)
        for _, child, _ in children[id]:
            check(child)
        active.remove(id)
        done.add(id)
        count = 1 if product_by_id[id].definition_id else 0
        for _, child, _ in children[id]:
            count += expansion_counts[child]
            if count > max_occurrences:
                # Avoid exponential-size integers while checking the source DAG.
                count = max_occurrences+1
                break
        expansion_counts[id] = count

    try:
        for id in product_by_id:
            check(id)
    except RecursionError as error:
        raise ValueError('assembly nesting exceeds supported Python recursion') from error
    if expansion_counts[root] > max_occurrences:
        raise ValueError('complete assembly exceeds max_occurrences; no partial expansion')
    occurrences, groups = [], []

    def expand(id, path, matrix):
        product = product_by_id[id]
        if product.definition_id:
            occurrences.append(Occurrence(len(occurrences)+1, path, id,
                               product.definition_id, product.name, product.reference, matrix))
            return
        group_slot, first = len(groups), len(occurrences)
        groups.append(None)
        for usage_id, child, local in children[id]:
            expand(child, path+(usage_id,), compose(matrix, local))
        groups[group_slot] = Group(path, id, product.name, product.reference, matrix,
                                  tuple(range(first+1, len(occurrences)+1)))

    try:
        expand(root, (), IDENTITY)
    except RecursionError as error:
        raise ValueError('assembly expansion exceeds supported Python recursion') from error
    return Assembly(root, tuple(occurrences), tuple(groups))


@dataclass(frozen=True)
class Explosion:
    """Authored world-space translations, indexed by stable occurrence ID."""
    offsets: tuple


def explosion(assembly, offsets):
    offsets = tuple(_finite_tuple(v, 3, 'explosion offset') for v in offsets)
    if len(offsets) != len(assembly.occurrences):
        raise ValueError('explosion must name every occurrence exactly once in ID order')
    return Explosion(offsets)


def placed_transform(occurrence, offset, alpha):
    if isinstance(alpha, bool):
        raise ValueError('explosion alpha must be finite in [0,1]')
    try:
        alpha = float(alpha)
    except (TypeError, ValueError, OverflowError) as error:
        raise ValueError('explosion alpha must be finite in [0,1]') from error
    if not math.isfinite(alpha) or not 0. <= alpha <= 1.:
        raise ValueError('explosion alpha must be finite in [0,1]')
    if alpha == 0.:
        return occurrence.transform
    offset = _finite_tuple(offset, 3, 'explosion offset')
    matrix = list(occurrence.transform)
    for i in range(3):
        matrix[4*i+3] += alpha*offset[i]
    if not all(math.isfinite(x) for x in matrix):
        raise ValueError('exploded placement overflow')
    return tuple(matrix)
