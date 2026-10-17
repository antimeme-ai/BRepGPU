# Binary64 conversion costs

conversion-costs.tsv contains one observation before/after the known-dyadic
Rational::from_double change on identical saved finite bit sequences. The timed
native loop converts each value and consumes the resulting integer limbs;
warm-up and input loading are excluded. Existing Budget counters measure work,
arbitrary divisions and peak live bytes. Pair checksums agree.

The exponent/sign strata contain34,138 values repeated8 times. The original
source-field prefix contains262,144 finite values from unchanged source data;
it does not represent mesher output-point frequency. Input hashes identify the
packets. There is no repeated timing distribution or whole-mesh speedup claim.
The independent canonical-word oracle, source review and sanitizer scope are
recorded in journal/2026-10-17-native-binary64-dyadic.md.
