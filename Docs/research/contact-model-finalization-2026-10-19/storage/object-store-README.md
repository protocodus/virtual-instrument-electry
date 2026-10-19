# Lossless cold storage of closed payloads

This is a standard ZIP archive. Each unique byte payload appears once as
`objects/<SHA256>`. The JSON index maps all **504 original paths** to the 305
objects and records each original's size/hash, mode, UID/GID, nanosecond mtime
and original device/inode. Identical contents do not imply identical metadata.
Creation preserved every original. A separate, freshly authorized retirement
run subsequently closed exit 0 after the root and peer actual-byte checks.

The original frozen code, metadata, result summaries and proof documents stay
in their original locations. Their historical payload hashes remain valid;
verifiers that require a retired original file need that file restored first.
The archive changes storage location, not the preserved evidence or its status.

* [ZIP archive](archive-attempt-v1/objects-v1.zip) — 728,792,930 bytes;
  SHA256 `472f143ab43306c2217de43d057f7ee1887cffe073b99a5a67cc01843c74c8c9`.
* [Complete path and metadata index](archive-attempt-v1/object-index-v1.json) —
  SHA256 `5c706cc87268263bd94182acbcb2f18996b484b4f1716b6e414bb43849fcf410`.
* [Creation receipt](archive-attempt-v1/execution-receipt-v1.json),
  [root actual verification](root-actual-verification-v1.json),
  [fixed inventory](../build-frozen-payload-object-archive-inventory/inventory-v1.json),
  [implementation manifest](implementation-manifest-v1.json).
* [Peer actual verification](../build-frozen-payload-object-store-peer-review/actual-byte-verification-v1/receipt-v1.json)
  independently verified all 305 objects and 504 originals. A final ZIP
  filename or a creation receipt alone does not authorize removal or
  restoration.
* [Closed retirement receipt](retirement-attempt-v1/execution-receipt-v1.json) —
  SHA256 `200825b2b0dfc19bffa6292a2dc58f8df59e0fb406b4436588853421db021e2f`.
  The run records all 504 originals removed, all 504 leaves confirmed absent
  with valid parents, and passing post-operation protected metadata, static
  bindings and archive checks.
* [Durable per-path retirement journal](retirement-attempt-v1/per-path-journal.jsonl) —
  SHA256 `569fe5c5350c64270c00f067469662434d72f2287d4c7c8f650469bfb58e672c`.
* [Independent post-retirement verification](../build-frozen-payload-object-store-peer-review/retirement-postcheck-v1/receipt-v1.json)
  passed: all 504 valid-parent absences and 1,008 ordered intent/success
  journal records match. The archive, index, protected metadata and Source24
  remain unchanged. No restoration was performed.

The exact net **logical saving is 714,677,084 bytes** versus the closed archive
package, before retirement and external verification metadata. This accounting
does not establish a causal change in physical free space on APFS.

## Selective restoration

The frozen [restorer](restore_store_draft_v1.py) requires a **fresh exact
authorization**, including the selected original paths in order, request ID,
archive/index/script/helper hashes and restore-only permission. It verifies
the entire archive and all objects before installing any selected file.
Example CLI (placeholders require the actual reviewed authority):

```sh
/Users/vojta/Dev/electry-real-dry-20261018/build-real-dry-review/python312/bin/python \
  /Users/vojta/Dev/electry-fundamental-20261018/build-frozen-payload-object-store/restore_store_draft_v1.py \
  --authorization /absolute/path/to/fresh-restore-authorization.json \
  --authorization-sha256 AUTHORIZATION_SHA256 \
  --index /Users/vojta/Dev/electry-fundamental-20261018/build-frozen-payload-object-store/archive-attempt-v1/object-index-v1.json \
  --request-id REVIEWED_UNIQUE_REQUEST_ID \
  --path /exact/original/path/from/index
```

Repeat `--path` for each explicitly authorized original. Existing targets are
never overwritten. Each alias receives an independent copy/inode with its own
verified metadata; aliases are not hardlinked together. Failure preserves
completed copies and partial evidence without automatic retry or repair.
The original UID/GID must be restorable by the invoking user; there is no
privilege assumption or metadata waiver.

The tool also enforces the frozen pinned Python312 environment, file bindings
and Source24. Later runtime or Source changes can therefore block automatic
restoration. Do not edit the frozen manifest or bypass a check to force it.
The standard ZIP objects remain independently readable; a different future
restoration route needs its own explicit review and authorization.

Logical-byte and `st_blocks` accounting does not establish causal physical
space recovery on APFS. Host free space changes while other jobs and swap run.
No audio, physics, naturalness or production qualification follows from storage
verification.
