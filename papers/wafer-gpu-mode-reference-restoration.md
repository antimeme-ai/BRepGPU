# Wafer / GPU MODE reference restoration

Research acquisition, 2026-10-14. The intact archives are ignored; no external
reference scripts were executed. Run these commands from a BRepGPU checkout,
with Python 3 and curl available. Extraction destinations must not exist.

```sh
mkdir -p quarantine/wafer-gpu-mode/archives
curl -fL --retry 3 'https://codeload.github.com/wafer-ai/gpu-perf-engineering-resources/zip/1c52412c3e36b678b43bc414522a790a8cb0212c' -o quarantine/wafer-gpu-mode/archives/wafer-resources-1c52412c3e36.zip
curl -fL --retry 3 'https://codeload.github.com/gpu-mode/lectures/zip/77a8df418834e5789c12da23e7d2719e0efabef1' -o quarantine/wafer-gpu-mode/archives/gpu-mode-lectures-77a8df418834.zip
python3 scripts/ingest_zip.py quarantine/wafer-gpu-mode/archives/wafer-resources-1c52412c3e36.zip quarantine/wafer-gpu-mode/wafer-resources --root gpu-perf-engineering-resources-1c52412c3e36b678b43bc414522a790a8cb0212c --skip-symlinks
python3 scripts/ingest_zip.py quarantine/wafer-gpu-mode/archives/gpu-mode-lectures-77a8df418834.zip quarantine/wafer-gpu-mode/gpu-mode-lectures --root lectures-77a8df418834e5789c12da23e7d2719e0efabef1 --skip-symlinks
```

Compare archive byte counts and SHA-256 against
[wafer-gpu-mode-sources.tsv](wafer-gpu-mode-sources.tsv). The first archive is
441,166 bytes and imports 10 files; the second is 197,572,953 bytes and imports
194 files. Both ZIPs contained zero entries matching the ingestion script's
metadata/detritus exclusions. The owned script rejects unsafe paths and special
files, omits nested Git metadata, filesystem detritus and requested symlinks,
and retains the downloaded archive unchanged. No dependency adoption follows
from acquisition.

GPU MODE includes an Apache-2.0 LICENSE, copyright 2024 CUDA MODE. Wafer's README
declares MIT, but this pinned archive does not contain a LICENSE file; record
that distinction before any reuse decision. Source instructions and copied
operational comments remain historical reference material. Do not republish
private-looking operational comments from examples.

Read scope and individual file hashes are in
[wafer-gpu-mode-entrypoints.tsv](wafer-gpu-mode-entrypoints.tsv). The findings are
in [the study](2026-10-14-wafer-gpu-mode-study.md). This block is suitable for
inclusion by the owner of the central quarantine manifest.
