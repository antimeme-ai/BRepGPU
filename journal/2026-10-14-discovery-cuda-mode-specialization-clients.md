# CUDA mode selection: pre-change direct clients

- Read the accepted plan/reviews, actual adapter methods/environment names,
  discovery case constructors, slab layouts, canonical CUDA failed NaN and
  existing profile/device callers before implementing clients.
- Added tests/test_discovery_modes.py: all-mode mixed invalid/tail cases,
  canonical NaN/precedence/zero-work checks, simple analytic plane brackets,
  profile-on/off raw numerical bytes, retained children, selected caps,
  rational degree-zero/mixed original offsets, one maximum-depth traversal,
  ABI1/ABI2 analytic smoke and caller/worker prior-device restoration.
- Added scripts/check_discovery_mode_identity.py. Fresh-process capture uses
  actual selected core/plugin/benchmark hashes, existing 309-row corpus and
  focused callers; saves original input/result/source/options buffers in owned
  binary files plus TSV indexes and deterministic metadata. Compare reads raw
  bytes, reports first changed field/slab/index, preserves NaNs/signed zeros,
  excludes only duration, and validates stored profile flags/duration rules.
  Plugin hash and human label are recorded variant provenance, not equality
  conditions; core/benchmark/adapter/fixture/client/input identity must match.
- The capture client does not run the separate original-UV/cache Fraction
  campaign. The unchanged exact qualifier remains the independent geometry
  mechanism once per new plugin/card. No new failure framework or dependencies.
- Local stdlib AST parsing passed for both files; no native call, build, test
  execution, fleet job or deployment. Verified native/cuda_backend.cu remains
  SHA-256 3da09294aa1d291218344fa004ea0db638f331681ceb0fe633811e2430cda4ca.
  Product changes await actual dynamic fleet capture and coordinator clearance.
- Integrated direct source-review tightening: valid rows require exactly one
  coverage/visit-cap reason and reject arithmetic/unknown flags. Deterministic
  metadata/options/provenance serialize named declared fields (floating frame
  values preserve bits); ctypes object padding is not compared. Numerical slabs
  and original input arrays still compare their complete raw bytes.
- Reviewed and dismissed an inherited-field suggestion with the reviewer:
  the three abstract version bases declare only __init__, while each actual
  serialized concrete type declares abi_version/struct_size in its own fields.
  Named metadata already includes them; no generic inheritance walker was added.

Coordinator commands (generic selected paths; no execution here):

```sh
BREPGPU_LIBRARY="$mode_core" BREPGPU_TEST_PLUGIN="$mode_dynamic_plugin" \
BREPGPU_TEST_BACKEND=cuda BREPGPU_TEST_DEVICE=0 PYTHONPATH=python \
python3 -B -m unittest discover -s tests -p test_discovery_modes.py -v

python3 -B scripts/check_discovery_mode_identity.py capture \
  --library "$mode_core" --plugin "$mode_dynamic_plugin" \
  --benchmark "$mode_benchmark" --device 0 --label dynamic \
  --output context/mode-identity/dynamic-0

# After approved CUDA change/build, repeat capture with --label specialized,
# the new actual plugin, the same core/benchmark, and a fresh output directory.
python3 -B scripts/check_discovery_mode_identity.py compare \
  --baseline context/mode-identity/dynamic-0 \
  --candidate context/mode-identity/specialized-0
```

Repeat on device 1. Requesting unavailable CUDA is a failure; the explicit CPU
test mode skips only the device-binding case and is not GPU qualification.
