"""Direct original-source export contracts; execute on the fleet only."""
import os
import dataclasses
import hashlib
import math
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


FIXTURES = Path(__file__).parent/'fixtures'


def executable():
    return Path(os.environ.get('BREPGPU_REFERENCE_SOURCE',
        'build/om10-reference-source/brepgpu-reference-source')).resolve()


def inspector():
    return Path(os.environ.get('BREPGPU_REFERENCE_SOURCE_FIXTURE',
        'build/om10-reference-source/brepgpu-reference-source-fixture')).resolve()


def check(path):
    """The fixture tool's independent reading of one exported .brep: BRepCheck and volume."""
    completed = subprocess.run((str(inspector()), '--check', str(path)), capture_output=True, text=True)
    if completed.returncode:
        raise AssertionError(completed.stderr)
    values = dict(line.split(' ', 1) for line in completed.stdout.splitlines() if not line.startswith('face '))
    return {k: float(v) for k, v in values.items()}


def records(directory, stage=None):
    rows = [line.split('\t') for line in (Path(directory)/'records.tsv').read_text().splitlines()[1:]]
    return [r for r in rows if stage is None or r[0] == stage]


class _Export:
    def export(self, directory, *, source=None, success=True, mode=None):
        source = source or FIXTURES/'reference-source.step'
        mode_arguments = ('--shape-processing', mode) if mode else ()
        completed = subprocess.run((str(executable()), *mode_arguments, str(source), str(directory)),
            capture_output=True, text=True)
        if success:
            self.assertEqual(completed.returncode, 0, completed.stderr)
        else:
            self.assertNotEqual(completed.returncode, 0, 'invalid export accepted')
        return completed

    def exported(self, text, directory, name, mode=None, success=True):
        step = Path(directory)/f'{name}.step'
        step.write_text(text)
        completed = self.export(Path(directory)/name, source=step, mode=mode, success=success)
        return completed, Path(directory)/name


class ReferenceSourceTests(unittest.TestCase, _Export):

    def test_fresh_original_source_export(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(__file__).parent/'fixtures'/'reference-source.step'
            completed=self.export(Path(directory)/'export', source=source)
            self.assertIn('shape_process_before_transfer\t1\t0',completed.stdout)
            self.assertIn('shape_process_after_transfer\t1\t0',completed.stdout)
            from brepgpu.reference_source import load_reference_source
            from brepgpu.imported_assembly import assemble
            result = load_reference_source(Path(directory)/'export'/'source.bgocct')
            self.assertEqual(result.source_sha256, hashlib.sha256(source.read_bytes()).hexdigest())
            self.assertEqual(result.units_mm, 1.)
            self.assertEqual(result.root_product_id, 5)
            self.assertEqual(tuple(d.id for d in result.definitions), (41, 395))
            self.assertEqual(tuple(len(d.faces) for d in result.definitions), (6, 3))
            self.assertEqual(tuple(u.id for u in result.usages), (380, 386, 502))
            assembly = assemble(result)
            self.assertEqual(tuple(o.definition_id for o in assembly.occurrences), (41, 41, 395))
            self.assertEqual(assembly.occurrences[1].transform[3::4], (4., -3., 2.))
            for a, b in zip(assembly.occurrences[1].transform,
                    (0.,-1.,0.,4.,1.,0.,0.,-3.,0.,0.,1.,2.)):
                self.assertAlmostEqual(a, b, places=14)
            self.assertEqual(tuple(f.orientation for f in result.definitions[0].faces), (-1,1,-1,1,-1,1))
            # Format version 2: one translated face per original face under
            # translation-only, the outer shell only, no occurrence styles here.
            self.assertTrue(all(len(f.serialized_storage_indices) == 1 and f.shell == 0 for f in result.faces))
            self.assertEqual(tuple(d.translated_face_count for d in result.definitions), (6, 3))
            self.assertEqual(result.occurrence_styles, ())
            self.assertIn('shape-processing=translation-only', result.adapter_revision)
            self.assertIn('raw-entity-corrections=none', result.adapter_revision)
            self.assertTrue(all(math.isfinite(t) and t >= 0 for t in result.phase_seconds.values()))
            with self.assertRaises(dataclasses.FrozenInstanceError):
                result.units_mm = 10.
            with self.assertRaises(TypeError):
                result.phase_seconds['read_step'] = 0.
            with self.assertRaises(TypeError):
                dataclasses.replace(result, products=list(result.products))

    def test_v4_readback_geometry_orientation_and_absent_mesh(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/'export'
            self.export(output)
            for id in (41,395):
                path = output/f'definition-{id}.brep'
                self.assertTrue(path.read_bytes().startswith(b'\nOpen CASCADE Topology V4, (c) Open Cascade\n'))
                completed = subprocess.run((str(inspector()),'--inspect',str(path)),capture_output=True,text=True)
                self.assertEqual(completed.returncode, 0, completed.stderr)
                rows = tuple(line.split() for line in completed.stdout.splitlines())
                points = {tuple(map(float,row[1:])) for row in rows if row[0]=='vertex'}
                planes = [tuple(map(float,row[1:])) for row in rows if row[0]=='plane']
                oriented = {(tuple(sign*n for n in normal),sign*sum(x*n for x,n in zip(origin,normal)))
                    for sign,*values in planes for origin,normal in [(values[:3],values[3:])]}
                if id == 41:
                    self.assertEqual(points,{(x,y,z) for x in (.125,1.125) for y in (.25,2.25) for z in (.5,3.5)})
                    self.assertEqual(oriented,{((-1.,0.,0.),-.125),((1.,0.,0.),1.125),
                        ((0.,-1.,0.),-.25),((0.,1.,0.),2.25),((0.,0.,-1.),-.5),((0.,0.,1.),3.5)})
                else:
                    self.assertEqual(points,{(-.75,2.25,.125),(-.75,2.25,2.125)})
                    self.assertEqual(oriented,{((0.,0.,-1.),-.125),((0.,0.,1.),2.125)})
                    self.assertEqual([tuple(map(float,row[1:])) for row in rows if row[0]=='cylinder'],
                        [(1.,-1.5,2.25,.125,0.,0.,1.,.75)])

    def test_malformed_manifest_graph_paths_and_partial_output_raise(self):
        from brepgpu.reference_source import load_reference_source
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/'export'
            self.export(output)
            path=output/'source.bgocct';raw=path.read_bytes();offsets={};at=88
            while at<len(raw):
                tag=raw[at:at+8];count,size=struct.unpack_from('<QQ',raw,at+8);p=at+24;entries=[]
                for _ in range(count):
                    n=struct.unpack_from('<Q',raw,p)[0];entries.append(p+8);p+=8+n
                offsets[tag]=entries;at+=24+size
            cases=[raw[:cut] for cut in (0,8,87,len(raw)-1)]+[raw+b'\0']
            def changed(offset,fmt,value):
                bad=bytearray(raw);struct.pack_into('<'+fmt,bad,offset,value);cases.append(bad)
            changed(8,'I',3);changed(12,'I',1);changed(48,'d',10.);changed(48,'d',float('nan'))
            changed(96,'Q',2**64-1)
            changed(offsets[b'PRODUCTS'][0]+8,'Q',999)
            meta=offsets[b'META____'][0];length=struct.unpack_from('<I',raw,meta)[0]
            changed(meta+4+length+8,'d',-1.);changed(meta+4+length+8,'d',float('nan'))
            definition=offsets[b'DEFS____'][0];length=struct.unpack_from('<I',raw,definition+16)[0]
            # Face rows: id, carrier, i32 orientation, u32 shell, u64 count, storage indices.
            name=definition+20;translated=name+length;firstface=name+length+16
            bad=bytearray(raw);bad[name:name+length]=b'../'+b'x'*(length-3);cases.append(bad)
            index=struct.unpack_from('<Q',raw,firstface+32)[0]
            changed(firstface+40+32,'Q',index);changed(firstface+16,'i',0)
            # A shell-ordinal gap; a translated face count that is not the face map's; version 1.
            changed(firstface+20,'I',2);changed(translated,'Q',7);changed(8,'I',1)
            for bad in cases:
                path.write_bytes(bad)
                with self.assertRaises(ValueError):
                    load_reference_source(path)
            for offset,fmt,value,message in ((8,'I',1,'version 1'),(firstface+20,'I',2,'shell ordinals have a gap'),
                                             (translated,'Q',7,'translated face count differs')):
                bad=bytearray(raw);struct.pack_into('<'+fmt,bad,offset,value);path.write_bytes(bad)
                with self.assertRaisesRegex(ValueError,message):
                    load_reference_source(path)
            path.write_bytes(raw)
            (output/'definition-41.brep').unlink()
            with self.assertRaisesRegex(ValueError,'missing'):
                load_reference_source(path)
            with self.assertRaises(OSError):
                load_reference_source(Path(directory)/'never-published.bgocct')

    def test_split_face_reads_its_pieces_and_refuses_shared_pieces(self):
        """A face OCCT's default read split carries several storage indices; the definition's
        translated face count counts them, and no piece belongs to two faces."""
        import dataclasses
        from brepgpu import reference_source
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/'export'
            self.export(output)
            source = reference_source.load_reference_source(output/'source.bgocct')
            box = source.definitions[0]
            first, second, *rest = box.faces
            def with_faces(faces, count):
                definition = dataclasses.replace(box, faces=faces, translated_face_count=count)
                return dataclasses.replace(source, definitions=(definition, *source.definitions[1:]))
            split = dataclasses.replace(first, serialized_storage_indices=(first.serialized_storage_index, 99))
            reference_source._validate(with_faces((split, second, *rest), 7), output)
            with self.assertRaisesRegex(ValueError, 'no single storage index'):
                split.serialized_storage_index
            with self.assertRaisesRegex(ValueError, 'translated face count differs'):
                reference_source._validate(with_faces((split, second, *rest), 6), output)
            shared = dataclasses.replace(first, serialized_storage_indices=(first.serialized_storage_index,
                                                                          second.serialized_storage_index))
            with self.assertRaisesRegex(ValueError, 'duplicate reference face or storage index'):
                reference_source._validate(with_faces((shared, second, *rest), 7), output)
            with self.assertRaisesRegex(ValueError, 'has no translated face'):
                reference_source._validate(with_faces((dataclasses.replace(first, serialized_storage_indices=()),
                                                       second, *rest), 5), output)

    def test_native_non_mm_existing_directory_and_write_failure_raise(self):
        with tempfile.TemporaryDirectory() as directory:
            directory=Path(directory)
            self.export(directory,success=False)
            self.export(directory/'absent-parent'/'output',success=False)
            source=directory/'cm.step'
            original=Path(__file__).parent/'fixtures'/'reference-source.step'
            source.write_bytes(original.read_bytes().replace(b'.MILLI.',b'.CENTI.'))
            self.export(directory/'cm-export',source=source,success=False)
            self.assertFalse((directory/'cm-export'/'source.bgocct').exists())


def substituted(text, old, new):
    if text.count(old) != 1:
        raise AssertionError(f'fixture substitution must match exactly once: {old!r}')
    return text.replace(old, new)


class VoidShellTests(unittest.TestCase, _Export):
    """bead .37.22: the reference export carries BREP_WITH_VOIDS voids as B1 did.

    Oracles: the STEP text (shells, same_sense, the void's .F.), the adapter's face
    table for the same file, and the enclosed volume 10^3 - 4^3 = 936 read
    independently from the exported .brep."""

    def test_void_block_exports_every_shell(self):
        from brepgpu.reference_source import load_reference_source
        from brepgpu.imported_source import load_source
        from tests.test_step_ingress import _Adapter
        text = (FIXTURES/'void-block.step').read_text()
        with tempfile.TemporaryDirectory() as directory:
            for mode in ('translation-only', 'default'):
                with self.subTest(mode=mode):
                    _, out = self.exported(text, directory, mode, mode=mode)
                    source = load_reference_source(out/'source.bgocct')
                    definition, = source.definitions
                    self.assertEqual((len(definition.faces), definition.translated_face_count), (12, 12))
                    self.assertEqual([f.shell for f in definition.faces], [0]*6+[1]*6)
                    measured = check(out/definition.filename)
                    self.assertEqual((measured['shells'], measured['valid']), (2, 1))
                    self.assertAlmostEqual(measured['volume'], 936., places=9)
            run, _, target = _Adapter.extract(self, text, directory, 'adapter')
            self.assertEqual(run.returncode, 0, run.stderr)
            owned = load_source(target)
            # Orientation is the use in the solid: same_sense composed with the void's .F.
            self.assertEqual({f.id: (f.carrier_id, f.orientation, f.shell, f.style_id) for f in source.faces},
                             {f.id: (f.carrier_id, f.orientation, f.shell, f.style_id) for f in owned.faces})

    def test_void_shell_rules_refuse_by_name(self):
        text = (FIXTURES/'void-block.step').read_text()
        cases = {'.T. void': (("#165 = ORIENTED_CLOSED_SHELL('',*,#166,.F.);", "#165 = ORIENTED_CLOSED_SHELL('',*,#166,.T.);"),
                              'ABSR WR4 requires every void'),
                 'face in two shells': (("#166 = CLOSED_SHELL('',(#167,", "#166 = CLOSED_SHELL('',(#17,#167,"),
                                        'source face in more than one shell')}
        with tempfile.TemporaryDirectory() as directory:
            for name, ((old, new), message) in cases.items():
                with self.subTest(name):
                    completed, _ = self.exported(substituted(text, old, new), directory, name.replace(' ', '-'),
                                                 success=False)
                    self.assertIn(message, completed.stderr)


class DefaultReadTests(unittest.TestCase, _Export):
    """bead .37.19: shape-processing=default is OCCT's own default STEP read, and its
    face map binds each original face to the faces OCCT's processing made of it.

    Oracles: OCCT's reported flags (nothing supplied before transfer, its default in
    force after), the minimal NVIDIA ingress's default read of the same bytes (the
    input NVIDIA receives), and BRepCheck on the exported files."""

    def test_default_read_declares_and_runs_occt_defaults(self):
        from brepgpu.reference_source import load_reference_source
        ingress = Path(os.environ.get('BREPGPU_NVIDIA_INGRESS', 'build/nvidia-ingress/brepgpu-nvidia-ingress')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            completed = self.export(directory/'default', mode='default')
            self.assertIn('shape_process_before_transfer\t0\t0\n', completed.stdout)
            after = [l.split('\t') for l in completed.stdout.splitlines() if l.startswith('shape_process_after_transfer')]
            self.assertEqual(after[0][1], '1')
            self.assertGreater(int(after[0][2]), 0)
            source = load_reference_source(directory/'default'/'source.bgocct')
            tokens = [t.strip() for t in source.adapter_revision.split(';')]
            self.assertIn('shape-processing=default', tokens)
            self.assertIn('raw-entity-corrections=none', tokens)
            bits, = [t.partition('=')[2] for t in tokens if t.startswith('shape-process-operations=')]
            subprocess.run((str(ingress), '--shape-processing', 'default', str(FIXTURES/'reference-source.step'),
                            str(directory/'minimal')), check=True, capture_output=True)
            policy = dict(l.split('\t') for l in (directory/'minimal'/'policy.tsv').read_text().splitlines()[1:])
            self.assertEqual(policy['operation_bits'], bits)
            self.export(directory/'translation', mode='translation-only')
            translation = load_reference_source(directory/'translation'/'source.bgocct')
            # The same graph, styles and original face inventory as translation-only.
            for field in ('products', 'usages', 'styles', 'occurrence_styles', 'root_product_id'):
                self.assertEqual(getattr(source, field), getattr(translation, field), field)
            self.assertEqual({f.id: (f.carrier_id, f.orientation, f.shell) for f in source.faces},
                             {f.id: (f.carrier_id, f.orientation, f.shell) for f in translation.faces})
            for definition in source.definitions:
                # Our face map describes the file NVIDIA receives.
                self.assertEqual((directory/'default'/definition.filename).read_bytes(),
                                 (directory/'minimal'/definition.filename).read_bytes())
                self.assertEqual(check(directory/'default'/definition.filename)['faces'], definition.translated_face_count)

    def test_default_read_that_changes_the_solid_refuses_by_name(self):
        """One shell holding the void block's two cubes, written as one MANIFOLD_SOLID_BREP:
        translation-only keeps one solid of one shell; OCCT's FixShape reads the nested cube
        as a void (two shells) and two disjoint cubes as two solids. A definition is one
        solid of the original's shells, so both default reads refuse, each by name. For
        the nested cubes the face-side pre-pass refuses first, in both modes (review
        round 2: an undecided face refuses translation-only too): FixShape turns the inner
        cube's faces, whose raw uses point into the material between the cubes, but each
        agrees with its own bound and the one raw shell is not inside out as a whole, so
        neither an S1 nor an S2 correction describes the turn."""
        import re
        nested = substituted(substituted((FIXTURES/'void-block.step').read_text(),
            "#15 = BREP_WITH_VOIDS('',#16,(#165));", "#15 = MANIFOLD_SOLID_BREP('',#16);"),
            "#16 = CLOSED_SHELL('',(#17,#57,#97,#119,#141,#153));",
            "#16 = CLOSED_SHELL('',(#17,#57,#97,#119,#141,#153,#167,#207,#247,#269,#291,#303));")
        def shifted(match):
            values = match.group(2).split(',')
            if int(match.group(1)) > 165 and len(values) == 3:
                values[0] = repr(float(values[0])+20.)
            return f"#{match.group(1)} = CARTESIAN_POINT('',({','.join(values)}));"
        disjoint = re.sub(r"#(\d+) = CARTESIAN_POINT\('',\(([^)]*)\)\);", shifted, nested)
        undecided = ('original face #167 of definition #15: undecidable: the face agrees with '
                     'its own bound, but its shell is not inside out as a whole')
        with tempfile.TemporaryDirectory() as directory:
            for name, text, message, translation in (
                    # Review round 2 ruling: an undecided face refuses translation-only too.
                    ('nested', nested, undecided, undecided),
                    # Coordinator ruling: a definition whose sides cannot be checked refuses too.
                    ('disjoint', disjoint, "OCCT's default read made original solid #15 into 2 solids",
                     "definition #15: OCCT's default read does not make it one solid, so its face sides cannot be checked")):
                with self.subTest(name):
                    completed, _ = self.exported(text, directory, name+'-translation', mode='translation-only',
                                                 success=translation is None)
                    if translation is not None:
                        self.assertIn(translation, completed.stderr)
                    completed, _ = self.exported(text, directory, name+'-default', mode='default', success=False)
                    self.assertIn(message, completed.stderr)

    def test_default_read_that_repairs_a_face_admits_it_with_a_record(self):
        """bead .37.35, decision (b): the void block with outer face #17's same_sense
        flipped, so its raw use points into the material. Translation-only keeps the raw
        use. OCCT's FixShape turns the face: its orientation flag differs from the raw use
        while its surface is the raw one, so the outward normals at an interior point are
        opposite, and ray parity from just off the face along the raw normal finds
        material. The processing repaired the file: the face is admitted with a record,
        and the exported solid is valid with the block's volume."""
        text = substituted((FIXTURES/'void-block.step').read_text(), "#17 = ADVANCED_FACE('',(#18),#52,.F.);",
                           "#17 = ADVANCED_FACE('',(#18),#52,.T.);")
        with tempfile.TemporaryDirectory() as directory:
            completed, out = self.exported(text, directory, 'translation', mode='translation-only')
            # bead .37.36: the translation-only export writes the outward side too (S1).
            self.assertIn('reference_face_orientation_corrected\t15\t17\tS1\n', completed.stdout)
            self.assertAlmostEqual(check(out/'definition-15.brep')['volume'], 936., places=9)
            completed, out = self.exported(text, directory, 'default', mode='default')
            self.assertIn('default_read_face_verdict\t15\t17\tturned: the raw use points into the material\t1 pieces\n',
                          completed.stdout)
            self.assertIn('default_read_repaired_faces\t1\n', completed.stdout)
            self.assertIn('default_read_turned_faces\t0\n', completed.stdout)
            self.assertEqual([r[2] for r in records(out, 'default_read_face_repaired')], ['15 17'])
            measured = check(out/'definition-15.brep')
            self.assertEqual(measured['valid'], 1)
            self.assertAlmostEqual(measured['volume'], 936., places=9)
        # The unedited block reads the same in both modes: no flag differs, nothing refuses.
        with tempfile.TemporaryDirectory() as directory:
            completed, _ = self.exported((FIXTURES/'void-block.step').read_text(), directory, 'plain', mode='default')
            self.assertIn('default_read_reoriented_faces\t0\n', completed.stdout)
            self.assertIn('default_read_turned_faces\t0\n', completed.stdout)

    def test_orientation_verdicts_on_authored_box_configurations(self):
        """bead .37.35, the decision itself: the exporter's per-face verdict
        (native/reference_source/orientation_verdict.hpp) on an OCCT box, whose material
        is known, for every face and every verdict. No STEP file was found for which
        OCCT's FixShape turns a face whose raw use agrees with its shell (every authored
        same_sense flip of the void block, 189 of them, is repaired), so that branch is
        driven here on authored geometry: OCCT's use opposite the box's, the raw use the box's."""
        completed = subprocess.run((str(inspector()), '--verdicts'), capture_output=True, text=True)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        rows = [line.split(' ', 3) for line in completed.stdout.splitlines()]
        expected = {'repaired': 'turned: the raw use points into the material',
                    'turned': 'turned: the raw use agrees with the closed shell',
                    'together': 'reversed_together'}
        self.assertEqual(len(rows), 24)
        for _, face, case, verdict in rows:
            with self.subTest(face=face, case=case):
                if case == 'moved':
                    self.assertTrue(verdict.startswith('undecidable: its exported face leaves the raw surface'), verdict)
                else:
                    self.assertEqual(verdict, expected[case])

    def test_face_verdict_multi_piece_branches(self):
        """B2-reader-export review round 1: face_verdict over several pieces of one face,
        on pieces of known material (the fixture tool's --verdict-pieces: faces on z=1 of
        fused boxes)."""
        completed = subprocess.run((str(inspector()), '--verdict-pieces'), capture_output=True, text=True)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        verdicts = dict(line.split(' ', 1) for line in completed.stdout.splitlines())
        self.assertEqual(verdicts, {
            'repaired': 'turned: the raw use points into the material',
            'left_inward': "turned: OCCT's default read leaves a piece of the face inward",
            'together': 'reversed_together',
            'turned': 'turned: the raw use agrees with the closed shell',
            'mixed': "undecidable: the raw use points into the material on some of OCCT's pieces of the face "
                     'and out of it on others'})

    def test_mode_is_named(self):
        with tempfile.TemporaryDirectory() as directory:
            completed = subprocess.run((str(executable()), '--shape-processing', 'fixshape',
                                        str(FIXTURES/'reference-source.step'), str(Path(directory)/'out')),
                                       capture_output=True, text=True)
            self.assertEqual(completed.returncode, 2)
            self.assertIn('translation-only|default', completed.stderr)


class VertexLoopFaceTests(unittest.TestCase, _Export):
    """bead .37.23 (vertex loops): OCCT's translation without ShapeProcess builds the
    apex cone's face with no seam and no pcurves, which BRepCheck refuses and whose
    volume is wrong; the default read builds a valid face.

    Oracle: the cone's analytic volume pi r^2 h / 3 (r 5, h 10) and BRepCheck, both
    read from the exported .brep by the fixture tool."""

    CONE = math.pi*25*10/3

    def test_apex_cone_is_recorded_under_translation_only_and_valid_under_default(self):
        text = (FIXTURES/'apex-cone.step').read_text()
        with tempfile.TemporaryDirectory() as directory:
            completed, out = self.exported(text, directory, 'translation', mode='translation-only')
            self.assertIn('reference_vertex_loop_faces_invalid\t1\n', completed.stdout)
            self.assertEqual([r[2] for r in records(out, 'reference_vertex_loop_face_invalid')], ['30 39 40'])
            measured = check(out/'definition-15.brep')
            self.assertEqual((measured['valid'], measured['invalid_faces']), (0, 1))
            self.assertGreater(abs(measured['volume']-self.CONE), 80.)
            completed, out = self.exported(text, directory, 'default', mode='default')
            self.assertIn('reference_vertex_loop_faces_invalid\t0\n', completed.stdout)
            self.assertEqual(records(out, 'reference_vertex_loop_face_invalid'), [])
            self.assertEqual([r[2] for r in records(out, 'reference_vertex_loop_default_read')], ['30 39 40'])
            measured = check(out/'definition-15.brep')
            self.assertEqual((measured['valid'], measured['invalid_faces']), (1, 0))
            self.assertAlmostEqual(measured['volume'], self.CONE, places=6)


class OccurrenceStyleTests(unittest.TestCase, _Export):
    """bead .37.23 (styles): the reference format carries per-occurrence styles, row for
    row the STEP source adapter's (both resolve them with native/step_styles.hpp).

    Oracle: the adapter's OCCSTYLE table, itself checked against the STEP text in
    test_step_ingress.ColourTests."""

    def test_occurrence_styles_equal_the_adapters(self):
        from brepgpu.imported_source import load_source
        from brepgpu.reference_source import load_reference_source
        from tests.test_step_ingress import _Adapter, styled_assembly, styled_review
        with tempfile.TemporaryDirectory() as directory:
            for name, text in (('nested', styled_assembly(nested=True)), ('review', styled_review())):
                with self.subTest(name):
                    run, _, target = _Adapter.extract(self, text, directory, name+'-adapter')
                    self.assertEqual(run.returncode, 0, run.stderr)
                    owned = load_source(target)
                    completed, out = self.exported(text, directory, name)
                    exported = load_reference_source(out/'source.bgocct')
                    self.assertTrue(owned.occurrence_styles)
                    self.assertEqual(exported.occurrence_styles, owned.occurrence_styles)
                    self.assertIn(f'reference_occurrence_styles\t{len(owned.occurrence_styles)}\n', completed.stdout)
                    self.assertEqual(records(out, 'reference_occurrence_style_not_carried'), [])

    def test_reader_refuses_a_broken_occurrence_style(self):
        import dataclasses
        from brepgpu import reference_source
        from brepgpu.imported_source import OccurrenceStyle
        from tests.test_step_ingress import styled_assembly
        with tempfile.TemporaryDirectory() as directory:
            _, out = self.exported(styled_assembly(nested=True), directory, 'nested')
            source = reference_source.load_reference_source(out/'source.bgocct')
            for name, row, message in (('not from the root', OccurrenceStyle(670, 0, (724,)), 'not connected from the root'),
                                       ('unknown style', OccurrenceStyle(999, 0, (386,)), 'unresolved reference occurrence style'),
                                       ('face elsewhere', OccurrenceStyle(670, 400, (386,)), 'is not on its occurrence'),
                                       ('empty', OccurrenceStyle(670, 0, ()), 'path is empty'),
                                       ('duplicate', source.occurrence_styles[0], 'duplicate reference occurrence style')):
                with self.subTest(name), self.assertRaisesRegex(ValueError, message):
                    bad = dataclasses.replace(source, occurrence_styles=source.occurrence_styles+(row,)
                                              if name == 'duplicate' else (row,))
                    reference_source._validate(bad, out)


def rotated_origin():
    text = substituted(MAPPED_TEXT(), '#902 = REPRESENTATION_MAP(#11,#10);',
                       "#902 = REPRESENTATION_MAP(#910,#10);\n#910 = AXIS2_PLACEMENT_3D('',#911,#907,#912);\n"
                       "#911 = CARTESIAN_POINT('',(1.,0.,0.));\n#912 = DIRECTION('',(0.,1.,0.));")
    return substituted(text, "#10 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#15),#345);",
                       "#10 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#15,#910),#345);")


# The B2-harness fixture's box re-placed through MAPPED_ITEM #901 (map #902: origin
# #11, the identity, onto target #903 at (0,5,0)).
MAPPED = FIXTURES/'mapped-item-solid.step'


def MAPPED_TEXT():
    return MAPPED.read_text()



ROTATED_ORIGIN = rotated_origin()


class MappedItemTests(unittest.TestCase, _Export):
    """bead .37.25: a solid placed through a MAPPED_ITEM is admitted.

    The mapped item is an occurrence: the holder product is an assembly node, the
    mapped representation #10 a part owning solid #15, the mapped item usage #901
    whose placement is read raw as target o origin^-1 (ISO 10303-43; the PDM Usage
    Guide's C = B A^-1) and must agree with OCCT's located solid.

    Oracle: the placement derived by hand from the authored frames."""

    def placed(self, text, directory, name, mode=None):
        from brepgpu.reference_source import load_reference_source
        from brepgpu.imported_assembly import assemble
        _, out = self.exported(text, directory, name, mode=mode)
        source = load_reference_source(out/'source.bgocct')
        return source, assemble(source)

    def test_mapped_solid_is_an_occurrence_placed_raw(self):
        with tempfile.TemporaryDirectory() as directory:
            for mode in ('translation-only', 'default'):
                with self.subTest(mode=mode):
                    source, assembly = self.placed(MAPPED.read_text(), directory, mode, mode)
                    self.assertEqual({p.id: p.definition_id for p in source.products}, {5: 0, 10: 15})
                    self.assertEqual(source.product(10).name, source.product(5).name)
                    occurrence, = assembly.occurrences
                    self.assertEqual((occurrence.path, occurrence.product_id, occurrence.definition_id), ((901,), 10, 15))
                    self.assertEqual(occurrence.transform, (1., 0., 0., 0., 0., 1., 0., 5., 0., 0., 1., 0.))
                    # bead .37.34: the oracle reads the mapped occurrence from the STEP text.
                    from brepgpu.imported_oracles import qualify_original_reference
                    self.assertLessEqual(qualify_original_reference(source, Path(directory)/f'{mode}.step'), 2.**-32)
                    self.assertIn('raw-entity-corrections=none', [t.strip() for t in source.adapter_revision.split(';')])

    def test_translated_origin_composes_target_after_origin_inverse(self):
        """Origin moved to (1,0,0): target o origin^-1 maps P to P + (-1,5,0); OCCT's
        origin^-1 o target agrees because the frames only translate."""
        text = substituted(MAPPED.read_text(), '#902 = REPRESENTATION_MAP(#11,#10);',
                           "#902 = REPRESENTATION_MAP(#910,#10);\n#910 = AXIS2_PLACEMENT_3D('',#911,#907,#908);\n"
                           "#911 = CARTESIAN_POINT('',(1.,0.,0.));")
        text = substituted(text, '#10 = ADVANCED_BREP_SHAPE_REPRESENTATION(\'\',(#11,#15),#345);',
                           "#10 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#15,#910),#345);")
        with tempfile.TemporaryDirectory() as directory:
            _, assembly = self.placed(text, directory, 'translated')
            self.assertEqual(assembly.occurrences[0].transform, (1., 0., 0., -1., 0., 1., 0., 5., 0., 0., 1., 0.))

    def test_rotated_origin_where_occt_misreads_is_a_declared_correction(self):
        """bead .37.33: origin turned a quarter about z at (1,0,0). The standard reads
        P -> Rz(-90)(P-(1,0,0)) + (0,5,0), translation (0,6,0); OCCT
        (STEPControl_ActorRead.cxx:2488) reads Rz(-90)(P+(0,5,0)-(1,0,0)), translation
        (5,1,0). The raw placement is written with a record and the export declares
        raw-entity-corrections=P2 (it refused in round 1)."""
        from brepgpu.imported_oracles import qualify_original_reference
        with tempfile.TemporaryDirectory() as directory:
            source, assembly = self.placed(ROTATED_ORIGIN, directory, 'rotated')
            self.assertEqual(assembly.occurrences[0].transform, (0., 1., 0., 0., -1., 0., 0., 6., 0., 0., 1., 0.))
            self.assertIn('raw-entity-corrections=P2', [t.strip() for t in source.adapter_revision.split(';')])
            self.assertEqual([r[2] for r in records(Path(directory)/'rotated', 'reference_placement_raw')], ['901 5 10'])
            step = Path(directory)/'rotated.step'
            self.assertLessEqual(qualify_original_reference(source, step), 2.**-32)

    def test_unread_mapped_item_forms_refuse_by_name(self):
        base = MAPPED.read_text()
        operator = substituted(base, '#901 = MAPPED_ITEM(\'\',#902,#903);',
                               "#901 = MAPPED_ITEM('',#902,#920);\n"
                               "#920 = CARTESIAN_TRANSFORMATION_OPERATOR_3D('','','',$,$,#904,$,$);")
        second = substituted(base, "#900 = SHAPE_REPRESENTATION('',(#901,#906),#345);",
                             "#900 = SHAPE_REPRESENTATION('',(#901,#906),#345);\n"
                             "#930 = SHAPE_REPRESENTATION('',(#931,#906),#345);\n#931 = MAPPED_ITEM('',#932,#906);\n"
                             "#932 = REPRESENTATION_MAP(#11,#10);")
        nested = substituted(second, "#901 = MAPPED_ITEM('',#902,#903);", "#901 = MAPPED_ITEM('',#933,#903);\n"
                             "#933 = REPRESENTATION_MAP(#906,#930);")
        with tempfile.TemporaryDirectory() as directory:
            for name, text, message in (('operator', operator, 'only axis2_placement_3d pairs are read raw'),
                                        ('nested', nested, 'nested mapped items are not read')):
                with self.subTest(name):
                    completed, _ = self.exported(text, directory, name, success=False)
                    self.assertIn(message, completed.stderr)


class RawPlacementTests(unittest.TestCase, _Export):
    """bead .37.33: usage placements are read raw through native/step_placements.hpp. OCCT's
    value is written where it agrees to the adapter's grade (2^-32 relative, at least 1), so
    the arm declares no correction; elsewhere the raw value is written with a record and the
    adapter's letter is declared (P1 cartesian operator, P2 item-defined transformation).

    Oracles: the hand-derived Part 42 and PDM-guide matrices of the adapter's .37.20 fixtures
    (tests/test_placement_oracles.py), and qualify_original_reference (bead .37.34)."""

    def reference(self, text, directory, name, mode='translation-only'):
        from brepgpu.reference_source import load_reference_source
        completed, out = self.exported(text, directory, name, mode=mode)
        return completed, out, load_reference_source(out/'source.bgocct')

    def corrections(self, source):
        value, = [t.strip().partition('=')[2] for t in source.adapter_revision.split(';')
                  if t.strip().startswith('raw-entity-corrections=')]
        return value

    def usage(self, source, id):
        return next(u for u in source.usages if u.id == id).transform

    def test_agreeing_placements_write_occts_value_and_declare_nothing(self):
        from brepgpu.imported_oracles import qualify_original_reference
        with tempfile.TemporaryDirectory() as directory:
            for mode in ('translation-only', 'default'):
                with self.subTest(mode=mode):
                    _, out, source = self.reference((FIXTURES/'source-uv.step').read_text(), directory, mode, mode)
                    self.assertEqual(self.corrections(source), 'none')
                    self.assertEqual(records(out, 'reference_placement_raw'), [])
                    self.assertLessEqual(qualify_original_reference(source, Path(directory)/f'{mode}.step'), 2.**-32)

    def test_adapter_fixtures_are_read_raw_and_declared(self):
        from brepgpu.imported_oracles import qualify_original_reference
        from tests.test_placement_oracles import CASES, IDT, PDM_ITEMS, PDM_MATRIX, ROTATED_BOX, edited
        cases = {'omitted-axis3': (edited((IDT, CASES['omitted-axis3'][0])), CASES['omitted-axis3'][1], 2.**-52, 'P1'),
                 'rotated-item-1': (edited(*PDM_ITEMS), PDM_MATRIX, 1e-6, 'P2')}
        with tempfile.TemporaryDirectory() as directory:
            for name, (text, expected, tolerance, letter) in cases.items():
                for mode in ('translation-only', 'default'):
                    with self.subTest(name=name, mode=mode):
                        completed, out, source = self.reference(text, directory, f'{name}-{mode}', mode)
                        for a, b in zip(self.usage(source, ROTATED_BOX), expected):
                            self.assertLessEqual(abs(a-b), tolerance, self.usage(source, ROTATED_BOX))
                        self.assertEqual(self.corrections(source), letter)
                        rows = records(out, 'reference_placement_raw')
                        self.assertEqual([r[2].split()[0] for r in rows], [str(ROTATED_BOX)])
                        self.assertIn(f'reference_placement_raw\t{ROTATED_BOX}\t', completed.stdout)
                        self.assertLessEqual(qualify_original_reference(source, Path(directory)/f'{name}-{mode}.step'), 2.**-32)

    def turned(self, item_1, item_2, rep=40, holder_items="(#11,#41)", context=371, relation=("#384", "#19"), dx=0.):
        """source-uv.step with usage relation item_1 a frame turned by item_1 about z and
        item_2 the given frame: the adapter's review round 2 placements (tests/test_step_reader_raw.py)."""
        from tests.test_step_reader_raw import moved
        text = (FIXTURES/'source-uv.step').read_text()
        if dx:
            text = moved(text, dx, first=41, last=370)
        text = substituted(text, f"#{rep} = ADVANCED_BREP_SHAPE_REPRESENTATION('',{holder_items},#{context});",
                           f"#{rep} = ADVANCED_BREP_SHAPE_REPRESENTATION('',{holder_items[:-1]},#703),#{context});")
        text = substituted(text, "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23),#27);",
                           "#10 = SHAPE_REPRESENTATION('',(#11,#15,#19,#23,#707),#27);")
        name, item = relation
        return substituted(text, f"{name} = ITEM_DEFINED_TRANSFORMATION('','',#11,{item});",
                           f"{name} = ITEM_DEFINED_TRANSFORMATION('','',#703,#707);\n#700 = CARTESIAN_POINT('',(0.,0.,0.));\n"
                           f"#701 = DIRECTION('',(0.,0.,1.));\n#702 = DIRECTION('',(1.,{item_1},0.));\n#703 = AXIS2_PLACEMENT_3D('',#700,#701,#702);\n"
                           + item_2)

    def test_the_shared_agreement_predicate_decides_usages(self):
        """Review round 1 item 8: the exporter calls native/step_placements.hpp agree, as
        the adapter does, on the adapter's three round 2 fixtures. (a) item_1 turned 1e-10
        rad, item_2 5000 mm out: translations differ by 5e-7 mm, inside 2^-32 relative but
        above the 1e-7 mm uncertainty; (b) item_1 turned 1.5e-10 rad, item_2 along y, the
        box 5000 mm out within its representation: the rotation moves its points by 7.5e-7
        mm. Both are written raw and declared P2. (c) the same turn placing the cylinder
        part, whose edges carry no pcurves in the translation-only read: over a sound
        extent the move is under 1e-9 mm and OCCT's value is written, no record; boxing
        OCCT's translation-only shape would span the unbounded cylinder and write it raw.
        Oracle: the adapter's verdicts on the same files."""
        y_axis = "#705 = DIRECTION('',(0.,1.,0.));\n#706 = DIRECTION('',(1.,0.,0.));\n#707 = AXIS2_PLACEMENT_3D('',#700,#705,#706);"
        far = ("#704 = CARTESIAN_POINT('',(5000.,5000.,0.));\n#706 = DIRECTION('',(1.,0.,0.));\n"
               "#707 = AXIS2_PLACEMENT_3D('',#704,#701,#706);")
        cases = (('translation', self.turned('1.E-10', far), 386, 'translation'),
                 ('far-rotation', self.turned('1.5E-10', y_axis, dx=5000.), 386, 'rotation'),
                 ('near-cylinder', self.turned('1.5E-10', y_axis, rep=394, holder_items='(#11,#395)', context=493,
                                               relation=('#500', '#23')), 502, None))
        with tempfile.TemporaryDirectory() as directory:
            for name, text, usage, raw in cases:
                for mode in ('translation-only', 'default'):
                    with self.subTest(name=name, mode=mode):
                        completed, out, source = self.reference(text, directory, f'{name}-{mode}', mode)
                        rows = records(out, 'reference_placement_raw')
                        if raw is None:
                            self.assertEqual(rows, [])
                            self.assertIn('reference_placements_raw\t0\n', completed.stdout)
                            self.assertEqual(self.corrections(source), 'none')
                            continue
                        self.assertEqual([r[2].split()[0] for r in rows], [str(usage)])
                        self.assertEqual(self.corrections(source), 'P2')
                        line, = [l.split('\t') for l in completed.stdout.splitlines() if l.startswith('reference_placement_raw\t')]
                        relative, translation, moved, resolution = map(float, line[2:])
                        self.assertLessEqual(relative, 2.**-32)
                        self.assertEqual(resolution, 1e-7)
                        if raw == 'translation':
                            self.assertGreater(translation, resolution)
                            self.assertEqual(self.usage(source, usage)[3], 5000.)
                        else:
                            self.assertLessEqual(translation, resolution)
                            self.assertGreater(moved, resolution)

    def test_the_shared_agreement_predicate_decides_mapped_items(self):
        """Review round 1 item 8, mapped items: the .37.25 fixture with origin turned 1.5e-10
        rad about z and target a frame along y, both at the origin, and the solid moved
        5000 mm out within its representation. OCCT's origin^-1 o target and the raw
        target o origin^-1 differ by 1.5e-10 in rotation entries (inside 2^-32) and move the
        solid's points by about 7.5e-7 mm, above the 1e-7 mm uncertainty: the raw value is
        written and declared P2. Without the move the same turn agrees and declares nothing."""
        from tests.test_step_reader_raw import moved
        def turned(dx):
            text = moved(MAPPED.read_text(), dx, first=13, last=344) if dx else MAPPED.read_text()
            text = substituted(text, '#902 = REPRESENTATION_MAP(#11,#10);',
                               "#902 = REPRESENTATION_MAP(#910,#10);\n#910 = AXIS2_PLACEMENT_3D('',#909,#907,#912);\n"
                               "#912 = DIRECTION('',(1.,1.5E-10,0.));")
            text = substituted(text, "#10 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#15),#345);",
                               "#10 = ADVANCED_BREP_SHAPE_REPRESENTATION('',(#11,#15,#910),#345);")
            return substituted(text, "#903 = AXIS2_PLACEMENT_3D('',#904,#907,#908);",
                               "#903 = AXIS2_PLACEMENT_3D('',#909,#913,#908);\n#913 = DIRECTION('',(0.,1.,0.));")
        with tempfile.TemporaryDirectory() as directory:
            for name, dx, expected in (('far', 5000., 'P2'), ('near', 0., 'none')):
                for mode in ('translation-only', 'default'):
                    with self.subTest(name=name, mode=mode):
                        completed, out, source = self.reference(turned(dx), directory, f'{name}-{mode}', mode)
                        self.assertEqual(self.corrections(source), expected)
                        rows = records(out, 'reference_placement_raw')
                        self.assertEqual([r[2] for r in rows], ['901 5 10'] if dx else [])
                        if dx:
                            line, = [l.split('\t') for l in completed.stdout.splitlines() if l.startswith('reference_placement_raw\t')]
                            relative, translation, moved_mm, resolution = map(float, line[2:])
                            self.assertLessEqual(relative, 2.**-32)
                            self.assertLessEqual(translation, resolution)
                            self.assertGreater(moved_mm, resolution)

    def test_the_placement_extent_holds_the_apex_cone(self):
        """Review round 2, item 4: the extent passed to the agreement predicate is
        native/shape_extent.hpp shape_bound of OCCT's translation. apex-cone.step's cone
        is bounded by its base circle alone, its apex 10 mm above: OCCT's box of the
        pcurve-completed copy (round 1's extent) spans the base only and misses the apex
        by 10 mm; shape_bound holds every point. Oracle: the processed solid's optimal
        box and samples along the raw edges (the review's diag/extent check)."""
        out = subprocess.run((str(inspector()), '--extent', str(FIXTURES/'apex-cone.step')),
                             capture_output=True, text=True, check=True).stdout
        row, = [line.split() for line in out.splitlines() if line.startswith('solid ')]
        values = dict(zip(row[2::2], map(float, row[3::2])))
        self.assertLessEqual(values['bound_miss_mm'], 1e-9)
        self.assertEqual(values['bound_whole'], 0)
        self.assertAlmostEqual(values['copy_miss_mm'], 10., places=6)

    def test_untransformed_placing_relation_refuses_as_the_adapter_does(self):
        from tests.test_placement_oracles import IDENTITY_RELATION, edited
        with tempfile.TemporaryDirectory() as directory:
            completed, _ = self.exported(edited(*IDENTITY_RELATION), directory, 'untransformed', success=False)
            self.assertIn('usage 380: placing relation 377 carries no transformation', completed.stderr)

    def test_byte_check_follows_the_declaration(self):
        """Placement corrections (P1, P2) change usages, not .brep bytes, so the
        translation-only byte check applies to every definition of such an export
        (B2-reader-export review round 1)."""
        import sys
        sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'examples'))
        from render_om10_nvidia import check_nvidia_input
        from tests.test_placement_oracles import CASES, IDT, edited
        ingress = Path(os.environ.get('BREPGPU_NVIDIA_INGRESS', 'build/nvidia-ingress/brepgpu-nvidia-ingress')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            for name, text, verdict in (('plain', (FIXTURES/'source-uv.step').read_text(), 'applied'),
                                        ('corrected', edited((IDT, CASES['omitted-axis3'][0])), 'applied')):
                with self.subTest(name):
                    _, out, source = self.reference(text, directory, name)
                    subprocess.run((str(ingress), '--shape-processing', 'none', str(directory/f'{name}.step'),
                                    str(directory/f'{name}-nvidia')), check=True, capture_output=True)
                    self.assertEqual(check_nvidia_input(directory/f'{name}-nvidia', 'occt-no-shape-process', source,
                                                        out, ingress), (2, verdict, set()))


class FaceSideCorrectionTests(unittest.TestCase, _Export):
    """bead .37.36 (decision): on a face whose raw use points into the material, by the
    exporter's own ray-parity verdict (native/reference_source/orientation_verdict.hpp),
    the translation-only export writes the outward side into the exported shape and
    declares S1 (one face against its own bound) or S2 (a whole shell inside out), with a
    record; the default read writes OCCT's repaired face and the same side in its face row.
    Faces whose raw use agrees with the shell are unchanged.

    Fixtures: the adapter's OrientationTests blocks (tests/test_step_reader_raw.py).
    Oracles: the cube's volume 1000 and BRepCheck from the exported .brep; the adapter's
    face table (written sides) and records for the same files."""

    def test_s1_and_s2_are_corrected_as_the_adapter_corrects_them(self):
        from brepgpu.imported_source import load_source
        from brepgpu.reference_source import load_reference_source
        from tests.test_step_ingress import _Adapter, plain_block
        from tests.test_step_reader_raw import BLOCK_FACES, flipped_block
        every = [face for face, _ in BLOCK_FACES]
        cases = {'plain': (plain_block(), set(), 'none'),
                 's1': (flipped_block(same_sense=(57,)), {57}, 'S1'),
                 's2': (flipped_block(same_sense=every, bounds=every), set(every), 'S2')}
        with tempfile.TemporaryDirectory() as directory:
            for name, (text, inward, letter) in cases.items():
                run, _, target = _Adapter.extract(self, text, directory, name+'-adapter')
                self.assertEqual(run.returncode, 0, run.stderr)
                owned = load_source(target)
                self.assertEqual({d.source_ids[1] for d in owned.diagnostics if d.stage == 'source_face_orientation_raw'}, inward)
                for mode in ('translation-only', 'default'):
                    with self.subTest(name=name, mode=mode):
                        completed, out = self.exported(text, directory, f'{name}-{mode}', mode=mode)
                        source = load_reference_source(out/'source.bgocct')
                        stage = 'reference_face_orientation_raw' if mode == 'translation-only' else 'default_read_face_repaired'
                        self.assertEqual({int(r[2].split()[1]) for r in records(out, stage)}, inward)
                        declared, = [t.strip().partition('=')[2] for t in source.adapter_revision.split(';')
                                     if t.strip().startswith('raw-entity-corrections=')]
                        self.assertEqual(declared, letter if mode == 'translation-only' else 'none')
                        # The written sides are the adapter's, face for face.
                        self.assertEqual({f.id: f.orientation for f in source.faces}, {f.id: f.orientation for f in owned.faces})
                        measured = check(out/'definition-15.brep')
                        self.assertEqual(measured['valid'], 1)
                        self.assertAlmostEqual(measured['volume'], 1000., places=9)

    def test_corrected_definition_is_out_of_the_byte_check(self):
        import sys
        sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'examples'))
        from render_om10_nvidia import check_nvidia_input
        from brepgpu.reference_source import load_reference_source
        from tests.test_step_reader_raw import flipped_block
        ingress = Path(os.environ.get('BREPGPU_NVIDIA_INGRESS', 'build/nvidia-ingress/brepgpu-nvidia-ingress')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            _, out = self.exported(flipped_block(same_sense=(57,)), directory, 's1', mode='translation-only')
            subprocess.run((str(ingress), '--shape-processing', 'none', str(directory/'s1.step'), str(directory/'nvidia')),
                           check=True, capture_output=True)
            source = load_reference_source(out/'source.bgocct')
            # Per definition (B2-reader-export review round 1): the corrected definition takes our .brep.
            self.assertEqual(check_nvidia_input(directory/'nvidia', 'occt-no-shape-process', source, out, ingress),
                             (0, 'applied per definition; corrected definitions [15] take our .brep', {15}))


class FaceSideReviewTests(unittest.TestCase, _Export):
    """B2-reader-export review round 1. S1 or S2 is decided per face from its own bound,
    never by counting faces; every correction is verified on the written solid (BRepCheck
    valid, signed volumes) before it is published. Oracles: the plain block's sides and
    volumes (1000, 936), BRepCheck, read from the exported files."""

    def sides(self, out):
        from brepgpu.reference_source import load_reference_source
        return {f.id: f.orientation for f in load_reference_source(out/'source.bgocct').faces}

    def test_every_face_against_its_bound_is_s1_not_s2(self):
        from tests.test_step_reader_raw import BLOCK_FACES, flipped_block
        from tests.test_step_ingress import plain_block
        every = [face for face, _ in BLOCK_FACES]
        with tempfile.TemporaryDirectory() as directory:
            _, plain = self.exported(plain_block(), directory, 'plain', mode='translation-only')
            for name, text, inward in (('all six same_sense', flipped_block(same_sense=every), every),
                                       ('five same_sense', flipped_block(same_sense=every[:5]), every[:5])):
                for mode in ('translation-only', 'default'):
                    with self.subTest(name=name, mode=mode):
                        completed, out = self.exported(text, directory, f'{name}-{mode}'.replace(' ', '-'), mode=mode)
                        stage = 'reference_face_orientation_corrected' if mode == 'translation-only' else 'default_read_face_repaired'
                        lines = [l.split('\t') for l in completed.stdout.splitlines() if l.startswith(stage+'\t')]
                        self.assertEqual(sorted((int(l[2]), l[3]) for l in lines), sorted((f, 'S1') for f in inward))
                        self.assertEqual(self.sides(out), self.sides(plain))
                        measured = check(out/'definition-15.brep')
                        self.assertEqual(measured['valid'], 1)
                        self.assertAlmostEqual(measured['volume'], 1000., places=9)

    def test_void_faces_against_their_bounds_are_s1(self):
        import re
        base = (FIXTURES/'void-block.step').read_text()
        void_faces = re.search(r"#166 = CLOSED_SHELL\('',\(([^)]*)\)\);", base).group(1).replace('#', '').split(',')
        def flipped(text, faces):
            for f in faces:
                m = re.search(rf"#{f} = ADVANCED_FACE\('',\((#\d+)\),(#\d+),(\.[TF]\.)\);", text)
                other = '.T.' if m.group(3) == '.F.' else '.F.'
                text = text.replace(m.group(0), f"#{f} = ADVANCED_FACE('',({m.group(1)}),{m.group(2)},{other});")
            return text
        with tempfile.TemporaryDirectory() as directory:
            _, plain = self.exported(base, directory, 'plain', mode='translation-only')
            for name, faces in (('one', void_faces[:1]), ('all', void_faces)):
                for mode in ('translation-only', 'default'):
                    with self.subTest(name=name, mode=mode):
                        completed, out = self.exported(flipped(base, faces), directory, f'{name}-{mode}', mode=mode)
                        self.assertEqual(self.sides(out), self.sides(plain))
                        measured = check(out/'definition-15.brep')
                        self.assertEqual((measured['valid'], measured['shells']), (1, 2))
                        self.assertAlmostEqual(measured['volume'], 936., places=9)

    def seams(self, path, completed=False):
        """The fixture tool's seam uses of one .brep: traversal direction and relative u
        (completed: on the verification copy, which adds a missing seam's pcurves)."""
        out = subprocess.run((str(inspector()), '--seams', *(('--completed',) if completed else ()), str(path)),
                             capture_output=True, text=True, check=True).stdout
        return sorted(tuple(line.split()[2:]) for line in out.splitlines() if line.startswith('seam '))

    def test_a_seam_face_against_its_bound_is_s1_with_each_seam_use_on_its_own_pcurve(self):
        """Review round 2, item 1. The cylinder part (#395) with its lateral face #397's
        same_sense flipped: the raw use points into the material and its bound (two
        circles and the seam) runs against it. Three files: reference-source.step, every
        pcurve present; the same with the circles' edge curves pointing at the circles,
        so they carry no pcurve while the seam keeps its two; and source-uv.step, where
        the seam carries none either (compared on the verification copy, which adds it). OCCT's translation keeps the loop's
        traversal and turns the face; reversing the wires inside an S1 face moves each seam
        use to the seam's other pcurve. The S1 face must keep each seam use on its own
        pcurve: its seam uses (3D traversal direction, pcurve u) are the ones OCCT's
        default read gives the repaired face. The published bytes are read back and verify
        on a copy that only adds missing pcurves, with the cylinder's volume pi 0.75^2 2;
        the adapter corrects #397 alone; and the uncorrected translation written beside the
        export is byte-identical to the minimal NVIDIA ingress's. Oracles: OCCT's default
        read, the adapter's record, the volume, the minimal ingress's bytes."""
        from tests.test_step_ingress import _Adapter
        from brepgpu.imported_source import load_source
        ingress = Path(os.environ.get('BREPGPU_NVIDIA_INGRESS', 'build/nvidia-ingress/brepgpu-nvidia-ingress')).resolve()
        every = (FIXTURES/'reference-source.step').read_text()
        circles = substituted(substituted(every, "#401 = EDGE_CURVE('',#402,#402,#404,.T.);",
                                          "#401 = EDGE_CURVE('',#402,#402,#405,.T.);"),
                              "#458 = EDGE_CURVE('',#436,#436,#459,.T.);", "#458 = EDGE_CURVE('',#436,#436,#460,.T.);")
        # source-uv.step: the circles and the seam point at their 3D curves; no pcurve on #397.
        for fixture, base, stripped_seam in (('every pcurve', every, False), ('circles stripped', circles, False),
                                             ('all stripped', (FIXTURES/'source-uv.step').read_text(), True)):
            text = substituted(base, "#397 = ADVANCED_FACE('',(#398),#411,.T.);", "#397 = ADVANCED_FACE('',(#398),#411,.F.);")
            with self.subTest(fixture=fixture), tempfile.TemporaryDirectory() as directory:
                run, _, target = _Adapter().extract(text, directory, 'adapter')
                self.assertEqual(run.returncode, 0, run.stderr)
                adapter = {d.source_ids[1] for d in load_source(target).diagnostics if d.stage == 'source_face_orientation_raw'}
                self.assertEqual(adapter, {397})
                seams = {}
                for mode in ('translation-only', 'default'):
                    completed, out = self.exported(text, directory, mode, mode=mode)
                    stage = 'reference_face_orientation_corrected' if mode == 'translation-only' else 'default_read_face_repaired'
                    lines = [l.split('\t') for l in completed.stdout.splitlines() if l.startswith(stage+'\t')]
                    self.assertEqual([(int(l[2]), l[3]) for l in lines], [(397, 'S1')], mode)
                    verified = subprocess.run((str(inspector()), '--verify', str(out/'definition-395.brep')),
                                              capture_output=True, text=True, check=True).stdout
                    self.assertIn('verify ok\n', verified, mode)
                    self.assertAlmostEqual(float(verified.split('completed_volume ')[1]), math.pi*0.75**2*2, places=6)
                    seams[mode] = self.seams(out/'definition-395.brep', completed=stripped_seam and mode == 'translation-only')
                self.assertEqual(len(seams['default']), 2)
                self.assertEqual(seams['translation-only'], seams['default'])
                subprocess.run((str(ingress), '--shape-processing', 'none', str(Path(directory)/'translation-only.step'),
                                str(Path(directory)/'minimal')), check=True, capture_output=True)
                ours = Path(directory)/'translation-only'/'uncorrected'/'definition-395.brep'
                self.assertEqual(ours.read_bytes(), (Path(directory)/'minimal'/'definition-395.brep').read_bytes())
                self.assertFalse((Path(directory)/'translation-only'/'uncorrected'/'definition-41.brep').exists())

    def test_the_verification_copy_raises_tolerances_only_where_it_added_pcurves(self):
        """Coordinator ruling after review round 2: the verification copy may raise an
        edge's tolerance, only on an edge that carries a pcurve it added and only to the
        measured deviation of those pcurves; every other edge keeps OCCT's tolerance, and
        verification.tsv carries the largest tolerance used. The cylinder part with every
        pcurve present (nothing added, nothing raised), and the same with the circles'
        edge curves pointing at the circles, so the circles get added pcurves while the
        seam, which keeps its two, must keep OCCT's tolerance exactly. Oracle: OCCT's
        tolerances read from the published file."""
        def tolerances(path):
            out = subprocess.run((str(inspector()), '--tolerances', str(path)), capture_output=True, text=True,
                                 check=True).stdout.split()
            return dict(zip(out[0::2], map(float, out[1::2])))
        every = (FIXTURES/'reference-source.step').read_text()
        circles = substituted(substituted(every, "#401 = EDGE_CURVE('',#402,#402,#404,.T.);",
                                          "#401 = EDGE_CURVE('',#402,#402,#405,.T.);"),
                              "#458 = EDGE_CURVE('',#436,#436,#459,.T.);", "#458 = EDGE_CURVE('',#436,#436,#460,.T.);")
        with tempfile.TemporaryDirectory() as directory:
            for name, base in (('every pcurve', every), ('circles stripped', circles)):
                with self.subTest(name):
                    text = substituted(base, "#397 = ADVANCED_FACE('',(#398),#411,.T.);",
                                       "#397 = ADVANCED_FACE('',(#398),#411,.F.);")
                    _, out = self.exported(text, directory, name.replace(' ', '-'), mode='translation-only')
                    values = tolerances(out/'definition-395.brep')
                    self.assertGreater(values['edges_without_added'], 0)
                    self.assertEqual(values['kept'], values['edges_without_added'])
                    self.assertEqual(values['lowered'], 0)
                    row, = [r for r in (line.split('\t') for line in (out/'verification.tsv').read_text().splitlines()[1:])]
                    self.assertEqual(row[:3], ['395', 'translation-only', '1'])
                    self.assertEqual(float(row[3]), values['largest_raised_tolerance_mm'])
                    if name == 'every pcurve':
                        self.assertEqual((values['edges_with_added'], values['largest_raised_tolerance_mm']), (0, 0))
                    else:
                        self.assertEqual(values['edges_with_added'], 2)

    def test_a_raised_tolerance_is_capped_at_the_declared_uncertainty(self):
        """Review round 3, item 2 (attacks/tol): source-uv.step's cylinder part with the
        seam face #397 turned (S1) and the cylinder's radius 0.8 or 0.76 instead of 0.75,
        so its circle edges sit 0.05 or 0.01 mm off the surface. The verification copy
        projects the circles' missing pcurves, which then deviate from the 3D circles by
        that much. The raise is capped at the definition's usable declared uncertainty
        (1e-7 mm here): above it, verification fails by name and translation-only
        refuses. At 0.75 nothing deviates and the export verifies. verification.tsv
        carries the deviation and the cap. Oracle: the authored radii."""
        base = substituted((FIXTURES/'source-uv.step').read_text(), "#397 = ADVANCED_FACE('',(#398),#411,.T.);",
                           "#397 = ADVANCED_FACE('',(#398),#411,.F.);")
        with tempfile.TemporaryDirectory() as directory:
            for radius, off in (('0.8', .05), ('0.76', .01), ('0.75', 0.)):
                with self.subTest(radius=radius):
                    text = substituted(base, "#411 = CYLINDRICAL_SURFACE('',#412,0.75);",
                                       f"#411 = CYLINDRICAL_SURFACE('',#412,{radius});")
                    completed, out = self.exported(text, directory, f'r{radius}', mode='translation-only', success=off == 0)
                    if off:
                        self.assertIn('the corrected face sides fail verification (an added pcurve deviates', completed.stderr)
                        self.assertIn('above the cap 1e-07 mm', completed.stderr)
                        continue
                    row, = [r.split('\t') for r in (out/'verification.tsv').read_text().splitlines()[1:]]
                    self.assertEqual(row[:3], ['395', 'translation-only', '1'])
                    self.assertLessEqual(float(row[4]), float(row[5]))
                    self.assertEqual(float(row[5]), 1e-7)

    def test_an_inside_out_shell_with_a_seam_face_and_no_pcurves_is_s2(self):
        """Review round 2, item 2 (attack a6, 'stripped | s2 all'). The cylinder part of
        source-uv.step, whose lateral face and seam carry no pcurve, with every face's
        same_sense and bound flipped: the whole shell is inside out. The seam face is
        classified on a completed copy, so it is decided (it agreed with its bound
        nowhere before, and its caps then refused as 'not inside out as a whole'). Both
        modes give S2 for all three faces, as the adapter does, and the published solid
        verifies with the cylinder's volume. Oracle: the adapter, the volume."""
        import re
        from tests.test_step_ingress import _Adapter
        from brepgpu.imported_source import load_source
        text = (FIXTURES/'source-uv.step').read_text()
        for face, bound in ((397, 398), (485, 486), (489, 490)):
            text = re.sub(rf"(#{face} = ADVANCED_FACE\('',\(#\d+\),#\d+,)(\.[TF]\.)",
                          lambda m: m.group(1)+('.F.' if m.group(2) == '.T.' else '.T.'), text)
            text = re.sub(rf"(#{bound} = FACE_BOUND\('',#\d+,)(\.[TF]\.)",
                          lambda m: m.group(1)+('.F.' if m.group(2) == '.T.' else '.T.'), text)
        with tempfile.TemporaryDirectory() as directory:
            run, _, target = _Adapter().extract(text, directory, 'adapter')
            self.assertEqual(run.returncode, 0, run.stderr)
            adapter = sorted(d.source_ids[1] for d in load_source(target).diagnostics if d.stage == 'source_face_orientation_raw')
            self.assertEqual(adapter, [397, 485, 489])
            for mode in ('translation-only', 'default'):
                with self.subTest(mode=mode):
                    completed, out = self.exported(text, directory, mode, mode=mode)
                    stage = 'reference_face_orientation_corrected' if mode == 'translation-only' else 'default_read_face_repaired'
                    lines = [l.split('\t') for l in completed.stdout.splitlines() if l.startswith(stage+'\t')]
                    self.assertEqual(sorted((int(l[2]), l[3]) for l in lines), [(397, 'S2'), (485, 'S2'), (489, 'S2')])
                    verified = subprocess.run((str(inspector()), '--verify', str(out/'definition-395.brep')),
                                              capture_output=True, text=True, check=True).stdout
                    self.assertIn('verify ok\n', verified)
                    self.assertAlmostEqual(float(verified.split('completed_volume ')[1]), math.pi*0.75**2*2, places=6)

    def test_an_inside_out_torus_with_a_vertex_loop_is_s2_and_verifies(self):
        """The adapter's sole-vertex-loop revolution fixture (tests/test_step_ingress.py):
        a circle of radius 2 at (5,0,0) swept about z closes one face, whose raw use's
        normal, dS/du x dS/dv at (7,0,0), points to -x, into the tube. OCCT's translation
        reads the vertex loop as the period rectangle, every edge a seam. The face is
        the whole shell, inside out: S2 in both modes, and the written or processed
        solid verifies with the torus's volume, 2 pi^2 5 2^2. The verification copy must
        leave a face's existing seam pcurves as they are: re-pairing them turned OCCT's
        valid default read of this face into an invalid one (the review's red state).
        Oracle: the analytic volume and normal."""
        from tests.test_step_ingress import VertexLoopTests
        text = VertexLoopTests().revolution(False)
        with tempfile.TemporaryDirectory() as directory:
            for mode in ('translation-only', 'default'):
                with self.subTest(mode=mode):
                    completed, out = self.exported(text, directory, mode, mode=mode)
                    stage = 'reference_face_orientation_corrected' if mode == 'translation-only' else 'default_read_face_repaired'
                    lines = [l.split('\t') for l in completed.stdout.splitlines() if l.startswith(stage+'\t')]
                    self.assertEqual([(int(l[2]), l[3]) for l in lines], [(30, 'S2')])
                    verified = subprocess.run((str(inspector()), '--verify', str(out/'definition-15.brep')),
                                              capture_output=True, text=True, check=True).stdout
                    self.assertIn('verify ok\n', verified)
                    self.assertAlmostEqual(float(verified.split('completed_volume ')[1]), 2*math.pi**2*5*4, places=6)

    def test_unpublished_mapped_items_and_cycles_refuse_by_name(self):
        """The adapter's .37.26 fixtures: a mapped item in the root's representation of the
        cylinder's representation ("direct"), and one of an empty representation joined to
        the box's by a plain relationship ("related"), refuse by name; the mapped-item
        cycle (styled_mapped with its plain usage) refuses before any transfer, including
        in --face-sides, instead of crashing OCCT."""
        from tests.test_step_ingress import styled_mapped
        from tests.test_step_reader_raw import with_root_items
        related = with_root_items('#962', records=(
            "#960 = SHAPE_REPRESENTATION('',(#11),#27);\n#961 = REPRESENTATION_MAP(#11,#960);\n"
            "#962 = MAPPED_ITEM('',#961,#11);\n#963 = SHAPE_REPRESENTATION_RELATIONSHIP('','',#960,#40);\n"))
        cases = (('direct', styled_mapped(plain=False), 'mapped item #795 in representation #10 places a solid'),
                 ('related', related, 'mapped item #962 in representation #10 places a solid'),
                 ('cycle', styled_mapped(), 'would recurse without bound'))
        with tempfile.TemporaryDirectory() as directory:
            for name, text, message in cases:
                for mode in ('translation-only', 'default'):
                    with self.subTest(name=name, mode=mode):
                        completed, _ = self.exported(text, directory, f'{name}-{mode}', mode=mode, success=False)
                        self.assertIn(message, completed.stderr)
                        self.assertNotEqual(completed.returncode, -11)
            sides = subprocess.run((str(executable()), '--face-sides', str(Path(directory)/'cycle-default.step'), '43'),
                                   capture_output=True, text=True)
            self.assertEqual(sides.returncode, 1)
            self.assertIn('would recurse without bound', sides.stderr)


if __name__ == '__main__':
    unittest.main()
