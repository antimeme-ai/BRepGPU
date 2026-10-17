# Imported analytic component reader: scoped source mathematics

Read tests/native_adaptive_component.py curve_terms, carrier_terms,
expression_box, original_facet, original_chord and original_boundary_checker,
and the extended cylinder_trig in native_adaptive_trimmed_caller.py.
No native or oracle run was added by this review.

Scoped ACCEPT within the explicit original analytic spatial-curve and
plane/cylinder carrier profile. Both literal curve harmonic amplitudes and all
nonorthogonal frame coefficients remain original values. The expression helper
cancels only equal exact original arguments before interval trig; it does not
identify approximate phases or stored full-turn constants. The exact Taylor
extension retains a finite term cap and checks that both first-omitted tails
are decreasing before applying the alternating-tail enclosure.

The L1 coefficient sum bounds the vector second derivative. Its h-squared/8
chord bound and scalar-parameter variance bound for a triangle are conservative.
Virtual recursion carries the SAME actual L by exact midpoint interpolation;
it does not resample source positions into the mesh. Original endpoint/source
node guards compare V/Ce/S, while Q departure is positional. The explicit
unsupported rational-clock/carrier assertions prevent this finite reader from
claiming general imported component coverage. These conclusions concern oracle
mathematics and source semantics, not the separate observed execution outcomes.
