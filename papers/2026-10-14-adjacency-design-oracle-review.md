# Adjacency design: oracle tribunal

Reviewed `docs/ADJACENCY_DESIGN.md` against the independent oracle study, actual
body factories/packing/oracles and topology study. No product implementation or
tests were run. The incidence and mate semantics are sound; accept after the
following concrete design amendments, with campaign sizes set in the plan.

## AO1 — complete source truth, not only degrees and seam roles

The qualification paragraph says literal expectations establish primitive
degrees and seam roles. That is too weak to distinguish a consistently wrong
body-to-flat-input face projection from correctly constructed adjacency. A
dictionary oracle derived from already misprojected input would agree with
both native outputs. All edges could still have degree2 and the seam survive
while cap/front/back faces are exchanged or a plate hole role is lost.

Require literal signed edge/coedge/face identity pairs for all canonical body
uses, plus retained loop/UV occurrence provenance. The research study already
supplies complete box sequences and cylinder/plate special pairs. Check body
flattening against those source expectations and original records; check the
actual native CSR and mate outputs against the separate dictionary relation.
These attack different contracts, not an oracle-for-oracle certification layer.
The general combinatorial input model needs no claim of body geometry.

The independent oracle should decode actual binary offsets, members, kinds,
mates and neighbors itself using stated layouts, rather than use product row
or identity accessors to produce expected or actual membership. Resolve source
keys from retained original input arrays; compare complete multiplicities, not
sets that erase duplicate output members. Check actual output byte extents.

## AO2 — measured globally shuffled large coedges

The proposed benchmark mentions small bodies and larger repeated batches but
does not require a measured globally shuffled large coedge input. Canonical
packing groups many opposed pairs close together. Only measuring that order
can flatter atomic-fill/cache behavior and fails the research's representative
storage-order requirement.

Require at least one large globally shuffled coedge assembly, preserving typed
body-instance/source keys and rewriting all dependent coedge slot references.
Benchmark both canonical and shuffled order if storage order materially changes
cost; do not require deterministic GPU row order. Edge/face permutation remains
a qualification metamorphism, while global coedge shuffle must appear in a real
matched CPU1/CPU24/both-P100 measured case.

The plan should name at least one meaningful large size in E/F/C and counts of
each primitive instance, plus repeat count/warmup/cold scopes. A repeated batch
is valid topology work but must remain labelled disconnected body instances,
not one complicated solid. A crowded high-degree graph can be a qualification
case without being sold as primitive B-rep performance.

## AO3 — mutation scope includes actual GPU builder and reader

The design's mutation paragraph does not require actual CUDA production sites.
Mutating only Python source mapping or the CPU reader cannot characterize the
new GPU count/scan/fill and reader's fault classes. Require at least one actual
CUDA builder fault (lost/wrong membership or wrong prefix) and one actual CUDA
reader fault (seam/sign/nonmanifold/mate provenance), with independent semantic
assertions in real device calls. Native preflight validation mutations target
direct no-write/rejection assertions. Healthy baselines must precede kills;
build/import/API errors do not earn kills. The implementation plan chooses
specific sites after the relevant code exists.

## Accepted parts and qualification boundaries

- Unordered rows are appropriate and preserve loop authority. Complete member
  comparison and identity-based reader outputs remain schedule-independent.
- Degree0/1/2/>2, equal-sign degree2 and same-face opposed degree2 have explicit
  results; the seam is an occurrence-preserving combinatorial class, not a
  generic geometric periodic-surface detector.
- The 2^22 public cap makes count and prefix overflow unreachable for admitted
  inputs. Oversized-count pre-access refusal is distinct from executing an
  allocation-scale prefix overflow; report that boundary accurately.
- The ABI preflight no-write guarantee is testable directly for malformed
  native extents/references/signs/aliases. Empty extents need explicit null
  pointer handling in the plan/header; it is not a mandatory error.
- Count/scan/fill and actual reader both sit inside native and complete caller
  spans. Independent relation qualification happens outside those intervals
  and its cost is reported separately. Synchronized phase measurements are
  host spans; report instrumentation overhead if phase synchronization changes
  the operation. Do not label those clocks kernel-only execution.
- Separate topology binaries protect prior numerical campaigns. No dependency,
  general session manager, editing/history or deferred bulk integration is
  needed to establish this beachhead.

Disposition: accepted after root added complete literal signed-use/source and
loop/UV provenance checks with independent binary decoding; an explicitly
measured globally shuffled assembly; and actual CUDA builder/reader mutants.
The implementation plan supplies concrete matched sizes and trial scopes.
No current product defect is alleged; these were missing qualification and
benchmark requirements.
