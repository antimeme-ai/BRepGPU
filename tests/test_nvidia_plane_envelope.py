"""Specification check for the external finite Plane-range preparation policy."""
import csv,math,os,pathlib,subprocess,tempfile,unittest
from fractions import Fraction as F
ROOT=pathlib.Path(__file__).resolve().parents[1]
FIXTURE=pathlib.Path(os.environ.get('BREPGPU_NVIDIA_PLANE_FIXTURE',ROOT/'build/nvidia-reference-recovery/brepgpu-om10-nvidia-plane-fixture'))
HELPER=pathlib.Path(os.environ.get('BREPGPU_NVIDIA_PLANE_ENVELOPE',ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-plane-envelope'))
PRODUCER=pathlib.Path(os.environ.get('BREPGPU_OM10_NVIDIA',ROOT/'build/nvidia-reference-recovery/brepgpu-om10-nvidia'))
class PlaneEnvelope(unittest.TestCase):
 def test_authored_rational_caps_keep_geometry_and_get_complete_support(self):
  from pxr import Usd,Vt,Gf
  from brepgpu.nvidia_reference import load_definition
  from brepgpu.imported_oracles import qualify_physical_solid
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);original=p/'original.usdc';subprocess.run([str(FIXTURE),str(original)],check=True,capture_output=True,text=True)
   s=Usd.Stage.Open(str(original));prim=next(x for x in s.Traverse() if x.GetTypeName()=='BrepArray');types=prim.GetAttribute('face:surfaceType').Get();planes=[i for i,x in enumerate(types) if str(x)=='BrepSurfacePlaneAPI'];self.assertEqual(len(planes),2)
   # Deliberately reproduce an insufficient finite support rectangle while the
   # literal original radius2 rational circles and every other attribute stay.
   narrow=p/'narrow.usdc';self.assertTrue(s.GetRootLayer().Export(str(narrow)));n=Usd.Stage.Open(str(narrow));q=n.GetPrimAtPath(prim.GetPath());a=q.GetAttribute('face:range');ranges=Vt.Vec2dArray(a.Get())
   for face in planes:
    ranges[2*face]=Gf.Vec2d(-.25,-.25);ranges[2*face+1]=Gf.Vec2d(.25,.25)
   self.assertTrue(a.Set(ranges));self.assertTrue(n.GetRootLayer().Save())
   prepared=p/'prepared';subprocess.run([os.sys.executable,'-B',str(ROOT/'scripts/prepare_nvidia_plane_envelope.py'),'--input-usd',str(narrow),'--output-directory',str(prepared),'--helper',str(HELPER)],check=True,capture_output=True,text=True)
   with (prepared/'source.controls.tsv').open() as f:controls=list(csv.DictReader(f,delimiter='\t'))
   self.assertTrue(any(float(r['w'])!=1 for r in controls if r['kind']=='curve'),'fixture must exercise positive rational weights')
   self.assertTrue(all(math.isfinite(float(r['w'])) and float(r['w'])>0 for r in controls))
   corrected=Usd.Stage.Open(str(prepared/'corrected.usdc'));actual=corrected.GetPrimAtPath(prim.GetPath()).GetAttribute('face:range').Get()
   # The SDK chooses a translated cap chart, not a center-origin chart. Map
   # literal physical circle cardinal points into that declared affine chart.
   # This does not use the captured curve hull or its AABB extrema.
   with (prepared/'source.canonical.tsv').open() as f:frames=list(csv.DictReader(f,delimiter='\t'))
   dot=lambda a,b:sum(x*y for x,y in zip(a,b))
   excluded=0
   for face,frame in zip(planes,frames):
    o,u,v=[tuple(F(float(frame[k])) for k in keys) for keys in [('ox','oy','oz'),('ux','uy','uz'),('vx','vy','vz')]]
    uu,vv,uv=dot(u,u),dot(v,v),dot(u,v);det=uu*vv-uv*uv
    z=F(.5) if abs(float(o[2])-.5)<1 else F(3.5)
    for x,y in [(F(3.25),F(-.75)),(F(-.75),F(-.75)),(F(1.25),F(1.25)),(F(1.25),F(-2.75))]:
     delta=(x-o[0],y-o[1],z-o[2]);du,dv=dot(delta,u),dot(delta,v);coordinates=((du*vv-dv*uv)/det,(dv*uu-du*uv)/det)
     for axis,value in enumerate(coordinates):
      excluded+=not(F(float(ranges[2*face][axis]))<=value<=F(float(ranges[2*face+1][axis])))
      self.assertLessEqual(F(float(actual[2*face][axis])),value);self.assertGreaterEqual(F(float(actual[2*face+1][axis])),value)
   self.assertGreater(excluded,0,'RED: narrow chart excludes actual literal circle points')
   for before,after in zip(prim.GetAttributes(),corrected.GetPrimAtPath(prim.GetPath()).GetAttributes()):
    self.assertEqual(before.GetName(),after.GetName())
    if before.GetName()!='face:range':self.assertEqual(before.Get(),after.Get())
   output=p/'triangles';subprocess.run([str(PRODUCER),'--heal','0','--chord-height','.005','--curve-angle','25','--surface-angle','25','--usd-definition','1',str(prepared/'corrected.usdc'),'--output-directory',str(output)],check=True,capture_output=True,text=True)
   mesh=load_definition(output/'definition-1.bgrefmesh',expected_id=1);self.assertEqual(set(mesh.face_ids),{0,1,2})
   physical=dict(zip(mesh.physical_keys,mesh.points));triangles=tuple(tuple(mesh.physical_keys[i] for i in t) for t in mesh.triangles);self.assertEqual(qualify_physical_solid(physical,triangles)['euler'],2)
   for x,y,z in mesh.points:
    radius=math.hypot(x-1.25,y+.75);self.assertLessEqual(radius,2+1e-12);self.assertGreaterEqual(z,.5-1e-12);self.assertLessEqual(z,3.5+1e-12)
    if min(abs(z-.5),abs(z-3.5))>1e-12:self.assertAlmostEqual(radius,2,delta=1e-12)
if __name__=='__main__':unittest.main()
