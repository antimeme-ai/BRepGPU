"""Independent standard-zlib reader for actual owned PNG output."""
import binascii
from pathlib import Path
import struct
import subprocess
import sys
import zlib

path = Path(sys.argv[2])
subprocess.run([sys.argv[1],str(path)], check=True)
b = path.read_bytes()
assert b[:8] == b'\x89PNG\r\n\x1a\n'
at = 8
compressed = bytearray()
seen = []
while at < len(b):
    n, = struct.unpack_from('>I',b,at)
    kind = b[at+4:at+8]
    data = b[at+8:at+8+n]
    crc, = struct.unpack_from('>I',b,at+8+n)
    assert crc == binascii.crc32(kind+data)
    seen.append(kind)
    if kind == b'IHDR':
        assert struct.unpack('>IIBBBBB',data) == (2,2,8,2,0,0,0)
    elif kind == b'IDAT':
        compressed.extend(data)
    at += 12+n
assert at == len(b) and seen == [b'IHDR',b'IDAT',b'IEND']
assert zlib.decompress(compressed) == bytes((0,255,0,0,0,255,0,0,0,0,255,255,255,255))
print('PASS\tindependent PNG CRC/zlib/scanline pixel answers')

wide = Path(str(path)+'.wide.png').read_bytes()
at=8
chunks=[]
while at<len(wide):
    n,=struct.unpack_from('>I',wide,at)
    kind=wide[at+4:at+8];data=wide[at+8:at+8+n]
    crc,=struct.unpack_from('>I',wide,at+8+n)
    assert binascii.crc32(kind+data)==crc
    if kind==b'IDAT':chunks.append(data)
    at+=n+12
raw=zlib.decompress(b''.join(chunks))
expected=b''.join(bytes((0,))+bytes(v for x in range(8192) for v in (x%251,y,(x+y)%251)) for y in range(3))
assert raw==expected and len(raw)>65535
print('PASS\tmultiple DEFLATE blocks and exact row/pixel reconstruction')
