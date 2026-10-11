"""Synthetic qualification-boundary regressions; no native/artifact qualification claims."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "windows/packaging"))
import verify_sdk_release as qualification


class QualificationBoundary(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name).resolve()
        self.repo, self.build = self.root / "source", self.root / "build"
        self.repo.mkdir()
        self.commit, self.fingerprint = "1" * 40, "2" * 64
        self.dirty = False
        self.bindings = {"canonical": {"sourceCommit": "3" * 40}, "engineeringPolicy": {"sourceCommit": "4" * 40},
                         "realizationProfiles": {"sourceCommit": "5" * 40, "profiles": []}}
        self.put(self.repo / "VERSION", b"1.0.0-rc.1\n")
        (self.repo / "windows/tests").mkdir(parents=True)
        qualification.write_json(self.repo / "windows/tests/test-inventory.json", {"tests": ["ash-test-one.exe", "ash-test-two.exe"]})
        self.put(self.build / "generated/tests/CanonicalFixtures.hpp", b"synthetic authority fixtures\n")
        self.git_patch = patch.object(qualification, "git", side_effect=lambda repo, *args: self.commit if args == ("rev-parse", "HEAD") else " M changed" if self.dirty else "")
        self.snapshot_patch = patch.object(qualification, "verify_source_snapshot", side_effect=lambda repo, snapshot: self.fingerprint)
        self.git_patch.start()
        self.snapshot_patch.start()
        self.addCleanup(self.git_patch.stop)
        self.addCleanup(self.snapshot_patch.stop)
        self.addCleanup(self.temporary.cleanup)
        self.completed, steps = {}, []
        for key in qualification.MATRIX_KEYS:
            target, config = key.split("-")
            binary = self.build / "bin" / key
            library = self.put(binary / "ash-core.lib", ("synthetic library " + key).encode())
            header_relative = f"generated/{key}/include/ash/Version.hpp"
            header = self.put(self.build / header_relative, ("synthetic identity " + key).encode())
            binaries = []
            for name in ("ash-test-one.exe", "ash-test-two.exe"):
                item = self.put(binary / name, ("synthetic test " + key + name).encode())
                binaries.append({"name": name, "sha256": qualification.digest(item)})
            metadata = {"sourceCommit": self.commit, "sourceDirty": False, "sourceSnapshot": {}, "platform": target,
                        "configuration": config, "version": "1.0.0-rc.1", "diagnosticsProfile": "Release" if config == "Release" else "Development",
                        "settings": {"runtimeLibrary": "/MT" if config == "Release" else "/MTd", "languageStandard": "C++20",
                                     "wholeProgramOptimization": False, "debugInformationFormat": "/Z7"},
                        "library": {"name": "ash-core.lib", "sha256": qualification.digest(library)},
                        "generatedIdentity": {"relativeName": header_relative, "sha256": qualification.digest(header)}, "binaries": binaries,
                        "toolchain": {"toolsVersion": "14.44.35207", "windowsSdk": "10.0.26100.0"}}
            qualification.write_json(binary / "build-metadata.json", metadata)
            log = self.put(self.root / ("build-" + key + ".log"), b"synthetic successful build log\n")
            step = {"name": "build-" + key, "execution": "executed", "status": "pass", "exitCode": 0,
                    "cwd": str(self.repo), "log": str(log), "logSha256": qualification.digest(log),
                    "command": ["powershell", "-File", str(self.repo / "windows/build/build.ps1"), "-Configuration", config,
                                "-Platform", target, "-BuildRoot", str(self.build), "-Rebuild"]}
            steps.append(step)
            self.completed[key] = qualification.completed_build(self.repo, self.build, key, step)
        direct_log = self.put(self.root / "direct.log", b"synthetic direct solution log\n")
        direct_step = {"name": "direct-solution-x64-Release", "execution": "executed", "status": "pass", "exitCode": 0,
                       "cwd": str(self.repo), "log": str(direct_log), "logSha256": qualification.digest(direct_log),
                       "command": ["MSBuild.exe", str(self.repo / "windows/build/ash-windows.sln"), "/t:Build",
                           "/p:Configuration=Release", "/p:Platform=x64", "/p:ASHBuildRoot=" + str(self.build),
                           "/p:VCToolsVersion=14.44.35207", "/p:WindowsTargetPlatformVersion=10.0.26100.0"]}
        steps.append(direct_step)
        prerequisite_steps = copy.deepcopy(steps)
        for name in qualification.PREREQUISITE_COMMANDS:
            if name == direct_step["name"]: continue
            log = self.put(self.root / (name + ".log"), b"synthetic mandatory command log\n")
            prerequisite_steps.append({"name": name, "execution": "executed", "status": "pass", "exitCode": 0,
                                       "command": ["synthetic-regression-command", name], "log": str(log), "logSha256": qualification.digest(log)})
        steps = copy.deepcopy(prerequisite_steps)
        for name in qualification.POST_PACKAGE_COMMANDS:
            log = self.put(self.root / (name + ".log"), b"synthetic post-package command log\n")
            steps.append({"name": name, "execution": "executed", "status": "pass", "exitCode": 0,
                          "command": ["synthetic-regression-command", name], "log": str(log), "logSha256": qualification.digest(log)})
        prerequisite = self.root / "package-prerequisites.json"
        qualification.write_json(prerequisite, {"status": "pass", "sourceCommit": self.commit, "authorityBindings": self.bindings,
                                                "steps": prerequisite_steps})
        self.assets = []
        for target in ("x64", "ARM64"):
            archive = self.put(self.root / ("synthetic-" + target + ".zip"), ("synthetic zip " + target).encode())
            self.assets.append({"name": archive.name, **qualification.file_evidence(archive)})
        self.previous = {"sourceCommit": self.commit, "sourceDirty": False, "sourceFingerprint": self.fingerprint,
                         "canonicalCommit": self.bindings["canonical"]["sourceCommit"], "policyCommit": self.bindings["engineeringPolicy"]["sourceCommit"],
                         "profileSourceCommit": self.bindings["realizationProfiles"]["sourceCommit"], "authorityBindings": self.bindings,
                         "status": "fail", "finalSourceVerified": True, "incompleteMandatoryGates": ["independentReview"],
                         "mandatoryGates": {**{name: "pass" for name in qualification.SUBSTANTIVE_GATES}, "independentReview": "fail"},
                         "builds": {key: True for key in qualification.MATRIX_KEYS}, "completedBuilds": self.completed,
                         "packagePrerequisitesEvidence": qualification.file_evidence(prerequisite), "steps": steps, "assets": self.assets,
                         "directSolution": {"status": "pass", "outputsUnchanged": True, "step": direct_step,
                             "outputs": {key: self.completed["x64-Release"][key] for key in
                                 ("metadata", "library", "generatedIdentity", "fixtureHeader", "binaries")}}}
        self.prior_path = self.root / "qualification-report.json"
        qualification.write_json(self.prior_path, self.previous)
        self.current = copy.deepcopy(self.previous)
        evidence = qualification.file_evidence(direct_log)
        self.review = {key: copy.deepcopy(self.previous[key]) for key in
                       ("sourceCommit", "sourceDirty", "sourceFingerprint", "canonicalCommit", "policyCommit", "profileSourceCommit", "authorityBindings")}
        self.review.update({"reviewer": "Synthetic independent regression reviewer", "independent": True,
                            "implementationAuthored": False, "humanApprovalClaimed": False,
                            "qualificationEvidence": qualification.file_evidence(self.prior_path),
                            "libraries": {target: self.completed[target + "-Release"]["library"]["sha256"] for target in ("x64", "ARM64")},
                            "sdkAssets": qualification.asset_identity(self.assets),
                            "conclusions": {name: {"status": "accepted", "evidence": "Synthetic regression evidence: direct solution log",
                                                   "evidenceFiles": [evidence]} for name in qualification.REVIEW_FIELDS}})
        self.review_path = self.root / "independent-review.json"

    def put(self, path, data):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def verify_review(self, review=None):
        qualification.write_json(self.review_path, self.review if review is None else review)
        return qualification.verify_artifact_review(self.review_path, self.current, self.repo, self.build)

    def verify_prior(self, previous=None):
        qualification.write_json(self.prior_path, self.previous if previous is None else previous)
        return qualification.verified_prior_qualification(self.prior_path, self.repo, self.build, self.bindings)

    def test_complete_synthetic_binding_accepted_without_native_claim(self):
        self.assertEqual(self.verify_prior()["status"], "fail")
        self.assertEqual(self.verify_review()["status"], "pass")

    def test_every_failed_substantive_gate_rejects_reuse(self):
        for name in qualification.SUBSTANTIVE_GATES:
            with self.subTest(gate=name):
                previous = copy.deepcopy(self.previous)
                previous["mandatoryGates"][name] = "fail"
                previous["incompleteMandatoryGates"] = [name, "independentReview"]
                with self.assertRaises(ValueError): self.verify_prior(previous)

    def test_missing_inconsistent_or_stale_prior_identity_rejected(self):
        mutations = {"sourceCommit": "9" * 40, "sourceDirty": True, "finalSourceVerified": False, "sourceFingerprint": "9" * 64,
                     "canonicalCommit": "9" * 40, "policyCommit": "9" * 40, "profileSourceCommit": "9" * 40,
                     "authorityBindings": {}, "builds": {}, "completedBuilds": {}, "incompleteMandatoryGates": [], "status": "pass"}
        for key, value in mutations.items():
            with self.subTest(field=key):
                previous = copy.deepcopy(self.previous)
                previous[key] = value
                with self.assertRaises((ValueError, KeyError)): self.verify_prior(previous)

    def test_each_completed_file_tamper_rejected(self):
        for key in qualification.MATRIX_KEYS:
            record = self.completed[key]
            for file in [record["metadata"], record["library"], record["generatedIdentity"], record["fixtureHeader"], *record["binaries"]]:
                path = Path(file["path"])
                original = path.read_bytes()
                with self.subTest(configuration=key, path=path.name):
                    path.write_bytes(original + b"changed")
                    try:
                        with self.assertRaises((ValueError, KeyError)): self.verify_prior()
                    finally: path.write_bytes(original)

    def test_source_changed_after_completed_build_rejected(self):
        self.fingerprint = "8" * 64
        with self.assertRaises(ValueError): self.verify_prior()

    def test_dirty_source_rejected(self):
        self.dirty = True
        with self.assertRaises(ValueError): self.verify_prior()

    def test_failed_or_mismatched_original_build_evidence_rejected(self):
        for key, value in {"status": "fail", "exitCode": 1, "execution": "reused", "cwd": str(self.root),
                           "name": "other-build", "command": [], "logSha256": "9" * 64}.items():
            with self.subTest(field=key):
                previous = copy.deepcopy(self.previous)
                previous["completedBuilds"]["x64-Release"]["buildStep"][key] = value
                with self.assertRaises(ValueError): self.verify_prior(previous)
        previous = copy.deepcopy(self.previous)
        previous["steps"] = [previous["directSolution"]["step"]]
        with self.assertRaises(ValueError): self.verify_prior(previous)

    def test_changed_build_log_and_prerequisite_rejected(self):
        for record in (self.completed["ARM64-Debug"]["buildStep"], self.previous["packagePrerequisitesEvidence"]):
            path = Path(record.get("log", record.get("path")))
            original = path.read_bytes()
            path.write_bytes(original + b"changed")
            try:
                with self.assertRaises(ValueError): self.verify_prior()
            finally: path.write_bytes(original)
        prerequisite = Path(self.previous["packagePrerequisitesEvidence"]["path"])
        qualification.write_json(prerequisite, {"status": "fail", "sourceCommit": self.commit, "authorityBindings": self.bindings})
        self.previous["packagePrerequisitesEvidence"] = qualification.file_evidence(prerequisite)
        with self.assertRaises(ValueError): self.verify_prior()

    def test_direct_solution_missing_command_or_changed_outputs_rejected(self):
        for field, value in {"status": "fail", "outputsUnchanged": False, "outputs": {}}.items():
            previous = copy.deepcopy(self.previous)
            previous["directSolution"][field] = value
            with self.subTest(field=field):
                with self.assertRaises(ValueError): self.verify_prior(previous)
        previous = copy.deepcopy(self.previous)
        previous["directSolution"]["step"]["command"] = ["MSBuild.exe", "/t:Build"]
        with self.assertRaises(ValueError): self.verify_prior(previous)

    def test_all_six_conclusions_require_status_narrative_and_hashed_files(self):
        for name in qualification.REVIEW_FIELDS:
            for field, value in {"status": "not_reviewed", "evidence": " ", "evidenceFiles": [],
                                 "evidenceFiles_corrupt": [{"path": self.previous["directSolution"]["step"]["log"], "sha256": "0" * 64, "size": 1}]}.items():
                with self.subTest(conclusion=name, field=field):
                    review = copy.deepcopy(self.review)
                    review["conclusions"][name]["evidenceFiles" if field == "evidenceFiles_corrupt" else field] = value
                    with self.assertRaises(ValueError): self.verify_review(review)

    def test_review_identity_independence_and_human_claim_rejected(self):
        changes = {"reviewer": " ", "independent": False, "implementationAuthored": True, "humanApprovalClaimed": True,
                   "sourceDirty": True, "sourceCommit": "0" * 40, "sourceFingerprint": "0" * 64,
                   "canonicalCommit": "0" * 40, "policyCommit": "0" * 40, "profileSourceCommit": "0" * 40, "authorityBindings": {}}
        for key, value in changes.items():
            with self.subTest(field=key):
                review = copy.deepcopy(self.review)
                review[key] = value
                with self.assertRaises(ValueError): self.verify_review(review)

    def test_review_binds_exact_qualification_library_and_asset_set(self):
        variants = []
        for field in ("path", "sha256", "size"):
            review = copy.deepcopy(self.review)
            review["qualificationEvidence"][field] = str(self.root / "absent.json") if field == "path" else "0" * 64 if field == "sha256" else 0
            variants.append(review)
        review = copy.deepcopy(self.review)
        review["libraries"]["x64"] = "0" * 64
        variants.append(review)
        for field in ("name", "sha256", "size"):
            review = copy.deepcopy(self.review)
            review["sdkAssets"][0][field] = "different" if field != "size" else 0
            variants.append(review)
        review = copy.deepcopy(self.review)
        review["sdkAssets"].pop()
        variants.append(review)
        for index, review in enumerate(variants):
            with self.subTest(variant=index):
                with self.assertRaises(ValueError): self.verify_review(review)

    def test_current_and_prior_archive_bytes_must_remain_exact(self):
        for asset in self.assets:
            path = Path(asset["path"])
            original = path.read_bytes()
            path.write_bytes(original + b"changed")
            try:
                with self.assertRaises(ValueError): self.verify_review()
            finally: path.write_bytes(original)

    def test_prior_report_changed_after_review_rejected(self):
        self.prior_path.write_bytes(self.prior_path.read_bytes() + b"\n")
        with self.assertRaises(ValueError): self.verify_review()

    def test_reused_build_steps_preserve_actual_original_command(self):
        previous = copy.deepcopy(self.previous)
        previous["steps"] = [step for step in previous["steps"] if not step["name"].startswith("build-")]
        previous["steps"].extend({"name": "build-" + key, "status": "pass", "execution": "reused",
                                 "originalBuildStep": self.completed[key]["buildStep"]} for key in qualification.MATRIX_KEYS)
        self.assertEqual(self.verify_prior(previous)["builds"], {key: True for key in qualification.MATRIX_KEYS})

    def test_all_actual_mandatory_commands_must_pass_even_if_gate_summary_says_pass(self):
        for name in (*qualification.PREREQUISITE_COMMANDS, *qualification.POST_PACKAGE_COMMANDS):
            previous = copy.deepcopy(self.previous)
            next(step for step in previous["steps"] if step["name"] == name)["exitCode"] = 1
            with self.subTest(command=name):
                with self.assertRaises(ValueError): self.verify_prior(previous)

    def test_native_record_report_binds_both_runners_binaries_and_schema_bytes(self):
        authority = self.root / "authority"
        schema_hashes = {}
        for name in ("diagnostic-envelope", "containment-record", "safe-halt-record"):
            file = self.put(authority / "schemas/1.0" / (name + ".schema.json"), b"synthetic selected schema\n")
            schema_hashes[name] = qualification.digest(file)
        runners = [self.put(self.root / ("runner-" + config + ".json"), ("synthetic runner " + config).encode()) for config in ("Release", "Debug")]
        report = {"status": "pass", "canonicalCommit": qualification.PIN, "schemaDigests": schema_hashes,
                  "profiles": [{"runnerReportSha256": qualification.digest(runner), "negativeControls": 9,
                                "binaryDigests": {item["name"]: item["sha256"] for item in self.completed["x64-" + config]["binaries"]}}
                               for config, runner in zip(("Release", "Debug"), runners)]}
        path = self.root / "native-diagnostic-records.json"
        qualification.write_json(path, report)
        self.assertEqual(qualification.native_record_evidence(path, runners, self.completed, authority)["status"], "pass")
        variants = []
        for field, value in {"status": "fail", "canonicalCommit": "0" * 40, "schemaDigests": {}, "profiles": []}.items():
            changed = copy.deepcopy(report); changed[field] = value; variants.append(changed)
        for index in (0, 1):
            for field, value in {"runnerReportSha256": "0" * 64, "negativeControls": 0, "binaryDigests": {}}.items():
                changed = copy.deepcopy(report); changed["profiles"][index][field] = value; variants.append(changed)
        for index, changed in enumerate(variants):
            qualification.write_json(path, changed)
            with self.subTest(variant=index):
                with self.assertRaises(ValueError): qualification.native_record_evidence(path, runners, self.completed, authority)
        qualification.write_json(path, report)
        runners[1].write_bytes(b"changed runner")
        with self.assertRaises(ValueError): qualification.native_record_evidence(path, runners, self.completed, authority)


if __name__ == "__main__":
    unittest.main(verbosity=2)
