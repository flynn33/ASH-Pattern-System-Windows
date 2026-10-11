# Native build and SDK qualification

Source verification uses Visual Studio 2022/MSBuild v143 with x64 and ARM64 C++ tools, a Windows SDK for both targets, Python 3, and PowerShell 5.1 or 7. These are development prerequisites; Python, Git and policy/canonical checkouts are not SDK runtime dependencies. The wrapper discovers actual installed tools and records resolved versions.

Acquire the pinned resources explicitly outside the source checkout:

```powershell
python .\windows\packaging\fetch-canonical-data.py --output $AuthorityCache --source-root $PinnedCanonicalCheckout --report $AcquisitionReport
python .\windows\packaging\run-canonical-conformance.py --authority-cache $AuthorityCache --report $ResourceReport
```

The source checkout must match the [immutable authority](../CANONICAL-DEFERENCE.md). Acquisition also supports explicitly selected pinned network retrieval when no checkout is supplied. Completed cache validation and later qualification are offline. Missing, incomplete or changed resources fail before fixture generation.

From the product root, select external build/evidence locations:

```powershell
.\windows\build\build.ps1 -Configuration Release -Platform x64 -BuildRoot $BuildRoot -AuthorityCache $AuthorityCache -Rebuild
.\windows\tests\run_all.ps1 -BinDir "$BuildRoot\bin\x64-Release" -ReportPath $TestReport
```

Repeat for Debug and ARM64. `-Clean` performs only Clean and rejects `-Test` or `-Rebuild`. `-Test` builds and executes the full seven-executable inventory. The runner inspects PE architecture and rejects missing, empty, failed or timed-out suites. On x64, ARM64 is cross-built and explicitly not executed.

Build metadata binds libraries/tests to commit, dirty state, exact source snapshot, generated identity, evaluated per-item settings and compiler/linker/librarian/SDK versions. Editing source during compilation invalidates completed-build metadata. Final packaging requires the exact clean built commit, passing native conformance and Release composition/dependency inspection.

The complete host-local entry point is:

```powershell
.\windows\packaging\verify-sdk-release.ps1 `
  -AuthorityCache $AuthorityCache -PolicyRoot $PinnedPolicyCheckout `
  -ProfileRoot $PinnedProfileCheckout -ProfileCommit $ProfileCommit -ContractRoot $ExternalContracts `
  -BuildRoot $BuildRoot -ConsumerRoot $ConsumerRoot -OutputRoot $ReleaseOutput `
  -EvidenceRoot $EvidenceRoot
```

Use separate external build, consumer, release and evidence roots. `PolicyRoot` is the exact clean engineering-policy authority. `ProfileRoot` and `ProfileCommit` select the separate clean realization-profile catalog. Both must agree with `windows/packaging/sdk-authority-bindings.json`; the catalog does not replace the engineering-policy pin. `ContractRoot` holds three issued realization contracts with actual accepted preimplementation reviews. The selected immutable source bindings govern this run; historical profiles and qualification reports remain historical evidence.

The first run omits `IndependentReviewReport`. It executes all substantive gates and retains exact metadata, library, generated identity, fixture and seven test-executable hashes for each of four builds. It also invokes `ash-windows.sln` directly from the product root with the same external build root, generated identity/fixtures, selected toolset and SDK. This incremental direct solution command must succeed and preserve every recorded output hash. The actual command, working directory and log hash are recorded in `directSolution`.

When every substantive gate passes, the first report still exits with failure, sets `readyForIndependentReview` to true and lists only `independentReview` in `incompleteMandatoryGates`. This is a reviewable candidate, and publication remains ineligible. Any other incomplete gate must be repaired and requalified before final review.

The independent reviewer inspects the clean source and actual candidate artifacts, then writes an external JSON review with these fields:

| Field | Required evidence |
|---|---|
| `reviewer`, `independent`, `implementationAuthored`, `humanApprovalClaimed` | Nonempty reviewer identity; respectively true, false and false for the three flags |
| `sourceCommit`, `sourceDirty`, `sourceFingerprint` | Exact candidate commit, false and complete source fingerprint |
| `canonicalCommit`, `policyCommit`, `profileSourceCommit`, `authorityBindings` | Exact values from the candidate qualification report, preserving separate engineering and catalog bindings |
| `qualificationEvidence` | Candidate report's `{path, sha256, size}`; its only incomplete mandatory gate may be independent review |
| `libraries` | Object mapping `x64` and `ARM64` to the actual qualified Release library SHA-256 values |
| `sdkAssets` | Name-sorted exact two-ZIP list, each with `name`, `sha256`, `size` |
| `conclusions` | All six named conclusions; accepted status, nonempty evidence narrative describing actual paths/commands, and nonempty `evidenceFiles` with verified `{path, sha256, size}` records |

Conclusion names are `sourceUnderstanding`, `instructionAccuracy`, `instructionAdherence`, `implementationConformance`, `dependencyImportProhibition` and `governanceConformance`. Governance may use `not_applicable` with concrete rationale and evidence. Each referenced evidence file must still have the reviewed bytes. These records document independent technical review and never claim human approval.

Complete qualification with the review and optional verified build reuse:

```powershell
.\windows\packaging\verify-sdk-release.ps1 `
  -AuthorityCache $AuthorityCache -PolicyRoot $PinnedPolicyCheckout `
  -ProfileRoot $PinnedProfileCheckout -ProfileCommit $ProfileCommit -ContractRoot $ExternalContracts `
  -BuildRoot $BuildRoot -ConsumerRoot $FreshConsumerRoot -OutputRoot $ReleaseOutput `
  -EvidenceRoot $EvidenceRoot -IndependentReviewReport $IndependentReviewReport `
  -CompletedBuildReport $FirstQualificationReport
```

`CompletedBuildReport` is optional. It verifies the prior source, authority bindings, successful original build commands/logs, direct solution proof, package prerequisites and every current completed output. Reused build steps are labeled `execution: reused`, retain the original executed command evidence and do not claim a rebuild. Integrity, regressions, direct solution invocation, x64 native tests, diagnostic-record validation, paired exports, archive composition, packaging, extraction, consumers, import negatives and delivery validation run again. A source change, stale binary/header/metadata/log or failed prior substantive gate rejects reuse. Omitting this option rebuilds all four configurations; the review still requires the current artifacts to match the previously reviewed ZIP and library identities.

Select a new empty `FreshConsumerRoot` for the second run. The consumer verifier preserves previous acceptance workspaces and rejects populated roots; retain the first consumer's evidence. Build and release-output roots may remain the same because their actual bytes are checked against the reviewed identities.

Each run retains commands, logs, exits and machine-readable `qualification-report.json`. Missing or stale review and failed native behavior leave qualification failed. The command performs no GitHub publication.

Independent diagnostic-record verification reads actual native output from both x64 configurations, validates the selected immutable schemas, recomputes nine-field SHA-256 identities and causal chains, and rejects deliberate corruptions. Consumer checks cover headers, positive links/execution, incompatible settings, import prohibitions and replacement/repair/removal. Publication separately requires an owner-authored annotated tag, downloaded-asset verification, actual wiki verification and local/remote synchronization.

Unsigned status is explicit. `/Z7` embeds debug information in archive objects; evidence discloses debug path metadata. [Compatibility](sdk-compatibility.md) separates OS intent from exercised environments.
