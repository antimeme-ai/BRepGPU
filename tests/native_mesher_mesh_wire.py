"""Independent literal BGNMESH1 fields and hashlib check of actual C++ output."""
from hashlib import sha256
from pathlib import Path
from struct import pack, unpack_from
import sys


data = Path(sys.argv[1]).read_bytes()
assert data[:8] == b"BGNMESH1"
assert unpack_from("<IIQII", data, 8) == (1, 0, len(data), 29, 0)
tags = ("SOURCE__ EXACT___ LIMBS___ DEFS____ FACES___ NODES___ SITES___ LINEAGE_ "
        "EDGES___ EDGSAM__ JOINTS__ USES____ USESAM__ LOOPS___ CHARTS__ DOMMAPS_ "
        "SUPPORTS PROFILES CELLS___ CELLMAPS CELLFAC_ FACETS__ CORNERS_ QUOTIENT "
        "CLOSURES COMPONTS QUALITY_ INSTS___ APPEAR__").split()
sections = {}
position = 96
for tag in tags:
    assert data[position:position+8] == tag.encode()
    count, length, width, flags = unpack_from("<QQII", data, position+8)
    assert flags == 0
    expected_width = {"EXACT___": 32, "LIMBS___": 4, "FACETS__": 64,
                      "QUALITY_": 24, "APPEAR__": 48}.get(tag, 0)
    assert width == expected_width
    position += 32
    payload = data[position:position+length]
    assert len(payload) == length
    if width:
        assert length == width*count
    else:
        cursor = 0
        for _ in range(count):
            row_size, = unpack_from("<Q", payload, cursor)
            cursor += 8+row_size
            assert cursor <= len(payload)
        assert cursor == len(payload)
    sections[tag] = (count, payload)
    position += length
assert position == len(data)
source_count, source = sections["SOURCE__"]
row_size, byte_count = unpack_from("<QQ", source)
assert source_count == 1 and row_size == byte_count+8 and len(source) == byte_count+16
assert sha256(source[16:]).digest() == data[64:96]
assert sections["EXACT___"][0] == 3
assert sections["EXACT___"][1] == (pack("<iIQIIQ", 0, 0, 0, 1, 0, 0)
                                  + pack("<iIQIIQ", 1, 1, 1, 1, 0, 2)
                                  + pack("<iIQIIQ", 1, 1, 3, 1, 0, 4))
assert sections["LIMBS___"] == (5, pack("<5I", 1, 1, 1, 2, 1))
assert sections["QUALITY_"] == (1, pack("<ddII", 2., 2., 1, 0))
assert sections["FACETS__"] == (2, pack("<16Q", 1, 1, 2, 3, 1, 2, 3, 1,
                                                    1, 1, 3, 4, 4, 5, 6, 1))
assert sections["APPEAR__"] == (1, pack("<QddddQ", 0, .2, .4, .8, .5, 101))
print("PASS independent29-section/literal exact/actual-facet/SHA wire oracle")
