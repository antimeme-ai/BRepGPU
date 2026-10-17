# Cross-owner immutable exact values

Sub-plan: implement the accepted runtime plan's qualified exact table. Shared
geometry must not capture mutable arithmetic budgets. The coordinator assigns
table identity before work; one builder appends canonical rational words into
flat run-accounted storage, seals it and loses mutation access. A consumer
imports those immutable views into its own budget. Actual output reindexes only
semantic references later; temporary pool order is not output geometry.

The direct fixture destroys the source arithmetic owner before reading/importing
the retained table, checks literal3/7, rejects a foreign generation and writes
after sealing, and checks shared lifetime. Observe the missing implementation
on the fleet before writing the implementation. No second bigint or shared
global mutable builder is introduced.

Actual missing-symbol fleet RED was followed by the flat-table implementation.
The direct fixture is GREEN under the strict release and ASan/UBSan/leak
profiles. Geometry owner independently reviewed append reservation ordering,
qualified identity, sealing, destination-only import and retained resource
lifetime: ACCEPT. Its clarification is explicit in the header: ExactView is
borrowed; retain the ExactTable or its owning task lease throughout use.
Actual-source clang-tidy/format checks are GREEN. No new registry/certifier or
second bigint was introduced.
