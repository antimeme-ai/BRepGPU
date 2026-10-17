# Current native model to the isolated NVIDIA reference boundary

2026-10-17. Reviewed finite adapter plan; no NVIDIA kernel or owned mesher edits.

The reference producer already calls the official v1.0.0 isolated static
OCCT_BREP_IMPORT entry, not an owned geometry bridge. The pinned source is
c9c5979f04cea8c38213004d4f04df0dcfb40d05. Its OcctFileConverter.cpp passes the
original parsed BRep to OcctStagingDriver::Run. Two retained failures are that
official driver's multi-turn periodic-chart refusals. Two other definitions
return actual SDK residual polygon rings with proper projected crossings; the
owned exporter correctly refuses them. SmApiTessellate has no triangulate flag,
and SmPolyFace::TriangulatePlanar explicitly excludes self-intersecting input.
No ring sorting, CDT fill, geometry approximation, dropped faces or relaxed
quality is selected. Public notes keep aggregate outcomes; source IDs and raw
coordinates remain in ignored operational logs.

Implement only the Python external delivery boundary in
examples/render_om10_nvidia.py, scripts/{benchmark,qualify}_om10_nvidia.py and
python/brepgpu/nvidia_reference.py, plus focused existing test_om10_nvidia cases.
Root and the owned watch continue independently.

1. Admit a current atomically published model root and explicit query index,
   retaining legacy matched-input support. Read model.tsv, source.bgsource,
   queries.tsv, faces.tsv, query-N/{scene.bgscene,views.bgview,labels.tsv}.
   Require complete_assembly, exact complete original definition/face/occurrence
   inventory and requested .005mm quality. Validate every index word, including
   extras/duplicates, and confined existing relative file paths. Do not fabricate
   old complete_nonvisual_qualification reports.
2. Decode source scene numeric profile1 explicitly. Positions remain supplied
   native XYZ. Validate complete definition partitions, original face inventory,
   occurrence source paths/IDs and styles metadata. Compose original usage
   matrices with Fraction exactly and round once to binary64, matching native
   transport; do not require repeated-float assemble() matrix words. After that
   check, retain the native actual matrices, explosion offsets and camera bytes
   for reference display. Preserve complete source-linked labels sidecar. The
   reference's existing illustrative palette remains explicitly labelled;
   matching cameras/placement is not a claim of equal per-face styling.
3. Extend the existing witness reader to numeric profile1 without rewriting
   header bits or interpreting UV as XYZ. Geometry probes remain separate from
   image creation and reference mesh geometry never comes from the owned mesh.
4. Benchmark current-model admission is complete native output/inventory only,
   not a relabelled old qualification PASS. Fresh reference export and strict
   workers still cover ALL original definitions under each predeclared heal
   policy. A failed pipeline retains actual reports and never emits a partial
   all-watch image or complete-render timing. If complete producer output exists,
   qualification uses fresh original query BReps explicitly (capability/solver
   failures remain non-green), actual native XYZ witnesses and unchanged quality.
5. Genuine RED: the existing matched-input adapter rejects current model layout
   / numeric1 witnesses. Extend focused existing tests for current model
   transport, exact original composition, incomplete/extra inventory and profile1
   witnesses. Use saved authored native model output where available; run only
   these finite boundary tests and the existing small genuine reference caller
   compatibility test on the fleet. No heavyweight whole-watch reference run
   until parent coordinates CPU resources and owned matched outputs exist.

No new dependency, production geometry callback, source replacement or third
party kernel fork. Keep actual failure attribution and nominal setting versus
measured finite admission distinct in final outcomes.
