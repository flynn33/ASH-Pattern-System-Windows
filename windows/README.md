# Windows native implementation

The product is a C++20 static SDK, `ash-core.lib`, built with Visual Studio 2022/MSBuild v143 for x64 and ARM64. SDK version `1.0.0-rc.1` uses Release `/MT` libraries, subject to the exact released `qualification.json`. Debug `/MTd` builds include a separate rich support collector; the Release source selection compiles only essential support collection.

Public semantic headers are under `src/include/ash`. Consumers receive only the explicit payload in `packaging/sdk/sdk-public-files.json`, a generated Version header and their architecture library. Native implementations, helpers, tests, canonical data, governance and release tooling remain outside the installed SDK boundary.

The current semantic authority is APS `1.0.0-rc.3` at `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8`, with engineering policy `0.7.0` and realization-profile catalog `0.7.2` bound separately. The earlier rc.1 contradictions are historical; adopting the corrected pin does not establish final SDK release qualification. [Canonical deference](CANONICAL-DEFERENCE.md) gives exact commits, profile identities and hashes.

Typed `RecoveryEngine::containment_record()` and `safe_halt_record()` projections expose the original/action diagnostics, complete causal chain, trigger and local UTC entry/adoption time. `retained_diagnostic_chains()` preserves earlier independent histories; `Diagnostics::serialize_chain()` exports each without inventing ancestry. Public record layout changes require a clean consumer rebuild/relink.

The explicit `GenerationRequest` has seven fields; separate planning inputs carry target provenance and optional axiom measurements. Plans retain complete topology/roles, all five explanatory axiom results and declared artifacts. `aps-windows-plan-2` replaces earlier incomplete hashes, while the legacy overload retains documented source defaults. The emitter audits complete stored evidence and copies declared descriptors without file writes. See [planning integration](docs/sdk-integration.md#planning-and-descriptor-emission); plan/artifact layout changes also require consumer rebuilds.

| Use | Documentation |
|---|---|
| Install and link | [SDK integration](docs/sdk-integration.md) |
| Lifetime/API migration | [Migration](docs/sdk-migration.md) |
| Fault export and privacy | [Support diagnostics](docs/support-diagnostics.md) |
| Platform, CRT and ABI contract | [Compatibility](docs/sdk-compatibility.md) |
| Source build and qualification | [Build instructions](docs/build-run-instructions.md) |
| Authority and core gates | [Canonical deference](CANONICAL-DEFERENCE.md), [acceptance](conformance/acceptance-judgment.md) |
| ZIP and signing scope | [Packaging decision](docs/packaging-decision.md) |

Build from the product root with an explicitly validated offline resource cache:

```powershell
.\windows\build\build.ps1 -Configuration Release -Platform x64 -AuthorityCache $AuthorityCache
.\windows\tests\run_all.ps1 -BinDir .\windows\build\out\bin\x64-Release
```

Normal builds never download authority files. Test success requires all seven executables and nonempty successful case reports. Cross-build success does not imply execution. Intended Windows 10 1809+/Windows 11 deployment and physical-device qualification are separate from host-local evidence. Historical status records are not current qualification.
