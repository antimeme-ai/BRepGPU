"""Prepare conservative finite Plane ranges at the external NVIDIA USD boundary.

Only Plane face:range entries widen. Source support geometry, topology and the
original USD remain unchanged. Requires the existing NVIDIA package usdpy and
its native plane-envelope helper; no meshing or distance certification occurs.
"""
import argparse,csv,itertools,math,pathlib,subprocess,time,hashlib,shutil
from fractions import Fraction as F
from pxr import Usd,Vt,Gf

def require(b,m):
 if not b:raise RuntimeError(m)
def read(path):
 with path.open() as f:return list(csv.DictReader(f,delimiter='\t'))
def exact(s):
 x=float(s);require(math.isfinite(x),'nonfinite control/frame');return F(x)
def outward(q,lower):
 x=float(q);require(math.isfinite(x),'nonfinite envelope')
 if (F(x)>q if lower else F(x)<q):x=math.nextafter(x,-math.inf if lower else math.inf)
 # Separate the mathematical endpoint by one further representable step.
 # This does not prove containment of every rounded SDK projection; final
 # mesh and finite source/UV gates remain necessary.
 x=math.nextafter(x,-math.inf if lower else math.inf)
 require(math.isfinite(x),'nonfinite outward envelope');return x
def inventory(stage):
 result=[]
 for p in stage.TraverseAll():
  attrs=[]
  for a in p.GetAttributes():
   require(not a.GetTimeSamples(),'animated attribute unsupported')
   require(str(a.GetTypeName()) not in ('asset','asset[]') or not a.Get(),'external asset attribute unsupported')
   meta=a.GetAllMetadata().copy();meta.pop('default',None)
   attrs.append((a.GetName(),str(a.GetTypeName()),a.IsCustom(),a.GetVariability(),meta,a.Get(),a.GetConnections()))
  rels=[(r.GetName(),r.GetAllMetadata(),r.GetTargets()) for r in p.GetRelationships()]
  result.append((str(p.GetPath()),p.GetAllMetadata(),attrs,rels))
 return result
def layer_metadata(stage):
 layer=stage.GetRootLayer()
 return (stage.GetPseudoRoot().GetAllMetadata(),
         {key:layer.pseudoRoot.GetInfo(key) for key in layer.pseudoRoot.ListInfoKeys()},
         layer.customLayerData)
def project(box,o,u,v):
 dot=lambda a,b:sum(x*y for x,y in zip(a,b))
 uu,vv,uv=dot(u,u),dot(v,v),dot(u,v);det=uu*vv-uv*uv;require(det>0,'degenerate plane basis')
 duals=[tuple((vv*u[i]-uv*v[i])/det for i in range(3)),tuple((uu*v[i]-uv*u[i])/det for i in range(3))]
 coords=[[],[]]
 for q in itertools.product(*box):
  delta=tuple(q[i]-o[i] for i in range(3))
  for axis in range(2):coords[axis].append(dot(delta,duals[axis]))
 return [(min(a),max(a)) for a in coords]
def prepare(original,prefix,driver):
 begin=time.monotonic()
 stage=Usd.Stage.Open(str(original));require(bool(stage),'USD unavailable');require(not stage.GetRootLayer().subLayerPaths,'sublayers unsupported')
 prims=list(stage.TraverseAll());require(all(not p.HasAuthoredReferences() and not p.HasAuthoredPayloads() and not p.GetVariantSets().GetNames() for p in prims),'composed USD unsupported')
 breps=[p for p in prims if p.GetTypeName()=='BrepArray'];require(len(breps)==1,'one complete BrepArray required');p=breps[0]
 types=list(p.GetAttribute('face:surfaceType').Get());ranges=p.GetAttribute('face:range').Get();require(types and len(ranges)==2*len(types),'incomplete face range inventory')
 planes=[i for i,t in enumerate(types) if str(t)=='BrepSurfacePlaneAPI']
 if not planes:
  # No Plane support domain exists to repair. Preserve the original bytes and
  # leave geometry admission of other surface families to the strict SDK.
  inventory(stage)
  corrected=prefix.parent/'corrected.usdc';require(not corrected.exists(),'corrected output exists')
  shutil.copyfile(original,corrected)
  require(original.read_bytes()==corrected.read_bytes(),'no-Plane layer bytes changed')
  elapsed=time.monotonic()-begin
  with (prefix.parent/'stages.tsv').open('w') as f:
   w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(['stage','seconds']);w.writerow(['no_plane_passthrough',elapsed])
  with (prefix.parent/'preparation.tsv').open('w') as f:
   w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(['field','value']);w.writerows([('policy','no_plane_passthrough_v1'),('input_sha256',hashlib.sha256(original.read_bytes()).hexdigest()),('output_sha256',hashlib.sha256(corrected.read_bytes()).hexdigest()),('source_faces',len(types)),('plane_ranges',0),('changed_attribute','none'),('support_geometry_modified',0),('finite_plane_domain_modified',0),('curves_topology_modified',0),('distance_qualification','not_performed')])
  return corrected
 names=['brep:surface:plane:'+s for s in ['origin','axis','refDirection']];data=[p.GetAttribute(n).Get() for n in names]
 require(all(a is not None and len(a)==len(planes) for a in data),'plane frame inventory mismatch')
 frame_path=pathlib.Path(str(prefix)+'.frames.tsv')
 with frame_path.open('w') as f:
  w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(['plane','ox','oy','oz','zx','zy','zz','ux','uy','uz'])
  for i in range(len(planes)):
   row=[float(x) for a in data for x in a[i]];require(all(map(math.isfinite,row)),'nonfinite staged frame');w.writerow([i,*map(repr,row)])
 start=time.monotonic();capture=subprocess.run([str(driver),str(original),str(prefix),str(frame_path)],capture_output=True,text=True,timeout=45)
 captured=time.monotonic();pathlib.Path(str(prefix)+'.capture.log').write_text(capture.stdout+capture.stderr);require(capture.returncode==0,'strict native hull capture failed')
 meta=read(pathlib.Path(str(prefix)+'.inventory.tsv'));require(len(meta)==1 and int(meta[0]['faces'])==len(types),'captured face inventory differs')
 controls=read(pathlib.Path(str(prefix)+'.controls.tsv'));curve_rows=[r for r in controls if r['kind']=='curve'];vertex_rows=[r for r in controls if r['kind']=='vertex']
 require(len(curve_rows)==int(meta[0]['controls']) and len(vertex_rows)==int(meta[0]['vertices']) and {int(r['owner']) for r in curve_rows}==set(range(int(meta[0]['edges']))),'incomplete boundary capture')
 pts=[]
 for r in controls:
  weight=exact(r['w']);require(weight>0,'nonpositive weight');pts.append(tuple(exact(r[k])/weight for k in ['hx','hy','hz']))
 require(pts,'empty boundary hull');box=[(min(q[a] for q in pts),max(q[a] for q in pts)) for a in range(3)]
 frames=read(pathlib.Path(str(prefix)+'.canonical.tsv'));require(len(frames)==len(planes) and [int(r['plane']) for r in frames]==list(range(len(planes))),'canonical frame inventory differs')
 new=Vt.Vec2dArray(ranges);changes=[]
 for face,row in zip(planes,frames):
  o,u,v=[tuple(exact(row[k]) for k in group) for group in [('ox','oy','oz'),('ux','uy','uz'),('vx','vy','vz')]]
  bounds=project(box,o,u,v);lower=[];upper=[]
  for a in range(2):
   oldlo,oldhi=map(float,[ranges[face*2][a],ranges[face*2+1][a]]);require(math.isfinite(oldlo) and math.isfinite(oldhi) and oldlo<oldhi,'invalid old plane range')
   lower.append(outward(min(F(oldlo),bounds[a][0]),True));upper.append(outward(max(F(oldhi),bounds[a][1]),False));require(lower[-1]<upper[-1],'collapsed plane range')
  new[face*2]=Gf.Vec2d(*lower);new[face*2+1]=Gf.Vec2d(*upper);changes.append((face,*ranges[face*2],*ranges[face*2+1],*lower,*upper))
 enclosed=time.monotonic();before=inventory(stage);before_metadata=layer_metadata(stage);corrected=prefix.parent/'corrected.usdc';temporary=prefix.parent/'corrected.pending.usdc';require(not corrected.exists() and not temporary.exists(),'corrected output exists');require(stage.GetRootLayer().Export(str(temporary)),'USD layer copy failed')
 changed=Usd.Stage.Open(str(temporary));require(changed.GetPrimAtPath(p.GetPath()).GetAttribute('face:range').Set(new),'range write failed');require(changed.GetRootLayer().Save(),'USD save failed');after=inventory(changed)
 require(before_metadata==layer_metadata(changed),'root-layer/stage metadata changed')
 require(len(before)==len(after),'prim inventory changed')
 for first,last in zip(before,after):
  require(first[:2]==last[:2] and first[3]==last[3],'prim metadata/relationships changed');require(len(first[2])==len(last[2]),'attribute inventory changed')
  for a,b in zip(first[2],last[2]):
   require(a[:5]==b[:5] and a[6:]==b[6:],'attribute metadata/connections changed')
   if first[0]==str(p.GetPath()) and a[0]=='face:range':
    require(list(b[5])==list(new),'authored plane range differs')
    for i,t in enumerate(types):
     if str(t)!='BrepSurfacePlaneAPI':require(a[5][2*i]==b[5][2*i] and a[5][2*i+1]==b[5][2*i+1],'nonplane range changed')
   else:require(a[5]==b[5],'nonrange attribute value changed')
 temporary.rename(corrected)
 with pathlib.Path(str(prefix)+'.ranges.tsv').open('w') as f:
  w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(['face','old_umin','old_vmin','old_umax','old_vmax','new_umin','new_vmin','new_umax','new_vmax']);w.writerows(changes)
 finished=time.monotonic()
 with (prefix.parent/'stages.tsv').open('w') as f:
  w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(['stage','seconds']);w.writerows([('usd_input_read_and_frames',start-begin),('native_capture_process',captured-start),('exact_hull_and_plane_envelopes',enclosed-captured),('usd_copy_write_and_typed_inventory',finished-enclosed),('prepare_complete',finished-begin)])
 with (prefix.parent/'preparation.tsv').open('w') as f:
  w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(['field','value']);w.writerows([('policy','positive_weight_control_hull_plane_range_v1'),('input_sha256',hashlib.sha256(original.read_bytes()).hexdigest()),('output_sha256',hashlib.sha256(corrected.read_bytes()).hexdigest()),('source_faces',len(types)),('source_edges',meta[0]['edges']),('curve_controls',len(curve_rows)),('plane_ranges',len(planes)),('changed_attribute','face:range Plane entries only'),('support_geometry_modified','0'),('finite_plane_domain_modified','1'),('curves_topology_modified','0'),('distance_qualification','not_performed')])
 print('PREPARED',corrected,'planes',len(planes),'source_edges',meta[0]['edges'],'controls',len(curve_rows),'seconds',finished-begin,flush=True)
 return corrected
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--input-usd',type=pathlib.Path,required=True);parser.add_argument('--output-directory',type=pathlib.Path,required=True);parser.add_argument('--helper',type=pathlib.Path,required=True);args=parser.parse_args()
 require(not args.output_directory.exists(),'output directory must be new');args.output_directory.mkdir(parents=True)
 prepare(args.input_usd.resolve(),args.output_directory.resolve()/'source',args.helper.resolve())
