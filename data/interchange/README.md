# Foreign interchange fixture

Unchanged `TestFiles/brep_validator/mouse.usda` from
[NVIDIA USD BRep](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/TestFiles/brep_validator/mouse.usda),
commit `c9c5979f04cea8c38213004d4f04df0dcfb40d05`.
37,613 bytes; SHA256
`0fedf1649d094ad38adb14095ddd0fef888016c80245ef488d66b70db1b4d39d`.
Upstream Apache 2.0 license is retained in LICENSE.usd-brep; no separate fixture
notice was present. This is source data, not adopted upstream implementation.

The file supplies four original NURBS faces. Two have supported single clamped
spans; two contain interior knots. All four have general trims. Carrier
conversion does not interpret those trims or import a solid. Keep every original
face/use and unsupported outcome. See the [design](../../docs/INTERCHANGE_DESIGN.md).
