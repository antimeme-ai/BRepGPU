# Actual NVIDIA converter periodic-input diagnosis

Scope: the two retained failing original-definition inputs, not the owned mesher,
a whole-watch replay or a claim that every NVIDIA import route fails. No product code
was changed by this source/API study. The separate polygon-export adapter unit is not
part of this diagnosis.

## Actual binary provenance

Checked NVIDIA's GitHub release API for v1.0.0 on2026-10-17. It reports the Linux x86_64
asset `usd_brep@1.0.0.v1-0-0.7419.dbb624c8.gl.manylinux_2_35_x86_64.release.7z`,
44,445,717bytes, digest
`a30bcd6f1c9028bd29092288e0963d5450ab2b9105fc92a034a6c8ce9d8de2b3`.
The retained intact archive hashes identically. Extracting its individual library
members to stdout, without replacing installed files, gives:

- `libOCCT_BREP_IMPORT.a`:3611130d37cfd063f9a862856326459bb7abbfe41f97b4eb31f35284b03af8d4
- `libBREP_STAGING.a`:4c021cd3ba3abe8aaff5d8fc2e82d09341c2405e85547541c3c367b1f2061479

These match the installed fleet package and the actual attempt8 runtime pins. Both
literal refusal strings and the named sampling/shear helpers occur in that archive's
`OcctStagingDriver.o`. Thus the diagnosis is grounded in this unmodified release member,
not merely an adapter calling something named "official". The release binary build ID
and source-tag revision are distinct; debug source paths alone would not prove provenance.
The retained source ZIP has its recorded digest d3f990ac… and its staging source is
byte-identical to the local reference tree (5fc2b074…); no quarantine source edit was used.

Primary URLs: [release API](https://api.github.com/repos/NVIDIA-Omniverse/usd-brep/releases/tags/v1.0.0),
[release asset](https://github.com/NVIDIA-Omniverse/usd-brep/releases/tag/v1.0.0),
[pinned source](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05).

## Fresh input cause

The existing read-only OCCT probe was run on the exact current
`context/nvidia-reference-ready/watch-attempt8-matched/heal0-prepare/source/definition-*.brep`
files, on one fleet CPU. It serializes no shape and samples no mesh. Results are in
`context/nvidia-periodic-usage-diagnosis/fresh-{1716,1811}.txt`.

| Input | failing storage face | original carrier | use/pcurve counts | measured U span |
|---|---:|---|---:|---:|
|1716|28|cylinder R.045mm|6/0|6.2831992489061825|
|1811|777|cone R1.1485mm, semiangle1.48352986419518|113/0|6.2832147816647783|

The complete-loop angular winding is zero in each diagnostic. Each has a repeated
physical seam use and nominally one full circumference; declared vertices and original
spatial seam curves disagree slightly. The resulting span exceeds2π by1.39417e-5 and
2.94745e-5 radians respectively. This is the observed input to the fixed1e-5 branch,
not a proof that the encoded tolerant boundary is exactly embedded or exactly periodic.
The earlier study records the individual source values and endpoint discrepancies.

At c9c5979 `source/OCCT_BREP_IMPORT/src/OcctStagingDriver.cpp`:

- `ComputeTightPeriodicDomainFrom3dBoundary` accumulates each declared start vertex,
 97 spatial-curve samples, and declared end vertex, unwrapping near its predecessor.
- `DriveFace` preserves a repeated seam only with complete pcurves already in the
 primary angular chart. Both fresh inputs have none. The sampled span therefore drives
 `bMultiTurnU = span>2π+1e-5`.
- The cylinder's attempted sheared patch has no nonoverlapping adjacent-turn candidate;
 the cone has no multi-turn-U lowering route. The two exact errors propagate here.
- This precedes `SmApiUsdImportBreps`; its heal0/heal1 policy cannot alter this failure.

`OcctFileConverter.h` exposes text/data/file conversions, all with only data/stage/path
and error arguments. The sample `tools/occt_to_usd/occt_to_usd.cpp` accepts input and
output paths, plus schema-plugin registration. There is no public period/split/healing
setting to turn on in that API. Units and unchanged chord.005mm are not the cause: no
NVIDIA tessellation is reached.

## Intended input coverage and next supported route

The published `tools/occt_to_usd_test/test_occt_to_usd.py` exercises missing pcurves for a
PARTIAL cylinder seam, full-period primitives with repeated seam topology, and near-full
ranges±5e-7. Those tests do not establish this translated full-period chart with endpoint
gaps. Whole-compound wrapping cannot change the failing per-face calculation and risks
ignored child failures in `OcctFileSource::AddComponentsFromRef`.

`SameParameter` is already true on the raw records. Setting a flag cannot construct
missing pcurves. OCCT's `ShapeFix_Edge::FixSameParameter` is explicitly intended after
correct3D/pcurves exist and may increase tolerances, so it is not a selected shortcut.
Nor is simply retrying default STEP FixShape justified: retained historical default
transfer added pcurves but left shiftedπ..3π charts and the same spatial endpoint gaps.

The documented OCCT route worth a two-input test is explicit closed-face representation
splitting. At pinned b8f597c, shape-healing guide§4.3.4 documents
`ShapeUpgrade_ShapeDivideClosed` and `SetNbSplitPoints(1)`.
`ShapeUpgrade_ClosedFaceDivide::SplitSurface` requires finite UV bounds and two distinct
seam pcurves; raw missing-pcurve/unbounded-V inputs are not a valid direct starting point.
First use the already reviewed reference-only `prepare_source_uv` RAW copy, preserving
its original physical source guards, then split that copy into ordinary periodic charts
and pass a new derivative BRep through the unchanged NVIDIA converter. This is a named
representation-preparation policy, not SMLib heal0/1 or an unmodified-input result.

The literal bounded plan is `docs/nvidia-periodic-input-diagnostic-plan.md`. Root review
precedes diagnostic code. It retains original files, source/face history, source basis
values and all descendants; reports preparation costs and failures; changes no source
CAD, requested quality, SDK library or original definition inventory. No complete result
is claimed until the actual route has executed and existing checks accept its output.

## Actual bounded representation diagnostic

Root accepted the literal plan before the private probe was written. Builds and all
calls used one fleet socket1 physical CPU, with180s per-input bounds. No shared target,
producer, SDK library or original file was overwritten. The attempt8 producer remained
SHA4406fce4f9efb89d305e41de26d4b86be6f0b34f52433e98ee453dcaa60ffcd3,
heal0/chord.005mm; its earlier polygon gate and tessellation settings were unchanged.

The probe builds original face/loop/use bindings from BinTools storage, calls existing
`native/source_uv/source_uv.cpp::prepare_source_uv` with query=false, and splits the RAW
copy using the documented one-cut tool. It snapshots source surface coefficients and
checks every original face has descendants with identical underlying analytic or spline
basis values. All result faces belong to that history. Original vertex XYZ and original
vertex/edge-descendant tolerances agree. These checks are not a whole-source distance
certificate for the resulting mesh.

| Observation |1716|1811|
|---|---:|---:|
| Original / RAW faces |6 /6|117 /117|
| Split faces / edges / vertices |10 /25 /16|122 /352 /232|
| Retained input max tolerance,mm |6.2737832424940552e-7|1.5649075974457773e-4|
| Result max tolerance,mm |1e-5|1.5649075974457773e-4|
| Prep total,seconds,final captured run |.00788329|.104998525|
| SDK definition scope through teardown,seconds |.021315224|.623848665|
| Actual SDK outcome |conversion FAIL|PASS,2096 triangles|

For1716, the larger1e-5mm tolerances occur on newly constructed cut edges. Original
vertex and original-edge-descendant tolerances did not change. This is far below.005mm
but is **not** unchanged topology metadata; any later production reference policy must
name it. The failing cylinder still has one descendant, with full shifted U bounds
[3.1415787118631964,9.4247779607693793]. The unchanged converter refuses it at its new
storage face49. Other closed faces split. Broadly saying "split succeeded" would hide
that this particular chart was not divided.

For1811, all117 original faces are retained through122 descendants. The official
converter/import/tessellator/strict export succeeds:352 edges,232 input vertices,
1048 physical mesh vertices,2096 genuine triangles, no lamina/spine edges and reported
input/output manifold flags1. Recorded SDK stage times: conversion.049310476s,
import.025167387s, tessellation.536947214s, extraction.006401564s, write.004123615s.
These are a single-definition diagnostic, not a complete-watch reference or speedup.

Root reviewed one source-derived closing-precision amendment for1716. Its initial
closing precision1e-7mm was tighter than the declared input maximum and endpoint gap;
OCCT passes this value into `ShapeFix_ComposeShell::Init`. The single repeat used the
input maximum for SetPrecision while keeping SetMaxTolerance unchanged. The cylinder
still had one descendant. An added stricter curve-history check then stopped before
serialization/SDK: an original closed circle interval
[5.7831853071804886,12.066370614360075] maps to two same-basis descendants
[2.6415926535897931,5.7831853071804886] and
[5.7831853071804886,8.9247779607693793]. A full-circle image may be cyclically
reparameterized, so strict linear-interval equality is stronger than physical image
equality here. This guard was not silently relaxed; no second SDK success or proof of
lost geometry is inferred. No further split-policy retry was selected.

Raw/derived data and logs remain ignored under
`context/nvidia-periodic-usage-diagnosis/`. Current original input hashes:
1716 `713ca34ce7ed8d8a8fa43a7ee7aed4442b239bf68efa4966dab12e2eafb185a4`;
1811 `87182e22ad48eae2cc15bf14e5a9e08654ff26cbca7ad46c25c1eb79e0ff6095`.
The successful1811 derivative is
`70be9846d5f2a196e7a9e7ca177717226bdb22b1378a08c3429a1c5694503d81`.
The original source/refusal outcomes remain intact. Supported preprocessing can resolve
at least one actual case; it does not establish that the other is unimportable through
all NVIDIA or OCCT representation routes.

Root stopped the split route after these outcomes: no periodic-history relaxation or
further split search is selected. The private driver now has a compiled `--raw-only`
branch that serializes `prep.raw_shape` immediately after the existing RAW source guards
and inventory check, before any splitter. It reports projection/seam/chart-approximation
and finite-probe residual counters. No extra input has been run through that branch;
root will align any such control with the polygon owner's imported-boundary trace.
