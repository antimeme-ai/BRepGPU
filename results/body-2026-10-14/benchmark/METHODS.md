# Actual body-sampling timing scope

24 cases, separate fresh process per case, fixed serial CPU1/CPU24/P1000/P1001 order. One cold/preparation observation, independent exact initial qualification, one excluded warmup and five reused calls. Every reused call checks native status, source provenance and finite nondegenerate normals. Exact initial qualification is outside reused timing; no exported point files are written in timed calls.

Reused wall retains the immutable body, prepared grid/classification, Engine and uploaded PatchSet. Fresh query packing/upload, evaluate, Python jet/provenance download, host conversion/normal/scatter and child cleanup are included. CPU24 changes native workers, not the serial Python stages. actual_reused_caller_ms is measured outside sampler.sample and includes final batch publication; the internal reused_wall_ms ends through child cleanup. Cold wall includes body/grid, first Engine/PatchSet and cleanup. Case first use is not a cold filesystem/driver cache claim. Independent phase medians are not additive.

CUDA input20/output76 bytes per evaluated row are RAM/VRAM payloads, not network traffic or total host copies. Native geometry upload is 16-byte DevicePatch plus homogeneous controls per patch; body adjacency tables remain on CPU. Download also reads retained host provenance136 bytes/descriptor plus20 bytes/query. Conversion and Python output allocation add host traffic.

No owned copy/build/qualification/performance work runs concurrently with this serial matrix. The host is shared, unreserved and clock-uncontrolled; five-sample ranges are descriptive. This caller does not change reference-kernel Booleans. Operation logs remain in ignored context; raw numerical samples are public.
