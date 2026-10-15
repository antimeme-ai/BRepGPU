# Strict source parser: adversarial semantics review

Disposition: **accepted**, no blocking finding. Source-only review of
`python/brepgpu/usda_subset.py` and all ten `tests/test_usda_subset.py` methods
against the accepted interchange design and the acquired OpenUSD grammar.

The parser consumes EOF and never returns a partial Document. Header/UTF-8
validation, one-token lexer consumption and explicit expected delimiters reject
trailing garbage, truncated arrays/strings, unknown escapes and unsupported
characters. Unique absolute prim paths, property identifiers and metadata-key
sets reject collisions before immutable records are published. Wrapper prims
cannot author properties; BrepArray children, scene composition, relationships,
time samples, transforms and unknown declaration/metadata types fail closed.
Original parsed attribute arrays remain available as immutable tuples, including
trim evidence not interpreted by the converter.

Integers require integer literals and declared 32-bit ranges; bools have their
own token path. Scalar/vector type names survive in records. Decimal-to-binary32
rounding corrects the binary64-intermediate midpoint ambiguity by comparing the
original exact Decimal against exact representable float32 neighbor midpoints.
The max-finite overflow midpoint and signed underflow/zero cases follow that
same rule. Nonfinite binary64 or binary32 results fail. Direct tests check a
literal decimal immediately above/below an even midpoint rather than merely
comparing to the same conversion machinery.

Byte, token, scalar-component, prim and nesting bounds are checked before adding
past each limit. Nesting includes metadata, arrays and vector tuples as well as
prims. Large integer lexical inputs are bounded before Python integer creation;
the file budget also bounds individual strings and numeric literals. None of
these limits silently truncates input.

The accepted comment syntax currently uses `#`; other upstream comment forms
are rejected by this declared subset. This is adequate for the unchanged foreign
fixture and observed reference exports, and does not claim general USD grammar
coverage. No production edit or local product execution was performed.
