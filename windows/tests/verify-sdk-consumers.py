#!/usr/bin/env python3
"""Inspect actual native archives and exercise isolated installed SDK consumers."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET


NS = "http://schemas.microsoft.com/developer/msbuild/2003"
ET.register_namespace("", NS)
MACHINES = {"x64": 0x8664, "ARM64": 0xAA64}
WINDOWS_DLLS = {"kernel32.dll", "advapi32.dll", "user32.dll", "shell32.dll", "ole32.dll",
                "oleaut32.dll", "ws2_32.dll", "bcrypt.dll", "ntdll.dll", "shlwapi.dll"}
STATIC_LIBS = {"libcmt", "libcpmt", "oldnames", "libucrt", "libvcruntime"}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def now():
    return datetime.now(timezone.utc).isoformat()


def overlaps(first, second):
    return first == second or first.is_relative_to(second) or second.is_relative_to(first)


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def archive_members(path):
    """Read real COFF members, excluding archive symbol/name tables."""
    data = path.read_bytes()
    require(data.startswith(b"!<arch>\n"), "Not a Microsoft COFF archive")
    position, names, members = 8, b"", []
    while position < len(data):
        require(position + 60 <= len(data), "Truncated archive member header")
        header = data[position:position + 60]
        require(header[58:60] == b"`\n", "Invalid archive member terminator")
        size = int(header[48:58].decode("ascii").strip())
        start = position + 60
        require(start + size <= len(data), "Truncated archive member body")
        name = header[:16].decode("ascii").strip()
        body = data[start:start + size]
        if name == "//":
            names = body
        elif name not in ("/", "/SYM64/"):
            if name.startswith("/") and name[1:].isdigit():
                offset = int(name[1:])
                require(offset < len(names), "Invalid archive long-name offset")
                name = names[offset:].split(b"\x00", 1)[0].split(b"/\n", 1)[0].decode("utf-8", errors="replace")
            else:
                name = name.rstrip("/")
            require(len(body) >= 20, "Truncated COFF member")
            big = body[:4] == b"\x00\x00\xff\xff"
            machine = struct.unpack_from("<H", body, 6 if big else 0)[0]
            members.append({"name": name, "machine": f"0x{machine:04x}", "bytes": size,
                            "sha256": hashlib.sha256(body).hexdigest()})
        position = start + size + (size % 2)
    require(position == len(data) and members, "Archive has no complete native member inventory")
    return members


class Verification:
    def __init__(self, args):
        self.args = args
        self.repo, self.build = args.repo_root.resolve(), args.build_root.resolve()
        self.consumer, self.evidence = args.consumer_root.resolve(), args.evidence_root.resolve()
        for output in (self.consumer, self.evidence):
            require(not overlaps(output, self.repo) and not overlaps(output, self.build),
                    "Acceptance outputs must be separate from source and native build inputs")
        require(not overlaps(self.consumer, self.evidence), "Consumer and evidence roots must be separate")
        require(not self.consumer.exists() or not any(self.consumer.iterdir()),
                "Consumer root must be fresh and empty; retain previous acceptance workspaces")
        require(not self.evidence.exists() or not any(self.evidence.iterdir()),
                "Evidence root must be fresh and empty; retain previous failures")
        self.consumer.mkdir(parents=True, exist_ok=True)
        self.evidence.mkdir(parents=True, exist_ok=True)
        self.commit = git(self.repo, "rev-parse", "HEAD")
        self.dirty = bool(git(self.repo, "status", "--porcelain", "--untracked-files=all"))
        self.commands, self.cases = [], []
        self.report = {"schemaVersion": "1.0", "status": "fail", "sourceCommit": self.commit,
                       "sourceDirty": self.dirty, "startedAtUtc": now(),
                       "mode": "development-build-inspection" if args.inspect_builds_only else "extracted-sdk-acceptance",
                       "dependencyBoundary": {"status": "fail", "thirdPartyRuntimeDependencies": []},
                       "releaseComposition": {}, "consumers": {}, "cases": self.cases,
                       "commands": self.commands,
                       "qualification": "Results apply only to the observed library hashes. ARM64 cross-link does not establish ARM64 execution or physical-hardware qualification."}
        if args.exercise_development_consumers:
            self.report["mode"] = "development-consumer-acceptance"
        self.environment = os.environ.copy()
        removed = {}
        for variable in ("INCLUDE", "LIB", "LIBPATH"):
            retained, excluded = [], []
            for entry in self.environment.get(variable, "").split(";"):
                if not entry:
                    continue
                candidate = Path(entry).resolve()
                if candidate.is_relative_to(self.repo) or candidate.is_relative_to(self.build):
                    excluded.append(entry)
                else:
                    retained.append(entry)
            self.environment[variable] = ";".join(retained)
            removed[variable] = excluded
        self.report["consumerEnvironment"] = {"sourceIncludeLibraryEntriesRemoved": removed}
        vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
        install = subprocess.check_output([str(vswhere), "-latest", "-version", "[17.0,18.0)", "-products", "*",
                                          "-requires", "Microsoft.Component.MSBuild", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                          "Microsoft.VisualStudio.Component.VC.Tools.ARM64", "-property", "installationPath"], text=True).strip()
        require(bool(install), "Installed Microsoft v143 x64 and ARM64 toolchain is required")
        self.install = Path(install)
        self.msbuild = self.install / "MSBuild/Current/Bin/MSBuild.exe"
        self.tools_version = (self.install / "VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt").read_text().strip()
        self.toolroot = self.install / "VC/Tools/MSVC" / self.tools_version / "bin/Hostx64/x64"
        self.sdk_version = read_json(self.build / "bin/x64-Release/build-metadata.json")["toolchain"]["windowsSdk"]
        self.report["toolchain"] = {"installation": str(self.install), "toolsVersion": self.tools_version,
                                    "windowsSdk": self.sdk_version, "msbuild": str(self.msbuild)}
        sys.path.insert(0, str(self.repo / "windows/packaging"))
        import sdk_release
        self.sdk_release = sdk_release

    def run(self, label, command, cwd=None, timeout=900):
        log = self.evidence / (label + ".log")
        require(not log.exists(), "Command evidence label collision: " + label)
        started = now()
        try:
            result = subprocess.run([str(x) for x in command], cwd=cwd or self.consumer,
                                    env=self.environment, text=True, encoding="utf-8", errors="replace", capture_output=True, timeout=timeout)
            output, code = result.stdout + result.stderr, result.returncode
        except subprocess.TimeoutExpired as error:
            output, code = str(error), None
        log.write_text(output, encoding="utf-8")
        record = {"label": label, "command": [str(x) for x in command], "workingDirectory": str(cwd or self.consumer),
                  "sourceCommit": self.commit, "sourceDirty": self.dirty, "startedAtUtc": started,
                  "finishedAtUtc": now(), "exitCode": code, "log": str(log), "logSha256": sha(log)}
        self.commands.append(record)
        return code, output

    def native(self, label, project, platform, configuration="Release", extra=()):
        return self.run(label, [self.msbuild, project, "/t:Rebuild", f"/p:Platform={platform}",
                               f"/p:Configuration={configuration}", f"/p:VCToolsVersion={self.tools_version}",
                               f"/p:WindowsTargetPlatformVersion={self.sdk_version}", *extra, "/nologo", "/verbosity:diagnostic"])

    def evaluate(self, label, project, platform, sdk=None):
        command = [self.msbuild, project, f"/p:Platform={platform}", "/p:Configuration=Release",
                   f"/p:VCToolsVersion={self.tools_version}", f"/p:WindowsTargetPlatformVersion={self.sdk_version}",
                   "-nologo", "-getProperty:PlatformToolset,OutDir,IntDir",
                   "-getItem:ClCompile,ProjectReference"]
        if sdk:
            command.append(f"/p:ASHSDKRoot={sdk}\\")
        else:
            command.append(f"/p:ASHBuildRoot={self.build}")
        code, output = self.run(label, command)
        require(code == 0, "MSBuild item/dependency evaluation failed: " + label)
        parsed = json.loads(output)
        write_json(self.evidence / (label + ".json"), parsed)
        return parsed

    def inspect(self, platform):
        metadata = read_json(self.build / "bin" / f"{platform}-Release/build-metadata.json")
        library = self.build / "bin" / f"{platform}-Release/ash-core.lib"
        require(metadata["sourceCommit"] == self.commit and metadata["platform"] == platform,
                "Build metadata source or target differs from inspected checkout")
        require(sha(library) == metadata["library"]["sha256"], "Archive differs from completed build hash")
        require(metadata["diagnosticsProfile"] == "release", "Non-release diagnostics cannot be distributed")
        identity = metadata.get("generatedIdentity", {})
        relative_identity = identity.get("relativeName", "")
        require(relative_identity == f"generated/{platform}-Release/include/ash/Version.hpp",
                "Generated native identity provenance is missing or does not match the target")
        require(sha(self.build / relative_identity) == identity.get("sha256"), "Generated identity bytes differ from completed native build")
        recorded = {item["path"]: item["sha256"] for item in metadata["sourceSnapshot"]["files"]}
        native_paths = {path.relative_to(self.repo).as_posix() for path in (self.repo / "windows/src").rglob("*") if path.is_file()}
        native_paths |= {path.relative_to(self.repo).as_posix() for path in (self.repo / "windows/build").glob("*") if path.is_file()}
        native_paths.add("VERSION")
        for relative in native_paths:
            require(relative in recorded and sha(self.repo / relative) == recorded[relative],
                    "Native source/project/version changed since completed build: " + relative)
        require(metadata["toolchain"]["toolsVersion"] == self.tools_version and metadata["toolchain"]["windowsSdk"] == self.sdk_version,
                "Acceptance toolchain differs from actual native build")
        require(metadata["settings"] == {"runtimeLibrary": "/MT", "languageStandard": "C++20",
                                         "wholeProgramOptimization": False, "debugInformationFormat": "/Z7"},
                "Completed-build settings violate native static-library contract")
        evaluated = self.evaluate("core-graph-" + platform, self.repo / "windows/build/ash-core.vcxproj", platform)
        require(not evaluated["Items"]["ProjectReference"], "Public library has an unexpected native project dependency")
        sources = evaluated["Items"]["ClCompile"]
        require(sources and all(Path(item["FullPath"]).is_relative_to(self.repo / "windows/src") for item in sources),
                "Public library compiles a source outside its declared native product boundary")
        require(not any("SupportDevelopmentBackend" in item["Identity"] for item in sources), "Development source evaluated into Release")
        members = archive_members(library)
        require(all(item["machine"] == f"0x{MACHINES[platform]:04x}" for item in members), "Wrong architecture COFF member")
        member_names = [item["name"].replace("\\", "/").rsplit("/", 1)[-1] for item in members]
        expected = {Path(item["Identity"]).stem + ".obj" for item in sources}
        require(len(member_names) == len(expected) and set(member_names) == expected, "Archive membership differs from actual native compile items")
        require("SupportReleaseBackend.obj" in member_names and "SupportDevelopmentBackend.obj" not in member_names,
                "Release archive backend composition is invalid")
        for option in ("/LIST",):
            code, _ = self.run("archive-members-" + platform, [self.toolroot / "lib.exe", option, library])
            require(code == 0, "Microsoft librarian could not inspect archive")
        code, symbols = self.run("archive-symbols-" + platform, [self.toolroot / "dumpbin.exe", "/SYMBOLS", library])
        require(code == 0 and "ReleaseBackend" in symbols and "DevelopmentBackend" not in symbols,
                "Release backend symbols missing or development symbols retained")
        code, directives = self.run("archive-directives-" + platform, [self.toolroot / "dumpbin.exe", "/DIRECTIVES", library])
        require(code == 0, "Archive default-library directive inspection failed")
        default_libs = sorted(set(value.strip('"').lower().removesuffix(".lib") for value in re.findall(r"/DEFAULTLIB:(\"[^\"]+\"|[^\s]+)", directives, re.I)))
        require(bool(default_libs) and set(default_libs) <= STATIC_LIBS, "Unexpected archive link dependency: " + repr(default_libs))
        data = library.read_bytes()
        debug_paths = bool(re.search(rb"[A-Za-z]:\\[^\x00\r\n]{3,}", data))
        result = {"status": "pass", "librarySha256": sha(library), "machine": f"0x{MACHINES[platform]:04x}",
                  "memberCount": len(members), "members": members, "compiledBackend": "ReleaseBackend",
                  "developmentBackendAbsent": True, "defaultLibraries": default_libs,
                  "embeddedDebugPathDisclosure": {"containsAbsolutePaths": debug_paths,
                                                   "explanation": "Compiler /Z7 debug records may retain build paths; raw inspection logs are private qualification evidence."}}
        self.report["releaseComposition"][platform] = result
        return library

    def payload(self, platform):
        source = self.args.sdk_root_x64 if platform == "x64" else self.args.sdk_root_arm64
        workspace = self.consumer if platform == "x64" else self.consumer / "arm64"
        destination = workspace / "sdk"
        if self.args.inspect_builds_only:
            destination.mkdir(parents=True)
            for relative, item in self.sdk_release.public_allowlist(self.repo, platform).items():
                target = destination / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                if relative.endswith("/ash-core.lib"):
                    origin = self.build / "bin" / f"{platform}-Release/ash-core.lib"
                elif relative == "include/ash/Version.hpp":
                    origin = self.build / "generated" / f"{platform}-Release/include/ash/Version.hpp"
                else:
                    origin = self.repo / item["source"]
                require(origin.is_file() and not origin.is_symlink(), "Missing development acceptance input: " + relative)
                shutil.copyfile(origin, target)
            self.sdk_release.project_version_header(destination / "include/ash/Version.hpp", destination / "include/ash/Version.hpp", platform)
        else:
            require(source is not None and source.is_dir(), "Both extracted SDK roots are required")
            source = source.resolve()
            require(not overlaps(source, self.consumer) and not overlaps(source, self.evidence), "SDK input overlaps acceptance outputs")
            manifest = read_json(source / "RELEASE-MANIFEST.json")
            require(manifest["sourceCommit"] == self.commit and manifest["platform"] == platform,
                    "Extracted SDK identity differs from current source/target")
            declared = {item["path"]: item for item in manifest["files"]}
            for relative, item in declared.items():
                self.sdk_release.safe_relative(relative)
                require(sha(source / relative) == item["sha256"], "Extracted SDK content differs from manifest: " + relative)
            expected = set(self.sdk_release.public_allowlist(self.repo, platform)) | {"SBOM.json", "RELEASE-MANIFEST.json", "checksums.sha256"}
            require({path.relative_to(source).as_posix() for path in source.rglob("*") if path.is_file()} == expected,
                    "Extracted SDK has an unexpected payload file")
            require(not any(path.is_symlink() for path in source.rglob("*")), "Extracted SDK symlink rejected")
            shutil.copytree(source, destination)
        require(sha(destination / f"lib/{platform.lower()}/Release/ash-core.lib") == self.report["releaseComposition"][platform]["librarySha256"],
                "Installed consumer library differs from inspected build")
        return destination

    def consumer_project(self, platform, sdk):
        app = sdk.parent / "app"
        shutil.copytree(sdk / "samples/StandaloneConsumer", app)
        project = app / "StandaloneConsumer.vcxproj"
        tree = ET.parse(project)
        tree.getroot().find(f"{{{NS}}}PropertyGroup/{{{NS}}}ASHSDKRoot").text = "$(MSBuildThisFileDirectory)..\\sdk\\"
        tree.write(project, encoding="utf-8", xml_declaration=True)
        boundary = self.repo / "windows/tests/VerifyConsumerBoundary.ps1"
        require(boundary.is_file(), "Required product-owned consumer boundary helper is missing")
        verify = sdk.parent / "verify"
        verify.mkdir(exist_ok=True)
        shutil.copyfile(boundary, verify / boundary.name)
        return app, project

    def boundary(self, platform, sdk):
        helper = sdk.parent / "verify/VerifyConsumerBoundary.ps1"
        report = self.evidence / ("static-consumer-boundary-" + platform + ".json")
        code, _ = self.run("static-consumer-boundary-" + platform, ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass",
                          "-File", helper, "-ConsumerRoot", sdk.parent, "-SDKRoot", sdk, "-ReportPath", report])
        require(code == 0 and read_json(report)["status"] == "pass", "Static consumer import boundary failed: " + platform)

    def execute(self, label, app, platform):
        host = os.environ.get("PROCESSOR_ARCHITECTURE", "unknown").upper()
        if platform == "ARM64" and host != "ARM64":
            return {"status": "not_runtime_qualified", "reason": "ARM64 cross-built and linked on " + host + " host; no authorized executable ARM64 route established"}
        route = "emulated" if platform == "x64" and host == "ARM64" else "native"
        executable = app / "bin" / platform / "StandaloneConsumer.exe"
        exports = self.evidence / (label + "-exports")
        code, output = self.run(label, [executable, exports], app, timeout=120)
        require(code == 0, "Standalone consumer failed on compatible host: " + label)
        markers = list(exports.rglob("COMPLETED.json"))
        require(bool(markers), "Standalone consumer did not export a completed support bundle")
        sys.path.insert(0, str(self.repo / "windows/tests"))
        from importlib.util import module_from_spec, spec_from_file_location
        spec = spec_from_file_location("support_export_verifier", self.repo / "windows/tests/verify-support-exports.py")
        verifier = module_from_spec(spec)
        spec.loader.exec_module(verifier)
        pairs = [verifier.verify(marker) for marker in markers]
        require(all(profile == "Release" for profile, _ in pairs), "Installed SDK retained development profile")
        return {"status": "pass", "route": route, "hostArchitecture": host, "executableSha256": sha(executable),
                "completedBundles": len(markers), "output": output}

    def positive(self, platform, sdk):
        app, project = self.consumer_project(platform, sdk)
        for path in [*app.rglob("*.vcxproj"), *app.rglob("*.cpp"), *app.rglob("*.hpp"), *sdk.joinpath("msbuild").glob("*")]:
            text = path.read_text(encoding="utf-8-sig")
            require(not re.search(r"(?<![A-Za-z0-9])[A-Za-z]:[\\/]|git\s+(?:clone|fetch)|Invoke-WebRequest|https?://[^\s]+(?:\.git|\.zip)", text, re.I),
                    "Source acquisition or absolute source path in consumer integration: " + str(path))
        evaluation = self.evaluate("consumer-graph-" + platform, project, platform, sdk)
        items = evaluation["Items"]["ClCompile"]
        require(not evaluation["Items"]["ProjectReference"] and len(items) == 2,
                "Standalone consumer acquired a project or changed its own source inventory")
        require(all(Path(item["FullPath"]).is_relative_to(app / "src") for item in items),
                "Consumer compiled source outside its own application")
        for item in items:
            includes = [part for part in item["AdditionalIncludeDirectories"].split(";") if part]
            require(all(Path(part).resolve() == sdk / "include" for part in includes), "Consumer needs an include outside installed SDK")
            require(item["RuntimeLibrary"] == "MultiThreaded" and item["LanguageStandard"] == "stdcpp20",
                    "Consumer compile contract is not /MT C++20")
        code, output = self.native("consumer-build-" + platform, project, platform, extra=(f"/p:ASHSDKRoot={sdk}\\",))
        require(code == 0, "Installed consumer failed build/link for " + platform)
        require(str(sdk / f"lib/{platform.lower()}/Release/ash-core.lib").lower() in output.lower(),
                "Verbose native link command did not use installed SDK library")
        executable = app / "bin" / platform / "StandaloneConsumer.exe"
        image = executable.read_bytes()
        pe = struct.unpack_from("<I", image, 0x3C)[0]
        require(image[pe:pe + 4] == b"PE\0\0" and struct.unpack_from("<H", image, pe + 4)[0] == MACHINES[platform],
                "Consumer executable machine differs from selected SDK")
        code, dependencies = self.run("consumer-dependencies-" + platform, [self.toolroot / "dumpbin.exe", "/DEPENDENTS", executable])
        require(code == 0, "Linked consumer runtime dependency inspection failed")
        dlls = sorted(set(re.findall(r"^\s+([A-Za-z0-9_.-]+\.dll)\s*$", dependencies, re.M | re.I)), key=str.lower)
        require(dlls and all(name.lower() in WINDOWS_DLLS or name.lower().startswith(("api-ms-win-", "ext-ms-win-")) for name in dlls),
                "Consumer linked a third-party or dynamic CRT runtime: " + repr(dlls))
        execution = self.execute("consumer-run-" + platform, app, platform)
        result = {"status": "pass", "librarySha256": self.report["releaseComposition"][platform]["librarySha256"],
                  "link": "pass", "machine": f"0x{MACHINES[platform]:04x}", "runtimeDependencies": dlls,
                  "execution": execution, "project": str(project), "sdkRoot": str(sdk)}
        self.report["consumers"][platform] = result
        return app, project

    def probe_project(self, root, sdk, platform="x64", configuration="Release", runtime="MultiThreaded", language="stdcpp20", definitions="NDEBUG", item_override="", targets=True):
        root.mkdir(parents=True)
        (root / "main.cpp").write_text("int main() { return 0; }\n", encoding="utf-8")
        (root / "other.cpp").write_text("int other() { return 1; }\n", encoding="utf-8")
        project = root / "Probe.vcxproj"
        escape = lambda value: str(value).replace("&", "&amp;").replace('"', "&quot;").replace("<", "&lt;")
        project.write_text(f'''<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="{NS}">
 <ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="{configuration}|{platform}"><Configuration>{configuration}</Configuration><Platform>{platform}</Platform></ProjectConfiguration></ItemGroup>
 <PropertyGroup Label="Globals"><ProjectGuid>{{74D1132B-6592-48DC-B216-F84B8A12A510}}</ProjectGuid></PropertyGroup>
 <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
 <PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><PlatformToolset>v143</PlatformToolset><UseDebugLibraries>false</UseDebugLibraries></PropertyGroup>
 <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
 <PropertyGroup><ASHSDKRoot>{escape(sdk)}\\</ASHSDKRoot><OutDir>$(MSBuildProjectDirectory)\\bin\\</OutDir><IntDir>$(MSBuildProjectDirectory)\\obj\\</IntDir></PropertyGroup>
 <ItemDefinitionGroup><ClCompile><RuntimeLibrary>{runtime}</RuntimeLibrary><LanguageStandard>{language}</LanguageStandard><WholeProgramOptimization>false</WholeProgramOptimization><DebugInformationFormat>OldStyle</DebugInformationFormat><PreprocessorDefinitions>{definitions};%(PreprocessorDefinitions)</PreprocessorDefinitions><ExceptionHandling>Sync</ExceptionHandling></ClCompile><Link><SubSystem>Console</SubSystem></Link></ItemDefinitionGroup>
 <Import Project="$(ASHSDKRoot)msbuild\\ASHPatternSystem.props" />
 <ItemGroup><ClCompile Include="main.cpp" /><ClCompile Include="other.cpp">{item_override}</ClCompile></ItemGroup>
 <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />
 {('<Import Project="$(ASHSDKRoot)msbuild\\ASHPatternSystem.targets" />' if targets else '')}
</Project>''', encoding="utf-8")
        return project

    def negative(self, name, sdk, platform="x64", configuration="Release", expected="ASH SDK", sample=False, **settings):
        root = self.evidence / "workspaces/negative" / name
        if sample:
            shutil.copytree(sdk / "samples/StandaloneConsumer", root)
            project = root / "StandaloneConsumer.vcxproj"
        else:
            project = self.probe_project(root, sdk, platform, configuration, **settings)
        code, output = self.native("negative-" + name, project, platform, configuration,
                                   extra=(f"/p:ASHSDKRoot={sdk}\\",))
        needles = (expected,) if isinstance(expected, str) else expected
        passed = code not in (0, None) and any(needle.lower() in output.lower() for needle in needles)
        self.cases.append({"case": name, "status": "pass" if passed else "fail", "expected": "Build rejects incompatible consumer", "exitCode": code,
                           "rejectionEvidence": expected})
        require(passed, "Negative consumer did not reject with expected evidence: " + name)

    def headers(self, platform, sdk):
        root = self.evidence / "workspaces/headers" / platform
        root.mkdir(parents=True)
        names = sorted(path.name for path in (sdk / "include/ash").glob("*.hpp"))
        require(len(names) == 15 and "Version.hpp" in names, "Public header inventory differs from declared 14 headers plus identity")
        project = self.probe_project(root / "project", sdk, platform)
        tree = ET.parse(project)
        xml = tree.getroot()
        for itemgroup in list(xml.findall(f"{{{NS}}}ItemGroup")):
            if itemgroup.find(f"{{{NS}}}ClCompile") is not None:
                xml.remove(itemgroup)
        for element in xml.findall(f"{{{NS}}}Import"):
            if "ASHPatternSystem" in element.attrib["Project"]:
                xml.remove(element)
        for element in xml.findall(f"{{{NS}}}PropertyGroup/{{{NS}}}ConfigurationType"):
            element.text = "StaticLibrary"
        definitions = xml.find(f"{{{NS}}}ItemDefinitionGroup/{{{NS}}}ClCompile")
        ET.SubElement(definitions, f"{{{NS}}}AdditionalIncludeDirectories").text = str(sdk / "include")
        group = ET.SubElement(xml, f"{{{NS}}}ItemGroup")
        for index, name in enumerate(names):
            file = project.parent / (name + ".cpp")
            file.write_text(f"#include <ash/{name}>\nint ash_header_{index}() {{ return {index}; }}\n", encoding="utf-8")
            ET.SubElement(group, f"{{{NS}}}ClCompile", Include=file.name)
        tree.write(project, encoding="utf-8", xml_declaration=True)
        code, _ = self.native("isolated-headers-" + platform, project, platform)
        require(code == 0, "Public header isolated compilation failed for " + platform)
        self.report["consumers"][platform]["isolatedHeaders"] = {"status": "pass", "headers": names,
                                                                  "forcedIncludes": [], "count": len(names)}

    def negative_matrix(self, sdks):
        sdk = sdks["x64"]
        definitions = "NDEBUG;_ITERATOR_DEBUG_LEVEL=2"
        for name, settings in (
            ("debug", {"configuration": "Debug", "runtime": "MultiThreadedDebug"}),
            ("dynamic-crt", {"runtime": "MultiThreadedDLL"}),
            ("debug-crt", {"runtime": "MultiThreadedDebug"}),
            ("iterator-mismatch", {"definitions": definitions, "expected": "iterator debugging level zero"}),
            ("cpp17", {"language": "stdcpp17"}),
            ("per-item-crt", {"item_override": "<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>"}),
            ("per-item-language", {"item_override": "<LanguageStandard>stdcpp17</LanguageStandard>"}),
            ("per-item-gl", {"item_override": "<WholeProgramOptimization>true</WholeProgramOptimization>"}),
            ("unsupported-platform", {"platform": "Win32"}),
            ("forced-header-crt", {"runtime": "MultiThreadedDLL", "targets": False, "expected": "Release /MT"}),
            ("forced-header-language", {"language": "stdcpp17", "targets": False, "expected": "C++20 or later"}),
            ("forced-header-iterator", {"definitions": definitions, "targets": False, "expected": "iterator debugging level zero"}),
        ):
            self.negative(name, sdk, **settings)
        for name, relative in (("missing-include", "include/ash/Version.hpp"), ("missing-library", "lib/x64/Release/ash-core.lib")):
            copy = self.evidence / "workspaces/negative-sdks" / name
            shutil.copytree(sdk, copy)
            (copy / relative).unlink()
            self.negative(name, copy)
        wrong = self.evidence / "workspaces/negative-sdks/wrong-architecture"
        shutil.copytree(sdk, wrong)
        shutil.copyfile(sdks["ARM64"] / "lib/arm64/Release/ash-core.lib", wrong / "lib/x64/Release/ash-core.lib")
        self.negative("wrong-architecture-library", wrong, sample=True, expected=("LNK1112", "LNK4272"))
        self.negative("forced-header-architecture", sdks["ARM64"], targets=False, expected="architecture does not match")
        project = self.probe_project(self.evidence / "workspaces/negative/unsupported-toolset", sdk)
        tree = ET.parse(project)
        tree.getroot().find(f"{{{NS}}}PropertyGroup[@Label='Configuration']/{{{NS}}}PlatformToolset").text = "v142"
        tree.write(project, encoding="utf-8", xml_declaration=True)
        # Evaluate the guard directly; no absent v142 installation is mistaken for this assertion.
        code, output = self.run("negative-unsupported-toolset", [self.msbuild, project, "/t:ValidateASHSDK", "/p:Configuration=Release", "/p:Platform=x64", "/nologo"])
        passed = code not in (0, None) and "requires the verified Microsoft v143 toolset" in output
        self.cases.append({"case": "unsupported-toolset", "status": "pass" if passed else "fail", "exitCode": code})
        require(passed, "SDK guard did not reject unsupported toolset")

    def lifecycle(self, platform, sdk):
        archive = self.args.archive_x64 if platform == "x64" else self.args.archive_arm64
        development = self.args.inspect_builds_only
        require(development or (archive is not None and archive.is_file()), "Actual SDK archives are required for repair re-extraction")
        root = self.evidence / "workspaces/lifecycle" / platform
        replacement = root / "replacement SDK with spaces"
        shutil.copytree(sdk, replacement)
        app = root / "application with spaces"
        shutil.copytree(sdk / "samples/StandaloneConsumer", app)
        project = app / "StandaloneConsumer.vcxproj"
        code, _ = self.native("replacement-build-" + platform, project, platform, extra=(f"/p:ASHSDKRoot={replacement}\\",))
        require(code == 0, "Side-by-side replacement SDK relink failed")
        self.execute("replacement-run-" + platform, app, platform)
        (replacement / "include/ash/Version.hpp").unlink()
        code, output = self.native("repair-missing-header-" + platform, project, platform, extra=(f"/p:ASHSDKRoot={replacement}\\",))
        require(code not in (0, None) and "public include directory is missing" in output, "Damaged installation did not fail before repair")
        if not development:
            extract = root / "re-extracted"
            result = self.sdk_release.extract_archive(archive, self.repo, extract)
            repaired = Path(result["extractedRoot"])
            code, _ = self.native("repair-build-" + platform, project, platform, extra=(f"/p:ASHSDKRoot={repaired}\\",))
            require(code == 0, "Re-extracted SDK did not repair consumer build/link")
            self.execute("repair-run-" + platform, app, platform)
        sentinel = root / "unrelated-owner-file.txt"
        sentinel.write_text("preserve this unrelated file\n", encoding="utf-8")
        sentinel_hash = sha(sentinel)
        # Remove SDK imports from this disposable application before deleting its owned SDK copy.
        tree = ET.parse(project)
        for element in list(tree.getroot().findall(f"{{{NS}}}Import")):
            if "ASHPatternSystem" in element.attrib["Project"]:
                tree.getroot().remove(element)
        for group in tree.getroot().findall(f"{{{NS}}}PropertyGroup"):
            for element in list(group):
                if element.tag == f"{{{NS}}}ASHSDKRoot":
                    group.remove(element)
        tree.write(project, encoding="utf-8", xml_declaration=True)
        require(replacement.resolve().is_relative_to(root.resolve()) and replacement.name == "replacement SDK with spaces", "Unsafe SDK removal target")
        shutil.rmtree(replacement)
        require(not replacement.exists() and sha(sentinel) == sentinel_hash and project.is_file(), "SDK removal affected unrelated files")
        self.report["consumers"][platform]["lifecycle"] = {"status": "development_pass_archive_repair_pending" if development else "pass",
                                                         "replacement": "Same observed candidate bytes installed side by side and explicitly relinked",
                                                         "repairArchiveSha256": None if development else sha(archive),
                                                         "repair": "unexecuted: requires an eligible actual SDK ZIP" if development else "Fresh verified ZIP extraction rebuilt and relinked",
                                                         "removal": "SDK references removed; only the disposable SDK folder deleted; unrelated sentinel preserved"}

    def verify(self):
        sdks = {}
        for platform in ("x64", "ARM64"):
            self.inspect(platform)
            sdks[platform] = self.payload(platform)
            self.positive(platform, sdks[platform])
            self.boundary(platform, sdks[platform])
        self.report["dependencyBoundary"] = {"status": "pass", "thirdPartyRuntimeDependencies": [],
                                              "evidence": "Actual core compile/project dependency graph, COFF default-library directives, installed-consumer verbose link commands and executable imports inspected for both architectures",
                                              "consumerRuntimeDependencies": {platform: self.report["consumers"][platform]["runtimeDependencies"] for platform in sdks}}
        if not self.args.inspect_builds_only or self.args.exercise_development_consumers:
            for platform, sdk in sdks.items():
                self.headers(platform, sdk)
            self.negative_matrix(sdks)
            for platform, sdk in sdks.items():
                self.lifecycle(platform, sdk)
        docs = self.consumer / "docs"
        docs.mkdir(exist_ok=True)
        text = ["# Standalone SDK consumer acceptance", "", "Source commit: " + self.commit,
                "Source dirty at execution: " + str(self.dirty).lower(),
                "Mode: " + self.report["mode"], "", "The application owns its public SDK objects and explicit export destination.",
                "It compiles only app/src/main.cpp and app/src/ConsumerApp.cpp, and links the installed ash-core.lib.", ""]
        for platform, result in self.report["consumers"].items():
            text.extend([f"## {platform} Release", "", "Runtime: /MT. Language: C++20. Toolset: v143.",
                         "Library SHA256: " + result["librarySha256"],
                         "Execution status: " + result["execution"]["status"]])
            archive = self.args.archive_x64 if platform == "x64" else self.args.archive_arm64
            if archive:
                text.extend(["Archive: " + archive.name, "Archive SHA256: " + sha(archive)])
            text.append("")
        text.append("ARM64 cross-build/link does not establish physical-hardware or ARM64 runtime qualification.")
        (docs / "ConsumerAcceptance.md").write_text("\n".join(text) + "\n", encoding="utf-8")
        self.report["status"] = "pass"
        self.report["finishedAtUtc"] = now()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("repo-root", "build-root", "consumer-root", "evidence-root", "report"):
        parser.add_argument("--" + name, type=Path, required=True)
    for name in ("sdk-root-x64", "sdk-root-arm64", "archive-x64", "archive-arm64"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--inspect-builds-only", action="store_true")
    parser.add_argument("--exercise-development-consumers", action="store_true",
                        help="With --inspect-builds-only, exercise real-library headers, negative builds and replacement/removal; archive repair stays pending")
    args = parser.parse_args()
    report_path = args.report.resolve()
    protected = [args.repo_root.resolve(), args.build_root.resolve()]
    protected.extend(path.resolve() for path in (args.sdk_root_x64, args.sdk_root_arm64) if path is not None)
    if report_path.exists() or any(report_path.is_relative_to(path) for path in protected):
        print(json.dumps({"status": "fail", "error": "Report must be a new external evidence file separate from protected inputs"}))
        return 1
    verification = None
    try:
        require(not args.exercise_development_consumers or args.inspect_builds_only,
                "--exercise-development-consumers requires --inspect-builds-only")
        verification = Verification(args)
        verification.verify()
        report = verification.report
    except Exception as error:
        report = verification.report if verification else {"schemaVersion": "1.0", "status": "fail"}
        report.update(status="fail", error=str(error), finishedAtUtc=now())
    write_json(args.report, report)
    print(json.dumps({key: report[key] for key in ("status", "mode", "error") if key in report}))
    return 0 if report["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
