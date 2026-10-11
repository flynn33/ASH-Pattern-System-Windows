"""Host-local SDK qualification. This command performs no publication."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import uuid

from sdk_release import POLICY_PIN, authority_bindings, contract_profile, digest, git, read_json, verify_source_snapshot, write_json
from canonical_resources import PIN

REVIEW_FIELDS = ("sourceUnderstanding", "instructionAccuracy", "instructionAdherence",
                 "implementationConformance", "dependencyImportProhibition", "governanceConformance")
MATRIX_KEYS = ("x64-Release", "x64-Debug", "ARM64-Release", "ARM64-Debug")
SUBSTANTIVE_GATES = ("policy", "resources", "regressions", "buildMatrix", "directSolution",
                     "nativeExecutions", "pairedExports", "nativeDiagnosticRecords", "nativeConformance", "releaseComposition",
                     "packages", "extractedConsumers", "deliveryValidation", "negativeImports", "finalSource")
PREREQUISITE_COMMANDS = (
    "policy-producer-canonical-contract", "policy-consumer-canonical-contract", "policy-consumer-windows-contract",
    "policy-producer-canonical-preflight", "resource-integrity", "runner-regressions", "packaging-regressions",
    "attribution-regressions", "qualification-regressions", "tests-x64-Release", "tests-x64-Debug",
    "direct-solution-x64-Release", "native-diagnostic-records", "support-paired-exports", "archive-composition")
POST_PACKAGE_COMMANDS = ("package-x64", "extract-x64", "package-ARM64", "extract-ARM64", "extracted-consumers",
    "policy-producer-canonical-delivery", "policy-consumer-canonical-delivery", "policy-consumer-windows-delivery",
    "policy-consumer-canonical-preflight", "policy-consumer-windows-preflight", "negative-imports")


def file_evidence(path):
    path = Path(path)
    if path.is_symlink():
        raise ValueError("Unsafe evidence symlink: " + str(path))
    path = path.resolve()
    if not path.is_file():
        raise ValueError("Missing or unsafe evidence file: " + str(path))
    return {"path": str(path), "sha256": digest(path), "size": path.stat().st_size}


def verify_file_evidence(record, expected=None):
    if not isinstance(record, dict) or not isinstance(record.get("path"), str):
        raise ValueError("Invalid file evidence declaration")
    declared = Path(record["path"])
    if declared.is_symlink(): raise ValueError("Unsafe evidence symlink")
    path = declared.resolve()
    if expected is not None and path != Path(expected).resolve():
        raise ValueError("Evidence path does not match the selected output")
    if file_evidence(path) != record:
        raise ValueError("Evidence bytes changed: " + str(path))
    return path


def verify_successful_commands(report, names):
    for name in names:
        steps = [step for step in report.get("steps", []) if step.get("name") == name]
        if (len(steps) != 1 or steps[0].get("status") != "pass" or steps[0].get("exitCode") != 0
                or steps[0].get("execution") != "executed" or not steps[0].get("command")
                or not steps[0].get("logSha256")
                or digest(Path(steps[0]["log"])) != steps[0]["logSha256"]):
            raise ValueError("Prior actual mandatory command/log did not pass or changed: " + name)


def completed_build(repo, build_root, key, step):
    """Bind real completed outputs; synthetic callers are confined to regression tests."""
    if key not in MATRIX_KEYS:
        raise ValueError("Unsupported completed-build configuration")
    target, config = key.split("-")
    binary_root = build_root / "bin" / key
    metadata_path = binary_root / "build-metadata.json"
    metadata = read_json(metadata_path)
    if (metadata.get("sourceCommit") != git(repo, "rev-parse", "HEAD")
            or metadata.get("sourceDirty") is not False or git(repo, "status", "--porcelain")
            or metadata.get("platform") != target or metadata.get("configuration") != config
            or metadata.get("version") != (repo / "VERSION").read_text(encoding="utf-8-sig").strip()
            or metadata.get("diagnosticsProfile", "").lower() != ("release" if config == "Release" else "development")
            or metadata.get("settings") != {"runtimeLibrary": "/MT" if config == "Release" else "/MTd",
                "languageStandard": "C++20", "wholeProgramOptimization": False, "debugInformationFormat": "/Z7"}):
        raise ValueError("Completed build is not the exact clean selected source/configuration")
    fingerprint = verify_source_snapshot(repo, metadata["sourceSnapshot"])
    library = file_evidence(binary_root / "ash-core.lib")
    if metadata.get("library") != {"name": "ash-core.lib", "sha256": library["sha256"]}:
        raise ValueError("Completed library differs from build metadata")
    relative_header = f"generated/{key}/include/ash/Version.hpp"
    identity = file_evidence(build_root / relative_header)
    if metadata.get("generatedIdentity") != {"relativeName": relative_header, "sha256": identity["sha256"]}:
        raise ValueError("Completed generated identity differs from build metadata")
    required = read_json(repo / "windows/tests/test-inventory.json")["tests"]
    declared = metadata.get("binaries", [])
    if (not required or len(required) != len(set(required)) or len(declared) != len(required)
            or {item["name"] for item in declared} != set(required)):
        raise ValueError("Completed build does not contain the authoritative test inventory")
    binaries = []
    for name in sorted(required):
        if Path(name).name != name or not name.startswith("ash-test-") or not name.endswith(".exe"):
            raise ValueError("Unsafe completed-test name")
        binary = file_evidence(binary_root / name)
        if next(item["sha256"] for item in declared if item["name"] == name) != binary["sha256"]:
            raise ValueError("Completed native test differs from build metadata: " + name)
        binaries.append({"name": name, **binary})
    if (step.get("name") != "build-" + key or step.get("status") != "pass"
            or step.get("exitCode") != 0 or step.get("execution") != "executed"
            or str(repo / "windows/build/build.ps1") not in step.get("command", [])
            or step.get("cwd") != str(repo) or not step.get("logSha256")):
        raise ValueError("Original successful public build command evidence is missing")
    command = step["command"]
    for option, value in (("-Configuration", config), ("-Platform", target), ("-BuildRoot", str(build_root)), ("-Rebuild", None)):
        if command.count(option) != 1 or (value is not None and (command.index(option) + 1 >= len(command) or command[command.index(option) + 1] != value)):
            raise ValueError("Original build command does not match the completed configuration")
    if digest(Path(step["log"])) != step["logSha256"]:
        raise ValueError("Original build command log changed")
    return {"metadata": file_evidence(metadata_path), "library": library, "generatedIdentity": identity,
            "fixtureHeader": file_evidence(build_root / "generated/tests/CanonicalFixtures.hpp"),
            "binaries": binaries, "sourceFingerprint": fingerprint, "buildStep": step}


def verified_prior_qualification(path, repo, build_root, bindings):
    previous = read_json(path)
    if (previous.get("sourceCommit") != git(repo, "rev-parse", "HEAD")
            or previous.get("sourceDirty") is not False or previous.get("finalSourceVerified") is not True
            or previous.get("authorityBindings") != bindings
            or previous.get("canonicalCommit") != bindings["canonical"]["sourceCommit"]
            or previous.get("policyCommit") != bindings["engineeringPolicy"]["sourceCommit"]
            or previous.get("profileSourceCommit") != bindings["realizationProfiles"]["sourceCommit"]
            or previous.get("incompleteMandatoryGates") not in ([], ["independentReview"])
            or any(previous.get("mandatoryGates", {}).get(name) != "pass" for name in SUBSTANTIVE_GATES)
            or previous.get("builds") != {key: True for key in MATRIX_KEYS}
            or set(previous.get("completedBuilds", {})) != set(MATRIX_KEYS)):
        raise ValueError("Prior qualification has failed or incomplete substantive gates/source bindings")
    expected_incomplete = [name for name, status in previous["mandatoryGates"].items() if status != "pass"]
    if (set(previous["mandatoryGates"]) != set(SUBSTANTIVE_GATES) | {"independentReview"}
            or expected_incomplete != previous["incompleteMandatoryGates"]
            or previous.get("status") != ("pass" if not expected_incomplete else "fail")
            or not previous.get("sourceFingerprint")):
        raise ValueError("Prior qualification gate summary is inconsistent")
    prerequisite = read_json(verify_file_evidence(previous["packagePrerequisitesEvidence"]))
    if (prerequisite.get("status") != "pass" or prerequisite.get("sourceCommit") != previous["sourceCommit"]
            or prerequisite.get("authorityBindings") != bindings):
        raise ValueError("Prior package prerequisites did not pass on this authority/source")
    verify_successful_commands(prerequisite, PREREQUISITE_COMMANDS)
    verify_successful_commands(previous, (*PREREQUISITE_COMMANDS, *POST_PACKAGE_COMMANDS))
    for key in MATRIX_KEYS:
        recorded = previous["completedBuilds"][key]
        actual = completed_build(repo, build_root, key, recorded["buildStep"])
        if actual != recorded or actual["sourceFingerprint"] != previous.get("sourceFingerprint"):
            raise ValueError("Prior completed native output/source evidence is stale: " + key)
        if not any(step == recorded["buildStep"] or
                   (step.get("execution") == "reused" and step.get("name") == "build-" + key
                    and step.get("originalBuildStep") == recorded["buildStep"])
                   for step in previous.get("steps", [])):
            raise ValueError("Original build evidence is absent from the prior executed/reused steps")
    if previous.get("directSolution", {}).get("status") != "pass":
        raise ValueError("Prior direct solution proof is missing")
    direct = previous["directSolution"]
    if (direct.get("outputsUnchanged") is not True or direct.get("step", {}).get("execution") != "executed"
            or direct["step"].get("status") != "pass" or direct["step"].get("exitCode") != 0
            or digest(Path(direct["step"]["log"])) != direct["step"].get("logSha256")):
        raise ValueError("Prior direct solution command/output proof is invalid")
    direct_command = direct["step"].get("command", [])
    metadata = read_json(Path(previous["completedBuilds"]["x64-Release"]["metadata"]["path"]))
    required_command = [str(repo / "windows/build/ash-windows.sln"), "/t:Build", "/p:Configuration=Release", "/p:Platform=x64",
                        "/p:ASHBuildRoot=" + str(build_root), "/p:VCToolsVersion=" + metadata["toolchain"]["toolsVersion"],
                        "/p:WindowsTargetPlatformVersion=" + metadata["toolchain"]["windowsSdk"]]
    expected_outputs = {key: previous["completedBuilds"]["x64-Release"][key]
                        for key in ("metadata", "library", "generatedIdentity", "fixtureHeader", "binaries")}
    if (any(direct_command.count(argument) != 1 for argument in required_command)
            or direct["step"].get("cwd") != str(repo) or direct.get("outputs") != expected_outputs
            or direct["step"] not in previous.get("steps", [])):
        raise ValueError("Prior direct solution invocation does not bind the supported entry point/current outputs")
    return previous


def asset_identity(assets):
    result = [{key: asset[key] for key in ("name", "sha256", "size")} for asset in assets]
    if len(result) != 2 or len({asset["name"] for asset in result}) != 2:
        raise ValueError("Exactly two qualified SDK ZIP identities are required")
    return sorted(result, key=lambda item: item["name"])


def native_record_evidence(path, runner_paths, completed, authority_cache):
    report = read_json(path)
    expected_schemas = {name: digest(authority_cache / "schemas/1.0" / (name + ".schema.json"))
                        for name in ("diagnostic-envelope", "containment-record", "safe-halt-record")}
    profiles = report.get("profiles", [])
    if (report.get("status") != "pass" or report.get("canonicalCommit") != PIN
            or report.get("schemaDigests") != expected_schemas or len(profiles) != 2
            or len(runner_paths) != 2):
        raise ValueError("Actual native diagnostic report/schema/authority binding is missing")
    for key, runner, profile in zip(("x64-Release", "x64-Debug"), runner_paths, profiles):
        expected_binaries = {binary["name"]: binary["sha256"] for binary in completed[key]["binaries"]}
        if (profile.get("runnerReportSha256") != digest(runner) or profile.get("binaryDigests") != expected_binaries
                or profile.get("negativeControls") != 9):
            raise ValueError("Native diagnostic evidence is not the exact current executed output: " + key)
    return {"status": "pass", **file_evidence(path)}


def verify_artifact_review(review_path, report, repo, build_root):
    review = read_json(review_path)
    if (not isinstance(review.get("reviewer"), str) or not review["reviewer"].strip()
            or review.get("independent") is not True or review.get("implementationAuthored") is not False
            or review.get("humanApprovalClaimed") is not False or review.get("sourceDirty") is not False
            or any(review.get(key) != report.get(key) for key in
                   ("sourceCommit", "sourceFingerprint", "canonicalCommit", "policyCommit", "profileSourceCommit", "authorityBindings"))):
        raise ValueError("Independent reviewer/source/authority identity is missing or mismatched")
    for field in REVIEW_FIELDS:
        conclusion = review.get("conclusions", {}).get(field, {})
        statuses = ("accepted", "not_applicable") if field == "governanceConformance" else ("accepted",)
        if (conclusion.get("status") not in statuses or not isinstance(conclusion.get("evidence"), str)
                or not conclusion["evidence"].strip() or not isinstance(conclusion.get("evidenceFiles"), list)
                or not conclusion["evidenceFiles"]):
            raise ValueError("Independent conclusion lacks accepted status and concrete evidence: " + field)
        for evidence in conclusion["evidenceFiles"]:
            verify_file_evidence(evidence)
    prior_path = verify_file_evidence(review.get("qualificationEvidence"))
    previous = verified_prior_qualification(prior_path, repo, build_root, report["authorityBindings"])
    if previous["sourceFingerprint"] != report["sourceFingerprint"]:
        raise ValueError("Reviewed prior source fingerprint differs from current source")
    libraries = {target: report["completedBuilds"][target + "-Release"]["library"]["sha256"] for target in ("x64", "ARM64")}
    if (review.get("libraries") != libraries
            or any(previous["completedBuilds"][target + "-Release"]["library"]["sha256"] != checksum
                   for target, checksum in libraries.items())):
        raise ValueError("Independent review does not bind the actual Release libraries")
    expected = asset_identity(report["assets"])
    if review.get("sdkAssets") != expected or asset_identity(previous.get("assets", [])) != expected:
        raise ValueError("Independent review SDK asset set/hash/size differs from qualified bytes")
    for candidate in (report["assets"], previous["assets"]):
        for asset in candidate:
            actual = file_evidence(asset["path"])
            if actual["sha256"] != asset["sha256"] or actual["size"] != asset["size"]:
                raise ValueError("Reviewed SDK ZIP changed")
    return {"status": "pass", "reviewer": review["reviewer"], "humanApprovalClaimed": False,
            "qualificationEvidence": review["qualificationEvidence"], "conclusions": review["conclusions"], **file_evidence(review_path)}


def now():
    return datetime.now(timezone.utc).isoformat()


class Qualification:
    def __init__(self, args):
        self.args = args
        self.repo = Path(__file__).resolve().parents[2]
        self.commit = git(self.repo, "rev-parse", "HEAD")
        self.dirty = bool(git(self.repo, "status", "--porcelain"))
        self.run = args.evidence_root / ("run-" + datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + uuid.uuid4().hex[:8])
        self.run.mkdir(parents=True)
        self.steps = []
        self.report = {"schemaVersion": "1.0", "scope": "host-local-sdk-qualification",
                       "sourceCommit": self.commit, "sourceDirty": self.dirty, "startedAtUtc": now(),
                       "host": {"platform": platform.platform(), "machine": platform.machine(), "python": sys.version},
                       "canonicalCommit": PIN, "policyCommit": POLICY_PIN, "profileSourceCommit": args.profile_commit,
                       "status": "fail", "publicationPerformed": False,
                       "limitations": ["Physical Windows deployment-floor qualification is an owner phase.",
                                       "ARM64 runtime is not executed on this x64 host."], "steps": self.steps}

    def release_metadata(self, metadata):
        if self.report["status"] != "pass":
            raise ValueError("Release metadata requires complete successful qualification")
        version = (self.repo / "VERSION").read_text(encoding="utf-8-sig").strip()
        canonical_version = read_json(self.args.authority_cache / "conformance/1.0/manifest.json")["product_version"]
        profiles = [{"contractRole": name,
                     "profileId": read_json(contract_profile(self.args.profile_root, self.args.contract_root / (name + ".json")))["profileId"],
                     "sha256": digest(contract_profile(self.args.profile_root, self.args.contract_root / (name + ".json")))}
                    for name in ("producer-canonical", "consumer-canonical", "consumer-windows")]
        bindings = {"canonical": {"repository": "flynn33/ASH-Pattern-System", "version": canonical_version, "sourceCommit": PIN},
                    "engineeringPolicy": {"repository": "flynn33/raven-forge-development", "version": "0.7.0", "sourceCommit": POLICY_PIN},
                    "realizationProfiles": {"repository": "flynn33/raven-forge-development",
                        "version": (self.args.profile_root / "VERSION").read_text(encoding="utf-8-sig").strip(),
                        "sourceCommit": self.args.profile_commit, "profiles": profiles}}
        if bindings != authority_bindings(self.repo):
            raise ValueError("Actual qualified authority/profile bindings differ from the source declaration")
        public = {"schemaVersion": "1.0", "product": "ASH Pattern System Windows SDK", "version": version,
                  "sourceCommit": self.commit, "sourceDirty": False, "sourceFingerprint": self.report["sourceFingerprint"],
                  "status": "pass", "signingStatus": "unsigned", "authorityBindings": bindings,
                  "host": self.report["host"], "limitations": self.report["limitations"],
                  "releaseComposition": {key: {"status": value["status"], "librarySha256": value["librarySha256"],
                      "embeddedDebugPathDisclosure": value["embeddedDebugPathDisclosure"]}
                      for key, value in self.report["releaseComposition"].items()},
                  "buildMatrix": [{"platform": value["platform"], "configuration": value["configuration"],
                      "diagnosticsProfile": value["diagnosticsProfile"], "settings": value["settings"],
                      "toolchain": value["toolchain"], "librarySha256": value["library"]["sha256"],
                      "binaries": value["binaries"]} for value in metadata],
                  "nativeConformance": {key: self.report["nativeConformance"][key] for key in
                      ("status", "canonicalCommit", "categoriesRequired", "invariantsRequired", "invariantIds", "binarySha256", "caseCount", "failures", "coverageVerified", "binaryProvenanceVerified", "coverage", "categories")},
                  "nativeDiagnosticRecords": {"status": self.report["nativeDiagnosticRecords"]["status"],
                      "sha256": self.report["nativeDiagnosticRecords"]["sha256"]},
                  "directSolution": {"status": self.report["directSolution"]["status"], "outputsUnchanged": True},
                  "mandatoryGates": self.report["mandatoryGates"],
                  "steps": [{key: step[key] for key in ("name", "status", "exitCode") if key in step} for step in self.steps],
                  "independentReview": {"status": "pass", "sha256": self.report["independentReview"]["sha256"],
                      "conclusions": {key: self.report["independentReview"]["conclusions"][key]["status"] for key in REVIEW_FIELDS}},
                  "sdkAssets": [{key: asset[key] for key in ("name", "sha256", "size")} for asset in self.report["assets"]]}
        target = self.args.output_root
        write_json(target / "qualification.json", public)
        tools = metadata[0]["toolchain"]
        notes = f'''# ASH Pattern System Windows SDK {version}

Unsigned prerelease static SDK for x64 and ARM64. Consume the named SDK ZIPs; automatic source archives are not installed SDKs.

Source commit: `{self.commit}`. Canonical APS {canonical_version}: `{PIN}`. Engineering policy 0.7.0: `{POLICY_PIN}`. Successor realization-profile source {bindings["realizationProfiles"]["version"]}: `{self.args.profile_commit}`. The canonical, policy, and profile-source bindings are separate and immutable.

Native Microsoft v143 builds passed for x64 Release/Debug and ARM64 Release/Debug. x64 native tests and the extracted Release consumer executed on the recorded Windows host. ARM64 was cross-built and linked; ARM64 runtime and physical deployment-floor qualification remain unverified owner phases. Canonical native assertions passed all {self.report["nativeConformance"]["invariantsRequired"]} required invariants and five categories. Independent delivery review accepted the applicable conclusions; its digest and actual gate results are in qualification.json.

Toolchain: compiler {tools["compiler"]}, linker {tools["linker"]}, librarian {tools["librarian"]}, Windows SDK {tools["windowsSdk"]}. Consumer contract: Release C++20, Microsoft v143 compiler 19.44 or later, `/MT`, iterator debugging level zero, matching architecture. `/Z7` debug information is embedded in library objects; qualification records actual archive composition and workstation/toolchain debug-path disclosure. Debug SDK assets are not distributed.

The diagnostic root reference is weak, while leaves retain their owning parent ancestry. Record layouts and semantic diagnostic identities changed; clean rebuild and relink every consumer with one complete SDK. Meaningful notes and structured details participate in identity; occurrence metadata does not. Confirmed containment breaches enter terminal SAFE_HALT with a frozen diagnostic chain. Prevented unauthorized operations remain contained and retain their denial records. The new subject-based axiom API executes the five canonical predicates; compatibility helpers still accept caller-supplied conclusions.

The complete planner records all seven canonical request fields, transformed realms, structural topology, roles, five-axiom diagnosis, warnings, artifact descriptions and registry provenance. Its explicit native identity format is `aps-windows-plan-2`; hashes from the earlier incomplete plan format change deliberately. The compatibility overload constructs a documented depth-zero request with unavailable measurements; emission validates the stored complete plan and returns traceable descriptors without file I/O.

Development diagnostics and the minimal Release backend are separately compiled. Support capture/export is caller-owned and explicit, with paired JSON/Markdown verification, privacy filters, bounds, and failure checks. The SDK adds no runtime third-party framework or authority-repository dependency.

Both ZIPs include curated headers, the architecture-specific ash-core.lib, relocatable MSBuild integration, a standalone sample, license, documentation, component inventory, manifest and internal checksums. SHA256SUMS.txt covers every other release asset. Checksums are integrity evidence, not signatures. No production certification, earlier linker compatibility, ARM64 runtime, or untested OS/hardware coverage is claimed.
'''
        (target / "release-notes.md").write_text(notes, encoding="utf-8", newline="\n")
        assets = self.report["assets"] + [{"name": name, "path": str(target / name), "sha256": digest(target / name), "size": (target / name).stat().st_size}
                                         for name in ("qualification.json", "release-notes.md")]
        index = {"schemaVersion": "1.0", "product": "ASH Pattern System Windows SDK", "version": version,
                 "tag": "v" + version, "sourceCommit": self.commit, "authorityBindings": bindings, "prerelease": True,
                 "signingStatus": "unsigned", "hashCoverage": "SDK ZIPs, release notes and qualification; excludes this index and SHA256SUMS.txt to avoid self-reference.",
                 "assets": [{key: asset[key] for key in ("name", "sha256", "size")} |
                            {"url": "https://github.com/flynn33/ASH-Pattern-System-Windows/releases/download/v" + version + "/" + asset["name"]}
                            for asset in assets]}
        write_json(target / "release-index.json", index)
        index_file = target / "release-index.json"
        assets.append({"name": index_file.name, "path": str(index_file), "sha256": digest(index_file), "size": index_file.stat().st_size})
        sums = target / "SHA256SUMS.txt"
        sums.write_text("".join(asset["sha256"] + "  " + asset["name"] + "\n" for asset in sorted(assets, key=lambda item: item["name"])), encoding="utf-8", newline="\n")
        assets.append({"name": sums.name, "path": str(sums), "sha256": digest(sums), "size": sums.stat().st_size})
        for asset in assets:
            if asset["sha256"] != digest(Path(asset["path"])): raise ValueError("Final release asset hash mismatch")
            if not asset["name"].endswith(".zip") and re.search(rb"(?<![A-Za-z0-9])(?:[A-Za-z]:[\\/]|/Users/|/home/)", Path(asset["path"]).read_bytes()):
                raise ValueError("Workstation path in public release metadata")
        self.report["releaseAssets"] = assets

    def command(self, name, command, timeout=1200):
        command = list(map(str, command))
        log = self.run / (name + ".log")
        step = {"name": name, "command": command, "cwd": str(self.repo), "sourceCommit": self.commit,
                "execution": "executed", "startedAtUtc": now(), "log": str(log), "status": "fail", "exitCode": None}
        print("Running " + name, flush=True)
        try:
            with log.open("w", encoding="utf-8") as output:
                result = subprocess.run(command, cwd=self.repo, stdout=output, stderr=subprocess.STDOUT,
                                        timeout=timeout, check=False,
                                        env={**os.environ, "ASH_SUPPORT_EVIDENCE_ROOT": str(self.run / "support-captures")})
            step["exitCode"] = result.returncode
            step["status"] = "pass" if result.returncode == 0 else "fail"
        except Exception as error:
            step["error"] = str(error)
        step["finishedAtUtc"] = now()
        if log.is_file(): step["logSha256"] = digest(log)
        self.steps.append(step)
        print(name + ": " + step["status"] + " (" + str(log) + ")", flush=True)
        return step["status"] == "pass"

    def ps(self, name, path, *arguments, timeout=1200):
        return self.command(name, [self.args.powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", path, *arguments], timeout)

    def py(self, name, path, *arguments, timeout=1200):
        return self.command(name, [sys.executable, path, *arguments], timeout)

    def policy(self, mode, name, root):
        profile = contract_profile(self.args.profile_root, self.args.contract_root / (name + ".json"))
        return self.py("policy-" + name + "-" + mode,
                       self.args.policy_root / "tools/validate_system_realization.py", mode,
                       "--contract", self.args.contract_root / (name + ".json"), "--repo-root", root,
                       "--profile", profile, "--report", self.run / ("policy-" + name + "-" + mode + ".json"))

    def direct_solution(self, completed):
        before = completed["x64-Release"]
        metadata = read_json(Path(before["metadata"]["path"]))
        vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        discovery = [str(vswhere), "-latest", "-version", "[17.0,18.0)", "-products", "*", "-requires",
                     "Microsoft.Component.MSBuild", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                     "Microsoft.VisualStudio.Component.VC.Tools.ARM64", "-property", "installationPath"]
        found = subprocess.run(discovery, capture_output=True, text=True, check=True, timeout=30).stdout.strip().splitlines()
        if len(found) != 1: raise ValueError("Direct solution requires one supported Visual Studio installation")
        msbuild = Path(found[0]) / "MSBuild/Current/Bin/MSBuild.exe"
        if not msbuild.is_file(): raise ValueError("Discovered MSBuild executable is absent")
        command = [msbuild, self.repo / "windows/build/ash-windows.sln", "/t:Build", "/nologo", "/m", "/verbosity:minimal",
                   "/p:Configuration=Release", "/p:Platform=x64", "/p:ASHBuildRoot=" + str(self.args.build_root),
                   "/p:VCToolsVersion=" + metadata["toolchain"]["toolsVersion"],
                   "/p:WindowsTargetPlatformVersion=" + metadata["toolchain"]["windowsSdk"]]
        passed = self.command("direct-solution-x64-Release", command)
        step = self.steps[-1]
        after = completed_build(self.repo, self.args.build_root, "x64-Release", before["buildStep"])
        unchanged = before == after
        return {"status": "pass" if passed and unchanged else "fail", "discoveryCommand": discovery,
                "installation": found[0], "step": step, "outputsUnchanged": unchanged,
                "outputs": {key: after[key] for key in ("metadata", "library", "generatedIdentity", "fixtureHeader", "binaries")}}

    def execute(self):
        a = self.args
        if git(a.policy_root, "rev-parse", "HEAD") != POLICY_PIN or git(a.policy_root, "status", "--porcelain"):
            raise ValueError("Policy checkout is not the exact clean immutable authority")
        if (not re.fullmatch(r"[0-9a-f]{40}", a.profile_commit)
                or git(a.profile_root, "rev-parse", "HEAD") != a.profile_commit
                or git(a.profile_root, "status", "--porcelain")):
            raise ValueError("Profile source is not the exact clean immutable successor authority")
        actual_profiles = [{"contractRole": name,
                            "profileId": read_json(contract_profile(a.profile_root, a.contract_root / (name + ".json")))["profileId"],
                            "sha256": digest(contract_profile(a.profile_root, a.contract_root / (name + ".json")))}
                           for name in ("producer-canonical", "consumer-canonical", "consumer-windows")]
        bindings = authority_bindings(self.repo)
        if (bindings["realizationProfiles"]["sourceCommit"] != a.profile_commit
                or bindings["realizationProfiles"]["profiles"] != actual_profiles
                or bindings["canonical"]["version"] != read_json(a.authority_cache / "conformance/1.0/manifest.json")["product_version"]):
            raise ValueError("Actual qualification inputs differ from the declared immutable authority bindings")
        self.report["authorityBindings"] = bindings
        prior = None
        if a.completed_build_report:
            prior = verified_prior_qualification(a.completed_build_report, self.repo, a.build_root, bindings)
            self.report["completedBuildReuse"] = {"status": "pass", "qualificationEvidence": file_evidence(a.completed_build_report),
                                                  "buildsReexecuted": False}
        policy_ok = all([self.policy("contract", name, self.repo if name == "producer-canonical" else a.consumer_root)
                         for name in ("producer-canonical", "consumer-canonical", "consumer-windows")])
        policy_ok = self.policy("preflight", "producer-canonical", self.repo) and policy_ok
        resources_ok = self.py("resource-integrity", self.repo / "windows/packaging/run-canonical-conformance.py",
                               "--authority-cache", a.authority_cache, "--report", self.run / "resource-integrity.json")
        regressions_ok = all([
            self.py("runner-regressions", self.repo / "windows/tests/test-build-scripts.py", "--evidence-root", self.run / "runner-regressions"),
            self.py("packaging-regressions", self.repo / "windows/tests/sdk_packaging_regressions.py"),
            self.py("attribution-regressions", self.repo / "windows/tests/attribution_regressions.py"),
            self.py("qualification-regressions", self.repo / "windows/tests/sdk_qualification_regressions.py")])
        builds = {}
        executions = {}
        completed = {}
        for target in ("x64", "ARM64"):
            for config in ("Release", "Debug"):
                key = target + "-" + config
                if prior:
                    builds[key] = True
                    completed[key] = prior["completedBuilds"][key]
                    self.steps.append({"name": "build-" + key, "status": "pass", "execution": "reused",
                                       "qualificationEvidence": self.report["completedBuildReuse"]["qualificationEvidence"],
                                       "originalBuildStep": completed[key]["buildStep"]})
                else:
                    builds[key] = resources_ok and self.ps("build-" + key, self.repo / "windows/build/build.ps1",
                        "-Configuration", config, "-Platform", target, "-BuildRoot", a.build_root,
                        "-AuthorityCache", a.authority_cache, "-Python", sys.executable, "-Rebuild")
                    if builds[key] and not self.dirty:
                        completed[key] = completed_build(self.repo, a.build_root, key, self.steps[-1])
                if target == "x64" and builds[key]:
                    executions[key] = self.ps("tests-" + key, self.repo / "windows/tests/run_all.ps1",
                        "-BinDir", a.build_root / "bin" / key, "-ReportPath", self.run / ("tests-" + key + ".json"))
                elif target == "ARM64":
                    self.steps.append({"name": "tests-" + key, "status": "not_run",
                                       "reason": "ARM64 cross-build on x64; no authorized compatible execution route available."})
        self.report["builds"] = builds
        self.report["executions"] = executions
        self.report["completedBuilds"] = completed
        direct_ok = False
        self.report["directSolution"] = {"status": "fail", "reason": "All four exact clean completed builds are required"}
        if set(completed) == set(MATRIX_KEYS):
            self.report["directSolution"] = self.direct_solution(completed)
            direct_ok = self.report["directSolution"]["status"] == "pass"
        diagnostic_path = self.run / "native-diagnostic-records.json"
        diagnostics_ok = False
        if executions == {"x64-Release": True, "x64-Debug": True}:
            diagnostics_ok = self.py("native-diagnostic-records", self.repo / "windows/tests/verify-native-diagnostic-records.py",
                "--authority-cache", a.authority_cache, "--runner-report", self.run / "tests-x64-Release.json",
                "--runner-report", self.run / "tests-x64-Debug.json", "--report", diagnostic_path)
        self.report["nativeDiagnosticRecords"] = {"status": "pass" if diagnostics_ok else "fail"}
        if diagnostics_ok:
            try:
                self.report["nativeDiagnosticRecords"] = native_record_evidence(diagnostic_path,
                    [self.run / "tests-x64-Release.json", self.run / "tests-x64-Debug.json"], completed, a.authority_cache)
            except (ValueError, KeyError, OSError, TypeError) as error:
                diagnostics_ok = False
                self.report["nativeDiagnosticRecords"] = {"status": "fail", "error": str(error)}
        elif diagnostic_path.is_file():
            self.report["nativeDiagnosticRecords"].update(file_evidence(diagnostic_path))
        exports_ok = self.py("support-paired-exports", self.repo / "windows/tests/verify-support-exports.py",
                             self.run / "support-captures", "--require-profiles", "Development", "Release",
                             "--report", self.run / "support-paired-exports.json")
        invariant_spec = (a.authority_cache / "specs/verification/invariant-spec.md").read_text(encoding="utf-8")
        invariant_ids = re.findall(r"^### (INV-[A-Z]+-[0-9]{3})\b", invariant_spec, flags=re.MULTILINE)
        if not invariant_ids or len(invariant_ids) != len(set(invariant_ids)):
            raise ValueError("Invalid pinned invariant inventory")
        native = {"status": "fail", "canonicalCommit": PIN, "categoriesRequired": 5,
                  "invariantsRequired": len(invariant_ids), "invariantIds": invariant_ids, "cases": []}
        runner = self.run / "tests-x64-Release.json"
        if runner.exists():
            test_report = read_json(runner)
            result = next((r for r in test_report["results"] if r["name"] == "ash-test-canonicalconformance.exe"), {})
            output = result.get("output", "").replace("\r\n", "\n")
            coverage = re.findall(r'^\[COVERAGE\] invariant=(INV-[A-Z]+-[0-9]{3}) category=([1-5]) test=(\S+) status=(passed|failed)$', output, flags=re.MULTILINE)
            categories = re.findall(r'^\[CATEGORY\] category=([1-5]) status=(passed|failed)$', output, flags=re.MULTILINE)
            summary = re.findall(r'^\[COVERAGE\] invariants=([0-9]+) categories=([0-9]+) scope=native_assertions authority_commit=([0-9a-f]{40}) judgment=(\S+)$', output, flags=re.MULTILINE)
            coverage_ok = (len(coverage) == len(invariant_ids) and {row[0] for row in coverage} == set(invariant_ids)
                           and all(row[3] == "passed" for row in coverage)
                           and len(categories) == 5 and {row[0] for row in categories} == set("12345")
                           and all(row[1] == "passed" for row in categories)
                           and summary == [(str(len(invariant_ids)), "5", PIN, "TESTS_PASSED")])
            metadata_path = a.build_root / "bin/x64-Release/build-metadata.json"
            binary_ok = False
            if metadata_path.exists():
                binaries = read_json(metadata_path).get("binaries", [])
                recorded = next((b for b in binaries if b["name"] == result.get("name")), {})
                binary_ok = recorded.get("sha256") == result.get("sha256") and bool(result.get("sha256"))
            native.update({"status": "pass" if result.get("status") == "passed" and coverage_ok and binary_ok else "fail", "runnerReport": str(runner),
                           "binarySha256": result.get("sha256"), "caseCount": result.get("caseCount"),
                           "failures": result.get("caseFailures"),
                           "coverageVerified": coverage_ok, "binaryProvenanceVerified": binary_ok,
                           "coverage": coverage, "categories": categories,
                           "cases": [line for line in output.splitlines() if line.startswith("[VECTOR ")]})
        self.report["nativeConformance"] = native
        inspect_ok = False
        inspection_path = self.run / "archive-composition.json"
        if builds.get("x64-Release") and builds.get("ARM64-Release"):
            inspect_ok = self.py("archive-composition", self.repo / "windows/tests/verify-sdk-consumers.py",
                "--inspect-builds-only", "--exercise-development-consumers", "--repo-root", self.repo, "--build-root", a.build_root,
                "--consumer-root", self.run / "composition-consumer", "--evidence-root", self.run / "archive-composition",
                "--report", inspection_path)
        inspection = read_json(inspection_path) if inspection_path.exists() else {}
        self.report["dependencyBoundary"] = inspection.get("dependencyBoundary", {"status": "fail"})
        self.report["releaseComposition"] = inspection.get("releaseComposition", {})
        prerequisites = (policy_ok and resources_ok and regressions_ok and builds == {key: True for key in MATRIX_KEYS}
                         and executions == {"x64-Release": True, "x64-Debug": True} and direct_ok and diagnostics_ok
                         and exports_ok and inspect_ok and native["status"] == "pass" and not self.dirty)
        prerequisite_report = self.run / "package-prerequisites.json"
        write_json(prerequisite_report, {**self.report, "scope": "package-prerequisites", "status": "pass" if prerequisites else "fail"})
        self.report["packagePrerequisites"] = str(prerequisite_report)
        self.report["packagePrerequisitesEvidence"] = file_evidence(prerequisite_report)
        assets, sdk_roots = [], {}
        if prerequisites:
            for target in ("x64", "ARM64"):
                result_path = self.run / ("package-" + target + ".json")
                if not self.py("package-" + target, self.repo / "windows/packaging/sdk_release.py", "package",
                              "--repo-root", self.repo, "--build-root", a.build_root,
                              "--authority-cache", a.authority_cache, "--gate-report", prerequisite_report,
                              "--platform", target, "--output", a.output_root, "--report", result_path):
                    break
                archive = Path(read_json(result_path)["archive"])
                extract_report = self.run / ("extract-" + target + ".json")
                if not self.py("extract-" + target, self.repo / "windows/packaging/sdk_release.py", "verify-archive",
                              "--repo-root", self.repo, "--archive", archive,
                              "--extract-root", self.run / ("extracted-" + target), "--report", extract_report):
                    break
                sdk_roots[target] = Path(read_json(extract_report)["extractedRoot"])
                assets.append({"name": archive.name, "path": str(archive), "sha256": digest(archive), "size": archive.stat().st_size})
        consumers_ok = False
        if len(sdk_roots) == 2:
            consumers_ok = self.py("extracted-consumers", self.repo / "windows/tests/verify-sdk-consumers.py",
                "--repo-root", self.repo, "--build-root", a.build_root, "--consumer-root", a.consumer_root,
                "--evidence-root", self.run / "consumers", "--sdk-root-x64", sdk_roots["x64"],
                "--sdk-root-arm64", sdk_roots["ARM64"], "--archive-x64", assets[0]["path"],
                "--archive-arm64", assets[1]["path"], "--report", self.run / "consumers.json")
        delivery_ok = all([self.policy("delivery", name, self.repo if name == "producer-canonical" else a.consumer_root)
                           for name in ("producer-canonical", "consumer-canonical", "consumer-windows")])
        imports_ok = False
        if consumers_ok:
            delivery_ok = all([self.policy("preflight", name, a.consumer_root)
                               for name in ("consumer-canonical", "consumer-windows")]) and delivery_ok
            imports_ok = self.py("negative-imports", self.repo / "windows/tests/verify-sdk-imports.py",
                                 "--consumer-root", a.consumer_root, "--policy-root", a.policy_root,
                                 "--profile-root", a.profile_root,
                                 "--contract-root", a.contract_root, "--evidence-root", self.run / "negative-imports",
                                 "--report", self.run / "negative-imports.json")
        review_ok = False
        fingerprint = None
        metadata = []
        source_current = not self.dirty and git(self.repo, "rev-parse", "HEAD") == self.commit and not git(self.repo, "status", "--porcelain")
        if source_current and set(completed) == set(MATRIX_KEYS):
            for key in MATRIX_KEYS:
                value = read_json(a.build_root / "bin" / key / "build-metadata.json")
                metadata.append(value)
                actual = completed_build(self.repo, a.build_root, key, completed[key]["buildStep"])
                observed = actual["sourceFingerprint"]
                source_current = source_current and value.get("sourceCommit") == self.commit and value.get("sourceDirty") is False
                source_current = source_current and actual == completed[key]
                if fingerprint is None: fingerprint = observed
                source_current = source_current and observed == fingerprint
        else:
            source_current = False
        self.report["sourceFingerprint"] = fingerprint
        self.report["finalSourceVerified"] = source_current
        self.report["assets"] = assets
        if a.independent_review_report:
            try:
                if not source_current: raise ValueError("Independent review requires exact current completed outputs")
                self.report["independentReview"] = verify_artifact_review(a.independent_review_report, self.report, self.repo, a.build_root)
                review_ok = True
            except (ValueError, KeyError, OSError, TypeError) as error:
                self.report["independentReview"] = {"status": "fail", "path": str(a.independent_review_report), "error": str(error)}
        else:
            self.report["independentReview"] = {"status": "missing", "required": True}
        gates = {"policy": policy_ok, "resources": resources_ok, "regressions": regressions_ok,
                 "buildMatrix": builds == {key: True for key in MATRIX_KEYS}, "directSolution": direct_ok,
                 "nativeExecutions": executions == {"x64-Release": True, "x64-Debug": True},
                 "pairedExports": exports_ok, "nativeDiagnosticRecords": diagnostics_ok, "nativeConformance": native["status"] == "pass",
                 "releaseComposition": inspect_ok, "packages": prerequisites and len(assets) == 2,
                 "extractedConsumers": consumers_ok, "deliveryValidation": delivery_ok, "negativeImports": imports_ok,
                 "finalSource": source_current, "independentReview": review_ok}
        self.report["mandatoryGates"] = {key: "pass" if value else "fail" for key, value in gates.items()}
        self.report["incompleteMandatoryGates"] = [key for key, value in gates.items() if not value]
        self.report["readyForIndependentReview"] = self.report["incompleteMandatoryGates"] == ["independentReview"]
        passed = all(gates.values())
        self.report["status"] = "pass" if passed else "fail"
        self.report["releaseEligible"] = passed
        if passed: self.release_metadata(metadata)
        self.report["finishedAtUtc"] = now()
        write_json(self.run / "qualification-report.json", self.report)
        print("Qualification " + self.report["status"] + ": " + str(self.run / "qualification-report.json"), flush=True)
        return passed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("authority-cache", "policy-root", "profile-root", "contract-root", "build-root", "consumer-root", "output-root", "evidence-root"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--powershell", required=True)
    parser.add_argument("--profile-commit", required=True)
    parser.add_argument("--independent-review-report", type=Path)
    parser.add_argument("--completed-build-report", type=Path,
                        help="Reuse only completed builds bound to a prior qualification with all substantive gates passed")
    args = parser.parse_args()
    for name in ("independent_review_report", "completed_build_report"):
        if getattr(args, name): setattr(args, name, getattr(args, name).resolve())
    for name in ("authority_cache", "policy_root", "profile_root", "contract_root", "build_root", "consumer_root", "output_root", "evidence_root"):
        setattr(args, name, getattr(args, name).resolve())
    repo = Path(__file__).resolve().parents[2]
    protected = (repo, args.authority_cache, args.policy_root, args.profile_root, args.contract_root)
    outputs = (args.build_root, args.consumer_root, args.output_root, args.evidence_root)
    for output in outputs:
        for source in protected:
            if output == source or output.is_relative_to(source) or source.is_relative_to(output):
                parser.error("Output path overlaps a protected source or authority")
    for index, output in enumerate(outputs):
        if any(output == other or output.is_relative_to(other) or other.is_relative_to(output) for other in outputs[index + 1:]):
            parser.error("Build, consumer, release, and evidence roots must be separate")
    qualification = Qualification(args)
    try:
        return 0 if qualification.execute() else 1
    except Exception as error:
        qualification.report.update({"status": "fail", "releaseEligible": False, "error": str(error), "finishedAtUtc": now()})
        write_json(qualification.run / "qualification-report.json", qualification.report)
        print(str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
