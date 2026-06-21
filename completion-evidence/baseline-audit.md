# Baseline Audit

Date: 2026-06-21

## Repository and Branch

- Repository: `https://github.com/flynn33/ASH-Pattern-System-Windows.git`
- Selected platform base: `origin/main` at `b832f6e9b76a8c08038ba9dfdaf76f64691405e5`
- Completion branch: `release/aps-windows-1.0.0-completion-clean`
- Canonical reference: `upstream/main` at `cc253f3d137a27f0eeb471bed62bbdb939e3b6d1`
- Merge base: `631047989227fbf938d45ed73c5287f292591904`
- Shallow clone: false

## Build Graph

- Native Windows solution: `windows/build/ash-windows.sln`
- Core project: `windows/build/ash-core.vcxproj`
- Test projects: state model, transition registry, diagnostics, recovery engine, completion modules
- Scripted Windows build entry: `windows/build/build.ps1`

## Architecture

- The Windows product is a native C++ core with Visual Studio project metadata.
- The Windows implementation keeps APS domain logic in module headers and sources under `windows/src` and `windows/src/include`.
- Tests are under `windows/tests` and use the repository test runner.
- No new dependency was added.

## Product Status

Current status is `NOT_SHIPPABLE`.

Completed in this pass:
- Removed branch-local temporary behavior from the remaining completion modules.
- Implemented realm encoding, topology generation, axiom evaluation, generation planning, and artifact emission.
- Corrected canonical b0 ordering in Windows state conversion.
- Added completion-module tests and solution integration.
- Updated Windows conformance documentation to reflect current status and remaining blockers.

Remaining blockers:
- Native MSBuild/PowerShell test path was not executable on this host.
- Canonical upstream corpus is not yet vendored into or executed by the Windows branch.
- Signed package, clean install, upgrade, uninstall, SBOM, checksums, and release manifest evidence are not complete.

## Protected Surface

- Product-mode protected-surface verification passed against `origin/main`.
- The package baseline write/verify round trip was run with `/tmp/aps-windows-protected-surface-baseline.json`.
- A repository-local protected baseline file was not added in this pass because the package baseline records protected governance path names that are not appropriate for new product artifacts.
