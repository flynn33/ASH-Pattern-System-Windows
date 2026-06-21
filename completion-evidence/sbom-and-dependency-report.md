# SBOM and Dependency Report

Date: 2026-06-21

## Current Dependency State

- No third-party runtime dependency was added.
- The Windows core uses C++20 standard-library facilities and Visual Studio/MSBuild project metadata.
- The local host does not provide MSBuild or the Windows native package toolchain, so a production SBOM was not generated.

## Blocking Requirements

Before release:

- Generate an SBOM from the production Windows build.
- Record compiler, SDK, toolchain, and package versions.
- Verify license and notice obligations.
- Record checksums for release artifacts.
- Verify dependency provenance for the build environment.

## Judgment

`NOT_SHIPPABLE`
