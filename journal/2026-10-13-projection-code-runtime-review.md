# 2026-10-13: resident projection runtime/Python cross-review

Reviewed additive headers, shared registry/admission, lazy v2 table negotiation, options/provenance, CPU range/thread behavior, Python snapshots/result publication, cleanup and cross-family fatal poison. Saved `papers/2026-10-13-projection-code-runtime-review.md` with reviewed source state and two concrete moderate findings.

Confirmed diagnostic/output alias could overwrite numeric or source data through generic diagnostic writeback; the owner added an outside-call nonthrowing alias guard, which this lane re-reviewed as resolved in source. The integrator captured the earlier direct red; corrected target execution remains separate.

Found published native handles lacked cleanup if subsequent Python wrapper allocation/construction raises. Sent the owned wrap-or-close fix requirement to the runtime owner and asked the independent caller owner for a direct actual-core constructor-failure red before implementation. No other boundary blocker found so far. No runtime-owned product file or Git mutation was changed by this lane.

Clarified the same invariant includes exceptions on native creator return, before construction: retain the ctypes output handle and guard invocation/value extraction/construction together, catching BaseException and cleaning nonzero IDs. This covers pending keyboard interruption without adding asynchronous API behavior. Supplied a direct actual-native-return injection strategy; no runtime-owned source was edited.

Re-reviewed the owner's common create-resource guard and guarded Engine initialization after direct constructor red was captured. Invocation, ctypes value conversion and construction are now one BaseException scope; cleanup preserves the primary object and records secondary native diagnostic/fatal information while tolerating partial finalizer cleanup. Both families consume the helper. R2-2 is resolved in source; R2-1 remains resolved. Appended final no-blocker disposition, keeping target green/compatibility/memory results separate.
