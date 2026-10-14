# ASH Pattern System — Windows SDK

The Windows edition provides the ASH Pattern System semantic core as a native C++20 static library, `ash-core.lib`. SDK product version **1.0.0-rc.1** uses separate x64 and ARM64 Release SDKs. A named asset is qualified only by its source identity and release evidence; historical binaries and test totals do not qualify it.

The public semantic modules are StateModel, TransitionRegistry, Diagnostics, RecoveryEngine, RealmEncoder, TopologyGenerator, AxiomEvaluator, GenerationPlanner and ArtifactEmitter. Separate caller-owned support diagnostics provide bounded, explicitly exported JSON/Markdown evidence. ArtifactEmitter returns descriptors; an application owns filesystem materialization.

Planning accepts an explicit seven-field request plus separate target/optional axiom inputs and retains complete topology, roles and all five explanatory axiom results. Its `aps-windows-plan-2` identity binds request, evidence and registry fields; old plan hashes must be regenerated. [Planning integration](windows/docs/sdk-integration.md#planning-and-descriptor-emission) documents the native references, legacy defaults and descriptor audit.

**Use the named SDK release assets. GitHub's automatically generated Source code ZIP/tarball is not the SDK. Do not import this repository wholesale into an application.**

Use the named architecture SDK ZIP from [Releases](https://github.com/flynn33/ASH-Pattern-System-Windows/releases), after mandatory qualification passes. Verify external SHA256SUMS.txt and internal checksums before use. The curated SDK includes public headers, one Release library, relocatable MSBuild integration, an authored standalone sample, license, component inventory, manifest and consumer documentation.

Consumers require MSVC v143 19.44 or later, C++20, Release `/MT`, iterator debug level zero and a matching target architecture. The library uses `/Z7` information embedded in objects and disables `/GL`. Consumers must rebuild after the ownership/API changes described in [SDK migration](windows/docs/sdk-migration.md). C++ ABI compatibility with arbitrary compilers, Debug CRTs or earlier linkers is not established.

Start with [SDK integration](windows/docs/sdk-integration.md), [support diagnostics](windows/docs/support-diagnostics.md), [compatibility](windows/docs/sdk-compatibility.md) or the [standalone sample](windows/samples/StandaloneConsumer/README.md). [Build and verification](windows/docs/build-run-instructions.md) covers source builds and offline qualification. The [Wiki](https://github.com/flynn33/ASH-Pattern-System-Windows/wiki) provides installation and public-boundary guidance.

The adopted canonical authority is APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`. Engineering policy remains `0.7.0`; the separately pinned realization-profile catalog is `0.7.2`. [Canonical deference](windows/CANONICAL-DEFERENCE.md) records all three immutable bindings and the retained Windows baseline profile. SDK product, canonical, registry, corpus and export-schema versions have separate meanings. The corrected authority closes the earlier diagnostic-notes and containment-breach contradictions; their original evidence remains historical. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes. Failed mandatory core conformance still means **NON-CONFORMANT** and blocks publication; [acceptance judgment](windows/conformance/acceptance-judgment.md) states the current gates.

`RecoveryEngine` exposes typed containment and safe-halt projections with complete diagnostic ancestry, explicit triggers and local UTC entry/adoption times. `Diagnostics` serializes those projections and exports each retained independent chain separately. A prevented contained operation retains a blocked audit record; a caller-confirmed breach enters frozen terminal halt. See [SDK integration](windows/docs/sdk-integration.md) for the APIs and imported-evidence rules.

The intended deployment floor is Windows 10 1809 and Windows 11. A Windows 11 x64 run does not prove that full range. ARM64 cross-build/link evidence is separate from runtime execution. Physical-device qualification remains an owner phase. An unsigned static-library prerelease is permitted after mandatory gates pass; SHA256 establishes integrity, not a publisher signature.

Historical records under `completion-evidence/` remain history. Generated packages and build outputs are no longer tracked or used as current release inputs.

Copyright (c) 2026 James Daley, Raven Forge Software. Licensed under the [MIT License](LICENSE). Outside contributions are not accepted; see [CONTRIBUTING.md](CONTRIBUTING.md).
