# Raw reference mesh CPU display

Reference-only owned CPU display of supplied XYZ and oriented corner normals.
This entry never constructs/evaluates carriers and links no CUDA, OCCT, SMLib
or OpenUSD library. Actual NVIDIA import/tessellation and source correspondence
belong to the separate reference producer.

```sh
cmake -S native/om10_reference -B build/om10-reference -DCMAKE_BUILD_TYPE=Release
cmake --build build/om10-reference --parallel 8
build/om10-reference/brepgpu-om10-reference-display \
  --mesh genuine-nvidia.bgrefmesh --views actual-matched.bgview \
  --threads 24 --output-directory new-output
```

Run product/tests only on the fleet. Actual watch raster requires complete
nonvisual whole-source PASS and accepted pilot. This separate target does not
rebuild existing qualified native targets.

BGREFM01 is explicit little-endian: header64,Definition40,Instance136,
RawVertex56,Triangle24,Material32. See the [reviewed study](../../papers/2026-10-15-om10-reference-display-study.md)
for fields. RawVertex is XYZ3 and normal3 binary64 plus one u64 physical key.
Face/physical keys are external identities, not automatically STEP source IDs.
Physical keys may be0 and repeat for different corner normals at identical XYZ
within a definition; different XYZ for the same key is rejected. Definition/
occurrence IDs are unique and cannot equal UINT64_MAX. Empty instance tables
permit definition-only producer exports; the whole caller separately establishes
the actual107 definitions and166 placements.

Raw coordinates/normals remain immutable. Projection performs proper rigid
placement, authored explosion and positive unit normalization of supplied
normals. Existing BGVIEW01 and unchanged projection, setup, coverage/depth/tie,
lighting and SS resolve supply shared display. The internal Jet transport holds
supplied `.used`/`.normal` only; it is never evaluated/exported as a source jet.
Reference scheduling matches the retained CPU16x16 tile scheme.

Outputs: frame-NNNNNN.rgba/depth/primitive/instance, optional BGPROJ01/BGSAMP01,
timings.tsv and metadata.tsv. No source-jets file. Existing PNG/SVG composition
consumes these files outside this executable. Validation/allocation/projection/
display/write failures terminate; existing output destinations are rejected.

`complete_native_through_host_teardown` includes raw writes and session/source/
request destruction, excluding final timing/metadata writes and exit.
`reused_frame_wall` includes host output allocation, projection/display,
diagnostic/raw writes and Frame destruction with the scene retained. Stages
overlap. The outer reference caller must measure the complete fresh bridge→
NVIDIA CPU import/tessellation→shared owned CPU display→both PNG/labeled SVG→exit
workflow separately. Do not call this a NVIDIA renderer or substitute owned/
OCCT triangles for failed NVIDIA output.

Six focused methods in tests/test_om10_reference.py check independent scalar
pixels/depth/IDs, shared edges/opposite winding, SS2, front/ties, two retained
poses, placement/normal rotation, duplicate normal corners, bad records and one
identical-plane CPU compatibility. Run with BREPGPU_OM10_THREADS=1 or24. A separate
build with BREPGPU_OM10_REFERENCE_SANITIZERS=ON runs the same cases with address/
undefined sanitizers; this does not repeat whole-product geometry qualification.
