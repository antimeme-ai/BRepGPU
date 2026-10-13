# First P100 numerical unit delivered

- Final CPU, P100 device 0 and device 1 exact/analytic qualification passed:
  1,749 requests and 19,170 component checks per backend.
- All 27 current caller cases passed on each P100; the CPU invocation had one
  inapplicable GPU restoration case. Both final device memory suites reported
  zero errors and zero leaked allocations.
- Six fresh-process modeled native failure scenarios passed ASan/UBSan. The
  allocation/fatal-cleanup composition has a precise failing old-core result
  and a passing corrected-core result; no hardware fault was induced.
- Published matrix covers 144 cases, seven measured samples after a warmup,
  with 4,680 raw phase rows. Kept build/test jobs outside the published matrix.
  Native CPU and CUDA public calls include fresh result allocation and completion.
- One million full-jet coherent queries: CPU24 resident 111.498 ms; P1000
  6.115 ms. Prepared native host-to-host: 168.977 ms versus 85.214 ms.
  Transfers limit the end-to-end gain; tiny batches favor CPU.
- Both polynomial and rational-cylinder Python samplers executed on the P100
  with analytic checks and exact input/query metadata. Polynomial CPU also passed.
- Feature branch retains separate plan, implementation, qualification, failure,
  benchmark, review, results and documentation milestones. Source commit
  15dd0f439dfd5d7a2a6e69845d3daf396883cfd0 matches the public source manifest.
