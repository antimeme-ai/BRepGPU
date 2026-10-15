"""Independent source-to-native interchange qualification; fleet execution only."""
from array import array
import copy
import ctypes as C
from dataclasses import fields,replace
from fractions import Fraction as F
import math
import os
from pathlib import Path
import struct
import unittest


def record(p=2,q=3,*,domain=(-2.,3.,4.,6.),nondyadic=False):
    points=tuple((i/4,j/8,(2*i+3*j+i*j)/64) for i in range(p+1) for j in range(q+1))
    weights=tuple(1+(i+2*j)/16 for i in range(p+1) for j in range(q+1))
    if nondyadic:
        points=tuple((x+.1,y-.03,z+.07) for x,y,z in points)
        weights=tuple(w+.1 for w in weights)
    a,b,c,d=domain
    return dict(ou=p+1,ov=q+1,nu=p+1,nv=q+1,ku=(a,)*(p+1)+(b,)*(p+1),
                kv=(c,)*(q+1)+(d,)*(q+1),points=points,weights=weights)


def source_bytes(records,*,name='Source'):
    # Literal source fixture writer; no product parser/converter/helper supplies it.
    def value(x):
        if type(x) is str:return '"'+x+'"'
        if type(x) is tuple:return '('+', '.join(value(v) for v in x)+')'
        return repr(x)
    attrs=[]
    def attr(kind,name,values):attrs.append('uniform '+kind+'[] '+name+' = ['+', '.join(value(v) for v in values)+']')
    kinds=tuple('BrepSurfacePlaneAPI' if r is None else 'BrepSurfaceNurbAPI' for r in records)
    n=len(records)
    attr('token','face:surfaceType',kinds);attr('token','face:trimType',('general',)*n)
    # Deliberately narrower face range: it is retained evidence, never the carrier domain.
    attr('double2','face:range',tuple(p for r in records for p in ((0.,0.),(0.,0.))))
    attr('uint','face:loopCount',tuple(1+i for i in range(n)))
    attr('uint','faceuse:faceIndex',tuple(range(n))+tuple(reversed(range(n))))
    attr('token','faceuse:orientationType',('same',)*n+('opposite',)*n)
    rational=tuple(r for r in records if r is not None)
    for key,kind,field in (('ou','uint','uOrder'),('ov','uint','vOrder'),('nu','uint','uVertexCount'),('nv','uint','vVertexCount')):
        attr(kind,'brep:surface:nurb:'+field,tuple(r[key] for r in rational))
    for key,kind,field in (('ku','double','uKnots'),('kv','double','vKnots'),('points','point3d','controlVertices'),('weights','double','weights')):
        attr(kind,'brep:surface:nurb:'+field,tuple(x for r in rational for x in r[key]))
    return ('#usda 1.0\ndef BrepArray "'+name+'" {\n'+'\n'.join(attrs)+'\n}\n').encode()


def altered(record,**changes):
    result=object.__new__(type(record))
    for field in fields(record):object.__setattr__(result,field.name,changes.get(field.name,getattr(record,field.name)))
    return result


def changed_batch(batch):
    result=copy.copy(batch)
    result.data=array('d',batch.data);result.statuses=array('I',batch.statuses)
    result._slots=array('I',batch._slots);result._uv=array('d',batch._uv)
    result._descriptors=type(batch._descriptors).from_buffer_copy(batch._descriptors)
    result.info=type(batch.info).from_buffer_copy(batch.info)
    return result



class InterchangeTests(unittest.TestCase):
    def test_existing_native_then_literal_rational_source_jet(self):
        from brepgpu import Engine,Patch
        with Engine(backend='cpu',threads=1) as engine:
            patch=Patch(1,1,((0.,0.,0.,1.),(0.,1.,0.,1.),(1.,0.,0.,1.),(1.,1.,0.,1.)))
            with engine.create_patch_set((patch,)) as patches:
                with engine.create_query_set(patches,patch_slots=(0,),uv=(.25,.75)) as queries:
                    with engine.evaluate(queries,order=1) as result:
                        local=result.download().require_ok()
            self.assertEqual(local.vector(0,0),(.25,.75,0.))
            self.assertEqual(local.vector(1,0),(1.,0.,0.))
        from brepgpu.interchange_oracles import source_jet
        from brepgpu.interchange import SourceNurbs
        source=SourceNurbs(2,1,2,1,(2.,2.,6.,6.),(-1.,1.),((0.,0.,0.),(1.,0.,0.)),(1.,2.))
        self.assertEqual(source_jet(source,3.,0.),((F(2,5),F(0),F(0)),(F(8,25),F(0),F(0)),(F(0),F(0),F(0))))

    def run_native(self,conversion,slots=None,uv=None):
        from brepgpu import Engine
        from brepgpu.interchange_oracles import qualify
        if slots is None:
            slots=tuple(i for i in range(len(conversion.patches)) for _ in range(9))
            uv=tuple(v for _ in conversion.patches for s,t in ((0.,0.),(0.,1.),(1.,0.),(1.,1.),(.25,.75),(.5,.5),(.75,.125),(.125,.5),(.75,.75)) for v in (s,t))
        backend=os.environ.get('BREPGPU_TEST_BACKEND','cpu')
        with Engine(backend=backend,device=int(os.environ.get('BREPGPU_TEST_DEVICE','0')),threads=int(os.environ.get('BREPGPU_TEST_THREADS','1'))) as engine:
            with engine.create_patch_set(conversion.patches) as patches:
                with engine.create_query_set(patches,patch_slots=slots,uv=uv) as queries:
                    with engine.evaluate(queries,order=1) as result:local=result.download()
            world=local.convert()
        self.assertEqual(tuple(local._slots),tuple(slots))
        self.assertEqual(tuple(struct.pack('<d',x) for x in local._uv),tuple(struct.pack('<d',x) for x in uv))
        self.assertEqual(local.info.backend,1 if backend=='cuda' else 0)
        quality=qualify(conversion,local,world)
        self.assertEqual(quality['queries'],len(slots))
        self.assertEqual(quality['faces'],len(conversion.faces))
        return local,world,quality

    def test_asymmetric_rational_nonunit_domains_and_nondyadic_controls(self):
        from brepgpu.interchange import convert
        from brepgpu.interchange_oracles import source_jet
        for nondyadic in (False,True):
            with self.subTest(nondyadic=nondyadic):
                conversion=convert(source_bytes((record(nondyadic=nondyadic),)))
                face=conversion.faces[0];patch=conversion.patches[0]
                self.assertEqual((patch.degree_u,patch.degree_v),(2,3))
                self.assertEqual(patch.uv,(-2.,3.,4.,6.))
                self.assertEqual(face.face_range,(0.,0.,0.,0.))
                self.assertEqual(face.faceuses,((0,'same'),(1,'opposite')))
                self.assertEqual(face.trim_scope,'uninterpreted')
                local,world,quality=self.run_native(conversion)
                self.assertEqual(quality['successful_queries'],9)
                self.assertEqual(quality['invalid_queries'],0)
                if nondyadic:self.assertGreater(face.position_bound,0)
                else:
                    self.assertEqual((face.position_bound,face.du_bound,face.dv_bound),(F(0),)*3)
                    # Exact literal weighted x/y at s=1/4,t=3/4, independently hand-derived moments.
                    jet=source_jet(face.nurbs,F(-3,4),F(11,2))
                    self.assertEqual(jet[0][:2],(F(29,224),F(129,448)))
                self.assertLessEqual(quality['max_source_du_error'],quality['max_source_du_budget'])

    def test_unchanged_foreign_mouse_complete_inventory_and_original_uses(self):
        from brepgpu.interchange import load
        conversion=load(Path(__file__).resolve().parents[1]/'data/interchange/mouse.usda')
        self.assertEqual(conversion.sha256,'0fedf1649d094ad38adb14095ddd0fef888016c80245ef488d66b70db1b4d39d')
        self.assertEqual(tuple(f.status for f in conversion.faces),('unsupported','converted','converted','unsupported'))
        self.assertEqual(tuple(f.patch_slot for f in conversion.faces),(None,0,1,None))
        self.assertEqual(tuple(f.faceuses for f in conversion.faces),(((2,'same'),(7,'opposite')),((0,'same'),(5,'opposite')),((3,'same'),(6,'opposite')),((1,'same'),(4,'opposite'))))
        self.assertEqual(tuple((p.degree_u,p.degree_v) for p in conversion.patches),((2,1),(1,1)))
        self.assertEqual(tuple(p.lineage.entity_id for p in conversion.patches),(2,3))
        self.assertEqual(conversion.faces[1].nurbs.points[0],(3.,4.8,0.))
        local,world,quality=self.run_native(conversion)
        self.assertEqual((quality['faces'],quality['converted_faces'],quality['unsupported_faces']),(4,2,2))
        self.assertTrue(all(f.trim_type=='general' and f.trim_scope=='uninterpreted' for f in conversion.faces))

    def test_nonexact_endpoint_difference_degree_zero_and_signed_zero(self):
        from brepgpu.interchange import convert,SourceNurbs
        from brepgpu.interchange_oracles import qualify,source_jet
        records=(record(2,1,domain=(-2.**-54,1.,-3.,2.)),record(0,2),record(2,0),record(0,0))
        conversion=convert(source_bytes(records))
        source=conversion.faces[0].nurbs
        self.assertEqual(F(source.domain[1])-F(source.domain[0]),1+F(1,1<<54))
        self.assertEqual(source.domain[1]-source.domain[0],1.)
        local,world,quality=self.run_native(conversion)
        self.assertEqual(quality['successful_queries'],36)
        for slot,axis in ((1,1),(2,2),(3,1),(3,2)):
            for i in range(slot*9,(slot+1)*9):self.assertEqual(local.vector(axis,i),(0.,0.,0.))
        # An actual native convert call consumes negative-zero local derivatives;
        # its unconditional +0 must yield the exact specified positive zero.
        signed=changed_batch(local)
        for i,slot in enumerate(signed._slots):
            if slot==3:
                for term in (1,2):
                    for axis in range(3):signed.data[(term*3+axis)*signed.count+i]=-0.
        converted=signed.convert()
        qualify(conversion,signed,converted)
        for i,slot in enumerate(signed._slots):
            if slot==3:self.assertEqual(struct.pack('<d',converted.vector(1,i)[0]),b'\0'*8)
        # Original source endpoint limits and an internal knot's right derivative.
        n=SourceNurbs(2,1,3,1,(0.,0.,1.,2.,2.),(0.,1.),((0.,0.,0.),(1.,0.,0.),(3.,0.,0.)),(1.,1.,1.))
        for u,x,dx in ((0,0,1),(1,1,2),(2,3,2)):
            self.assertEqual(source_jet(n,u,F(1,2)),((F(x),F(0),F(0)),(F(dx),F(0),F(0)),(F(0),F(0),F(0))))

    def test_multiple_prims_equal_carriers_permuted_rows_and_owned_namespace(self):
        from brepgpu.body import make_body
        from brepgpu.interchange import convert
        a=source_bytes((record(),None,record()),name='A')
        b=source_bytes((record(),),name='B').split(b'\n',1)[1]
        conversion=convert(a+b)
        body=make_body('box')
        self.assertNotEqual(body.body_id,conversion.body_id)
        self.assertEqual(tuple(f.key[1:] for f in conversion.faces),(('/A',0),('/A',1),('/A',2),('/B',0)))
        self.assertEqual(tuple(p.lineage.entity_id for p in conversion.patches),(1,3,4))
        self.assertEqual(conversion.patches[0].controls,conversion.patches[1].controls)
        self.assertNotEqual(conversion.patches[0].lineage,conversion.patches[1].lineage)
        slots=(2,0,1,2,1,0)
        uv=tuple(v for pair in ((.75,.125),(.25,.75),(.5,.5),(1.,0.),(0.,1.),(.125,.25)) for v in pair)
        local,world,quality=self.run_native(conversion,slots,uv)
        self.assertEqual(quality['converted_faces'],3)
        self.assertEqual(tuple(x.entity_id for x in local.lineage),(4,1,3,4,3,1))

    def test_invalid_query_statuses_nan_fields_and_empty_batch(self):
        from brepgpu.interchange import convert
        conversion=convert(source_bytes((record(),)))
        local,world,quality=self.run_native(conversion,(0,0,1,0,0),(.25,.75,-.1,.5,.2,.3,math.nan,.5,.2,math.inf))
        self.assertEqual(tuple(local.statuses),(0,2,1,2,2))
        self.assertEqual((quality['successful_queries'],quality['invalid_queries']),(1,4))
        local,world,quality=self.run_native(conversion,(),())
        self.assertEqual(quality['queries'],0)

    def test_qualified_immutable_sources_and_snapshots_survive_external_edits(self):
        from dataclasses import FrozenInstanceError
        from brepgpu.interchange import convert
        conversion=convert(source_bytes((record(nondyadic=True),)))
        original=conversion.source_bytes
        with self.assertRaises(FrozenInstanceError):conversion.body_id=0
        with self.assertRaises(FrozenInstanceError):conversion.faces[0].reason='changed'
        with self.assertRaises(FrozenInstanceError):conversion.faces[0].nurbs.weights=()
        self.run_native(conversion)
        self.assertIs(conversion.source_bytes,original)

    def test_falsified_source_inventory_identity_weights_domain_and_bounds_rejected(self):
        from brepgpu.interchange import convert
        from brepgpu.interchange_oracles import qualify
        conversion=convert(source_bytes((record(nondyadic=True),None,record()),name='Original'))
        local,world,_=self.run_native(conversion)
        face=conversion.faces[0];patch=conversion.patches[0]
        variations=[altered(conversion,source_bytes=conversion.source_bytes+b'\n'),altered(conversion,faces=conversion.faces[:-1])]
        for changed in (altered(face,ordinal=True),altered(face,key=(conversion.sha256,'/Different',0)),
                        altered(face,faceuses=((0,'opposite'),(5,'same'))),
                        altered(face,nurbs=replace(face.nurbs,weights=tuple(reversed(face.nurbs.weights)))),
                        altered(face,face_range=(-2.,3.,4.,6.)),altered(face,position_bound=F(0)),
                        altered(face,du_bound=face.du_bound+1),altered(face,patch_slot=1)):
            variations.append(altered(conversion,faces=(changed,)+conversion.faces[1:]))
        values=list(patch.controls);values[0]=math.nextafter(values[0],math.inf)
        for changed in (replace(patch,controls=values),replace(patch,uv=(-1.,4.,4.,6.)),replace(patch,degree_u=3)):
            variations.append(altered(conversion,patches=(changed,)+conversion.patches[1:]))
        for changed in variations:
            with self.assertRaises(AssertionError):qualify(changed,local,world)

    def test_falsified_native_descriptors_queries_info_status_and_local_jets_rejected(self):
        from brepgpu.interchange import convert
        from brepgpu.interchange_oracles import qualify
        conversion=convert(source_bytes((record(),record())))
        local,world,_=self.run_native(conversion)
        variations=[]
        for field,value in (('control_offset',1),('degree_u',3),('chart_id',999),('body_id',0),('flags',0),('scale',4.)):
            batch=changed_batch(local);setattr(batch._descriptors[0],field,value);variations.append(batch)
        batch=changed_batch(local);batch._descriptors[0].uv[0]=-.5;variations.append(batch)
        batch=changed_batch(local);batch._slots[0]=1;variations.append(batch)
        batch=changed_batch(local);batch._uv[0]=.125;variations.append(batch)
        batch=changed_batch(local);batch.info.terms=1;variations.append(batch)
        batch=changed_batch(local);batch.info.backend=1-local.info.backend;variations.append(batch)
        batch=changed_batch(local);batch.statuses[0]=2;variations.append(batch)
        batch=changed_batch(local);batch.data[0]+=.01;variations.append(batch)
        for batch in variations:
            with self.assertRaises(AssertionError):qualify(conversion,batch,world)

    def test_one_ulp_wrong_converted_product_cannot_hide_inside_geometry_budget(self):
        from brepgpu.interchange import convert
        from brepgpu.interchange_oracles import qualify
        conversion=convert(source_bytes((record(nondyadic=True),)))
        local,world,_=self.run_native(conversion)
        for term in range(3):
            batch=changed_batch(world);index=(term*3)*batch.count+4
            value=batch.data[index];batch.data[index]=math.nextafter(value,math.inf)
            self.assertLess(abs(batch.data[index]-value),2.**-30)
            with self.assertRaisesRegex(AssertionError,'exact native frame product'):
                qualify(conversion,local,batch)


if __name__=='__main__':unittest.main()
