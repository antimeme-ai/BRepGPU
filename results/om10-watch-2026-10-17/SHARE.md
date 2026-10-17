# OM10 share images

All four posters are **3840×4200 PNGs**, containing actual 3840×3600 renders at
2× supersampling, with source credit and separate meshing/render timing panels.

| Backend | Assembled | Exploded, all 166 part labels |
|---|---|---|
| CPU12 | [Download](https://raw.githubusercontent.com/antimeme-ai/BRepGPU/main/assets/om10/cpu12-assembled-hero-4k.png) | [Download](https://raw.githubusercontent.com/antimeme-ai/BRepGPU/main/assets/om10/cpu12-exploded-poster-4k.png) |
| Tesla P100 | [Download](https://raw.githubusercontent.com/antimeme-ai/BRepGPU/main/assets/om10/p1001-assembled-hero-4k.png) | [Download](https://raw.githubusercontent.com/antimeme-ai/BRepGPU/main/assets/om10/p1001-exploded-poster-4k.png) |

Suggested caption:

> OpenMovement OM10 through BRepGPU: 166 parts, 9,162 B-rep faces, owned C++/CUDA meshing. Supersampled 4K assembled raster: 1.81s CPU / 0.21s P100. CPU meshing still dominates the full run. Assembled + exploded, all timings: github.com/antimeme-ai/BRepGPU

The raster figures include result download, excluding shared setup, file output
and poster composition. Original cold source-snapshot-to-mesh-and-both-1920-images
times are 875.16s CPU / 872.50s P100; additional 4K replay/export is separately
timed. These are single observations. Fresh NVIDIA references each complete
103/107 definitions and fail the pinned pipeline, with no complete reference
image or render-time ratio. Full scopes and every recorded phase are in the
[report](REPORT.md).

Design: Association openmovement, OM10 MAN VA01.3,
[CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/).
BRepGPU adds tessellation, illustrative materials, lighting, explosion and labels.
Credit is embedded in each poster; retain it when sharing. Full source-derived
names are in [labels.tsv](labels.tsv). Asterisks on the exploded poster distinguish
projected mesh-center anchors from sampled visible anchors.
