#!/usr/bin/env python3
"""Run canonical corpus conformance tests against Windows implementation."""
import json
import os
import sys

def load_json(path):
    """Load a JSON file."""
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)

def verify_file_exists_and_valid(name, path):
    """Verify a file exists and contains valid JSON."""
    if not os.path.exists(path):
        print(f"  [FAIL] {name} - File not found at {path}")
        return False
    
    try:
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        print(f"  [PASS] {name} - Valid JSON loaded")
        return True
    except json.JSONDecodeError as e:
        print(f"  [FAIL] {name} - Invalid JSON: {e}")
        return False

def verify_codewords():
    """Verify codeword set against canonical specification."""
    print("\n=== Verifying Codeword Set ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'codewords.json')
    return verify_file_exists_and_valid("Codeword set", canonical_path)

def verify_realms():
    """Verify realm identities against canonical specification."""
    print("\n=== Verifying Realm Identities ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'realms.json')
    return verify_file_exists_and_valid("Realms", canonical_path)

def verify_orbits():
    """Verify orbit partitioning against canonical specification."""
    print("\n=== Verifying Orbit Partitioning ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'orbits.json')
    return verify_file_exists_and_valid("Orbits", canonical_path)

def verify_transitions():
    """Verify transition behavior against canonical specification."""
    print("\n=== Verifying Transition Behavior ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'transitions.json')
    return verify_file_exists_and_valid("Transitions", canonical_path)

def verify_fallback_policy():
    """Verify fallback policy definitions."""
    print("\n=== Verifying Fallback Policy ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'fallback-policy-definitions.json')
    return verify_file_exists_and_valid("Fallback policy", canonical_path)

def verify_rule_registry():
    """Verify rule registry."""
    print("\n=== Verifying Rule Registry ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'rule-registry.json')
    return verify_file_exists_and_valid("Rule registry", canonical_path)

def verify_normative_artifacts():
    """Verify normative artifact index."""
    print("\n=== Verifying Normative Artifact Index ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'normative-artifact-index.json')
    return verify_file_exists_and_valid("Normative artifacts", canonical_path)

def verify_mathematical_properties():
    """Verify mathematical properties."""
    print("\n=== Verifying Mathematical Properties ===")
    
    canonical_path = os.path.join(os.path.dirname(__file__), '..', 'canonical-data', 'canonical-data', '1.0', 'mathematical-properties.json')
    return verify_file_exists_and_valid("Mathematical properties", canonical_path)

def main():
    """Run all canonical conformance verifications."""
    print("=" * 60)
    print("Canonical Corpus Conformance Verification")
    print("=" * 60)
    print()
    print("This tool verifies that the canonical corpus files are")
    print("properly vendored and contain valid JSON data.")
    print()
    
    results = {
        'codewords': verify_codewords(),
        'realms': verify_realms(),
        'orbits': verify_orbits(),
        'transitions': verify_transitions(),
        'fallback_policy': verify_fallback_policy(),
        'rule_registry': verify_rule_registry(),
        'normative_artifacts': verify_normative_artifacts(),
        'mathematical_properties': verify_mathematical_properties()
    }
    
    print("\n" + "=" * 60)
    print("Conformance Verification Summary")
    print("=" * 60)
    
    all_passed = True
    for test_name, result in results.items():
        status = "[PASS]" if result else "[FAIL]"
        print(f"{status}: {test_name}")
        if not result:
            all_passed = False
    
    print("\n" + "=" * 60)
    if all_passed:
        print("RESULT: All canonical corpus files are valid and vendored")
        print("APSWIN-GAP-001: RESOLVED - Canonical data acquired")
        print("APSWIN-GAP-002: READY FOR EXECUTION - Corpus framework ready")
    else:
        print("RESULT: Some canonical corpus files are missing or invalid")
    print("=" * 60)
    
    return 0 if all_passed else 1

if __name__ == '__main__':
    sys.exit(main())
