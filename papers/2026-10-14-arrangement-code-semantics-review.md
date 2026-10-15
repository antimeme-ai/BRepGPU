# Arrangement product, oracle and test semantics review

2026-10-14. **Accepted, with no blocking findings.** Reviewed the complete
`python/brepgpu/arrangements.py`, `python/brepgpu/arrangement_oracles.py`,
`tests/test_arrangements.py`, the direct entry test and
`scripts/run_arrangement_mutations.py` against the accepted arrangement design
and plan. Caller source review and its resolved findings are recorded separately
in `2026-10-14-arrangement-caller-semantics-review.md`. This review used source
inspection only; root owns serialized fleet execution and measurements.

## Geometry, identity and topology

The product constructs the bounded rectangular grid with complete, strictly
interior and mutually separated circle disks. Exact rational inequalities reject
clipping, tangency, overlap, nesting and duplicate disks before publication.
Source sections remain bound to the actual target snapshot and face, including
both operand orders. Their four complete physical pieces, both surface maps,
events, chart joins and component order are checked and retained.

Each circle is one closed edge with one anchor vertex and two opposed uses.
Atlas quarter joins remain source geometry rather than extra arrangement
vertices. Grid edges have opposed twins, and every use belongs to exactly one
loop. Cell boundaries, circle holes, disk boundaries and the exterior have the
specified UV winding and ownership. Original face orientation remains separate
from this UV orientation. Directed original boundary parameters may decrease;
the reader interpolates t0/t1 without applying coedge orientation twice.

Exact area records retain rational and pi coefficients. Cell witnesses lie
strictly before every hole footprint, and disk witnesses are their centers.
Location returns all closed incident regions at grid crossings, target boundary
points and circle boundaries. Outside points return the exterior. Rational
subnormal inputs remain exact. Bounded iterator admission and construction into
local records prevent partial result publication on invalid or late-failing
inputs.

## Independent truth and meaningful tests

The oracle derives original surface coordinates from source controls and frames,
evaluates source map coefficients independently, and invokes the existing
independent whole-section qualifier. It reconstructs grid edge extents and
directed source parameters rather than calling product geometry helpers.
Complete expected directed boundary words, hole membership, twins, next/previous
links and use coverage test the actual region graph. Counts, Euler relations
and total area supplement that comparison; none substitutes for complete loops.
Location truth enumerates closed cells and disks independently of the product's
bisect-based lookup.

The ten arrangement methods plus the entry test cover every owned rectangular
face orientation, translated/scaled frames, non-dyadic cuts, both boundary
parameter directions and both section operand orders. Literal circle reader
coordinates, one-anchor identity, multiple holes in one cell, mixed grid/hole
domains, source order permutations and exact area coefficients are checked.
Closed incidence, subnormal cuts and just-below/exact/just-above tangency cases
attack the important equality distinctions. Invalid iterators, immutable
publication, unsupported sources and reader admission receive direct checks.

The eight production mutants attack omitted holes, wrong twins, broken next
links, reversed source parameters, the wrong UV operand, missing closed grid
incidence, admitted tangent disks and lost use orientation. The controller
requires a completed assertion-failure result and exit status 1; crashes, import
errors and stale bytecode do not earn a kill. Each mutation runs between healthy
baseline and retry executions, with protected native binary hashes unchanged.

Root reports the complete fleet suite green: 72 tests in 12.881 seconds, with
one existing GPU-only skip. All eight actual mutants were assertion-killed with
healthy baseline and retry runs. Those are root's execution results, distinct
from this source review. The accepted claim remains this exact CPU arrangement
family; general circle overlay, clipping, arbitrary curves, topology edits and
GPU construction remain outside it. No further source changes are requested.
