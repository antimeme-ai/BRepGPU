# Interchange affected runtime/body compatibility

Ran the existing test_runtime.py (18 methods) and test_body.py (9 methods) as the
sole coordinated GPU test lane, using the unchanged pair-release core/plugin.
CPU1: 27 tests in 3.065s, OK with one documented CUDA-device-selection-only skip.
P100 device0: 27 tests in 4.278s, all passed. P100 device1: 27 tests in 4.659s,
all passed. This checks affected Patch/Engine/source-frame behavior and canonical
body storage/namespace/sampling compatibility. No source changes, native rebuild,
new-family rerun or unrelated broad checks. Released the GPU lane to root before
writing this entry so the complete caller campaign could begin.
