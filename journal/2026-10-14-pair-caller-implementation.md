# Actual pair caller and matched measurements

Implemented a deterministic mixed-body scene recipe and an actual broad-phase
caller exporting source/local/world bounds, packed u32 mask, typed shortlist,
independent qualification, timings and an SVG built from actual bits. Its
admitted size cap keeps this demonstration within one complete page; separate
direct tests exercise continuation. Added a serial fresh-process eight-case
benchmark (4/128 bodies per side; CPU1/CPU24/P1000/P1001), five repeated
page/download/cleanup/decode calls with retained indexes. Setup and exact
qualification are separate observations; event profiling disabled in the
main wall comparison. Local work limited to source and AST checks. Independent
caller review and fleet execution still required.
