# Finite pole consumer geometry review

Reviewed the actual `native/mesher/face_engine.cpp` pole branches:
`initialize_poles`, `validate_pole_source_coverage`, `evaluate_pole`,
`pole_refinement`, and `take_cell_work` host dispatch. This review does not
self-approve the geometry producer or the analytic band theorem.

The same mutable topology installs the ordinary remainder with deferred initial
seal, appends the positive-chart nonzero pole facets, then seals once. Consumed
original meridian intervals must each be exactly one current original source
segment. The complete original nondegenerate boundary multiset is compared to
the actual constructed boundary by BoundaryKey and canonical physical endpoint
keys, preserving multiplicity and preventing a dropped source interval. An
internal face cut must have incidence two; an empty-remainder original cut must
retain its source-edge key and have incidence one. Proxy sites remain explicit
source-to-active mappings and never become ordinary UV residual cells.

The host dispatcher recomputes the actual original analytic fan bound against
current physical nodes. It verifies map-role permutation and regular sheet,
charges position plus finite-source band once, and completes the same canonical
frontier key. A failed-only fixed-profile unresolved result uses the pole-only
term, not the changing cut-corner maximum; successful actual cells are not
rejected because a retained global source upper is looser. No blocking consumer
finding in this scope.

One profile clarification was sent to the topology owner: the producer accepts
an explicit GeometryBoundPrecision, whereas evaluate_pole currently supplies
its default. If nondefault preparation profiles are supported, preserve that
profile in the frozen cap and use it consistently; alternatively the whole Run
must explicitly enforce its fixed default policy. This is distinct from cache
history and does not alter the current default source-backed cohort.
