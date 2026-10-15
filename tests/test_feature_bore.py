"""Actual profile/extrusion/bore composition obligations; fleet only."""
from dataclasses import FrozenInstanceError,fields,replace
from fractions import Fraction as F
import unittest

TRIANGLE=((-F(5,8),-F(1,2)),(F(3,4),-F(3,8)),(-F(1,8),F(3,4)))
CHAMFER=tuple((F(x,256),F(y,256)) for x,y in ((-160,-128),(64,-160),(192,-32),(128,128),(-32,192),(-192,32)))
SQUARE=tuple((F(x,8),F(y,8)) for x,y in ((-5,-5),(5,-5),(5,5),(-5,5)))


def maximum32():
    quadrant=((192,0),(188,37),(177,73),(160,107),(136,136),(107,160),(73,177),(37,188))
    return tuple(tuple(F(v,256) for v in ((x,y),(-y,x),(-x,-y),(y,-x))[q])
                 for q in range(4) for x,y in quadrant)


def changed(record,**values):
    result=object.__new__(type(record))
    for field in fields(record):object.__setattr__(result,field.name,values.get(field.name,getattr(record,field.name)))
    return result


def operands(points=CHAMFER,height=F(1,2),scale=1.,origin=(0.,0.,0.),cutter_scale=None,offset=(0.,0.)):
    from brepgpu.body import Frame,make_body
    from brepgpu.extrusion import make_profile,extrude
    profile=make_profile(points,Frame(origin,scale));feature=extrude(profile,height)
    cutter=make_body('cylinder',Frame((origin[0]+offset[0]*scale,origin[1]+offset[1]*scale,origin[2]),
                                     scale if cutter_scale is None else cutter_scale))
    return feature,cutter


class FeatureBoreTests(unittest.TestCase):
    def result(self,*args,**kwargs):
        from brepgpu.feature_bore import difference
        from brepgpu.feature_bore_oracles import qualify_result
        feature,cutter=operands(*args,**kwargs);result=difference(feature,cutter)
        self.assertIs(result.feature,feature);self.assertIs(result.cutter,cutter)
        self.assertEqual(qualify_result(result)['genus'],1)
        return result

    def test_actual_extrusion_consumed_and_original_sources_retained(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.extrusion import make_profile,extrude
        from brepgpu.feature_bore import difference
        from brepgpu.feature_bore_oracles import qualify_result
        profile=make_profile(tuple((F(x,256),F(y,256)) for x,y in
                                   ((-160,-128),(64,-160),(192,-32),(128,128),(-32,192),(-192,32))))
        feature=extrude(profile,F(1,2))
        cutter=make_body('cylinder',Frame((0.,0.,0.),1.))
        result=difference(feature,cutter)
        self.assertIs(result.feature,feature);self.assertIs(result.cutter,cutter)
        self.assertIs(result.feature.profile,profile)
        qualify_result(result)

    def test_independent_complete_profiles_and_source_order(self):
        for points in (TRIANGLE,SQUARE,CHAMFER,maximum32()[::2],maximum32()):
            for shifted in (points,points[1:]+points[:1]):
                with self.subTest(n=len(points),first=shifted[0]):
                    result=self.result(shifted);n=len(points)
                    self.assertEqual((len(result.history.records),len(result.history.origins)),(14*n+29,14*n+25))
                    self.assertEqual(len(result.composed_history.records),3*n+26)

    def test_frames_offcenter_and_actual_raw_cutter_band(self):
        for scale in (.25,.5,1.,2.,4.):
            with self.subTest(scale=scale):
                result=self.result(SQUARE,F(1,4),scale,(.1,-.2,.3),offset=(.0625,-.03125))
                self.assertEqual(result.solid.wall_band,(F(3,8),F(5,8)))
        for h in (F(1),F(511,256)):
            result=self.result(maximum32(),h,.5,cutter_scale=1.)
            self.assertEqual(result.solid.wall_band,(F(1,2)-h/4,F(1,2)+h/4))

    def test_exact_circle_support_admission_tangent_exterior_and_crossing(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.feature_bore import difference,UnsupportedBore
        feature,_=operands(SQUARE,F(1,4));tiny=2.**-40
        # Edge x=5/8, circle radius1/4: exact center3/8 is tangent.
        for x in (.375,.375+tiny,.5,1.):
            with self.subTest(center=x):
                cutter=make_body('cylinder',Frame((x,0.,0.),1.))
                with self.assertRaises(UnsupportedBore):difference(feature,cutter)
        good=difference(feature,make_body('cylinder',Frame((.375-tiny,0.,0.),1.)))
        from brepgpu.feature_bore_oracles import qualify_result
        qualify_result(good)
        # Oblique edge x+y=1/2 with center inside yet full disk outside.
        diamond=tuple((F(x,2),F(y,2)) for x,y in ((1,0),(0,1),(-1,0),(0,-1)))
        f,_=operands(diamond,F(1,4))
        with self.assertRaises(UnsupportedBore):difference(f,make_body('cylinder',Frame((.125,.125,0.),1.)))

    def test_cutter_span_axis_source_types_and_failure_preservation(self):
        from brepgpu.body import Frame,make_body
        from brepgpu.feature_bore import difference,UnsupportedBore
        f,c=operands();prior=difference(f,c);before=(f.solid.vertices,f.history.origins,c.patches,prior.history.origins)
        for cutter in (make_body('cylinder',Frame((0.,0.,0.),.5)),
                       make_body('cylinder',Frame((0.,0.,.125),1.)),make_body('box'),make_body('plate')):
            with self.assertRaises((UnsupportedBore,TypeError)):difference(f,cutter)
        for source in (f.solid,f.profile,c,object(),None):
            with self.assertRaises(TypeError):difference(source,c)
        for cutter in (None,f,object()):
            with self.assertRaises(TypeError):difference(f,cutter)
        self.assertEqual(before,(f.solid.vertices,f.history.origins,c.patches,prior.history.origins))
        self.assertEqual(difference(f,c).solid.vertices,prior.solid.vertices)
        too_tall,_=operands(maximum32(),F(2))
        with self.assertRaises(UnsupportedBore):difference(too_tall,make_body('cylinder',Frame((0.,0.,0.),2.)))

    def test_all_original_map_paths_and_foreign_keys(self):
        from brepgpu.boolean import EntityKey
        result=self.result();profile=result.feature.profile;body=result.solid
        for origin in result.composed_history.origins:
            self.assertTrue(origin.paths)
            for path in origin.paths:
                self.assertEqual(path.steps[0].target,origin.target)
                if path.root.body_id==profile.body_id:
                    self.assertEqual(len(path.steps),2)
                    self.assertIn(path.steps[1].target,path.steps[0].parents)
                    self.assertIn(path.root,path.steps[1].parents)
                else:self.assertEqual(path.root.body_id,result.cutter.body_id)
        n=len(profile.vertices)
        lower=result.composed_history.origin(EntityKey(body.body_id,1,'face',1)).paths[0]
        self.assertEqual(lower.steps[1].local_translation,(F(0),F(0),-result.feature.height/2))
        self.assertEqual(lower.steps[1].source_uv,((F(0),F(1),F(0)),(F(0),F(0),F(1))))
        side=result.composed_history.origin(EntityKey(body.body_id,1,'face',3)).paths[0]
        self.assertEqual(side.steps[1].axial,(-result.feature.height/2,F(0),result.feature.height))
        self.assertEqual({p.root.kind for p in result.composed_history.origin(EntityKey(body.body_id,1,'face',3)).paths},
                         {'edge','coedge'})
        for history in (result.history,result.composed_history):
            for bad in (EntityKey(body.body_id+1000,1,'face',1),EntityKey(body.body_id,2,'face',1),None,True):
                with self.assertRaises(KeyError):history.origin(bad)
                with self.assertRaises(KeyError):history.from_input(bad)

    def test_factory_immutability_and_source_guards(self):
        from brepgpu.feature_bore import BoreResult,BoreSolid,ComposedHistory
        from brepgpu.meshing import tessellate as canonical
        from brepgpu.cut_meshing import tessellate as old_cut
        result=self.result()
        for cls in (BoreResult,BoreSolid,ComposedHistory):
            with self.assertRaises(TypeError):cls()
        for obj,name in ((result,'feature'),(result.solid,'radius'),(result.history,'origins'),
                         (result.composed_history.origins[0].paths[0],'root')):
            with self.assertRaises((FrozenInstanceError,AttributeError,TypeError)):setattr(obj,name,None)
        with self.assertRaises(TypeError):canonical(result.solid,tolerance=1)
        with self.assertRaises(TypeError):old_cut(result,tolerance=1)

    def test_independent_history_and_source_map_faults(self):
        from brepgpu.feature_bore_oracles import qualify_result
        result=self.result();history=result.history;first=history.origins[0]
        # Reciprocal tables can be consistently wrong: exact target-role check catches it first.
        bad_parent=replace(first.parents[0],entity_id=2)
        bad=replace(first,parents=(bad_parent,))
        corrupt=changed(result,history=changed(history,origins=(bad,)+history.origins[1:]))
        with self.assertRaisesRegex(AssertionError,'independent original source role/maps'):qualify_result(corrupt)
        origin=next(o for o in history.origins if o.target.kind=='chart' and o.target.entity_id==1)
        bad=replace(origin,source_uv=(F(0),F(3,4),F(0),F(1)))
        origins=tuple(bad if o is origin else o for o in history.origins)
        with self.assertRaisesRegex(AssertionError,'independent original source role/maps'):
            qualify_result(changed(result,history=changed(history,origins=origins)))
        co=result.composed_history;first=co.origins[0];path=first.paths[0]
        wrong=replace(first,paths=(replace(path,steps=path.steps[:1]),)+first.paths[1:])
        with self.assertRaisesRegex(AssertionError,'composed path lost actual original source/map owner'):
            qualify_result(changed(result,composed_history=changed(co,origins=(wrong,)+co.origins[1:])))


if __name__=='__main__':unittest.main()
