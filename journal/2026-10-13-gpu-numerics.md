# 2026-10-13: GPU numerical geometry reconnaissance

Read current workspace doctrine and repository instructions. Studied primary papers on Bézier derivatives, subdivision, GPU interpolation, trimmed NURBS ray tracing and GPU midsurface abstraction, plus NVIDIA Pascal/toolchain/floating-point documentation. Acquired seven valid PDFs into ignored `papers/pdfs/` and extracted text into ignored `context/paper-text/` for reading. Two additional PDF acquisitions failed; their valid public landing pages are recorded in the acquisition manifest. Removed downloaded HTML/empty files that had misleading PDF filenames.

Wrote [research report](../papers/2026-10-13-gpu-numerics.md), [BibTeX](../papers/gpu-numerics.bib) and [restoration manifest](../papers/gpu-numerics-acquisitions.tsv). Findings include a new 2026 constant-storage evaluator paper and gMidSurf's explicit GPU/host stage boundary. Proposed an owned FP64 rational surface jet contract, explicit request failures, conservative subdivision and a bounded inverse-query consumer. Added an exact rational quarter-circle fixture with unequal endpoint weights to improve oracle independence.

No GPU implementation, hardware benchmark, package install, production dependency selection, Git mutation or external communication was performed by this research lane. Publication and integration remain with the repository owner.
