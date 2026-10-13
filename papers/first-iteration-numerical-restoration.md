# Numerical reference restoration

Research snapshots only. Run the following commands from the repository root. Archives remain intact; extraction omits nested Git metadata and filesystem detritus. Do not run archived instruction files as current project instructions.

## sisl

Source: https://codeload.github.com/SINTEF-Geometry/SISL/zip/e0a82b1a851e932657f3e1af25033fbbcd33e907
Revision: `e0a82b1a851e932657f3e1af25033fbbcd33e907`
Archive SHA-256: `226967bec2d5d7a94c57e24ffa5d2ba3b92ed5b8f740e87598cbb66d8293cc2e`

```sh
mkdir -p quarantine/new-iteration/archives
curl -L --fail 'https://codeload.github.com/SINTEF-Geometry/SISL/zip/e0a82b1a851e932657f3e1af25033fbbcd33e907' -o 'quarantine/new-iteration/archives/sisl-e0a82b1a851e932657f3e1af25033fbbcd33e907.zip'
python3 scripts/ingest_zip.py 'quarantine/new-iteration/archives/sisl-e0a82b1a851e932657f3e1af25033fbbcd33e907.zip' 'quarantine/new-iteration/sisl' --root 'SISL-e0a82b1a851e932657f3e1af25033fbbcd33e907' --skip-symlinks
```

## opennurbs

Source: https://codeload.github.com/mcneel/opennurbs/zip/eb92af3ba1806b0a34a99aba0d3bda83e3d46083
Revision: `eb92af3ba1806b0a34a99aba0d3bda83e3d46083`
Archive SHA-256: `efc8fcaf42bc89434094b7ad7b4381d54f99327c66be3a995078853593f8c5c2`

```sh
mkdir -p quarantine/new-iteration/archives
curl -L --fail 'https://codeload.github.com/mcneel/opennurbs/zip/eb92af3ba1806b0a34a99aba0d3bda83e3d46083' -o 'quarantine/new-iteration/archives/opennurbs-eb92af3ba1806b0a34a99aba0d3bda83e3d46083.zip'
python3 scripts/ingest_zip.py 'quarantine/new-iteration/archives/opennurbs-eb92af3ba1806b0a34a99aba0d3bda83e3d46083.zip' 'quarantine/new-iteration/opennurbs' --root 'opennurbs-eb92af3ba1806b0a34a99aba0d3bda83e3d46083' --skip-symlinks
```

## ltcsbezier

Source: https://codeload.github.com/valasekg/LTCSBezier/zip/44fdd9838a1e87fe21de3118b1a79e7949fb7542
Revision: `44fdd9838a1e87fe21de3118b1a79e7949fb7542`
Archive SHA-256: `f9527dcb4d7bdca075efd5e7a36d567d562dd9ea0d723b9501b90dc2b87402b8`

```sh
mkdir -p quarantine/new-iteration/archives
curl -L --fail 'https://codeload.github.com/valasekg/LTCSBezier/zip/44fdd9838a1e87fe21de3118b1a79e7949fb7542' -o 'quarantine/new-iteration/archives/ltcsbezier-44fdd9838a1e87fe21de3118b1a79e7949fb7542.zip'
python3 scripts/ingest_zip.py 'quarantine/new-iteration/archives/ltcsbezier-44fdd9838a1e87fe21de3118b1a79e7949fb7542.zip' 'quarantine/new-iteration/ltcsbezier' --root 'LTCSBezier-44fdd9838a1e87fe21de3118b1a79e7949fb7542' --skip-symlinks
```

GPU-NURBS was studied only through public browser pages and its primary paper; no source archive or extraction is retained.
