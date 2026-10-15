# Carrier converter: independent source review

2026-10-15. Reviewed actual `python/brepgpu/interchange.py` and its nine direct
conversion tests after implementing the separate source/native qualifier. No
converter edits or local product execution. **Accepted; no blocking finding.**

The converter validates complete typed face/use and NURBS packed arrays before
allocating a namespace or publishing an output. Original NURBS offsets advance
for every carrier, including unsupported faces. Active domains, counts/orders,
positive weights and knot order are checked independently of admission; reversed
face ranges and invalid source use indices/orientations abort the conversion.
Source ranges and every original face-use occurrence remain separate from the
full carrier domain and intrinsic parameter orientation.

The admitted representation is exactly one clamped span per axis, degrees 0..3.
The weight exponent raises the smallest original weight into [1,2); rejecting a
maximum above 256 correctly identifies absence of any common power-of-two scale.
Fraction scaling also handles original subnormal weights without first creating
an overflowing binary64 multiplier. Emitted weights must retain exact ratios.
Spatial scale selection and one-round exact homogeneous products match the
specified envelope. Cartesian source displacement is measured after actual
rounded products; the recorded position and derivative coefficient bounds use
exact original domain spans and unchanged weight ratios.

Original source bytes are hashed and parsed from the same immutable read. The
factory owns tuple records, document and patches; no borrowed foreign handles
survive. Shared body namespace allocation and document-wide source ordinals avoid
collision with canonical bodies or other prims. Unsupported faces remain in the
inventory with no patch. All trim scope markers remain uninterpreted. General
scene/trim/topology conversion and cross-session naming are not implied.

The converter's tests exercise malformed full packing, exact normalization,
unsupported carriers/envelopes and snapshot ownership. The independent whole
family adds literal foreign inventory, exact source and emitted-control jets,
complete actual native descriptor/query binding, invalid/empty rows, and bitwise
frame conversion. Actual fleet execution passed that ten-method family on
CPU1, CPU24 and each P100; those results do not establish general interchange or
external alpha qualification.
