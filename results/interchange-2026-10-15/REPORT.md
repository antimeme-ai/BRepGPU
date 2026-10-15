# Reference-to-owned carrier interchange

The breadth beachhead now reads actual foreign static ASCII USD BRep carrier
records into owned patches, retains every original face/use and explicit
unsupported outcome, and consumes positions/first derivatives on native CPU
and both P100s. It does not interpret trims or import a solid. No new dependency,
reference-library adoption, native ABI or GPU kernel was introduced.

[User guide](../../docs/INTERCHANGE.md) · [Design](../../docs/INTERCHANGE_DESIGN.md) ·
[Reviewed plan](../../docs/INTERCHANGE_PLAN.md) · [Source inputs/license](../../data/interchange/README.md).

## Actual inputs and independent geometry checks

| Input | Source faces | Converted carriers | Unsupported | Queries per caller |
| --- | ---: | ---: | ---: | ---: |
| Unchanged upstream mouse.usda | 4 | 2: original faces1/2, quadratic×linear and bilinear | 2 multispans, original faces0/3 | 578 |
| Actually constructed/exported foreign polynomial sheet | 1 | 1 bicubic | 0 | 289 |
| Owned literal asymmetric variable-weight degree-(2,3) fixture | 2 | 2 equal-geometry carriers with distinct source/use identity | 0 | 578 |

The original upstream mouse file is pinned by content hash and retained unchanged
with its Apache2 license. Every face is recorded; all four source trims are
`general` and remain **uninterpreted**, including on the accepted carriers.
The full knot domain is the carrier domain; the face's bounding range is not a
membership test. Analytic, multispan, unclamped, high-degree and out-of-envelope
carriers have explicit unsupported records. Unsupported scene syntax or corrupt
packing aborts the whole snapshot. The literal rational fixture supplies carrier
records, not complete trimmed topology or a claimed valid foreign B-rep.

Every caller's original source net is checked against the complete emitted
homogeneous net and exact conversion displacement bounds. Independent exact
Cox–de Boor/quotient first jets evaluate the original source. Independently
expanded Bernstein first jets grade the actual supplied binary64 controls.
Full descriptor, source-face/occurrence, lineage and actual query binding are
checked; native status/finite checks alone are insufficient. Frame conversion
is checked bitwise against the prescribed rounded product and native +0, then
against original source geometry with coefficient/span/factor/product budgets.

Across all48 campaign callers, the maximum normalized scalar position error was
**1.622e-16**, du error **1.665e-16**, dv error **1.710e-16**. These satisfy local
limits2^-40 for position and2^-30 for first partials. Maximum source-coordinate
position/du/dv errors were **1.298e-15 / 2.234e-16 / 1.110e-16**; corresponding
maximum per-query budgets were **7.276e-12 / 1.233e-9 / 2.904e-9**. Maximum errors
and maximum budgets may occur at different queries. Each actual query was checked
against its own budget, not only against these aggregate extrema.

The three grids cover1445 queried source points per placement/repetition.
Across48 callers (one excluded warmup plus three measured runs for each of the
three inputs/four placements), **23,120 first jets / 208,080 scalar components**
were independently qualified. Repetitions are the same source/grid, not additional
geometric coverage. The separate initial caller outputs are also retained.

## Actual foreign reference correspondence

The existing isolated usd-brep1.0.0 runtime imported the exact same source bytes
and returned scalar position/du/dv at the recorded **actual submitted binary64
U/V**, using its own USD reader and surface evaluator. Original natural domains
match all four mouse faces and the exported sheet. For this observed one-body/
one-prim fixture family, original face ordinals are checked by domain and jets;
no general foreign reorder/naming guarantee is inferred.

- Mouse:578 foreign scalar queries; maximum component errors against exact original
  source were position2.220e-15, du3.573e-16, dv2.220e-16.
- Sheet:289 foreign scalar queries; maxima were position2.135e-17,
  du/dv1.110e-16. Its complete controls are separately checked against the intended
  polynomial S=(u,v,2.25u(1−u)v(1−v)), with a tiny measured coefficient-rounding
  bound rather than assuming successful construction establishes that map.

Foreign acceptance is2^-32×max(1,|expected component|), distinct from local native
grades. Observed errors are much smaller. Native/reference gaps additionally
account for exact-affine versus rounded foreign query coordinates. The same867
foreign queries were compared against actual CPU1 and each P100 output:
[CPU mouse](comparison-mouse/checks.tsv), [CPU sheet](comparison-sheet/checks.tsv),
[P1000 mouse](comparison-mouse-cuda0/checks.tsv),
[P1001 mouse](comparison-mouse-cuda1/checks.tsv),
[P1000 sheet](comparison-sheet-cuda0/checks.tsv),
[P1001 sheet](comparison-sheet-cuda1/checks.tsv). This reuses one actual foreign
query set per input, not three independently executed foreign runs.

Reference acquisition/export and scalar queries have separate costs in
[reference-acquisition](reference-acquisition/costs.tsv),
[reference-mouse](reference-mouse/costs.tsv) and
[reference-sheet](reference-sheet/costs.tsv). Export operates on a copied body;
no borrowed reference object enters the owned core. The wheel's native loader
reported absent optional pxr Python modules, while actual import/export/queries
succeeded. Private native logs are retained outside public results; this warning
is recorded explicitly in [environment.tsv](environment.tsv).

## Complete input-to-consumed-result costs

Median milliseconds, with measured min–max in parentheses; three measured fresh
subprocesses after one excluded warmup, requested CPU worker counts1/24:

| Input | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| Mouse,578 queries | 34.259 (27.471–37.880) | 34.835 (29.541–76.746) | 319.586 (294.241–325.877) | 710.947 (583.395–872.206) |
| Sheet,289 queries | 8.190 (8.102–9.359) | 11.099 (10.359–13.641) | 289.157 (286.321–331.199) | 548.397 (510.822–633.659) |
| Rational,578 queries | 7.579 (7.250–7.892) | 10.194 (10.071–10.643) | 290.973 (289.877–309.892) | 638.473 (511.269–744.337) |

The useful wall includes capped read, full parse/validation/carrier conversion,
query preparation, context setup, patch/query packing and uploads, evaluate,
download/provenance, required validation/source conversion, all-field checksum
consumption and native cleanup. Exact independent qualification, module/process
startup, artifact writes and optional reference acquisition are outside it;
parent child-process time is separately retained and includes the child's
qualification/artifact work plus launch/wait. Excluded warmup does **not** mean
retained GPU context: every repetition starts a fresh process and context.

CPU wins these tiny cold tasks. Mouse CPU1/CPU24 ranges overlap and the median
difference is not a reliable multicore benefit. For mouse, median P1000/P1001
context setup alone was252.560/547.433ms and context cleanup33.920/108.768ms;
these phases dominate the complete GPU calls. Per-phase medians do not sum to the
median complete wall. GPU device ranges differ substantially here; this unpinned
three-sample run does not isolate the cause or promise those cold costs generally.
No resident, large-batch, NCSX render or whole-CAD speedup is measured.

[Mouse raw/summary](costs-mouse/summary.tsv),
[sheet raw/summary](costs-sheet/summary.tsv),
[rational raw/summary](costs-rational/summary.tsv); each directory retains raw.tsv,
controller source hash, cost scope and every actual repetition. Required
validation is in the useful wall; independent truth checking remains separate.
Setup/residency optimization stays deferred until after the breadth sweep.

## Direct tests and support limits

Parser:10 direct methods; converter:9 direct methods, including real mixed/type-
prefix packing and immutable ownership. All ran on the fleet after actual missing-
module reds. The10 source/native qualification methods passed on CPU1,CPU24 and
both P100s, including non-dyadic coefficients, unequal degrees, both degree-zero
axes, exact span1+2^-54 whose binary64 subtraction rounds to1, endpoint limits,
invalid/empty queries, signed zero, duplicate geometry/distinct identities and
falsified records/fields. These are direct assertions, not a mutation coverage
claim. No extra production-mutant campaign was needed: complete coefficient/
identity checks and explicit malformed-input cases attack this unit's stated
faults without treating selected kills as broad coverage.

Affected existing compatibility:27 runtime/body methods on CPU1 (one explicit
CUDA-only skip),27 on P1000 and27 on P1001 (all pass). No new native memory/API
code needed a rebuild; all five protected native library hashes remain unchanged
in [preserved-native.tsv](preserved-native.tsv). See the
[direct execution journal](../../journal/2026-10-15-interchange-oracle-implementation.md)
and [compatibility journal](../../journal/2026-10-15-interchange-affected-compatibility.md).

[Source/code reviews](../../papers/README.md#reference-carrier-interchange) and
[actual artifact review](../../papers/2026-10-15-interchange-caller-result-review.md)
retain findings and dispositions. Each caller directory includes original source,
faces/uses, knots/source controls, actual native controls/domains/lineage,
query bits, local/source-world binary jets, consumed rows, metrics, timing and
source/native hashes. No private hostnames or absolute workspace paths are emitted.

This is one delivered interchange beachhead. General USD composition/binary USD,
STEP/IGES, multispan extraction, imported trims/solids, persistent naming and
applicable community exchange qualification remain future. The foreign fixture
and differential checks do not satisfy the required full alpha corpus. Continue
Boolean/history and feature construction; integration tuning remains deferred.
