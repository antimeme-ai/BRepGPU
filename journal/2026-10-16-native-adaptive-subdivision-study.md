# Cached adaptive subdivision algorithm study

The operator selected cached adaptive subdivision as the primary mesher,
superseding the prior direct-CDT/refinement default. This unit reread pinned
NVIDIA SmTess/SmCurveCache/SmSurfaceCache mechanisms and Balazs2004 full-text
sections5–8, then proposed one source-cache→packets→boundary-region construction
in `papers/native-adaptive-subdivision-algorithm.md`. No foreign code/dependency
was adopted and no product code, geometry job, build or test was launched.

The substantive ownership finding is that independent leaf clipping cannot emit
new private source-boundary vertices: converting a selected PL/grid crossing to
original spatialt can change recovered UV. The selected inside-packet plus
boundary-region construction keeps physical master chains intact. Exact leaf
crossings are classification/bound partition events only. Root and geometry
reviewers agreed with that mechanism. A second progress finding is that cache
restriction does not improve an unchanged emitted triangle; region refinement
must also install a new cache-derived site strictly inside the failed triangle,
or route to the true source feature/domain limitation.

The report specifies shared exact side schedules, an O(k) no-center zipper,
cached actual-corner bilinear residual transport, and monotone packet demotion
for point-touching weak rims. Root received explicit remaining policy decisions
instead of an implementation-ready claim while those choices were open.

Acquired the original Ma/Hewitt adaptive trimmed-surface publisher PDF and read
the full paper. Original bytes/hash/read scope are retained in
`papers/native-adaptive-subdivision/acquisition.tsv`; no extraction altered the
PDF. The Kumar/Manocha author abstract was read, but its primary linked full
PostScript returned404, so no full-text claims or theorems are inherited.
