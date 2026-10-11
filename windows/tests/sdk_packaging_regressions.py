"""Synthetic integrity/negative tests; these do not qualify native binaries."""
import copy
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zipfile

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "windows/packaging"))
import canonical_resources as resources
import sdk_release as sdk


class ReferenceIntegrity(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.lock = {"repository": resources.REPOSITORY, "sourceCommit": resources.PIN, "files": [
            {"path": "conformance/1.0/manifest.json", "size": 3, "sha256": resources.sha256(b"{}\n"),
             "gitBlob": resources.hashlib.sha1(b"blob 3\0{}\n").hexdigest()}]}
        self.lock_path = self.root / "lock.json"
        self.lock_path.write_text(json.dumps(self.lock), encoding="utf-8")
        self.cache = self.root / "cache"
        (self.cache / "conformance/1.0").mkdir(parents=True)
        (self.cache / "conformance/1.0/manifest.json").write_bytes(b"{}\n")
        manifest = {"sourceCommit": resources.PIN, "resourceLockSha256": resources.sha256(self.lock_path.read_bytes()), "files": self.lock["files"]}
        (self.cache / "authority-resource-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
        self.mock_lock = patch.object(resources, "LOCK", self.lock_path)
        self.mock_lock.start()

    def tearDown(self):
        self.mock_lock.stop()
        self.temporary.cleanup()

    def test_complete_cache_offline(self):
        with patch.object(resources.urllib.request, "urlopen", side_effect=AssertionError("Network attempted")):
            self.assertEqual(resources.validate_cache(self.cache)["status"], "pass")
            self.assertEqual(resources.acquire(self.cache)["resourceCount"], 1)

    def test_missing_required_resource(self):
        (self.cache / "conformance/1.0/manifest.json").unlink()
        with self.assertRaises(ValueError): resources.validate_cache(self.cache)

    def test_corrupt_exact_bytes(self):
        (self.cache / "conformance/1.0/manifest.json").write_bytes(b"{}\r\n")
        with self.assertRaises(ValueError): resources.validate_cache(self.cache)

    def test_partial_cache_without_completion_manifest(self):
        (self.cache / "authority-resource-manifest.json").unlink()
        with self.assertRaises(ValueError): resources.validate_cache(self.cache)

    def test_partial_acquisition_is_not_promoted(self):
        new_cache = self.root / "new-cache"
        with patch.object(resources.urllib.request, "urlopen", side_effect=OSError("synthetic interrupted download")):
            with self.assertRaises(OSError): resources.acquire(new_cache)
        self.assertFalse(new_cache.exists())
        self.assertEqual(list(self.root.glob(".authority-*")), [])

    def test_source_repository_destination_rejected(self):
        with self.assertRaises(ValueError): resources.acquire(REPO / "resources")


class ArchiveIntegrity(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.stage = self.root / "payload"
        self.stage.mkdir()
        self.name = "ash-pattern-system-windows-sdk-1.0.0-rc.1-x64-release"
        for relative in sdk.public_allowlist(REPO, "x64"):
            file = self.stage / relative
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(b"synthetic payload fixture\n")
        (self.stage / "include/ash/Version.hpp").write_text(
            '#pragma once\n#define ASH_SDK_VERSION "1.0.0-rc.1"\n#define ASH_SOURCE_COMMIT "' + "1" * 40 +
            '"\n#define ASH_SOURCE_DIRTY 0\n#define ASH_DIAGNOSTICS_PROFILE "release"\n' + sdk.version_contract("x64"),
            encoding="utf-8", newline="\n")
        bindings = sdk.authority_bindings(REPO)
        sbom = {"schemaVersion": "1.0", "version": "1.0.0-rc.1", "sourceCommit": "1" * 40,
                "authorityBindings": bindings, "files": sdk.inventory(self.stage)}
        sdk.write_json(self.stage / "SBOM.json", sbom)
        manifest = {"schemaVersion": "1.0", "version": "1.0.0-rc.1", "sourceCommit": "1" * 40, "sourceDirty": False,
                    "platform": "x64", "configuration": "Release", "canonicalCommit": resources.PIN,
                    "canonicalVersion": bindings["canonical"]["version"], "authorityBindings": bindings,
                    "diagnosticsProfile": "Release", "libraryTarget": "ash-core", "libraryArtifact": "ash-core.lib",
                    "diagnosticExportSchemaVersion": "1.0", "settings": sdk.RELEASE_SETTINGS,
                    "files": sdk.inventory(self.stage)}
        sdk.write_json(self.stage / "RELEASE-MANIFEST.json", manifest)
        (self.stage / "checksums.sha256").write_text("".join(f"{item['sha256']}  {item['path']}\n" for item in sdk.inventory(self.stage)), encoding="utf-8")
        self.archive = self.root / "fixture.zip"
        self.rezip()

    def tearDown(self): self.temporary.cleanup()

    def rezip(self, extras=(), omitted=()):
        with zipfile.ZipFile(self.archive, "w") as zipped:
            for file in sorted(self.stage.rglob("*")):
                if file.is_file() and file.relative_to(self.stage).as_posix() not in omitted:
                    zipped.write(file, self.name + "/" + file.relative_to(self.stage).as_posix())
            for name, data in extras: zipped.writestr(name, data)

    def test_complete_internal_integrity_and_fresh_extraction(self):
        result = sdk.extract_archive(self.archive, REPO, self.root / "fresh")
        self.assertEqual(result["status"], "pass")
        self.assertEqual(result["fileCount"], len(sdk.inventory(self.stage)))
        with self.assertRaises(ValueError): sdk.extract_archive(self.archive, REPO, self.root / "fresh")

    def test_tampering(self):
        (self.stage / "LICENSE").write_bytes(b"tampered")
        self.rezip()
        with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_invalid_architecture_identity(self):
        manifest = sdk.read_json(self.stage / "RELEASE-MANIFEST.json")
        manifest["platform"] = "x86"
        sdk.write_json(self.stage / "RELEASE-MANIFEST.json", manifest)
        self.rezip()
        with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_missing_metadata(self):
        self.rezip(omitted=("SBOM.json",))
        with self.assertRaises((ValueError, KeyError)): sdk.inspect_archive(self.archive, REPO)

    def refresh_integrity(self):
        sbom = sdk.read_json(self.stage / "SBOM.json")
        sbom["files"] = [item for item in sdk.inventory(self.stage) if item["path"] not in ("SBOM.json", "RELEASE-MANIFEST.json", "checksums.sha256")]
        sdk.write_json(self.stage / "SBOM.json", sbom)
        manifest = sdk.read_json(self.stage / "RELEASE-MANIFEST.json")
        manifest["files"] = [item for item in sdk.inventory(self.stage) if item["path"] not in ("RELEASE-MANIFEST.json", "checksums.sha256")]
        sdk.write_json(self.stage / "RELEASE-MANIFEST.json", manifest)
        (self.stage / "checksums.sha256").write_text("".join(f"{item['sha256']}  {item['path']}\n" for item in sdk.inventory(self.stage) if item["path"] != "checksums.sha256"), encoding="utf-8")
        self.rezip()

    def test_rehashed_incompatible_release_contract_is_rejected(self):
        original = sdk.read_json(self.stage / "RELEASE-MANIFEST.json")
        for key, value in (("diagnosticsProfile", "Development"), ("schemaVersion", "9.0"),
                           ("libraryTarget", "other"), ("libraryArtifact", "other.lib"),
                           ("diagnosticExportSchemaVersion", "9.0"), ("sourceDirty", 0),
                           ("settings", {**sdk.RELEASE_SETTINGS, "runtimeLibrary": "/MD"})):
            with self.subTest(key=key):
                sdk.write_json(self.stage / "RELEASE-MANIFEST.json", {**original, key: value})
                self.refresh_integrity()
                with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_rehashed_wrong_authority_or_profile_source_is_rejected(self):
        original = sdk.read_json(self.stage / "RELEASE-MANIFEST.json")
        for container, key in (("canonical", "sourceCommit"), ("engineeringPolicy", "sourceCommit"),
                               ("realizationProfiles", "sourceCommit")):
            with self.subTest(container=container):
                changed = copy.deepcopy(original)
                changed["authorityBindings"][container][key] = "0" * 40
                sdk.write_json(self.stage / "RELEASE-MANIFEST.json", changed)
                self.refresh_integrity()
                with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)
        changed = copy.deepcopy(original)
        changed["authorityBindings"]["realizationProfiles"]["profiles"][0]["sha256"] = "0" * 64
        sdk.write_json(self.stage / "RELEASE-MANIFEST.json", changed)
        self.refresh_integrity()
        with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_rehashed_wrong_canonical_version_is_rejected(self):
        manifest = sdk.read_json(self.stage / "RELEASE-MANIFEST.json")
        manifest["canonicalVersion"] = "1.0.0-rc.1"
        sdk.write_json(self.stage / "RELEASE-MANIFEST.json", manifest)
        self.refresh_integrity()
        with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_rehashed_wrong_generated_identity_or_guard_is_rejected(self):
        header = self.stage / "include/ash/Version.hpp"
        original = header.read_text(encoding="utf-8")
        for changed in (original.replace('ASH_SOURCE_DIRTY 0', 'ASH_SOURCE_DIRTY 1'),
                        original.replace('ASH_DIAGNOSTICS_PROFILE "release"', 'ASH_DIAGNOSTICS_PROFILE "development"'),
                        original.replace("_M_X64", "_M_ARM64"),
                        original + '#define ASH_SDK_VERSION "2.0.0-rc.1"\n'):
            header.write_text(changed, encoding="utf-8", newline="\n")
            self.refresh_integrity()
            with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_traversal_and_absolute_and_backslash_and_case_collision(self):
        for name in (self.name + "/../escaped", "/absolute", self.name + "/bad\\path", self.name + "/license"):
            with self.subTest(name=name):
                self.rezip(extras=((name, b"bad"),))
                destination = self.root / "not-extracted"
                with self.assertRaises((ValueError, KeyError)): sdk.extract_archive(self.archive, REPO, destination)
                self.assertFalse(destination.exists())

    def test_duplicate_path(self):
        self.rezip(extras=((self.name + "/LICENSE", b"duplicate"),))
        with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_wrong_architecture_or_version_root(self):
        manifest = sdk.read_json(self.stage / "RELEASE-MANIFEST.json")
        manifest["platform"] = "ARM64"
        sdk.write_json(self.stage / "RELEASE-MANIFEST.json", manifest)
        self.rezip()
        with self.assertRaises(ValueError): sdk.inspect_archive(self.archive, REPO)

    def test_output_overlap_rejected_before_writes(self):
        for output in (REPO, REPO.parent, REPO / "packages"):
            with self.subTest(output=output):
                args = SimpleNamespace(repo_root=REPO, build_root=self.root / "build", authority_cache=self.root / "authority", output=output)
                with self.assertRaises(ValueError): sdk.package(args)


class PackagingPromotion(unittest.TestCase):
    """Staging mechanics with explicit synthetic build-gate fixtures, not qualification."""
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.repo, self.build, self.output = self.root / "source", self.root / "build", self.root / "release"
        declaration = self.repo / "windows/packaging/sdk/sdk-public-files.json"
        declaration.parent.mkdir(parents=True)
        sdk.write_json(declaration, {"files": [{"destination": "LICENSE", "source": "LICENSE", "role": "license"}]})
        sdk.write_json(self.repo / "windows/packaging/sdk-authority-bindings.json", sdk.authority_bindings(REPO))
        (self.repo / "LICENSE").write_text("synthetic license fixture\n", encoding="utf-8")
        library = self.build / "bin/x64-Release/ash-core.lib"
        library.parent.mkdir(parents=True)
        library.write_bytes(b"synthetic archive fixture")
        header = self.build / "generated/x64-Release/include/ash/Version.hpp"
        header.parent.mkdir(parents=True)
        header.write_text('#pragma once\n#define ASH_SDK_VERSION "1.0.0-rc.1"\n#define ASH_SOURCE_COMMIT "' + "1" * 40 + '"\n#define ASH_SOURCE_DIRTY 0\n#define ASH_DIAGNOSTICS_PROFILE "release"\n', encoding="utf-8")
        self.metadata = {"version": "1.0.0-rc.1", "sourceCommit": "1" * 40,
                         "settings": {"runtimeLibrary": "/MT", "languageStandard": "C++20", "wholeProgramOptimization": False, "debugInformationFormat": "/Z7"},
                         "toolchain": {"compiler": "synthetic fixture"}, "library": {"sha256": sdk.digest(library)}}
        gate = self.root / "synthetic-gate.json"
        sdk.write_json(gate, {"dependencyBoundary": {"thirdPartyRuntimeDependencies": []}})
        self.args = SimpleNamespace(repo_root=self.repo, build_root=self.build, output=self.output,
                                    authority_cache=self.root / "authority", gate_report=gate, platform="x64")
        self.patches = [patch.object(sdk, "validate_cache", return_value={"status": "pass"}),
                        patch.object(sdk, "verify_build", return_value=self.metadata)]
        for mocked in self.patches: mocked.start()

    def tearDown(self):
        for mocked in self.patches: mocked.stop()
        self.temporary.cleanup()

    def test_retry_reuses_exact_archive_and_returns_path(self):
        first = sdk.package(self.args)
        second = sdk.package(self.args)
        self.assertEqual(first["archive"], second["archive"])
        self.assertEqual(first["sha256"], second["sha256"])

    def test_missing_input_never_promotes_archive(self):
        (self.repo / "LICENSE").unlink()
        with self.assertRaises(ValueError): sdk.package(self.args)
        self.assertEqual(list(self.output.glob("*.zip")), [])
        self.assertEqual(list(self.output.glob(".sdk-stage-*")), [])

    def test_interrupted_promotion_then_retry(self):
        with patch.object(sdk.os, "rename", side_effect=OSError("synthetic interrupted promotion")):
            with self.assertRaises(OSError): sdk.package(self.args)
        self.assertEqual(list(self.output.glob("*.zip")), [])
        self.assertEqual(list(self.output.glob(".sdk-stage-*")), [])
        self.assertEqual(sdk.package(self.args)["status"], "pass")

    def test_other_release_workspace_not_overwritten(self):
        self.output.mkdir()
        owner = {"version": "2.0.0-rc.1", "sourceCommit": "2" * 40}
        sdk.write_json(self.output / ".sdk-release-workspace.json", owner)
        with self.assertRaises(ValueError): sdk.package(self.args)
        self.assertEqual(sdk.read_json(self.output / ".sdk-release-workspace.json"), owner)

    def test_rebuilt_library_cannot_reuse_stale_archive(self):
        sdk.package(self.args)
        self.metadata["library"]["sha256"] = "2" * 64
        with self.assertRaises(ValueError): sdk.package(self.args)

    def test_changed_curated_input_or_version_projection_cannot_reuse_archive(self):
        sdk.package(self.args)
        license_file = self.repo / "LICENSE"
        original = license_file.read_bytes()
        license_file.write_bytes(b"changed curated file")
        with self.assertRaises(ValueError): sdk.package(self.args)
        license_file.write_bytes(original)
        header = self.build / "generated/x64-Release/include/ash/Version.hpp"
        header.write_text(header.read_text(encoding="utf-8") + "// changed generated projection\n", encoding="utf-8")
        with self.assertRaises(ValueError): sdk.package(self.args)

    def test_public_urls_are_allowed_but_workstation_paths_rejected(self):
        (self.repo / "LICENSE").write_text("https://example.invalid/license\n", encoding="utf-8")
        self.assertEqual(sdk.package(self.args)["status"], "pass")
        self.args.output = self.root / "second-release"
        (self.repo / "LICENSE").write_text("Private path C:\\Users\\fixture\\source\n", encoding="utf-8")
        with self.assertRaises(ValueError): sdk.package(self.args)
        self.assertEqual(list(self.args.output.glob("*.zip")), [])


if __name__ == "__main__": unittest.main()
