# SDK installation and replacement

**Use the named SDK release assets. GitHub's automatically generated Source code ZIP/tarball is not the SDK. Do not import this repository wholesale into an application.**

Choose the named x64 or ARM64 Release ZIP only after its mandatory release qualification passes.

Verify downloaded assets against external SHA256SUMS.txt, extract into a selected empty directory, and verify every internal checksum. Each SDK has one versioned root with include/ash, lib, msbuild, samples, docs, license, SBOM and manifest. No global registration or administrator installation is required.

The intended SDK `1.0.0-rc.1` release has separate `ash-pattern-system-windows-sdk-1.0.0-rc.1-x64-release.zip` and `ash-pattern-system-windows-sdk-1.0.0-rc.1-arm64-release.zip` assets, accompanied by `qualification.json`, `release-notes.md`, `release-index.json` and `SHA256SUMS.txt`. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes. The selected asset's qualification names its exact source, authority/profile bindings and bytes.

Use a separate directory for upgrade, update explicit SDK project references and rebuild/relink all consumers. Repair by fresh verified extraction. Remove project references before deleting only that SDK directory; preserve application source and unrelated files.

See [Integration](Integration), [Compatibility](Compatibility) and [Releases](Releases). Unsigned status is explicit; hashes establish integrity, not publisher signatures.
