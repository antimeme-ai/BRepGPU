# After the complete bounded arrangement beachhead

Continue uncovered broad-pass themes before strengthening this geometry family.
The first arrangement supports full axial cuts and separated circle disks
strictly inside cells, complete regions and exact source provenance. These
notes select no new implementation, dependency or language architecture.

- Clipped/transverse circle cuts need new exact boundary events, cyclic branch
  ordering, split original boundary ranges and complete resulting region domains.
  Existing SSI endpoints are useful inputs; their existence does not determine
  a correct arrangement embedding by itself.
- Tangencies, nesting, overlapping/coincident disks and coincident line/curve
  spans need explicit contact/ownership semantics. Zero-dimensional contacts,
  pinched loops and lower-dimensional cells cannot silently become ordinary
  positive-area regions or disappear through tolerance welding.
- Source faces with existing holes, finite/nonaxial chords and general conics
  require separately complete domain and event models. A rectangle carrier is
  insufficient to preserve its trim holes. The current operation creates hole
  loops but does not remove material or import a general trimmed face.
- Cross-face Boolean assembly must establish shared curve/vertex identity,
  material selection, normal/coedge orientation, closed candidate topology and
  transactional history. Current arrangement IDs are result-local; source
  body/revision/face/piece/event identities survive, but persistent edited-body
  naming and cross-result edge unification are not delivered by local IDs.
- General event scheduling/GPU region construction can use owned count/scan/fill
  after geometry and ownership are specified. Preserve the complete semantic
  graph under scheduling, splitting and compaction. GPU CSR alone does not
  establish an arrangement or Boolean speedup.
- Later native bulk geometry/region consumers and reusable application storage
  belong with the deferred integration and
  [Python-role decision](2026-10-14-python-role-return-pass.md). Preserve exact
  source ranges and immutable live results; moving arithmetic to C++ requires
  an owned exact strategy or an explicitly discussed dependency adoption.

Primary [semantics](2026-10-14-arrangement-semantics-study.md),
[runtime](2026-10-14-arrangement-runtime-study.md) and
[independent region truth](2026-10-14-arrangement-oracles-study.md) studies record
the literature/source basis and the boundaries of their actual reading.
