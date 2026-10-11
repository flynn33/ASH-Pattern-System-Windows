"""Validate real native exports against immutable schemas and independent identities."""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "packaging"))
from canonical_resources import PIN, validate_cache

SEMANTIC = ("diagnostic_kind", "disposition", "rule_ids", "severity", "stage",
            "subject_reference", "summary", "notes", "details")
PHASE = {name: phase for phase, names in enumerate((
    ("DETECTION", "PARSE"), ("CLASSIFICATION", "ASSESS", "CLASSIFY"),
    ("RECOVERY", "RECOVER", "FALLBACK"), ("ESCALATION", "CONTAIN"), ("TERMINAL", "HALT"))) for name in names}
OPERATIONS = {"TRANSFORM", "PLAN", "EVALUATE", "EMIT", "VALIDATE", "VERIFY"}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def schema_check(value, schema, location="record"):
    """Strict evaluator for the keywords actually present in the three pinned schemas."""
    supported = {"$id", "$schema", "title", "description", "product_version", "type", "const", "enum",
                 "properties", "required", "additionalProperties", "items", "minItems", "minLength", "pattern", "minimum"}
    require(not set(schema).difference(supported), location + ": unsupported schema keyword")
    types = {"object": isinstance(value, dict), "array": isinstance(value, list), "string": isinstance(value, str),
             "integer": isinstance(value, int) and not isinstance(value, bool), "boolean": isinstance(value, bool), "null": value is None}
    if "type" in schema:
        allowed = schema["type"] if isinstance(schema["type"], list) else [schema["type"]]
        require(any(types.get(name, False) for name in allowed), location + ": wrong type")
    if "const" in schema:
        require(type(value) is type(schema["const"]) and value == schema["const"], location + ": wrong constant")
    if "enum" in schema:
        require(value in schema["enum"], location + ": unsupported vocabulary")
    if isinstance(value, dict):
        require(set(schema.get("required", [])).issubset(value), location + ": missing required property")
        properties = schema.get("properties", {})
        if schema.get("additionalProperties") is False:
            require(set(value).issubset(properties), location + ": unexpected property")
        for key, item in value.items():
            if key in properties:
                schema_check(item, properties[key], location + "." + key)
    if isinstance(value, list):
        require(len(value) >= schema.get("minItems", 0), location + ": short array")
        for index, item in enumerate(value):
            if "items" in schema:
                schema_check(item, schema["items"], f"{location}[{index}]")
    if isinstance(value, str):
        value.encode("utf-8", errors="strict")
        require(len(value) >= schema.get("minLength", 0), location + ": short string")
        if "pattern" in schema:
            require(re.search(schema["pattern"], value) is not None, location + ": invalid pattern")
    if "minimum" in schema:
        require(value >= schema["minimum"], location + ": below minimum")


def json_domain(value):
    if value is None or isinstance(value, bool):
        return
    if isinstance(value, int):
        require(-(2**63) <= value < 2**63, "Diagnostic integer outside signed 64-bit domain")
    elif isinstance(value, str):
        value.encode("utf-8", errors="strict")
    elif isinstance(value, list):
        for item in value:
            json_domain(item)
    elif isinstance(value, dict):
        for key, item in value.items():
            require(isinstance(key, str), "Diagnostic object key is not text")
            key.encode("utf-8", errors="strict")
            json_domain(item)
    else:
        raise ValueError("Unsupported diagnostic JSON scalar")


def record_check(record):
    require(all(isinstance(record[name], str) and record[name].strip() for name in ("summary", "subject_reference")), "Missing explanatory text")
    require("\n" not in record["summary"] and "\r" not in record["summary"], "Multiline summary")
    require(record["notes"] and all(note.strip() for note in record["notes"]), "Missing meaningful explanatory note")
    json_domain(record["details"])
    json_domain(record["occurrence_metadata"])
    payload = {name: record[name] for name in SEMANTIC}
    encoded = json.dumps(payload, sort_keys=True, ensure_ascii=False, separators=(",", ":"), allow_nan=False).encode("utf-8")
    expected = "APS-DIAG-" + hashlib.sha256(encoded).hexdigest()[:24].upper()
    require(record["diagnostic_id"] == expected, "Native identity differs from independent nine-field SHA-256")
    require(1 <= record["sequence_number"] < 2**63, "Sequence outside canonical integer domain")


def chain_check(chain):
    require(bool(chain), "Empty chain")
    root = chain[0]
    require(root["parent_diagnostic_id"] is None and root["sequence_number"] == 1, "Invalid root")
    phase = PHASE.get(root["stage"], 0)
    for index, record in enumerate(chain):
        record_check(record)
        require(record["chain_root_diagnostic_id"] == root["diagnostic_id"], "Lost chain root")
        require(record["sequence_number"] == index + 1, "Broken sequence")
        if not index:
            continue
        parent = chain[index - 1]
        require(parent["diagnostic_kind"] != "SAFE_HALT" and parent["disposition"] != "TERMINAL" and PHASE.get(parent["stage"]) != 4, "Child after frozen terminal")
        require(record["parent_diagnostic_id"] == parent["diagnostic_id"], "Broken parent identity")
        severity = ("INFO", "WARNING", "ERROR", "CRITICAL")
        require(severity.index(record["severity"]) >= severity.index(parent["severity"]), "Severity de-escalated")
        if record["stage"] in OPERATIONS:
            require(phase < 3, "Operation after containment")
            continue
        current = PHASE[record["stage"]]
        require(current >= phase, "Causal phase de-escalated")
        if current == 3:
            require(phase in (1, 2) or (phase == 3 and record["diagnostic_kind"] == "CONTAINMENT"), "Invalid containment entry")
        if current == 4:
            require(phase in (2, 3), "Invalid halt entry")
        phase = current


def projection_check(value, name, schemas):
    schema_check(value, schemas[name])
    data = value["data"]
    if name == "diagnostic-envelope":
        record_check(data)
        require(value["id"] == data["diagnostic_id"], "Wrapper identity mismatch")
    else:
        chain_check(data["full_diagnostic_chain"])
        require(data["original_diagnostic"] == data["full_diagnostic_chain"][0], "Original is not actual root")
        require(data["diagnostic"] == data["full_diagnostic_chain"][-1], "Action is not actual tail")
        require(value["id"] == data["diagnostic"]["diagnostic_id"], "Action wrapper identity mismatch")
        action = data["diagnostic"]
        if name == "containment-record":
            require(action["diagnostic_kind"] == "CONTAINMENT" and PHASE.get(action["stage"]) == 3,
                    "Containment projection does not preserve an actual containment action")
        else:
            require(action["diagnostic_kind"] == "SAFE_HALT" and PHASE.get(action["stage"]) == 4
                    and action["disposition"] == "TERMINAL", "Halt projection does not preserve a terminal SAFE_HALT action")


def verify(args):
    integrity = validate_cache(args.authority_cache)
    version = (Path(__file__).resolve().parents[2] / "VERSION").read_text(encoding="utf-8-sig").strip()
    schema_paths = {name: args.authority_cache / "schemas/1.0" / (name + ".schema.json")
                    for name in ("diagnostic-envelope", "containment-record", "safe-halt-record")}
    schemas = {name: json.loads(path.read_text(encoding="utf-8")) for name, path in schema_paths.items()}
    findings = []
    for runner_path in args.runner_report:
        runner = json.loads(runner_path.read_text(encoding="utf-8-sig"))
        require(runner["status"] == "pass", "Native runner did not pass")
        counts = {"diagnostic-envelope": 0, "containment-record": 0, "safe-halt-record": 0, "retained-chain": 0, "diagnostic-chain": 0}
        mapping = {"JSON_RECORD": "diagnostic-envelope", "JSON_CONTAINMENT": "containment-record", "JSON_SAFE_HALT": "safe-halt-record", "JSON_RETAINED_CHAIN": "retained-chain", "JSON_DIAGNOSTIC_CHAIN": "diagnostic-chain"}
        exports = []
        for result in runner["results"]:
            require(result["status"] == "passed" and result["sha256"], "Unverified native executable")
            for line in result["output"].splitlines():
                match = re.fullmatch(r"\[(JSON_RECORD|JSON_CONTAINMENT|JSON_SAFE_HALT|JSON_RETAINED_CHAIN|JSON_DIAGNOSTIC_CHAIN)\] (.*)", line)
                if not match:
                    continue
                name = mapping[match[1]]
                value = json.loads(match[2])
                if name in ("retained-chain", "diagnostic-chain"):
                    for record in value:
                        schema_check({"schema_version": "1.0", "product_version": "validation", "id": record["diagnostic_id"], "data": record}, schemas["diagnostic-envelope"])
                    chain_check(value)
                else:
                    require(value["product_version"] == version, "Export does not identify actual SDK build version")
                    projection_check(value, name, schemas)
                counts[name] += 1
                exports.append((name, value))
        require(counts == {"diagnostic-envelope": 16, "containment-record": 4, "safe-halt-record": 3, "retained-chain": 1, "diagnostic-chain": 7}, "Incomplete native serialized record inventory")
        # Independent corruption controls prove schemas/identity/chain checks
        # fail even if native tests report success and export syntactically valid JSON.
        terminal = next(value for name, value in exports if name == "safe-halt-record")
        for mutation in ("empty-notes", "stale-details", "broken-parent", "wrong-original", "nonterminal", "missing-time", "wrapper-id", "wrong-halt-kind", "wrong-containment-kind"):
            projection = "containment-record" if mutation == "wrong-containment-kind" else "safe-halt-record"
            original = next(value for name, value in exports if name == projection) if projection == "containment-record" else terminal
            damaged = copy.deepcopy(original)
            data = damaged["data"]
            if mutation == "empty-notes": data["full_diagnostic_chain"][0]["notes"] = []
            if mutation == "stale-details": data["full_diagnostic_chain"][0]["details"]["unobserved"] = True
            if mutation == "broken-parent": data["full_diagnostic_chain"][-1]["parent_diagnostic_id"] = None
            if mutation == "wrong-original": data["original_diagnostic"] = data["diagnostic"]
            if mutation == "nonterminal": data["is_terminal"] = False
            if mutation == "missing-time": del data["halt_time"]
            if mutation == "wrapper-id": damaged["id"] = "APS-DIAG-" + "0" * 24
            if mutation in ("wrong-halt-kind", "wrong-containment-kind"):
                action = data["full_diagnostic_chain"][-1]
                action["diagnostic_kind"] = "RECOVERY"
                payload = {key: action[key] for key in SEMANTIC}
                action["diagnostic_id"] = "APS-DIAG-" + hashlib.sha256(json.dumps(payload, sort_keys=True, ensure_ascii=False,
                    separators=(",", ":"), allow_nan=False).encode("utf-8")).hexdigest()[:24].upper()
                data["diagnostic"] = copy.deepcopy(action)
                damaged["id"] = action["diagnostic_id"]
            rejected = False
            try: projection_check(damaged, projection, schemas)
            except (ValueError, UnicodeError): rejected = True
            require(rejected, "Corruption was accepted: " + mutation)
        findings.append({"runnerReportSha256": hashlib.sha256(runner_path.read_bytes()).hexdigest(), "counts": counts,
                         "negativeControls": 9, "binaryDigests": {item["name"]: item["sha256"] for item in runner["results"]}})
    return {"status": "pass", "canonicalCommit": PIN, "resourceCount": integrity["resourceCount"], "profiles": findings,
            "schemaDigests": {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in schema_paths.items()}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--authority-cache", type=Path, required=True)
    parser.add_argument("--runner-report", type=Path, action="append", required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    try: report = verify(args)
    except Exception as error: report = {"status": "fail", "error": str(error), "canonicalCommit": PIN}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report))
    return 0 if report["status"] == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
