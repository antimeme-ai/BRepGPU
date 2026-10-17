# Model external reader review

Root actual-source review accepted reuse of the existing component numerical and
scene structural readers, complete BOM/source mapping and publication checks.
One finding was that faces.tsv extra query/definition rows could be ignored by
per-definition filtering. The reader now requires the exact complete canonical
(query, definition, ORIGINAL face) word for all qualities before any filtering.
The SAME saved outputs pass this stronger inventory check; no native rerun.
The scene debug-output hook skips only the already-qualified fixture-only jets
and projected records, after the component reader has checked original geometry.
No production output or numerical policy changed.
