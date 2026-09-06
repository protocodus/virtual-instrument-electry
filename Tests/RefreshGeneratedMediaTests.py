import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "scripts/refresh-generated-media.py"
SPEC = importlib.util.spec_from_file_location("refresh_generated_media", SCRIPT)
UPDATER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(UPDATER)
PNG = b"\x89PNG\r\n\x1a\nfixtureIEND\xaeB`\x82"
README = b"Human introduction\n" + UPDATER.PEAKS_BEGIN + b"\nold table\n" + UPDATER.PEAKS_END + b"\nHuman footer\n"


@unittest.skipUnless(shutil.which("git"), "Git is required for publication tests")
class RefreshGeneratedMediaTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="electry-media-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.remote = self.root / "remote.git"
        self.seed = self.root / "seed"
        self.checkout = self.root / "checkout"
        self.media = self.root / "media"
        self.env = dict(os.environ, GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull)
        self.git(self.root, "init", "--bare", "--initial-branch=main", str(self.remote))
        self.git(self.root, "init", "--initial-branch=main", str(self.seed))
        self.git(self.seed, "config", "user.name", "Fixture")
        self.git(self.seed, "config", "user.email", "fixture@example.invalid")
        self.git(self.seed, "config", "core.autocrlf", "false")
        self.put(self.seed, "README.md", README)
        self.put(self.seed, "source.txt", b"source version one\n")
        self.put(self.seed, UPDATER.SCREENSHOT, PNG)
        self.put(self.seed, "Docs/audio/00-retired.wav", b"retired demo")
        self.put(self.seed, "Docs/audio/listening-tests/frozen.wav", b"frozen evidence")
        self.git(self.seed, "add", ".")
        self.git(self.seed, "commit", "-m", "source")
        self.source = self.git(self.seed, "rev-parse", "HEAD").strip().decode()
        self.git(self.seed, "remote", "add", "origin", str(self.remote))
        self.git(self.seed, "push", "origin", "main")
        self.git(self.root, "clone", str(self.remote), str(self.checkout))
        self.git(self.checkout, "config", "core.autocrlf", "false")
        self.put(self.media, "README.md", README.replace(b"old table", b"new table"))
        self.put(self.media, UPDATER.SCREENSHOT, PNG.replace(b"fixture", b"new screenshot"))
        for index in range(1, 24):
            self.put(self.media, f"Docs/audio/{index:02d}-demo.wav", f"RIFF demo {index}".encode())

    def git(self, repo, *args):
        result = subprocess.run(["git", *args], cwd=repo, env=self.env,
                                capture_output=True, check=False)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        return result.stdout

    @staticmethod
    def put(root, relative, content):
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)

    def remote_show(self, path):
        return self.git(self.remote, "show", f"main:{path}")

    def tip(self):
        return self.git(self.remote, "rev-parse", "main").strip()

    def publish(self, success=True):
        result = subprocess.run(
            [sys.executable, str(SCRIPT), "--source-commit", self.source,
             "--media-dir", str(self.media), "--remote", str(self.remote)],
            cwd=self.checkout, env=self.env, capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, success, result.stderr)
        return result

    def upstream(self, path, content):
        self.put(self.seed, path, content)
        self.git(self.seed, "add", path)
        self.git(self.seed, "commit", "-m", "concurrent update")
        self.git(self.seed, "push", "origin", "main")
        return self.tip()

    def assert_published(self):
        self.assertEqual(self.remote_show("README.md"), README.replace(b"old table", b"new table"))
        self.assertEqual(self.remote_show(UPDATER.SCREENSHOT), PNG.replace(b"fixture", b"new screenshot"))
        self.assertEqual(self.remote_show("Docs/audio/23-demo.wav"), b"RIFF demo 23")
        self.assertEqual(self.remote_show("Docs/audio/listening-tests/frozen.wav"), b"frozen evidence")
        names = self.git(self.remote, "ls-tree", "-r", "--name-only", "main").decode().splitlines()
        self.assertNotIn("Docs/audio/00-retired.wav", names)
        self.assertEqual(sum(UPDATER.root_wav(name) for name in names), 23)

    def test_combined_update_and_unchanged_noop(self):
        self.publish()
        self.assert_published()
        published = self.tip()
        self.assertIn("unchanged", self.publish().stdout)
        self.assertEqual(self.tip(), published)
        self.assertEqual(self.git(self.remote, "rev-list", "--count", "main").strip(), b"2")

    def test_new_source_prevents_stale_update(self):
        latest = self.upstream("source.txt", b"new source\n")
        self.assertIn("Skipped", self.publish().stdout)
        self.assertEqual(self.tip(), latest)
        self.assertEqual(self.remote_show(UPDATER.SCREENSHOT), PNG)

    def test_human_readme_change_is_preserved(self):
        changed = README.replace(b"Human introduction", b"Edited human introduction")
        latest = self.upstream("README.md", changed)
        self.assertIn("Skipped", self.publish().stdout)
        self.assertEqual(self.tip(), latest)
        self.assertEqual(self.remote_show("README.md"), changed)

    def test_generated_table_commit_does_not_suppress_screenshot(self):
        latest = self.upstream("README.md", README.replace(b"old table", b"other generated table"))
        self.publish()
        self.assert_published()
        self.assertEqual(self.git(self.remote, "rev-parse", "main^").strip(), latest)

    def test_invalid_media_does_not_write_repository(self):
        self.put(self.media, UPDATER.SCREENSHOT, b"truncated PNG")
        self.publish(success=False)
        self.assertEqual(self.tip().decode(), self.source)
        self.assertEqual(self.git(self.checkout, "status", "--porcelain"), b"")

    def test_non_fast_forward_push_rechecks_and_reapplies_media(self):
        original_git = UPDATER.git
        raced_tip = None

        def competing_push(repo, *args, **kwargs):
            nonlocal raced_tip
            if args[0] == "push" and raced_tip is None:
                raced_tip = self.upstream("README.md", README.replace(b"old table", b"racing table"))
            return original_git(repo, *args, **kwargs)

        previous_directory = Path.cwd()
        try:
            os.chdir(self.checkout)
            with mock.patch.dict(os.environ, self.env):
                with mock.patch.object(UPDATER, "git", side_effect=competing_push):
                    UPDATER.refresh(self.source, self.media, str(self.remote), "main")
        finally:
            os.chdir(previous_directory)
        self.assert_published()
        self.assertEqual(self.git(self.remote, "rev-parse", "main^").strip(), raced_tip)
        self.assertEqual(self.git(self.remote, "rev-list", "--count", "main").strip(), b"3")


if __name__ == "__main__":
    unittest.main()
