"""Exercise issued consumer import prohibitions on disposable copies."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "packaging"))
from sdk_release import contract_profile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("consumer-root", "policy-root", "profile-root", "contract-root", "evidence-root", "report"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    source = args.consumer_root.resolve()
    evidence = args.evidence_root.resolve()
    if evidence.exists() and any(evidence.iterdir()):
        parser.error("Negative-import evidence directory must be fresh")
    if evidence == source or evidence.is_relative_to(source) or source.is_relative_to(evidence):
        parser.error("Disposable import copies must be separate from the real consumer")
    if not (source / "app/src/ConsumerApp.cpp").is_file():
        parser.error("Populated consumer implementation is required")
    evidence.mkdir(parents=True, exist_ok=True)
    results = []

    def validate(name, root, contract, expected_rule=None):
        profile = contract_profile(args.profile_root, args.contract_root / (contract + ".json"))
        report = evidence / (name + ".json")
        command = [sys.executable, str(args.policy_root / "tools/validate_system_realization.py"), "delivery",
                   "--contract", str(args.contract_root / (contract + ".json")), "--repo-root", str(root),
                   "--profile", str(profile), "--report", str(report)]
        started = datetime.now(timezone.utc).isoformat()
        with (evidence / (name + ".log")).open("w", encoding="utf-8") as log:
            process = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=120, check=False)
        value = json.loads(report.read_text(encoding="utf-8"))
        rules = [finding.get("ruleId", finding.get("rule_id", finding.get("id"))) for finding in value.get("findings", [])]
        passed = ((process.returncode == 0 and value.get("status") != "block") if expected_rule is None
                  else process.returncode == 1 and value.get("status") == "block" and expected_rule in rules)
        results.append({"name": name, "command": command, "startedAtUtc": started,
                        "finishedAtUtc": datetime.now(timezone.utc).isoformat(), "exitCode": process.returncode,
                        "expectedRule": expected_rule, "observedRules": rules, "status": "pass" if passed else "fail"})

    for contract in ("consumer-windows", "consumer-canonical"):
        validate("clean-before-" + contract, source, contract)
    fixtures = [
        ("native-source", "consumer-windows", "windows/src/Forbidden.cpp", "void forbidden() {}\n", "RFD-SR-102"),
        ("native-tests", "consumer-windows", "windows/tests/Forbidden.test.cpp", "void forbidden_test() {}\n", "RFD-SR-102"),
        ("test-target", "consumer-windows", "app/forbidden.vcxproj", '<Project><PropertyGroup><TargetName>ash-test-forbidden</TargetName></PropertyGroup></Project>\n', "RFD-SR-106"),
        ("source-submodule", "consumer-windows", ".gitmodules", '[submodule "native"]\n path = Vendor/ASH-Pattern-System-Windows\n url = https://github.com/flynn33/ASH-Pattern-System-Windows.git\n', "RFD-SR-105"),
        ("canonical-vendor", "consumer-canonical", "Vendor/ASH-Pattern-System/README.md", "prohibited vendored authority\n", "RFD-SR-102"),
        ("canonical-acquisition", "consumer-canonical", "app/acquisition.targets", '<Project><Target Name="Acquire"><Exec Command="git clone https://github.com/flynn33/ASH-Pattern-System.git" /></Target></Project>\n', "RFD-SR-106"),
    ]
    for name, contract, relative, text, rule in fixtures:
        copy = evidence / ("copy-" + name)
        shutil.copytree(source, copy, ignore=shutil.ignore_patterns("bin", "obj", ".git"))
        file = copy / relative
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(text, encoding="utf-8")
        validate(name, copy, contract, rule)
    for contract in ("consumer-windows", "consumer-canonical"):
        validate("clean-after-" + contract, source, contract)
    report = {"schemaVersion": "1.0", "kind": "policy-negative-import", "status": "pass" if all(r["status"] == "pass" for r in results) else "fail", "results": results}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(report["status"] + ": " + str(args.report))
    return 0 if report["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
