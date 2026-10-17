# OM10 exploded-assembly presentation: source study

The actual4K poster was inspected. Its native layout is proportional centroid
separation, not functional explosion. At852215550f3e7db634533226c795991aff4fe913,
native/mesher/presentation.cpp::make_presentation sets radial displacement to
.2×strength×distance from the whole-assembly center and axial displacement to
strength×axial center distance. adaptive_model_main.cpp calls that function;
render_adaptive_gallery.py preserves its saved fields. The older name-tiered
examples/om10_scene.py::authored_layout is not in this actual path. Large plates
therefore remain layered closely while peripheral fasteners move conspicuously.

Actual source hierarchy, read on fleet, has31 assembly groups, including20 named
top-level non-leaf modules. The named plate, winding mechanism, barrel, train
mobiles, bridges, anchor, cock and balance/spiral provide real grouping authority.
Grouping and explanatory displacement must remain distinct from source contacts
or a physical removal order. Repeated usages retain separate identity.

Primary illustration reference: Agrawala, Phan, Heiser, Haymaker, Klingner,
Hanrahan and Tversky, *Designing Effective Step-By-Step Assembly Instructions*,
SIGGRAPH2003, sections2 and5.2. The authors relate semantic hierarchy to clear
visual organization. Their presenter builds stacks with accumulated separation
and uses alignment guides; it warns that common fixed displacement can leave
new parts mutually occluded. We select hierarchy, accumulated clearances and
guides as presentation principles. Their contact/blocking/assembly planner is
not adopted or claimed. Our translations are authored illustration choices.
[Primary PDF](https://graphics.stanford.edu/papers/assembly_instructions/assembly.pdf)
was acquired unchanged as ignored papers/pdfs/agrawala-2003-assembly-instructions.pdf,
SHA2569d649a8da979cb184bc43daecf0c1a57a2c35dc9dbb2f68695fe4c797153d2a2.

The already acquired [OM10 technical sheet](https://openmovement.org/documents/OM10/OM10_Technical_Information_20210314.pdf),
p2, explicitly distinguishes transmission, setting work and regulation and names
the train stages. The original STEP/BGsource hierarchy provides the actual parts
and placements for this revision. The acquired official ETA6497-1 communication,
revision13 (2025-04-15), p12, shows a recognizably exploded mainplate/wheel/bridge
stack with axial guides and separate fasteners. It is a visual reference, not
OM10 geometry or an OM10 service sequence; no reference image is copied into
our poster. PDF hashes/paths are in the existing watch-drawing acquisitions TSV.

The selected design is a central source-aligned plate/train/bridge stack plus
coherent power, winding, regulator and dial/calendar lanes. Each166 source
occurrence receives one recorded translation; original orientations and all
stored geometry remain unchanged. Complete legend and major visible-module
labels replace166 long crossing leaders through one opaque mass. The exact
layout and finite acceptance plan are in docs/om10-exploded-assembly-unit.md.
This is presentation-only study, no new dependency, render or mesher change.
