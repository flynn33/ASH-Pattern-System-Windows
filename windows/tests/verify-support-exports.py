#!/usr/bin/env python3
"""Verify complete native support bundles, including both retained views."""
import argparse
import html
import json
from pathlib import Path

FIELDS = {
    "trigger", "requirement", "precondition", "expected", "observed", "canonical_rule",
    "canonical_case", "canonical_reference", "source_location", "recovery_attempt",
    "recovery_outcome", "terminal_outcome", "fixture", "supported_finding", "hypothesis",
    "unknown_evidence", "resource_observation",
}
EVENT_KEYS = {
    "event_id", "utc", "sequence", "monotonic_ns", "kind", "level", "component",
    "operation_id", "correlation_id", "parent_operation_id", "incident_id", "message",
    "state_before", "state_after", "duration_ns", "unavailable_reason", "context",
    "errors", "redactions", "truncated",
}
HEALTH_KEYS = {
    "accepted", "dropped_capacity", "evicted", "rejected_invalid", "excluded_profile",
    "omitted_fields", "redacted_fields", "truncated_events", "cancelled_captures",
    "collector_failures", "retained_events", "retained_bytes", "closed",
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def load(path):
    return json.loads(path.read_text(encoding="utf-8"), parse_constant=lambda x: (_ for _ in ()).throw(ValueError(x)))


def fnv1a64(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return f"{value:016x}"


def lines(text):
    result = []
    for line in text.splitlines():
        if line.startswith("- ") and ": " in line:
            key, value = line[2:].split(": ", 1)
            result.append((key, html.unescape(value)))
    return result


def verify(marker_path):
    marker = load(marker_path)
    root = marker_path.parent
    require(marker["schema_version"] == "1.0", "marker schema")
    require(marker["checksum_algorithm"] == "fnv1a64-noncryptographic", "checksum identity")
    require({f["path"] for f in marker["files"]} == {"evidence.json", "evidence.md"}, "part inventory")
    for entry in marker["files"]:
        data = (root / entry["path"]).read_bytes()
        require(len(data) == entry["bytes"] <= 8388608, "file size")
        require(fnv1a64(data) == entry["checksum"], "file checksum")
    bundle = load(root / "evidence.json")
    require(bundle["schema_version"] == "1.0", "bundle schema")
    require(bundle["bundle_id"] == marker["bundle_id"], "bundle identity")
    require(bundle["session_id"] == marker["session_id"], "session identity")
    require(bundle["profile"] in ("Development", "Release"), "profile")
    require(bundle["event_count"] == marker["event_count"] == len(bundle["events"]), "event count")
    require(len(bundle["events"]) <= 4096, "event limit")
    require(set(bundle["health"]) == HEALTH_KEYS, "health schema")
    require(bundle["health"]["retained_events"] == len(bundle["events"]), "health count")
    require(0 <= bundle["health"]["retained_bytes"] <= 4194304, "retention bound")
    require(bundle["attachments"] == [], "unexpected attachments")
    md = (root / "evidence.md").read_text(encoding="utf-8")
    sections = md.split("\n### Event ")
    heading_values = dict(lines(sections[0]))
    for name in ("schema_version", "bundle_id", "generated_at_utc", "session_id", "profile"):
        require(heading_values[name] == bundle[name], f"paired {name}")
    for name in ("capture_start_utc", "capture_end_utc"):
        require(heading_values[name] == str(bundle[name] if bundle[name] is not None else "null"), f"paired {name}")
    require(int(heading_values["event_count"]) == len(bundle["events"]), "Markdown count")
    require(json.loads(heading_values["identity_json"]) == bundle["identity"], "paired identity")
    require(json.loads(heading_values["health_json"]) == bundle["health"], "paired health")
    require(len(sections) - 1 == len(bundle["events"]), "full Markdown retained evidence")
    previous = 0
    event_ids = set()
    for event, section in zip(bundle["events"], sections[1:]):
        require(set(event) == EVENT_KEYS, "event schema")
        require(isinstance(event["sequence"], int) and event["sequence"] > previous, "sequence ordering")
        previous = event["sequence"]
        require(event["event_id"] not in event_ids, "duplicate event id")
        event_ids.add(event["event_id"])
        require(isinstance(event["monotonic_ns"], int) and event["monotonic_ns"] >= 0, "monotonic timing")
        require(event["kind"] in ("lifecycle", "operation", "decision", "fault", "health"), "event kind")
        require(event["level"] in ("information", "warning", "error", "critical"), "event severity")
        require(type(event["truncated"]) is bool and type(event["redactions"]) is int, "event health types")
        require(len(json.dumps(event, ensure_ascii=False, separators=(",", ":")).encode("utf-8")) <= 16384, "event size")
        values = lines(section.split("\n## Export boundary", 1)[0])
        expected = []
        scalar_names = ("event_id", "utc", "sequence", "monotonic_ns", "kind", "level", "component",
                        "operation_id", "correlation_id", "parent_operation_id", "incident_id", "message",
                        "state_before", "state_after", "duration_ns", "unavailable_reason")
        for name in scalar_names:
            value = event[name]
            expected.append((name, "null" if value is None else str(value)))
        for context in event["context"]:
            require(set(context) == {"field", "value"} and context["field"] in FIELDS, "allowlisted context")
            expected.append((context["field"], context["value"]))
        for error in event["errors"]:
            require(set(error) == {"domain", "code", "message", "causes"}, "typed error")
            expected.extend((("error.domain", error["domain"]), ("error.code", "null" if error["code"] is None else str(error["code"])),
                             ("error.message", error["message"])))
            expected.extend(("error.cause", cause) for cause in error["causes"])
        expected.extend((("redactions", str(event["redactions"])), ("truncated", str(event["truncated"]).lower())))
        require(values == expected, f"complete paired event {event['sequence']}")
        if bundle["profile"] == "Release":
            require(event["kind"] in ("fault", "health"), "release category")
            require(event["state_before"] is None and event["state_after"] is None and event["duration_ns"] is None,
                    "release state/timing capture excluded")
            require(all(not error["message"] and not error["causes"] for error in event["errors"]), "release error detail")
    for incident in bundle["incidents"]:
        require(incident["trigger_event_id"] in event_ids, "incident references retained event")
        require(html.unescape(md).count(incident["incident_id"]) >= 2, "paired incident index and account")
    boundary_values = dict(lines(md.split("\n## Export boundary\n", 1)[1]))
    require(int(boundary_values["export_redactions"]) == bundle["export_redactions"], "paired export redactions")
    return bundle["profile"], len(bundle["events"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--require-profiles", nargs="+", choices=("Development", "Release"), default=[])
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    require(args.root.is_dir(), "missing support evidence root")
    markers = sorted(args.root.rglob("COMPLETED.json"))
    require(bool(markers), "no complete support bundles")
    profiles = set()
    results = []
    for marker in markers:
        profile, events = verify(marker)
        profiles.add(profile)
        results.append({"marker": str(marker), "profile": profile, "event_count": events, "status": "pass"})
    require(set(args.require_profiles) <= profiles, "required profile export is absent")
    report = {"schema_version": "1.0", "bundles": len(results), "profiles": sorted(profiles), "results": results}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {len(results)} complete support bundles: {', '.join(sorted(profiles))}")


if __name__ == "__main__":
    main()
