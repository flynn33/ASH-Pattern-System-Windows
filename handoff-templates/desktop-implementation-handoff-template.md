# Windows Desktop Implementation Checklist

## Target Class

Windows desktop implementation of the ASH Pattern System.

## Scope

This checklist applies to the active Windows edition in this repository. It covers native C++ implementation, Windows build tooling, desktop release packaging, and release evidence.

## Required Sections In This Repository

### 1. Target Environment

Document the specific Windows target:

- supported Windows versions;
- supported processor architectures;
- native runtime environment;
- UI framework status;
- filesystem access model;
- concurrency and threading model.

Active document: `windows/docs/architecture.md`.

### 2. Semantic-Module Mapping

Map each of the 9 APS semantic modules to concrete C++ modules.

Active document: `windows/conformance/module-mapping.md`.

### 3. Verification Inputs

Document how the Windows implementation verifies conformance:

- test framework and tooling;
- coverage of all 5 conformance categories;
- x64 and ARM64 build/test matrix;
- any Windows-specific verification considerations.

Active document: `windows/conformance/verification-plan.md`.

### 4. Diagnostics Integration

Document how diagnostics are represented and validated in the Windows core:

- diagnostic envelope type;
- schema and taxonomy conformance;
- chain integrity;
- runtime validation behavior.

Active document: `windows/conformance/diagnostics-conformance-plan.md`.

### 5. Materialization Boundary

Document how the planner/emitter boundary is respected:

- where planning occurs;
- where descriptor emission occurs;
- how the boundary is enforced by interfaces and tests.

Active document: `windows/conformance/materialization-boundary-plan.md`.

### 6. Packaging / Build / Deployment Decisions

Document Windows-specific decisions:

- MSBuild solution and project layout;
- MSVC settings;
- PowerShell wrappers;
- installer or package format;
- signing and verification;
- update and configuration strategy.

Active document: `windows/docs/build-run-instructions.md`.

### 7. Performance / Resource Constraints

Document Windows-specific constraints:

- memory behavior;
- startup time;
- responsiveness requirements after a UI exists;
- storage behavior;
- offline operation requirements if applicable.

Active document: `windows/docs/architecture.md`.

### 8. Caveat / Deviation Tracking

Maintain a deviation log for Windows limitations and release blockers.

Active document: `windows/conformance/deviation-log.md`.

### 9. Proof-Of-Conformance Deliverables

Produce the deliverables listed in `common-downstream-handoff-requirements.md` before the Windows product can move beyond `NOT_SHIPPABLE`.
