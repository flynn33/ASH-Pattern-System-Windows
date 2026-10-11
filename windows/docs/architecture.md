# Windows SDK architecture

The native product is `ash-core`, producing `ash-core.lib`. SDK `1.0.0-rc.1` uses separate x64 and ARM64 Release assets. Its current canonical binding is APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`. Engineering policy and realization-profile catalog have separate immutable bindings in [canonical deference](../CANONICAL-DEFERENCE.md). Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes.

## Target and ownership

The intended deployment range is Windows 10 1809+ and Windows 11 on x64/ARM64. The assigned x64 host does not establish ARM64 runtime coverage or the full physical deployment floor. ARM64 cross-build/link and machine inspection are distinct from execution; unverified OS/device coverage remains an owner phase.

The deliverable is a C++20 static library with an authored console consumer example. A GUI, installer, hosted service and accessibility-qualified application UI are outside this SDK scope. The semantic core performs no filesystem materialization: planning returns values and emission returns descriptors. Separate support exports write only to an explicitly supplied destination.

Applications own semantic module instances. `RecoveryEngine` borrows `StateModel` and `TransitionRegistry`, which must outlive it. Mutable semantic objects and public diagnostic fields require caller serialization. A diagnostic leaf owns its parent ancestry; its root is a weak observer. Typed safety projections hold owning const references to their complete chains, extending lifetime without promising immutability against other mutable owners. Public record layout changes require a clean consumer rebuild/relink.

## Module flow

| Responsibility | Native module |
|---|---|
| Structural validation and explicit-context operational assessment | `StateModel` |
| Canonical codeword lookup and XOR transition | `TransitionRegistry` |
| Correction, supplied-policy fallback, restricted containment and frozen halt | `RecoveryEngine` |
| Complete record identity, causal-chain validation and canonical transport serialization | `Diagnostics` |
| Realm/orbit and topology evidence | `RealmEncoder`, `TopologyGenerator` |
| Five canonical subject predicates | `AxiomEvaluator` |
| Inspectable pure plan and deterministic identity | `GenerationPlanner` |
| Plan-traceable in-memory descriptors | `ArtifactEmitter` |

`GenerationPlanner` accepts the seven-field `GenerationRequest` and separate `GenerationPlanningInputs` for target provenance and optional axiom measurements. The complete `GenerationPlan` retains normalized/transformed state and realms, every requested structural topology node/leaf and typed role, all five explanatory axiom results, declared artifacts, warnings and registry metadata. Missing/failing measurements remain visible; planning success alone is not an axiom pass. No callable or I/O target is accepted. The length-prefixed `aps-windows-plan-2` identity binds that complete request/evidence/registry payload and changes earlier hashes. [Planning integration](sdk-integration.md#planning-and-descriptor-emission) lists supported references and legacy defaults.

`ArtifactEmitter` consumes the stored plan plus target configuration, audits complete evidence and hash through fixed semantic modules, and copies every declared descriptor field with its plan-element reference. It rejects inconsistent plans even after rehashing. There is no planner callback or filesystem write; the application owns later materialization. [Module mapping](../conformance/module-mapping.md) gives source and test paths. Public plan/artifact layout changes require consumer rebuilds.

## Diagnostics and runtime safety

`DiagnosticEnvelope.hpp` defines complete canonical records, recursive owning JSON values and the fluent builder. Identity hashes the nine semantic fields under compact sorted UTF-8 JSON. Meaningful notes and a details object are required; occurrence metadata may be null. Schema, parent/root placement, sequence and occurrence metadata are outside the semantic digest. `Diagnostics` rejects malformed/cyclic ancestry, stale identity and invalid vocabulary and validates explicit causal phases rather than enum ordinals.

`RecoveryEngine::containment_record()` and `safe_halt_record()` expose original/action diagnostics, full ordered ancestry, typed triggers and local UTC entry/adoption times. A prevented contained operation appends blocked audit evidence without state mutation. A caller-confirmed boundary breach enters terminal SAFE_HALT; later calls return the same frozen terminal diagnostic. Valid external terminal evidence is locally enforced as `POLICY_HALT_REQUEST` with local adoption time, without inventing an external trigger/time. Invalid imported graph/action evidence rejects before adoption. Independent prior histories remain accessible and exportable separately through `retained_diagnostic_chains()` and `Diagnostics::serialize_chain()`.

The support responsibility has its own `Session` and immutable `Snapshot`, distinct from canonical safety transports. Debug-only source contains actual rich state/duration/context/error-cause collection; Release-only source contains minimal Fault/Health collection. Shared capture retains common identity/privacy, bounds, health and storage behavior. No runtime setting activates the excluded rich collector. Capture/snapshot/close may run concurrently, but callers must join operations before session destruction.

Support retention is bounded to 4096 events, 4 MiB aggregate accounting and 16 KiB per event, with an essential reserve; each export file is limited to 8 MiB. Local JSON/Markdown exports use the same snapshot and publish a completion marker last. Development evidence is caller-instrumented and does not provide ambient collection, arbitrary memory/stack capture or automatic crash flush. See [support diagnostics](support-diagnostics.md).

## Toolchain and build boundary

Public Release consumers require C++20, Microsoft v143 compiler 19.44 or later, `/MT`, iterator debug level zero and the matching architecture. Producer Debug verification uses `/MTd`; Debug SDK archives are not public assets. Object debug information uses `/Z7` and `/GL` is disabled. Exact produced/consumed tools and evaluated settings belong to artifact-bound qualification; earlier linker compatibility is unverified.

The solution contains the core library and seven test executables covering semantic modules, complete canonical comparisons and support diagnostics. Their required inventory is independent of a historical pass total. [Build instructions](build-run-instructions.md) own the supported source-build and qualification procedures.

Diagnostic records and their validation may allocate. `validate_envelope` and `validate_chain` return false when allocation fails; serialization and snapshot creation may throw. There is no measured universal allocation-free, startup or throughput guarantee. The support tests measure a synthetic capture loop, snapshot and local export on the assigned host, with method and limits recorded in evidence. Core persistence and migration services are outside this SDK.
