# NVIDIA original whole-input boundary

Read-only source audit, 2026-10-17. NVIDIA USD/BRep source revision
[`c9c5979f04cea8c38213004d4f04df0dcfb40d05`](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05)
provides `occt::ConvertOcctFileToUsd`, which reads buffered bytes and calls
`ConvertOcctDataToUsd`. That function selects BinTools binary or BRepTools ASCII
parsers. It has no STEP reader; the file convenience entry caps input at256MiB.
See `source/OCCT_BREP_IMPORT/src/OcctFileConverter.cpp`.

The existing strict107-definition reference pipeline genuinely calls this
official converter and NVIDIA tessellator. Its input bridge first transfers the
unchanged original STEP once through OCCT, then serializes each original solid
as BinTools V4. This is not an isolated STEP re-export. At pinned OCCT revision
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`, `BinTools_ShapeSet::AddGeometry`
records existing edge `PCurve`, `PCurve2`, surfaces and locations; its writer
serializes those representations. This operation does not regenerate seams or
refit the original solid geometry. The owned exporter also disables shape
processing and serialized triangulations/normals.

The official `OcctFileSource::AddComponentsFromRef` supports compound roots and
accumulates child placements, but ignores failed child recursion. A successful
conversion can therefore omit malformed children before strict USD import.
`OcctStagingDriver::Run` stages the collected solids as connected components;
later `DriveFace` failures propagate. Its analytic periodic-domain/lowering
guards operate per face, including the recorded multi-turn rejection paths.
Whole-compound packaging is no demonstrated remedy for these failures.

The audited outputs establish per-definition attempts, not an official direct
whole-compound outcome. The existing exporter/assembly adapter does not retain
whole-compound definition/occurrence provenance for complete exploded delivery.
No new adapter or compound run was selected here. The fresh matched existing
pipeline remains the baseline operation after owned complete images; failures
remain failures, without claiming that every official whole-input path fails.
