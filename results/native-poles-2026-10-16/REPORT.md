# Native analytic pole construction

The actual geometry producer passed twelve original-source families through
shared-edge master subdivision, finite cap construction, the existing face
engine, and full source-to-active site mapping. See [producer.tsv](producer.tsv).
The families cover north/south spheres and cones, same/distinct declared
endpoint IDs, authoritative straight source traces, and reversed face senses.

Each case obtained strict interior samples on BOTH original meridians before
publishing its shared face-owned cut. The initial mesh has two ordinary
remainder facets and one nonzero physical pole facet. Every case also passed
an angular-only restart with unchanged source-boundary generation, followed by
radial replacement that preserves the old cut across a one-sided source epoch
and requires both endpoint parameter gaps to contract by at least 3/4.

The source occurrence inventory remains complete, including the original
collapsed pcurve range and declared vertex aliases. Original source facts are
separate from active construction sites. Authoritative identity traces retain
a named domain map; nontrivial B/T maps coupled to caps are unsupported in this
finite profile. The producer admits a four-use outer pole sector with constant-u
meridians and proves its convex cap is separated from the other selected trims.
It does not manufacture private nodes on an original regular-side shared edge.

A direct primitive test caught a real error in the finite-to-ideal band bound:
a derivative bound confined to the near-pole strip missed interior pointwise
correspondence error on a nonorthogonal sphere with a large Z axis. The corrected
bound covers the entire convex v hull of finite boundary, ideal pole and both
cut endpoints. The same test now passes. This is a whole affine-rho source map,
not an image-only proximity claim. The primitive also checks independent analytic
source-versus-output samples, exact 1/16 pole displacement, nonzero physical
facets, and unequal cut heights that exercise the mixed derivative term.

These are construction/correspondence results, not a complete-watch result or
a speedup measurement. The first full sanitizer cohort was censored at120s in
existing exact trig/jet arithmetic. The selected final north producer path
passed ASan/UBSan/leaks within its180s bound, including angular and both-sided
radial continuation. The corrected primitive also passed sanitizers. No GPU construction claim follows from these tests.
