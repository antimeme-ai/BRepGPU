"""Synthetic TSV/poster integration only; no watch raster or invented public times."""
import base64
import csv
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zlib

ROOT=Path(__file__).resolve().parents[1]
SOURCE='a'*64
COLUMNS=('config','view','scope','n','status','complete_seconds','elapsed_seconds',
         'source_sha256','views_sha256','detail')


def tsv(path,header,values):
    with path.open('w',newline='') as f:
        writer=csv.writer(f,delimiter='\t',lineterminator='\n');writer.writerow(header);writer.writerows(values)


def png(color):
    def chunk(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',1,1,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b'\0'+bytes((*color,255))))+chunk(b'IEND',b'')


def fixture(root):
    benchmark=root/'benchmark';benchmark.mkdir()
    request=struct.pack('<8sII',b'BGVIEW01',1,2)
    for alpha in (0.,1.):
        request+=struct.pack('<4I19dII2d',2400,2000,2,0,alpha,1.,0.,0.,0.,1.,0.,0.,0.,1.,
            0.,0.,0.,1200.,1000.,1.,.01,.01,.01,0,0,0.,0.)
    for backend in ('cpu24','p100-0'):
        full=benchmark/('cold-00-both-'+backend);full.mkdir();(full/'views.bgview').write_bytes(request)
        tsv(full/'metadata.tsv',('field','value'),(('source_sha256',SOURCE),('tolerance_mm','.005')))
        for i,state in enumerate(('assembled','exploded')):
            (full/f'{i:03d}-{state}.png').write_bytes(png((255,0,0) if i==0 else (0,0,255)))
            tsv(full/f'{i:03d}-{state}-callouts.tsv',('label','label_x','label_y','anchor_x','anchor_y'),())
    summary=[('cold_complete_process','both','cpu24',1,123.5),('cold_complete_process','both','p100-0',1,114.5)]
    for i,state in enumerate(('assembled','exploded')):
        for backend,value in (('cpu1',101.),('cpu24',11.),('p100-0',1.1),('p100-1',1.0)):
            summary.append(('native_complete_process',state,backend,3,value+i))
    tsv(benchmark/'summary.tsv',('scope','view','backend','n','median_seconds'),summary)
    tsv(benchmark/'observations.tsv',('scope',),())
    tsv(benchmark/'benchmark-pins.tsv',('role','sha256'),())
    digest=hashlib.sha256(request).hexdigest()
    records=[dict(zip(COLUMNS,(config,'both','fresh_step_both_labeled_views','1','PASS',str(seconds),str(seconds),SOURCE,digest,'SYNTHETIC TEST ONLY')))
        for config,seconds in (('heal0',333.25),('heal1',444.5))]
    return benchmark,records


class PosterTests(unittest.TestCase):
    def compose(self,root,benchmark,records):
        table=root/'reference-outcomes.tsv';tsv(table,COLUMNS,([r[k] for k in COLUMNS] for r in records))
        output=root/'posters'
        result=subprocess.run((sys.executable,str(ROOT/'scripts/compose_om10_posters.py'),
            '--benchmark',str(benchmark),'--reference-outcomes',str(table),'--output',str(output)),
            text=True,capture_output=True)
        return output,result

    def test_both_actual_reference_rows_shared_totals_and_fixed_pose_images(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);benchmark,records=fixture(root);output,result=self.compose(root,benchmark,records)
            self.assertEqual(result.returncode,0,result.stderr)
            for i,state in enumerate(('assembled','exploded')):
                svg=ET.parse(output/(state+'.svg')).getroot();text=' '.join(svg.itertext())
                for expected in ('NVIDIA CPU / heal0','NVIDIA CPU / heal1','333.250 s','444.500 s',
                                 '123.500 s','114.500 s','SHARED TOTAL','THIS POSE','MEDIAN n=3'):
                    self.assertIn(expected,text)
                image=svg.find('{http://www.w3.org/2000/svg}image')
                embedded=image.attrib['{http://www.w3.org/1999/xlink}href'].split(',',1)[1]
                expected=(benchmark/'cold-00-both-p100-0'/f'{i:03d}-{state}.png').read_bytes()
                self.assertEqual(base64.b64decode(embedded),expected)
                self.assertEqual((output/(state+'-p100-0-native.png')).read_bytes(),expected)
            self.assertEqual((output/'reference-outcomes.tsv').read_bytes(),(root/'reference-outcomes.tsv').read_bytes())

    def test_failed_admission_and_unsupported_are_explicit_without_comparable_times(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);benchmark,records=fixture(root)
            records[0].update(status='FAIL',scope='failed_qualification',complete_seconds='777.77',elapsed_seconds='888.88')
            records[1].update(status='UNSUPPORTED',scope='unsupported',complete_seconds='',elapsed_seconds='22.22')
            output,result=self.compose(root,benchmark,records)
            self.assertEqual(result.returncode,0,result.stderr)
            for state in ('assembled','exploded'):
                svg=ET.parse(output/(state+'.svg')).getroot();text=' '.join(svg.itertext())
                self.assertIn('FAIL admission / no total',text)
                self.assertIn('UNSUPPORTED / no total',text)
                for omitted in ('777.770','888.880','22.220','speedup'):
                    self.assertNotIn(omitted,text)
                values=[element for element in svg.findall('.//{http://www.w3.org/2000/svg}text')
                        if element.attrib.get('x')=='1010' and element.find('{http://www.w3.org/2000/svg}tspan') is not None]
                self.assertEqual(tuple(float(e.attrib['y']) for e in values),(1845.,1874.,1903.,1932.))
            with (output/'reference-outcomes.tsv').open() as stream:
                retained=list(csv.DictReader(stream,delimiter='\t'))
            self.assertEqual(retained[0]['complete_seconds'],'777.77')

    def test_missing_duplicate_bad_identity_scope_and_nonfinite_times_reject(self):
        changes=({'config':'heal0'}, {'source_sha256':'b'*64}, {'views_sha256':'b'*64},
            {'view':'assembled'}, {'n':'3'}, {'scope':'native_complete_process'},
            {'complete_seconds':'nan'}, {'elapsed_seconds':'inf'}, {'complete_seconds':'-1'},
            {'complete_seconds':'500','elapsed_seconds':'400'},
            {'complete_seconds':''}, {'status':'NOT_RUN'},
            {'status':'FAIL','scope':'failed_pipeline','complete_seconds':'20'},
            {'status':'UNSUPPORTED','scope':'unsupported','complete_seconds':'20'})
        for change in changes:
            with self.subTest(change=change),tempfile.TemporaryDirectory() as directory:
                root=Path(directory);benchmark,records=fixture(root);records[1].update(change)
                output,result=self.compose(root,benchmark,records)
                self.assertNotEqual(result.returncode,0)
                self.assertFalse(output.exists(),'invalid external outcome wrote partial posters')
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);benchmark,records=fixture(root)
            output,result=self.compose(root,benchmark,records[:1])
            self.assertNotEqual(result.returncode,0)
            self.assertFalse(output.exists())


if __name__=='__main__':unittest.main()
