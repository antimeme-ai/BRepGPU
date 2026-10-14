# Actual CUDA mode experiment and default restoration

- Completed scalar field and merged PR #11 with its substantive commits/branch;
  issue #8 closed. Study PR #13 is also merged. Both follow-up source branches
  incorporated the public research/measurement updates.
- Preserved actual dynamic core/plugin/native benchmark before changing CUDA.
  Three focused tests and 309-row/132-batch captures passed on both P100s.
- Built only the candidate CUDA target with the unchanged Release/numerical
  flags. Core and benchmark hashes stayed unchanged. Both cards then passed the
  new focused tests under Compute Sanitizer, zero errors/leaks, complete raw
  output/input/metadata identity, and one independent 309-row/11,415-node exact
  qualification each with zero violations.
- Added pinned cuobjdump/nvdisasm12.8.90 from the existing pinned manifest to the
  isolated CUDA toolchain, retaining verified archives, extractions and licenses.
  Cuobjdump required its companion nvdisasm; installed it before any timing.
  SASS/ELF inspection found registers206/254/210 for AABB/DOP/Bernstein, stack448
  and zero spills. Source and arithmetic were unchanged outside the CUDA delta.
- Completed112 matched native invocations covering28 conditions/card/variant,
  then24 reversed-order selected invocations for cases3/9/23/25/26/27. All seven
  samples, quality values and work histograms are retained. No owned concurrent
  build, copy, qualification or performance job ran alongside either field.
- The candidate failed the predeclared matched/both-card >=5% native-phase value
  criterion. Some signs reversed with order; capped DOP on card1 retained a
  greater-than-5% resident regression. Restored the original CUDA source and
  actual default plugin from the protected dynamic stage; verified core,
  plugin, benchmark and CUDA source hashes match original identities. The
  candidate binary remains intact in its separate stage and source at ba44794.
- Published qualification captures, independent exact artifacts, both complete
  timing fields, order/provenance/condition TSVs and owned SVGs. Operational logs,
  SASS/ELF dumps and build/staged binaries stay private. No new product library,
  mixed dispatch or scheduling policy adopted; tighter-bound work remains a
  separate proposal.
