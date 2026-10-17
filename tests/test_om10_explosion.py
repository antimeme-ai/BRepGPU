"""Direct translation-only scene transport properties; run on the fleet."""
import pathlib,struct,sys,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'examples'))
from om10_explosion import translated_scene,translated_reference_scene

class TranslationTransport(unittest.TestCase):
    def scene(self):
        header=struct.pack('<8sII8Q',b'BGMESH01',1,1,2,3,0,0,0,0,0,0)
        definitions=struct.pack('<10Q',201,0,0,0,0,202,0,0,0,0)
        matrices=[(1.,0.,0.,3.,0.,1.,0.,4.,0.,0.,1.,5.),
                  (0.,-1.,0.,-2.,1.,0.,0.,7.,0.,0.,1.,9.),
                  (1.,0.,0.,8.,0.,1.,0.,1.,0.,0.,1.,6.)]
        instances=b''.join(struct.pack('<QII15d',identifier,slot,0,*matrix,0.,0.,0.)
                           for identifier,slot,matrix in zip((41,99,105),(0,0,1),matrices))
        return header+definitions+instances,matrices
    def test_all_occurrences_keep_original_rotations_and_every_other_byte(self):
        original,matrices=self.scene();offsets={41:(2.,3.,4.),99:(-8.,.5,0.),105:(0.,0.,0.)}
        actual=translated_scene(original,offsets)
        allowed=set()
        for i,identifier in enumerate((41,99,105)):
            at=80+80+136*i
            row=struct.unpack_from('<QII15d',actual,at)
            self.assertEqual(row[0],identifier);self.assertEqual(row[3:15],matrices[i]);self.assertEqual(row[15:18],offsets[identifier])
            allowed.update(range(at+112,at+136))
            point=(1.,2.,3.)
            before=tuple(sum(matrices[i][4*r+k]*point[k] for k in range(3))+matrices[i][4*r+3] for r in range(3))
            after=tuple(sum(row[3+4*r+k]*point[k] for k in range(3))+row[3+4*r+3]+row[15+r] for r in range(3))
            self.assertEqual(after,tuple(before[k]+offsets[identifier][k] for k in range(3)))
        self.assertTrue(all(x==y for i,(x,y) in enumerate(zip(original,actual)) if i not in allowed))
        self.assertEqual(original,self.scene()[0])
    def test_incomplete_unknown_nonfinite_and_duplicate_identity_refuse(self):
        original,_=self.scene()
        for offsets in ({41:(0.,0.,0.)},{41:(0.,0.,0.),99:(0.,0.,0.),105:(0.,0.,0.),999:(0.,0.,0.)},
                        {41:(float('nan'),0.,0.),99:(0.,0.,0.),105:(0.,0.,0.)}):
            with self.subTest(offsets=offsets),self.assertRaises(ValueError):translated_scene(original,offsets)
        duplicate=bytearray(original);struct.pack_into('<Q',duplicate,80+80+136,41)
        with self.assertRaises(ValueError):translated_scene(duplicate,{41:(0.,0.,0.),105:(0.,0.,0.)})

class ReferenceTranslationTransport(unittest.TestCase):
    def reference_scene(self):
        original,matrices=TranslationTransport().scene()
        header=struct.pack('<8sII6Q',b'BGREFM01',1,0,2,3,6,2,1,0)
        definitions=struct.pack('<10Q',201,0,3,0,1,202,3,3,1,1)
        instances=original[160:]
        vertices=b''.join(struct.pack('<6dQ',*p,0.,0.,1.,i%3+1)
                          for i,p in enumerate(((0.,0.,0.),(1.,0.,0.),(0.,1.,0.),
                                                (0.,0.,2.),(1.,0.,2.),(0.,1.,2.))))
        triangles=struct.pack('<4IQ4IQ',0,1,2,0,701,0,1,2,0,702)
        material=struct.pack('<4d',.1,.2,.3,.4)
        expected={i:(d,m) for i,d,m in zip((41,99,105),(201,201,202),matrices)}
        return header+definitions+instances+vertices+triangles+material,expected
    def test_reference_geometry_normals_faces_materials_and_poses_unchanged(self):
        original,expected=self.reference_scene()
        offsets={41:(2.,3.,4.),99:(-8.,.5,0.),105:(0.,0.,0.)}
        actual=translated_reference_scene(original,offsets,expected)
        allowed=set()
        for i,identifier in enumerate((41,99,105)):
            at=64+80+136*i
            self.assertEqual(struct.unpack_from('<3d',actual,at+112),offsets[identifier])
            allowed.update(range(at+112,at+136))
        self.assertTrue(all(x==y for i,(x,y) in enumerate(zip(original,actual)) if i not in allowed))
        self.assertEqual(original[64+80+3*136:],actual[64+80+3*136:])
    def test_reference_wrong_original_identity_pose_and_offsets_refuse(self):
        original,expected=self.reference_scene();offsets={i:(0.,0.,0.) for i in expected}
        bad_pose=dict(expected);bad_pose[99]=(201,expected[41][1])
        bad_definition=dict(expected);bad_definition[99]=(202,expected[99][1])
        for wrong in (bad_pose,bad_definition):
            with self.assertRaises(ValueError):translated_reference_scene(original,offsets,wrong)
        for wrong in ({41:(0.,0.,0.)},{**offsets,999:(0.,0.,0.)},{**offsets,99:(float('inf'),0.,0.)}):
            with self.assertRaises(ValueError):translated_reference_scene(original,wrong,expected)
        duplicate=bytearray(original);struct.pack_into('<Q',duplicate,64+80+136,41)
        with self.assertRaises(ValueError):translated_reference_scene(duplicate,offsets,expected)
if __name__=='__main__':unittest.main()
