# Seeded projection qualification

Requested backend `cuda`, device `1`, CPU thread option `1`. 104 requests; 0 direct violations.

The corpus checks supplied-input exact geometry at returned binary64 UV, distance, box residual, status/counters and known analytic/curvature fixtures. It includes all16 degree pairs, singular and admitted conditioned charts, initial saddles/flats, boundary cones, zero update/search budgets and a stable distance-change witness. Stationarity is a first-order candidate result; it is not a minimum or global nearest guarantee.

| Reported-field error | Maximum |
| --- | ---: |
| point_0 | 1.1102230246251565e-16 |
| point_1 | 1.2285413170850564e-16 |
| point_2 | 1.0638909757255132e-16 |
| distance | 4.0717591017567059e-16 |
| residual | 2.3492465953705938e-14 |

Requests and controls preserve exact input bits; exact output expectations are hexadecimal integer ratios. Worst witnesses, every violation and work/status distributions accompany this report. Allowances follow the accepted plan's finite-corpus evaluator/rounding budgets, without universal error enclosures. Public snapshot/lifetime/guard/ABI checks run separately in test_projection.py; native fault, compatibility and memory checks are separate mechanisms.
