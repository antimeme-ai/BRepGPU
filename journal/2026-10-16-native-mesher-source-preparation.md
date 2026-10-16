# Whole-source native preparation

The native source inspector now optionally compiles original geometry using
`--geometry`. The complete watch source retained all 107 definitions, 9,162
faces, 77,613 curves and original carrier/source records. Reading, validation and
indexing took 0.212757 seconds. Original-root preparation took 48.136 seconds,
producing 80,339 curve roots and 2,839 surface roots. It charged 1,618,223,287
exact-work units; scratch peaked at 1,295,588 bytes and complete run allocations
at 285,478,013 bytes. This is one observed native preparation call, not a
completed mesh, benchmark campaign or render.

The independent small geometry fixture prepared ten curve roots and five surface
roots in 0.00123789 seconds after 0.000153291-second source admission.

The measured whole-source preparation makes two runtime questions concrete:
avoid extracting unused reconstruction guides, and compile independent original
owners in parallel without sharing mutable arithmetic budgets. They belong to
whole-run work selection and ownership. They do not justify another isolated
watch-specific hotspot rewrite while charts and face integration remain open.
