# Extrusion caller/reference/controller runtime review

Source-only whole-unit review of `examples/extrude_profile.py`,
`scripts/reference_extrusion.py` and `benchmarks/extrusion/run.py`, after the
sampler's seven direct methods passed CPU1/CPU24/P1000/P1001. No caller execution,
benchmark, source edits or duplicate geometry qualification by this reviewer.

Two bounded findings sent to root before measurement:

1. The optional reference's after-sweep original-curve preservation query sits
   outside every named cost row. Move its timer start before that query or give
   it an explicit stage; do not imply that the listed reference phases include
   all preservation work when they do not.
2. The caller separately invokes qualify_profile, then qualify_result, whose
   implementation already qualifies the actual retained profile and returns
   those metrics. Remove the duplicate invocation; preserve the complete result
   qualification rather than adding another layer of the same direct check.

The actual feature inputs are exact original lattice tuples with genuinely
different source sizes. Prepared-profile wall begins after make_profile, while
points wall begins just before that factory; scope explicitly starts with already
owned exact point tuples rather than claiming serialized-source decoding. Both
walls include actual result publication, material/edge/non-dyadic UV/history
reads, grid preparation, native Engine setup/sampling/checksum/cleanup, topology
packing/library load/build and actual pairs/mates/neighbor reading. Independent
truth, SVG/record output and process/module/reference overhead are outside; parent
child wall includes the child's full work. Nested spans are not additive medians.

Thirteen measured source pins include actual borrowed Boolean sampling helpers
and cut_bore export helpers; source/native/controller before/after checks and
requested/loaded core agreement are present. Complete source/output graphs,
six-kind forward/reverse history, original profile points, material/boundary/use
reads, actual candidate/sample rows and native input/CSR bytes are exported.
Returned result/profile/request/backend/device bindings and all native status/
descriptor/query checks occur before useful output consumption. Canonical types
and native binaries are not changed.

Reference source construction checks exact world endpoints and sweep distance
before passing actual binary64 to the existing capped pure linear_sweep API.
Original curve parameter domains and three actual evaluated points are compared
before/after. Returned bounds have finite exact2×3 admission, volume/manifold
comparison uses original exact shoelace/height/frame, and exact edge witnesses
plus spread native INTERIOR samples cover every face. No foreign material/history
or external alpha claim is invented. Reference execution is still pending at
source-review time; public API availability is not a passing actual experiment.

The SVG is emitted after independent qualification and uses exact output line
boundaries plus actual native points/normals. Counts come from actual source/
output records or the qualified role formula. It states no general result mesher.
No occluded surface/triangle solid or GPU feature-construction claim follows.
Final acceptance awaits root's two small corrections and source re-read.

Final disposition: re-read actual corrected files. The redundant caller
qualify_profile invocation/import is removed; full result qualification still
checks the retained original profile. Reference timer now starts before the
after-sweep preservation probes and is explicitly named
reference_input_preservation_and_measure. **Accepted with no remaining blocker.**
This is source acceptance; actual caller/reference/campaign outcomes remain
pending and must not be inferred from this review.
