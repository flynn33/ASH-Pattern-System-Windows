# ASH Pattern System Windows SDK

This versioned Release SDK contains the native C++20 `ash-core` public library, curated headers, MSBuild integration, and an independent console consumer example. Read `RELEASE-MANIFEST.json` for the exact product version, source commit, canonical authority, toolchain, architecture, and qualification status.

**Use the named SDK release assets. GitHub's automatically generated Source code ZIP/tarball is not the SDK. Do not import this repository wholesale into an application.**

Extract into a directory you own. Verify all `checksums.sha256` entries before use. Build the sample under `samples/StandaloneConsumer` with Microsoft v143 in Release `/MT`, matching the SDK architecture. Debug, `/MD`, debug iterator settings, and other architectures are rejected. See `docs/sdk-integration.md` for the native project imports and lifecycle procedures.

The ZIP is unsigned. SHA256 checksums verify matching bytes; they are not a digital signature. Compiler `/Z7` symbols are embedded in static objects; no separate linker PDB is promised. Canonical specifications, tests, source implementations, and governance are not runtime dependencies or SDK payload.

Product ownership and layouts changed during this prerelease. Clean rebuild and relink consumers; see `docs/sdk-migration.md`. Complete diagnostic records and typed containment/halt projections preserve causal evidence; UTC engine entry/adoption times are outside semantic identity. Independent imported histories are retained for separate audit export. Support diagnostics are caller-owned and opt-in; the public Release library compiles only minimal essential collection, with actual rich collector code confined to Debug source. The SDK is a library, with descriptor emission rather than a filesystem project generator. Physical hardware, deployment-floor checks and unexecuted ARM64 runtime qualification are separate owner-phase limitations described in the release evidence.

Explicit generation requests retain complete topology, node roles and all five explanatory axiom results. Separate inputs carry target provenance and optional measured subject evidence; unavailable measurements remain `INDETERMINATE`. The native `aps-windows-plan-2` format binds all request/evidence/registry fields and requires old plan hashes to be regenerated. The emitter audits the stored plan before copying every declared descriptor field. Supported native references and legacy defaults are documented in `docs/sdk-integration.md`.

License: the included original `LICENSE` applies. `SBOM.json` is a product-owned component inventory, with explicit file coverage; it does not claim SPDX/CycloneDX certification.
