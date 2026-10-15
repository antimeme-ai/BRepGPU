# Orbital reliquary

Actual cuda device 0 evaluation: 13 independent torus
carriers, 208 degree-(2,2) patches, 60,112 samples, first jets.
All statuses succeeded; all outputs finite. Every sampled position and both
first derivatives were compared against independent exact Fraction torus maps
at the submitted binary64 parameters. See qualification.tsv for absolute errors
and existing native grades. Closed chart seams also agree in position and
oriented unit normal; raw seam parameter speeds intentionally differ.

The assembly is a visual arrangement of separate carriers, not a fused B-rep.
Depth-buffered display triangles are samples without a chord or trim guarantee.
Lighting uses actual returned derivatives; the owned display rasterizer uses an orthographic depth buffer. No performance claim is made.

controls.tsv preserves homogeneous binary64 inputs; assembly.tsv defines the
analytic recipe. Query order: patch-major, u-major/v-fast, 17×17, parameters
i/16,j/16 including endpoints. jets.f64 is native little-endian binary64,
structure-of-arrays: Sx,Sy,Sz,Sux,Suy,Suz,Svx,Svy,Svz, each 60112 values.
Status success is checked before writing. Source hashes are in sources.tsv.

Reproduce from repository root with PYTHONPATH=python and BREPGPU_LIBRARY set
to an existing qualified native library:

```sh
python3 examples/trophy_surface.py --backend cuda --device 0 \
  --output-directory results/local-trophies/orbital
```

## Actual placement and renderer revision

Evaluated on Tesla P100-PCIE-16GB device0, driver580.178.04, using the existing
qualified pair-release core/plugin without rebuilding. Native SHA256 values:

- libbrepgpu.so: b63af7f6d64ab06f2dccfdd85b27931ec008a8586313463d2d4cbac4ce8dcfd6
- libbrepgpu_cuda.so: 4aeb0d7dd124fd00044f6be7dbe7a04b104748366fd01058e9815f838d148e61

The numerical batch used the caller at commit4e62fb8; sources.tsv records that
source hash. The initial mean-depth polygon preview showed incorrect occlusion
at crossings. The final image reuses those same qualified saved jets and uses
the revised owned triangle depth buffer. render-sources.tsv records the renderer
source separately. surface.png is the transparent geometry layer; orbital.svg
embeds it with labels. assets/trophies/orbital.png was rasterized from the final
SVG with the already installed standalone rsvg-convert2.62.1 tool.
