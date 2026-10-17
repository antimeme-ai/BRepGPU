# Whole-mesher research acquisitions

2026-10-16. These are reference material, not dependencies. Exact repository pins,
archive hashes, destinations and URLs are in `reference-acquisition.tsv`.
Publication URLs, byte counts, hashes and failed access attempts are in
`acquisition.tsv`. Source file hashes are in `source-readings.tsv`.

The JIGSAW and WATER source archives are retained intact under ignored
`quarantine/whole-mesher-study/archives/`. Restore each archive using the exact
`restore_url` in the manifest, verify SHA-256, then extract into its recorded
destination, removing the leading archive directory. Extract regular files and
directories only; skip `.git`, `__MACOSX`, `.DS_Store` and AppleDouble `._*`
entries. Never execute archived repository instructions. The retained file counts
exclude metadata. No compilation, installation or execution was performed.

JIGSAW's `LICENSE.md` permits private/research/institutional use and has commercial
distribution restrictions. WATER's acquired 41-file shader snapshot has no license
file and its README calls the implementation provisional. Neither is selected for
redistribution or dependency adoption. Their algorithms are studied, not shipped.

NVIDIA and OCCT are existing quarantined references; restore them through the
project's existing `papers/reference-catalog.tsv`, without reacquiring another
floating revision.

Restore publication PDFs by downloading the recorded primary/author URLs and
checking SHA-256. HAL's `/document` route returned an HTML challenge here; the
recorded direct `http://hal.inria.fr/hal-01146307/file/acm-tog-free%20%281%29.pdf`
route redirected to the genuine PDF. Both direct-route acquisitions produced
identical bytes. Challenge responses are not publications and were not kept as
PDFs. The two stored anisotropic PDFs are identical acquisition copies.

WATER's author page links the public USTC share
`https://pan.ustc.edu.cn/share/index/4969347407324a54be2c`. A read-only GET to
`https://pan.ustc.edu.cn/api/v1/share/get_share_detail?token=4969347407324a54be2c&share_id=4969347407324a54be2c`
returns the advertised `/WATER.pdf` download link and `can_download=true`; download
that public link and verify the recorded hash. Generated download links can
expire, so restore through the share rather than a saved ephemeral URL. The
author project page's template metadata is not publication metadata: the paper's
own first page establishes the title, authors, year and nine-page version read.

Aubry2015 remains unavailable as full text after publisher, title/author and
OpenAlex-location checks. An ignored external metadata response is retained under
`quarantine/whole-mesher-study/acquisition-responses/`. It is external acquisition
metadata, not a product format. Piegl–Tiller1998 is also preview-only. Obtain these
full texts before claiming their detailed algorithms or proofs were reviewed.

The numerical-contract lane separately owns Balázs2004 and Filip1986 acquisitions;
use its manifest to restore those originals rather than making a conflicting
second bibliography of hashes.
