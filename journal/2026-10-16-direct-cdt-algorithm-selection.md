# Direct CDT algorithm selection

Operator resumed work after clarifying that performance ambition means selecting
strong methods before timings, rather than accepting inherited algorithms or
watch-specific shortcuts. This lane studied one conceptual unit: direct exact
constrained triangulation of immutable sampled source domains.

Read shared BLACKBIRD.md, repository AGENTS.md, current native exact domain
representation, prior Lawson proposal and algorithm/reference papers. Studied
existing pinned Geogram and Triangle sources without executing them. Acquired
pinned artem-ogre/CDT reference source and retained archive; QUARANTINE.md records
revision, license, hash and restoration. No production code, tests, dependencies,
live fleet process or sibling checkout was changed by this lane. No benchmark
or conformance execution was performed; source observations are labeled as such.

Findings and recommendation are saved in
papers/2026-10-16-direct-cdt-algorithm-selection.md. Early findings sent to root:
full-CDT divide/conquer is different from initial-DT divide/conquer plus recovery;
BRIO is not plain spatial sort; special cavity linear-time recovery is not a
general arbitrary-polygon triangulator. Seidel is the optimal sweep alternative,
not Chew's divide/conquer. Chin–Wang's linear simple-polygon CDT remains relevant
but cannot be claimed for holes through weak bridge words. Root independently
owns the GPU frontier comparison; this lane does not preselect CPU globally.

## Retained original PDF bytes

All were acquired 2026-10-16. PDF files are ignored. Commands restore the original
bytes; compare SHA-256 before treating replacements as the same source.

| File in papers/pdfs/ | URL | SHA-256 |
| --- | --- | --- |
| shewchuk-brown-incremental-cdt-socg2013.pdf | https://people.eecs.berkeley.edu/~jrs/papers/inccdt.pdf | e1dbaae1b9c84d710f068928951039658a1b2d1d4daa9dcb24eddf147e555e8c |
| amenta-choi-rote-brio-2003.pdf | https://people.eecs.berkeley.edu/~jrs/meshpapers/BRIO.pdf | c08bc9c0a0b567743d1caaf40c1864fc4ee5895d4b850317da75f1c0a35d730d |
| chew-constrained-delaunay-1989.pdf | https://www.wias-berlin.de/people/si/course/files/Chew89cdt.pdf | e5469fcc9be8c7acd78d36609fef86875703ae9d116cac3c80bd1cc0816abfef |

```sh
mkdir -p papers/pdfs
curl -fL https://people.eecs.berkeley.edu/~jrs/papers/inccdt.pdf -o papers/pdfs/shewchuk-brown-incremental-cdt-socg2013.pdf
curl -fL https://people.eecs.berkeley.edu/~jrs/meshpapers/BRIO.pdf -o papers/pdfs/amenta-choi-rote-brio-2003.pdf
curl -fL https://www.wias-berlin.de/people/si/course/files/Chew89cdt.pdf -o papers/pdfs/chew-constrained-delaunay-1989.pdf
shasum -a 256 papers/pdfs/shewchuk-brown-incremental-cdt-socg2013.pdf papers/pdfs/amenta-choi-rote-brio-2003.pdf papers/pdfs/chew-constrained-delaunay-1989.pdf
```

Read whole Chew 1989 paper, Shewchuk–Brown 2013 algorithm/theorem/performance
sections and BRIO round construction/theoretical premises. The read author's
inccdt.pdf is ten-page SoCG 2013, not the 2015 journal full text. Publisher's
Chin–Wang abstract/bibliography was verified, but full algorithm not acquired.
Seidel 1988 original report also not acquired; root notified of these gaps.

Private pdftotext copies live in context/direct-cdt-study/ for reading. Existing
Geogram revision and archive restoration remain in papers/reference-catalog.tsv;
no duplicate source acquisition. Study-only CDT extraction retains original
file bytes; no instructions from extracted references were applied.

## Independent oracle design review

Reviewed docs/DIRECT_CDT_ORACLE_DESIGN.md and PLAN against the actual existing
qualify_external, qualify_domain, _simple_domains and Python Lawson implementation.
ACCEPT semantic design: full rational matrix determinant checks local CDT only
after complete source incidence/domain/embedding; constrained exemption and zero
policy match the intended fixed-site constant-metric CDT. Proposed mutation
flippability guards must not become oracle excuses to skip actual positive
violations.

Sent root and oracle owner two concrete points: align stale plan steps that wait
for nonexistent native API/RED with the currently authorized missing-oracle RED
and actual Python Lawson consumers; explicitly check source node inventory before
case.points=dict(nodes) or set projection loses duplicate IDs, extra unreferenced
sites or repeated loop occurrences. This remains bounded input admission, not a
new framework or new triangulator scope. No product or test execution in review.

## Strategy and scoped oracle implementation review

Reviewed root docs/DIRECT_CDT_STRATEGY.md and GPU-frontier study: ACCEPT the
architecture and guarantee wording. The hybrid's bound is not confused with
Chew's full constrained merge; BRIO/location requirements, exact bit costs,
source-input quantization and GPU driver/build limitations are explicit. No
claim of executing the GPU reference was introduced.

At root's request, limited actual-oracle review to original source inventory,
embedding and constraint semantics; the predicate colleague owns determinant/
metric arithmetic review. Original-row checks now resolve the dictionary/set
collapse finding. Sent owner/root one remaining admission finding: Python float
and bool output IDs alias int IDs through dict/set equality, while source IDs
claim uint64 identity. Explicit output-ID validation (and bool input rejection)
closes this bounded checker gap. No production-native defect or new geometry
execution was claimed. Exact arithmetic review is not duplicated here.
