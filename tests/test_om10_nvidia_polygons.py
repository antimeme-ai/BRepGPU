"""Literal native oracles at the genuine SDK polygon extraction boundary."""
import os
from pathlib import Path
import subprocess
import unittest

ROOT=Path(__file__).resolve().parents[1]
EXECUTABLE=Path(os.environ.get('BREPGPU_OM10_NVIDIA_POLYGON_FIXTURE',
    ROOT/'build/om10-nvidia/brepgpu-om10-nvidia-polygon-fixture'))

class NvidiaPolygonExtractionTests(unittest.TestCase):
    def run_mode(self,mode):
        result=subprocess.run((str(EXECUTABLE),mode),text=True,stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,env=dict(os.environ,CUDA_VISIBLE_DEVICES='',
                PXR_WORK_THREAD_LIMIT='1',OMP_NUM_THREADS='1'))
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn('PASS',result.stdout)

    def test_literal_axis_rectangles_keep_directed_boundaries_and_parent_normals(self):
        self.run_mode('admit')

    def test_crossed_and_nonrectangular_rings_fail_closed(self):
        self.run_mode('reject')

    def test_actual_sdk_noncompletion_restores_completed_children_and_siblings(self):
        self.run_mode('restore_failure')
