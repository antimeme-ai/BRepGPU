# Owned source carrier conversion subplan

Implement the complete `interchange.py` immutable conversion unit and its direct
conversion tests after accepted design/plan reviews. Parser ownership is separate;
consume its typed immutable Document rather than tokenizing again. Preserve all
source faces/uses and copied NURBS records, validate packed extents before slicing,
then admit only the designed single-span numerical envelope. Compute homogeneous
controls from exact input binary64 ratios and measure actual emitted-control
position/partial bounds. Use the existing shared body namespace and Patch API.

Write direct specification tests first and record actual missing-converter red
on the fleet. Implement after that red, then run only this CPU conversion suite
on the fleet after parser availability; no native rebuild or local product run.
Tests cover real foreign packing, literal rational order/UV/identity, exact
coefficient bounds, degree zero, malformed arrays and complete unsupported
inventories, envelope limits and immutable source-file ownership. Parent and
independent oracle owners review source before final qualification.
