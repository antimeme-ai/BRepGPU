# GPU CDT frontier: mechanisms worth reaching for

Read the original author-hosted 2026 gCDT PDF and the pinned implementation,
alongside the original NUS GPU-CDT/gDel2D project descriptions. This is reference
study, not a dependency adoption, implementation, build or benchmark.

## What the current paper contributes

Fan et al., *gCDT: A Highly Parallel GPU Algorithm for Large-Scale Constrained
Delaunay Triangulation*, author-hosted2026 manuscript: separates point insertion,
constraint enforcement and Delaunay restoration. An insertion history supports
location; ordered constraint-strip traversal feeds parallel retriangulation;
visibility pruning resolves overlapping recovery regions. Face-disjoint local
flip propagation reduces repeated whole-edge passes. These are serious
algorithm/data-flow contenders. Its reported large-input timings use RTX4090
and exclude preprocessing. Coordinates are quantized to31-bit integers and
predicates use128-bit arithmetic. They do not establish exact arbitrary-rational
source geometry or a P100 timing. The author page labels the work SIGGRAPH2026;
the retained PDF still has placeholder publication/DOI metadata. Cite that
specific manuscript rather than inventing a final DOI.

Primary sources: [author project](https://min-tang.github.io/gCDT/),
[retained manuscript source](https://min-tang.github.io/gCDT/files/gcdt.pdf),
[pinned code](https://github.com/yingtix/gCDT/tree/c5252b1eac5d0511af00f8ae3218bfc03a96e871).

## What the actual source requires

Inspected CMake, `kernel.cu`, `rawMesh.cu`, `addConstraints.cu`, `fastDelaunay.cu`
and integer/predicate headers. The distributed CMake requests70/75/80/86 and
also explicitly adds `-arch=sm_70`; P100 is60. Its current executable starts a
hardcoded9-million-point, zero-constraint random workload, with file loading
commented out. `transPoints` multiplies floating coordinates by a resolution and
casts to integers. Therefore a stock launch is neither our constrained-domain
benchmark nor an exact-input comparison. A controlled reference adapter would
need to preserve a known exactly representable integer dataset, restore source
IDs and export actual triangles for our independent reader. Source changes for
such an experiment belong in an isolated working copy with the archive intact.
The checked source sets `logPointRes=30` and stores sites in `int2`; keep that
actual configuration distinct from the manuscript's general31-bit bound.

No top-level license was found in the24-file archive; its README attributes a
bundled integer header separately. Study algorithms and own the implementation.
No source was copied into production and no reference binary was executed.

## Applying the ideas to our contract

The owned input is exact UV plus protected source constraints and a constant
positive metric. Replacing that input with a convenient integer grid changes
geometry. Instead, select integer-width specializations from proved bounds of
the lossless prepared input, or certified filters with exact fallback. Overflow
or an uncertain sign never decides topology. The GPU design must account for
metric coefficients as well as coordinate width. Original node IDs and each
collinear source segment survive every reorder and topology update.

Our general CAD workload also contains many independent small face domains.
An owned GPU contender should batch them with explicit face ownership and
flattened point/constraint/triangle ranges, rather than treating each face as
a million-site benchmark. Store dense source-slot maps and topology arrays;
use scans and compacted active lists between stages. Face/region claims precede
mutation. Deferred local work remains discoverable by the next global pass;
local scratch overflow cannot silently remove the final Delaunay obligation.
If exact wide decisions are resolved on CPU, include all pending-decision
storage, transfers, synchronization, result publication and fallback cost.

The algorithm study must compare that parallel pipeline against direct CPU CDT
on the same original domains and local-Delaunay/source oracles. Report small
single-face latency and batched throughput separately. Data-dependent rounds,
constraint strip lengths, conflict density, exact fallback and memory traffic
must explain unfavorable families. A paper's advertised throughput is a reason
to study its mechanisms, not a substitute for this comparison.

Earlier primary references: [Qi/Cao/Tan GPU-CDT](https://www.comp.nus.edu.sg/~tants/cdt.html),
2013, DOI `10.1109/TVCG.2012.307`; [NUS gDel2D description](https://www.comp.nus.edu.sg/~tants/gdel3d.html),
which explicitly distinguishes2D constraint flipping from3D CPU star repair.

## Acquisition and restoration

The archive is intact; extraction contains no nested Git metadata or filesystem
detritus. Operational files and source originals remain ignored.

- Revision: `c5252b1eac5d0511af00f8ae3218bfc03a96e871`.
- Archive: `quarantine/archives/gcdt-c5252b1eac5d.zip`,85658bytes,
  SHA256 `dd5494e9f4dd4b9cfbc29bdc3b688d294d49a5f950b763a8346541cbffef69bd`.
- Clean source: `quarantine/sources/gcdt`,24files.
- PDF: `papers/pdfs/gcdt-2026-author.pdf`,6041859bytes,
  SHA256 `ed5b7fd5b1531c9d6fed31b7bd8e452750501a2f53e673430ceeeff7c41099d8`.

```sh
curl -fL https://codeload.github.com/yingtix/gCDT/zip/c5252b1eac5d0511af00f8ae3218bfc03a96e871 -o quarantine/archives/gcdt-c5252b1eac5d.zip
python3 scripts/ingest_zip.py quarantine/archives/gcdt-c5252b1eac5d.zip quarantine/sources/gcdt --root gCDT-c5252b1eac5d0511af00f8ae3218bfc03a96e871 --skip-symlinks
curl -fL https://min-tang.github.io/gCDT/files/gcdt.pdf -o papers/pdfs/gcdt-2026-author.pdf
```
