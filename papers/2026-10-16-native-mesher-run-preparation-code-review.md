# Native whole-run preparation review

Scoped actual-source peer review of native/mesher/run_preparation.hpp/.cpp,
consumed SourceStore row-count admission, GeometryStore root allocation,
prepare_assembly borrowing, and bound-bank capacity admission. Root owns actual
strict, sanitizer and tidy execution; no duplicate execution was requested.

The job uses one scheduler scratch Budget synchronously. Definition envelopes
and their borrowed rational bbox views remain alive through assembly preparation;
sealed assembly/envelope tables and the original GeometryStore retain all values
needed after that scratch dies. Every original definition is visited and B0
sealed before assembly allowance transport and root-bank publication. Only the
completed immutable PreparedRun lease escapes. Source/header/resource failure
prevents output publication; a failed started job cannot be retried as new work.
Empty root banks are a legitimate all-analytic source case and remain admitted.

Finding: GeometryStore permits the last uint32 slot UINT32_MAX, so its vector
can contain UINT32_MAX+1 roots. The run preparation's uint32 loop index could
wrap forever for that admitted count. Root fixed the finding with explicit
uint32 bank-capacity admission before
source selection; actual-source reread confirms both root vectors greater than
UINT32_MAX are refused before the loops.
No infeasible multi-billion-root execution is requested to establish integer
wrap semantics. Source definition row loops already consume parser counts
capped at UINT32_MAX and do not share this boundary.

Ownership and complete-inventory review otherwise ACCEPT. This is not a claim
that preparation alone realizes or qualifies any source face mesh.
