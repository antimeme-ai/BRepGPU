# Discovery design runtime review

- Reviewed integrated three-mode iteration3 design against actual registry,
  admission, cleanup/publication, plugin v2 loading and Python resource machinery.
- Reported two medium blockers: alias/no-write rule must explicitly include
  ABI3 info records and handle-versus-diagnostic; exported DOP/Bernstein cache
  indexing must fix node/polynomial/active coefficient/endpoint order.
- Recorded implementable shared lifetime/fatal/oldABI compatibility, actual
  resident exports, mode enum/product/4,000MiB storage admission and worker
  environment/join obligations. No product/test code or Git mutations.
- Saved papers/2026-10-14-discovery-design-runtime-review.md; root dispositions
  and integrated-design re-read pending.
- Re-read integrated corrections: explicit all-ABI3 descriptor/payload/diagnostic
  alias no-write rule and normative node-major DOP/Bernstein indexing resolve
  both findings. Fixed13-entry stack overflow is invariant ARITHMETIC; only a
  visit cap uses the root-bound fallback. No runtime design blocker remains.
