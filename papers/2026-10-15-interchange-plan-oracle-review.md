# Interchange plan: qualification review

2026-10-15. Read `docs/INTERCHANGE_PLAN.md` against the accepted design, existing
JetBatch source/download/convert readers and native `bg1_convert`. No product
execution or implementation. The conceptual units, reference/input distinction,
source truth mechanisms and bounded cost campaign are appropriate.

## IPO1 — Enforce the actual frame operation as well as its source-error budget

Before acceptance, make explicit that every successful converted binary64
component must equal the independently correctly rounded product of the **actual
local** value and the independently computed prescribed native factor, including
the native expression's +0 addition for every component (position origin is
zero). Compare bits, including the specified zero
behavior. The separate source-error budget then checks geometric accuracy.
Merely computing a correct factor for that budget does not establish that native
conversion used it: a slightly wrong result can fit inside the native grade.
Invalid-row NaNs require the documented status/NaN semantics, not an invented
NaN payload equality requirement.

Full request binding must include actual local/world count, order/terms, observed
backend, statuses, complete descriptor fields, source slots and normalized UV
bits. Compare descriptor degree/domain/frame/lineage/offsets to the conversion
and native packing; descriptor agreement between two erroneous batches alone is
insufficient. The caller independently binds returned slot/UV data to its actual
submitted arrays before the qualifier consumes those arrays, as already planned.

Include an explicit non-exact span subtraction witness, for example source
U=[−2^-54,1]. The exact span is 1+2^-54 while the native binary64 subtraction is
1. Nonunit exact spans 5 and 2 alone do not exercise the design-review correction.
No native ABI or arithmetic change is necessary.

## Accepted aspects

The independent source Cox–de Boor and actual emitted-control Bernstein models
check distinct claims. Exact whole-net correspondence, complete source face/use
inventory and coefficient bounds supply conversion truth beyond sampling.
The real foreign mouse input, actual reference-created polynomial export and
separately authored asymmetric rational source have appropriately distinct
roles. Foreign scalar queries use their actual submitted rounded source UVs.
Parser direct tests belong to the source boundary owner; a second lexer in the
geometry oracle would duplicate a mechanism. Typed source records plus literal
parser tests and an independent foreign reader address that interface instead.

All source-byte ownership, unsupported outcomes, resource errors and trim limits
remain explicit. Qualification stays outside file-to-consumed-result wall time,
with every actual measured result still checked; named fault mutations describe
specific sensitivity. No broad dependency, new native rebuild or extra release
gate is introduced. External alpha gaps remain non-green.

## Disposition

Re-read the updated design and plan: independent bitwise converted-value checks
and complete request/descriptor/batch binding are integrated. Clarified against
the actual C++ expression that the +0 operation applies to derivative components
as well as positions; this matters for signed zero. The non-exact span witness
above belongs in the owned direct family. With those explicit implementation
requirements, the plan is accepted; no broader scope or gate is needed.
