#!/usr/bin/env python3
"""Validate matched scores, measure audio and foreground the audible pass-four changes.

Usage: python3 Experiments/Realism20260908Pass4/AnalyzeRealism.py before after output
Uses the original analyzer unchanged for float validation, signal measurements,
constant-gain RMS matching and safe PCM listening copies.
"""
from pathlib import Path
import re
import runpy
import sys

if __name__ == "__main__":
    shared = Path(__file__).resolve().parents[1] / "Realism20260908/AnalyzeRealism.py"
    runpy.run_path(str(shared), run_name="__main__")
    page = Path(sys.argv[3]) / "listening.html"
    content = page.read_text().replace("Electry realism · before and after", "Electry realism · pick attack and ringing")
    content = content.replace(
        "Legato probes stay on that same physical string.",
        "Start with a hard pick: compare the front edge, then the way the ringing tone darkens. "
        "Soft and medium picks show how Pick Hardness changes that edge. "
        "The natural-harmonic example compares the corrected picking mechanism.")
    content = content.replace(
        "These comparisons have not been assigned listening or realism scores.",
        "The before version is the complete third-pass working tree; after includes the fourth pass. "
        "The old natural-harmonic pulse can exceed full scale in raw float audio; "
        "all listening copies have safe headroom. "
        "These are audition pairs, not assigned listening or realism scores.")
    cards = list(re.finditer(r"<section>.*?</section>", content, re.DOTALL))
    if len(cards) != 53:
        raise ValueError("Expected 53 matched phrases")
    focus = [4, 5, 2, 3, 0, 1]
    for title in ("wound sustain fret0", "harmonic style3 position50", "harmonic style4 position50"):
        index = next(i for i, c in enumerate(cards) if f"<h2>{title}</h2>" in c.group())
        focus.append(index)
    ordered = [cards[i].group() for i in focus]
    remainder = [c.group() for i, c in enumerate(cards) if i not in focus]
    content = (content[:cards[0].start()] + "\n".join(ordered)
        + '<details><summary>Show 44 further regression phrases</summary>'
        + "\n".join(remainder) + '</details>' + content[cards[-1].end():])
    content = content.replace("</style>", "summary{cursor:pointer;padding:16px;color:#8dc6ff}details{margin-top:24px}</style>")
    for hardness, label in ((15, "Soft"), (50, "Medium"), (95, "Hard")):
        for kind, phrase in (("ring", "picked sustain"), ("chug", "palm-muted repicks")):
            content = content.replace(f"picked {kind} hardness{hardness}",
                                      f"{label} pick · {phrase}")
    for style, label in ((3, "Natural harmonics"), (4, "Pinch harmonics")):
        for position in (10, 50, 90):
            content = content.replace(f"harmonic style{style} position{position}",
                                      f"{label} · Pick Position {position}%")
    for style, label in ((0, "Sustain"), (1, "Palm mute"), (6, "Dead")):
        content = content.replace(f"finger held repick style{style}", f"Held finger · {label} repicks")
    page.write_text(content)
