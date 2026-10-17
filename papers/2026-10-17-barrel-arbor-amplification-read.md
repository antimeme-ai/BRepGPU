# Read-only barrel-arbor amplification audit

After-images notes, 2026-10-17. No product edit, build, probe, debugger, new geometry
run or selected repair. Source data were parsed as metadata only; one OLD saved
boundary artifact was decoded without evaluating geometry. Current captured
face/site count, failing bound reason and work-budget owner remain unobserved.

## Observed source and scale

Original definition1755:45faces/45distinct carriers,17plane/15cylinder/13cone,
101edges/64vertices. Physical curves:48circle,40line,13quadratic B-spline.
Thus a shared-carrier union across these45faces is not this definition's explanation.
Many literal UV hints span a whole angular period and a thin V strip; they are
hints, NOT the immutable prepared model bank. No rounded-hint=bank assumption.
The previous failed Quad append had no free slot and crossed65,536live records;
planar E<=3N−6 implies N>=21848 if the embedded simple-graph invariant holds. This
is only a lower bound, not a measurement of actual N or proof which face grew.

## Concrete amplification paths in current code

1. `adaptive_regions.cpp::plan_construction_refinement`1887–1968 adds ONE new
central strict-interior anchor for EACH failed Band facet, retaining old anchors
(`adaptive_trimmed.cpp::append_anchors`489). It also splits one containing cache
leaf. For a component with A anchors/B boundary sites, F=2A+B+2H−2C; if most F
facets fail, adding F anchors makes A_next≈3A+B+2H−2C. This is a possible per-horizon
amplification, not an observed growth series. The central lambda>=1/6 rule removes
the old triangle and bounds direct-star child area; the subsequent CDT does not
retain that star and has no global diameter/angle contraction theorem.
2. `RegionBuilder::band_facets`1493–1524 retriangulates the ENTIRE changed connected
Band component, including its previously accepted triangles, then marks all new
Band facets changed. Independent unchanged components reuse work correctly. This
creates repeated full-component admission, coordinate conversion, D&C/recovery,
normalization and rebounding as A grows; it is not incremental cavity insertion.
All retained anchors are imported/sealed and requested again each construction
round, although the SAME numerical PointPool can reuse values.
3. Frozen footprint (`apply_footprint`638–665) forbids later Band→Packet growth.
Classification marks CLOSED source-segment contact as Band (`classify`499), while
cylinders declare V affine-separable (`source_cache.cpp`531) and never need V
cache splitting for regular patch error (`fair_axis`368). A thin rectangular
trim whose initial leaves span its full V extent can consequently be all Band,
not a small collar. Cones may similarly have few V rows. These are constructive
conditions, not proof that the hot face has this layout.
4. Bounds depend on the actual emitted affine L, not cache depth alone. Analytic
source bound (`adaptive_trimmed_numeric.hpp`155–222) uses nodal residual plus
(Muu*hu²+2Muv*hu*hv+Mvv*hv²)/8; cone mixed term is nonzero. Its precise route uses
the same sufficient derivative form at higher arithmetic precision. Cached
clipped bounds preserve SAME L and add source-to-bilinear error; cache refinement
alone cannot repair a truly inaccurate L. Rebuilding many triangles can expose
new long/coupled directions even after prior triangles passed.

## Metric hypothesis checked

`face_metric`758–817 uses averaged opposite-edge secants of ONE SELECTED leaf's
four actual Qs, with exact UV widths; freezes its SPD Cholesky factor through
rebuilds. It does NOT use the whole original0/2pi corner chord. Analytic prepared
U panels are <=common pi/2 lower (`source_cache::angular_panels`294–337), then cache
refines further. Therefore whole-period source hints do not establish a collapsed
U metric. A fixed first-band-leaf metric may still poorly represent a cone with
varying radius; this is a quality hypothesis needing the actual leaf/factor/face,
not a diagnosed bug or a reason to change the metric now.

## Boundary controller and saved evidence

Boundary allowance is minimum incident-face epsilon (`adaptive_curves.cpp`415).
`complete_trace_intervals`1801 accepts physical chord<=epsilon AND selected-carrier
trace+authoritative-domain<=epsilon. Delta remains the separate ORIGINAL V/Ce/S
node guard; no fractional interior margin is reserved. Whole-face actual bounds
use the same epsilon and actual Q. Ordinary point-floor logic tests S(UV)-Q at
vertices; a physical source-point floor returns typed unresolved (`submit_floors`
514), whereas other point floors promote the SAME authority.

Ordinary plan_construction_refinement does NOT populate its BoundaryKey requests.
The state machine supports forwarding such requests, but currently ordinary
bound failures go to interior anchors/cache splits; actual boundary requests
come from coarse/invalid chosen-loop evidence and cap/master producers. This is
an absent ordinary boundary-limited recovery route, not proof of1755's cause.
The finite design explicitly claims no universal termination/shape-quality;
there is no established contract violation or observed whole-edge floor here.

OLD standalone boundary-preflight-v5/definition-1755/query-0.bgacrv:1104nodes,
1003physical intervals,202uses/2006trace intervals. Maximum retained selected-trace
upper/epsilon=.8852765275794856; none>.9. Thus this OLD snapshot does not even show
near-epsilon retained trace uppers. Upper bounds cannot prove a positive actual
floor. Standalone raw profiles differ from prepared MODEL banks; it is not the
current model boundary generation and cannot diagnose its margins/failure.

## Latest work-limit ambiguity

Root reports v9 pair failed native1 `exact work budget exhausted` after CPU1587s/
P1001549s;256MiB avoided the earlier allocation failure. Budget::charge uses the
same message at multiple levels: model context2^44; outer scheduler step2^42;
FaceTrim remaining cumulative2^42; Surface/curve child2^34,total2^44; ordinary/cap
TrimFacetLimits.work=1,000,000,000 per bound horizon. step_facets limits each local
context to min(child_work,remaining1e9) and accumulates slices. The last observed
memory stack is not evidence which work owner now exhausted. No automatic cap
increase or algorithm selection follows without that missing stage/reason data.
