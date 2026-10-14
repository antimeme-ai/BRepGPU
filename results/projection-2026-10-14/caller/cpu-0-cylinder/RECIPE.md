# Cylinder seeded projection

Backend cpu, device 0, 1 requested CPU workers, 1024 requests. Owned degree (2,1) carrier with local origin 0, scale 1, unit-square parameters. No imported body/face lineage. records.tsv preserves exact binary64 targets, seeds and returned candidate fields; controls.tsv preserves homogeneous controls. The SVG shows at most 180 target-to-candidate segments over an analytic carrier grid.

All rows met stationary status and independent exact geometry/known-solution checks. Maximum observed errors: {'point_0': 2.1512784673415843e-16, 'point_1': 2.1714078542646365e-16, 'point_2': 1.1102230246251565e-16, 'distance': 4.440893923033275e-16, 'residual': 3.681531878007365e-16}. The named target family has independently known nearest solutions; the operation itself promises seeded first-order stationarity.

Options: updates 64, backtracks 24, tolerance 0x1.0000000000000p-24. timings.tsv separates preparation, first use, native snapshots/uploads, resident solve, allocating download with host provenance, cleanup, exact qualification and exports. Device result payload is 72 bytes/request; retained-source materialization adds 44 host bytes/request plus descriptors/options and Python wrappers. These single caller samples are separate from native medians. Source CAD export/import, global search, trims/topology and automatic CPU fallback are absent.
