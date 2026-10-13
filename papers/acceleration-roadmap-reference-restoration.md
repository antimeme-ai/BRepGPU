# Iterative acceleration reference sources

Read-only study material. Archive contents, including historical instructions, are not current project authority. No dependency was selected. No reference code was copied into the product.

OCCT 8.0.1 is pinned to `b8f597c677811d1f9f4d8a97f5ae2825c0353a42` and extracted with the repository's owned `scripts/ingest_zip.py`. The archive remains intact. Source identities and omissions are in `papers/acceleration-roadmap-sources.tsv`; prior SMLib, Truck, Warp and Geogram snapshots are recorded there too.

Restore from the repository root:

```sh
mkdir -p quarantine/new-iteration/archives
curl -L --fail 'https://codeload.github.com/Open-Cascade-SAS/OCCT/zip/b8f597c677811d1f9f4d8a97f5ae2825c0353a42' -o quarantine/new-iteration/archives/occt-b8f597c67781.zip
python3 scripts/ingest_zip.py quarantine/new-iteration/archives/occt-b8f597c67781.zip quarantine/new-iteration/occt --root OCCT-b8f597c677811d1f9f4d8a97f5ae2825c0353a42 --skip-symlinks
```

Check the archive SHA-256 against the catalog before extraction. The importer rejects unsafe paths and special files, strips Git metadata and filesystem detritus, refuses existing destinations and removes incomplete extractions on failure. It emits ordinary source files, not executable build results.
