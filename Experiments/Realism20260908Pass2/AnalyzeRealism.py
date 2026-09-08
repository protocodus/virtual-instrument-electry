#!/usr/bin/env python3
"""Reuse the first pass's unchanged signal and level-matching measurements.

Usage: python3 Experiments/Realism20260908Pass2/AnalyzeRealism.py before after output
The only presentation changes identify the added plain-string control and
explain the full-level articulation stress score's preserved float headroom.
"""

from pathlib import Path
import runpy
import sys


if __name__ == "__main__":
    shared = Path(__file__).resolve().parents[1] / "Realism20260908/AnalyzeRealism.py"
    runpy.run_path(str(shared), run_name="__main__")
    directory = Path(sys.argv[3])
    page = directory / "listening.html"
    page.write_text(page.read_text().replace(
        "Legato probes stay on that same physical string.",
        "Slides stay on that string except the explicitly named plain E4 control.").replace(
        "These comparisons have not been assigned listening or realism scores.",
        "The rapid articulation stress phrase exceeds 0 dBFS in raw dry audio in both versions; "
        "the float files preserve those peaks and these listening copies have safe headroom. "
        "These comparisons have not been assigned listening or realism scores."))
