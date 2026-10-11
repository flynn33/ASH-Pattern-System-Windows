"""Exercise branch and annotated-tag event metadata without GitHub writes."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[2] / ".github/scripts/no_attribution_check.py"
spec = importlib.util.spec_from_file_location("sentinel", SOURCE)
sentinel = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sentinel)


class AttributionEvents(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.previous = Path.cwd()
        os.chdir(self.temporary.name)
        self.git("init", "-b", "main")
        self.git("config", "user.name", "Jim Daley")
        self.git("config", "user.email", "94642455+flynn33@users.noreply.github.com")
        Path("file.txt").write_text("native SDK\n", encoding="utf-8")
        self.git("add", "file.txt")
        self.git("commit", "-m", "Initial native product")
        self.base = self.git("rev-parse", "HEAD")
        Path("file.txt").write_text("native SDK candidate\n", encoding="utf-8")
        self.git("commit", "-am", "Package native SDK candidate")
        self.head = self.git("rev-parse", "HEAD")
        self.git("tag", "-a", "v1.0.0-rc.1", "-m", "Windows SDK candidate")

    def tearDown(self):
        os.chdir(self.previous)
        self.temporary.cleanup()

    @staticmethod
    def git(*args):
        return subprocess.check_output(["git", *args], stderr=subprocess.STDOUT, text=True).strip()

    def test_normal_branch_push(self):
        selector = sentinel.commit_selector("push", {"before": self.base, "after": self.head})
        self.assertEqual(selector, f"{self.base}..{self.head}")
        self.assertEqual(sentinel.commits(selector), [self.head])

    def test_new_branch_explicit_inventory(self):
        self.assertEqual(sentinel.commit_selector("push", {"before": sentinel.ZERO_SHA,
            "after": self.head, "ref": "refs/heads/release/candidate", "commits": [{"id": self.head}]}), [self.head])

    def test_empty_new_branch_fails_closed(self):
        with self.assertRaises(sentinel.AttributionError):
            sentinel.commit_selector("push", {"before": sentinel.ZERO_SHA, "after": self.head, "commits": []})

    def test_annotated_tag_empty_commit_array(self):
        event = {"before": sentinel.ZERO_SHA, "after": self.git("rev-parse", "v1.0.0-rc.1"),
                 "ref": "refs/tags/v1.0.0-rc.1", "commits": [], "head_commit": {"id": self.head}}
        self.assertEqual(sentinel.commit_selector("push", event), [self.head])
        errors = []
        sentinel.check_commit_metadata([self.head], errors)
        sentinel.check_tag_metadata("push", event, errors)
        self.assertEqual(errors, [])
        self.assertEqual(sentinel.changed_files([self.head]), {"file.txt"})

    def test_tag_target_mismatch(self):
        with self.assertRaises(sentinel.AttributionError):
            sentinel.commit_selector("push", {"after": self.base, "ref": "refs/tags/v1.0.0-rc.1"})

    def test_tag_payload_requires_actual_object_sha_and_valid_ref(self):
        for after, ref in (("HEAD", "refs/tags/v1.0.0-rc.1"), (self.head, "refs/tags/invalid name")):
            with self.subTest(after=after, ref=ref), self.assertRaises(sentinel.AttributionError):
                sentinel.commit_selector("push", {"after": after, "ref": ref})

    def test_deleted_tag_cannot_qualify_publication(self):
        for payload in ({"deleted": True, "after": self.head}, {"deleted": False, "after": sentinel.ZERO_SHA}):
            with self.subTest(payload=payload), self.assertRaises(sentinel.AttributionError):
                sentinel.commit_selector("push", {**payload, "ref": "refs/tags/v1.0.0-rc.1"})

    def test_prohibited_tag_message_detected(self):
        self.git("tag", "-a", "v1.0.0-rc.2", "-m", "Created using " + "Open" + "AI")
        errors = []
        sentinel.check_tag_metadata("push", {"ref": "refs/tags/v1.0.0-rc.2"}, errors)
        self.assertEqual(len(errors), 1)


if __name__ == "__main__":
    unittest.main()
