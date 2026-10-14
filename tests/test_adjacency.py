"""Direct incidence and typed edge-use reader truth; execute only on the fleet."""
import os
import struct
import unittest


def backend_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")))


class AdjacencyTests(unittest.TestCase):
    def test_cylinder_seam_literal_members_and_mates(self):
        from brepgpu import Engine
        from brepgpu.body import make_body
        from brepgpu.body_sampling import Candidate, SampleRequest, BodySampler
        body = make_body("cylinder")
        # Establish healthy existing geometry/native execution before reaching
        # the deliberately absent new operation in the initial direct red.
        with Engine(**backend_arguments(), plugin_path=os.environ.get("BREPGPU_TEST_PLUGIN")) as engine:
            with BodySampler(engine, body) as sampler:
                sampled = sampler.sample(SampleRequest(body, (Candidate(2, 0., .5),)))
            self.assertEqual(sampled.rows[0].native_status, 0)
            self.assertEqual(sampled.rows[0].position, (.25, 0., 0.))

        from brepgpu.adjacency import Source, TopologyInput, pack_bodies, AdjacencyLibrary
        source = pack_bodies((body,))
        self.assertIsInstance(source, TopologyInput)
        self.assertEqual((source.edge_count, source.face_count, source.coedge_count), (3, 3, 6))
        self.assertEqual(source.snapshots, (body,))
        self.assertEqual(source.edge_sources, tuple(Source(0, body.body_id, 1, i) for i in (1, 2, 3)))
        self.assertEqual(source.face_sources, tuple(Source(0, body.body_id, 1, i) for i in (1, 2, 3)))
        self.assertEqual(source.coedge_sources, tuple(Source(0, body.body_id, 1, i) for i in range(1, 7)))
        self.assertEqual(source.coedge_edges, struct.pack("<6I", 1, 0, 0, 2, 1, 2))
        self.assertEqual(source.coedge_faces, struct.pack("<6I", 0, 1, 2, 2, 2, 2))
        self.assertEqual(source.orientations, struct.pack("<6i", 1, -1, 1, 1, -1, -1))

        result = AdjacencyLibrary(os.environ["BREPGPU_TOPOLOGY_LIBRARY"]).build(source, **backend_arguments())
        self.assertIs(result.input, source)
        for field in ("offsets", "members", "kinds", "mates", "neighbors"):
            self.assertIsInstance(getattr(result, field), bytes)
        # Read the actual owned binary arrays independently of product readers.
        offsets = struct.unpack("<4I", result.offsets)
        members = struct.unpack("<6I", result.members)
        self.assertEqual(offsets, (0, 2, 4, 6))
        for edge, wanted in enumerate(((1, 2), (0, 4), (3, 5))):
            self.assertCountEqual(members[offsets[edge]:offsets[edge+1]], wanted)
            self.assertCountEqual(result.edge_uses(edge), tuple(source.coedge_sources[i] for i in wanted))
        self.assertEqual(struct.unpack("<3I", result.kinds), (2, 2, 3))
        self.assertEqual(struct.unpack("<6I", result.mates), (4, 2, 1, 5, 0, 3))
        self.assertEqual(struct.unpack("<6I", result.neighbors), (2, 2, 1, 2, 0, 2))
        for use, mate in enumerate((4, 2, 1, 5, 0, 3)):
            self.assertEqual(result.mate(use), source.coedge_sources[mate])
        for use, face in enumerate((2, 2, 1, 2, 0, 2)):
            self.assertEqual(result.neighbor(use), source.face_sources[face])
        # Same-face seam retains coedge4 and coedge6, never a unique-face dedup.
        self.assertNotEqual(result.mate(3), source.coedge_sources[3])
        self.assertEqual(result.neighbor(3), source.face_sources[2])
        self.assertEqual(result.neighbor(5), source.face_sources[2])


if __name__ == "__main__":
    unittest.main()
