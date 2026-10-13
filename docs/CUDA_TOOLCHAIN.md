# CUDA 12 toolchain on the P100 execution host

CUDA compilation and runtime execution are qualified on both initial P100 GPUs.
The setup is user-owned and isolated under:

```sh
cad_cuda_base="$HOME/BRepGPU/toolchains/cuda-12.8.1"
cad_cuda_root="$cad_cuda_base/prefix"
```

The official CUDA 12.8.1 component archives supply nvcc, NVVM/libdevice, compiler
CRT headers, device runtime, host runtime/development headers, SDK C++ headers
and Compute Sanitizer. Source archives remain intact in `archives/`; full
extractions remain in `components/`; component licenses are retained separately.
Only the combined `prefix/` is used for compilation. `prefix/lib64` is a relative
symlink to `lib`, matching nvcc's Linux profile with the archive layout.

| Component | Pinned version |
| --- | --- |
| CUDA release manifest | 12.8.1, released 2025-03-06 |
| NVCC/compiler | 12.8.93, build `cuda_12.8.r12.8/compiler.35583870_0` |
| CUDA Runtime | 12.8.90 |
| CUDA SDK C++ headers / CCCL component | 12.8.90 |
| Sanitizer component | 12.8.93 |
| Compute Sanitizer executable | 2025.1.0.0, build 35583870 |
| Host GCC / G++ | 13.3.0 |
| CMake | 3.28.3 |
| Existing NVIDIA driver | 580.178.04 |

The four archives total 91,238,248 compressed bytes. No NVIDIA driver, system
package, shell startup file or system CUDA symlink was changed. The installation
does not include GPU math/CAD libraries or a Python binding dependency.

[Acquisition catalog](cuda-toolchain-acquisitions.tsv) records the official URLs,
component versions, sizes and SHA-256 digests. The pinned manifest digest is
`249e28a83008d711d5f72880541c8be6253f6d61608461de4fcb715554a6cf17`.

## Recreate the isolated prefix

Run on Linux x86-64 with Python 3.12 and an existing compatible host compiler.
This is the component-archive assembly used for the setup, including the required
`lib64` link. It does not invoke a package manager or a driver installer.

```sh
python3 - <<'PY'
from pathlib import Path
import hashlib, json, shutil, tarfile, urllib.request

base = Path.home() / 'BRepGPU' / 'toolchains' / 'cuda-12.8.1'
archives, components, prefix = (base / n for n in ('archives', 'components', 'prefix'))
for directory in (archives, components, prefix):
    directory.mkdir(parents=True, exist_ok=True)
origin = 'https://developer.download.nvidia.com/compute/cuda/redist/'
manifest_url = origin + 'redistrib_12.8.1.json'
raw = urllib.request.urlopen(manifest_url, timeout=120).read()
if hashlib.sha256(raw).hexdigest() != '249e28a83008d711d5f72880541c8be6253f6d61608461de4fcb715554a6cf17':
    raise RuntimeError('Pinned NVIDIA manifest has changed')
manifest = json.loads(raw)  # External NVIDIA distribution manifest.
(archives / 'redistrib_12.8.1.json').write_bytes(raw)
for name in ('cuda_nvcc', 'cuda_cudart', 'cuda_cccl', 'cuda_sanitizer_api'):
    entry = manifest[name]['linux-x86_64']
    archive = archives / Path(entry['relative_path']).name
    if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest() != entry['sha256']:
        with urllib.request.urlopen(origin + entry['relative_path'], timeout=120) as src:
            with archive.with_suffix('.part').open('wb') as dst:
                shutil.copyfileobj(src, dst)
        archive.with_suffix('.part').rename(archive)
    if archive.stat().st_size != int(entry['size']) or hashlib.sha256(archive.read_bytes()).hexdigest() != entry['sha256']:
        raise RuntimeError(f'Archive mismatch: {name}')
    target = components / name
    target.mkdir(exist_ok=True)
    with tarfile.open(archive, 'r:xz') as package:
        package.extractall(target, filter='data')
    roots = list(target.iterdir())
    if len(roots) != 1 or not roots[0].is_dir():
        raise RuntimeError(f'Unexpected archive layout: {name}')
    for child in roots[0].iterdir():
        if child.name.lower().startswith(('license', 'eula')):
            licenses = base / 'licenses' / name
            licenses.mkdir(parents=True, exist_ok=True)
            if child.is_file():
                shutil.copy2(child, licenses / child.name)
        elif child.is_dir():
            shutil.copytree(child, prefix / child.name, dirs_exist_ok=True, symlinks=True)
        else:
            shutil.copy2(child, prefix / child.name, follow_symlinks=False)
link = prefix / 'lib64'
if not link.exists():
    link.symlink_to('lib', target_is_directory=True)
print(prefix)
PY
```

The selected binaries remain NVIDIA SDK software under their accompanying
licenses. The setup uses NVIDIA's documented archive deliverables and local
assembly method. [CUDA 12.8.1 archive documentation](https://docs.nvidia.com/cuda/archive/12.8.1/cuda-installation-guide-linux/index.html#tarball-and-zip-archive-deliverables),
[release EULA](https://docs.nvidia.com/cuda/archive/12.8.1/eula/index.html).

## Build settings

Call the compiler directly, without altering the user's default PATH:

```sh
cad_cuda_root="$HOME/BRepGPU/toolchains/cuda-12.8.1/prefix"
mkdir -p context/toolchain-setup
"$cad_cuda_root/bin/nvcc" --version
"$cad_cuda_root/bin/nvcc" -std=c++17 -O2 -lineinfo --cudart shared \
  -gencode arch=compute_60,code=sm_60 -Xcompiler -fno-fast-math \
  -Xlinker -rpath -Xlinker "$cad_cuda_root/lib" \
  scripts/toolchain-probe/fp64-probe.cu -o context/toolchain-setup/fp64-probe
"$cad_cuda_root/bin/nvcc" -std=c++17 -O2 -lineinfo --cubin \
  -gencode arch=compute_60,code=sm_60 \
  scripts/toolchain-probe/fp64-probe.cu -o context/toolchain-setup/fp64-probe.cubin
context/toolchain-setup/fp64-probe 0
context/toolchain-setup/fp64-probe 1
```

These are the actual compile options used for the setup probe, with paths made
relative. They produce native `sm_60` code. The exact measured owned
[probe source](../scripts/toolchain-probe/fp64-probe.cu) and
[CMake project](../scripts/toolchain-probe/CMakeLists.txt) are published so the
qualification can be reproduced from a checkout. Built binaries/logs remain
in ignored `context/`; this is setup validation tooling, not a CAD implementation.
The probe expects exactly two compute-capability-6.0 GPUs, matching the qualified
execution host, and fails explicitly on another inventory.

Source SHA-256 digests, matching the files used on the execution host:

| File | SHA-256 |
| --- | --- |
| `scripts/toolchain-probe/fp64-probe.cu` | `7686759a33a70f571ede5873e1ef0318922a4e8abff662879e06cc3088261d70` |
| `scripts/toolchain-probe/CMakeLists.txt` | `bd2f703fe62cc107c3bf9aac815dbdc7526546558087e0ac3065586dedecf1c3` |

For CMake projects, explicitly choose the same compiler/root and real architecture:

```sh
cmake -S scripts/toolchain-probe -B context/toolchain-setup/cmake-build \
  -DCMAKE_CUDA_COMPILER="$cad_cuda_root/bin/nvcc" \
  -DCUDAToolkit_ROOT="$cad_cuda_root" \
  -DCMAKE_CUDA_ARCHITECTURES=60-real
cmake --build context/toolchain-setup/cmake-build -j2
```

The setup probe used a CMake project with CXX/CUDA languages, CUDA standard 17,
`CUDA_RUNTIME_LIBRARY Shared`, `find_package(CUDAToolkit REQUIRED)` and a
`CUDA::cudart` link. CMake configured and built successfully. Compiler support
for older architectures emits a deprecation warning; the pinned CUDA 12 toolchain
still generates the required P100 binary. [Pascal support boundary](https://docs.nvidia.com/datacenter/tesla/drivers/latest/cuda-toolkit-driver-and-architecture-matrix.html).

## Direct setup qualification

The probe transferred 257 FP64 inputs to each device, ran
`y = fma(x, x, fma(3, x, 2))`, copied results to the host and synchronized its
owned stream. For integer `n = i - 128`, input `x = n/16` has the exact analytic
expected result `(n*n + 48*n + 512)/256`. All these values are exactly representable
in binary64; the oracle used this integer numerator rather than repeating the
GPU's floating-point expression.

| Device | Capability | Loaded kernel binary | Queries | Maximum absolute error |
| --- | --- | --- | ---: | ---: |
| P100 PCIe 16 GB, ordinal 0 | 6.0 | `binaryVersion = 60` | 257 | 0 |
| P100 PCIe 16 GB, ordinal 1 | 6.0 | `binaryVersion = 60` | 257 | 0 |

`cudaRuntimeGetVersion` returned 12080; `cudaDriverGetVersion` returned 13000.
The executable resolved shared `libcudart.so.12` from the isolated prefix.
The standalone cubin's ELF machine is NVIDIA CUDA architecture.
Runtime-reported device memory was 17,059,545,088 bytes on each device;
`nvidia-smi` separately reports 16,384 MiB total per card.

Compute Sanitizer 2025.1.0.0 successfully instrumented both P100 runs:

```sh
for device in 0 1; do
  "$cad_cuda_root/bin/compute-sanitizer" --tool memcheck \
    --leak-check full --error-exitcode 3 \
    context/toolchain-setup/fp64-probe "$device"
done
```

Each run reported **0 errors and 0 bytes leaked in 0 allocations**. This establishes
that the pinned sanitizer can check this probe on P100; it is not a claim that
every current sanitizer release supports Pascal or that a future CAD evaluator
is correct. The driver remained 580.178.04 after setup.

This is compiler/runtime qualification. No CAD operation or performance speedup
was measured by the probe. Numerical and ownership contracts still require the
iteration's independent implementation and review.
