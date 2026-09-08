#!/usr/bin/env python3
"""Reuse the first pass's unchanged signal and level-matching measurements.

Usage: python3 Experiments/Realism20260908Pass3/AnalyzeRealism.py before after output
The only presentation changes identify the added plain-string control and
explain the full-level articulation stress score's preserved float headroom.
"""

from pathlib import Path
import runpy
import sys
import re


if __name__ == "__main__":
    shared = Path(__file__).resolve().parents[1] / "Realism20260908/AnalyzeRealism.py"
    runpy.run_path(str(shared), run_name="__main__")
    directory = Path(sys.argv[3])
    page = directory / "listening.html"
    content = page.read_text().replace(
        "Electry realism · before and after", "Electry realism · pass 3").replace(
        "Legato probes stay on that same physical string.",
        "Slides stay on that string except the explicitly named plain E4 control. "
        "The eight new harmonic and fast-slide phrases appear first, followed by the 39 earlier checks.").replace(
        "These comparisons have not been assigned listening or realism scores.",
        "Full-level harmonic and articulation stress phrases exceed 0 dBFS in raw dry audio in both versions; "
        "the float files preserve those peaks and these listening copies have safe headroom. "
        "These comparisons have not been assigned listening or realism scores.")
    cards = list(re.finditer(r"<section>.*?</section>", content, re.DOTALL))
    if len(cards) != 47:
        raise ValueError("Expected 47 phrases before presenting the third-pass audition")
    ordered = cards[-8:] + cards[:-8]
    content = (content[:cards[0].start()] + "\n".join(m.group() for m in ordered)
               + content[cards[-1].end():])
    for style, label in ((3, "Natural harmonics"), (4, "Pinch harmonics")):
        for position in (10, 50, 90):
            content = content.replace(f"harmonic style{style} position{position}",
                                      f"{label} · Pick Position {position}%")
    for style, label in ((0, "Sustain"), (1, "Palm mute"), (6, "Dead")):
        content = content.replace(f"finger held repick style{style}",
                                  f"Held finger · {label} repicks")
    page.write_text(content)
