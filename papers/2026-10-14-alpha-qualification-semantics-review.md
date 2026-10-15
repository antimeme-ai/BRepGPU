# External alpha qualification policy: bounded semantics review

2026-10-14. **Accepted; no blocking factual error or release-gate loophole found.**
Read `docs/ALPHA_QUALIFICATION.md` and the complete alpha oracle audit, OCCT suite
study and CGAL external-oracle study. This is a bounded source/publication
review, not new research, a test run, a library decision or a claim that the
external qualification gate has passed.

The policy implements the operator's actual distinction: broad external suites
may remain red during development, while alpha requires its entire declared
external population green. Applicability precedes observed outcomes; missing
input, missing adapters, required unsupported geometry, crashes, unresolved
truth and skipped/xfail/known-BAD results do not become passes. Changes to the
advertised scope or required corpus need operator review, and an empty or small
handpicked passing subset cannot satisfy the gate. Every advertised execution
backend and the actual release candidate must be exercised.

Stock OCCT/CGAL runner success is correctly separated from BRepGPU output
qualification. The studies retain source provenance, original expected checks,
foreign geometry and semantic normalization responsibilities. Different valid
edge/chart decompositions may compare equal, but this does not authorize lost
seam multiplicity, missing regions, repaired-away defects or widened tolerances.
The OCCT study explicitly preserves original count checks and requires a reason
for each legitimate normalized replacement. A known upstream defect needs its
intended contract determined; it is not an owned expected success. A negative
input case can legitimately require rejection without qualifying a valid case.

Independently checked the audit's pinned Boolean inventory: 4,109 files, 2,078
with `locate_data_file` and 17 with TODO. Its cylinder/box `ZC6` example matches
the actual recipe. The OCCT group inventory distinguishes registrations from
unique scripts and executed tests; inherited fixture loading and confidential
data limits remain explicit. The CGAL proposal is a separate test reference
for exact planar geometry, with assertion-enabled upstream health tests,
complete semantic region comparison and correct closed-incidence expansion.
That bounded first adapter does not replace broad B-rep alpha qualification.

All three studies acknowledge weak external assertions and shared assumptions
in the current owned oracles. Their targeted mutation claims remain limited to
named observed faults, without extrapolation to general adequacy. NIST/CAx-IF,
mesh and input-corpus roles are scoped to the claims they can support. No
universal certification, external passes, installed reference tooling or
production dependency is invented. The specific future alpha inventory,
adapters and direct candidate runs remain required implementation work; this
publication records that debt clearly.
