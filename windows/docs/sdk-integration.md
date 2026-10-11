# Native SDK integration

SDK product version `1.0.0-rc.1` comes from root `VERSION`. It adopts canonical APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`; engineering policy `0.7.0` and realization-profile catalog `0.7.2` are separate immutable bindings. Package identity and semantic registry `aps-windows-1.0` keep their own meanings. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes.

**Use the named SDK release assets. GitHub's automatically generated Source code ZIP/tarball is not the SDK. Do not import this repository wholesale into an application.** Download the architecture-specific `ash-pattern-system-windows-sdk-<version>-x64-release.zip` or `...-arm64-release.zip` and its published checksums. Extract its one versioned root into a directory you own. Administrative installation, registry registration, and Program Files are unnecessary.

The dependency boundary is the curated `include/ash` headers and `lib/<architecture>/Release/ash-core.lib`. Keep `.git`, implementation sources, internal tests, canonical corpora, policy, migration tools and whole repositories outside your application dependency graph. The installed SDK needs no runtime Python, Git, authority checkout or network service.

Set an explicit absolute `ASHSDKRoot` ending in a directory separator when your project lives outside the installed sample. Import the property sheet after your native compile settings and the targets after `Microsoft.Cpp.targets`:

```xml
<ItemDefinitionGroup>
  <ClCompile>
    <RuntimeLibrary>MultiThreaded</RuntimeLibrary>
    <LanguageStandard>stdcpp20</LanguageStandard>
    <WholeProgramOptimization>false</WholeProgramOptimization>
    <PreprocessorDefinitions>NDEBUG;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  </ClCompile>
</ItemDefinitionGroup>
<Import Project="$(ASHSDKRoot)msbuild\ASHPatternSystem.props" />
<!-- Declare only your application's own sources here. -->
<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
<Import Project="$(ASHSDKRoot)msbuild\ASHPatternSystem.targets" />
```

Use Release, C++20, Microsoft v143 compiler 19.44 or later, `/MT` and `_ITERATOR_DEBUG_LEVEL=0`, with the architecture matching the asset. The verification evidence records the exact linker used; compatibility with earlier linkers is unverified. Per-file incompatible settings are rejected. Existing application flags are preserved; integration never silently turns `/MD` into `/MT`. Do not suppress mismatch diagnostics or use `/NODEFAULTLIB` to conceal a conflict. Debug SDK assets are not distributed; source contributor Debug builds use `/MTd` independently.

The installed sample demonstrates state/transition operations, valid diagnostic ancestry, rejected invalid correction, deterministic side-effect-free planning, descriptor emission, and an explicit paired support export. Build its native solution and pass a chosen output directory to the executable. ARM64 build/link does not establish ARM64 execution.

Semantic objects are caller-owned. `RecoveryEngine` borrows `StateModel` and `TransitionRegistry`, which must outlive it. Mutable semantic objects require caller serialization. A diagnostic leaf owns its parent ancestry; its root identity is weak. Retain the returned shared owner, use `.lock()` for root access, and reject expired references. Builders throw `std::logic_error` for invalid required fields or ancestry. Contained operations preserve state and append a denial to the audit chain. Safe halt is terminal; rejected calls return the original frozen diagnostic without adding records.

`StateModel` structural classification alone cannot certify application health. Supply validated `OperationalContext` for operational assessment and caller-declared domain-safe fallback instances. No universally safe fallback state is invented. `GenerationPlanner` returns an inspectable in-memory plan; `ArtifactEmitter` returns plan-traceable descriptors and does not create filesystem projects. Support diagnostics have their own documented session concurrency/privacy contract in [support-diagnostics.md](support-diagnostics.md).

A successful policy replacement can reassess as `STABLE`, `CORRECTABLE` or `UNSTABLE`. A replacement in either latter class still requires its declared correction or target-resolution action; a resolved fallback result does not certify a stable operational state.

Use `AxiomEvaluator::diagnose_axioms(AxiomSubject, AxiomEvaluationProfile)` to evaluate the five canonical subject predicates. Supply the measured relation count, compressed/raw measures, scale count, erasure cost, and both self-reference flags. Missing or nonfinite measurements remain `INDETERMINATE`; a known failure takes precedence over missing evidence. `overall_pass` is empty when no Boolean conclusion is justified. The profile identifier labels the fixed canonical predicates; it is not a threshold registry. The older `evaluate_axiom` and `evaluate_axioms` helpers preserve caller-supplied verdicts and do not measure a subject.

## Planning and descriptor emission

Use `GenerationPlanner::generate_plan(const GenerationRequest&, const GenerationPlanningInputs&)` for an explicit request. `GenerationRequest` has exactly these seven fields; target provenance and optional measured axiom evidence belong to the separate inputs object.

| Request field | Native meaning |
|---|---|
| `project_name` | Nonempty ASCII letters, digits, dot, underscore or hyphen |
| `seed_state` | Full `Bit9State`; structural normalization preserves every coordinate |
| `transition_sequence` | Ordered `APS-TRANSITION-CW-00` through `APS-TRANSITION-CW-15` references, applied by full-state XOR |
| `topology_depth` | Signed depth from 0 through 6; all structural nodes and leaves are retained |
| `profile_id` | `aps-windows-descriptor-1.0` |
| `naming_policy_id` | `aps-windows-realm-names-1.0` or `aps-windows-project-names-1.0` |
| `emission_target_kind` | `descriptor` |

These profile and naming IDs belong to the documented native registry. They are downstream choices, not canonical `DEFAULT` or `BASELINE` names. `GenerationPlanningInputs::target` carries `TargetMetadata`: target class `desktop`, `mobile` or `service`, runtime hint, matching native profile ID and nonblank caller `registry_version` provenance. Its default target is desktop with empty runtime hint and registry label `aps-windows-1.0`. Inputs also carry optional `AxiomSubject` measurements and an `AxiomEvaluationProfile` label; neither label changes the five fixed predicates. Unknown references, malformed project/identity labels, invalid transitions or unsupported depth return a blocked diagnostic and an empty plan.

```cpp
#include <ash/GenerationPlanner.hpp>

ash::PlanResult make_example_plan() {
    const ash::GenerationRequest request{
        "example", ash::from_int(0), {"APS-TRANSITION-CW-01"}, 1,
        "aps-windows-descriptor-1.0", "aps-windows-realm-names-1.0", "descriptor"
    };
    ash::GenerationPlanningInputs inputs;
    inputs.target = {"desktop", "example-runtime", "aps-windows-descriptor-1.0",
                     "my-application-registry-1"};
    // No measured subject is supplied; all five axiom results remain INDETERMINATE.
    return ash::GenerationPlanner{}.generate_plan(request, inputs);
}
```

A resolved plan stores the request and inputs, normalized/transformed states, source/destination realm and orbit identities, complete ordered topology and leaf indices, every node's typed role, all five explanatory axiom evaluations and their aggregate, warnings, declared artifacts and registry metadata. Missing measurements remain `INDETERMINATE`; known axiom failures remain `FAIL` with warnings. A resolved planning diagnostic does not certify that the subject passes its axioms. Realm naming uses the destination realm; project naming additionally prefixes descriptor names with the project. Both supported policies declare state-summary and diagnostic-chain-export descriptors with IDs, kind, name, summary and references to all node roles.

`generation_plan_identity_payload(plan)` exposes the native `aps-windows-plan-2` bytes. Each field uses `name:byte_count:value` followed by LF; ordered collections bind their counts, optional evidence binds presence, and double measurements bind exact IEEE-754 bits. The lowercase SHA256 plan hash covers every request, target/provenance, state/realm, topology/role, full canonical axiom record, artifact, warning and registry metadata field, excluding `plan_hash` itself. This replaces the earlier incomplete plan payload, so old hashes must be regenerated with their old provenance retained. SDK product version does not replace any registry label.

The legacy `(Bit9State, TargetMetadata)` overload delegates with project `ash-plan`, no transitions, depth zero, realm naming, descriptor output and no measured subject. It preserves those source defaults while producing the complete current plan and new hash format. `ArtifactEmitter::materialize_plan(plan, config)` audits the stored plan using fixed semantic modules and its hash before output; it blocks incomplete, inconsistent or altered plans, including inconsistencies hidden by recomputing the hash. An empty `TargetConfiguration::target_class` uses the plan's class; a conflicting nonempty class is blocked. It copies every declared descriptor's ID, kind, name, summary and role references, preserving its plan-element reference. It has no planner callback or filesystem writes; `output_directory` is reserved caller data. Public plan/artifact layout changes require a clean consumer rebuild/relink.

## Diagnostic and safety export

Diagnostic records contain meaningful notes, a `details` object, and nullable `occurrence_metadata`. Use the builder's typed object/array values for structured facts; permitted values are null, Boolean, signed 64-bit integers, valid UTF-8 strings, arrays and objects. `Diagnostics::serialize_record` emits the complete inner canonical record; `serialize_transport` includes its schema/product wrapper. The nine-field semantic identity includes notes and details. Timestamps, host/process metadata, parent/root placement and sequence do not change semantic identity. Validate both the record and its complete chain before relying on serialized input.

`RecoveryEngine::containment_record()` and `safe_halt_record()` return optional typed projections. Each includes the original diagnostic, action diagnostic, complete causal chain and typed trigger. Containment adds restrictions, UTC `containment_time`, awaiting-resolution status and resolution; halt adds UTC `halt_time` and terminal/frozen flags. Serialize either with the corresponding `Diagnostics::serialize_transport` overload and your generated product version. The engine's UTC entry/adoption timestamps are outside diagnostic identity.

```cpp
#include <ash/Diagnostics.hpp>
#include <ash/RecoveryEngine.hpp>
#include <ash/Version.hpp>
#include <ostream>

// The application supplies its own output stream and handles export exceptions.
void write_safety_evidence(const ash::RecoveryEngine& engine, std::ostream& output) {
    ash::Diagnostics diagnostics;
    if (const auto record = engine.containment_record())
        output << diagnostics.serialize_transport(*record, ash::sdk_version) << '\n';
    if (const auto record = engine.safe_halt_record())
        output << diagnostics.serialize_transport(*record, ash::sdk_version) << '\n';
    for (const auto& prior_leaf : engine.retained_diagnostic_chains())
        output << diagnostics.serialize_chain(prior_leaf) << '\n';
}
```

A contained operation that was prevented appends blocked audit evidence and preserves state. `breach_containment()` reports a caller-confirmed boundary violation and enters terminal `SAFE_HALT`; later engine calls return the same frozen terminal diagnostic. Importing a valid external terminal chain retains that chain as the active frozen history and preserves any prior independent history. Its safety projection uses `POLICY_HALT_REQUEST` for this engine's local enforcement and records local adoption time, without inventing an external trigger/time. Invalid imported chains or terminal actions throw before adoption changes the engine. Export independent chains separately; `serialize_chain` does not splice them into the active causal chain.

Install by extraction and hash verification. For upgrade, extract the replacement SDK side-by-side, update `ASHSDKRoot`, and clean rebuild/relink. Preserve your application's own source and outputs. Repair by re-extracting the same verified release into a fresh directory and switching the reference. Remove SDK references first, then delete only the SDK's versioned folder; unrelated files require no changes. The release qualification exercises these procedures in disposable acceptance workspaces.

## Authority bindings

The SDK uses these separate immutable sources. Installed consumers require no authority checkout or policy runtime; the release manifest records the bindings for provenance.

| Source role | Version | Commit |
|---|---|---|
| Canonical APS | `1.0.0-rc.3` | `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8` |
| Engineering policy | `0.7.0` | `8dd6e75a06336491b249fac3577db75458db8c13` |
| Realization-profile catalog | `0.7.2` | `82049af6b10d4dc21d24fb9c67f1b93155e4b7e3` |

Producer/consumer canonical contracts use `rfd.aps.canonical.1.0.0-rc.3`, SHA256 `27108f8bc77d7585e0b2877a3f44052ac4e80b0bfcefadbe589d65a8a5ccbd40`. The native Windows contract remains `rfd.aps.windows.b1c05f1`, SHA256 `8f27f926bc0cf8a8da0e7424c4e99093614279cd72f0dab9aba246ae10bf63f1`, retaining the original baseline identity rather than naming the current SDK commit. Historical rc.1 contradiction inputs and evidence are preserved separately; current expectations come from the corrected rc.3 pin.
