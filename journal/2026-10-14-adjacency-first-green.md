# Literal adjacency greens and initial native checks

The separate CUDA-enabled topology target compiled on the fleet without touching
prior binary trees. The original cylinder red test now passes CPU1 (0.046s),
CPU24 (0.039s), P1000 (0.559s) and P1001 (1.220s), preserving distinct seam uses
and same-face neighbors. The existing geometry probe also remains healthy.
Direct native checks pass all four placements, including literal incidence,
preflight no-writes, zero rows, recursive scan and high-degree membership.
These are initial greens, not the final sanitizer/independent qualification.

Root's native review found CPU1 initially used atomic increment despite the
reviewed direct-counter baseline. The native owner corrected CPU1 to owned
uint32 counters before these greens; CPU24 retains atomic count/fill. Wrapper
and actual caller reviews accept source ownership, permutations, ABI lifetimes,
raw-byte qualification, measured full consumption and transfer formulas. A
separate topology review found the public edge_kind reader lacked direct truth;
the oracle owner added literal accessor and invalid-slot assertions. Root is
requesting an explicit prior-CUDA-device restoration probe before final memory
qualification. Compiler diagnostics are bounded diagnostic-string truncation
warnings; no build failure or native arithmetic issue was observed.
