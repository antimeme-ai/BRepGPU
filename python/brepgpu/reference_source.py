"""Owned immutable BGOCC001 metadata for the isolated original-source export.

The manifest carries source identity, styles, per-occurrence styles and the
fresh occurrence graph. Geometry remains in the unmeshed BinTools V4 files,
never in this module.

Format version 2 (owned reference source v5): every shell of a solid (outer
first, then BREP_WITH_VOIDS voids; a face's orientation is its use in the
solid, same_sense composed with its shell's), the translated faces of each
original face as storage indices (one under translation-only; the faces OCCT's
processing made of it under the default read), each definition's translated
face count, and the OCCSTYLE table. Version 1 is refused by name.
"""
from dataclasses import dataclass
import math
from pathlib import Path
import struct
from types import MappingProxyType

from .imported_source import Product, Usage, Style, OccurrenceStyle
from .imported_assembly import assemble, rigid_transform

_TAGS = (b'META____', b'PRODUCTS', b'DEFS____', b'USAGES__', b'STYLES__', b'FACE_STY', b'OCCSTYLE')
FORMAT_VERSION = 2
_PHASES = ('read_step', 'transfer_step', 'collect_source_graph', 'write_reference_brep')


@dataclass(frozen=True)
class ReferenceFace:
    id: int
    carrier_id: int
    # BinTools storage indices of the translated faces this original face
    # became in its definition's .brep: one under translation-only, one or more
    # under OCCT's default read (FixShape may split a face).
    serialized_storage_indices: tuple
    # Use in the solid: same_sense composed with the shell's orientation.
    orientation: int
    style_id: int = 0
    # 0 the outer shell, k the k-th BREP_WITH_VOIDS void.
    shell: int = 0

    @property
    def serialized_storage_index(self):
        """The one translated face of a face that the read did not split."""
        if len(self.serialized_storage_indices) != 1:
            raise ValueError(f'original face #{self.id} is {len(self.serialized_storage_indices)} translated faces; '
                             'no single storage index')
        return self.serialized_storage_indices[0]


@dataclass(frozen=True)
class ReferenceDefinition:
    id: int
    product_id: int
    filename: str
    faces: tuple
    # Faces in the exported .brep: the original face count under
    # translation-only, what OCCT's processing made of them under the default read.
    translated_face_count: int = 0

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
    occurrence_styles: tuple = ()

    def __post_init__(self):
        def immutable(value):
            if type(value) in (int, float, bool, str):
                return
            if type(value) is tuple:
                for item in value:
                    immutable(item)
                return
            if type(value) in (Product, Usage, Style, ReferenceDefinition, ReferenceFace, OccurrenceStyle):
                for field in value.__dataclass_fields__:
                    immutable(getattr(value, field))
                return
            raise TypeError('reference source requires owned immutable values')
        for field in ('products', 'definitions', 'usages', 'styles', 'occurrence_styles'):
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
        indices, shells = set(), set()
        if not definition.faces:
            raise ValueError('reference definition has no original faces')
        for face in definition.faces:
            _positive(face.id, 'original face ID')
            _positive(face.carrier_id, 'original carrier ID')
            if not face.serialized_storage_indices:
                raise ValueError(f'original face #{face.id} has no translated face')
            for index in face.serialized_storage_indices:
                _positive(index, 'serialized face index')
                if index in indices:
                    raise ValueError('duplicate reference face or storage index')
                indices.add(index)
            if face.id in face_ids:
                raise ValueError('duplicate reference face or storage index')
            if face.orientation not in (-1, 1):
                raise ValueError('invalid original face orientation')
            face_ids.add(face.id)
            shells.add(face.shell)
        # Shell ordinals run from 0 without gaps (0 the outer shell).
        if shells != set(range(len(shells))):
            raise ValueError(f'reference definition #{definition.id} shell ordinals have a gap')
        if definition.translated_face_count != len(indices):
            raise ValueError(f'reference definition #{definition.id} translated face count differs from its face map')
    for usage in source.usages:
        rigid_transform(usage.transform, f'reference usage #{usage.id}')
    # Existing owned graph mechanism rejects duplicate IDs, dangling products,
    # cycles, unreachable definitions and invalid placements; no second walker.
    assembly = assemble(source)
    reachable = {o.product_id for o in assembly.occurrences} | {g.product_id for g in assembly.groups}
    if reachable != {p.id for p in source.products}:
        raise ValueError('reference graph contains unreachable products')
    # Occurrence styles, under the STEP source reader's rules
    # (imported_source.validate_source): a known style, a usage path connected
    # from the root, a face on that occurrence's definition, one row per key.
    products, usages = {p.id: p for p in source.products}, {u.id: u for u in source.usages}
    face_definitions = {f.id: d.id for d in source.definitions for f in d.faces}
    keys = set()
    for row in source.occurrence_styles:
        if row.style_id not in style_ids:
            raise ValueError('unresolved reference occurrence style')
        if not row.usage_ids:
            raise ValueError('reference occurrence style path is empty')
        parent = source.root_product_id
        for usage_id in row.usage_ids:
            usage = usages.get(usage_id)
            if usage is None or usage.parent_product_id != parent:
                raise ValueError(f'reference occurrence style path {row.usage_ids} is not connected from the root')
            parent = usage.child_product_id
        if row.face_id:
            definition = products[parent].definition_id
            if not definition or face_definitions.get(row.face_id) != definition:
                raise ValueError(f'reference occurrence style face {row.face_id} is not on its occurrence')
        if (row.usage_ids, row.face_id) in keys:
            raise ValueError('duplicate reference occurrence style')
        keys.add((row.usage_ids, row.face_id))
    return source


def load_reference_source(path):
    """Read a complete source.bgocct and its strictly local original BRep files."""
    path = Path(path)
    r = _Reader(path.read_bytes())
    if bytes(r.take(8)) != b'BGOCC001':
        raise ValueError('unsupported reference source format')
    version = r.scalar('I')
    if version != FORMAT_VERSION:
        raise ValueError(f'unsupported reference source version {version}: this reader reads BGOCC001 version '
                         f'{FORMAT_VERSION} (voids, split faces, occurrence styles); re-export the source')
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
    products, definitions, usages, styles, face_styles, occurrence_styles = [], [], [], [], {}, []
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
    for row in sections[b'OCCSTYLE']:
        style, face = row.values('Q', 2)
        occurrence_styles.append(OccurrenceStyle(style, face, row.values('Q', row.scalar('Q'))))
        row.end()
    for row in sections[b'DEFS____']:
        id, product = row.values('Q', 2)
        filename = row.string()
        translated, count = row.values('Q', 2)
        if count > (len(row.data)-row.pos)//40:
            raise ValueError('oversized reference face count')
        faces = []
        for _ in range(count):
            fid, carrier = row.values('Q', 2)
            orientation, shell = row.scalar('i'), row.scalar('I')
            indices = row.values('Q', row.scalar('Q'))
            if fid not in face_styles:
                raise ValueError('missing reference face style row')
            faces.append(ReferenceFace(fid, carrier, indices, orientation, face_styles.pop(fid), shell))
        row.end()
        definitions.append(ReferenceDefinition(id, product, filename, tuple(faces), translated))
    if face_styles:
        raise ValueError('orphan reference face style row')
    source = ReferenceSource(digest, units, uncertainty, root, revision,
        tuple(products), tuple(definitions), tuple(usages), tuple(styles), phases, tuple(occurrence_styles))
    return _validate(source, path.parent)
