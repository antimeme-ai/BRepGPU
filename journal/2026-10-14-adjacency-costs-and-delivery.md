# Adjacency complete costs, graphics and return-pass handoff

All24 fresh-process matched cases completed: three sizes, ordered/global-permuted
storage and CPU1/CPU24/bothP100s. The120 repeated timed calls and168 independently
qualified outputs produce2,544 raw rows/720 summaries. All14 measured source
hashes and the actual release binary remain unchanged throughout. No owned
copy/build/test/mutant/profiler overlapped measured workers.

For491,520 edges/983,040 uses, ordered native medians are CPU1 8.193ms, CPU24
9.063ms and P1000/1 17.325/17.008ms. Shuffled medians are100.415/19.427/21.789/
15.306ms. P1001's shuffled median is21% below CPU24 but ranges overlap; P1000
is slower. GPU construction/reader stages alone take0.315–0.317ms ordered and
1.397–1.419ms shuffled, around17.6–17.7x/11.4–11.6x CPU24. Stage sums were
computed periteration before medians, with initialization-placement caveats.
Full typed consumers remain4–5s and no full gain is established. Packing takes
8–9s and global permutation adds8s. Large independent qualification costs16–24s
initially and10–16s perrepeat outside named cold/fullcaller wall. All these
scopes and ranges are published rather than treating one as another.

Each large GPU call uploads11,796,480B and downloads15,728,644B; source identity,
geometry/loop/UV transfers are zero. P1000's shuffled upload8.53ms differs from
P1001's3.90ms, without assigning NUMA/hardware causation. Future native consumption,
residency/workers and alternative scans are queued AFTER the broad-pass rocks.

Saved caller and separate performance/topology/runtime reviews accept actual
arrays, source roles, all120 diagram labels, raw-derived summaries and graphics.
Final report explicitly separates transient fleet qualification of168 matrix
outputs from offline reconstruction of four saved binary/SVG callers, and notes
same-coordinate primitive occurrences rather than a complex single solid.
CMake's cached global architecture52 differs from target60-real; actualcompiler
flags and cuobjdump confirm sm_60, tied to the measured library hash. Final
wording distinguishes CUDA upload from CPU zero-transfer and percent lower
median from ambiguous speed ratios. All valid findings are resolved.

README now shows actual seam incidence and complete native-cost graphics; root
visually inspected both through local SVG rendering, without local product
execution. API, topology, roadmap, issue index and research index identify the
working GPU adjacency beachhead. Journals/source notes retain round-two work.
PR24 will be finalized and merged with substantive feature commits preserved;
merged branch and old fleet binary archives remain available.
