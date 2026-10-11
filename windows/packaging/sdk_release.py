"""SDK payload preparation and hostile-archive verification (development tool)."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

from canonical_resources import PIN, safe_relative, validate_cache

RELEASE_SETTINGS = {"runtimeLibrary": "/MT", "languageStandard": "C++20",
                    "wholeProgramOptimization": False, "debugInformationFormat": "/Z7"}
POLICY_PIN = "8dd6e75a06336491b249fac3577db75458db8c13"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_json(path: Path, value) -> None:
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def git(repo: Path, *args: str) -> str:
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def verify_source_snapshot(repo: Path, snapshot: dict) -> str:
    files = snapshot.get("files", [])
    expected = {item["path"]: item["sha256"] for item in files}
    if not files or len(expected) != len(files):
        raise ValueError("Invalid completed-build source snapshot")
    observed = git(repo, "-c", "core.quotePath=false", "ls-files", "--cached", "--others", "--exclude-standard").splitlines()
    if set(observed) != set(expected):
        raise ValueError("Source inventory changed since build")
    for relative, checksum in expected.items():
        safe_relative(relative)
        if digest(repo / relative) != checksum:
            raise ValueError("Source changed since build: " + relative)
    fingerprint = hashlib.sha256("\n".join(item["path"] + "\t" + item["sha256"] for item in files).encode("utf-8")).hexdigest()
    if fingerprint != snapshot.get("fingerprint"):
        raise ValueError("Source snapshot fingerprint mismatch")
    return fingerprint


def contract_profile(profile_root: Path, contract: Path) -> Path:
    ids = read_json(contract)["system"]["realizationProfiles"]
    if len(ids) != 1:
        raise ValueError("This qualification route requires exactly one declared realization profile")
    matches = [path for path in (profile_root / "profiles/system-realization").glob("*.json")
               if read_json(path).get("profileId") == ids[0]]
    if len(matches) != 1:
        raise ValueError("Declared realization profile is missing or ambiguous")
    return matches[0]


def authority_bindings(repo: Path) -> dict:
    bindings = read_json(repo / "windows/packaging/sdk-authority-bindings.json")
    canonical = bindings["canonical"]
    policy = bindings["engineeringPolicy"]
    profiles = bindings["realizationProfiles"]
    if (canonical != {"repository": "flynn33/ASH-Pattern-System", "version": "1.0.0-rc.3", "sourceCommit": PIN}
            or policy != {"repository": "flynn33/raven-forge-development", "version": "0.7.0", "sourceCommit": POLICY_PIN}
            or profiles["repository"] != "flynn33/raven-forge-development" or profiles["version"] != "0.7.2"
            or not re.fullmatch(r"[0-9a-f]{40}", profiles["sourceCommit"])):
        raise ValueError("Invalid or mismatched immutable authority bindings")
    expected_ids = {"producer-canonical": "rfd.aps.canonical.1.0.0-rc.3",
                    "consumer-canonical": "rfd.aps.canonical.1.0.0-rc.3",
                    "consumer-windows": "rfd.aps.windows.b1c05f1"}
    entries = profiles["profiles"]
    if (len(entries) != len(expected_ids) or {item["contractRole"] for item in entries} != set(expected_ids)
            or any(item["profileId"] != expected_ids[item["contractRole"]]
                   or not re.fullmatch(r"[0-9a-f]{64}", item["sha256"]) for item in entries)):
        raise ValueError("Invalid realization profile identity or digest inventory")
    return bindings


def public_allowlist(repo: Path, platform: str) -> dict[str, dict]:
    declared = read_json(repo / "windows/packaging/sdk/sdk-public-files.json")
    result = {}
    for item in declared["files"]:
        destination = safe_relative(item["destination"])
        if destination.casefold() in {path.casefold() for path in result}:
            raise ValueError("Colliding public payload declaration")
        result[destination] = item
    result[f"lib/{platform.lower()}/Release/ash-core.lib"] = {"role": "native-static-library"}
    result["include/ash/Version.hpp"] = {"role": "generated-public-identity"}
    return result


def inventory(root: Path) -> list[dict]:
    return [{"path": path.relative_to(root).as_posix(), "size": path.stat().st_size,
             "sha256": digest(path)} for path in sorted(root.rglob("*")) if path.is_file()]


def validate_paths(names: list[str], expected_root: str) -> None:
    seen = set()
    files = set()
    for name in names:
        safe_relative(name)
        parts = PurePosixPath(name).parts
        if len(parts) < 2 or parts[0] != expected_root:
            raise ValueError("Archive must contain one declared versioned root")
        key = name.casefold()
        if key in seen:
            raise ValueError("Duplicate or case-colliding ZIP entry")
        seen.add(key)
        files.add(key)
    for name in files:
        if any(parent.as_posix().casefold() in files for parent in PurePosixPath(name).parents):
            raise ValueError("ZIP file/directory collision")


def inspect_archive(archive: Path, repo: Path) -> dict:
    with zipfile.ZipFile(archive) as zipped:
        entries = zipped.infolist()
        if not entries or any(entry.is_dir() for entry in entries):
            raise ValueError("SDK ZIP must contain an explicit file inventory")
        root_names = {entry.filename.split("/")[0] for entry in entries}
        if len(root_names) != 1:
            raise ValueError("SDK ZIP has multiple roots")
        root = next(iter(root_names))
        validate_paths([entry.filename for entry in entries], root)
        if any(((entry.external_attr >> 16) & 0o170000) == 0o120000 for entry in entries):
            raise ValueError("Symlink ZIP entry rejected")
        if sum(entry.file_size for entry in entries) > 512 * 1024 * 1024:
            raise ValueError("SDK expanded size limit exceeded")
        manifest = json.loads(zipped.read(f"{root}/RELEASE-MANIFEST.json"))
        if (manifest.get("platform") not in ("x64", "ARM64")
                or not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+-rc\.[1-9][0-9]*", manifest.get("version", ""))
                or not re.fullmatch(r"[0-9a-f]{40}", manifest.get("sourceCommit", ""))):
            raise ValueError("Invalid SDK product/source/architecture identity")
        expected_root = f"ash-pattern-system-windows-sdk-{manifest['version']}-{manifest['platform'].lower()}-release"
        if root != expected_root:
            raise ValueError("Versioned SDK root does not match manifest identity")
        allowed = set(public_allowlist(repo, manifest["platform"])) | {"SBOM.json", "RELEASE-MANIFEST.json", "checksums.sha256"}
        actual = {entry.filename[len(root) + 1:] for entry in entries}
        if actual != allowed:
            raise ValueError(f"SDK file allowlist mismatch: {sorted(actual ^ allowed)}")
        checksums = {}
        for line in zipped.read(f"{root}/checksums.sha256").decode("utf-8").splitlines():
            match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
            if not match:
                raise ValueError("Malformed SDK checksum line")
            checksum, relative = match.groups()
            safe_relative(relative)
            if relative.casefold() in {path.casefold() for path in checksums}:
                raise ValueError("Duplicate checksum path")
            checksums[relative] = checksum
        if set(checksums) != allowed - {"checksums.sha256"}:
            raise ValueError("SDK checksum coverage mismatch")
        contents = {}
        for relative, checksum in checksums.items():
            data = zipped.read(f"{root}/{relative}")
            if hashlib.sha256(data).hexdigest() != checksum:
                raise ValueError(f"SDK content checksum mismatch: {relative}")
            contents[relative] = data
            if not relative.endswith(".lib") and re.search(rb"(?<![A-Za-z0-9])(?:[A-Za-z]:[\\/]|/Users/|/home/)", data):
                raise ValueError(f"Absolute workstation path in public text: {relative}")
        sbom = json.loads(contents["SBOM.json"])
        for metadata, excluded in ((sbom, {"SBOM.json", "RELEASE-MANIFEST.json", "checksums.sha256"}),
                                   (manifest, {"RELEASE-MANIFEST.json", "checksums.sha256"})):
            declared = {item["path"]: item for item in metadata["files"]}
            if len(declared) != len(metadata["files"]):
                raise ValueError("Duplicate metadata file declaration")
            if set(declared) != allowed - excluded:
                raise ValueError("Metadata file coverage mismatch")
            for relative, item in declared.items():
                if hashlib.sha256(contents[relative]).hexdigest() != item["sha256"] or len(contents[relative]) != item["size"]:
                    raise ValueError("Metadata hash/size mismatch")
        if manifest["version"] != sbom["version"] or manifest["sourceCommit"] != sbom["sourceCommit"]:
            raise ValueError("SBOM/manifest identity mismatch")
        bindings = authority_bindings(repo)
        if (manifest.get("authorityBindings") != bindings or sbom.get("authorityBindings") != bindings
                or manifest.get("canonicalVersion") != bindings["canonical"]["version"]):
            raise ValueError("Archive authority/profile provenance mismatch")
        if (manifest.get("schemaVersion") != "1.0" or sbom.get("schemaVersion") != "1.0"
                or manifest.get("configuration") != "Release" or manifest.get("sourceDirty") is not False
                or manifest.get("canonicalCommit") != PIN or manifest.get("diagnosticsProfile") != "Release"
                or manifest.get("libraryTarget") != "ash-core" or manifest.get("libraryArtifact") != "ash-core.lib"
                or manifest.get("diagnosticExportSchemaVersion") != "1.0"
                or any(manifest.get("settings", {}).get(key) != value for key, value in RELEASE_SETTINGS.items())
                or manifest.get("settings", {}).get("wholeProgramOptimization") is not False):
            raise ValueError("Archive is not a clean pinned Release SDK")
        validate_version_projection(contents["include/ash/Version.hpp"].decode("utf-8-sig"), manifest)
        return {"status": "pass", "root": root, "sha256": digest(archive),
                "fileCount": len(actual), "manifest": manifest}


def extract_archive(archive: Path, repo: Path, destination: Path) -> dict:
    result = inspect_archive(archive, repo)
    destination = destination.resolve()
    if destination.exists():
        raise ValueError("Fresh extraction destination already exists")
    destination.mkdir(parents=True)
    try:
        with zipfile.ZipFile(archive) as zipped:
            for entry in zipped.infolist():
                target = destination / entry.filename
                if not target.resolve().is_relative_to(destination):
                    raise ValueError("Extraction escaped its destination")
                target.parent.mkdir(parents=True, exist_ok=True)
                with zipped.open(entry) as reader, target.open("xb") as writer:
                    shutil.copyfileobj(reader, writer)
        result["extractedRoot"] = str(destination / result["root"])
        return result
    except Exception:
        # A failed extraction is intentionally retained for evidence, not reused.
        raise


def verify_build(repo: Path, build: Path, platform: str, gate: Path | None) -> dict:
    metadata = read_json(build / "bin" / f"{platform}-Release" / "build-metadata.json")
    commit = git(repo, "rev-parse", "HEAD")
    if metadata["sourceCommit"] != commit or metadata["sourceDirty"] or git(repo, "status", "--porcelain"):
        raise ValueError("Release packaging requires the exact clean built source commit")
    if metadata["platform"] != platform or metadata["configuration"] != "Release":
        raise ValueError("Build configuration/architecture mismatch")
    version = (repo / "VERSION").read_text(encoding="utf-8-sig").strip()
    if metadata["version"] != version:
        raise ValueError("Build version projection is stale")
    verify_source_snapshot(repo, metadata["sourceSnapshot"])
    library = build / "bin" / f"{platform}-Release" / "ash-core.lib"
    if digest(library) != metadata["library"]["sha256"]:
        raise ValueError("Build library checksum mismatch")
    header = build / "generated" / f"{platform}-Release" / "include/ash/Version.hpp"
    if digest(header) != metadata.get("generatedIdentity", {}).get("sha256"):
        raise ValueError("Generated public identity differs from completed build")
    identity = header.read_text(encoding="utf-8-sig")
    for name, value in (("ASH_SDK_VERSION", version), ("ASH_SOURCE_COMMIT", commit), ("ASH_DIAGNOSTICS_PROFILE", "Release")):
        if not re.search(r'^#define ' + name + ' "' + re.escape(value) + '"$', identity, flags=re.MULTILINE | re.IGNORECASE):
            raise ValueError("Generated identity macro does not match build: " + name)
    if not re.search(r'^#define ASH_SOURCE_DIRTY 0\r?$', identity, flags=re.MULTILINE):
        raise ValueError("Generated SDK identity is not clean")
    if not gate:
        raise ValueError("Mandatory native qualification report is missing")
    report = read_json(gate)
    if report.get("status") != "pass" or report.get("sourceCommit") != commit or report.get("nativeConformance", {}).get("status") != "pass":
        raise ValueError("Mandatory native conformance gate did not pass on this source commit")
    bindings = authority_bindings(repo)
    if (report.get("authorityBindings") != bindings or report.get("canonicalCommit") != PIN
            or report.get("policyCommit") != POLICY_PIN
            or report.get("profileSourceCommit") != bindings["realizationProfiles"]["sourceCommit"]):
        raise ValueError("Qualification authority/profile provenance mismatch")
    settings = metadata["settings"]
    if (settings["runtimeLibrary"] != "/MT" or settings["languageStandard"] != "C++20"
            or settings["wholeProgramOptimization"] or settings["debugInformationFormat"] != "/Z7"):
        raise ValueError("Public static-library build settings mismatch")
    if metadata["diagnosticsProfile"].lower() != "release":
        raise ValueError("Development diagnostic profile cannot be packaged")
    boundary = report.get("dependencyBoundary", {})
    composition = report.get("releaseComposition", {}).get(platform, {})
    if boundary.get("status") != "pass" or not isinstance(boundary.get("thirdPartyRuntimeDependencies"), list):
        raise ValueError("Verified native dependency graph evidence is missing")
    if composition.get("status") != "pass" or composition.get("librarySha256") != metadata["library"]["sha256"]:
        raise ValueError("Release archive composition/architecture inspection did not pass")
    return metadata


def version_contract(platform: str) -> str:
    if platform not in ("x64", "ARM64"):
        raise ValueError("Unsupported SDK architecture")
    machine = "_M_X64" if platform == "x64" else "_M_ARM64"
    return f'''
// Installed Release SDK consumption contract.
#if !defined(_MSC_VER) || _MSC_VER < 1944
#error This SDK requires Microsoft v143 compiler 19.44 or later.
#endif
#if !defined(_MSVC_LANG) || _MSVC_LANG < 202002L
#error This SDK requires C++20 or later.
#endif
#if !defined({machine})
#error SDK architecture does not match the compilation target.
#endif
#if !defined(_MT) || defined(_DLL) || defined(_DEBUG)
#error This SDK requires Release /MT; /MD and Debug are incompatible.
#endif
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
#error This SDK requires release iterator debugging level zero.
#endif
#include <string_view>
#if !defined(_ITERATOR_DEBUG_LEVEL) || _ITERATOR_DEBUG_LEVEL != 0
#error This SDK requires release iterator debugging level zero.
#endif
'''


def project_version_header(source: Path, destination: Path, platform: str) -> None:
    # Reject incompatible explicit settings before the first standard header can
    # report its own error or instantiate an incompatible library configuration.
    destination.write_text(version_contract(platform) + source.read_text(encoding="utf-8-sig"), encoding="utf-8", newline="\n")


def validate_version_projection(identity: str, manifest: dict) -> None:
    for name, value in (("ASH_SDK_VERSION", manifest["version"]), ("ASH_SOURCE_COMMIT", manifest["sourceCommit"]),
                        ("ASH_DIAGNOSTICS_PROFILE", "release")):
        matches = re.findall(r'^#define ' + name + r' "([^"\r\n]+)"\r?$', identity, flags=re.MULTILINE)
        if matches != [value]:
            raise ValueError("Archive generated identity mismatch: " + name)
    if re.findall(r'^#define ASH_SOURCE_DIRTY ([^\r\n]+)\r?$', identity, flags=re.MULTILINE) != ["0"]:
        raise ValueError("Archive generated identity is not clean")
    if not identity.startswith(version_contract(manifest["platform"])):
        raise ValueError("Archive architecture/Release consumption projection mismatch")


def payload_input(repo: Path, build: Path, platform: str, relative: str, item: dict) -> Path:
    if relative.endswith("/ash-core.lib"):
        source = build / "bin" / f"{platform}-Release" / "ash-core.lib"
    elif relative == "include/ash/Version.hpp":
        source = build / "generated" / f"{platform}-Release" / "include/ash/Version.hpp"
    else:
        source = repo / safe_relative(item["source"])
    if source.is_symlink() or not source.is_file():
        raise ValueError(f"Missing or unsafe SDK input: {relative}")
    return source


def package(args) -> dict:
    repo, build, output = args.repo_root.resolve(), args.build_root.resolve(), args.output.resolve()
    authority = args.authority_cache.resolve()
    for protected in (repo, build, authority):
        if output == protected or output.is_relative_to(protected) or protected.is_relative_to(output):
            raise ValueError("Unsafe release output path overlaps protected inputs")
    validate_cache(authority)
    metadata = verify_build(repo, build, args.platform, args.gate_report)
    bindings = authority_bindings(repo)
    version = metadata["version"]
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+-rc\.[1-9][0-9]*", version):
        raise ValueError("SDK candidate version must be an explicit rc version")
    root_name = f"ash-pattern-system-windows-sdk-{version}-{args.platform.lower()}-release"
    output.mkdir(parents=True, exist_ok=True)
    owner_file = output / ".sdk-release-workspace.json"
    owner = {"version": version, "sourceCommit": metadata["sourceCommit"]}
    if owner_file.exists():
        if read_json(owner_file) != owner:
            raise ValueError("Output belongs to another release/source commit")
    elif any(output.iterdir()):
        raise ValueError("Output directory is not an empty release workspace")
    else:
        write_json(owner_file, owner)
    archive = output / (root_name + ".zip")
    if archive.exists():
        result = inspect_archive(archive, repo)
        if result["manifest"]["sourceCommit"] != metadata["sourceCommit"]:
            raise ValueError("Existing SDK archive belongs to different source")
        relative_library = f"lib/{args.platform.lower()}/Release/ash-core.lib"
        archived_library = next(item for item in result["manifest"]["files"] if item["path"] == relative_library)
        if archived_library["sha256"] != metadata["library"]["sha256"]:
            raise ValueError("Existing SDK archive differs from the current qualified library")
        with zipfile.ZipFile(archive) as zipped:
            for relative, item in public_allowlist(repo, args.platform).items():
                source = payload_input(repo, build, args.platform, relative, item)
                expected = ((version_contract(args.platform) + source.read_text(encoding="utf-8-sig")).encode("utf-8")
                            if relative == "include/ash/Version.hpp" else source.read_bytes())
                if zipped.read(result["root"] + "/" + relative) != expected:
                    raise ValueError("Existing SDK archive differs from current curated input: " + relative)
        result["archive"] = str(archive)
        return result
    temporary = Path(tempfile.mkdtemp(prefix=".sdk-stage-", dir=output))
    root = temporary / root_name
    root.mkdir()
    try:
        declared = public_allowlist(repo, args.platform)
        for relative, item in declared.items():
            destination = root / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            source = payload_input(repo, build, args.platform, relative, item)
            shutil.copyfile(source, destination)
        version_header = root / "include/ash/Version.hpp"
        project_version_header(version_header, version_header, args.platform)
        payload = inventory(root)
        sbom = {"schemaVersion": "1.0", "format": "ASH product-owned component inventory",
                "name": "ASH Pattern System Windows SDK", "version": version,
                "sourceCommit": metadata["sourceCommit"], "license": "Apache-2.0", "authorityBindings": bindings,
                "components": [{"name": "ash-core", "kind": "native-static-library"}],
                "directLinkRequirements": ["Microsoft v143 /MT static CRT provided by consumer toolchain", "Windows SDK platform libraries"],
                "thirdPartyRuntimeDependencies": read_json(args.gate_report)["dependencyBoundary"]["thirdPartyRuntimeDependencies"],
                "dependencyEvidence": "Native project item/dependency graph and extracted-consumer link inspection are recorded in release qualification evidence. CRT code inclusion is not inferred from /MT on archive objects.",
                "hashCoverage": "Payload only; excludes this inventory and later manifest/checksums/archive.", "files": payload}
        write_json(root / "SBOM.json", sbom)
        manifest = {"schemaVersion": "1.0", "name": "ASH Pattern System Windows SDK", "version": version,
                    "sourceCommit": metadata["sourceCommit"], "sourceDirty": False,
                    "canonicalVersion": bindings["canonical"]["version"], "canonicalCommit": PIN,
                    "authorityBindings": bindings,
                    "semanticRegistryVersion": "aps-windows-1.0", "diagnosticExportSchemaVersion": "1.0",
                    "platform": args.platform, "configuration": "Release", "libraryTarget": "ash-core",
                    "libraryArtifact": "ash-core.lib", "diagnosticsProfile": "Release", "settings": metadata["settings"],
                    "toolchain": metadata["toolchain"], "signingStatus": "unsigned",
                    "qualification": "Candidate. x64 host execution and ARM64 cross-build/link are separate results in release evidence; owner physical qualification remains pending.",
                    "symbols": "Compiler /Z7 debug information is embedded in library objects. Toolchain debug path metadata is disclosed in qualification evidence.",
                    "hashCoverage": "Final payload plus SBOM; excludes this manifest and future checksums/archive.",
                    "files": inventory(root)}
        write_json(root / "RELEASE-MANIFEST.json", manifest)
        (root / "checksums.sha256").write_text("".join(f"{item['sha256']}  {item['path']}\n" for item in inventory(root)), encoding="utf-8")
        pending = temporary / "pending.zip"
        with zipfile.ZipFile(pending, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zipped:
            for file in sorted(root.rglob("*")):
                if file.is_file():
                    info = zipfile.ZipInfo(f"{root_name}/{file.relative_to(root).as_posix()}", (1980, 1, 1, 0, 0, 0))
                    info.compress_type = zipfile.ZIP_DEFLATED
                    info.external_attr = 0o100644 << 16
                    zipped.writestr(info, file.read_bytes())
        result = inspect_archive(pending, repo)
        os.rename(pending, archive)
        result["archive"] = str(archive)
        return result
    finally:
        if temporary.exists():
            if not temporary.resolve().is_relative_to(output) or not temporary.name.startswith(".sdk-stage-"):
                raise ValueError("Refusing unsafe staging cleanup")
            shutil.rmtree(temporary)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    prepare = commands.add_parser("package")
    prepare.add_argument("--repo-root", type=Path, required=True)
    prepare.add_argument("--build-root", type=Path, required=True)
    prepare.add_argument("--authority-cache", type=Path, required=True)
    prepare.add_argument("--gate-report", type=Path, required=True)
    prepare.add_argument("--platform", choices=["x64", "ARM64"], required=True)
    prepare.add_argument("--output", type=Path, required=True)
    check = commands.add_parser("verify-archive")
    check.add_argument("--repo-root", type=Path, required=True)
    check.add_argument("--archive", type=Path, required=True)
    check.add_argument("--extract-root", type=Path)
    for command in (prepare, check):
        command.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = package(args) if args.command == "package" else (
            extract_archive(args.archive, args.repo_root, args.extract_root) if args.extract_root else inspect_archive(args.archive, args.repo_root))
    except Exception as error:
        result = {"status": "fail", "error": str(error), "command": args.command}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    write_json(args.report, result)
    print(json.dumps({key: value for key, value in result.items() if key != "manifest"}))
    return 0 if result["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
