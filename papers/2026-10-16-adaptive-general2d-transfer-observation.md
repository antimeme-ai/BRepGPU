# General2D fine patch after the selected transfer bound

The current implementation completed the same 1e-5 mm coupled quadratic source
at **66,049 points / 131,072 triangles**, where the earlier pinned binaries'
processes had all reached the 120 s censor. The SAME independent whole-facet and
coverage reader passed one current actual output; every current cold/warm
artifact from CPU1, CPU4 and P1000 was byte-identical.

| Current backend | Process, two queries s | Cold query s | Warm fresh query s | Aggregate reserved peak, bytes |
|---|---:|---:|---:|---:|
| CPU1 | 12.833 | 8.423 | 4.284 | 376,190,846 |
| CPU4 | 9.681 | 5.591 | 3.959 | 376,454,838 |
| P1000, CPU4 host | 9.116 | 4.777 | 3.853 | 699,488,538 |

CPU4 is the best observed parallel CPU baseline here. P1000's complete process
was about 0.565 s shorter in this one observation. These individual measurements
on the shared development machine do not establish a general speedup. They
include two fresh constructions and actual outputs, device open/close and
process exit; the external checker is excluded. Cold and warm query scope
includes actual packet construction and facet bounds, rather than returning a
stored mesh.

The selected numerical change uses the minimum of the six-coefficient residual
hull and `||D||/4 + max_corner ||B(p)-L(p)||` for the SAME actual affine triangle;
current native-adapter closure was rebuilt at the same time. The
[earlier eight-row observation](2026-10-16-adaptive-general2d-observation.md)
remains untouched. This is a separately authorized affected comparison, not a
replacement of its censors or a replay of the whole cohort. It demonstrates
current completion at the requested accuracy; without the denied old-process
profile it does not isolate every cause of the old completion cliff.

Both cold and warm exact-fallback and actual-facet-failure counters were zero in
all three current modes. Cold P1000 work included 66,049 original point evaluations,
131,071 node bounds and 131,070 control restrictions. Its device allocation peak
was 256,641,928 bytes. Cold upload/download were 101,266,544/19,443,720 bytes; warm
were 88,080,056/13,631,408 bytes. These transfers are host↔GPU, not network traffic.
Device open/close took 0.272/0.077 s. Actual artifact writing took approximately
0.037–0.040s per result. The aggregate peak column is RunResource-accounted
reserved memory, including device reservations, not process RSS. Separate
session host summed-peak upper bounds and all elapsed scopes are retained in the
[three-row TSV](../benchmarks/native_mesher/results/2026-10-16-adaptive-general2d-transfer.tsv).

The source, original formula and exact variance/covariance oracle are unchanged
from the earlier observation. Limits remain 2,048 MiB, 262,144 nodes, depth64, 120 s per
native process. Root initially authorized only current CPU1+P1000; before
publication it required the material missing parallel baseline. Exactly one
current CPU4 process was then added, using the already checked current artifact
for byte comparison, without rerunning the source oracle. No current P1001,
additional quality, larger workload or automatic tuning followed.

| Artifact | SHA-256 |
|---|---|
| Current native caller | `3664b977bb1ce8423a1a4d68d29c4fe8b0e9e39d063d37e92348eff6abea57ad` |
| Current CUDA module | `e7c942eef2955d8e8adce4a2f1ab19d4584c621ca25dbb788728b6e8747fffd8` |
| Same original source file | `162bfc54c5019ab4356b0320a3a273fbd756b132eaab2039f17b7b9c4b4b44ce` |
| Every current actual mesh | `48906d5b7b9f7053d5dac085ab2891e7f571c73c33dbf33182bbbb1cfcdedb2b` |

This remains an untrimmed parametric patch operation, not a complete B-rep,
trimmed face, watch render or external-kernel reference measurement. All raw
commands/logs/artifacts and identity tables remain private in fleet context.
