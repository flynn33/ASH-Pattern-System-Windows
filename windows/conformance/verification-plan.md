# Verification Plan — APS Windows

This plan maps Windows C++ tests to APS module behavior and platform release gates. Canonical APS main remains the semantic authority.

## Local Test Executables

| Executable | Source test file | Coverage |
|---|---|---|
| `ash-test-statemodel` | `windows/tests/StateModel.test.cpp` | 512-state structural coverage, normalization identity, system-state/recovery mapping |
| `ash-test-transitionregistry` | `windows/tests/TransitionRegistry.test.cpp` | 16 codewords, closure, weights, b8 invariance, deterministic XOR transitions, all 8,192 state/codeword pairs, noncanonical rejection |
| `ash-test-diagnostics` | `windows/tests/Diagnostics.test.cpp` | Diagnostic builder requirements, rule ID validation, chain integrity |
| `ash-test-recoveryengine` | `windows/tests/RecoveryEngine.test.cpp` | Recovery diagnostics, empty fallback escalation, containment, safe halt |
| `ash-test-completionmodules` | `windows/tests/CompletionModules.test.cpp` | Realm/orbit encoding, topology, axiom tri-state evaluation, generation planning, emission traceability |

## Requirement Coverage

| Requirement | Evidence |
|---|---|
| 512 well-formed states remain realm-preserving under structural normalization | `normalize_all_well_formed_states_is_identity_with_resolved_diagnostic` |
| Canonical b0-most-significant integer/signature mapping | `from_int_uses_canonical_msb_bit_order` |
| Exact 16-member codeword set | `codeword_set_has_exactly_16_members`, `codeword_set_is_closed_under_xor`, `codeword_set_weights_are_in_0_4_8` |
| Full-vector XOR transitions and involution | `apply_transition_operates_on_full_9_bits`, `apply_transition_is_involutive` |
| Realm and orbit identity format | `realm_encoder_returns_canonical_identity`, `realm_encoder_matches_canonical_orbit_partition_for_all_512_states` |
| 8,192 state/codeword transformations | `apply_transition_matches_all_8192_canonical_pairs` |
| Ordered ternary topology | `topology_generator_builds_ordered_ternary_tree`, `topology_generator_rejects_depth_over_limit_before_output` |
| Evidence-based axiom results | `axiom_evaluator_reports_pass_fail_and_indeterminate`, `axiom_evaluator_aggregates_required_axioms` |
| Side-effect-free inspectable planning | `generation_planner_produces_inspectable_hashed_plan` |
| Plan-bound emission traceability | `artifact_emitter_traces_each_artifact_to_plan_element`, `artifact_emitter_refuses_incomplete_plan` |
| Diagnostic completeness | `Diagnostics.test.cpp`, recovery and transition diagnostic tests |

## Release Verification Command

On a supported Windows host:

```powershell
cd windows\build
.\build.ps1 -Configuration Release -Platform x64 -Test
.\build.ps1 -Configuration Release -Platform ARM64 -Test
```

The local macOS execution environment can compile and run the same C++ test sources with `clang++` for behavioral validation, but that does not replace the required MSVC/MSBuild release matrix.

## Remaining Release Gates

- MSVC/MSBuild Release x64 and ARM64 builds.
- Windows static analysis where configured.
- UI/accessibility verification after a product UI exists.
- Signed MSIX or owner-approved Microsoft-supported distribution artifact.
- Clean install, launch, upgrade, repair, and uninstall validation on supported Windows systems.
- SBOM, signing, checksums, and release evidence.
