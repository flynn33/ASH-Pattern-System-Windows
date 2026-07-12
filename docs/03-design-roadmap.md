# Design Roadmap

## Goal

Complete the Windows edition as a native, testable, releasable implementation of the ASH Pattern System for supported Windows desktop targets.

## Current State

The repository currently contains:

- C++20 implementations for all nine APS semantic modules;
- Visual Studio/MSBuild project files;
- PowerShell build and test wrappers;
- C++ module tests;
- Windows conformance documentation;
- release-readiness evidence under `completion-evidence/`;
- an acceptance judgment of `NOT_SHIPPABLE`.

## Completed Platform Work

### Semantic Core

Implemented modules:

- `StateModel`
- `RecoveryEngine`
- `TransitionRegistry`
- `Diagnostics`
- `RealmEncoder`
- `TopologyGenerator`
- `AxiomEvaluator`
- `GenerationPlanner`
- `ArtifactEmitter`

### Windows Build Assets

The Windows build uses:

- Visual Studio solution and project files;
- MSBuild;
- MSVC configured for C++20;
- PowerShell wrappers for build and test execution.

### Conformance Documentation

Windows conformance coverage is documented in:

- `windows/conformance/module-mapping.md`
- `windows/conformance/verification-plan.md`
- `windows/conformance/diagnostics-conformance-plan.md`
- `windows/conformance/materialization-boundary-plan.md`
- `windows/conformance/deviation-log.md`
- `windows/conformance/acceptance-judgment.md`

## Remaining Work

### Release Build Matrix

Run and archive evidence for:

- Release x64 build and test;
- Release ARM64 build and test;
- any configured Windows static-analysis checks.

### Product Packaging

Complete a signed Windows distribution path, including:

- packaging model decision;
- production signing;
- signature verification;
- checksums;
- release manifest.

### Install And Upgrade Validation

Verify clean install, launch, upgrade, repair, and uninstall behavior on supported Windows systems.

### Product UI And Accessibility

The current repository state does not include a product UI. Any UI added later must pass the Windows accessibility matrix before release.

### Operational Context And Fallback Policy

Complete a typed operational context model and fallback policy instance model before final release acceptance.

### Final Release Judgment

The repository may not move beyond `NOT_SHIPPABLE` until all release blockers are resolved and owner-controlled release approvals are complete.
