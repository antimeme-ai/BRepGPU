# OM10: the official reference bridge's apparent multi-turn trims

The actual NVIDIA CPU producer ran all107 original definition files under both
declared SMLib healing policies. Each policy emitted102 meshes; three failures
concern the owned triangle-only export and are studied separately. This note
covers the two failures in NVIDIA's OCCT-to-USD bridge, before SMLib import:
definition1716 reports storage face28, and1811 reports storage face777. These
are serialized shape-storage indices, not local face ordinals or original STEP
IDs. Keep the actual failed outcomes; this study changes no input or policy.

## Pinned primary contracts

The official [v1.0.0 source](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05)
and matched release package are the reference. Its
[`OcctFileConverter.h`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/OCCT_BREP_IMPORT/inc/OcctFileConverter.h)
accepts serialized OCCT BRepTools/BinTools data and reports parse, topology,
staging or write failures. The converter and
[`occt_to_usd` tool](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/tools/occt_to_usd/occt_to_usd.cpp)
do not read STEP or invoke OCCT ShapeFix. No STEP-to-BRep example or explicit
ShapeFix prerequisite was found in this pinned USD/BRep source tree. Absence of
that example does not prove a general upstream input contract beyond the API.

The distinct pinned
[OCCT STEP guide](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/dox/user_guides/step/step.md)
shows ReadFile followed by TransferRoots without disabling shape processing.
Its [STEP resource](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/resources/XSTEPResource/STEP)
selects `FromSTEP.exec.op: FixShape`; this is the OCCT default recipe, not an
explicit NVIDIA bridge requirement. The actual current reference exporter
deliberately selects no processing after ReadFile and asserts that policy on
both sides of transfer. The owned source pipeline is a separate implementation.

[`OcctFileSource.h`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/OCCT_BREP_IMPORT/inc/OcctFileSource.h)
explicitly permits missing pcurves: ComputeFaceUVDomain returns an empty range
and the caller falls back. Its source preserves the two seam uses and orders
coedges into a connected directed loop. In
[`OcctStagingDriver.cpp`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/OCCT_BREP_IMPORT/src/OcctStagingDriver.cpp),
ComputeTightPeriodicDomainFrom3dBoundary unwraps each start vertex,97 spatial
curve samples and end vertex near the preceding angle. DriveFace preserves an
explicit analytic seam only when complete pcurves already lie inside primary
U[0,2π], within fixed angular tolerance1e-5. Otherwise the 3D-derived range can
replace the source range. A span above2π+1e-5 enters the multi-turn branch:
cylinders try a nonoverlapping sheared patch, while other analytic U surfaces
reject. These failures precede SMLib healing and tessellation.

## Exact read-only observations

Two tiny private probes read the existing raw binary files: the official
neutral parser identifies their storage records, and pinned OCCT evaluates the
unchanged spatial curves and declared vertices with the same97-point angular
walk. They emit text only. All recorded locations are identity. No source,
query BRep, mesh, image, producer or owned qualification binary was changed.

| Raw reference definition |1716|1811|
|---|---:|---:|
| Storage face |28|777|
| Supporting surface |cylinder|cone|
| Radius, mm |.045|1.1485|
| Cone semiangle, radians |—|1.48352986419518|
| Original wire uses |6|113|
| Available pcurves |0|0|
| Repeated physical seam storage edge |23|766|
| Unwrapped sampled minimum U |3.1415787118631964|3.1415926535897931|
| Unwrapped sampled maximum U |9.4247779607693793|9.4248074352545714|
| Excess span above2π, radians |.0000139417265963|.0000294744851921|
| Seam spatial-curve/vertex endpoint gap, mm |.000000627377696872|.0000397463431456|

Both loops explicitly traverse a physical seam twice in opposite directions
and include exactly one circumference on the opposite boundary; the whole-loop
angular winding is zero. The cylinder's top two circular intervals together
span exactly2π. The cone has an exact full-circle edge with interval[π,3π]
and a serrated outer boundary. Their small angular excess is generated where
the declared seam vertex and original spatial seam curve disagree within source
tolerance. This is evidence of one-period seam charts with tolerance gaps,
not evidence that these mechanical parts require physically overlapping
multi-turn patches. It is a finite diagnostic, not an embedding theorem.

Raw native UVBounds is U[0,2π] and unbounded V because there are no pcurves;
the official neutral model correctly has no pcurve box. Its fallback combines
spatial-curve points with declared vertices, so their small discrepancy crosses
the bridge's fixed1e-5 angular threshold. That threshold has no source-length
tolerance/radius term.

The retained historical default-FixShape export supplies all6/113 pcurves,
but its unchanged spatial curves produce exactly the same diagnostic ranges.
Its cylinder pcurve box is[3.1415787118631964,9.4247779607693793] and its cone
pcurve box is[π,3π]. Both are outside the primary period, so the source does
not take explicit-seam preservation. Thus simply assuming the default OCCT
STEP recipe resolves these failures is unsupported. No new conversion run or
reference policy was selected from this comparison.

## Next bounded unit proposed

Treat this as an OCCT-to-USD periodic-chart admission limitation, with original
source tolerance gaps as its trigger; do not call it a NVIDIA kernel or GPU
limitation. Keep both actual raw/no-processing policy failures in the report.

Before changing reference preparation, author one cylinder and one cone seam
fixture with literal single-period spatial circles, a repeated generatrix and
a declared endpoint gap. The bridge-facing design must distinguish a translated
one-period seam chart from genuine multi-turn coverage while retaining complete
physical edges, vertices and original correspondence. An upstream fix or a
documented reference-only canonical chart preparation are candidates; neither
is selected here. Merely raising1e-5, dropping a use, replacing spatial curves,
or silently enabling broad healing is not an answer.

Any later preparation policy must be named independently of SMLib heal0/heal1,
include its complete CPU cost in the reference wall, and pass original geometry,
assembly and the actual reference boundary checks. A changed bridge build must
be identified as such; an official unmodified runtime does not make a modified
interchange adapter an unmodified official converter. This study authorizes no
production change, full reference rerun or trophy rendering.
