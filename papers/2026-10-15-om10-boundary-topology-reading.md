# OM10 near-joint sampling and topological approximation

The complete-source diagnostic found planar faces167654/167656 whose sampled
five-arc boundary intersects. Current code reproduces the failure. The full
nonvisual run was stopped before edits; no actual watch rasterization occurred.
The geometry and independent-oracle owners are studying the original circles,
declared joints and candidate schedules. This note selects no implementation.

## Literature read

K. E. Jordan, J. Li, T. J. Peters and J. A. Roulier, **Isotopic equivalence by
Bézier curve subdivision for application to high performance computing**,
Computer Aided Geometric Design31(9),642–655,2014,
[DOI](https://doi.org/10.1016/j.cagd.2014.07.002),
[author preprint](https://tpeters.engr.uconn.edu/Top-Bezier4b.pdf).
Read assumptions, angular convergence, separation of consecutive pieces and
the pipe-neighborhood conditions in sections2,5,6 and8. Distance convergence
alone is insufficient for topological equivalence. Their results add regularity,
angular and separation conditions; the tubular-neighborhood arguments have
additional smoothness assumptions. These are conditional results for simple
composite Bézier curves, not a theorem validating our tolerant imported joints
or our sampled chords. The useful design lesson is to check joint neighborhoods
and global separation alongside approximation error.

The [Held laboratory's approximation project](https://www.cosy.sbg.ac.at/~held/projects/apx/apx.html)
also distinguishes geometric tolerance from preserving the topology of input
collections. Its polygon-to-curve approximation direction differs from our
source-curve-to-chord task; it is background, not an adopted algorithm.

## Consequences for the current source study

Our original near-joint circle coefficients do not establish exactly disjoint
real curves: the saved center distance differs from the radius difference by
about1.5e-16mm. Do not claim an exact isotopy of that rounded analytic input.
Preserve the declared original boundary graph and report the actual source
discrepancy. A candidate sampling change must preserve every original edge,
full directed parameter interval, original endpoint and mandatory knot, share
its final schedule across all incident faces, and pass the existing whole-
interval world/UV approximation budgets and exact polygon-domain checks.
Removing an optional sample does not remove the original curve interval it
approximates; that interval still needs its bound. No epsilon bypass of a real
chord intersection or silently altered source coefficient is acceptable.

The candidate under study coordinates near-joint circular parameter distances.
Calling a radius-scaled parameter an exact arc length would overstate what the
rounded original frames establish. Acceptance must inspect actual saved XYZ/UV
and all incident faces, rather than rely on that name or a nominal tangency.

## Acquisition

Author preprint retained intact as
`papers/jordan-li-peters-roulier-2014-isotopic-equivalence.pdf` (ignored),
SHA-256`dee1f6669e7123c9cbcb43b584fc7a196a87b8c461cc2f6eb6caacb538833b0e`.
Restore with:

```sh
curl -fL https://tpeters.engr.uconn.edu/Top-Bezier4b.pdf \
  -o papers/jordan-li-peters-roulier-2014-isotopic-equivalence.pdf
```
