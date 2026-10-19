# Experimental contact-model research packet

This packet preserves research sources and historical evidence at finalization.
It is outside the production build. None of these files changes the instrument,
its version, its defaults, or previously released audio behavior.

Read the [status report](../contact-model-finalization-2026-10-19.md) first.
`packet-manifest-v1.json` lists the exact published bytes. The separate
`snapshot-provenance-v1.json` maps copied files to their original local research
paths and hashes. Historical receipts retain their original paths and scopes;
those local paths are provenance, not portable dependencies supplied by Git.

| Directory | What is preserved | Qualification at publication |
| --- | --- | --- |
| `kernel` | Capacity-five C++17 kernel, drivers, algebra review and compile receipts | Algebra/source review and strict Apple arm64 compile/link only; no native solver run |
| `replay` | Independent oracle, complete input recipes, archived route registration and runner drafts | Source-reviewed, unexecuted; original v1 and corrected v2 both retained |
| `event-service` | Event-relative controller, planned controls and contact-retirement derivation | Source/algebra only; controls and continuous physical lifecycle unexecuted |
| `source-study` | Exact frozen simulation sources, method, manifest, saved-audit source and closure metadata from the separate 96-case study | Closed incomplete at 95/96; independent audit refused; no export or audio analysis |
| `comparison` | Export, saved-render checker and analysis source drafts, corrections, protocols and source reviews | Unexecuted; incomplete source blocks their prerequisites; no new tone result |
| `storage` | Completed preservation/cleanup records | Historical storage operations, not sound-quality evidence |

The replay runner's v1 is retained as rejected source history: its inherited
compile-review path validation and cleanup-success reporting needed correction.
The v2 source addresses those two findings. Neither runner was executed. No
execution manifest or portable input corpus is supplied here. The copied Python
modules and C++ drivers are research snapshots, not a supported command-line
workflow. Do not substitute the archive directory for an authorized live run.

The simulation manifest also names earlier drafts retained in `source-study/source`.
Their inclusion preserves history. Only the selected entry and its bound protocol
belong to the final attempt; older drafts receive no qualification by inclusion.

The fixture stiffness, contraction bounds and resource ceilings identify no
physical finger, fret or guitar. Compilation establishes no force, energy,
realtime, naturalness, recording-parity or eight-string hardware result.
No binary, generated field history, PCM, recording or third-party dataset is
redistributed in this packet. Original failed/partial studies and archives
remain preserved in their local evidence roots.

Verify only the packet's byte integrity, without importing research modules:

```sh
/Users/vojta/Dev/electry-real-dry-20261018/build-real-dry-review/python312/bin/python \
  Docs/research/contact-model-finalization-2026-10-19/verify_packet.py
```

The verifier uses only the standard library. A successful verification proves
the included file hashes and report binding, not the scientific claims in a
receipt or the existence of every historical local dependency.
