# Bounded complete-definition diagnosis

All six complete definitions around the fresh whole-watch trial's last face
checkpoint constructed within their 120s process deadlines. No unusually slow
complete definition was reproduced in this interval when run independently.

| Definition | Original faces / edges | Triangles | Complete mesh call | Prepare / sample | Triangulate / refine |
| --- | --- | --- | --- | --- | --- |
| 1774 | 72 / 210 | 4,400 | 5.468s | 3.755s | 1.682s |
| 1775 | 13 / 30 | 1,620 | 1.826s | 1.242s | .554s |
| 1776 | 9 / 13 | 3,668 | 2.673s | 1.564s | 1.080s |
| 1777 | 10 / 24 | 68 | .133s | .059s | .045s |
| 1778 | 37 / 102 | 13,748 | 14.559s | 10.041s | 4.486s |
| 1779 | 6 / 12 | 10,624 | 3.108s | .872s | 2.204s |

These are construction observations. Independent mesh qualification, display
and normals were **NOT_RUN**. The complete watch has not been constructed or
qualified by these runs. The table's whole-call timer also includes call overhead
beyond the internal prepare/sample and triangulate/refine phases.

Each fresh process decoded the same original whole-source snapshot in
7.10–7.27s, then selected a complete original definition. Closure validation
required zero removed foreign edge incidents and zero removed foreign carrier
faces. Original quality and the unchanged direct DSO were used; this does not
prune faces or alter the original geometry. Decode/provenance/closure times and
source-code/binary pins are retained in TSV.

The sum of six independent part mesh calls is 27.767s for 147 original faces
and 34,128 triangles; it is not a measured stage of the whole-watch run.
This bounds the suspected checkpoint interval in isolation; it does not explain
the entire earlier prefix or exclude costs dependent on whole-source size.
Do not extrapolate a whole-watch completion time from this sample.

Runs were sequential on CPU4 of the shared Xeon E5-2697v2 host, with a 120s total
process deadline per candidate and the existing default quality (0.005mm
tolerance, 0.001mm source residual). No candidate reached the 45s diagnostic
profile threshold. No profiler sample, product change, qualifier run or unchanged
whole-watch retry was needed. The separate historical original process remained
untouched.
