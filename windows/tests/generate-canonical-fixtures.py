"""Project canonical expected vectors into native test data after byte-integrity validation.

Development-only: generated inputs stay in the explicit external build root.
The generator never calls the native implementation to obtain expected results.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "packaging"))
from canonical_resources import PIN, validate_cache


def q(value):
    if not isinstance(value, str): raise ValueError("C++ string fixture must be text")
    return '"' + ''.join(chr(byte) if 32 <= byte < 127 and byte not in (34, 92)
                         else ('\\"' if byte == 34 else '\\\\' if byte == 92 else f'\\{byte:03o}')
                         for byte in value.encode("utf-8")) + '"'


def cpp_value(value):
    if value is None: return "ash::DiagnosticValue{nullptr}"
    if isinstance(value, bool): return "ash::DiagnosticValue{" + str(value).lower() + "}"
    if isinstance(value, int) and -(2 ** 63) <= value < 2 ** 63:
        return "ash::DiagnosticValue{std::int64_t{" + str(value) + "}}"
    if isinstance(value, str): return "ash::DiagnosticValue{" + q(value) + "}"
    if isinstance(value, list): return "ash::DiagnosticValue{ash::DiagnosticValue::Array{" + ",".join(map(cpp_value, value)) + "}}"
    if isinstance(value, dict): return "ash::DiagnosticValue{" + cpp_object(value) + "}"
    raise ValueError("Unsupported canonical diagnostic value")


def cpp_object(value):
    if not isinstance(value, dict): raise ValueError("Diagnostic object required")
    return "ash::DiagnosticObject{" + ",".join("{" + q(key) + "," + cpp_value(item) + "}" for key, item in sorted(value.items())) + "}"


def diagnostic_row(vector_id, payload, occurrence, expected, child=False, same_identity=False):
    if set(payload) - {"diagnostic_kind", "severity", "stage", "disposition", "rule_ids", "subject_reference", "summary", "notes", "details", "occurrence_metadata"}:
        raise ValueError("Unmapped diagnostic payload field")
    fields = [q(vector_id)] + ["ash::" + enum + "::" + payload[key] for enum, key in (
        ("DiagnosticKind", "diagnostic_kind"), ("Severity", "severity"), ("Stage", "stage"), ("Disposition", "disposition"))]
    fields += [q(payload["subject_reference"]), q(payload["summary"]),
               "{" + ",".join(map(q, payload["rule_ids"])) + "}", "{" + ",".join(map(q, payload["notes"])) + "}",
               cpp_object(payload["details"]), "std::nullopt" if occurrence is None else cpp_object(occurrence),
               q(json.dumps(expected, sort_keys=True, separators=(",", ":"), ensure_ascii=False)),
               str(child).lower(), str(same_identity).lower()]
    return ",".join(fields)


def default_plan_payload(datasets):
    """Independent golden for the documented native v2 compatibility request.

    Canonical realm/topology expectations come from the pinned corpus. The
    downstream request, registry IDs, descriptor names and wire ordering are
    explicit fixtures; no native planner, hash or diagnostic helper is called.
    """
    realm = next(r["expected_output"] for r in datasets["states"]
                 if r["expected_output"]["state_signature"] == "000011110")
    topology = next(r["expected_output"] for r in datasets["topology"]
                    if r["input"]["depth"] == 0)
    assert topology["total_nodes"] == 1 and topology["leaf_count"] == 1
    node = topology["nodes"][0]
    role_id = "APS-ROLE-" + node["node_id"]
    axiom_names = ["a1_relational_existence", "a2_structural_compressibility",
                   "a3_multi_scale_persistence", "a4_energetic_cost_of_erasure",
                   "a5_self_reference_for_consciousness"]
    explanations = [
        "relation_count=unavailable; required relation_count > 0",
        "compressed_measure=unavailable; raw_measure=unavailable; required compressed_measure < raw_measure",
        "scale_count=unavailable; required scale_count > 1",
        "erasure_cost=unavailable; required erasure_cost > 0",
        "self_reference_capable=unavailable; self_model_available=unavailable; required both values true"]
    notes = [name + ": INDETERMINATE; " + explanation
             for name, explanation in zip(axiom_names, explanations)]
    compact = lambda value: json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)

    def record(summary, record_notes):
        semantic = {"details": {}, "diagnostic_kind": "AXIOM", "disposition": "PENDING",
                    "notes": record_notes, "rule_ids": ["ASH-AXIOM-001", "ASH-AXIOM-002"],
                    "severity": "WARNING", "stage": "EVALUATE",
                    "subject_reference": "axiom-subject[ash-plan/APS-REALM-030] profile[canonical-five-axioms]",
                    "summary": summary}
        identifier = "APS-DIAG-" + hashlib.sha256(compact(semantic).encode()).hexdigest()[:24].upper()
        return compact(dict(semantic, chain_root_diagnostic_id=identifier, diagnostic_id=identifier,
                            occurrence_metadata=None, parent_diagnostic_id=None,
                            schema_version="1.0", sequence_number=1))

    def field(name, value):
        if isinstance(value, bool): value = int(value)
        value = str(value)
        return f"{name}:{len(value.encode('utf-8'))}:{value}\n"

    def strings(name, values):
        return field(name + ".count", len(values)) + "".join(field(name + ".item", v) for v in values)

    subject = field("identifier", "ash-plan/APS-REALM-030")
    for name in ("relation_count", "compressed_measure", "raw_measure", "scale_count",
                 "erasure_cost", "self_reference_capable", "self_model_available"):
        subject += field(name + ".present", 0)
    diagnosis = "".join(field(name, "INDETERMINATE") for name in axiom_names)
    diagnosis += field("overall_status", "INDETERMINATE") + field("overall_pass.present", 0)
    diagnosis += field("evaluations.count", 5)
    for name, note in zip(axiom_names, notes):
        diagnosis += field("evaluation.status", "INDETERMINATE") + field("evaluation.diagnostic.present", 1)
        diagnosis += field("evaluation.diagnostic", record(name + ": INDETERMINATE", [note]))
    diagnosis += field("diagnostic.present", 1) + field("diagnostic", record(
        "Canonical five-axiom diagnosis: INDETERMINATE", notes + [
            "All five canonical predicates are required: a known failure takes precedence over unavailable evidence; no missing measurement is inferred from state."]))
    payload = "".join(field(name, value) for name, value in [
        ("identity_format", "aps-windows-plan-2"), ("request.project_name", "ash-plan"),
        ("request.seed_state", "000011110"), ("request.transition_sequence.count", 0),
        ("request.topology_depth", 0), ("request.profile_id", "aps-windows-descriptor-1.0"),
        ("request.naming_policy_id", "aps-windows-realm-names-1.0"), ("request.emission_target_kind", "descriptor"),
        ("inputs.target.target_class", "desktop"), ("inputs.target.runtime_hint", "win32"),
        ("inputs.target.profile_id", "aps-windows-descriptor-1.0"), ("inputs.target.registry_version", "aps-windows-1.0"),
        ("inputs.axiom_subject.present", 0), ("inputs.axiom_profile.identifier", "canonical-five-axioms"),
        ("source_signature", "000011110"), ("source_realm_id", "APS-REALM-030"),
        ("target_class", "desktop"), ("registry_version", "aps-windows-1.0"), ("runtime_hint", "win32"),
        ("profile_id", "aps-windows-descriptor-1.0"), ("normalized_state", "000011110"), ("transformed_state", "000011110")])
    for prefix in ("source_realm", "destination_realm"):
        for name in ("state_signature", "realm_index", "realm_id", "orbit_id", "b8"):
            payload += field(prefix + "." + name, realm[name])
    payload += field("topology_nodes.count", 1)
    for name in ("path", "node_id", "depth", "generation_ordinal", "global_ordinal"):
        payload += field("node." + name, node[name])
    for name, value in [("parent_index", 0), ("has_parent", 0), ("parent_path", ""), ("is_leaf", True)]:
        payload += field("node." + name, value)
    payload += field("leaf_indices.count", 1) + field("leaf_indices.item", 0) + field("roles.count", 1)
    for name, value in [("assignment_id", role_id), ("node_id", node["node_id"]), ("role", "root"), ("realm_id", realm["realm_id"])]:
        payload += field("role." + name, value)
    payload += strings("role_assignments", ["source-state=APS-REALM-030", "target-class=desktop",
        "node-role=" + role_id + ";node=" + node["node_id"] + ";role=root;realm=APS-REALM-030"])
    payload += field("axiom_subject", subject) + field("axiom_profile.identifier", "canonical-five-axioms")
    payload += field("axiom_diagnostic", diagnosis) + field("artifacts.count", 2)
    descriptions = ["canonical-state-summary:APS-REALM-030", "diagnostic-chain-export:APS-REALM-030"]
    for suffix, description in zip(("state-summary", "diagnostic-chain-export"), descriptions):
        payload += field("artifact.artifact_id", "ash-plan/" + suffix) + field("artifact.kind", "descriptor")
        payload += field("artifact.name", description) + field("artifact.summary", description)
        payload += strings("artifact.role_references", [role_id])
    payload += strings("artifact_descriptions", descriptions) + strings("warnings", notes)
    metadata = {"identity_format": "aps-windows-plan-2", "project_name": "ash-plan", "profile_id": "aps-windows-descriptor-1.0",
                "profile_registry_version": "aps-windows-profiles-1.0", "naming_policy_id": "aps-windows-realm-names-1.0",
                "naming_registry_version": "aps-windows-naming-1.0", "emission_target_kind": "descriptor",
                "target_registry_version": "aps-windows-targets-1.0", "codeword_registry_version": "aps-canonical-codewords-1.0",
                "topology_registry_version": "aps-canonical-ternary-1.0", "axiom_predicate_version": "aps-canonical-five-axioms-1.0",
                "registry_version": "aps-windows-1.0", "runtime_hint": "win32", "target_class": "desktop",
                "axiom_profile_id": "canonical-five-axioms", "axiom_evidence_source": "UNAVAILABLE"}
    payload += field("metadata.count", len(metadata))
    for name, value in sorted(metadata.items()):
        payload += field("metadata.key", name) + field("metadata.value", value)
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--authority-cache", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    integrity = validate_cache(args.authority_cache)
    if integrity["status"] != "pass":
        raise ValueError("Authority cache integrity did not pass")
    corpus = args.authority_cache / "conformance" / "1.0" / "vectors"
    datasets = {p.stem: [json.loads(line) for line in p.read_text(encoding="utf-8").splitlines()] for p in sorted(corpus.glob("*.jsonl"))}
    expected_fields = {
        "states": {"b8", "bits", "orbit_id", "realm_id", "realm_index", "state_signature"},
        "codewords": {"bits", "codeword_id", "is_identity", "ordering_rank", "signature", "weight"},
        "realm-identities": {"realm_id", "realm_index", "state_signature"},
        "reachability": {"reachable_target_count", "reachable_targets", "unreachable_target_count"},
        "topology": {"depth", "leaf_count", "leaves", "nodes", "total_nodes"},
        "state-assessments": {"assessment_blocked", "codeword_id", "input_status", "state_class"},
        "recovery": {"category", "delta"}, "fallback": {"replacement_kind"},
        "containment": {"mode", "restricted_operations_only", "breach_confirmed", "chain_append_count", "diagnostics_frozen", "diagnostics_preserved", "terminal", "transitions_allowed"},
        "safe-halt": {"expected_failure", "terminal", "transitions_allowed", "mode",
                      "diagnostics_preserved", "diagnostics_frozen", "chain_append_count"},
        "axioms": {"axiom_result"}, "generation-plans": {"expected_failure", "plan_hash_changes", "plan_hash_stable"},
        "materialization-boundary": {"every_output_has_plan_reference", "expected_failure"},
        "diagnostics": {"chain_root_diagnostic_id", "diagnostic_id", "diagnostic_kind", "disposition", "notes", "parent_diagnostic_id", "rule_ids", "sequence_number", "severity", "stage", "subject_reference", "summary", "schema_version", "details", "occurrence_metadata"},
        "diagnostic-chains": {"chain_valid", "records", "identity_unchanged"},
        "averaging": {"is_idempotent", "member_count", "result", "result_inside_orbit", "result_on_same_orbit", "result_outside_orbit"},
    }
    for dataset, rows in datasets.items():
        for row in rows:
            if set(row.get("expected_output", {})) - expected_fields.get(dataset, set()):
                raise ValueError(f"Unmapped expected field in {row['vector_id']}")
            if dataset in {"states", "codewords"}:
                signature = row["expected_output"].get("state_signature", row["expected_output"].get("signature"))
                if row["expected_output"]["bits"] != [int(bit) for bit in signature]:
                    raise ValueError(f"Inconsistent bits/signature in {row['vector_id']}")
            if dataset == "topology":
                expected = row["expected_output"]
                if expected["depth"] != row["input"]["depth"] or expected["total_nodes"] != len(expected["nodes"]) or expected["leaves"] != [n for n in expected["nodes"] if n["is_leaf"]] or expected["leaf_count"] != len(expected["leaves"]):
                    raise ValueError(f"Inconsistent topology counts/leaves in {row['vector_id']}")
            if dataset == "diagnostics":
                expected = row["expected_output"]
                if any(expected[key] != value for key, value in row["input"].items()) or expected["parent_diagnostic_id"] is not None or not expected["notes"] or any(not note.strip() for note in expected["notes"]):
                    raise ValueError(f"Unmapped diagnostic root/notes/rules in {row['vector_id']}")
            if dataset == "averaging" and any(value.get("denominator") != 16 for value in row["expected_output"].values() if isinstance(value, dict)):
                raise ValueError(f"Unexpected averaging denominator in {row['vector_id']}")
    output = ["// Pinned canonical vectors and explicit downstream wire golden; not SDK payload.",
              "#pragma once", "#include <cstdint>", "#include <cstddef>", '#include "ash/DiagnosticEnvelope.hpp"', "namespace canonical_fixture {",
              'inline constexpr const char* source_commit = ' + q(PIN) + ';']

    def array(name, declaration, rows):
        output.append(declaration)
        output.append(f"inline const {declaration.split('struct ')[1].split(' ')[0]} {name}[]{{")
        output.extend("    {" + row + "}," for row in rows)
        output.append("};")

    array("realms", "struct RealmRow { const char* id; const char* signature; std::uint16_t index; const char* realm; const char* orbit; bool b8; };", [
        ",".join([q(r["vector_id"]), q(r["expected_output"]["state_signature"]), str(r["expected_output"]["realm_index"]), q(r["expected_output"]["realm_id"]), q(r["expected_output"]["orbit_id"]), str(bool(r["expected_output"]["b8"])).lower()]) for r in datasets["states"]])
    array("realm_identities", "struct RealmIdentityRow { const char* id; const char* signature; std::uint16_t index; const char* realm; };", [
        ",".join([q(r["vector_id"]), q(r["expected_output"]["state_signature"]), str(r["expected_output"]["realm_index"]), q(r["expected_output"]["realm_id"])]) for r in datasets["realm-identities"]])
    array("codewords", "struct CodewordRow { const char* id; const char* codeword_id; const char* signature; std::size_t rank; std::size_t weight; bool identity; };", [
        ",".join([q(r["vector_id"]), q(r["expected_output"]["codeword_id"]), q(r["expected_output"]["signature"]), str(r["expected_output"]["ordering_rank"]), str(r["expected_output"]["weight"]), str(r["expected_output"]["is_identity"]).lower()]) for r in datasets["codewords"]])
    array("transformations", "struct TransformRow { const char* id; std::uint16_t source; std::uint16_t target; std::size_t codeword; const char* source_realm; const char* target_realm; const char* source_orbit; const char* target_orbit; bool source_b8; bool target_b8; };", [
        ",".join([q(r["vector_id"]), str(int(r["source_signature"], 2)), str(int(r["target_signature"], 2)), str(int(r["codeword_id"].split("-")[-1])), q(r["source_realm_id"]), q(r["target_realm_id"]), q(r["source_orbit_id"]), q(r["target_orbit_id"]), str(bool(r["source_b8"])).lower(), str(bool(r["target_b8"])).lower()]) for r in datasets["transformations"]])
    array("reachability", "struct ReachRow { const char* id; std::uint16_t source; std::uint16_t targets[16]; std::size_t reachable_count; std::size_t unreachable_count; };", [
        ",".join([q(r["vector_id"]), str(int(r["input"]["source_signature"], 2)), "{" + ",".join(str(int(t, 2)) for t in r["expected_output"]["reachable_targets"]) + "}", str(r["expected_output"]["reachable_target_count"]), str(r["expected_output"]["unreachable_target_count"])]) for r in datasets["reachability"]])
    nodes = []
    for r in datasets["topology"]:
        depth = r["input"]["depth"]
        for n in r["expected_output"]["nodes"]:
            nodes.append(",".join([q(r["vector_id"]), str(depth), q(n["node_id"]), q(n["path"]), q(n["parent_path"] or ""), str(n["depth"]), str(n["generation_ordinal"]), str(n["global_ordinal"]), str(n["is_leaf"]).lower()]))
    array("topology_nodes", "struct TopologyRow { const char* id; std::size_t request_depth; const char* node_id; const char* path; const char* parent; std::size_t depth; std::size_t ordinal; std::size_t global; bool leaf; };", nodes)
    array("assessments", "struct AssessmentRow { const char* id; const char* candidate; const char* stable[2]; std::size_t stable_count; bool context_present; bool blocked; const char* state_class; const char* codeword_id; const char* input_status; };", [
        ",".join([q(r["vector_id"]), q(r["input"]["candidate"]), "{" + ",".join(q(s) for s in (r["input"].get("allowed_stable_states", []) + [""] * 2)[:2]) + "}", str(len(r["input"].get("allowed_stable_states", []))), str("allowed_stable_states" in r["input"]).lower(), str(r["expected_output"].get("assessment_blocked", False)).lower(), q(r["expected_output"].get("state_class", "")), q(r["expected_output"].get("codeword_id", "")), q(r["expected_output"].get("input_status", ""))]) for r in datasets["state-assessments"]])
    array("recovery", "struct RecoveryRow { const char* id; const char* current; const char* target; const char* state_class; const char* category; const char* delta; };", [
        ",".join(q(v) for v in [r["vector_id"], r["input"].get("current", ""), r["input"].get("target", ""), r["input"].get("state_class", ""), r["expected_output"]["category"], r["expected_output"].get("delta", "")]) for r in datasets["recovery"]])
    array("fallback", "struct FallbackRow { const char* id; const char* policy_id; const char* replacement_kind; };", [
        ",".join(q(v) for v in [r["vector_id"], r["input"]["policy_id"], r["expected_output"]["replacement_kind"]]) for r in datasets["fallback"]])
    array("diagnostics", "struct DiagnosticRow { const char* id; ash::DiagnosticKind kind; ash::Severity severity; ash::Stage stage; ash::Disposition disposition; const char* subject; const char* summary; std::vector<std::string> rules; std::vector<std::string> notes; ash::DiagnosticObject details; std::optional<ash::DiagnosticObject> occurrence; const char* expected_record; bool child; bool same_identity; };", [
        diagnostic_row(r["vector_id"], r["input"], r["input"]["occurrence_metadata"], r["expected_output"]) for r in datasets["diagnostics"]])
    chain_rows = []
    for row in datasets["diagnostic-chains"]:
        expected = row["expected_output"]
        source = row["input"]
        if "payloads" in source:
            if set(source) != {"payloads", "occurrence_metadata"} or expected != {"chain_valid": True, "records": expected["records"]}:
                raise ValueError("Unmapped diagnostic chain scenario")
            pairs = [(payload, source["occurrence_metadata"], index != 0, False) for index, payload in enumerate(source["payloads"])]
        else:
            if set(source) != {"payload", "occurrences"} or expected != {"identity_unchanged": True, "records": expected["records"]}:
                raise ValueError("Unmapped occurrence variation scenario")
            pairs = [(source["payload"], occurrence, False, index != 0) for index, occurrence in enumerate(source["occurrences"])]
        if len(pairs) != len(expected["records"]): raise ValueError("Diagnostic chain record count mismatch")
        for (payload, occurrence, child, same_identity), record in zip(pairs, expected["records"]):
            chain_rows.append(diagnostic_row(row["vector_id"], payload, occurrence, record, child, same_identity))
    output.append("inline const DiagnosticRow diagnostic_chains[]{")
    output.extend("    {" + row + "}," for row in chain_rows)
    output.append("};")
    array("averaging", "struct AverageRow { const char* id; const char* function; const char* orbit; const char* state; double inside; double outside; bool idempotency_specified; bool idempotent; std::size_t member_count; };", [
        ",".join([q(r["vector_id"]), q(r["input"]["function"]), q(r["input"].get("orbit_id", "")), q(r["input"].get("state_signature", "")), str((r["expected_output"].get("result") or r["expected_output"].get("result_inside_orbit") or r["expected_output"].get("result_on_same_orbit"))["numerator"] / 16), str(r["expected_output"].get("result_outside_orbit", {"numerator": 0})["numerator"] / 16), str("is_idempotent" in r["expected_output"]).lower(), str(r["expected_output"].get("is_idempotent", False)).lower(), str(r["expected_output"].get("member_count", 0))]) for r in datasets["averaging"]])
    array("containment", "struct ContainmentRow { const char* id; const char* risk; const char* operation; const char* initial_mode; bool boundary_breached; bool attempt_prevented; bool propagation_observed; const char* mode; bool restricted; bool breach_confirmed; std::size_t append_count; bool frozen; bool preserved; bool terminal; bool transitions_allowed; };", [
        ",".join([q(r["vector_id"]), q(r["input"].get("risk", "")), q(r["input"].get("attempted_operation", "")), q(r["input"]["initial_mode"])] +
                 [str(r["input"].get(key, False)).lower() for key in ("boundary_breached", "attempt_prevented", "propagation_observed")] +
                 [q(r["expected_output"]["mode"])] + [str(r["expected_output"].get(key, False)).lower() for key in ("restricted_operations_only", "breach_confirmed")] +
                 [str(r["expected_output"]["chain_append_count"])] + [str(r["expected_output"].get(key, False)).lower() for key in ("diagnostics_frozen", "diagnostics_preserved", "terminal", "transitions_allowed")]) for r in datasets["containment"]])
    array("halt", "struct HaltRow { const char* id; const char* state_class; const char* transition; bool terminal; bool transitions_allowed; const char* failure; const char* mode; std::size_t append_count; bool preserved; bool frozen; };", [
        ",".join([q(r["vector_id"]), q(r["input"].get("initial_mode", r["input"].get("state_class", ""))), q(r["input"].get("attempted_transition", "")), str(r["expected_output"]["terminal"]).lower(), str(r["expected_output"]["transitions_allowed"]).lower(), q(r["expected_output"].get("expected_failure", "")), q(r["expected_output"].get("mode", "")), str(r["expected_output"].get("chain_append_count", 0)), str(r["expected_output"].get("diagnostics_preserved", False)).lower(), str(r["expected_output"].get("diagnostics_frozen", False)).lower()]) for r in datasets["safe-halt"]])
    array("axioms", "struct AxiomRow { const char* id; const char* evidence; const char* result; };", [
        ",".join(q(v) for v in [r["vector_id"], r["input"]["evidence"], r["expected_output"]["axiom_result"]]) for r in datasets["axioms"]])
    array("plans", "struct PlanRow { const char* id; const char* source; const char* transition; bool registry_changed; const char* profile; bool hash_stable; bool hash_changes; const char* failure; };", [
        ",".join([q(r["vector_id"]), q(r["input"].get("source", "")), q(r["input"].get("transition_id", "")), str(r["input"].get("registry_version_changed", False)).lower(), q(r["input"].get("profile_id", "")), str(r["expected_output"].get("plan_hash_stable", False)).lower(), str(r["expected_output"].get("plan_hash_changes", False)).lower(), q(r["expected_output"].get("expected_failure", ""))]) for r in datasets["generation-plans"]])
    array("emission", "struct EmissionRow { const char* id; bool complete; bool traceable; const char* failure; };", [
        ",".join([q(r["vector_id"]), str(r["input"]["plan_complete"]).lower(), str(r["expected_output"].get("every_output_has_plan_reference", False)).lower(), q(r["expected_output"].get("expected_failure", ""))]) for r in datasets["materialization-boundary"]])
    for dataset in ("containment", "safe-halt", "axioms", "generation-plans", "materialization-boundary"):
        # Retain every original scenario field, not just an inferred success bit.
        output.append(f"inline const char* {dataset.replace('-', '_')}_json[]{{")
        output.extend(q(json.dumps(r, sort_keys=True, separators=(",", ":"))) + "," for r in datasets[dataset])
        output.append("};")
    payload = default_plan_payload(datasets)
    output.append("inline const char* default_plan_payload = " + q(payload) + ";")
    output.append("inline constexpr const char* default_plan_sha256 = " + q(hashlib.sha256(payload.encode()).hexdigest()) + ";")
    output.append("} // namespace canonical_fixture")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fixture_bytes = ("\n".join(output) + "\n").encode("utf-8")
    # All configurations share these exact immutable inputs. Preserve the
    # timestamp when bytes match so a later configuration does not invalidate
    # completed earlier outputs during the direct solution verification.
    if not args.output.is_file() or args.output.read_bytes() != fixture_bytes:
        args.output.write_bytes(fixture_bytes)
    args.output.with_suffix(".provenance.json").write_text(json.dumps({
        "schemaVersion": "1.0", "sourceCommit": PIN,
        "resourceIntegrity": integrity, "counts": {k: len(v) for k, v in datasets.items()},
        "fixtureSha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
        "expectedResultSource": "Pinned corpus fields; native implementation is never invoked by generator",
        "nativePlanGolden": {"identityFormat": "aps-windows-plan-2",
                             "source": "Explicit compatibility request, downstream wire fixture, pinned canonical realm and topology",
                             "payloadSha256": hashlib.sha256(payload.encode()).hexdigest()},
        "expectedFieldInventory": {key: sorted(value) for key, value in expected_fields.items()},
    }, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "pass", "counts": {k: len(v) for k, v in datasets.items()}}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
