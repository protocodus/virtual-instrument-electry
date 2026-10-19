199 closed historical traces are stored in `archive-v1/closed199-v1.tar.gz`.
All cases are included, including negative results, failed runs and empty
streams. Their bytes have not been transformed beyond lossless compression.

Original payload: 2,504,347,387 bytes. Archive: 245,901,527 bytes.
The archive replaces the original files at the exact 199 paths listed in
`archive-v1/archive-index-v1.json`; it saves 2,258,445,860 bytes before the small
administrative records. Active source experiments, reference recordings,
capture data, NPZ histories, code, old manifests and old result receipts remain
at their existing paths.

Archive SHA-256:
`c883ef93d24bd06626037100b56b58d7e9165e6de982cb858e3dedaa05ba27f6`

Index SHA-256:
`9c4e4d9be8fd56848a05282f91157bd902cefebf2327a1a6b8af05b49bb7b101`

The original archive execution and two independent checks verified every
member's hash, size, order, file mode and exact nanosecond timestamp against
the frozen historical proof records. Both independent checks also verified
owner/group and consumed gzip through EOF/CRC. Original retirement then
rehashed all originals, checked open handles and protected dependencies,
synced archive/index/journal records, removed exactly the approved names and
verified the archive hash again. See:

- `archive-v1/execution-receipt-v1.json`
- `root-completed-verification-v1/receipt-v1.json`
- `../build-closed-trace-lossless-archive-peer-review/post-archive/receipt-v1.json`
- `retirement-v1/execution-receipt-v1.json`
- `retirement-v1/per-path-retirement-journal-v1.jsonl`
- `../build-closed-trace-lossless-archive-peer-review/post-retirement/receipt-v2.json`

The independent post-retirement check confirmed all 398 journal rows (one
intent and one success per file), the absence of exactly 199 original leaves
under valid parent directories, unchanged protected and historical metadata,
and the unchanged archive hash. Its first checker attempt stopped on a
metadata schema mismatch before accessing the journal or archive; that failed
attempt is retained alongside the corrected, successful second checker. The
cleanup itself was executed once.

Old validators that read a retired raw path require its restoration first.
Their historical PASS/failure receipts have not been rewritten. The archive
is a standard gzip/PAX tar containing repository-relative member names.
It can also be inspected or extracted into an empty directory using standard
archive tools without touching live experiment data.

`restore_draft_v1.py` provides selective restoration to original paths. It
requires an exact request manifest binding its script hash, index hash,
selected member names and unique request ID. It re-verifies the whole archive
and all 199 members before installing a selected file, preserves bytes/mode/
nanosecond mtime, and refuses existing destinations or symlink traversal.
Use the pinned Python 3.12.8 runtime named in the script. No restoration has
been performed during this cleanup. Restored files have new filesystem inodes;
the once-only archival and retirement attempts must not be rerun.
