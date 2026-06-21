# Semantic Contract Boundary

This file records how the Windows edition separates APS semantic requirements from Windows-specific implementation decisions.

## Boundary Statement

The Windows edition implements the APS semantic contract in native C++20. If a Windows-specific choice conflicts with the APS semantic contract files bundled in this repository, the platform choice must change or the conflict must remain an unresolved release blocker.

Windows implementation details may document how behavior is built, tested, packaged, or released on Windows. They may not redefine APS state semantics, recovery behavior, diagnostic requirements, module responsibilities, or acceptance rules.

## What The Windows Edition May Do

- Implement the nine APS semantic modules for Windows desktop C++.
- Contain Windows-specific source code, MSBuild project files, PowerShell scripts, tests, and implementation documentation.
- Record platform decisions such as integer encoding, realm identity string format, CRT linkage, compiler flags, and test harness design.
- Track unresolved platform limitations in `windows/conformance/deviation-log.md`.
- Preserve APS behavior while adapting implementation mechanics to Windows-native tooling.

## What The Windows Edition May Not Do

- Redefine, override, extend, or restrict APS semantics.
- Reintroduce the superseded 8+1 model as current behavior.
- Weaken contracts, diagnostics, registries, invariants, or conformance requirements.
- Treat Windows build, packaging, UI, or release decisions as semantic requirements for other platforms.
- Mark the Windows product shippable without the required Windows release evidence.

## Source And Test Discipline

C++ headers under `windows/src/include/ash/` describe the APS contract each module implements. Tests verify behavior of the Windows implementation, including deterministic state behavior, transition semantics, diagnostic chain integrity, recovery/fallback behavior, topology generation, axiom evaluation, generation planning, and artifact descriptor emission.

Where a Windows-specific decision exists, the relevant conformance document records:

1. the platform choice;
2. why the choice is local to the Windows edition;
3. how the choice would be changed if later release requirements require it.

## Conflict Resolution

If a proposed Windows change conflicts with the APS semantic contract:

1. Stop the change.
2. Record the conflict as a blocker or deviation.
3. Resolve the semantic ambiguity before shipping the Windows product.
4. Do not silently adapt Windows code or documentation in a way that changes APS behavior.

## Reference Documents

- `specs/core/`
- `specs/algorithms/`
- `specs/interfaces/`
- `specs/verification/`
- `windows/conformance/deviation-log.md`
- `windows/conformance/acceptance-judgment.md`
