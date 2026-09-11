#!/usr/bin/env python3
"""Generate matched dry/Modern audition pairs for ordinary playing."""
from pathlib import Path
import runpy
import sys

if __name__ == "__main__":
    shared = Path(__file__).resolve().parents[1] / "Realism20260908/AnalyzeRealism.py"
    runpy.run_path(str(shared), run_name="__main__")
    page = Path(sys.argv[3]) / "listening.html"
    text = page.read_text().replace("Electry realism · before and after", "Electry · everyday playing, realism pass five")
    text = text.replace("E1 is the open low string;\nF♯1 is fret 2 of the Drop-E build. Legato probes stay on that same physical string.",
        "Start with short note stops, fretted palm chugs and power chords. "
        "Sustained notes expose the pitch settling; the pick-edge examples compare three hardness settings. "
        "The MIDI score records physical string selection and every control change.")
    text = text.replace("These comparisons have not been assigned listening or realism scores.",
        "Before is commit 3baafa6. After contains all five changes. "
        "Signal measurements describe what changed; these audition pairs have no assigned realism scores.")
    for midi, label in ((28, "E1 · lowest string"), (40, "E2 · open wound string"), (64, "E4 · plain high string")):
        text = text.replace(f"sustained note {midi}", f"Sustain · {label}")
        text = text.replace(f"short note stops {midi}", f"Short note stops · {label}")
    for value, label in ((15, "Soft"), (50, "Medium"), (95, "Hard")):
        text = text.replace(f"pick edge {value}", f"{label} pick · three velocities")
    for value, label in ((0, "Downstroke"), (1, "Upstroke"), (2, "Alternate")):
        text = text.replace(f"power chords pick {value}", f"Power chords · {label}")
    page.write_text(text)
