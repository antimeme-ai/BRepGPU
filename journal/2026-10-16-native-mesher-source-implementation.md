# Native immutable source unit

Sub-plan: implement the accepted source/value milestone in native C++20. Decode
all existing BGSRC001 fields directly into immutable typed rows and flat pools,
retain original bytes, validate lengths/counts/ownership and build reusable
incident CSR. Source units are retained metadata; the adapter already emits mm.
Run allocations use the aggregate resource. Source IDs remain distinct from slots.

The direct fixture independently writes source bytes. It checks original signed
zero/control words, inventory, ID resolution, metadata and lifetime accounting;
then attacks every truncated prefix, structural fields, weights, cyclic usages
and allocation/count limits. The first fleet build is RED because read_source
and the immutable data constructor/lookup have no implementation. No old mesher
or watch worker is involved. Implementation now follows that observed RED.
