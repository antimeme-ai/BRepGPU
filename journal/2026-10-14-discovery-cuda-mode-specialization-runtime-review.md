# CUDA mode-specialization design review

Read current repository instructions and shared doctrine, then reviewed the
specialization proposal against CUDA discovery/upload callbacks, DeviceScope,
Operation/Pending cleanup, runtime publication/profile ownership, private v3
table, shared arithmetic mode preparation and existing native benchmark scopes.
Only this review and matching journal were written in the specialization branch.
No product edits, build/test/copy jobs, fleet execution or baseline mutation.

No numerical/ABI/ownership blocker was found. Two concrete design clarifications
were sent to the coordinator: invalid retained private modes must be rejected
outside the nonempty launch guard so empty requests cannot publish them; and
the existing event bracket cannot promise exclusion of host dispatch/submission
gaps. Preserve callback cleanup and event locations, explaining their scope.

The review is saved at
papers/2026-10-14-discovery-cuda-mode-specialization-runtime-review.md.
Qualified exact geometry, separate binary output identity, all-mode boundaries,
profiling/restoration and compact useful native-phase comparisons remain the
appropriate plan inputs. No timing result is claimed from this source review.

Coordinator integrated both findings. Re-read the corrected proposal and
accepted their dispositions: private mode validation precedes allocation and
the nonempty guard; event scope explicitly allows a submission gap. Binary
output comparison is now explicitly byte-based. No remaining blocker; ready
for implementation-plan review. No additional source or execution work.
