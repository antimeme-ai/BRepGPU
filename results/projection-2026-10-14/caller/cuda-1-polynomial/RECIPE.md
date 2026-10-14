# Polynomial seeded projection

Backend cuda, device 1, 1 requested CPU workers, 1024 requests. Owned degree (2,1) carrier with local origin 0, scale 1, unit-square parameters. No imported body/face lineage. records.tsv preserves exact binary64 targets, seeds and returned candidate fields; controls.tsv preserves homogeneous controls. The SVG shows at most 180 target-to-candidate segments over an analytic carrier grid.

All rows met stationary status and independent exact geometry/known-solution checks. Maximum observed errors: {'point_0': 1.1102230246251565e-16, 'point_1': 0.0, 'point_2': 1.1102230246249691e-16, 'distance': 9.540820720199258e-18, 'residual': 4.163336342343212e-16}. The named target family has independently known nearest solutions; the operation itself promises seeded first-order stationarity.

Options: updates 64, backtracks 24, tolerance 0x1.0000000000000p-24. timings.tsv separates preparation, first use, native snapshots/uploads, resident solve, allocating download with host provenance, cleanup, exact qualification and exports. Device result payload is 72 bytes/request; retained-source materialization adds 44 host bytes/request plus descriptors/options and Python wrappers. These single caller samples are separate from native medians. Source CAD export/import, global search, trims/topology and automatic CPU fallback are absent.
