# Native actual-cell residual bound unit

After accepted exact arithmetic, selected shared CPU/P100 task unit under runtime
plan section3/4. Original rational tensor/curve roots and exact subcell/actual-L
descriptors are the inputs. Production device policy must restrict original
resident roots and form/compare residuals; it is not delivered by doing full exact
host restriction on every cell and offloading comparisons. Host exact geometry
restriction is the unresolved fallback. Geometry owner supplies the exact
restriction path and authoritative ordering, avoiding a second source theory.

Authored native bound API and55 independent original Fraction power-polynomial
vectors (29 accept/26 fail). The producer expands source tensor Bernstein into
power polynomials, substitutes original UV, forms actual-facet residuals, then
uses an independent falling-factorial power-to-simplex conversion. Covers all
surface p,q0..3, curve degrees0/1/2/3/7/16/32, equality and near-equality, tiny
dyadic subcells, positive nonuniform weights, nonunit original parameter ranges
and rational quarter circle. Actual GPU/device filters will consume the same
task corpus and unresolved answers will reach this exact comparison.

Genuine isolated fleet RED before bound implementation: producer emitted50,468
bytes; actual native link failed exit1 for missing surface_bound_exact and
curve_bound_exact. No unrelated compiler failure or fabricated postimplementation
failure supplied the RED. Raw vector/build output stays ignored.

While RED was running, independently reviewed the source reader as parent
requested. Found null-resource load_source PMR construction before validation;
sent concrete fix request. Review saved in
`papers/2026-10-16-native-mesher-source-code-review.md`. No source product edits
by this reviewer and no new source execution campaign.

## Owned exact, CPU interval and original-root P100 implementation

Exact surface fallback calls the geometry owner's original tensor-to-triangle
restriction; curve fallback calls the same owner's exact interval restriction.
The residual coefficient test forms R=N-WL in degree d+1, min positive W and
compares squared Euclidean coefficient norms to tau²*minW², with equality accepted.
No sampled maximum or newly rounded virtual L point replaces the actual facet.

CPU/P100 filters restrict original roots themselves. Surface coefficients use
tensor blossoms averaged over barycentric argument partitions with the exact
multinomial ratios. Device curves use two de Casteljau splits with warp shuffles,
including the33rd degree32 endpoint. Degree buckets select16 surface and33 curve
specializations; four warps/block, one task/warp,9,216 shared bytes. A common
exact root anchor A=N0/W0 translates source N-WA once at resident packing and
actual affine L-A at task packing; residual/weights/allowances and canonical
physical XYZ stay unchanged. Root/batch integration may cache that tiny exact
anchor rather than recompute its three quotients for each packed task.

Host outward intervals require checked nearest/no-excess/no-FTZ/DAZ on each call;
unrecognized platforms/environment, invalid endpoints, positivity uncertainty
or overlap produce unresolved. CUDA uses directed binary64 intrinsics in the
strict sm60 profile. Overflow enclosure prechecks are exact: a real Budget
failure is never swallowed as harmless floating uncertainty.

The C++17 POD CUDA ABI owns resident roots, bounded device arrays and pinned
staging. One active ticket per session; stable degree grouping retains original
answer slots and keys. Every call restores the caller's device. Invalid/stale
transactions are rejected. Destroy drains before release; failed drain keeps
the poisoned session/reservations alive. Parent review required documenting that
create can return a nonnull owned session on failure, constructor-path device
restoration, and unified rollback for all create exceptions; those fixes landed.
Packed CPU work is charged to a child Budget and device submits reserve a
complete degree-based interval-operation upper model before staging/launch.
Zero-work cases prove cached inputs cannot bypass these work limits.

## Focused qualification and scope

The bounded original task corpus is now88 cases:46 exact accepts/42 exact fails.
Added a translated2^40 plane requiring a filter proof, and thresholds immediately
above/below the independently derived exact coefficient bounds for every tensor
degree pair. This closes the weakness of testing only trivially tiny/huge
allowances. Source/knot restrictions stay the original independent POWER-basis
oracle, not a mirror of production blossom code.

Exact/CPU filtering: strict Release GREEN0.28s, ASan+UBSan/leak GREEN13.18s.
CPU and both actual P100s each report26 proved accepts,24 proved failures and38
unresolved comparisons; every proof matches the independent original oracle.
Native exact fallback was independently checked on all88 cases. GPU lifecycle
tests cover staging input mutation after submit, active-session rejection,
stale tickets, memory/work budgets, device restoration and destruction of a
live second batch. Actual compute-sanitizer memcheck/racecheck/synccheck on both
P100s report zero errors/hazards. Test durations are qualification durations,
not comparable whole-mesher CPU/GPU performance results.

Repository clang-tidy and named100-column format checks pass. Swappable internal
pointer/index arguments were replaced with explicit coefficient/binomial tuples;
no diagnostic suppression was added. Geometry owner independently accepted
the numerical coefficient/min-weight/interval logic and exact anchor identity;
parent reviewed the device-session lifecycle and requested the fixes above.

Static device resource inspection exposed a400-byte per-thread copied packet/
parameter stack; changed task access to an immutable resident-buffer reference
whose lifetime is already protected through drain. Actual sm60 stack dropped
to96 bytes/thread, with the largest surface specialization retaining112
registers,9,216 shared bytes and zero separately reported LOCAL bytes. All three
device memory/race/synchronization checks repeated after this access change and
remain zero errors/hazards on both cards. This is a measured compiled resource
change, not an unmeasured throughput claim. No geometry/numerical policy changed.

Final independent vector producer SHA256:
`b9f41449741a83c3840533508285d3570609ddf12044a2d86d42fe19568b5810`.
Generated88-case payload SHA256:
`70102a1b892aa6c607d94e76fb4551885c7392f9927d580f916c0905c4adac4b`.
Raw generated vectors/build and sanitizer outputs remain ignored.

The native scheduler/whole-mesh consumer remains another accepted-plan unit.
These functions and device filters do not claim a completed native watch mesh,
completed higher-degree whole-trace/analytic bounds, or a measured end-to-end
GPU gain. No Python production callbacks or new numerical library were added.
