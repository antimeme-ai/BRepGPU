"""Direct source-to-carrier specifications; execution belongs on the fleet."""
from dataclasses import FrozenInstanceError
from fractions import Fraction as F
from hashlib import sha256
from pathlib import Path
import tempfile
import unittest


def source(*, count_u=3, count_v=4, order_u=3, order_v=4,
           knots_u=None, knots_v=None, points=None, weights=None, copies=1,
           surface='BrepSurfaceNurbAPI'):
    """Literal independent source author; no production helper participates."""
    if knots_u is None: knots_u=[-2.0]*order_u+[6.0]*count_u
    if knots_v is None: knots_v=[3.0]*order_v+[5.0]*count_v
    if points is None:
        points=[(i/10.,j/7.,(i+2*j)/11.) for i in range(count_u) for j in range(count_v)]
    if weights is None: weights=[1.+(i%3)/10. for i in range(count_u*count_v)]
    attrs=[('token','face:surfaceType',[surface]*copies),
           ('token','face:trimType',['general']*copies),
           ('uint','face:loopCount',[1]*copies),
           ('double2','face:range',[(-2.,3.),(6.,5.)]*copies),
           ('uint','faceuse:faceIndex',list(range(copies))*2),
           ('token','faceuse:orientationType',['same']*copies+['opposite']*copies)]
    if surface=='BrepSurfaceNurbAPI':
        attrs += [('uint','brep:surface:nurb:uOrder',[order_u]*copies),
                  ('uint','brep:surface:nurb:vOrder',[order_v]*copies),
                  ('uint','brep:surface:nurb:uVertexCount',[count_u]*copies),
                  ('uint','brep:surface:nurb:vVertexCount',[count_v]*copies),
                  ('double','brep:surface:nurb:uKnots',knots_u*copies),
                  ('double','brep:surface:nurb:vKnots',knots_v*copies),
                  ('point3d','brep:surface:nurb:controlVertices',points*copies),
                  ('double','brep:surface:nurb:weights',weights*copies)]
    def fmt(value):
        if isinstance(value,str): return '"'+value+'"'
        if isinstance(value,tuple): return '('+', '.join(map(repr,value))+')'
        return repr(value)
    body='\n'.join('uniform '+t+'[] '+n+' = ['+', '.join(map(fmt,v))+']' for t,n,v in attrs)
    return ('#usda 1.0\ndef BrepArray "source" {\n'+body+'\n}\n').encode()


class ConversionTests(unittest.TestCase):
    def test_literal_rational_layout_identity_and_exact_bounds(self):
        from brepgpu.interchange import convert
        data=source(copies=2)
        result=convert(data)
        self.assertEqual(result.sha256,sha256(data).hexdigest())
        self.assertEqual(result.source_bytes,data)
        self.assertEqual(result.trim_scope,'uninterpreted')
        self.assertEqual(len(result.faces),2)
        for ordinal,(face,patch) in enumerate(zip(result.faces,result.patches)):
            self.assertEqual(face.key,(result.sha256,'/source',ordinal))
            self.assertEqual((face.ordinal,face.patch_slot,face.status,face.reason),(ordinal,ordinal,'converted',''))
            self.assertEqual(face.faceuses,((ordinal,'same'),(ordinal+2,'opposite')))
            self.assertEqual(face.face_range,(-2.,6.,3.,5.))
            self.assertEqual(face.nurbs.domain,(-2.,6.,3.,5.))
            self.assertEqual((patch.degree_u,patch.degree_v),(2,3))
            self.assertEqual(patch.uv,face.nurbs.domain)
            self.assertEqual((patch.lineage.body_id,patch.lineage.entity_kind,patch.lineage.entity_id,
                              patch.lineage.revision,patch.lineage.chart_id),
                             (result.body_id,5,ordinal+1,1,ordinal+1))
            controls=tuple(zip(*(iter(patch.controls),)*4))
            ratios=[F(c[3])/F(w) for c,w in zip(controls,face.nurbs.weights)]
            self.assertEqual(len(set(ratios)),1)
            delta=max(abs(F(c[k])/F(c[3])*F(patch.scale)-F(p[k]))
                      for c,p in zip(controls,face.nurbs.points) for k in range(3))
            self.assertEqual(face.position_bound,delta)
            kappa=max(map(F,face.nurbs.weights))/min(map(F,face.nurbs.weights))
            self.assertEqual(face.du_bound,4*2*kappa*delta/8)
            self.assertEqual(face.dv_bound,4*3*kappa*delta/2)
            self.assertGreater(delta,0)
            for c,p,w in zip(controls,face.nurbs.points,face.nurbs.weights):
                W=F(w)*ratios[0]
                self.assertEqual(c[3],float(W))
                self.assertEqual(c[:3],tuple(float(F(x)*W/F(patch.scale)) for x in p))
                self.assertTrue(1<=c[3]<=256)
                self.assertTrue(all(abs(x)<=c[3] for x in c[:3]))
        self.assertNotEqual(result.patches[0].lineage,result.patches[1].lineage)
        self.assertNotEqual(convert(data).body_id,result.body_id)

    def test_real_foreign_mouse_complete_inventory(self):
        from brepgpu.interchange import load
        result=load(Path(__file__).resolve().parents[1]/'data/interchange/mouse.usda')
        self.assertEqual(result.sha256,'0fedf1649d094ad38adb14095ddd0fef888016c80245ef488d66b70db1b4d39d')
        self.assertEqual([f.status for f in result.faces],['unsupported','converted','converted','unsupported'])
        self.assertEqual([f.patch_slot for f in result.faces],[None,0,1,None])
        self.assertEqual([f.nurbs.count_u for f in result.faces],[10,3,2,10])
        self.assertEqual([len(f.faceuses) for f in result.faces],[2]*4)
        self.assertEqual(result.faces[1].nurbs.points[0],(3.,4.8,0.))
        self.assertEqual(result.faces[2].nurbs.points,((-4.,6.,0.),(-4.,-7.,0.),(4.,6.,0.),(4.,-7.,0.)))
        self.assertTrue(all(f.trim_type=='general' for f in result.faces))

    def test_degree_zero_and_nonexact_span(self):
        from brepgpu.interchange import convert
        data=source(count_u=1,count_v=1,order_u=1,order_v=1,
                    knots_u=[-2.**-54,1.],knots_v=[0.,1.],points=[(.1,.2,.3)],weights=[1.1])
        result=convert(data)
        face=result.faces[0]
        self.assertEqual((result.patches[0].degree_u,result.patches[0].degree_v),(0,0))
        self.assertEqual((face.du_bound,face.dv_bound),(F(0),F(0)))
        self.assertEqual(F(face.nurbs.domain[1])-F(face.nurbs.domain[0]),1+F(1,2**54))

    def test_complete_unsupported_analytic_inventory(self):
        from brepgpu.interchange import convert
        result=convert(source(surface='BrepSurfacePlaneAPI',copies=2))
        self.assertEqual(result.patches,())
        self.assertEqual(len(result.faces),2)
        self.assertTrue(all(f.status=='unsupported' and f.reason and f.nurbs is None
                            and f.patch_slot is None and f.position_bound==0 for f in result.faces))

    def test_type_prefix_and_document_face_ordinal(self):
        from brepgpu.interchange import convert
        literal=b'''#usda 1.0
def BrepArray "mixed" {
uniform token[] face:surfaceType = ["BrepSurfacePlaneAPI", "BrepSurfaceNurbAPI", "FutureSurfaceAPI", "BrepSurfaceNurbAPI"]
uniform token[] face:trimType = ["general", "general", "rectangular", "general"]
uniform uint[] face:loopCount = [0, 0, 0, 0]
uniform double2[] face:range = [(0,0),(1,1),(0,0),(1,1),(0,0),(1,1),(0,0),(1,1)]
uniform uint[] faceuse:faceIndex = [3,1,0,3]
uniform token[] faceuse:orientationType = ["opposite", "same", "same", "same"]
uniform uint[] brep:surface:nurb:uOrder = [1,1]
uniform uint[] brep:surface:nurb:vOrder = [1,1]
uniform uint[] brep:surface:nurb:uVertexCount = [1,1]
uniform uint[] brep:surface:nurb:vVertexCount = [1,1]
uniform double[] brep:surface:nurb:uKnots = [0,1,0,1]
uniform double[] brep:surface:nurb:vKnots = [0,1,0,1]
uniform point3d[] brep:surface:nurb:controlVertices = [(1,2,3),(4,5,6)]
uniform double[] brep:surface:nurb:weights = [1,1]
}
'''
        # A second prim shares geometry, never source or native identity.
        data=literal+literal.split(b'\n',1)[1].replace(b'"mixed"',b'"other"',1)
        result=convert(data)
        self.assertEqual([f.patch_slot for f in result.faces],[None,0,None,1,None,2,None,3])
        self.assertEqual([f.ordinal for f in result.faces],list(range(8)))
        self.assertEqual(result.faces[3].faceuses,((0,'opposite'),(3,'same')))
        self.assertEqual(result.faces[1].nurbs.points,((1.,2.,3.),))
        self.assertEqual(result.faces[3].nurbs.points,((4.,5.,6.),))
        self.assertEqual([p.lineage.entity_id for p in result.patches],[2,4,6,8])
        self.assertEqual(result.faces[5].key[1:],('/other',1))

    def test_multispan_unclamped_and_envelope_outcomes(self):
        from brepgpu.interchange import convert
        cases=[source(count_u=4,order_u=3,knots_u=[0.,0.,0.,.5,1.,1.,1.]),
               source(knots_u=[-2.,-1.,0.,1.,2.,3.]),
               source(weights=[1.,257.]+[1.]*10),
               source(weights=[1.,255.]+[1.]*10),
               source(points=[(2.**21,0.,0.)]*12),
               source(knots_v=[0.]*4+[2.**-21]*4)]
        # 1..255 has a legal common power-of-two multiplier (one).
        for i,data in enumerate(cases):
            with self.subTest(case=i):
                result=convert(data)
                self.assertEqual(result.faces[0].status,'converted' if i==3 else 'unsupported')
                if i!=3: self.assertTrue(result.faces[0].reason)

    def test_weight_ratio_alone_insufficient_and_extreme_scales(self):
        from brepgpu.interchange import convert
        result=convert(source(weights=[.75,150.]+[1.]*10))
        self.assertEqual(result.faces[0].status,'unsupported')
        for weight in (2.**-1074,2.**1023):
            result=convert(source(weights=[weight]*12))
            self.assertEqual(result.faces[0].status,'converted')
            self.assertTrue(all(w==1. for w in result.patches[0].controls[3::4]))

    def test_corrupt_structure_rejects_whole_file(self):
        from brepgpu.interchange import convert,SourceError
        valid=source()
        cases=[source(weights=[1.]*11),source(weights=[0.]+[1.]*11),
               source(knots_u=[-2.,-2.,-2.,7.,6.,6.]),
               valid.replace(b'faceuse:faceIndex = [0, 0]',b'faceuse:faceIndex = [1, 0]'),
               valid.replace(b'face:loopCount = [1]',b'face:loopCount = []'),
               valid.replace(b'uint[] brep:surface:nurb:uOrder',b'int[] brep:surface:nurb:uOrder'),
               valid.replace(b'"opposite"',b'"sideways"')]
        for data in cases:
            with self.subTest(data=data[-100:]),self.assertRaises(SourceError): convert(data)

    def test_owned_immutable_bytes_and_factory(self):
        from brepgpu.interchange import convert,load,Conversion,SourceError,SourceBudgetError
        with tempfile.TemporaryDirectory() as directory:
            filename=Path(directory)/'source.usda'; data=source(); filename.write_bytes(data)
            result=load(filename); filename.write_bytes(b'garbage')
            self.assertEqual(result.source_bytes,data)
            self.assertEqual(result.sha256,sha256(data).hexdigest())
            self.assertEqual(result.faces[0].nurbs.points[0],(0.,0.,0.))
        with self.assertRaises((TypeError,ValueError)): Conversion()
        with self.assertRaises((FrozenInstanceError,AttributeError)): result.faces=()
        with self.assertRaises((FrozenInstanceError,AttributeError)): result.faces[0].nurbs.points=()
        with self.assertRaises((TypeError,SourceError)): convert(bytearray(data))
        with self.assertRaises(SourceBudgetError): convert(b'x'*(16*1024*1024+1))


if __name__=='__main__': unittest.main()
