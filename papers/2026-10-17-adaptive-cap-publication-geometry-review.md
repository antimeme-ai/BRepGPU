# Cap publication source and normal review

Scoped ACCEPT of `docs/adaptive-cap-publication-unit.md`, read against actual
`adaptive_cap_domain.hpp`, `adaptive_trim_types.hpp`, `scene_view.cpp::normal_flags`
and `om10/math.hpp::source_evaluate`.

The mapped source region is the finite reference quadrilateral, with both lower
corners mapping to the same stored pole Q. The proposed records retain that map,
source-role permutation, current physical cut authority and original occurrence
sectors. Exact refs must be read from their actual owner tables. Neither the
pole locator triangle nor a fabricated original edge can stand for this region.

The existing consumer's sphere limit is the appropriately signed cross product
of the original X/Y columns. Its cone limit is
`sin(alpha)*cross(ep,e)+cos(alpha)*cross(ep,Z)`, multiplied by the sign of the
regular radial sheet. These are the limits of ORIGINAL Su cross Sv for the
declared sheet, including nonorthogonal frames. The new retained kind/sheet/
angular descriptor supplies the needed semantics without another renderer ABI.
Original face orientation is applied once.

Using the exact angular-piece midpoint as a cosmetic normal locator, while
copying actual XYZ verbatim and retaining original locator refs separately, is
coherent. Sector identity must participate in display dedup. Existing finite
point/derivative/unit-normal refusal remains required in the actual CPU/P100
consumer; preparation must not silently replace a failed source normal.
No geometry execution or extra normal evaluator was added for this review.

Actual writer and scene source reread: ACCEPT. BGACRV3/BGATRM2 read exact
values from the owning pools, map local wedge ordinals to the paired snapshot
by original incidence and finite UV equality, retain independent full domain
keys and both original meridian intervals. The originating sampler row is
explicitly diagnostic rather than a serialized PointPool pointer.

Actual adaptive_scene.cpp checks source-kind, full cap identity, role inverse,
sheet and exact angular sector against the produced piece. It copies stored Q,
uses exact-midpoint cosmetic pole locators, retains original UV refs, includes
piece identity in display dedup and applies source face orientation once.
No second normal evaluation is introduced by the adapter.

The existing native_adaptive_scene fixture and original-output reader now admit
an explicit quotient bank and inspect optional mapped-normal piece provenance.
The native fixture checks exact midpoint locators from their produced piece and
keeps original UV refs and stored XYZ words unchanged. The Python reader checks
finite source quads with the SAME whole-source mechanism, original analytic
sphere/cone normals (including sheet/sector), physical IDs and actual API2 jets/
projected instance normals. North CPU passed448 facets; cone P1000 passed80
facets using the existing source-profile display module. Geometry construction
in this fixture remains CPU; the latter checks actual P100 display evaluation.
These are the two requested finite consumers, not a new numerical cohort or a
complete imported-watch result. No additional rerun was started.
The normal reader additionally derives the regular sheet from original cut
cos(V) or cone radius and checks the retained sign, so a descriptor/consumer
sign error cannot agree silently. Saved outputs passed this strengthened
source check; no native operation was repeated.
