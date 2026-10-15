"""Owned immutable BGOCC001 metadata for the isolated original-source export.

The manifest carries source identity, styles and the fresh occurrence graph.
Geometry remains in the unmeshed BinTools V4 files, never in this module.
"""
from dataclasses import dataclass
import math
from pathlib import Path
import struct
from types import MappingProxyType

from .imported_source import Product, Usage, Style
from .imported_assembly import assemble, rigid_transform

_TAGS = (b'META____', b'PRODUCTS', b'DEFS____', b'USAGES__', b'STYLES__', b'FACE_STY')
_PHASES = ('read_step', 'transfer_step', 'collect_source_graph', 'write_reference_brep')


@dataclass(frozen=True)
class ReferenceFace:
    id: int
    carrier_id: int
    serialized_storage_index: int
    orientation: int
    style_id: int = 0


@dataclass(frozen=True)
class ReferenceDefinition:
    id: int
    product_id: int
    filename: str
    faces: tuple

    @property
    def face_ids(self):
        return tuple(f.id for f in self.faces)


@dataclass(frozen=True)
class ReferenceSource:
    source_sha256: str
    units_mm: float
    uncertainty_mm: float
    root_product_id: int
    adapter_revision: str
    products: tuple
    definitions: tuple
    usages: tuple
    styles: tuple
    phase_seconds: object

    def __post_init__(self):
        def immutable(value):
            if type(value) in (int, float, bool, str):
                return
            if type(value) is tuple:
                for item in value:
                    immutable(item)
                return
            if type(value) in (Product, Usage, Style, ReferenceDefinition, ReferenceFace):
                for field in value.__dataclass_fields__:
                    immutable(getattr(value, field))
                return
            raise TypeError('reference source requires owned immutable values')
        for field in ('products', 'definitions', 'usages', 'styles'):
            immutable(getattr(self, field))
        phases = dict(self.phase_seconds)
        immutable(tuple(phases.items()))
        object.__setattr__(self, 'phase_seconds', MappingProxyType(phases))

    @property
    def source_uncertainty_mm(self):
        return self.uncertainty_mm

    @property
    def faces(self):
        return tuple(f for d in self.definitions for f in d.faces)

    def product(self, id):
        for row in self.products:
            if row.id == id:
                return row
        raise ValueError(f'unknown original product #{id}')

    def definition(self, id):
        for row in self.definitions:
            if row.id == id:
                return row
        raise ValueError(f'unknown original definition #{id}')


class _Reader:
    def __init__(self, data):
        self.data, self.pos = memoryview(data), 0

    def take(self, n):
        if n < 0 or n > len(self.data)-self.pos:
            raise ValueError('truncated reference manifest')
        result = self.data[self.pos:self.pos+n]
        self.pos += n
        return result

    def scalar(self, fmt):
        return struct.unpack('<'+fmt, self.take(struct.calcsize('<'+fmt)))[0]

    def values(self, fmt, n):
        size = struct.calcsize('<'+fmt)
        if n > (len(self.data)-self.pos)//size:
            raise ValueError('oversized reference vector')
        return tuple(struct.unpack('<'+str(n)+fmt, self.take(size*n))) if n else ()

    def string(self):
        try:
            return bytes(self.take(self.scalar('I'))).decode('utf-8')
        except UnicodeDecodeError as error:
            raise ValueError('invalid reference UTF-8') from error

    def zero(self):
        if self.scalar('I'):
            raise ValueError('nonzero reference reserved field')

    def boolean(self):
        value = self.scalar('I')
        if value not in (0, 1):
            raise ValueError('invalid reference boolean')
        return bool(value)

    def row(self):
        return _Reader(self.take(self.scalar('Q')))

    def end(self):
        if self.pos != len(self.data):
            raise ValueError('trailing reference bytes')


def _positive(value, description):
    if not 0 < value < 2**64:
        raise ValueError(f'invalid {description}')


def _validate(source, directory):
    if source.units_mm != 1.0:
        raise ValueError('reference source requires millimetre units')
    if not math.isfinite(source.uncertainty_mm) or source.uncertainty_mm < 0:
        raise ValueError('invalid reference context uncertainty')
    if any(not math.isfinite(x) or x < 0 for x in source.phase_seconds.values()):
        raise ValueError('invalid reference phase duration')
    style_ids = set()
    for style in source.styles:
        _positive(style.id, 'source style ID')
        if style.id in style_ids:
            raise ValueError('duplicate reference style')
        style_ids.add(style.id)
        if (not all(math.isfinite(x) and 0 <= x <= 1 for x in style.rgba)
                or not math.isfinite(style.density) or style.density < 0):
            raise ValueError('invalid reference source style')
        for id in style.source_ids:
            _positive(id, 'style original source ID')
    for row in (*source.products, *source.usages, *source.faces):
        if row.style_id and row.style_id not in style_ids:
            raise ValueError('unresolved reference style')
    face_ids, filenames = set(), set()
    for definition in source.definitions:
        if (definition.filename != f'definition-{definition.id}.brep'
                or definition.filename in filenames):
            raise ValueError('invalid or duplicate reference filename')
        filenames.add(definition.filename)
        path = directory/definition.filename
        if path.resolve().parent != directory.resolve():
            raise ValueError('reference filename escapes source directory')
        if not path.is_file() or not 0 < path.stat().st_size <= 256*1024*1024:
            raise ValueError('missing or oversized reference BRep')
        indices = set()
        if not definition.faces:
            raise ValueError('reference definition has no original faces')
        for face in definition.faces:
            _positive(face.id, 'original face ID')
            _positive(face.carrier_id, 'original carrier ID')
            _positive(face.serialized_storage_index, 'serialized face index')
            if face.id in face_ids or face.serialized_storage_index in indices:
                raise ValueError('duplicate reference face or storage index')
            if face.orientation not in (-1, 1):
                raise ValueError('invalid original face orientation')
            face_ids.add(face.id)
            indices.add(face.serialized_storage_index)
    for usage in source.usages:
        rigid_transform(usage.transform, f'reference usage #{usage.id}')
    # Existing owned graph mechanism rejects duplicate IDs, dangling products,
    # cycles, unreachable definitions and invalid placements; no second walker.
    assembly = assemble(source)
    reachable = {o.product_id for o in assembly.occurrences} | {g.product_id for g in assembly.groups}
    if reachable != {p.id for p in source.products}:
        raise ValueError('reference graph contains unreachable products')
    return source


def load_reference_source(path):
    """Read a complete source.bgocct and its strictly local original BRep files."""
    path = Path(path)
    r = _Reader(path.read_bytes())
    if bytes(r.take(8)) != b'BGOCC001' or r.scalar('I') != 1:
        raise ValueError('unsupported reference source format')
    r.zero()
    digest = bytes(r.take(32)).hex()
    units, uncertainty = r.values('d', 2)
    root = r.scalar('Q')
    if r.scalar('I') != len(_TAGS):
        raise ValueError('wrong reference section count')
    r.zero()
    if r.scalar('Q') != len(r.data):
        raise ValueError('wrong reference total size')
    sections = {}
    for _ in _TAGS:
        tag = bytes(r.take(8))
        if tag not in _TAGS or tag in sections:
            raise ValueError('duplicate or unknown reference section')
        count, size = r.values('Q', 2)
        s = _Reader(r.take(size))
        if count > size//8:
            raise ValueError('oversized reference row count')
        rows = tuple(s.row() for _ in range(count))
        s.end()
        sections[tag] = rows
    r.end()
    if len(sections[b'META____']) != 1:
        raise ValueError('reference needs one metadata row')
    meta = sections[b'META____'][0]
    revision = meta.string()
    if meta.scalar('I') != 4:
        raise ValueError('reference requires explicit BinTools V4')
    meta.zero()
    phases = MappingProxyType(dict(zip(_PHASES, meta.values('d', 4))))
    meta.end()
    products, definitions, usages, styles, face_styles = [], [], [], [], {}
    for row in sections[b'PRODUCTS']:
        id, definition, style = row.values('Q', 3)
        products.append(Product(id, row.string(), row.string(), definition, style))
        row.end()
    for row in sections[b'USAGES__']:
        id, parent, child, style = row.values('Q', 4)
        usages.append(Usage(id, parent, child, row.string(), row.values('d', 12), style))
        row.end()
    for row in sections[b'STYLES__']:
        id = row.scalar('Q')
        sources = row.values('Q', row.scalar('Q'))
        rgba = row.values('d', 4)
        color, density_set = row.boolean(), row.boolean()
        material, unit, density = row.string(), row.string(), row.scalar('d')
        styles.append(Style(id, sources, rgba, color, material, unit, density, density_set))
        row.end()
    for row in sections[b'FACE_STY']:
        id, style = row.values('Q', 2)
        if id in face_styles:
            raise ValueError('duplicate reference face style row')
        face_styles[id] = style
        row.end()
    for row in sections[b'DEFS____']:
        id, product = row.values('Q', 2)
        filename, count = row.string(), row.scalar('Q')
        if count > (len(row.data)-row.pos)//32:
            raise ValueError('oversized reference face count')
        faces = []
        for _ in range(count):
            fid, carrier, index = row.values('Q', 3)
            orientation = row.scalar('i')
            row.zero()
            if fid not in face_styles:
                raise ValueError('missing reference face style row')
            faces.append(ReferenceFace(fid, carrier, index, orientation, face_styles.pop(fid)))
        row.end()
        definitions.append(ReferenceDefinition(id, product, filename, tuple(faces)))
    if face_styles:
        raise ValueError('orphan reference face style row')
    source = ReferenceSource(digest, units, uncertainty, root, revision,
        tuple(products), tuple(definitions), tuple(usages), tuple(styles), phases)
    return _validate(source, path.parent)
