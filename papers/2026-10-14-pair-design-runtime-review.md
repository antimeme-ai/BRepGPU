# Pair discovery design: runtime/CUDA/wrapper review

2026-10-14. Accepted after resolving PR-R1. Reviewed
[PAIR_DISCOVERY_DESIGN.md](../docs/PAIR_DISCOVERY_DESIGN.md) against the actual
registry/admission/publication/cleanup/lazy-extension paths in runtime.cpp,
guarded allocation/device/stream callbacks in cuda_backend.cu, and Python
lazy declarations/resource-publication guards. No implementation or execution.

Reviewed design SHA-256:
`9f1d200ac5b5a0d6e462d98151e7184781a44f625b6a0dca69dd17ed5f305183`.

## PR-R1 — actual index download missing from the table: resolved

The initial five-callback table omitted index download while PairIndex.download
and independent qualification promised actual resident inputs. Returning a
retained host snapshot would not implement the specified CUDA storage download.
The coordinator added an explicit sixth index-download callback. Re-read the
corrected six-callback, 72-byte table requirement: actual device boxes must be
copied on the retained context stream, completed/restored before success, with
the same failure/alias rules as other downloads. CPU returns its actual copied
index storage. This closes the concrete omission; no unresolved blocker.

## Size and boundary checks

The public ABI info40 is consistent: two u32 version/size fields, five u32
box/options/index/page/diagnostic size fields, and three u32 double/integer/
handle size fields. Existing ABI1 already checks pointer size. Page options24,
index info24 and page info80 match the named field counts/alignment. With the
existing private table prefix and six pointer callbacks, the qualified 64-bit
table is 72 bytes. Canonical sizeof checks belong in the header handoff; larger
public byte capacities do not relax canonical record sizes or numeric extents.

Exact finite-bit admission/overlap avoids inherited DAZ/rounding behavior;
signed zeros share a key. The 65,536-item limit permits T=2^32, so cursor/total
must remain u64. Nonempty pages are bounded by 2^20 pairs; word-owned masks avoid
atomic append, lost candidates and ordering ambiguity. Empty/terminal pages
avoid division and launches. Tail bits stay zero. FULL_DOMAIN and REACHED_END
distinguish whole-domain coverage from a terminal suffix, including the empty
domain equalities. No false all-zero success is allowed on failure.

## Lifetime, CUDA and Python disposition

Same-context single-lock admission fits the current registry: resolving the
right input under left admission avoids opposite-order context deadlock.
Retained index objects preserve device buffers and copied inputs after their
public handles close; retained context ownership preserves the stream until
final child cleanup. Wrong/stale/foreign handles fail before work. Registry
publication uses existing unpublished-token cleanup and fatal-flag composition.

The pure lazy v4 factory/base identity leaves prior tables and operations
unchanged. Stable capability refusal is distinct from transient/fatal errors.
Existing guarded/Pending/DeviceScope/finish machinery covers allocation,
upload, kernel, download, restoration and once-only cleanup; CPU workers join
before unpublished output is discarded. Profiling is optional and scoped to
an actual nonempty event bracket, with NaN otherwise. No new global registry,
device ownership or cleanup framework is needed.

Python incremental input bounds, lazy declarations and guarded wrapper
publication reuse the established patterns. The body consumer owns indexes,
borrows Engine, retains exact scenes and closes a native page before publishing
immutable host values. Its explicit BaseException guard must preserve primary
identity and safely attach cleanup details, following the resolved sampler
finding rather than assuming the older generic resource __exit__ is sufficient.

The separate CPU body adapter uses actual depth0 roots and exact outward world
conversion before upload. Its geometry/FP-environment/provenance cost stays
outside the comparison-only native contract. All-pairs and carrier-box survivor
claims remain narrow; no trimming/contact/intersection or unexamined-prefix
exclusion is implied. Proposed qualification directly attacks word truth,
storage, coverage, handle/alias failures and optional-table compatibility.
