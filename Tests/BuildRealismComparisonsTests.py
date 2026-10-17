"""Regression: an output inside the canonical repo must receive its DSP diff."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('comparisons', root / 'Tools/BuildRealismComparisons.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class ComparisonBuildTests(unittest.TestCase):
    def test_nested_archive_is_patched_without_changing_outer_repository(self):
        with tempfile.TemporaryDirectory() as tmp:
            outer = Path(tmp)
            subprocess.run(['git', 'init', '-q', str(outer)], check=True)
            original = outer / 'Source/DSP/probe.cpp'
            original.parent.mkdir(parents=True)
            original.write_text('float sample = 1;\n')
            subprocess.run(['git', 'add', '.'], cwd=outer, check=True)
            original.write_text('float sample = 2;\n')
            patch = subprocess.check_output(['git', 'diff', '--', 'Source/DSP'], cwd=outer)
            original.write_text('float sample = 1;\n')
            archive = outer / 'build-review/candidate/source'
            copy = archive / 'Source/DSP/probe.cpp'
            copy.parent.mkdir(parents=True)
            copy.write_text('float sample = 1;\n')
            builder.apply_dsp_patch(archive, patch)
            self.assertEqual(copy.read_text(), 'float sample = 2;\n')
            self.assertEqual(original.read_text(), 'float sample = 1;\n')

    def test_empty_and_inapplicable_patches_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError):
                builder.apply_dsp_patch(Path(tmp), b'')
            with self.assertRaises(RuntimeError):
                builder.apply_dsp_patch(Path(tmp), b'not a patch')


if __name__ == '__main__':
    unittest.main()
