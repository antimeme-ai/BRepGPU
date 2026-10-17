# STEP shape-processing lifecycle correction

2026-10-15, focused source-policy design for review. This supersedes the earlier
claim that a pre-ReadFile empty-operation assignment disabled shape processing.
Root stopped V3 by TERM after the source-export orientation check exposed the
defect. V3 is not a PASS. Its logs, old source snapshots and executable pins
remain retained; no product file has been changed for this correction yet.

## Concrete failure and primary source

The stronger reference exporter checks original `ADVANCED_FACE.SameSense`, the
face's direct transfer binding and its actual use in the exported solid.
The fresh whole export fails at face172770, solid1808, carrier4614: original
and direct transfer face orientation are reversed, while the solid use differs.
`context/om10-reference-source-whole-v2.txt` retains that actual failure. V1
preceded the actual-use gate and is not the final qualified reference dataset.

Read the pinned OCCT revision
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`:

1. `XSControl_Reader::SetShapeProcessFlags` obtains the current transfer actor
   and only changes it if non-null. `XSControl_TransferReader::Actor` requires
   a non-null model to create that actor.
2. `XSControl_Reader::ReadFile` calls `XSControl_WorkSession::InitTransferReader(4)`.
   That calls `SetTransferReader` even for the same reader; `SetTransferReader`
   calls `TR->SetController(myController)` unconditionally.
3. `XSControl_TransferReader::SetController` nullifies `myActor` and clears
   transfer results. Thus a pre-read assignment is not retained across read.
4. Transfer calls `XSControl_Reader::InitializeMissingParameters`. If the actor's
   processing-flags `second` (explicitly supplied) is false, it installs the
   default shape-processing flags.
5. `STEPControl_ActorRead` applies `XSAlgo_ShapeProcessor::ProcessShape` to
   mapped manifold shapes using those flags. `ShapeProcess::Perform` with an
   actually empty operation bitset has no operators and returns without work.

Both the existing owned source adapter and the first new reference exporter
assigned empty flags **before** `ReadFile`, then labeled their output
`shape-processing=none`. That label was unsupported. Independent original raw
geometry checks still describe the actual old extracted coefficients; they do
not establish that an unrelated foreign processing policy was disabled.

The ordinary manifold `StepToTopoDS_TranslateFace` sets raw SameSense and
`TranslateShell`/`Builder` add faces directly to forward shell/solid containers.
The special nonmanifold reused-face reversal is inactive in the manifold path.
Do not normalize the exported shape or discard the failed orientation check
to conceal a lifecycle error.

Primary source: [pinned OCCT tree](https://github.com/Open-Cascade-SAS/OCCT/tree/b8f597c677811d1f9f4d8a97f5ae2825c0353a42).
An ignored one-process probe records the actual before/read/set/transfer flag
states and original face172770 root/shell/local/composed orientation. Actual
probe observations will be appended before implementation; the source trace
alone does not claim a completed corrected whole-model import.

Actual retained probes now establish the defect and correction. The old recipe
reports supplied0 before read, after the attempted pre-read assignment and
after read; transfer then reports supplied1 with operation bits
`001000000000000000`. Face172770 has root/shell FORWARD, bound REVERSED but
local/composed solid use FORWARD. An assertion that the actual post-transfer
bitset is empty fails in `context/step-policy-oldrecipe-red.txt`.

The corrected probe sets flags after read: supplied1 and all18 bits zero both
before and after transfer. The same original face then has bound, local and
composed REVERSED orientations under the forward root/shell. Logs are
`context/reference-flags-old-probe.txt` and `context/reference-flags-probe.txt`.
The direct reference test additionally fails before implementation because the
old executable cannot emit asserted actual pre/post policy state; its log is
`context/om10-source-policy-reference-red.txt`. These are actual lifecycle
observations, not a whole-model corrected qualification claim.

## Selected correction and actual metadata

For **both** native adapters, set an explicitly empty operation bitset only
after successful `ReadFile` and unit handling, immediately before `Transfer`.
Require `GetShapeProcessFlags().second == true` and `.first.none()` immediately
before transfer and again after successful transfer. If either check fails,
raise with stage and actual supplied/operation state before publishing output.
Do not depend on defaults, shape-fix parameter values or a success status.

Keep existing source units, geometry extraction and full topology checks. The
product source `META____` revision string will name this corrected lifecycle
and actual asserted zero-operation state. The reference export metadata names
the same actual state. The binary schemas need no new fields: the revision
string records the applied adapter policy, while the fixed flags remain zero
and the caller retains each executable/source pin. Print the before/after
supplied/operation counts in the adapter's human diagnostic log.

The reference exporter keeps the strict original/direct/actual solid orientation
checks. If a corrected no-processing transfer still disagrees, retain that
separate actual failure and study it; do not broaden admission speculatively.
No silent topology or curve repair is authorized.

## Fresh source delta and V4 decision

After focused greens, extract a new complete owned source snapshot and complete
reference export from fresh no-processing transfers. Retain old snapshots; do
not overwrite or relabel V1/V2/V3 artifacts. Compare exact original identities,
product/style/usage metadata, all original geometry coefficients, parameter
ranges, original/translated topology diagnostics and recovered UV records. Run
the existing independent raw geometry/placement/assembly qualifiers on the new
owned snapshot. The new reference manifest uses the existing independent raw
entity/assembly mechanisms; it cannot stand in for full carrier qualification.

Report actual changed fields/families and the first unsupported import stage,
if any. Recovered pcurves are foreign data and may legitimately change when the
unintended processing is removed; source IDs and whole original geometry must
remain grounded in the supplied STEP. Existing corrected mesher capabilities
remain unchanged unless actual new source behavior justifies a separate reviewed
unit. Root selects a fresh complete V4 only after the actual delta, affected
tests and independent code/source reviews close.

The exporter/reference geometry helper is paused behind this coherent source
policy unit. No actual watch raster or benchmark is authorized by this plan.
