"""Owned reader for a bounded, static ASCII USD BRep export subset.

This reader does not compose USD stages. Unsupported syntax is an error rather
than a request to ignore parts of the source scene.
"""
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
import math
import struct

MAX_BYTES = 16 * 1024 * 1024
MAX_TOKENS = 1_000_000
MAX_COMPONENTS = 500_000
MAX_PRIMS = 4096
MAX_DEPTH = 64


class SourceError(ValueError):
    """Malformed source bytes or typed values."""


class UnsupportedSource(SourceError):
    """Source syntax or scene semantics outside the declared subset."""


class SourceBudgetError(SourceError):
    """Source exceeds a declared acquisition or parsing budget."""


@dataclass(frozen=True)
class Attribute:
    name: str
    type_name: str
    value: tuple


@dataclass(frozen=True)
class Prim:
    path: str
    type_name: str
    attributes: tuple
    metadata: tuple


@dataclass(frozen=True)
class Document:
    prims: tuple
    metadata: tuple


@dataclass(frozen=True)
class _Token:
    kind: str
    value: str
    offset: int


def _first(c):
    return c == '_' or 'a' <= c <= 'z' or 'A' <= c <= 'Z'


def _later(c):
    return _first(c) or '0' <= c <= '9'


def _identifier(value, namespaced=False):
    parts = value.split(':') if namespaced else (value,)
    return all(part and _first(part[0]) and all(_later(c) for c in part[1:])
               for part in parts)


class _Lexer:
    def __init__(self, text, start):
        self.text, self.i, self.count = text, start, 0

    def next(self):
        text, size = self.text, len(self.text)
        while self.i < size:
            c = text[self.i]
            if c in ' \t\r\n':
                self.i += 1
            elif c == '#':
                end = text.find('\n', self.i)
                self.i = size if end < 0 else end + 1
            else:
                break
        start = self.i
        if start == size:
            return _Token('eof', '', start)
        if self.count >= MAX_TOKENS:
            raise SourceBudgetError(f'token budget at character {start}')
        self.count += 1
        c = text[self.i]
        if c in '{}()[]=,.':
            if c == '.' and self.i + 1 < size and text[self.i + 1].isdigit():
                return self.number()
            self.i += 1
            return _Token(c, c, start)
        if c == '"':
            self.i += 1
            out = []
            escapes = {'"': '"', '\\': '\\', 'n': '\n', 'r': '\r', 't': '\t'}
            while self.i < size:
                c = text[self.i]
                self.i += 1
                if c == '"':
                    return _Token('string', ''.join(out), start)
                if c == '\\':
                    if self.i >= size or text[self.i] not in escapes:
                        raise SourceError(f'unsupported or incomplete string escape at character {self.i}')
                    c = escapes[text[self.i]]
                    self.i += 1
                elif ord(c) < 32:
                    raise SourceError(f'unescaped control character in string at {self.i - 1}')
                out.append(c)
            raise SourceError(f'unterminated string at character {start}')
        if _first(c):
            self.i += 1
            while self.i < size and (_later(text[self.i]) or text[self.i] == ':'):
                self.i += 1
            return _Token('id', text[start:self.i], start)
        if c in '+-' or '0' <= c <= '9':
            return self.number()
        raise UnsupportedSource(f'unsupported character {c!r} at character {start}')

    def number(self):
        text, size, start = self.text, len(self.text), self.i
        if text[self.i] in '+-':
            self.i += 1
        digits = 0
        while self.i < size and '0' <= text[self.i] <= '9':
            self.i += 1
            digits += 1
        if self.i < size and text[self.i] == '.':
            self.i += 1
            while self.i < size and '0' <= text[self.i] <= '9':
                self.i += 1
                digits += 1
        if not digits:
            raise SourceError(f'invalid numeric literal at character {start}')
        if self.i < size and text[self.i] in 'eE':
            self.i += 1
            if self.i < size and text[self.i] in '+-':
                self.i += 1
            exponent = self.i
            while self.i < size and '0' <= text[self.i] <= '9':
                self.i += 1
            if exponent == self.i:
                raise SourceError(f'incomplete exponent at character {start}')
        return _Token('number', text[start:self.i], start)


def _binary32(text):
    """Round the decimal directly to binary32, including midpoint neighbors.

    float→pack alone can double-round a decimal very near a binary32 midpoint.
    Decimal comparisons correct that one-neighbor ambiguity without constructing
    huge powers for an arbitrarily long source exponent.
    """
    try:
        exact = Decimal(text)
        value = abs(float(text))
        if not math.isfinite(value):
            raise SourceError('nonfinite float value')
        try:
            bits = struct.unpack('<I', struct.pack('<f', value))[0]
        except OverflowError:
            bits = 0x7f7fffff
        candidate = struct.unpack('<f', struct.pack('<I', bits))[0]
        absolute = exact.copy_abs()
        if bits:
            previous = struct.unpack('<f', struct.pack('<I', bits - 1))[0]
            lower = Decimal.from_float((previous + candidate) / 2)
            if absolute < lower or (absolute == lower and bits & 1):
                bits -= 1
        candidate = struct.unpack('<f', struct.pack('<I', bits))[0]
        if bits == 0x7f7fffff:
            previous = struct.unpack('<f', struct.pack('<I', bits - 1))[0]
            upper_value = candidate + (candidate - previous) / 2
        else:
            following = struct.unpack('<f', struct.pack('<I', bits + 1))[0]
            upper_value = (candidate + following) / 2
        upper = Decimal.from_float(upper_value)
        if absolute > upper or (absolute == upper and bits & 1):
            bits += 1
        if bits >= 0x7f800000:
            raise SourceError('float value overflows binary32')
        result = struct.unpack('<f', struct.pack('<I', bits))[0]
        return math.copysign(result, -1. if exact.is_signed() else 1.)
    except (ValueError, InvalidOperation, OverflowError) as error:
        if isinstance(error, SourceError):
            raise
        raise SourceError('invalid float value') from error


_SCALARS = {'uint', 'int', 'double', 'float', 'bool', 'string', 'token'}
_VECTORS = {f'{base}{width}': (base, width)
            for base in ('double', 'float', 'int') for width in (2, 3, 4)}
_VECTORS.update({'point3d': ('double', 3), 'vector3d': ('double', 3)})


class _Parser:
    def __init__(self, text, start):
        self.lex = _Lexer(text, start)
        self.token = self.lex.next()
        self.depth = self.components = 0
        self.prims, self.paths = [], set()

    def error(self, message, kind=SourceError):
        raise kind(f'{message} at character {self.token.offset}')

    def take(self, kind, value=None):
        token = self.token
        if token.kind != kind or (value is not None and token.value != value):
            self.error(f'expected {value or kind}, got {token.value!r}')
        self.token = self.lex.next()
        return token.value

    def enter(self, kind):
        if self.depth >= MAX_DEPTH:
            self.error('nesting budget', SourceBudgetError)
        self.take(kind)
        self.depth += 1

    def leave(self, kind):
        self.take(kind)
        self.depth -= 1

    def scalar(self, type_name, component=False):
        if component:
            if self.components >= MAX_COMPONENTS:
                self.error('array component budget', SourceBudgetError)
            self.components += 1
        if type_name in ('string', 'token'):
            return self.take('string')
        if type_name == 'bool':
            if self.token.kind != 'id' or self.token.value not in ('true', 'false'):
                self.error('expected true or false')
            return self.take('id') == 'true'
        text = self.take('number')
        if type_name in ('uint', 'int'):
            if any(c in text for c in '.eE'):
                self.error('integer attribute requires an integer literal')
            digits = text.lstrip('+-').lstrip('0') or '0'
            if len(digits) > 10:
                self.error('integer outside declared 32-bit range')
            value = int(('-' if text.startswith('-') else '') + digits)
            lower, upper = (0, 2**32 - 1) if type_name == 'uint' else (-2**31, 2**31 - 1)
            if not lower <= value <= upper:
                self.error('integer outside declared 32-bit range')
            return value
        if type_name == 'float':
            return _binary32(text)
        try:
            value = float(text)
        except (ValueError, OverflowError) as error:
            raise SourceError('invalid double value') from error
        if not math.isfinite(value):
            self.error('nonfinite double value')
        return value

    def array(self, type_name):
        self.enter('[')
        values = []
        while self.token.kind != ']':
            if type_name in _VECTORS:
                scalar, width = _VECTORS[type_name]
                self.enter('(')
                row = []
                for index in range(width):
                    if index:
                        self.take(',')
                    row.append(self.scalar(scalar, component=True))
                self.leave(')')
                value = tuple(row)
            else:
                value = self.scalar(type_name, component=True)
            values.append(value)
            if self.token.kind == ']':
                break
            self.take(',')
        self.leave(']')
        return tuple(values)

    def metadata(self, stage=False):
        self.enter('(')
        values, seen = [], set()
        while self.token.kind != ')':
            key = self.take('id')
            if key == 'prepend' and not stage:
                key = self.take('id')
                if key != 'apiSchemas':
                    self.error('only apiSchemas supports prepend', UnsupportedSource)
            if key in seen:
                self.error('duplicate metadata key')
            seen.add(key)
            allowed = ('defaultPrim', 'upAxis', 'metersPerUnit') if stage else ('apiSchemas', 'customData')
            if key not in allowed:
                self.error(f'unsupported metadata {key!r}', UnsupportedSource)
            self.take('=')
            if key == 'apiSchemas':
                value = self.array('token')
            elif key == 'customData':
                self.enter('{')
                entries = []
                while self.token.kind != '}':
                    if self.take('id') != 'string':
                        self.error('customData supports string source only', UnsupportedSource)
                    if self.take('id') != 'source':
                        self.error('customData supports source only', UnsupportedSource)
                    if entries:
                        self.error('duplicate customData source')
                    self.take('=')
                    entries.append(('source', self.scalar('string')))
                self.leave('}')
                value = tuple(entries)
            elif key == 'metersPerUnit':
                value = self.scalar('double')
                if value <= 0:
                    self.error('metersPerUnit must be positive')
            else:
                value = self.scalar('token' if key == 'upAxis' else 'string')
                if key == 'upAxis' and value not in ('Y', 'Z'):
                    self.error('upAxis must be Y or Z')
                if key == 'defaultPrim' and not _identifier(value):
                    self.error('invalid defaultPrim identifier')
            values.append((key, value))
        self.leave(')')
        return tuple(values)

    def prim(self, parent=''):
        if len(self.prims) >= MAX_PRIMS:
            self.error('primitive budget', SourceBudgetError)
        if self.token.kind != 'id' or self.token.value != 'def':
            self.error('only def primitives supported', UnsupportedSource)
        self.take('id', 'def')
        type_name = self.take('id')
        if type_name not in ('Xform', 'Scope', 'BrepArray'):
            self.error(f'unsupported primitive type {type_name!r}', UnsupportedSource)
        name = self.take('string')
        if not _identifier(name):
            self.error('invalid primitive name')
        path = parent + '/' + name
        if path in self.paths:
            self.error('duplicate primitive path')
        self.paths.add(path)
        ordinal = len(self.prims)
        self.prims.append(None)
        metadata = self.metadata() if self.token.kind == '(' else ()
        self.enter('{')
        attributes, seen = [], set()
        while self.token.kind != '}':
            if self.token.kind == 'eof':
                self.error('unterminated primitive')
            if type_name != 'BrepArray':
                if self.token.kind != 'id' or self.token.value != 'def':
                    self.error('identity wrappers cannot author properties', UnsupportedSource)
                self.prim(path)
                continue
            if self.token.kind != 'id' or self.token.value != 'uniform':
                self.error('only uniform array properties supported', UnsupportedSource)
            self.take('id', 'uniform')
            attribute_type = self.take('id')
            if attribute_type not in _SCALARS and attribute_type not in _VECTORS:
                self.error(f'unsupported attribute type {attribute_type!r}', UnsupportedSource)
            self.take('[')
            self.take(']')
            name = self.take('id')
            if not _identifier(name, namespaced=True):
                self.error('invalid property name')
            if name == 'xformOpOrder' or name.startswith('xformOp:') or self.token.kind == '.':
                self.error('authored transforms and time samples unsupported', UnsupportedSource)
            if name in seen:
                self.error('duplicate attribute')
            seen.add(name)
            self.take('=')
            attributes.append(Attribute(name, attribute_type, self.array(attribute_type)))
        self.leave('}')
        self.prims[ordinal] = Prim(path, type_name, tuple(attributes), metadata)

    def document(self):
        metadata = self.metadata(stage=True) if self.token.kind == '(' else ()
        while self.token.kind != 'eof':
            self.prim()
        return Document(tuple(self.prims), metadata)


def parse(data: bytes) -> Document:
    """Read exactly this immutable byte snapshot; never load dependencies/assets."""
    if type(data) is not bytes:
        raise TypeError('source must be immutable bytes')
    if len(data) > MAX_BYTES:
        raise SourceBudgetError('source byte budget')
    if data.startswith(b'PXR-USDC'):
        raise UnsupportedSource('binary USD is unsupported')
    try:
        text = data.decode('utf-8', 'strict')
    except UnicodeDecodeError as error:
        raise SourceError('source is not valid UTF-8') from error
    end = text.find('\n')
    header = text if end < 0 else text[:end]
    if header not in ('#usda 1.0', '#usda 1.0\r'):
        raise SourceError('expected exact #usda 1.0 header')
    return _Parser(text, len(text) if end < 0 else end + 1).document()
