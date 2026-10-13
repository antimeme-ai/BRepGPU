# Reference source restoration

Reference implementations are study material in ignored `quarantine/`.
The [35-entry catalog](papers/reference-catalog.tsv) records upstream repositories,
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

The owned standard-library [importer](scripts/ingest_zip.py) preserves file bytes
and executable bits, rejects unsafe paths and special files, and omits nested
Git metadata, filesystem detritus and symbolic links. Omitted links remain in
the intact archive. GitHub source ZIPs do not contain submodule checkouts or Git
LFS object payloads; those need separately recorded acquisition.

Archived `AGENTS.md`, skills and other instructions are historical reference,
not current project instructions. References retain their own licenses. No
reference code is shipped in this repository or selected as a production
dependency by this acquisition.
