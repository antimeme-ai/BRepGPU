# Component coordinator construction review

2026-10-17. Scoped ACCEPT of native/mesher/adaptive_component.hpp/.cpp after
reading the actual implementation and the accepted coordinator plan.

`wave` drains submitted jobs and takes all completions before ordered folds may
throw. Each reusable face owner is retired only after complete/first-request
return, and the request union is applied only after the complete face horizon.
`face_generations` computes the use-derived generation map once per settled
boundary; invalidation compares each retained face's captured generation. Thus
unrelated earlier snapshots stay valid without an F-times-uses scan.

The immutable final contact horizon imports constructed sites only. Boundary
nodes retain actual authority keys and bitwise XYZ; generated keys use source
face/global quality/site ordinal rather than assignment or worker identity.
One flat key sort assigns canonical NodeSlots, source intervals retain declared
edge identity, and internal edges use canonical physical node pairs. The existing
filtered/exact contact owner remains the sole contact predicate. Errors cancel
and drain tasks and leave the coordinator terminal; host result leases survive
owner retirement/close.

Direct support: the independent SAME face/source-chain reader passed the authored
closed box, periodic seam and actual coarse closed-loop request component on
CPU1/CPU4/both P100s, 12 queries / 36 complete face outputs, with identical saved
artifacts. Separate-ID coincident plane faces refused without output. These
checks establish the finite source/ownership/complete-horizon connection; they
do not establish general curved authoritative maps, singular quotient support,
watch delivery or a performance advantage.
