# Target Repository Shape

## Windows Edition Structure

```text
ash-pattern-system-windows/
├── README.md
├── docs/
│   ├── 00-repository-purpose.md
│   ├── 01-design-philosophy.md
│   ├── 02-target-repository-shape.md
│   └── 03-design-roadmap.md
├── specs/
│   ├── core/
│   ├── algorithms/
│   ├── interfaces/
│   ├── registries/
│   └── verification/
├── windows/
│   ├── README.md
│   ├── CANONICAL-DEFERENCE.md
│   ├── build/
│   │   ├── ash-windows.sln
│   │   ├── *.vcxproj
│   │   └── build.ps1
│   ├── src/
│   │   ├── include/ash/
│   │   └── *.cpp
│   ├── tests/
│   │   ├── test_runner.hpp
│   │   ├── *.test.cpp
│   │   └── run_all.ps1
│   ├── conformance/
│   │   ├── acceptance-judgment.md
│   │   ├── diagnostics-conformance-plan.md
│   │   ├── deviation-log.md
│   │   ├── materialization-boundary-plan.md
│   │   ├── module-mapping.md
│   │   └── verification-plan.md
│   └── docs/
│       ├── architecture.md
│       └── build-run-instructions.md
└── completion-evidence/
```

## Structural Rules

### `docs/`

Contains repository-level Windows Edition documentation: purpose, design philosophy, repository shape, and roadmap.

### `specs/`

Contains APS semantic contract material bundled with this repository for implementation reference. These files describe the behavior the Windows implementation must preserve.

### `windows/build/`

Contains the Visual Studio solution, MSBuild project files, and PowerShell build wrapper.

### `windows/src/`

Contains native C++20 implementation code. Public headers live under `windows/src/include/ash/`; module implementations live beside them as `.cpp` files.

### `windows/tests/`

Contains the C++ test harness, module tests, and PowerShell test runner.

### `windows/conformance/`

Contains Windows conformance documentation and current release-gate status for semantic modules, diagnostics, materialization, deviations, verification, and acceptance.

### `windows/docs/`

Contains Windows architecture and build/run documentation.

### `completion-evidence/`

Contains audit and release-readiness evidence for the current Windows product state.

## Exclusions From This Repository Shape

The Windows edition should not add unrelated platform implementations, non-Windows release packaging, third-party runtime frameworks, or product claims that cannot be verified by Windows evidence.
