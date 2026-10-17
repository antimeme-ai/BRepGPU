# Reference source restoration

Reference implementations are study material in ignored `quarantine/`.
The [67-entry catalog](papers/reference-catalog.tsv) records upstream repositories,
exact revisions, archive filenames and SHA-256 digests, clean destinations,
archive roots, retained file counts and omissions. The matching
[source bibliography](papers/reference-snapshots.bib) is public.

In the operator workspace, `quarantine/sources` and `quarantine/archives` link
to the already acquired shared collection. Intact ZIP archives remain available;
the links and extracted source are excluded from Git. A standalone checkout can
restore selected references into directories with the catalog's relative paths.

## Restore the USD BRep reference

```sh
mkdir -p quarantine/archives quarantine/sources
curl -fL \
  https://codeload.github.com/NVIDIA-Omniverse/usd-brep/zip/c9c5979f04cea8c38213004d4f04df0dcfb40d05 \
  -o quarantine/archives/usd-brep-c9c5979f04ce.zip
printf '%s  %s\n' \
  d3f990ac48c36b0f569920eadfbbf9dfa08ff4a7c91716a1f0a2893ad5c4b448 \
  quarantine/archives/usd-brep-c9c5979f04ce.zip | shasum -a 256 -c -
python3 scripts/ingest_zip.py \
  quarantine/archives/usd-brep-c9c5979f04ce.zip \
  quarantine/sources/usd-brep \
  --root usd-brep-c9c5979f04cea8c38213004d4f04df0dcfb40d05 --skip-symlinks
```

The destination must be new. For another catalog entry, use its repository and
revision with GitHub's codeload ZIP endpoint and substitute its recorded paths,
archive root and digest. If an upstream ZIP digest changes, preserve both
archives and investigate before replacing a reference.

The 2026-10-17 fast-mesher study added 28 entries, restored the same way except:
the OCCT test dataset (`.tar.xz`, extract with `tar -xJf --strip-components 1`),
the Mmg CI corpora and TetGen (`.tgz`/`.tar.gz`, extract with `tar -xzf`),
Triangle (flat netlib ZIP, extract with `unzip` into a new directory), Shewchuk's
`predicates.c` (single file) and ten selected Blender files listed in
`quarantine/archives/blender-selected-3f4216e1bad1/SHA256SUMS`. Exact URLs and
commands are in the Appendix A sections of
[the CAD test-plan study](papers/2026-10-17-mesher-test-plans-cad.md) and
[the mesh-generation test-plan study](papers/2026-10-17-mesher-test-plans-meshgen.md).

The owned standard-library [importer](scripts/ingest_zip.py) preserves file bytes
and executable bits, rejects unsafe paths and special files, and omits nested
Git metadata, filesystem detritus and symbolic links. Omitted links remain in
the intact archive. GitHub source ZIPs do not contain submodule checkouts or Git
LFS object payloads; those need separately recorded acquisition.

Archived `AGENTS.md`, skills and other instructions are historical reference,
not current project instructions. References retain their own licenses. No
reference code is shipped in this repository or selected as a production
dependency by this acquisition.

The first-design study additionally acquired pinned OCCT, SISL, openNURBS and
LTCSBezier under `quarantine/new-iteration/`, with intact archives and clean
extractions. Their primary source/hashes are in the
[numerical catalog](papers/first-iteration-numerical-sources.tsv) and
[acceleration catalog](papers/acceleration-roadmap-sources.tsv).
[OCCT restoration](papers/acceleration-roadmap-reference-restoration.md) and
the numerical catalog's codeload URLs restore those sources with the same owned
importer. GPU-NURBS remains public-browser/paper study only, without a source
archive or adopted code.

## Wafer and GPU MODE

The requested study acquired Wafer's GPU performance resource index and the
GPU MODE lectures it explicitly links, at pinned revisions. Intact ZIPs and
clean extracted trees are in `quarantine/wafer-gpu-mode/`; reference scripts
were not executed and no dependency was adopted.

[Archive catalog](papers/wafer-gpu-mode-sources.tsv) records revisions, sizes,
digests, destinations and source URLs. [Restoration commands](papers/wafer-gpu-mode-reference-restoration.md)
use the same owned safe ZIP importer. [Inspected files](papers/wafer-gpu-mode-entrypoints.tsv)
and [study findings](papers/2026-10-14-wafer-gpu-mode-study.md) record the actual
read scope. GPU MODE includes Apache-2.0 licensing; Wafer's README declares MIT
but its pinned archive has no LICENSE file. Acquisition selects neither for
product reuse.

## NVIDIA OM10 CPU reference package

Acquired2026-10-15 from NVIDIA's official v1.0.0 release for isolated reference
execution. The original archive is44,445,717bytes, SHA-256
`a30bcd6f1c9028bd29092288e0963d5450ab2b9105fc92a034a6c8ce9d8de2b3`.
The annotated v1.0.0 tag resolves to source commit
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`, already retained above; the
binary asset's build identifier is `1.0.0.v1-0-0.7419.dbb624c8`.

```sh
mkdir -p quarantine/archives
curl -fL 'https://github.com/NVIDIA-Omniverse/usd-brep/releases/download/v1.0.0/usd_brep%401.0.0.v1-0-0.7419.dbb624c8.gl.manylinux_2_35_x86_64.release.7z' \
  -o quarantine/archives/nvidia-usd-brep-1.0.0-linux.7z
printf '%s  %s\n' a30bcd6f1c9028bd29092288e0963d5450ab2b9105fc92a034a6c8ce9d8de2b3 \
  quarantine/archives/nvidia-usd-brep-1.0.0-linux.7z | shasum -a256 -c -
mkdir quarantine/nvidia-usd-brep-1.0.0-linux
tar -xf quarantine/archives/nvidia-usd-brep-1.0.0-linux.7z \
  -C quarantine/nvidia-usd-brep-1.0.0-linux
```

Use a tar implementation with7z support; the extraction destination must be
new. No nested Git metadata or filesystem detritus was found in the extracted
package. Retain `PACKAGE-LICENSES/LICENSE`, `THIRD_PARTY_NOTICES.md`, and all
bundled third-party licenses. NVIDIA-authored components are Apache-2.0 except
as stated there; OpenUSD and other bundled components keep their own licenses.
Historical archived instructions are reference material. Package binaries differ
from the installed usd-brep1.0.0 wheel and must run in their own library path;
do not combine the package's static bridge with arbitrary wheel libraries.
The package is a reference toolchain, not a BRepGPU product dependency.

## Isolated live Python diagnostic

Acquired 2026-10-15: Ben Frederickson's `py-spy` 0.4.1 Linux x86-64 wheel
from PyPI, solely to observe the long-running owned watch qualifier without
pausing it. No package installation or product dependency was introduced.
The intact wheel and its extracted executable remain ignored under
`quarantine/py-spy-diagnostic/`; upstream uses the MIT license.

Wheel: `py_spy-0.4.1-py2.py3-none-manylinux_2_5_x86_64.manylinux1_x86_64.whl`,
SHA-256 `6a80ec05eb8a6883863a367c6a4d4f2d57de68466f7956b6367d4edd5c61bb29`.
Source and usage: <https://github.com/benfred/py-spy/tree/v0.4.1>.

Restore on the Linux diagnostic host:

```sh
mkdir -p quarantine/py-spy-diagnostic
python3 -m pip download --only-binary=:all: --no-deps py-spy==0.4.1 \
  -d quarantine/py-spy-diagnostic
```

Verify the recorded digest, then extract only the wheel's single path ending
`/py-spy` with Python's standard-library `zipfile`, writing it as `py-spy` in
the same ignored directory and marking it executable. Preserve the wheel.
Use `dump --nonblocking` or a bounded `record --nonblocking` without locals,
native frames or process launches. Nonblocking samples can be incomplete;
these are phase observations, not full profiles or benchmark measurements.

## Direct CDT source study, 2026-10-16

Acquired artem-ogre/CDT revision `53eff288a4e77ad1c0afe1a10f60bd2b57f877b8`
from <https://github.com/artem-ogre/CDT>, for static reference study only.
Author: Artem Ogurets and upstream contributors; MPL-2.0. No dependency selected.
Intact archive `quarantine/direct-cdt-study/CDT-53eff288a4e7.zip`, SHA-256
`9ccfb86a1e388b48af1401007734bad984591b0928c52c6ef51b2fcd9686b0af`.
Clean extraction: `quarantine/direct-cdt-study/CDT-53eff288a4e7`.
Archive bytes retained; extraction omits nested Git metadata, filesystem detritus
and symlinks. Upstream licenses remain. Archived instructions are historical.

Restore into a new destination:

```sh
mkdir -p quarantine/direct-cdt-study
curl -fL https://codeload.github.com/artem-ogre/CDT/zip/53eff288a4e77ad1c0afe1a10f60bd2b57f877b8 -o quarantine/direct-cdt-study/CDT-53eff288a4e7.zip
shasum -a 256 quarantine/direct-cdt-study/CDT-53eff288a4e7.zip
python3 scripts/ingest_zip.py quarantine/direct-cdt-study/CDT-53eff288a4e7.zip quarantine/direct-cdt-study/CDT-53eff288a4e7 --root CDT-53eff288a4e77ad1c0afe1a10f60bd2b57f877b8 --skip-symlinks
```

Compare the printed SHA-256 to the digest above before extraction.
The [algorithm-selection study](papers/2026-10-16-direct-cdt-algorithm-selection.md)
records exact source functions studied and the paper acquisition journal.

## Current GPU CDT source study, 2026-10-16

Acquired `yingtix/gCDT` revision
`c5252b1eac5d0511af00f8ae3218bfc03a96e871` for static reference study.
Intact archive `quarantine/archives/gcdt-c5252b1eac5d.zip`, SHA-256
`dd5494e9f4dd4b9cfbc29bdc3b688d294d49a5f950b763a8346541cbffef69bd`.
Clean extraction `quarantine/sources/gcdt`,24files, has no nested Git metadata
or filesystem detritus. No top-level license was present; the README separately
attributes an included integer header. No code was copied into production and
no dependency or reference executable was adopted/run. The
[GPU study](papers/2026-10-16-gpu-cdt-frontier.md) records the author PDF hash,
exact restoration commands, quantized input and actual driver/build limitations.
