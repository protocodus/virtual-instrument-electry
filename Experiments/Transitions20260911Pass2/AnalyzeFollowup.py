#!/usr/bin/env python3
"""Run the established audio checks and identify the second-pass audition."""
import importlib.util
from pathlib import Path
import sys

original = Path(__file__).resolve().parents[1] / "Transitions20260911/AnalyzeTransitions.py"
spec = importlib.util.spec_from_file_location("transition_analysis", original)
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)

if __name__ == "__main__":
    analysis.main()
    output = Path(sys.argv[3])
    page = output / "listening.html"
    text = page.read_text()
    text = text.replace("<title>Electry · Open / mute / open</title>",
                        "<title>Electry · Transition refinement 2</title>")
    text = text.replace("<h1>Open. Palm down. Open again.</h1>",
                        "<h1>Open / mute / open · Second pass</h1>"
                        "<p><strong>Before</strong> is the previous transition pass. "
                        "<strong>After</strong> corrects spare-string damping and "
                        "preserves palm relaxation through a reopened pick. "
                        "The first three riffs use exactly the same score. "
                        "Two additional low-string rhythms expose the spaces between attacks.</p>")
    page.write_text(text)
    # Validate again after the title/introduction change; audio and controls
    # remain the established player implementation.
    import json
    manifest = json.loads((output / "score.json").read_text())
    analysis.validate_listening_artifacts(output, manifest)
