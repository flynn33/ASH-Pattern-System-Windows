# Design Philosophy

## Primary Principle

The Windows edition implements APS semantics with native Windows tooling while keeping platform decisions explicit, testable, and auditable.

## Governing Ideas

### 1. Semantics Before Platform Mechanics

Windows build files, compiler flags, and packaging decisions must support the APS state model and recovery behavior. They must not change what the semantic modules mean.

### 2. Native Windows Tooling

Production implementation work uses C++20, MSVC, MSBuild, Visual Studio project files, and PowerShell. Third-party runtime dependencies are not part of the current Windows core.

### 3. Determinism Matters

Equal inputs must produce equal semantic outputs for:

- state classification;
- normalization;
- realm and orbit identity;
- transition application;
- topology generation;
- axiom evaluation;
- generation planning;
- artifact descriptor emission.

### 4. Diagnostics Are Part of Behavior

The Windows core must expose diagnostics that explain:

- why a state is valid or invalid;
- why a transition is accepted or rejected;
- why recovery, fallback, containment, or safe halt occurred;
- why an axiom passed, failed, or was indeterminate;
- why an emission request was accepted or blocked.

Silent correction and silent failure are not acceptable behavior.

### 5. Planning Before Materialization

`GenerationPlanner` produces an inspectable in-memory plan before any output is materialized. `ArtifactEmitter` consumes that plan and produces traceable descriptors. The current semantic core does not write files.

### 6. Fail Closed On Missing Release Evidence

If packaging, signing, installation, accessibility, security, migration, or platform build evidence is missing, the Windows edition remains `NOT_SHIPPABLE`.

### 7. Platform Decisions Stay Local

Windows-specific choices, such as MSVC flags, CRT linkage, integer encoding, and PowerShell wrappers, are documented as implementation decisions for this edition. They do not redefine APS semantics.

## Design Test

A design decision is aligned only if it preserves APS behavior, fits the Windows-native architecture, and can be verified through code, tests, or release evidence.
