# Original curve literature

`acquisition.tsv` records original URLs, intact bytes' SHA256 and actual reading
scope. The PDFs are ignored. Restore each file from its URL, preserving the
download bytes, then compare `shasum -a 256 FILE` with the recorded hash. Extracted
reading text remains in ignored `context/adaptive-curves-*.txt`.

Previously acquired Filip1986, Balazs2004 and Marussig/Hughes2017 remain in
`papers/whole-mesher-2026-10-16/`, with their existing acquisition manifests.
Reference code remains in ignored quarantine at the pinned source revision;
this literature acquisition adopts no production library or source code.

Nairn/Peters/Lutterkort1999: Purdue technical-report landing page was readable,
but the PDF download returned403. That full text was not read in this unit;
Karavelas/Kaklis/Kostas2004 is the acquired/read primary descendant used for the
limited polynomial control-polygon comparison. No theorem is claimed from an
unread Nairn paper.
