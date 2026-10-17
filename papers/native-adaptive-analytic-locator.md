# Original analytic inverse proposals for reconstructed traces

2026-10-17. The selected mechanism and finite direct oracle are in
[the unit design/plan](../docs/adaptive-analytic-locator-unit.md).

The inspected OCCT8.0.1 ElSLib inverse implementations (pinned revision
b8f597c677811d1f9f4d8a97f5ae2825c0353a42, ElSLib.cxx1558–1710) directly motivate
analytic seeds. ConeParameters specifically projects onto a generator instead
of setting V=z/cos(alpha), and treats the other apex sheet by reversing radial
direction. SphereParameters preserves neither our arbitrary frame metric nor
an occurrence's lifted sector; TorusParameters handles a spindle alternative
outside our separated-ring profile. These differences are explicit in our plan.

Our exact original frame dual and bounded physical correction are owned
derivations. SISL s1773_s9dir's tangent Gram system and Hu/Wallner2005 section3
explain why a local tangent step is useful; neither establishes a global inverse
or source correspondence. Existing projection literature/catalog is reused:
[numerical study](2026-10-13-projection-numerical-study.md),
[primary-source catalog](projection-numerical-sources.tsv),
[PDF read catalog](projection-numerical-pdfs.tsv). No new dependency is adopted.

The failed actual watch node is distinguished from a source-infeasibility claim.
A reconstructed guide is a locator proposal. Its roughly1.2878um departure
does not justify raising the1um node cap when a substantially better original
cone locator exists. All mathematical acceptance remains with the original
V/Ce/S enclosure guard and whole-interval same-Q bounds; ordinary inverse
arithmetic can only nominate the UV those existing operations test.
