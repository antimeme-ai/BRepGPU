# Investigate Python's role after the breadth sweep

The operator explicitly requested this investigation on 2026-10-14. Keep it
after the broad-pass big rocks, alongside the integration work in
[RETURN_PASS](../docs/RETURN_PASS.md). Continue delivering uncovered geometry
themes now.

The question is whether Python belongs in the delivered CAD system, and in
which role. Compare an owned native core/application, a thin optional scripting
binding, and Python orchestration. Research and independent qualification can
continue using Python regardless of the shipping runtime decision.

Motivation comes from actual consumers: the million-use adjacency traversal
takes roughly 4–5 seconds while complete native construction takes about
15–22 milliseconds on the P100s. Intersection request preparation costs 27–32
seconds and the reused complete consumer 2.3–2.5 seconds despite roughly
0.11–0.13 milliseconds of native evaluation. These are different operations;
neither number predicts a native replacement's speed or proves a language
decision by itself. See the published [adjacency](../results/adjacency-2026-10-14/REPORT.md)
and [intersection](../results/intersections-2026-10-14/REPORT.md) reports.

Study actual downstream modeling operations after the broad pass. Attribute
representation conversion, Python object traversal, exact arithmetic, source
identity management, validation, transfers and resource lifetime separately.
Compare full cold/reused/shutdown costs, memory, ownership and failure behavior,
plus scripting extensibility, interoperability and implementation cost. Preserve
independent geometry qualification while changing consumers.

Deliver a reasoned architecture choice, measured prototype where useful, and a
concrete sequence of owned changes. Do not silently adopt a dependency or assume
that moving an existing algorithm into C++ automatically improves it. No language
redesign, new profiling campaign or integration tuning starts under this note.
