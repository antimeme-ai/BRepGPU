"""Direct camera/framing and encoded output answers; execute on fleet only."""
import struct
import tempfile
import unittest
import zlib
from pathlib import Path


class SceneDirectTests(unittest.TestCase):
    def test_original_world_axes_project_up_and_camera_frames_all_source_corners(self):
        from om10_scene import camera_view, project_point, pack_views
        # Source thickness is along Z; top view must retain the XY movement plane.
        points = ((-10., -10., -1.), (10., 10., 1.), (-10., 10., 1.), (10., -10., -1.))
        view = camera_view(points, width=800, height=600, azimuth=-90., elevation=90.,
                           padding=(100, 100, 60, 90))
        origin = project_point(view, (0., 0., 0.))
        up = project_point(view, (0., 1., 0.))
        self.assertLess(up[1], origin[1])
        for p in points:
            x, y, _ = project_point(view, p)
            self.assertTrue(100 <= x <= 700)
            self.assertTrue(60 <= y <= 510)
        wire = pack_views((view,))
        self.assertEqual((wire[:8], len(wire)), (b'BGVIEW01', 208))
        self.assertEqual(struct.unpack_from('<4I', wire, 16), (800, 600, 1, 0))

    def test_png_contains_actual_complete_rgba_and_valid_chunk_crcs(self):
        from om10_scene import write_png
        pixels = bytes((255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255))
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'actual.png'
            write_png(path, 2, 2, pixels)
            data = path.read_bytes()
        self.assertEqual(data[:8], b'\x89PNG\r\n\x1a\n')
        offset, compressed = 8, bytearray()
        while offset < len(data):
            size, = struct.unpack_from('>I', data, offset)
            tag = data[offset+4:offset+8]
            payload = data[offset+8:offset+8+size]
            crc, = struct.unpack_from('>I', data, offset+8+size)
            self.assertEqual(zlib.crc32(tag+payload)&0xffffffff, crc)
            if tag == b'IDAT':
                compressed.extend(payload)
            offset += 12+size
        self.assertEqual(zlib.decompress(compressed), b'\x00'+pixels[:8]+b'\x00'+pixels[8:])
