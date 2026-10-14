# CUDA mode-specialization plan numerical review

Date: 2026-10-14. Plan reviewed:
[`docs/CUDA_DISCOVERY_MODE_PLAN.md`](../docs/CUDA_DISCOVERY_MODE_PLAN.md), SHA-256
`446cff6c202bc4a284d71c667ad76f9bb0b1ad90f73c5dce7bd1eca77c0ffdd6`.
Read against the accepted numerical design review and current shared helpers/CUDA
callback. Source review only; no product changes, tests, builds, target operations
or timing measurements.

Disposition: accepted. No blocking finding or required numerical correction.

## Numerical and integration sequence

The permitted delta is correctly limited to three compile-time instances of the
existing CUDA kernel, host dispatch and direct clients. Passing an unsigned mode
constant into unchanged force-inlined helpers preserves directed FP64 arithmetic,
original-net upper witness, active coefficient extent, strict pruning, DFS order,
caps/counters and full-count SoA layout. Shared CPU/math/ABI/adapter/cache builder,
launch geometry and numerical flags remain unchanged.

The retained-mode validation is explicitly before allocation and the empty guard.
An impossible private mode creates no token/events and cannot silently choose an
unrelated cache interpretation. No public-option test is misrepresented as
executing that private corruption path. Launch remains inside device/stream scope,
with error collection, event cleanup, completion/restoration and pending-result
cleanup retained. The corrected event bracket description includes possible
submission/idle gaps and does not imply pure device execution time.

## Direct oracles without a duplicate campaign

The raw capture client serves an execution-compatibility claim: identical input
and binary identities must produce identical double/integer slab bytes and
deterministic metadata. It does not calculate geometry, compare float tuples,
normalize NaNs/zeros or use only result hashes. Fresh-process capture isolates
library/plugin selection and process state. Input/control/options/profile/count
keys and actual binary identities prevent comparing superficially similar batches.

The existing 309-row Fraction qualifier separately checks exact original-input
geometry and cache containment once per new plugin/card. Preserved dynamic plugin
qualification is reused only for the matching actual binary. This avoids another
old-plugin Fraction campaign while retaining independent truth for the new binary.
The identity capture of that same corpus adds compatibility evidence rather than
an oracle-for-oracle layer.

All-mode 128-thread boundary/tail tests are relevant to the three new entry points;
they should use analytic repeated-row answers rather than run a Fraction campaign
over every duplicated tail row. Selected degree-(0,0), rational one-third, mixed
degree/source offset, depth-12, budget, capped/completed and padding cases constrain
active-path specialization. Exact work counts apply to fixtures that independently
specify them; old/new integer identity checks the remainder. Profile on/off,
retained-parent and per-thread device restoration checks constrain the changed
dispatch integration. Focused sanitizer runs and v1/v2 smoke checks cover that
new code-generation/integration boundary without rerunning unchanged modeled
cleanup, CPU qualification or the full production mutation campaign.

Pre-change focused baselines correctly must pass. A behavior-preserving performance
experiment has no desired functional red and no useful invented timing assertion.
Missing backend, failed identity, geometry violation or sanitizer failure stops
measurement. Neither CPU parity nor an epsilon can waive such a difference.

## Measurement and decision

The 28 matched conditions cover launch overhead, saturated batches, each cache
mode, aligned/off-grid precision, shallow sufficient trees and capped common
projector work. Their table cardinality is consistent: 12+6+2+2+6=28. The same
preserved core and benchmark binaries select different sibling plugins, so setup
and native caller implementations do not change with the variant. CPU1/CPU24
measurements are retained, and no fleet work starts until the scalar queue ends.

Seven samples after excluded warm-up, alternating variant order, sequential cards
and no overlapping owned jobs provide a coherent bounded comparison. Precision,
work counters and full condition keys accompany each native timing scope. Event
duration, resident operation and cached-host consumption are compared separately;
independent median components are not combined into a fictional end-to-end time.
Resource observations alone cannot select the change.

The decision rule is concrete: at least one large-batch condition must improve a
native phase by at least 5%, confirmed in reverse order on both cards, while no
other large-batch native phase has a confirmed regression greater than 5%.
Small-batch regressions exceeding both the relative threshold and 0.05 ms receive
investigation. These are explicit experiment acceptance thresholds; no runtime or
CI contract depends on them. Reverse-order repeats are limited to plausible gains,
regressions or order/spread ambiguity rather than expanding the entire matrix.

When reporting acceptance, identify the same qualifying condition and native phase
on both cards and retain each card's individual result/spread; do not select an
unrelated best case per card or substitute event-only gains. This follows directly
from the plan's matched comparison and confirmation rule. Mixed dispatch or other
optimization after a mode regression remains a separate reviewed choice.

The plan makes no precision, CAD-modeling or independent-target generalization.
The existing repeated-input limitations and off-grid unresolved populations remain
visible. No additional ceremony or broader numerical redesign is needed before
the implementation owner proceeds after coordinator acceptance.
