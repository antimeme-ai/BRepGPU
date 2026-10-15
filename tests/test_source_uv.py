"""Source conversion checks with literal authored geometry; fleet execution only."""
import hashlib
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT=Path(__file__).resolve().parents[1]


class SourceUVTests(unittest.TestCase):
    def adapter(self):
        return Path(os.environ.get('BREPGPU_STEP_SOURCE',
            ROOT/'build/om10-step-source/brepgpu-step-source')).resolve()

    def test_authored_missing_nonplanar_uv_is_reconstructed_without_source_changes(self):
        from brepgpu.imported_source import load_source
        from brepgpu.imported_oracles import (qualify_original_geometry,
            qualify_original_assembly, curve_point, carrier_jet)
        from brepgpu.imported_assembly import assemble
        fixture=ROOT/'tests/fixtures/source-uv.step'
        with tempfile.TemporaryDirectory() as directory:
            target=Path(directory)/'source.bgsource'
            run=subprocess.run((str(self.adapter()),str(fixture),str(target)),capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            source=load_source(target)
            self.assertEqual(source.source_sha256,hashlib.sha256(fixture.read_bytes()).hexdigest())
            self.assertEqual(tuple(d.id for d in source.definitions),(41,395))
            self.assertEqual(len(source.faces),9)
            qualify_original_geometry(source,fixture)
            qualify_original_assembly(source,fixture,assemble(source))
            uses={use.id:use for use in source.coedges}
            for face in source.faces:
                carrier=source.carrier(face.carrier_id)
                for loop_id in face.loop_ids:
                    for id in source.loop(loop_id).coedge_ids:
                        use=uses[id];edge=source.edge(use.edge_id)
                        for index in range(17):
                            t=edge.parameter_range[0]+(edge.parameter_range[1]-edge.parameter_range[0])*index/16
                            original=tuple(map(float,curve_point(source.curve(edge.curve_id),t)))
                            uv=tuple(map(float,curve_point(source.curve(use.curve_id),t)))
                            actual=tuple(map(float,carrier_jet(carrier,*uv)[0]))
                            self.assertLessEqual(math.dist(original,actual),2**-32)

    def test_query_export_and_source_bytes_remain_deterministic(self):
        from brepgpu.imported_source import load_source
        fixture=ROOT/'tests/fixtures/source-uv.step'
        with tempfile.TemporaryDirectory() as directory:
            outputs=[]
            for index in range(2):
                output=Path(directory)/f'source-{index}.bgsource'
                query=Path(directory)/f'query-{index}'
                run=subprocess.run((str(self.adapter()),str(fixture),str(output),
                    '--query-brep-dir',str(query),'--source-residual-mm','.001',
                    '--tolerance-mm','.005'),capture_output=True,text=True)
                self.assertEqual(run.returncode,0,run.stderr)
                source=load_source(output)
                self.assertEqual({p.name for p in query.iterdir()},
                    {f'definition-{d.id}.brep' for d in source.definitions})
                for path in query.iterdir():
                    self.assertTrue(path.read_bytes().startswith(b'\nOpen CASCADE Topology V4, (c) Open Cascade\n'))
                self.assertIn('source_uv_seconds\t',run.stdout)
                outputs.append(output.read_bytes())
            self.assertEqual(*outputs,'wall timing leaked into deterministic source bytes')

    def test_global_domain_control_recovery_is_bounded_and_preserves_raw(self):
        executable=Path(os.environ.get('BREPGPU_SOURCE_UV_FIXTURE',
            ROOT/'build/om10-step-source/brepgpu-source-uv-fixture')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            run=subprocess.run((str(executable),'global-domain-controls',str(Path(directory)/'query.brep')),
                capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            rows=[row.split('\t') for row in run.stdout.splitlines()]
            self.assertEqual([tuple(map(float,row[1:])) for row in rows if row[0]=='query_pc_basis'],[(2.,3.,0.,1.)])
            self.assertEqual([tuple(map(float,row[1:])) for row in rows if row[0]=='query_control'],
                [(1.,0.,0.,1.),(2.,.5,0.,1.),(3.,1.,0.,1.)])
            self.assertEqual([float(row[1]) for row in rows if row[0]=='raw_middle_v'],[-8*math.ulp(1.)])
            for row in rows:
                if row[0]!='boundary' or row[1]!='10':continue
                t=float(row[2]);xyz=tuple(map(float,row[3:6]));uv=tuple(map(float,row[6:8]))
                self.assertLessEqual(math.dist(xyz,(t,0.,0.)),2**-32)
                self.assertLessEqual(math.dist(uv,(t,0.)),2**-32)
                self.assertLessEqual(math.dist(tuple(map(float,row[8:11])),xyz),2**-32)
            bad=subprocess.run((str(executable),'global-domain-controls-too-far',str(Path(directory)/'bad.brep')),
                capture_output=True,text=True)
            self.assertNotEqual(bad.returncode,0)
            self.assertIn('natural-domain',bad.stderr)

    def test_conic_source_chart_approximation_preserves_original_curve_and_budget(self):
        executable=Path(os.environ.get('BREPGPU_SOURCE_UV_FIXTURE',
            ROOT/'build/om10-step-source/brepgpu-source-uv-fixture')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            run=subprocess.run((str(executable),'conic-boundary',str(Path(directory)/'query.brep')),
                capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            rows=[row.split('\t') for row in run.stdout.splitlines()]
            self.assertEqual([tuple(map(float,row[1:])) for row in rows if row[0]=='original_circle'],
                [(.01,0.,.04)])
            self.assertEqual([row[1:] for row in rows if row[0]=='original_uses'],[['10','11','12','13']])
            self.assertEqual([row[1:] for row in rows if row[0]=='ordered_uses'],[['4']])
            provenance=[tuple(map(float,row[1:])) for row in rows if row[0]=='source_chart']
            self.assertEqual(len(provenance),1)
            self.assertEqual(provenance[0][:4],(1.,10.,0.,1.))
            delta,charge,guide_residual,selected_residual=provenance[0][4:]
            self.assertGreater(delta,128*math.ulp(1.))
            self.assertGreaterEqual(charge,delta)  # Literal tensor plane has |Su|=1.
            self.assertLessEqual(charge,.005/8)
            self.assertLessEqual(guide_residual,.001)
            self.assertLessEqual(selected_residual,.001)
            query_charge=[float(row[1]) for row in rows if row[0]=='query_uv_metric']
            self.assertEqual(len(query_charge),1)
            self.assertGreaterEqual(query_charge[0],charge)
            self.assertLessEqual(query_charge[0],.005/8)
            raw=[row for row in rows if row[0]=='raw_boundary']
            self.assertEqual(len(raw),17)
            self.assertTrue(all(float(row[2])==1. for row in raw),'owned source retained unconstrained guide')
            boundary=[row for row in rows if row[0]=='boundary' and row[1]=='10']
            self.assertEqual(len(boundary),17)
            center=1-.01*math.cos(.02)
            for row in boundary:
                t=float(row[2]);xyz=tuple(map(float,row[3:6]));u,v=map(float,row[6:8])
                expected=(center+.01*math.cos(t-.02),.01*math.sin(t-.02),0.)
                self.assertLessEqual(math.dist(xyz,expected),2**-32)
                self.assertEqual(u,1.)
                self.assertLessEqual(math.dist((u,v,0.),xyz),.005/8)
                self.assertLessEqual(math.dist(tuple(map(float,row[8:11])),(u,v,0.)),2**-32)
            bad=subprocess.run((str(executable),'conic-boundary-too-far',str(Path(directory)/'bad.brep')),
                capture_output=True,text=True)
            self.assertNotEqual(bad.returncode,0)
            self.assertIn('source grade',bad.stderr)
            tight=subprocess.run((str(executable),'conic-boundary-tight-budget',str(Path(directory)/'tight.brep')),
                capture_output=True,text=True)
            self.assertNotEqual(tight.returncode,0)
            self.assertIn('allocation',tight.stderr)

    def test_reversed_cylinder_preserves_all_native_seam_uses(self):
        executable=Path(os.environ.get('BREPGPU_SOURCE_UV_FIXTURE',
            ROOT/'build/om10-step-source/brepgpu-source-uv-fixture')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            run=subprocess.run((str(executable),'cylinder-reversed',str(Path(directory)/'query.brep')),
                capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            rows=[row.split('\t') for row in run.stdout.splitlines()]
            self.assertEqual([row[1:] for row in rows if row[0]=='original_uses'],[['10','11','12','13']])
            self.assertEqual([row[1:] for row in rows if row[0]=='ordered_uses'],[['4']])
            self.assertEqual([row[1:] for row in rows if row[0]=='input_face_free'],[['0']])
            self.assertEqual([row[1:] for row in rows if row[0]=='query_face_free'],[['0']])
            for row in rows:
                if row[0]!='boundary':continue
                xyz=tuple(map(float,row[3:6]));u,v=map(float,row[6:8])
                self.assertLessEqual(math.dist(xyz,(.345*math.cos(u),.345*math.sin(u),v)),2**-32)

    def test_reversed_pole_keeps_actual_original_word(self):
        executable=Path(os.environ.get('BREPGPU_SOURCE_UV_FIXTURE',
            ROOT/'build/om10-step-source/brepgpu-source-uv-fixture')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            run=subprocess.run((str(executable),'sphere-octant-reversed',str(Path(directory)/'query.brep')),
                capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            rows=[row.split('\t') for row in run.stdout.splitlines()]
            self.assertEqual([row[1:] for row in rows if row[0]=='original_uses'],[['12','11','10']])
            self.assertEqual([row[1:] for row in rows if row[0]=='ordered_uses'],[['4']])
            self.assertEqual([row[1:] for row in rows if row[0]=='input_face_free'],[['0']])
            self.assertEqual([row[1:] for row in rows if row[0]=='query_face_free'],[['0']])
            connector=[tuple(map(float,row[1:])) for row in rows if row[0]=='connector']
            self.assertEqual(len(connector),1)
            self.assertEqual((connector[0][1],connector[0][3]),(math.pi/2,math.pi/2))
            for row in rows:
                if row[0]!='boundary':continue
                t=float(row[2]);xyz=tuple(map(float,row[3:6]));u,v=map(float,row[6:8])
                expected={10:(2.5*math.cos(t),2.5*math.sin(t),0.),
                    11:(0.,2.5*math.sin(t),2.5*math.cos(t)),
                    12:(2.5*math.cos(t),0.,2.5*math.sin(t))}[int(row[1])]
                self.assertLessEqual(math.dist(xyz,expected),2**-32)
                self.assertLessEqual(math.dist(xyz,(2.5*math.cos(v)*math.cos(u),
                    2.5*math.cos(v)*math.sin(u),2.5*math.sin(v))),.001)

    def test_literal_sphere_octant_pole_and_checked_failure_paths(self):
        executable=Path(os.environ.get('BREPGPU_SOURCE_UV_FIXTURE',
            ROOT/'build/om10-step-source/brepgpu-source-uv-fixture')).resolve()
        with tempfile.TemporaryDirectory() as directory:
            run=subprocess.run((str(executable),'sphere-octant',str(Path(directory)/'query.brep')),
                capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            rows=[row.split('\t') for row in run.stdout.splitlines()]
            boundary=[row for row in rows if row[0]=='boundary']
            self.assertEqual({int(row[1]) for row in boundary},{10,11,12})
            self.assertEqual(len(boundary),3*17)
            self.assertEqual([row[1:] for row in rows if row[0]=='input_face_free'],[['0']])
            self.assertEqual([row[1:] for row in rows if row[0]=='query_face_free'],[['0']])
            for _,use,t,*values in boundary:
                t=float(t);values=tuple(map(float,values));xyz,uv,actual=values[:3],values[3:5],values[5:]
                expected={10:(2.5*math.cos(t),2.5*math.sin(t),0.),
                          11:(0.,2.5*math.sin(t),2.5*math.cos(t)),
                          12:(2.5*math.cos(t),0.,2.5*math.sin(t))}[int(use)]
                sphere=(2.5*math.cos(uv[1])*math.cos(uv[0]),
                        2.5*math.cos(uv[1])*math.sin(uv[0]),2.5*math.sin(uv[1]))
                self.assertLessEqual(math.dist(xyz,expected),2**-32)
                self.assertLessEqual(math.dist(actual,sphere),2**-32)
                self.assertLessEqual(math.dist(xyz,actual),.001)
            connectors=[tuple(map(float,row[1:])) for row in rows if row[0]=='connector']
            self.assertEqual(len(connectors),1)
            a,b,c,d=connectors[0]
            self.assertEqual((b,d),(math.pi/2,math.pi/2))
            self.assertAlmostEqual(abs(c-a),math.pi/2,places=12)
            self.assertEqual([row[1:] for row in rows if row[0]=='original_uses'],[['10','11','12']])
            for name in ('sphere-v-shift','period-basis-edit','dropped-use'):
                bad=subprocess.run((str(executable),name,str(Path(directory)/(name+'.brep'))),
                    capture_output=True,text=True)
                self.assertNotEqual(bad.returncode,0,'unsupported source conversion accepted')
                self.assertIn('source UV',bad.stderr)


if __name__=='__main__':
    unittest.main()
