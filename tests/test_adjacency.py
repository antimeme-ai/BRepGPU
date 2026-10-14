"""Direct incidence and typed edge-use reader truth; execute only on the fleet."""
import os
import random
import struct
import unittest
from dataclasses import FrozenInstanceError


def backend_arguments():
    return dict(backend=os.environ.get("BREPGPU_TEST_BACKEND", "cpu"),
                device=int(os.environ.get("BREPGPU_TEST_DEVICE", "0")),
                threads=int(os.environ.get("BREPGPU_TEST_THREADS", "1")))


class AdjacencyTests(unittest.TestCase):
    def build(self, source):
        from brepgpu.adjacency import AdjacencyLibrary
        return AdjacencyLibrary(os.environ["BREPGPU_TOPOLOGY_LIBRARY"]).build(source, **backend_arguments())

    def graph(self, edge_count, face_count, rows):
        from brepgpu.adjacency import Source, TopologyInput
        rows = tuple(rows)
        # Non-dense high IDs attack any confusion between stable names and slots.
        return TopologyInput(
            tuple(Source(7, 19, 3, (1 << 63)+17*i+1) for i in range(edge_count)),
            tuple(Source(7, 19, 3, 31*i+3) for i in range(face_count)),
            tuple(Source(7, 19, 3, 29*i+5) for i in range(len(rows))),
            tuple(row[0] for row in rows), tuple(row[1] for row in rows), tuple(row[2] for row in rows))

    def qualified(self, source, *, literal=False):
        from brepgpu.adjacency_oracles import qualify_input, qualify_result
        qualify_input(source, literal=literal)
        result = self.build(source)
        metrics = qualify_result(result, literal=literal)
        self.assertIs(result.input, source)
        return result, metrics

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

    def test_all_primitive_signed_sources_holes_and_reader_roles(self):
        from brepgpu.body import make_body
        from brepgpu.body_oracles import qualify_snapshot
        from brepgpu.adjacency import pack_bodies
        for shape, counts, same_face in (("box", (12, 6, 24), 0),
                                         ("cylinder", (3, 3, 6), 1),
                                         ("plate", (15, 7, 30), 1)):
            with self.subTest(shape=shape):
                body = make_body(shape)
                qualify_snapshot(body)
                result, metrics = self.qualified(pack_bodies((body,)), literal=True)
                self.assertEqual((metrics["edges"], metrics["faces"], metrics["coedges"]), counts)
                self.assertEqual(metrics["same_face_pair"], same_face)
                self.assertEqual(metrics["paired"], counts[0]-same_face)
                if shape == "plate":
                    # Actual cavity cap-hole/ring neighbors and distinct seam uses.
                    self.assertEqual(result.mate(4), result.input.coedge_sources[27])
                    self.assertEqual(result.mate(9), result.input.coedge_sources[29])
                    self.assertEqual(result.mate(26), result.input.coedge_sources[28])
                    self.assertEqual(result.neighbor(26), result.input.face_sources[6])

    def test_mixed_repeated_instances_preserve_typed_identity_under_permutation(self):
        from brepgpu.body import make_body
        from brepgpu.adjacency import pack_bodies
        box, cylinder, plate = (make_body(shape) for shape in ("box", "cylinder", "plate"))
        source = pack_bodies((box, cylinder, plate, cylinder))
        original, metrics = self.qualified(source, literal=True)
        self.assertEqual(metrics["instances"], 4)
        self.assertEqual(metrics["same_face_pair"], 3)
        self.assertEqual(metrics["edges"], 33)
        self.assertEqual(metrics["coedges"], 66)
        rng = random.Random(0xA5B0)
        orders = [list(range(count)) for count in (source.edge_count, source.face_count, source.coedge_count)]
        for order in orders:
            rng.shuffle(order)
        changed = source.permuted(edge_order=orders[0], face_order=orders[1], coedge_order=orders[2])
        result, shuffled = self.qualified(changed, literal=True)
        self.assertEqual(shuffled, metrics)
        # Compare actual public consumer answers in typed occurrence space.
        original_mates = {key: original.mate(i) for i, key in enumerate(source.coedge_sources)}
        original_faces = {key: original.neighbor(i) for i, key in enumerate(source.coedge_sources)}
        self.assertEqual({key: result.mate(i) for i, key in enumerate(changed.coedge_sources)}, original_mates)
        self.assertEqual({key: result.neighbor(i) for i, key in enumerate(changed.coedge_sources)}, original_faces)
        # Same snapshot geometry is repeated without collapsing its instances.
        repeated = [key for key in changed.coedge_sources if key.body_id == cylinder.body_id and key.entity_id == 4]
        self.assertEqual({key.instance for key in repeated}, {1, 3})

    def test_all_degree_sign_and_same_face_classes(self):
        rows = ((1, 0, 1), (2, 0, 1), (2, 1, -1), (3, 2, 1), (3, 2, -1),
                (4, 0, -1), (4, 1, -1), (5, 0, 1), (5, 1, -1), (5, 2, 1))
        source = self.graph(7, 3, rows)
        result, metrics = self.qualified(source)
        self.assertEqual(struct.unpack("<7I", result.kinds), (0, 1, 2, 3, 4, 5, 0))
        self.assertEqual(tuple(result.edge_kind(slot) for slot in range(7)), (0, 1, 2, 3, 4, 5, 0))
        self.assertEqual((metrics["unused"], metrics["boundary"], metrics["paired"],
                          metrics["same_face_pair"], metrics["orientation_conflict"], metrics["multiple_uses"]),
                         (2, 1, 1, 1, 1, 1))
        for slot in (0, 5, 6, 7, 8, 9):
            self.assertIsNone(result.mate(slot))
            self.assertIsNone(result.neighbor(slot))
        self.assertEqual(result.mate(1), source.coedge_sources[2])
        self.assertEqual(result.neighbor(1), source.face_sources[1])
        self.assertEqual(result.mate(3), source.coedge_sources[4])
        self.assertEqual(result.neighbor(3), source.face_sources[2])

    def test_empty_inputs_and_unused_edge_tables(self):
        from brepgpu.adjacency import Source, TopologyInput, pack_bodies
        for source in (pack_bodies(()), self.graph(7, 0, ()),
                       TopologyInput((), (Source(0, 1, 1, 1),), (), (), (), ())):
            with self.subTest(edges=source.edge_count, faces=source.face_count):
                result, metrics = self.qualified(source)
                self.assertEqual(result.offsets, bytes(4*(source.edge_count+1)))
                self.assertEqual(result.members, b"")
                self.assertEqual(result.mates, b"")
                self.assertEqual(result.neighbors, b"")
                self.assertEqual(metrics["unused"], source.edge_count)

    def test_random_incidence_and_schedule_independent_membership(self):
        rng = random.Random(0xB0D1E5)
        for size in (1, 17, 255, 256, 257, 1025):
            rows = [(rng.randrange(size), rng.randrange(7), rng.choice((-1, 1))) for _ in range(3*size)]
            rng.shuffle(rows)
            with self.subTest(size=size):
                self.qualified(self.graph(size, 7, rows))
        crowded = self.graph(19, 5, ((7, index % 5, 1 if index % 2 else -1) for index in range(4097)))
        # Normalize the generator in graph independently; no scheduling order
        # expectation is asserted, only the complete occurrence relation.
        for _ in range(3):
            result, metrics = self.qualified(crowded)
            self.assertEqual(metrics["multiple_uses"], 1)
            self.assertEqual(len(result.edge_uses(7)), 4097)

    def test_recursive_scan_uneven_boundaries_and_empty_tail(self):
        # 257 blocks of counts, so the totals themselves need multiple blocks.
        # A nonzero count at edge65536 attacks upper-level uniform-add.
        rows = [(edge, use % 3, 1 if use % 2 else -1)
                for edge, degree in ((0, 1), (254, 3), (255, 2), (256, 7),
                                     (257, 1), (65534, 2), (65535, 3), (65536, 4))
                for use in range(degree)]
        rows.reverse()
        source = self.graph(65539, 3, rows)
        result, metrics = self.qualified(source)
        offsets = struct.unpack("<65540I", result.offsets)
        for edge, expected in ((0, 0), (1, 1), (254, 1), (255, 4), (256, 6),
                               (257, 13), (258, 14), (65534, 14), (65535, 16),
                               (65536, 19), (65537, 23), (65538, 23), (65539, 23)):
            self.assertEqual(offsets[edge], expected)
        self.assertEqual(metrics["unused"], 65531)

    def test_frames_do_not_change_topology_and_owned_outputs_survive_library(self):
        from brepgpu.body import Frame, make_body
        from brepgpu.adjacency import pack_bodies
        for shape in ("box", "cylinder", "plate"):
            plain = pack_bodies((make_body(shape),))
            framed = pack_bodies((make_body(shape, Frame((16., -16., .125), 4.)),))
            a, ma = self.qualified(plain, literal=True)
            b, mb = self.qualified(framed, literal=True)
            self.assertEqual(ma, mb)
            for name in ("offsets", "kinds", "mates", "neighbors"):
                self.assertEqual(getattr(a, name), getattr(b, name))
            for item, field in ((plain, "coedge_edges"), (a, "offsets"), (plain.edge_sources[0], "entity_id")):
                with self.assertRaises((FrozenInstanceError, AttributeError, TypeError)):
                    setattr(item, field, b"")
            # self.build's local library object is already gone; bytes/readers
            # must remain usable independently of its Python or CUDA lifetime.
            self.assertEqual(len(a.edge_uses(0)), 2)

    def test_prepared_rejection_and_slot_admission(self):
        from brepgpu.adjacency import Source, TopologyInput, pack_bodies
        from brepgpu.body import make_body
        one = (Source(0, 1, 1, 1),)
        wrong_instance = (Source(1, 1, 1, 1),)
        wrong_revision = (Source(0, 1, 2, 1),)
        for edge, face, coedge, es, fs, signs in (
                (one, one, one, (1,), (0,), (1,)),
                (one, one, one, (0,), (1,), (1,)),
                (one, one, one, (0,), (0,), (0,)),
                (one, one, one, (0,), (0,), (2,)),
                (one, one, one, (), (0,), (1,)),
                (one, one, one, b"\0", (0,), (1,)),
                (one+one, one, one, (0,), (0,), (1,)),
                (one, wrong_instance, one, (0,), (0,), (1,)),
                (one, wrong_revision, one, (0,), (0,), (1,)),
                ((), one, one, (0,), (0,), (1,))):
            with self.subTest(edge=edge, face=face, signs=signs):
                with self.assertRaises((TypeError, ValueError, OverflowError)):
                    TopologyInput(edge, face, coedge, es, fs, signs)
        for fields in ((-1, 1, 1, 1), (0, 0, 1, 1), (0, 1, 0, 1), (0, 1, 1, 0),
                       (1 << 64, 1, 1, 1), (0, 1, 1, 1 << 64), (False, 1, 1, 1)):
            with self.subTest(source=fields):
                with self.assertRaises((TypeError, ValueError, OverflowError)):
                    Source(*fields)
        source = pack_bodies((make_body("cylinder"),))
        result = self.build(source)
        for method, count in ((result.edge_uses, 3), (result.edge_kind, 3),
                              (result.mate, 6), (result.neighbor, 6)):
            for slot in (-1, count, True, .5):
                with self.subTest(method=method.__name__, slot=slot):
                    with self.assertRaises((IndexError, TypeError, ValueError)):
                        method(slot)
        for orders in (dict(edge_order=(0, 0, 2)), dict(face_order=(0, 1)),
                       dict(coedge_order=(0, 1, 2, 3, 4, 6))):
            with self.assertRaises((TypeError, ValueError, IndexError)):
                source.permuted(**orders)

    def test_prepared_arrays_and_source_tables_own_submitted_values(self):
        from brepgpu.adjacency import Source, TopologyInput
        edges = [Source(0, 5, 2, 7)]
        faces = [Source(0, 5, 2, 11), Source(0, 5, 2, 13)]
        uses = [Source(0, 5, 2, 17), Source(0, 5, 2, 19)]
        es = bytearray(struct.pack("<2I", 0, 0))
        fs = bytearray(struct.pack("<2I", 0, 1))
        signs = bytearray(struct.pack("<2i", 1, -1))
        source = TopologyInput(edges, faces, uses, es, memoryview(fs), signs)
        edges.clear()
        faces.clear()
        uses.clear()
        es[:] = struct.pack("<2I", 123, 456)
        fs[:] = struct.pack("<2I", 789, 987)
        signs[:] = struct.pack("<2i", 0, 0)
        result, metrics = self.qualified(source)
        self.assertEqual((source.edge_count, source.face_count, source.coedge_count), (1, 2, 2))
        self.assertEqual(source.coedge_edges, struct.pack("<2I", 0, 0))
        self.assertEqual(source.coedge_faces, struct.pack("<2I", 0, 1))
        self.assertEqual(source.orientations, struct.pack("<2i", 1, -1))
        self.assertEqual(result.mate(0), Source(0, 5, 2, 19))
        self.assertEqual(result.neighbor(0), Source(0, 5, 2, 13))
        self.assertEqual(metrics["paired"], 1)


if __name__ == "__main__":
    unittest.main()
