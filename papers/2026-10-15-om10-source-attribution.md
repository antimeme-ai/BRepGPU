# OM10 source attribution for the rendered study

Checked the association's primary material on 2026-10-15 while implementing the
whole-source renderer. The supplied original MAN VA01.3 STEP remains private
and byte-identical, SHA-256
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.

The [General Conditions](https://openmovement.org/general-conditions/) identify
published plans as CC BY-SA 3.0 and separately state that 2D/3D data may have
purchase terms. The [technical sheet](https://openmovement.org/documents/OM10/OM10_Technical_Information_20210314.pdf)
identifies the movement and documentation with CC BY-SA 3.0, names Association
openmovement and describes credit, license-link and adaptation notices. It also
describes marking manufactured movements; this study produces illustrations.

Retain Association openmovement, OM10 MAN VA01.3 and a link to the project and
[CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) with source-derived
artwork. Identify our actual changes: owned tessellation, illustrative palette,
camera/lighting, authored explosion and labels. Publish those rendered assets
under the same license, separately from the repository's software license.
Attribute BRepGPU for its implementation and presentation without implying OM
endorsement. Keep original CAD and operational binary snapshots ignored.

The exact current General Conditions HTML is retained intact in
`quarantine/om10/general-conditions-20261015.html`, 99,762 bytes,
SHA-256 `2840ffbdf4f215f28ea598cdefdb4f49db8217893c03cc3b5065a12ff49c2622`.
Restore with `curl -fL https://openmovement.org/general-conditions/ -o quarantine/om10/general-conditions-20261015.html`;
dynamic site markup may change the resulting digest. The original retained
technical PDF and its source/hash are in the watch acquisition catalog.
