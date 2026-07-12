# Packaging and Signing Baseline

Date: 2026-06-21

## Current Status

`NOT_SHIPPABLE`

## Evidence

- Native Visual Studio project metadata exists under `windows/build`.
- Local host does not provide `msbuild`, `pwsh`, or `powershell`.
- No production package, installer, signing envelope, checksum manifest, clean install, upgrade, or uninstall evidence was produced in this pass.

## Required Closure

1. Run the Visual Studio/MSBuild build matrix on Windows.
2. Run the repository PowerShell build/test entry point.
3. Build production package artifacts.
4. Sign artifacts with the owner-approved signing path.
5. Verify signatures.
6. Perform clean install, exercise, upgrade, and uninstall checks.
7. Create checksums, SBOM, release manifest, and release-readiness evidence.
