# 2026-10-13: projection plan oracle review

Reviewed the implementation plan against the revised candidate design and
independent source/oracle study. Saved a no-blocker review with its conditional
distance/gradient/rho allowances, curvature/counter/compatibility coverage and
delivered-value timing assessment.

Derived a real cubic carrier whose full initial Gauss–Newton trial increases
distance at a regular seed; zero backtracks must return LINESEARCH_LIMIT,
retaining the seed with zero accepted updates and two evaluations. Verified the
integrator's normal-target parabolic family: exact known UV/distance and a
uniform positive Hessian lower bound throughout the rectangle. The integrator
accepted both explicit additions before recording the plan disposition.

No product/test code or remote execution during review. Independent fixture/
caller implementation begins only after accepted plan and canonical ABI handoff.
