# Operator places integration after broad-pass big rocks

The operator first requested thinking about GPU setup/teardown optimization,
then clarified that integration/making-it-nice work belongs AFTER the big rocks
of the broad pass. Captured concrete measured setup, lifetime, scene reuse,
buffer ownership and host-conversion opportunities in docs/RETURN_PASS.md.
Directed the two source-study lanes to finish notes only; no new product code,
profiling campaign or active optimization lane. Corrected README/roadmap/agent
instructions accordingly. Intersection initialization/representation remains
the next major implementation theme; pair qualification/publication proceeds.
Opened issue19 as explicitly deferred work, linked to the measured/source notes.
