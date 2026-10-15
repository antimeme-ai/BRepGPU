# Boolean sampler source review

Reviewed boolean_sampling.py and test_boolean_sampling.py against the accepted
plan and existing Engine/JetBatch/child cleanup machinery. No blocking source
finding. Actual binary64 copies precede classification; exact Fraction queries
remain separate. Original per-Patch carrier/frame descriptors and complete query
bits, Lineage, count/order/terms/backend/statuses are checked both before and
after world conversion. Wall normals use actual face sense after conversion,
not the topological frame. Rejected/invalid rows retain NaNs/status−1.

Each patch/query/result child has the existing primary-error-preserving cleanup
scope. Late host failure captures all three actual child handles, then retries
with the same result and borrowed Engine. A real downloaded descriptor corruption
checks request/source binding. Empty/all-rejected requests and bounds are direct
cases. No raw jets are retained or stronger derivative accuracy claimed; the
independent oracle qualifies output positions/normals under explicit limits.
Fleet green is pending the CPU constructor dependency; source acceptance is not
an execution result. Native binaries are unchanged.

Fleet integration correction: the initial sampler incorrectly demanded a finite
kernel_ms without respecting optional profiling. include/brepgpu.h specifies
NaN unless BG_RESULT_PROFILED is set. The owner corrected the check to admit only
known flags0/1 and require NaN when unprofiled, finite nonnegative timing when
profiled. Root re-read the header/runtime field publication and corrected code;
accepted. Actual CPU1/CPU24 runs then passed all8 direct sampler tests in0.517/
0.529s. This finding was caught by the real native caller rather than hidden by
a mock. P100 placement qualification follows separately.
