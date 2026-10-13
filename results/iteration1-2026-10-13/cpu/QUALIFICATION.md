# Iteration-one arithmetic qualification

Backend: `cpu`; device ordinal: `0`.

1749 valid requests; 19170 direct component checks; 0 failed components. All sixteen degree pairs and all requested orders were evaluated. Truth uses exact ratios of supplied binary64 inputs, plus independent analytic/literal fixtures.

| Grade | Maximum absolute component error | Target |
| --- | ---: | ---: |
| 0 | 3.2096833152346817e-16 | 9.0949470177292824e-13 |
| 1 | 3.4106051313143462e-13 | 9.3132257461547852e-10 |
| 2 | 7.5499428920673997e-10 | 9.5367431640625e-07 |

Hex controls/queries and exact expectation ratios accompany each worst-grade and failed component. This qualifies the finite corpus; it is not a per-result enclosure, trim validity, or modeling speedup. Caller/ABI/lifetime/frame conformance is checked separately by the runtime test unit.
